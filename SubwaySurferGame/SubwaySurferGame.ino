#include <Wire.h>
#include <TinyScreen.h>
#include "SubwaySurferGame.h"

TinyScreen display = TinyScreen(TinyScreenDefault);

enum GameSelection {
  GAME_SUBWAY_SURFER,
  GAME_PLACEHOLDER1,
  GAME_PLACEHOLDER2,
  GAME_COUNT
};

int currentSelection = 0;
bool inGame = false;

void setup() {
  Wire.begin();
  display.begin();
  display.setBrightness(15);
  display.clearScreen();
}

void drawMenu() {
  display.clearScreen();
  display.setFont(liberationSans_8ptFontInfo);

  char title[] = "Game Menu";
  int w = display.getPrintWidth(title);
  display.setCursor((96 - w) / 2, 10);
  display.print(title);

  const char* games[GAME_COUNT] = {
    "Subway Surfer",
    "Coming Soon 1",
    "Coming Soon 2"
  };

  for (int i = 0; i < GAME_COUNT; i++) {
    if (i == currentSelection) {
      display.setCursor(10, 25 + i * 12);
      display.print("> ");
    } else {
      display.setCursor(10, 25 + i * 12);
      display.print("  ");
    }

    display.setCursor(25, 25 + i * 12);
    display.print(games[i]);
  }
}

void readMenuButtons() {
  uint8_t buttons = display.getButtons(TSButtonUpperLeft | TSButtonUpperRight | TSButtonLowerLeft | TSButtonLowerRight);

  if (buttons & TSButtonUpperLeft) {
    currentSelection--;
    if (currentSelection < 0) currentSelection = GAME_COUNT - 1;
  }
  if (buttons & TSButtonUpperRight) {
    currentSelection++;
    if (currentSelection >= GAME_COUNT) currentSelection = 0;
  }

  if (buttons & (TSButtonLowerLeft | TSButtonLowerRight)) {
    inGame = true;
  }
}

void loop() {
  static int lastSelection = -1; // track last menu selection

  if (!inGame) {
    readMenuButtons();

    // Only redraw if selection changed
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

      default:
        display.clearScreen();
        display.setFont(liberationSans_8ptFontInfo);
        display.setCursor(10, 30);
        display.print("Game not ready!");
        delay(1500);
        break;
    }

    inGame = false;
    lastSelection = -1; // force redraw on return
  }
}

