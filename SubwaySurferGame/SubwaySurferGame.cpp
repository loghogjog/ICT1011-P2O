#include "SubwaySurferGame.h"
#include <Wire.h>
#include <TinyScreen.h>

extern TinyScreen display;

const int SCREEN_WIDTH = 96;
const int SCREEN_HEIGHT = 64;
const int LANES = 3;
const int LANE_WIDTH = SCREEN_WIDTH / LANES;

// Track scrolling
static int scrollOffset = 0; // Track how far the ties have scrolled down
const int TIE_SPACING = 6;
const int SCROLL_SPEED = 2;

// Player animation control
static unsigned long lastPlayerFrame = 0;
const int PLAYER_FRAME_DELAY = 250; // Rate at which the stickman frames switch

// Player
static int playerLane = 1; // The lane the player is in. (0: Left, 1: Middle, 2: Right)
const int PLAYER_Y = 48; // Stickman's vertical position on the screen
const int PLAYER_SCALE = 2; // Scale of stickman (Increase to make stickman bigger, decrease to make smaller)

// Stickman frames
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

int playerFrame = 0; // Used to track which frame (0: First frame, 1: Second frame)

struct Obstacle {
  int lane; // Which lane the obstacle is in
  int y; // Vertical position
  bool active; // Used to determine if it's currently on screen. Default set to active
  int type; // 0: Sign, 1: Train
  int trainHeight; 
};

const int MAX_OBS = 5; // Number of obstacles allowed on screen
static Obstacle obstacles[MAX_OBS]; // Array to hold the obstacles appearing on screen

// Game state
static bool gameOver = false;

// Limit game updates (Reduce flickering)
static unsigned long lastFrame = 0; // Stores timestamp of last frame update
const int FRAME_DELAY = 250;

// Player movement
void readButtons() {
  if (display.getButtons(TSButtonUpperLeft) && playerLane > 0) playerLane--;
  if (display.getButtons(TSButtonUpperRight) && playerLane < LANES - 1) playerLane++;
}

// Obstacles
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

void drawStickman(int x, int y, int frame) { // Draw stickman frame based on x,y position given
    uint8_t* frameData = frame == 0 ? stickman1 : stickman2; // Choose which frame to draw
    const int SCALE = PLAYER_SCALE; // Scale the stickman drawing based on scale given
    uint8_t stickmanColor = TS_8b_Red; 

    for (int row = 0; row < 7; row++) { // Loop over each stickman frame array row
        for (int col = 0; col < 5; col++) { // Loop over the column of that row
            if (frameData[row] & (1 << (4 - col))) { // Checks if a pixel needs to be drawn
                
                display.drawRect(
                    x + (col * SCALE),       // X Start
                    y + (row * SCALE),       // Y Start
                    SCALE,                   // Width
                    SCALE,                   // Height
                    stickmanColor,           // Border Color
                    stickmanColor            // Fill Color 
                );
            }
        }
    }
}

void drawObstacle(int lane, int y, int type, int trainHeight = 25) {
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

// Draw game
void drawGame() {
  display.startData();

  display.clearScreen(); // Clear previous frame (Prevent ghost images)

  // Draw tracks
  drawTracks();

  // Draw obstacles (aligned with track width)
  for (int i = 0; i < MAX_OBS; i++) { // Loop through each obstacle
    if (obstacles[i].active) { // Only draw active obstacle
      drawObstacle(obstacles[i].lane, obstacles[i].y, obstacles[i].type,
             obstacles[i].type == 1 ? obstacles[i].trainHeight : 0);
    }
  }

  // Draw player in center of lane
  int laneCenter = playerLane * LANE_WIDTH + (LANE_WIDTH / 2); // Left edge of current lane + middle of lane
  int px = laneCenter - (5 * PLAYER_SCALE / 2); // Minus off the stickman width to make it look centered
  drawStickman(px, PLAYER_Y, playerFrame);
  display.endTransfer();
}

// Game over screen
void showGameOver() {
  display.clearScreen(); // Clear the screen so if game over, cant see the game

  // Game Over Text
  display.setFont(liberationSans_12ptFontInfo);
  char title[] = "Game Over!";
  int wTitle = display.getPrintWidth(title); // Get px width of the msg
  display.setCursor((96 - wTitle) / 2, 5); // Set the start position of the text ((96 - wTitle) / 2) centres the text, 5 for 5px down
  display.print(title);

  // Small message below - wrap into two lines to fit screen
  display.setFont(liberationSans_8ptFontInfo);
  char msg1[] = "LL: Return to menu";
  char msg2[] = "Press any";
  char msg3[] = "button to try again";

  int w1 = display.getPrintWidth(msg1);
  display.setCursor((96 - w1) / 2, 24); 
  display.print(msg1);

  int w2 = display.getPrintWidth(msg2);
  display.setCursor((96 - w2) / 2, 36); 
  display.print(msg2);
  
  int w3 = display.getPrintWidth(msg3);
  display.setCursor((96 - w3) / 2, 48);  
  display.print(msg3);

  display.startData();
  display.endTransfer();
}

void waitForButton(bool &exitToMenu) {
  while (true) {
    uint8_t buttons = display.getButtons(TSButtonUpperLeft | TSButtonUpperRight | TSButtonLowerLeft | TSButtonLowerRight);
    if (buttons & TSButtonLowerLeft) { // If user press lower left button, set exitToMenu to true
      exitToMenu = true;
      return;
    }
    if (buttons) return; // any other button restarts game
  }
}


void resetGame() {
  gameOver = false;
  playerLane = 1;
  for (int i = 0; i < MAX_OBS; i++) obstacles[i].active = false; // Set all obstacle to not active to clear the track for new game
}

void setupSubwaySurfer(TinyScreen &display) {
  Wire.begin();
  display.begin();
  display.setBrightness(15);
  display.clearScreen();
  for (int i = 0; i < MAX_OBS; i++) obstacles[i].active = false; // Make sure game starts with no obstacles first
  randomSeed(analogRead(0)); // Make sure the random() produces different result (Used when generating obstacles)
}

void runSubwaySurfer(TinyScreen &display, bool &exitToMenu) { // Runs on loop each time
    if (gameOver) { // Check if game over
      showGameOver();
      waitForButton(exitToMenu);
      if (exitToMenu) return; // Check if user decides to return to menu and exit the loop if reset
      resetGame();
      return;
    }


    if (millis() - lastFrame >= FRAME_DELAY) { // Make sure game update at fixed speed (millis() returns current timeline)
        
        lastFrame += FRAME_DELAY; // Move last frame forward by one frame interval

        // If the system fell significantly behind, reset lastFrame to catch up.
        if (millis() - lastFrame >= FRAME_DELAY) {
            lastFrame = millis();
        }
        
        readButtons();
        updateObstacles();
        checkCollision();
        drawGame();
    }

    if (millis() - lastPlayerFrame >= PLAYER_FRAME_DELAY) { // Check if its time to change stickman frame
        lastPlayerFrame += PLAYER_FRAME_DELAY;
        playerFrame = 1 - playerFrame; // switch animation frame
    }
}
