#pragma once

#include <array>
#include <cstddef>

// Self-contained 2048 rules engine: a 4x4 grid of tile values (0 = empty,
// otherwise a power of two), sliding/merging moves, scoring, and win/loss
// detection. No rendering or storage dependencies, mirroring SokobanGame so
// it stays easy to unit-test and to remove along with the rest of
// CPR_ENABLE_EXTRA_ACTIVITIES if this feature is ever dropped.
class Game2048 {
 public:
  static constexpr int kSize = 4;

  // Clears the board, resets the score, and spawns the two starting tiles.
  void reset();

  // Slides every tile toward (dr, dc) in {-1, 0, 1} (exactly one of dr/dc
  // must be non-zero), merging equal adjacent tiles at most once per move.
  // Returns true if the board actually changed, in which case a new tile is
  // spawned and the score is updated.
  bool move(int dr, int dc);

  // True once no empty cell remains and no adjacent tiles can merge.
  bool isGameOver() const;

  // True once any tile has reached the 2048 value.
  bool hasWon() const;

  int valueAt(int row, int col) const { return grid_[static_cast<size_t>(row) * kSize + col]; }
  int score() const { return score_; }

 private:
  int& at(int row, int col) { return grid_[static_cast<size_t>(row) * kSize + col]; }

  // Places a 2 (90% chance) or 4 (10% chance) in a random empty cell. Does
  // nothing if the board is full.
  void spawnTile();

  std::array<int, kSize * kSize> grid_{};
  int score_ = 0;
};
