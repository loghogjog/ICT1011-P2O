#include "Obstacles.h"
#include <TinyScreen.h>
#include "SubwaySurfer.h"
#include "Player.h"
#include "Globals.h"

extern TinyScreen display;
const int MAX_OBS = 5; // Number of obstacles allowed on screen
Obstacle obstacles[MAX_OBS];

void updateObstacles() {
  for (int i = 0; i < MAX_OBS; i++) {
    if (obstacles[i].active) {
      obstacles[i].y += 5; // Move active obstacle downwards
      if (obstacles[i].y > SCREEN_HEIGHT) obstacles[i].active = false; // If go off screen, deactivate
    }
  }
  
  if (random(0, 10) > 7) { // 20% chance to spawn
    // Find all free lanes
    bool laneUsed[LANES] = {false};
    for (int i = 0; i < MAX_OBS; i++) {
        if (obstacles[i].active) laneUsed[obstacles[i].lane] = true;
    }

    // Count free lanes
    int freeCount = 0;
    int freeLanes[LANES];
    for (int lane = 0; lane < LANES; lane++) {
        if (!laneUsed[lane]) freeLanes[freeCount++] = lane;
    }

    if (freeCount > 0) {
        int lane = freeLanes[random(0, freeCount)]; // pick a random free lane

        // Find first inactive obstacle slot
        for (int i = 0; i < MAX_OBS; i++) {
            if (!obstacles[i].active) {
                obstacles[i].active = true;
                obstacles[i].lane = lane;
                obstacles[i].y = 0;
                obstacles[i].type = random(0, 2);
                if (obstacles[i].type == 1)
                    obstacles[i].trainHeight = random(15, 35);
                break;
            }
        }
    }
  }
}

// Collision detection
void checkCollision() {
  int px = playerLane * LANE_WIDTH + 2; // Make the stickman in the middle of the track
  int py = PLAYER_Y; // Vertical position on the screen
  
  // Area of player hitbox
  int pw = 5; // player width
  int ph = 7; // player height

  for (int i = 0; i < MAX_OBS; i++) { // Goes through each obstacle
    if (obstacles[i].active) { // Checks the obstacles are active
      int ox = obstacles[i].lane * LANE_WIDTH + 2; // Make sure obstacle is centre on the track
      int oy = obstacles[i].y; // Vertical position of obstacle
      int ow, oh; // Width and height of obstacle hitbox (Different depending on obstacle type)

      if (obstacles[i].type == 0) {
        // Sign
        ow = 20; 
        oh = 8;
      } else {
        // Train with variable height
        ow = 18; 
        oh = obstacles[i].trainHeight; 
      }

      // Axis-Aligned Bounding Box collision (Check if stickman & obstacle hitbox overlap)
      if (px < ox + ow && px + pw > ox && py < oy + oh && py + ph > oy) {
        gameOver = true;
      }
    }
  }
}

void drawObstacle(int lane, int y, int type, int trainHeight) {
    int laneX = lane * LANE_WIDTH; // Find start of lane

    if (type == 0) {
        // === SIGN ===
        int signWidth = 20;
        int signHeight = 8;
        int signX = laneX + (LANE_WIDTH - signWidth) / 2; // Centres the sign on the track

        // Draw the sign rectangle
        display.drawRect(signX, y, signWidth, signHeight, TS_8b_Yellow, TS_8b_Yellow);

        // Draw 3 small lights at the top of the sign
        int lightY = y + 1;
        for (int i = 0; i < 3; i++) {
            int lightX = signX + 3 + (i * 6);
            display.drawRect(lightX, lightY, 2, 2, TS_8b_White, TS_8b_White);
        }

    } else if (type == 1) {
        // === TRAIN ===
        int trainWidth = 18;
        int trainX = laneX + (LANE_WIDTH - trainWidth) / 2; // Centre the train

        // Main blue train body
        display.drawRect(trainX, y, trainWidth, trainHeight, TS_8b_Blue, TS_8b_Blue);

        // Front white rectangle at the bottom
        int frontY = y + trainHeight - 4;  // 3px tall
        display.drawRect(trainX + 3, frontY, trainWidth - 6, 3, TS_8b_White, TS_8b_White);

        // Roof line
        display.drawRect(trainX, y, trainWidth, 1, TS_8b_Gray, TS_8b_Gray);

        // Windows along the body
        for (int wy = y + 2; wy < y + trainHeight - 5; wy += 4) {
            display.drawRect(trainX + 3, wy, trainWidth - 6, 2, TS_8b_White, TS_8b_White);
        }

        // Wheels 
        display.drawRect(trainX + 2, y + trainHeight - 2, 2, 2, TS_8b_Black, TS_8b_Black);
        display.drawRect(trainX + trainWidth - 4, y + trainHeight - 2, 2, 2, TS_8b_Black, TS_8b_Black);
    }
}