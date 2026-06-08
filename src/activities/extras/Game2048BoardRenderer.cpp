#include "Game2048BoardRenderer.h"

#include <algorithm>
#include <string>

#include "GfxRenderer.h"
#include "fontIds.h"

namespace Game2048BoardRenderer {

namespace {

void drawTile(GfxRenderer& renderer, int value, int x, int y, int size) {
  renderer.drawRect(x, y, size, size, true);
  if (value <= 0) return;

  const std::string text = std::to_string(value);
  const int fontId = (text.size() >= 4) ? SMALL_FONT_ID : UI_12_FONT_ID;
  const int textWidth = renderer.getTextWidth(fontId, text.c_str());
  const int textHeight = renderer.getTextHeight(fontId);
  renderer.drawText(fontId, x + (size - textWidth) / 2, y + (size - textHeight) / 2, text.c_str());
}

}  // namespace

void draw(GfxRenderer& renderer, const Game2048& game, int x, int y, int width, int height) {
  const int size = Game2048::kSize;
  const int cellSize = std::min(width / size, height / size);
  if (cellSize < 8) return;

  const int boardWidth = cellSize * size;
  const int boardHeight = cellSize * size;
  const int originX = x + (width - boardWidth) / 2;
  const int originY = y + (height - boardHeight) / 2;

  for (int row = 0; row < size; row++) {
    for (int col = 0; col < size; col++) {
      drawTile(renderer, game.valueAt(row, col), originX + col * cellSize, originY + row * cellSize, cellSize);
    }
  }
}

}  // namespace Game2048BoardRenderer
