#include "Track.h"
#include "SubwaySurfer.h"
#include "Player.h"
#include <TinyScreen.h>
#include "Globals.h"

extern TinyScreen display;

const int TIE_SPACING = 6;
int scrollOffset = 0;

void drawTracks() {
  uint8_t railColor = TS_8b_Gray;
  uint8_t tieColor = TS_8b_Yellow;

  for (int lane = 0; lane < LANES; lane++) {
    int laneX = lane * LANE_WIDTH; 
    int leftRail = laneX + 10;
    int rightRail = laneX + LANE_WIDTH - 10;
    display.drawLine(leftRail, 0, leftRail, SCREEN_HEIGHT, railColor);
    display.drawLine(rightRail, 0, rightRail, SCREEN_HEIGHT, railColor);
  }

  for (int y = -TIE_SPACING; y < SCREEN_HEIGHT; y += TIE_SPACING) {
    int yPos = y + scrollOffset; 
    if (yPos >= 0 && yPos < SCREEN_HEIGHT) { 
      for (int lane = 0; lane < LANES; lane++) { 
        int laneX = lane * LANE_WIDTH;
        display.drawLine(laneX + 10, yPos, laneX + LANE_WIDTH - 10, yPos, tieColor);
      }
    }
  }

  scrollOffset += SCROLL_SPEED; 
  if (scrollOffset >= TIE_SPACING) scrollOffset = 0; 
}