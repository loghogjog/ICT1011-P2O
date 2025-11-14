#include <Wire.h>
#include <SPI.h>
#include <TinyScreen.h>

extern TinyScreen display;

// Button definitions
#define BTN_LEFT        0x02
#define BTN_RIGHT       0x04
#define BTN_BACK        TSButtonLowerLeft   // Lower-left button for exit

// Player settings
int playerX = 40;
const int playerY = 55;
const int playerWidth = 8;
const int playerHeight = 3;

// Bullet settings
#define MAX_BULLETS 5
float bulletX[MAX_BULLETS];
float bulletY[MAX_BULLETS];
bool bulletActive[MAX_BULLETS];
const float bulletSpeed = 3.0f;
unsigned long lastShotTime = 0;
const unsigned long shotInterval = 300;

// Falling block settings
#define MAX_BLOCKS 5
float blockX[MAX_BLOCKS];
float blockY[MAX_BLOCKS];
bool blockActive[MAX_BLOCKS];
const int blockSize = 6;
const float blockSpeed = 0.3f;  
unsigned long lastBlockTime = 0;
const unsigned long blockInterval = 600;

// Game states
enum GameState {WAIT_COUNTDOWN, RUNNING, GAME_OVER};
GameState gameState = WAIT_COUNTDOWN;

// Countdown screen
void countdownStart(TinyScreen &display) {
  display.setFont(thinPixel7_10ptFontInfo);

  char nums[][2] = {"3", "2", "1"};

  for (int i = 0; i < 3; i++) {
    display.clearScreen();
    int w = display.getPrintWidth(nums[i]);
    int h = display.getFontHeight();
    display.setCursor((96 - w)/2, (64 - h)/2);
    display.fontColor(TS_8b_Green, TS_8b_Black);
    display.print(nums[i]);
    delay(1000);
  }

  display.clearScreen();
  display.fontColor(TS_8b_Blue, TS_8b_Black);
  display.setCursor(30, 25);
  display.print("START!");
  delay(800);
  display.clearScreen();

  gameState = RUNNING;
}

// Reset game
void resetShootingGame(TinyScreen &display) {
  playerX = 40;
  for (int i = 0; i < MAX_BULLETS; i++) bulletActive[i] = false;
  for (int i = 0; i < MAX_BLOCKS; i++) blockActive[i] = false;
  lastShotTime = 0;
  lastBlockTime = 0;
  gameState = WAIT_COUNTDOWN;

  countdownStart(display);
}

void setupShooting(TinyScreen &display) {
  Wire.begin();
  display.setBrightness(10);
  resetShootingGame(display);
}

void runShooting(TinyScreen &display, bool &quitGame) {

  uint8_t btn = display.getButtons();

  // --- BACK BUTTON (lower-left) ---
  if (btn & BTN_BACK) {
    quitGame = true;
    return;
  }

  // Handle game over
  if (gameState == GAME_OVER) {

    display.clearScreen();
    display.setFont(thinPixel7_10ptFontInfo);
    char gameOverTxt[] = "GAME OVER";
    int w = display.getPrintWidth(gameOverTxt);
    display.setCursor((96 - w)/2, 25);
    display.fontColor(TS_8b_Red, TS_8b_Black);
    display.print(gameOverTxt);

    // Back button also returns to menu
    if (btn & BTN_BACK) {
      quitGame = true;
      return;
    }

    delay(70);
    return;
  }

  if (gameState != RUNNING) return;

  // Player movement
  if ((btn & BTN_LEFT) && playerX > 0) playerX -= 2;
  if ((btn & BTN_RIGHT) && playerX < 88) playerX += 2;

  // Auto shooting
  if (millis() - lastShotTime >= shotInterval) {
    for (int i = 0; i < MAX_BULLETS; i++) {
      if (!bulletActive[i]) {
        bulletActive[i] = true;
        bulletX[i] = playerX + playerWidth / 2;
        bulletY[i] = playerY - 2;
        lastShotTime = millis();
        break;
      }
    }
  }

  // Update bullets
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bulletActive[i]) {
      bulletY[i] -= bulletSpeed;
      if (bulletY[i] < 0) bulletActive[i] = false;

      // Collision block
      for (int j = 0; j < MAX_BLOCKS; j++) {
        if (blockActive[j] &&
            bulletX[i] >= blockX[j] &&
            bulletX[i] <= blockX[j] + blockSize &&
            bulletY[i] >= blockY[j] &&
            bulletY[i] <= blockY[j] + blockSize) {
          bulletActive[i] = false;
          blockActive[j] = false;
        }
      }
    }
  }

  // Spawn blocks
  if (millis() - lastBlockTime >= blockInterval) {
    for (int i = 0; i < MAX_BLOCKS; i++) {
      if (!blockActive[i]) {
        blockActive[i] = true;
        blockX[i] = random(0, 96 - blockSize);
        blockY[i] = 0;
        lastBlockTime = millis();
        break;
      }
    }
  }

  // Update blocks
  for (int i = 0; i < MAX_BLOCKS; i++) {
    if (blockActive[i]) {
      blockY[i] += blockSpeed;
      if (blockY[i] >= 64) gameState = GAME_OVER;
    }
  }

  // Render
  display.clearWindow(0,0,96,64);

  display.drawRect(playerX, playerY, playerWidth, playerHeight, TSRectangleFilled, 1);

  for (int i = 0; i < MAX_BULLETS; i++)
    if (bulletActive[i])
      display.drawPixel((int)bulletX[i], (int)bulletY[i], 1);

  for (int i = 0; i < MAX_BLOCKS; i++)
    if (blockActive[i])
      display.drawRect((int)blockX[i], (int)blockY[i], blockSize, blockSize, TSRectangleFilled, 1);

  delay(30);
}
