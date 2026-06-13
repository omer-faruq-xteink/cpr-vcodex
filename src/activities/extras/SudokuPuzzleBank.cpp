#include "SudokuPuzzleBank.h"

#include <HalStorage.h>
#include <Logging.h>

#include <algorithm>
#include <cctype>

namespace {

constexpr char kModule[] = "SUDO";
constexpr size_t kPuzzleLength = 81;
constexpr size_t kMaxLineLength = 256;
// How many leading lines open() will skip while looking for the first record
// that parses for the bank's format -- enough for a CSV header row or a
// handful of "#" comment lines in a line81 file.
constexpr int kMaxHeaderLines = 8;

struct FormatFolder {
  const char* folder;
  SudokuPuzzleBank::Format format;
};

constexpr FormatFolder kFormatFolders[] = {
    {"sudoku-exchange-puzzle-bank", SudokuPuzzleBank::Format::SudokuExchange},
    {"line81", SudokuPuzzleBank::Format::Line81},
    {"kaggle-csv", SudokuPuzzleBank::Format::KaggleCsv},
};

// Reads one line (without the trailing '\n'/'\r') from an open file.
// Returns false at EOF when nothing was read.
bool readLine(FsFile& file, std::string& outLine) {
  outLine.clear();
  int c = file.read();
  if (c < 0) return false;
  while (c >= 0 && c != '\n') {
    if (c != '\r' && outLine.size() < kMaxLineLength) {
      outLine.push_back(static_cast<char>(c));
    }
    c = file.read();
  }
  return true;
}

bool isPuzzleChar(char c) { return (c >= '0' && c <= '9') || c == '.' || c == '-'; }

// Sudoku-exchange / line81: the puzzle is the one whitespace-separated token
// of length 81 in the line (an id or rating field, if present, will be some
// other length). This also tolerates "#"-style comment lines, which simply
// contain no such token.
bool extractPuzzleToken(const std::string& line, std::string& outPuzzle) {
  size_t start = 0;
  while (start < line.size()) {
    while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) start++;
    size_t end = start;
    while (end < line.size() && !std::isspace(static_cast<unsigned char>(line[end]))) end++;

    if (end - start == kPuzzleLength) {
      const std::string token = line.substr(start, kPuzzleLength);
      if (std::all_of(token.begin(), token.end(), isPuzzleChar)) {
        outPuzzle = token;
        return true;
      }
    }
    start = end;
  }
  return false;
}

// Kaggle CSV: "<puzzle>,<solution>" with the solution column optional. The
// puzzle is the field before the first comma (or the whole line if there is
// no comma). A header row such as "quizzes,solutions" has no 81-char first
// field, so it simply fails to match.
bool extractPuzzleCsv(const std::string& line, std::string& outPuzzle) {
  const size_t comma = line.find(',');
  const std::string token = (comma == std::string::npos) ? line : line.substr(0, comma);
  if (token.size() != kPuzzleLength) return false;
  if (!std::all_of(token.begin(), token.end(), isPuzzleChar)) return false;
  outPuzzle = token;
  return true;
}

bool extractPuzzle(const std::string& line, SudokuPuzzleBank::Format format, std::string& outPuzzle) {
  if (format == SudokuPuzzleBank::Format::KaggleCsv) return extractPuzzleCsv(line, outPuzzle);
  return extractPuzzleToken(line, outPuzzle);
}

bool hasPuzzleExtension(const std::string& name) {
  const size_t dot = name.find_last_of('.');
  if (dot == std::string::npos) return false;
  std::string ext = name.substr(dot);
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
  return ext == ".txt" || ext == ".csv";
}

std::string stripExtension(const std::string& name) {
  const size_t dot = name.find_last_of('.');
  return (dot == std::string::npos) ? name : name.substr(0, dot);
}

}  // namespace

std::vector<SudokuPuzzleBank::Entry> SudokuPuzzleBank::listBanks(const std::string& sudokuDir) {
  std::vector<Entry> entries;

  for (const auto& folder : kFormatFolders) {
    const std::string dirPath = sudokuDir + "/" + folder.folder;

    auto dir = Storage.open(dirPath.c_str());
    if (!dir || !dir.isDirectory()) {
      if (dir) dir.close();
      continue;
    }

    dir.rewindDirectory();
    std::vector<std::string> names;
    char name[256];
    for (auto file = dir.openNextFile(); file; file = dir.openNextFile()) {
      file.getName(name, sizeof(name));
      if (!file.isDirectory() && hasPuzzleExtension(name)) {
        names.push_back(name);
      }
      file.close();
    }
    dir.close();

    std::sort(names.begin(), names.end());
    for (const auto& filename : names) {
      entries.push_back({dirPath + "/" + filename, folder.format, std::string(folder.folder) + "/" + stripExtension(filename)});
    }
  }

  return entries;
}

bool SudokuPuzzleBank::open(const std::string& path, Format format) {
  close();

  FsFile file;
  if (!Storage.openFileForRead(kModule, path, file)) {
    LOG_DBG(kModule, "Could not open puzzle bank: %s", path.c_str());
    return false;
  }

  const size_t fileSize = file.size();

  // Skip any leading header/comment lines (a CSV "quizzes,solutions" row, or
  // "#" comments in a line81 file) to find the first record that parses for
  // this format. Its byte length, including the line terminator, is then
  // assumed to be the fixed stride of every following record.
  uint32_t headerBytes = 0;
  uint32_t stride = 0;
  std::string line;
  std::string puzzle;
  for (int i = 0; i < kMaxHeaderLines; i++) {
    const size_t lineStart = file.position();
    if (!readLine(file, line)) break;
    const size_t lineEnd = file.position();
    if (extractPuzzle(line, format, puzzle)) {
      headerBytes = static_cast<uint32_t>(lineStart);
      stride = static_cast<uint32_t>(lineEnd - lineStart);
      break;
    }
  }
  file.close();

  if (stride < kPuzzleLength) {
    LOG_DBG(kModule, "No valid puzzles in bank: %s", path.c_str());
    return false;
  }

  // Whole records, plus a final line that may lack a trailing newline.
  const size_t dataBytes = fileSize - headerBytes;
  int count = static_cast<int>(dataBytes / stride);
  if (dataBytes % stride >= kPuzzleLength) count++;
  if (count <= 0) return false;

  path_ = path;
  format_ = format;
  headerBytes_ = headerBytes;
  stride_ = stride;
  puzzleCount_ = count;
  return true;
}

void SudokuPuzzleBank::close() {
  path_.clear();
  format_ = Format::SudokuExchange;
  headerBytes_ = 0;
  stride_ = 0;
  puzzleCount_ = 0;
}

bool SudokuPuzzleBank::loadPuzzle(int index, std::string& outPuzzle) const {
  outPuzzle.clear();
  if (index < 0 || index >= puzzleCount_ || stride_ == 0) return false;

  FsFile file;
  if (!Storage.openFileForRead(kModule, path_, file)) {
    LOG_DBG(kModule, "Could not reopen puzzle bank: %s", path_.c_str());
    return false;
  }

  if (!file.seekSet(headerBytes_ + static_cast<size_t>(index) * stride_)) {
    file.close();
    return false;
  }

  std::string line;
  const bool gotLine = readLine(file, line);
  file.close();

  if (!gotLine) return false;
  return extractPuzzle(line, format_, outPuzzle);
}
