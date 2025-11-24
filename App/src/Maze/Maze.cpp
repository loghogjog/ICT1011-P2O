#include "Maze.h"
#include <Wire.h>
#include <SPI.h>
#include <TinyScreen.h>
#include <BMA250.h>

extern TinyScreen display;
BMA250 accel;

// BMA250 Accelerometer I2C addresses
#define BMA250_I2C_ADDRESS 0x19 // address used
#define BMA250_ALT_ADDRESS 0x18

MazeGameState currentState = MAZE_MENU;
int menuSelection = 0;
const int NUM_MENU_ITEMS = 3; // 3 levels

// Button debouncing
unsigned long mazeLastButtonPress  = 0;
const int DEBOUNCE_DELAY = 150;

// Ball physics
float ballX = 10.0;
float ballY = 10.0;
float ballVelX = 0.0;
float ballVelY = 0.0;
float prevX = 10.0;  // remember last ball X
float prevY = 10.0;  // remember last ball y
const float BALL_RADIUS = 1.0;
const float FRICTION = 0.90;
const float ACCEL_SCALE = 0.05;
const float MAX_VELOCITY = 1.5;

// Timer
unsigned long levelStartTime;
unsigned long lastFrameTime = 0;
int timeLimit; // seconds
int timeRemaining;

// Maze data - 1 = wall, 0 = path, 2 = exit
const int MAZE_WIDTH = 24;
const int MAZE_HEIGHT = 16;
const int CELL_SIZE = 4;

// Easy Level Maze
const uint8_t mazeEasy[MAZE_HEIGHT][MAZE_WIDTH] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Medium Level Maze
const uint8_t mazeMedium[MAZE_HEIGHT][MAZE_WIDTH] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,1},
  {1,0,1,0,1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,1,0,1},
  {1,0,1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,1},
  {1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,0,1,0,1},
  {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
  {1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,0,1,1,1,0,1},
  {1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,0,1},
  {1,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,1},
  {1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,0,1},
  {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,2,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Hard Level Maze
const uint8_t mazeHard[MAZE_HEIGHT][MAZE_WIDTH] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,0,1},
  {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,0,1},
  {1,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,1,0,1},
  {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,0,1,0,1},
  {1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
  {1,1,1,0,1,0,1,1,1,1,1,0,1,1,1,1,1,1,0,1,0,1,1,1},
  {1,0,0,0,1,0,1,0,0,0,1,0,0,0,0,0,0,1,0,1,0,0,0,1},
  {1,0,1,1,1,0,1,0,1,0,1,1,1,1,1,1,0,1,0,1,1,1,0,1},
  {1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,1,0,1},
  {1,0,1,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1},
  {1,0,1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,1},
  {1,0,1,1,1,0,1,0,1,1,1,1,0,1,1,1,0,1,0,1,1,1,0,1},
  {1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1},
  {1,1,1,0,1,1,1,1,1,0,1,1,1,1,0,1,1,1,1,1,0,1,2,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Current maze pointer
const uint8_t* currentMaze;

// =============================================================================
// ACCELEROMETER FUNCTIONS
// =============================================================================
void writeAccelRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(BMA250_I2C_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void readAccelData(int16_t &x, int16_t &y, int16_t &z) {
  Wire.beginTransmission(BMA250_I2C_ADDRESS);
  Wire.write(0x02); // X-axis LSB register
  Wire.endTransmission();
  
  Wire.requestFrom(BMA250_I2C_ADDRESS, 6);
  
  if(Wire.available() >= 6) {
    uint8_t xLSB = Wire.read();
    uint8_t xMSB = Wire.read();
    uint8_t yLSB = Wire.read();
    uint8_t yMSB = Wire.read();
    uint8_t zLSB = Wire.read();
    uint8_t zMSB = Wire.read();
    
    // Combine bytes (test left-aligned bits 15:6 )
    x = (int16_t)((xMSB << 8) | xLSB) >> 8;
    y = (int16_t)((yMSB << 8) | yLSB) >> 6;
    z = (int16_t)((zMSB << 8) | zLSB) >> 6;
  } else {
    x = -1;
    y = -1;
    z = -1;
  }
}

void initAccelerometer() {
  // Try to detect accelerometer at primary address
  Wire.beginTransmission(BMA250_I2C_ADDRESS);
  byte error = Wire.endTransmission();
  
  delay(10);
  
  // Soft reset
  writeAccelRegister(0x14, 0xB6);
  delay(10);
  
  // Set range to ±2g
  writeAccelRegister(0x0F, 0x03);
  
  // Set bandwidth to 125Hz
  writeAccelRegister(0x10, 0x0C);
  
  // Set power mode to normal
  writeAccelRegister(0x11, 0x00);
  
  delay(10);
}

// =============================================================================
// SETUP
// =============================================================================
void setupMaze(TinyScreen &display) {
  Wire.begin();
  display.begin();
  display.setBrightness(10);
  
  // Initialize accelerometer
  accel.begin(BMA250_range_2g, BMA250_update_time_64ms);
  delay(50);
  
  randomSeed(analogRead(0));
}

// =============================================================================
// MAIN LOOP
// =============================================================================
void runMaze(TinyScreen &display, bool &exitToMenu) {
  
  accel.read();

  switch (currentState) {
    case MAZE_MENU:
      runMenu(exitToMenu);
      break;
    case PLAYING:
      runGame();
      break;
    case LEVEL_COMPLETE:
      showLevelComplete();
      break;
    case GAME_OVER:
      showGameOver();
      break;
  }

}

// =============================================================================
// MENU SYSTEM
// =============================================================================
void runMenu(bool &exitToMenu) {
  display.clearScreen();
  display.setFont(thinPixel7_10ptFontInfo);
  display.fontColor(TS_8b_White, TS_8b_Black);
  
  // Title
  display.setCursor(20, 2);
  display.print("TILT MAZE");
  
  // Menu options
  const char* menuItems[] = {"EASY", "MEDIUM", "HARD"};
  
  for(int i = 0; i < NUM_MENU_ITEMS; i++) {
    if(i == menuSelection) {
      display.fontColor(TS_8b_Black, TS_8b_Green);
      display.setCursor(5, 16 + i*11);
      display.print(">           <");
    }
    display.fontColor(i == menuSelection ? TS_8b_Black : TS_8b_White, 
                      i == menuSelection ? TS_8b_Green : TS_8b_Black);
    display.setCursor(12, 16 + i*11);
    display.print(i+1);
    display.print(". ");
    display.print(menuItems[i]);
  }
  
  // Handle input
  if(millis() - mazeLastButtonPress  > DEBOUNCE_DELAY) {
    if(display.getButtons(TSButtonUpperLeft)) { // Moves selection up
      menuSelection = (menuSelection - 1 + NUM_MENU_ITEMS) % NUM_MENU_ITEMS;
      mazeLastButtonPress  = millis();
    }
    if(display.getButtons(TSButtonUpperRight)) { // Moves selection down
      menuSelection = (menuSelection + 1) % NUM_MENU_ITEMS;
      mazeLastButtonPress  = millis();
    }
    if(display.getButtons(TSButtonLowerRight)) {
      if(menuSelection < 3) {
        startLevel(menuSelection);
      }
      mazeLastButtonPress  = millis();
    }
    if(display.getButtons(TSButtonLowerLeft)) {
        exitToMenu = true;  // signal to exit to main menu
        mazeLastButtonPress  = millis();  // debounce
    }
  }
  
  delay(50);
}

// =============================================================================
// GAME LOGIC
// =============================================================================
void startLevel(int level) {
  currentState = PLAYING;
  
  // Set maze and time limit based on level
  switch(level) {
    case 0: // Easy
      currentMaze = (const uint8_t*)mazeEasy;
      timeLimit = 60;
      break;
    case 1: // Medium
      currentMaze = (const uint8_t*)mazeMedium;
      timeLimit = 45;
      break;
    case 2: // Hard
      currentMaze = (const uint8_t*)mazeHard;
      timeLimit = 30;
      break;
  }
  
  // Reset ball position (start position)
  ballX = 1.5 * CELL_SIZE;
  ballY = 1.5 * CELL_SIZE;
  prevX = ballX;
  prevY = ballY;
  ballVelX = 0;
  ballVelY = 0;

  // Draw maze once
  display.clearScreen();
  drawMaze();
  
  levelStartTime = millis();
  lastFrameTime = millis();
}

void runGame() {
    unsigned long currentTime = millis();

    // Back button
    if(display.getButtons(TSButtonLowerRight)) {
        delay(200);
        currentState = MAZE_MENU;
        menuSelection = 0;
        return;
    }

    if(currentTime - lastFrameTime < 33) return;
    lastFrameTime = currentTime;

    // --- Read accelerometer using working library method ---
    accel.read(); // MUST call this
    static float smoothX = 0, smoothY = 0;
    smoothX = (smoothX * 0.8f) + (accel.X * 0.2f); // smooth tilt
    smoothY = (smoothY * 0.8f) + (accel.Y * 0.2f);

    // --- Axis sign constants
    const float AXIS_SIGN_X = -1.0f; // -1.0 so that X movement is inverted
    const float AXIS_SIGN_Y = -1.0f; 

    // --- Apply acceleration ---
    ballVelX += (AXIS_SIGN_X * smoothY) * ACCEL_SCALE; // tilt X moves Y
    ballVelY -= (AXIS_SIGN_Y * smoothX) * ACCEL_SCALE; // tilt Y moves X

    // --- Friction ---
    ballVelX *= FRICTION;
    ballVelY *= FRICTION;

    // --- Limit speed ---
    ballVelX = constrain(ballVelX, -MAX_VELOCITY, MAX_VELOCITY);
    ballVelY = constrain(ballVelY, -MAX_VELOCITY, MAX_VELOCITY);

    // --- Calculate new position ---
    float newX = ballX + ballVelX;
    float newY = ballY + ballVelY;

    // --- Collision check ---
    if(!checkCollision(newX, ballY)) ballX = newX; else ballVelX = 0;
    if(!checkCollision(ballX, newY)) ballY = newY; else ballVelY = 0;

    // --- Check exit ---
    if(checkExitOverlap(ballX, ballY)) {
    currentState = LEVEL_COMPLETE;
    return;
    }

    // --- Draw ball ---
    display.drawRect((int)prevX-2, (int)prevY-2, 4, 4, TSRectangleFilled, TS_8b_Black);
    display.drawRect((int)ballX-2, (int)ballY-2, 4, 4, TSRectangleFilled, TS_8b_Yellow);
    prevX = ballX;
    prevY = ballY;

    // --- Timer ---
    timeRemaining = timeLimit - (millis() - levelStartTime) / 1000;
    
    if(timeRemaining <= 0) {
      currentState = GAME_OVER;
      return;
    }

    // draw timer
    display.setFont(thinPixel7_10ptFontInfo);
    display.fontColor(TS_8b_White, TS_8b_Black);

    char timeText[6];
    sprintf(timeText, "T:%02d", timeRemaining);

    // accomodate for display when changing from T:10 -> T:09
    int textWidth = display.getPrintWidth(timeText);
    display.drawRect(96 - 2 - 20, 0, 20, 10, TSRectangleFilled, TS_8b_Black);

    display.setCursor(96 - 2 - display.getPrintWidth(timeText), 0);
    display.print(timeText);

}



bool checkCollision(float x, float y) {
  // Check collision with walls using the ball's radius
  int checkPoints = 8;
  for(int i = 0; i < checkPoints; i++) {
    float angle = (i * 2 * PI) / checkPoints;
    float checkX = x + cos(angle) * BALL_RADIUS;
    float checkY = y + sin(angle) * BALL_RADIUS;
    
    int gridX = (int)(checkX / CELL_SIZE);
    int gridY = (int)(checkY / CELL_SIZE);
    
    if(gridX < 0 || gridX >= MAZE_WIDTH || gridY < 0 || gridY >= MAZE_HEIGHT) {
      return true; // Out of bounds
    }
    
    uint8_t cell = *((currentMaze + gridY * MAZE_WIDTH) + gridX);
    if(cell == 1) {
      return true; // Hit wall
    }
  }
  
  return false;
}

bool checkExitOverlap(float x, float y) {
  // Quick center-cell check (fast path)
  int centerGX = (int)(x / CELL_SIZE);
  int centerGY = (int)(y / CELL_SIZE);
  if(centerGX >= 0 && centerGX < MAZE_WIDTH && centerGY >= 0 && centerGY < MAZE_HEIGHT) {
    if(*((currentMaze + centerGY * MAZE_WIDTH) + centerGX) == 2) return true;
  }

  // Bounding-box scan (covers partial overlaps)
  int minGX = (int)floor((x - BALL_RADIUS) / CELL_SIZE);
  int maxGX = (int)floor((x + BALL_RADIUS) / CELL_SIZE);
  int minGY = (int)floor((y - BALL_RADIUS) / CELL_SIZE);
  int maxGY = (int)floor((y + BALL_RADIUS) / CELL_SIZE);

  // clamp to maze bounds
  if(minGX < 0) minGX = 0;
  if(minGY < 0) minGY = 0;
  if(maxGX >= MAZE_WIDTH)  maxGX = MAZE_WIDTH - 1;
  if(maxGY >= MAZE_HEIGHT) maxGY = MAZE_HEIGHT - 1;

  for(int gy = minGY; gy <= maxGY; gy++) {
    for(int gx = minGX; gx <= maxGX; gx++) {
      if(*((currentMaze + gy * MAZE_WIDTH) + gx) == 2) return true;
    }
  }

  // Perimeter sampling — detect if any point on the ball's circumference lies in an exit cell.
  const int checkPoints = 12; // more samples -> more sensitive
  for(int i = 0; i < checkPoints; i++) {
    float angle = (i * 2.0f * PI) / checkPoints;
    float checkX = x + cos(angle) * BALL_RADIUS;
    float checkY = y + sin(angle) * BALL_RADIUS;
    int gx = (int)(checkX / CELL_SIZE);
    int gy = (int)(checkY / CELL_SIZE);
    if(gx >= 0 && gx < MAZE_WIDTH && gy >= 0 && gy < MAZE_HEIGHT) {
      if(*((currentMaze + gy * MAZE_WIDTH) + gx) == 2) return true;
    }
  }

  return false;
}

void drawMaze() {
  for(int y = 0; y < MAZE_HEIGHT; y++) {
    for(int x = 0; x < MAZE_WIDTH; x++) {
      uint8_t cell = *((currentMaze + y * MAZE_WIDTH) + x);
      
      if(cell == 1) {
        // Wall
        display.drawRect(x * CELL_SIZE, y * CELL_SIZE, 
                        CELL_SIZE, CELL_SIZE, TSRectangleFilled, TS_8b_Blue);
      } else if(cell == 2) {
        // Exit
        display.drawRect(x * CELL_SIZE, y * CELL_SIZE, 
                        CELL_SIZE, CELL_SIZE, TSRectangleFilled, TS_8b_Green);
      }
    }
  }
}

// =============================================================================
// LEVEL COMPLETE SCREEN
// =============================================================================
void showLevelComplete() {
  display.clearScreen();
  display.drawRect(8, 15, 80, 35, TSRectangleFilled, TS_8b_Green);
  
  display.setFont(thinPixel7_10ptFontInfo);
  display.fontColor(TS_8b_White, TS_8b_Green);
  
  int width = display.getPrintWidth("LEVEL");
  display.setCursor(48 - width/2, 20);
  display.print("LEVEL");
  
  width = display.getPrintWidth("COMPLETE!");
  display.setCursor(48 - width/2, 30);
  display.print("COMPLETE!");
  
  width = display.getPrintWidth("Press to menu");
  display.setCursor(48 - width/2, 40);
  display.print("Press to menu");
  
  if(display.getButtons()) {
    delay(200);
    currentState = MAZE_MENU;
    menuSelection = 0;
  }
  
  delay(50);
}

// =============================================================================
// GAME OVER SCREEN
// =============================================================================
void showGameOver() {
  display.clearScreen();
  display.drawRect(8, 15, 80, 35, TSRectangleFilled, TS_8b_Red);
  
  display.setFont(thinPixel7_10ptFontInfo);
  display.fontColor(TS_8b_White, TS_8b_Red);
  
  int width = display.getPrintWidth("TIME'S UP!");
  display.setCursor(48 - width/2, 22);
  display.print("TIME'S UP!");
  
  width = display.getPrintWidth("Press to menu");
  display.setCursor(48 - width/2, 36);
  display.print("Press to menu");
  
  if(display.getButtons()) {
    delay(200);
    currentState = MAZE_MENU;
    menuSelection = 0;
  }
  
  delay(50);
}
