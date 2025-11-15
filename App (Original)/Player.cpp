#include "Player.h"
#include "SubwaySurfer.h" 
#include <TinyScreen.h>
#include "Globals.h"

extern TinyScreen display;
unsigned long lastPlayerFrame = 0;
const int PLAYER_FRAME_DELAY = 250;
int playerLane = 1; // The lane the player is in. (0: Left, 1: Middle, 2: Right)
const int PLAYER_Y = 48; // Stickman's vertical position on the screen
const int PLAYER_SCALE = 2; // Scale of stickman (Increase to make stickman bigger, decrease to make smaller)

uint8_t stickman1[8] = {
  0b00100,
  0b01110,
  0b01110,
  0b00100,
  0b01110,
  0b00100,
  0b01010,
  0b10001
};

uint8_t stickman2[8] = {
  0b00100,
  0b01110,
  0b01110,
  0b00100,
  0b01010,
  0b00100,
  0b01100,
  0b10001
};

int playerFrame = 0;

void drawStickman(int x, int y, int frame) {
    uint8_t* frameData = frame == 0 ? stickman1 : stickman2;
    const int SCALE = PLAYER_SCALE;
    uint8_t stickmanColor = TS_8b_Red;

    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (frameData[row] & (1 << (4 - col))) {
                display.drawRect(x + col * SCALE, y + row * SCALE, SCALE, SCALE, stickmanColor, stickmanColor);
            }
        }
    }
}

void readPlayerButtons() {
    if (display.getButtons(TSButtonUpperLeft) && playerLane > 0) playerLane--;
    if (display.getButtons(TSButtonUpperRight) && playerLane < LANES - 1) playerLane++;
}
