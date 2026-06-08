#include "Game2048Activity.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cstdlib>

#include "Game2048BoardRenderer.h"
#include "activities/util/ConfirmationActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/HeaderDateUtils.h"

namespace {
constexpr char kModule[] = "2048";
constexpr char kBestScorePath[] = "/.crosspoint/game2048_best.txt";

// Tiny "best score" file -- a JsonDocument would be overkill for one integer,
// and this keeps the feature's footprint minimal (mirrors Sokoban's progress file).
int readBestScoreFile() {
  if (!Storage.exists(kBestScorePath)) return 0;
  const String contents = Storage.readFile(kBestScorePath);
  if (contents.isEmpty()) return 0;
  return atoi(contents.c_str());
}

void writeBestScoreFile(int bestScore) {
  Storage.mkdir("/.crosspoint");
  HalFile file;
  if (!Storage.openFileForWrite(kModule, kBestScorePath, file)) return;
  const std::string contents = std::to_string(bestScore);
  file.write(contents.c_str(), contents.size());
  file.flush();
  file.close();
}

}  // namespace

void Game2048Activity::loadBestScore() { bestScore = readBestScoreFile(); }

void Game2048Activity::saveBestScore() {
  bestScore = std::max(bestScore, game.score());
  writeBestScoreFile(bestScore);
}

void Game2048Activity::newGame() {
  game.reset();
  showTransientMessage(tr(STR_2048_NEW_GAME));
  requestUpdate();
}

void Game2048Activity::confirmNewGame() {
  startActivityForResult(
      std::make_unique<ConfirmationActivity>(renderer, mappedInput, tr(STR_2048_NEW_GAME_CONFIRM), ""),
      [this](const ActivityResult& result) {
        if (!result.isCancelled) {
          newGame();
        }
        renderer.requestNextFullRefresh();
        requestUpdate(true);
      });
}

void Game2048Activity::showTransientMessage(const std::string& message, unsigned long durationMs) {
  transientMessage = message;
  transientUntilMs = millis() + durationMs;
  requestUpdate();
}

void Game2048Activity::onEnter() {
  Activity::onEnter();
  loadBestScore();
  game.reset();
  lockLongPressConfirm = false;
  renderer.requestNextFullRefresh();
  requestUpdate(true);
}

void Game2048Activity::onExit() {
  saveBestScore();
  Activity::onExit();
}

void Game2048Activity::loop() {
  if (!transientMessage.empty() && millis() >= transientUntilMs) {
    transientMessage.clear();
    requestUpdate();
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  if (lockLongPressConfirm && mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    lockLongPressConfirm = false;
    return;
  }

  if (mappedInput.isPressed(MappedInputManager::Button::Confirm) && mappedInput.getHeldTime() >= kLongPressMs) {
    lockLongPressConfirm = true;
    confirmNewGame();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    return;
  }

  if (game.isGameOver()) return;

  int dr = 0;
  int dc = 0;
  if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    dr = -1;
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    dr = 1;
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    dc = -1;
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    dc = 1;
  } else {
    return;
  }

  const bool wasWon = game.hasWon();
  if (game.move(dr, dc)) {
    bestScore = std::max(bestScore, game.score());
    requestUpdate();
    if (!wasWon && game.hasWon()) {
      showTransientMessage(tr(STR_2048_WIN), 2500);
    } else if (game.isGameOver()) {
      showTransientMessage(tr(STR_2048_GAME_OVER), 2500);
    }
  }
}

void Game2048Activity::render(RenderLock&&) { renderPlaying(); }

void Game2048Activity::renderPlaying() {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const auto& metrics = UITheme::getInstance().getMetrics();

  const std::string subtitle = std::string(tr(STR_SCORE)) + ": " + std::to_string(game.score()) + "   " +
                               tr(STR_2048_BEST) + ": " + std::to_string(bestScore);
  HeaderDateUtils::drawHeaderWithDate(renderer, tr(STR_2048), subtitle.c_str());

  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing;
  const int sideMargin = metrics.contentSidePadding;
  const int statsY = pageHeight - metrics.buttonHintsHeight - 18;

  Game2048BoardRenderer::draw(renderer, game, sideMargin, contentTop, pageWidth - 2 * sideMargin, contentHeight);

  if (!transientMessage.empty() && millis() < transientUntilMs) {
    renderer.drawCenteredText(SMALL_FONT_ID, statsY, transientMessage.c_str());
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_2048_NEW_GAME), tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}
