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

  // Draw static rails
  for (int lane = 0; lane < LANES; lane++) {
    int laneX = lane * LANE_WIDTH; // Get the start of the lane
    // Rails drawn in the lane with 10px marging from lane edge
    int leftRail = laneX + 10;
    int rightRail = laneX + LANE_WIDTH - 10;
    display.drawLine(leftRail, 0, leftRail, SCREEN_HEIGHT, railColor);
    display.drawLine(rightRail, 0, rightRail, SCREEN_HEIGHT, railColor);
  }

  // Draw moving ties (The horizontal lines connecting rails)
  for (int y = -TIE_SPACING; y < SCREEN_HEIGHT; y += TIE_SPACING) { // Loop ever 6px
    int yPos = y + scrollOffset; // Moves ties down based on the offset
    if (yPos >= 0 && yPos < SCREEN_HEIGHT) { // Only draw ties within the screen
      for (int lane = 0; lane < LANES; lane++) { // Inner loop to draw the tie for each lane
        int laneX = lane * LANE_WIDTH; // Get start of lane
        display.drawLine(laneX + 10, yPos, laneX + LANE_WIDTH - 10, yPos, tieColor);
      }
    }
  }

  // Scroll ties
  scrollOffset += SCROLL_SPEED; // Each frame moves the tie down by 2px (From scroll speed)
  if (scrollOffset >= TIE_SPACING) scrollOffset = 0; // Reset to 0 to prevent ties from ruining the even spacing between each other
}