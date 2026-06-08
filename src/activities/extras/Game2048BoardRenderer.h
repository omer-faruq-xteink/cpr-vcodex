#pragma once

#include "Game2048.h"

class GfxRenderer;

// Draws a Game2048 board as plain bordered cells with the tile values printed
// as text -- no icon assets, keeping the feature's flash footprint at zero
// when CPR_ENABLE_EXTRA_ACTIVITIES is off and the e-ink redraw cheap, mirroring
// SokobanBoardRenderer.
namespace Game2048BoardRenderer {

// Draws `game` centered inside the rectangle (x, y, width, height), choosing
// the largest integer cell size that fits the 4x4 grid.
void draw(GfxRenderer& renderer, const Game2048& game, int x, int y, int width, int height);

}  // namespace Game2048BoardRenderer
