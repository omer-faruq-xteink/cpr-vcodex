#pragma once

#include <string>

#include "../Activity.h"
#include "Game2048.h"

// Minimal 2048 puzzle activity. Slides and merges tiles on a 4x4 board (see
// Game2048) and draws it with plain bordered cells (see
// Game2048BoardRenderer). The only progress tracked across sessions is the
// best score reached so far. Entirely compiled out unless
// CPR_ENABLE_EXTRA_ACTIVITIES is enabled.
class Game2048Activity final : public Activity {
  static constexpr unsigned long kLongPressMs = 1000;

  Game2048 game;
  int bestScore = 0;
  bool lockLongPressConfirm = false;

  std::string transientMessage;
  unsigned long transientUntilMs = 0;

  void newGame();
  void confirmNewGame();
  void showTransientMessage(const std::string& message, unsigned long durationMs = 1500);
  void loadBestScore();
  void saveBestScore();

  void renderPlaying();

 public:
  explicit Game2048Activity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("2048", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
