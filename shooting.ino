#include <Wire.h>
#include <SPI.h>
#include <TinyScreen.h>

TinyScreen display = TinyScreen(TinyScreenPlus);

// Button definitions
#define BTN_LEFT  0x02
#define BTN_RIGHT 0x04

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

// Countdown (logic + screen)
void countdownStart() {
  display.setFont(thinPixel7_10ptFontInfo);

  char numbers[][2] = {{'3','\0'}, {'2','\0'}, {'1','\0'}};

  for (int i = 0; i < 3; i++) {
    display.clearScreen();
    int w = display.getPrintWidth(numbers[i]);
    int h = display.getFontHeight();
    display.setCursor((96 - w) / 2, (64 - h) / 2);
    display.fontColor(TS_8b_Green, TS_8b_Black);
    display.print(numbers[i]);
    delay(1000);
  }

  char startText[] = {'S','T','A','R','T','\0'};
  display.clearScreen();
  int w = display.getPrintWidth(startText);
  int h = display.getFontHeight();
  display.setCursor((96 - w)/2, (64 - h)/2);
  display.fontColor(TS_8b_Blue, TS_8b_Black);
  display.print(startText);
  delay(1000);
  display.clearScreen();

  gameState = RUNNING;
}

// Reset game
void resetGame() {
  playerX = 40;
  for (int i = 0; i < MAX_BULLETS; i++) bulletActive[i] = false;
  for (int i = 0; i < MAX_BLOCKS; i++) blockActive[i] = false;
  lastShotTime = 0;
  lastBlockTime = 0;
  gameState = WAIT_COUNTDOWN;
  countdownStart();
}

void setup() {
  Wire.begin();
  display.begin();
  display.setBrightness(10);
  resetGame();
}

void loop() {
  uint8_t buttons = display.getButtons();

  // Reset after game over with left button
  if (gameState == GAME_OVER) {
    display.clearScreen();
    display.setFont(thinPixel7_10ptFontInfo);
    char gameOverText[] = {'G','A','M','E',' ','O','V','E','R','\0'};
    int w = display.getPrintWidth(gameOverText);
    int h = display.getFontHeight();
    display.setCursor((96 - w)/2, (64 - h)/2);
    display.fontColor(TS_8b_Red, TS_8b_Black);
    display.print(gameOverText);

    if (buttons & BTN_LEFT) { // left button resets
      resetGame();
    }
    delay(50);
    return;
  }

  if (gameState != RUNNING) return;

  // Player movement
  if (buttons & BTN_LEFT && playerX > 0) playerX -= 2;
  if (buttons & BTN_RIGHT && playerX < 88) playerX += 2;

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
        blockY[i] = 0.0f;
        lastBlockTime = millis();
        break;
      }
    }
  }

  // Update blocks
  for (int i = 0; i < MAX_BLOCKS; i++) {
    if (blockActive[i]) {
      blockY[i] += blockSpeed;
      if (blockY[i] + blockSize >= 64) gameState = GAME_OVER;
    }
  }

  // Draw everything
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
