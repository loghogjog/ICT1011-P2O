#include <Wire.h>
#include <SPI.h>
#include <TinyScreen.h>
#include "BMA250.h"

// Display
TinyScreen display = TinyScreen(2);
char brightness = 8;

// Accelerometer
BMA250 accel_sensor;

// Menu state
enum AppState {
  MENU,
  ROLLING
};
AppState currentState = MENU;

// Menu settings
int selectedFaces = 6;  // Default to 6-sided dice
const int MIN_FACES = 2;
const int MAX_FACES = 10;

// Dice state
int diceResult = 1;
int lastDiceResult = 1;
bool isRolling = false;
unsigned long lastShakeTime = 0;
const unsigned long SHAKE_COOLDOWN = 2000;

// Dice animation
int diceX = 32;
int diceY = 24;
int velX = 2;
int velY = 2;
const int DICE_SIZE = 24;
const int MINI_DICE_SIZE = 20;

// Border effect
bool flashLeft = false;
bool flashRight = false;
bool flashTop = false;
bool flashBottom = false;
unsigned long flashTimer = 0;
const unsigned long FLASH_DURATION = 150;

// Button state
unsigned long lastButtonPress = 0;
const unsigned long BUTTON_DEBOUNCE = 200;
const unsigned long BUTTON_HOLD_TIME = 800;
byte lastButtonState = 0;
bool buttonWasHeld = false;
unsigned long buttonPressStartTime = 0;

// Button mapping for TinyScreen (4 buttons)
#define BTN_TOP_RIGHT 4
#define BTN_TOP_LEFT 2
#define BTN_BOTTOM_RIGHT 8
#define BTN_BOTTOM_LEFT 1

// Dice color
#define DICE_COLOR TS_8b_Red 

void setup() {
  SerialUSB.begin(9600);
  Wire.begin();
  display.begin();
  display.setFlip(1);
  display.setBrightness(brightness);
  
  accel_sensor.begin(BMA250_range_2g, BMA250_update_time_32ms);
  accel_sensor.read();
  
  // Start in menu
  showMenu();
}

void loop() {
  byte currentButtonState = display.getButtons();
  
  if (currentState == MENU) {
    handleMenuInput(currentButtonState);
  } else if (currentState == ROLLING) {
    handleRollingInput(currentButtonState);
  }
  
  lastButtonState = currentButtonState;
  delay(50);
}

// ==================== MENU FUNCTIONS ====================

void showMenu() {
  display.clearScreen();
  
  // Number of faces
  display.setFont(liberationSans_16ptFontInfo);
  display.setCursor(35, 26);
  showDice(selectedFaces, 42, 27, MINI_DICE_SIZE, false);

  // Rest of display goes below cause showDice does display.clearScreen
  display.setFont(liberationSans_10ptFontInfo);
  display.fontColor(TS_8b_White, TS_8b_Black);

  // Title
  display.setCursor(15, 5);
  display.print("No. of Faces");
  
  // Left arrow
  display.setCursor(15, 30);
  display.print("<");
  
  
  // Right arrow
  display.setFont(liberationSans_10ptFontInfo);
  display.setCursor(78, 30);
  display.print(">");
  
  // Bottom buttons hint
  display.setFont(liberationSans_8ptFontInfo);
  display.setCursor(5, 52);
  display.print("Back");
  display.setCursor(78, 52);
  display.print("OK");
}

void handleMenuInput(byte currentButtonState) {
  // Detect button press (only on rising edge)
  if (currentButtonState != 0 && lastButtonState == 0) {
    if (millis() - lastButtonPress > BUTTON_DEBOUNCE) {
      lastButtonPress = millis();
      
      // Top Left - Decrease faces
      if (currentButtonState & BTN_TOP_LEFT) {
        selectedFaces--;
        if (selectedFaces < MIN_FACES) {
          selectedFaces = MAX_FACES;
        }
        showMenu();
      }
      
      // Top Right - Increase faces
      else if (currentButtonState & BTN_TOP_RIGHT) {
        selectedFaces++;
        if (selectedFaces > MAX_FACES) {
          selectedFaces = MIN_FACES;
        }
        showMenu();
      }
      
      // Bottom Right - OK (start rolling mode)
      else if (currentButtonState & BTN_BOTTOM_RIGHT) {
        currentState = ROLLING;
        diceResult = random(1, selectedFaces + 1);
        setBackground();
        showDice(diceResult, (96 - DICE_SIZE) / 2, (64 - DICE_SIZE) / 2, DICE_SIZE, false);
      }
      
      // TODO: Bottom Left - return to main menu
    }
  }
}

// ==================== ROLLING FUNCTIONS ====================

void handleRollingInput(byte currentButtonState) {
  // Detect button press start
  if (currentButtonState != 0 && lastButtonState == 0) {
    buttonPressStartTime = millis();
    buttonWasHeld = false;
  }
  
  // Check if button is being held
  if (currentButtonState != 0 && !buttonWasHeld) {
    if (millis() - buttonPressStartTime > BUTTON_HOLD_TIME) {
      buttonWasHeld = true;
      showLastResult();
    }
  }
  
  // Detect button release
  if (currentButtonState == 0 && lastButtonState != 0) {
    unsigned long pressDuration = millis() - buttonPressStartTime;
    
    // Bottom Left - Back to menu
    if (lastButtonState & BTN_BOTTOM_LEFT) {
      currentState = MENU;
      showMenu();
      return;
    }
    
    // Quick press - roll dice
    if (pressDuration < BUTTON_HOLD_TIME && (millis() - lastButtonPress > BUTTON_DEBOUNCE)) {
      lastButtonPress = millis();
      if (!isRolling) {
        rollDice();
      }
    }
  }
  
  // Check for shake
  if (checkShake() && !isRolling) {
    rollDice();
  }
}

bool checkShake() {
  accel_sensor.read();
  float x = accel_sensor.X / 256.0;
  float y = accel_sensor.Y / 256.0;
  float z = accel_sensor.Z / 256.0;
  float totalAccel = sqrt(x*x + y*y + z*z);

  if (totalAccel > 2.5 && (millis() - lastShakeTime > SHAKE_COOLDOWN)) {
    lastShakeTime = millis();
    return true;
  }
  return false;
}

void setBackground() {
  display.clearScreen();
  display.drawRect(0, 0, 96, 64, TSRectangleFilled, TS_8b_Black);
  display.startData();
  delay(100);
}

void drawBorderFlash() {
  // Only draw if within flash duration
  if (millis() - flashTimer > FLASH_DURATION) {
    flashLeft = flashRight = flashTop = flashBottom = false;
    return;
  }
  
  // Draw flashing borders
  if (flashLeft) {
    display.drawLine(0, 0, 0, 64, DICE_COLOR);
    display.drawLine(1, 0, 1, 64, DICE_COLOR); //double line thickness
  }
  if (flashRight) {
    display.drawLine(95, 0, 95, 64, DICE_COLOR);
    display.drawLine(94, 0, 94, 64, DICE_COLOR);
  }
  if (flashTop) {
    display.drawLine(0, 0, 96, 0, DICE_COLOR);
    display.drawLine(0, 1, 96, 1, DICE_COLOR);
  }
  if (flashBottom) {
    display.drawLine(0, 63, 96, 63, DICE_COLOR);
    display.drawLine(0, 62, 96, 62, DICE_COLOR);
  }
}

void rollDice() {
  isRolling = true;
  
  // Save previous result
  lastDiceResult = diceResult;
  
  // Generate new result
  diceResult = random(1, selectedFaces + 1);
  
  SerialUSB.println("Rolling " + String(selectedFaces) + "-sided dice");
  SerialUSB.println("Result: " + String(diceResult));
  
  unsigned long startTime = millis();
  
  // Reset velocity
  velX = random(2, 5) * (random(0, 2) ? 1 : -1);
  velY = random(2, 5) * (random(0, 2) ? 1 : -1);
  
  byte lastAnimButtonState = display.getButtons() & ~BTN_BOTTOM_LEFT;

  // Animation loop
  while (millis() - startTime < 1500) {
    // Check for back button separately (returns to menu immediately)
    byte allButtons = display.getButtons();
    if (allButtons & BTN_BOTTOM_LEFT) {
      while(display.getButtons() != 0) {
        delay(10);
      }
      isRolling = false;
      currentState = MENU;
      showMenu();
      return;
    }
    
    // Check other buttons (for skipping animation)
    byte currentAnimButtonState = allButtons & ~BTN_BOTTOM_LEFT;
    bool buttonPressedDuringAnim = (currentAnimButtonState != 0) && (lastAnimButtonState == 0);
    
    if (buttonPressedDuringAnim) {
      SerialUSB.println("Animation skipped!");
      while(display.getButtons() != 0) {
        delay(10);
      }
      break;
    }
    
    lastAnimButtonState = currentAnimButtonState;
    
    // Update position
    diceX += velX;
    diceY += velY;
    
    // Bounce off walls and trigger flash
    if (diceX <= 0 || diceX + DICE_SIZE >= 96) {
      velX = -velX;
      diceX = constrain(diceX, 0, 96 - DICE_SIZE);
      
      // Trigger appropriate border flash
      if (diceX <= 0) {
        flashLeft = true;
      } else {
        flashRight = true;
      }
      flashTimer = millis();
    }
    if (diceY <= 0 || diceY + DICE_SIZE >= 64) {
      velY = -velY;
      diceY = constrain(diceY, 0, 64 - DICE_SIZE);
      
      // Trigger appropriate border flash
      if (diceY <= 0) {
        flashTop = true;
      } else {
        flashBottom = true;
      }
      flashTimer = millis();
    }

    // Show random numbers during animation
    int randomNum = random(1, selectedFaces + 1);
    showDice(randomNum, diceX, diceY, DICE_SIZE, false);
    delay(80);
  }

  // Show final result
  finalResult();
  isRolling = false;
}

void finalResult() {
  // Center the dice
  diceX = (96 - DICE_SIZE) / 2;
  diceY = (64 - DICE_SIZE) / 2;
  
  showDice(diceResult, diceX, diceY, DICE_SIZE, false);
}

void showLastResult() {
  display.clearScreen();
  display.setFont(liberationSans_8ptFontInfo);
  display.fontColor(TS_8b_White, TS_8b_Black);
  display.setCursor(15, 5);
  
  bool showLastResult = true;
  int centerX = (96 - DICE_SIZE) / 2;
  int centerY = (64 - DICE_SIZE) / 2 + 8;
  showDice(lastDiceResult, centerX, centerY, DICE_SIZE, showLastResult);
  
  while(display.getButtons() != 0) {
    delay(10);
  }
  
  showDice(diceResult, diceX, diceY, DICE_SIZE, false);
}

void showDice(int num, int x, int y, int dice_size, bool showLastResult) {
  display.clearScreen();
  
  // Draw border flashes if active
  drawBorderFlash();

  if (showLastResult) {
    display.print("Last Roll:");
  }
  
  // Draw dice box
  display.drawRect(x, y, dice_size, dice_size, 1, DICE_COLOR);
  display.drawRect(x, y, dice_size, dice_size, 0, TS_8b_Black);
  
  // For dice with 6 or fewer faces, show dots
  // For more faces, show the number
  if (selectedFaces <= 6) {
    drawDots(num, x, y, dice_size);
  } else {
    drawNumber(num, x, y, dice_size);
  }
}

void drawNumber(int num, int x, int y, int dice_size) {
  if (dice_size == MINI_DICE_SIZE) {
    display.setFont(liberationSans_10ptFontInfo);
  }
  else {
    display.setFont(liberationSans_14ptFontInfo);
  }
  display.fontColor(TS_8b_Black, DICE_COLOR);
  
  // Center the number in the dice
  int textX = x + 6;
  int textY = y + 4;
  
  if (num >= 10) {
    textX = x + 2;  // Adjust for two digits
  }
  
  display.setCursor(textX, textY);
  display.print(num);
}

void drawDots(int num, int x, int y, int dice_size) {
  int dotSize = 3;
  int offset = 6;
  int center = dice_size/ 2;
  
  int left = x + offset;
  int right = x + dice_size - offset;
  int top = y + offset;
  int bottom = y + dice_size - offset;
  int centerX = x + center;
  int centerY = y + center;
  
  switch(num) {
    case 1:
      drawDot(centerX, centerY, dotSize);
      break;
      
    case 2:
      drawDot(left, top, dotSize);
      drawDot(right, bottom, dotSize);
      break;
      
    case 3:
      drawDot(left, top, dotSize);
      drawDot(centerX, centerY, dotSize);
      drawDot(right, bottom, dotSize);
      break;
      
    case 4:
      drawDot(left, top, dotSize);
      drawDot(right, top, dotSize);
      drawDot(left, bottom, dotSize);
      drawDot(right, bottom, dotSize);
      break;
      
    case 5:
      drawDot(left, top, dotSize);
      drawDot(right, top, dotSize);
      drawDot(centerX, centerY, dotSize);
      drawDot(left, bottom, dotSize);
      drawDot(right, bottom, dotSize);
      break;
      
    case 6:
      drawDot(left, top, dotSize);
      drawDot(left, centerY, dotSize);
      drawDot(left, bottom, dotSize);
      drawDot(right, top, dotSize);
      drawDot(right, centerY, dotSize);
      drawDot(right, bottom, dotSize);
      break;
  }
}

void drawDot(int x, int y, int size) {
  display.drawRect(x - size/2, y - size/2, size, size, 1, TS_8b_Black);
}
