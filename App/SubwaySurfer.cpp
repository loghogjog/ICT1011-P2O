#include "SubwaySurfer.h"
#include <Wire.h>
#include <TinyScreen.h>
#include "Player.h"
#include "Obstacles.h"
#include "Track.h"
#include "Globals.h"

extern TinyScreen display;

unsigned long lastFrame = 0;
bool gameOver = false;

void resetGame() {
    gameOver = false;
    playerLane = 1;
    for (int i = 0; i < MAX_OBS; i++) obstacles[i].active = false;
}

void showSSGameOver() {
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
    if (buttons & TSButtonLowerLeft) { // If user press lower left button, return to main menu
      exitToMenu = true;
      return;
    }
    if (buttons) return; // any other button restarts game
  }
}

void drawGame() {
    display.startData();
    display.clearScreen();

    drawTracks(); 

    for (int i = 0; i < MAX_OBS; i++) {
        if (obstacles[i].active) {
            drawObstacle(obstacles[i].lane, obstacles[i].y, obstacles[i].type,
                         obstacles[i].type == 1 ? obstacles[i].trainHeight : 0);
        }
    }

    int laneCenter = playerLane * LANE_WIDTH + (LANE_WIDTH / 2);
    int px = laneCenter - (5 * PLAYER_SCALE / 2);
    drawStickman(px, PLAYER_Y, playerFrame);

    display.endTransfer();
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
      showSSGameOver();
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
        
        readPlayerButtons();
        updateObstacles();
        checkCollision();
        drawGame();
    }

    if (millis() - lastPlayerFrame >= PLAYER_FRAME_DELAY) { // Check if its time to change stickman frame
        lastPlayerFrame += PLAYER_FRAME_DELAY;
        playerFrame = 1 - playerFrame; // switch animation frame
    }
}
