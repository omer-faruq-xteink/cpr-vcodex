#include "Game2048.h"

#include <Arduino.h>

#include <vector>

namespace {

// Maps a position `i` along a slide line back to grid coordinates. `i == 0`
// is always the cell nearest the edge the line slides toward, so the merge
// logic below can stay direction-agnostic.
void lineCell(int dr, int dc, int line, int i, int& row, int& col) {
  if (dc != 0) {
    row = line;
    col = (dc < 0) ? i : (Game2048::kSize - 1 - i);
  } else {
    col = line;
    row = (dr < 0) ? i : (Game2048::kSize - 1 - i);
  }
}

// Compacts non-zero values toward index 0 and merges each adjacent equal
// pair once (classic 2048 slide rules), adding merged values to `gained`.
// Returns true if the resulting line differs from the input.
bool slideLine(std::array<int, Game2048::kSize>& line, int& gained) {
  std::array<int, Game2048::kSize> compact{};
  int count = 0;
  for (const int value : line) {
    if (value != 0) compact[static_cast<size_t>(count++)] = value;
  }

  std::array<int, Game2048::kSize> result{};
  int writeIndex = 0;
  for (int i = 0; i < count; i++) {
    if (i + 1 < count && compact[static_cast<size_t>(i)] == compact[static_cast<size_t>(i + 1)]) {
      const int merged = compact[static_cast<size_t>(i)] * 2;
      result[static_cast<size_t>(writeIndex++)] = merged;
      gained += merged;
      i++;
    } else {
      result[static_cast<size_t>(writeIndex++)] = compact[static_cast<size_t>(i)];
    }
  }

  const bool changed = (result != line);
  line = result;
  return changed;
}

}  // namespace

void Game2048::reset() {
  grid_.fill(0);
  score_ = 0;
  spawnTile();
  spawnTile();
}

void Game2048::spawnTile() {
  std::vector<int> emptyIndices;
  for (size_t i = 0; i < grid_.size(); i++) {
    if (grid_[i] == 0) emptyIndices.push_back(static_cast<int>(i));
  }
  if (emptyIndices.empty()) return;

  const int index = emptyIndices[static_cast<size_t>(random(static_cast<long>(emptyIndices.size())))];
  grid_[static_cast<size_t>(index)] = (random(10) == 0) ? 4 : 2;
}

bool Game2048::move(int dr, int dc) {
  if ((dr == 0) == (dc == 0)) return false;

  bool moved = false;
  int gained = 0;

  for (int line = 0; line < kSize; line++) {
    std::array<int, kSize> values{};
    for (int i = 0; i < kSize; i++) {
      int row = 0;
      int col = 0;
      lineCell(dr, dc, line, i, row, col);
      values[static_cast<size_t>(i)] = at(row, col);
    }

    const std::array<int, kSize> original = values;
    slideLine(values, gained);
    if (values == original) continue;

    moved = true;
    for (int i = 0; i < kSize; i++) {
      int row = 0;
      int col = 0;
      lineCell(dr, dc, line, i, row, col);
      at(row, col) = values[static_cast<size_t>(i)];
    }
  }

  if (moved) {
    score_ += gained;
    spawnTile();
  }
  return moved;
}

bool Game2048::isGameOver() const {
  for (int row = 0; row < kSize; row++) {
    for (int col = 0; col < kSize; col++) {
      const int value = valueAt(row, col);
      if (value == 0) return false;
      if (col + 1 < kSize && valueAt(row, col + 1) == value) return false;
      if (row + 1 < kSize && valueAt(row + 1, col) == value) return false;
    }
  }
  return true;
}

bool Game2048::hasWon() const {
  for (const int value : grid_) {
    if (value >= 2048) return true;
  }
  return false;
}
