#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Reads Sudoku puzzle banks from format-named subfolders of /sudoku, so the
// loader knows how a file is laid out from its folder name alone:
//
//   /sudoku/sudoku-exchange-puzzle-bank/*.txt|*.csv
//       "<12-char id> <81 puzzle digits> <rating>" per line (grantm's
//       sudoku-exchange-puzzle-bank layout).
//   /sudoku/line81/*.txt|*.csv
//       One 81-character puzzle per line ('0'/'.'/'-' for blanks). Leading
//       "#"-style comment lines are tolerated.
//   /sudoku/kaggle-csv/*.txt|*.csv
//       "<81-digit puzzle>,<81-digit solution>" per line (the solution
//       column is optional); a non-puzzle header row such as
//       "quizzes,solutions" is skipped automatically.
//
// In every format, the on-disk files are used exactly as downloaded -- no
// conversion or preprocessing. loadPuzzle() relies on every data record
// having the same byte width: open() measures the stride from the first
// record that parses for the bank's format (skipping any leading
// header/comment lines), so a bank can hold hundreds of thousands of puzzles
// without an in-memory offset table -- loadPuzzle(index) seeks straight to
// headerBytes + index * stride and reads a single record -- O(1) time and
// effectively zero extra RAM.
class SudokuPuzzleBank {
 public:
  enum class Format {
    SudokuExchange,
    Line81,
    KaggleCsv,
  };

  // One puzzle file found while scanning /sudoku's format subfolders.
  struct Entry {
    std::string path;
    Format format;
    std::string displayName;  // "<format-folder>/<file name without extension>", for the bank list
    std::string fileName;     // "<file name without extension>", for the in-game header
  };

  // Scans the known format subfolders of `sudokuDir` for puzzle files.
  static std::vector<Entry> listBanks(const std::string& sudokuDir);

  bool open(const std::string& path, Format format);
  void close();

  bool isOpen() const { return puzzleCount_ > 0; }
  int puzzleCount() const { return puzzleCount_; }
  const std::string& path() const { return path_; }

  // Reads puzzle `index` (0-based) into outPuzzle as an 81-character string.
  // Returns false on a bad index or a malformed record.
  bool loadPuzzle(int index, std::string& outPuzzle) const;

 private:
  std::string path_;
  Format format_ = Format::SudokuExchange;
  uint32_t headerBytes_ = 0;  // bytes to skip before the first record
  uint32_t stride_ = 0;       // bytes from the start of one record to the next
  int puzzleCount_ = 0;
};
