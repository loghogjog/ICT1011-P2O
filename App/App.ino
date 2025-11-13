#include <Wire.h>
#include <TinyScreen.h>
#include "SubwaySurfer.h"
#include "Maze.h"
#include "DecisionDice.h"
#include "8Ball.h"

TinyScreen display = TinyScreen(TinyScreenDefault);

enum GameSelection {
  GAME_SUBWAY_SURFER,
  GAME_MAZE,
  GAME_DECISION_DICE,
  GAME_8_BALL,
  GAME_COUNT
};

int currentSelection = 0;
bool inGame = false;

// Scroll offset for menu scrolling
int menuScrollOffset = 0;

// --- Helper function to draw a filled rectangle ---
void fillRect(int x, int y, int w, int h, uint16_t color) {
  for (int i = x; i < x + w; i++) {
    for (int j = y; j < y + h; j++) {
      display.drawPixel(i, j, color);
    }
  }
}

// --- Draw the game menu ---
// --- Draw the game menu ---
void drawMenu() {
  display.clearScreen();
  display.setFont(liberationSans_8ptFontInfo);

  // Centered title
  const char* title = "Game Menu";
  int titleWidth = display.getPrintWidth(const_cast<char*>(title));
  int titleHeight = display.getFontHeight();  // height of the font

  int xTitle = (128 - titleWidth) / 2;        // horizontal center
  int yTitle = 10;                            // vertical offset (can tweak)
  display.setCursor(xTitle, yTitle);
  display.print(title);

  const char* games[GAME_COUNT] = {
    "Subway Surfer",
    "Maze",
    "Decision Dice",
    "8-Ball"
  };

  const int visibleItems = 2; // number of items visible at once
  const int startY = yTitle + titleHeight + 10; // start below the title
  const int spacing = 12;

  // Adjust menuScrollOffset if selection is out of visible range
  if (currentSelection < menuScrollOffset) menuScrollOffset = currentSelection;
  if (currentSelection >= menuScrollOffset + visibleItems) menuScrollOffset = currentSelection - visibleItems + 1;

  // Draw visible menu items
  for (int i = menuScrollOffset; i < GAME_COUNT && i < menuScrollOffset + visibleItems; i++) {
    int y = startY + (i - menuScrollOffset) * spacing;

    if (i == currentSelection) {
      // Highlight selection
      fillRect(0, y, 128, spacing, TS_16b_White);
      display.fontColor(TS_16b_Black, TS_16b_White); // fg=black, bg=white
    } else {
      display.fontColor(TS_16b_White, TS_16b_Black); // normal text
    }

    display.setCursor(2, y);
    display.print(games[i]);
  }
}


// --- Read button input for menu navigation ---
void readMenuButtons() {
  static uint32_t lastPressTime = 0;
  uint32_t currentTime = millis();
  const uint32_t debounceDelay = 200; // 200ms debounce

  uint8_t buttons = display.getButtons(TSButtonUpperLeft | TSButtonUpperRight | TSButtonLowerLeft | TSButtonLowerRight);

  // Only register button presses if enough time has passed
  if (currentTime - lastPressTime > debounceDelay) {

    if (buttons & TSButtonLowerLeft) {
      currentSelection--;
      if (currentSelection < 0) currentSelection = GAME_COUNT - 1;
      lastPressTime = currentTime;
    }
    else if (buttons & TSButtonLowerRight) {
      currentSelection++;
      if (currentSelection >= GAME_COUNT) currentSelection = 0;
      lastPressTime = currentTime;
    }
    else if (buttons & (TSButtonUpperLeft | TSButtonUpperRight)) {
      inGame = true;
      lastPressTime = currentTime;
    }
  }
}


void setup() {
  Wire.begin();
  display.begin();
  display.setBrightness(15);
  display.clearScreen();
  drawMenu();
}

void loop() {
  static int lastSelection = -1;

  if (!inGame) {
    readMenuButtons();

    if (currentSelection != lastSelection) {
      drawMenu();
      lastSelection = currentSelection;
    }

    delay(100);
  } 
  else {
    bool exitToMenu = false;

    switch (currentSelection) {
      case GAME_SUBWAY_SURFER:
        setupSubwaySurfer(display);
        while (!exitToMenu) {
          runSubwaySurfer(display, exitToMenu);
        }
        break;

      case GAME_MAZE:
        setupMaze(display);
        while (!exitToMenu) {
          runMaze(display, exitToMenu);
        }
        break;
        
      case GAME_DECISION_DICE:
        setupDecisionDice(display);
        while (!exitToMenu) {
          runDecisionDice(display, exitToMenu);
        }
        break;
        
      case GAME_8_BALL:
        setup8Ball(display);
        while (!exitToMenu) {
          run8Ball(display, exitToMenu);
        }
        break;
      default:
        display.clearScreen();
        display.setFont(liberationSans_8ptFontInfo);
        display.setCursor(10, 30);
        display.print("Game not ready!");
        delay(1500);
        break;
    }

    inGame = false;
    lastSelection = -1;
    drawMenu();
  }
}