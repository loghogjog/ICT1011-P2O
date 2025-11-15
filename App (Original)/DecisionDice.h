#ifndef DICEGAME_H
#define DICEGAME_H

#include <TinyScreen.h>
#include "BMA250.h"

// External variables (globally defined in the main file)
extern TinyScreen display;
extern BMA250 accel_sensor;

extern char brightness;

enum DiceAppState {
  MENU,
  ROLLING
};

extern DiceAppState diceCurrentState;

extern int selectedFaces;
extern const int MIN_FACES;
extern const int MAX_FACES;

extern int diceResult;
extern int lastDiceResult;
extern bool isRolling;
extern unsigned long lastShakeTime;
extern const unsigned long SHAKE_COOLDOWN;

extern int diceX;
extern int diceY;
extern int velX;
extern int velY;
extern const int DICE_SIZE;
extern const int MINI_DICE_SIZE;

extern bool flashLeft;
extern bool flashRight;
extern bool flashTop;
extern bool flashBottom;
extern unsigned long flashTimer;
extern const unsigned long FLASH_DURATION;

extern unsigned long lastButtonPress;
extern const unsigned long BUTTON_DEBOUNCE;
extern const unsigned long BUTTON_HOLD_TIME;
extern byte lastButtonState;
extern bool buttonWasHeld;
extern unsigned long buttonPressStartTime;

extern const byte BTN_TOP_RIGHT;
extern const byte BTN_TOP_LEFT;
extern const byte BTN_BOTTOM_RIGHT;
extern const byte BTN_BOTTOM_LEFT;

extern const uint8_t DICE_COLOR;

// Functions prototypes
void setupDecisionDice(TinyScreen &display);
void runDecisionDice(TinyScreen &display, bool &exitToMenu);

void showMenu();
void handleMenuInput(byte currentButtonState, bool &exitToMenu);
void handleRollingInput(byte currentButtonState);

bool checkShake();
void setBackground();
void drawBorderFlash();

void rollDice();
void finalResult();
void showLastResult();

void showDice(int num, int x, int y, int dice_size, bool showLastResult);
void drawNumber(int num, int x, int y, int dice_size);
void drawDots(int num, int x, int y, int dice_size);
void drawDot(int x, int y, int size);

#endif
