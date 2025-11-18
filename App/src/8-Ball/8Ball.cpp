#include "8ball.h"
#include <Wire.h>
#include <SPI.h>
#include <TinyScreen.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

extern TinyScreen display;

// =============== Colors (RGB565) ===============
const uint16_t COL_WHITE       = RGB565(255,255,255);
const uint16_t COL_RING_DIM    = RGB565(150,170,200);
const uint16_t COL_TEXT_LIGHT  = RGB565(230,234,245);
const uint16_t COL_TEXT_DARK   = RGB565(10,10,10);

// triangle fills + text shadows
const uint16_t COL_TRI_CLASSIC = RGB565(16,40,96);     // deep navy for Classic
const uint16_t COL_TRI_INVERT  = RGB565(250,220,120);  // light gold for Inverted
const uint16_t COL_SHADOW_DARK  = RGB565(10,10,14);
const uint16_t COL_SHADOW_LIGHT = RGB565(255,255,255);

// theme
Theme theme = Classic;

// =============== Tiny 5x7 font data ===============
// ASCII 32..90 subset (from 'space' to 'Z'). Each glyph 5 columns x 7 rows, MSB top.
const uint8_t FONT5x7[][5] = {
  // 32 ' ' to 47 '/'
  {0x00,0x00,0x00,0x00,0x00}, // ' '
  {0x00,0x00,0x5F,0x00,0x00}, // '!'
  {0x00,0x07,0x00,0x07,0x00}, // '"'
  {0x14,0x7F,0x14,0x7F,0x14}, // '#'
  {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
  {0x23,0x13,0x08,0x64,0x62}, // '%'
  {0x36,0x49,0x55,0x22,0x50}, // '&'
  {0x00,0x05,0x03,0x00,0x00}, // '''
  {0x00,0x1C,0x22,0x41,0x00}, // '('
  {0x00,0x41,0x22,0x1C,0x00}, // ')'
  {0x14,0x08,0x3E,0x08,0x14}, // '*'
  {0x08,0x08,0x3E,0x08,0x08}, // '+'
  {0x00,0x50,0x30,0x00,0x00}, // ','
  {0x08,0x08,0x08,0x08,0x08}, // '-'
  {0x00,0x60,0x60,0x00,0x00}, // '.'
  {0x20,0x10,0x08,0x04,0x02}, // '/'

  // 48 '0' to '9'
  {0x3E,0x51,0x49,0x45,0x3E}, // 0
  {0x00,0x42,0x7F,0x40,0x00}, // 1
  {0x42,0x61,0x51,0x49,0x46}, // 2
  {0x21,0x41,0x45,0x4B,0x31}, // 3
  {0x18,0x14,0x12,0x7F,0x10}, // 4
  {0x27,0x45,0x45,0x45,0x39}, // 5
  {0x3C,0x4A,0x49,0x49,0x30}, // 6
  {0x01,0x71,0x09,0x05,0x03}, // 7
  {0x36,0x49,0x49,0x49,0x36}, // 8
  {0x06,0x49,0x49,0x29,0x1E}, // 9

  // 58 ':' to 64 '@'
  {0x00,0x36,0x36,0x00,0x00}, // ':'
  {0x00,0x56,0x36,0x00,0x00}, // ';'
  {0x08,0x14,0x22,0x41,0x00}, // '<'
  {0x14,0x14,0x14,0x14,0x14}, // '='
  {0x00,0x41,0x22,0x14,0x08}, // '>'
  {0x02,0x01,0x51,0x09,0x06}, // '?'
  {0x32,0x49,0x79,0x41,0x3E}, // '@'

  // 65 'A' to 90 'Z'
  {0x7E,0x11,0x11,0x11,0x7E}, // A
  {0x7F,0x49,0x49,0x49,0x36}, // B
  {0x3E,0x41,0x41,0x41,0x22}, // C
  {0x7F,0x41,0x41,0x22,0x1C}, // D
  {0x7F,0x49,0x49,0x49,0x41}, // E
  {0x7F,0x09,0x09,0x09,0x01}, // F
  {0x3E,0x41,0x49,0x49,0x7A}, // G
  {0x7F,0x08,0x08,0x08,0x7F}, // H
  {0x00,0x41,0x7F,0x41,0x00}, // I
  {0x20,0x40,0x41,0x3F,0x01}, // J
  {0x7F,0x08,0x14,0x22,0x41}, // K
  {0x7F,0x40,0x40,0x40,0x40}, // L
  {0x7F,0x02,0x0C,0x02,0x7F}, // M
  {0x7F,0x04,0x08,0x10,0x7F}, // N
  {0x3E,0x41,0x41,0x41,0x3E}, // O
  {0x7F,0x09,0x09,0x09,0x06}, // P
  {0x3E,0x41,0x51,0x21,0x5E}, // Q
  {0x7F,0x09,0x19,0x29,0x46}, // R
  {0x46,0x49,0x49,0x49,0x31}, // S
  {0x01,0x01,0x7F,0x01,0x01}, // T
  {0x3F,0x40,0x40,0x40,0x3F}, // U
  {0x1F,0x20,0x40,0x20,0x1F}, // V
  {0x7F,0x20,0x18,0x20,0x7F}, // W
  {0x63,0x14,0x08,0x14,0x63}, // X
  {0x07,0x08,0x70,0x08,0x07}, // Y
  {0x61,0x51,0x49,0x45,0x43}  // Z
};


/*
* font drawing function
* loops through the 5 columns and 7 rows and draws a pixel
* if it finds a '1'
*/
void drawChar5x7(int x, int y, char character, uint16_t colour){
  // for any characters outside of font data, draw a '?' instead
  if(character < 32 || character > 90) 
  {
    character = '?';
  }
  // since font data (ASCII) starts at 32 ' ' (space), in order to get array index of lets say 'A' (ASCII 65)
  // we do 65 - 32 = 33. so FONT5x7[33] holds font data for 'A'
  const uint8_t* charData = FONT5x7[character - 32];
  // columns
  for (uint8_t columnIndex=0; columnIndex<5; columnIndex++){
    uint8_t columnPixelData = charData[columnIndex];
    // rows
    for (uint8_t rowIndex=0; rowIndex<7; rowIndex++){
      // we check if the bit for the current row is '1'.
      //
      // (1 << rowIndex) creates a mask that moves one spot up the column each loop:
      // Loop 1 (rowIndex 0): 0b00000001 (checks the 1st bit)
      // Loop 2 (rowIndex 1): 0b00000010 (checks the 2nd bit)
      // Loop 3 (rowIndex 2): 0b00000100 (checks the 3rd bit)
      // etc.
      //
      // The '&' operator checks if 'columnPixelData' has a '1' in that exact spot.
      if (columnPixelData & (1 << rowIndex))
      {
        display.drawPixel(x + columnIndex, y + rowIndex, colour);
      }
    }
  }
}

/*
* calculates width of string in pixels (used to center the text later)
*/
inline int textWidth5x7_fast(const char* textString){
  // gets number of characters in the string
  size_t charCount = strlen(textString);
  return charCount ? (int)(charCount*6 - 1) : 0; // 5 px character width + 1 px spacing, minus last gap
}

/*
* draws the actual characters of the string 
*/
void drawString5x7(int x, int y, const char* textString, uint16_t colour) {
  // set starting point to the 'x' position.
  int cursorX = x;

  // loop through every character in the textString
  // until we hit the null terminator ('\0') at the end.
  for (const char* currentChar = textString; *currentChar != '\0'; currentChar++) {

    // draw the single character that 'currentChar' is pointing to at the current cursor position.
    // *currentChar is used to get the actual character (e.g 'A').
    drawChar5x7(cursorX, y, *currentChar, colour);

    // advance the cursor 6 pixels to the right to make room
    // for the next character (5px for the char + 1px for spacing).
    cursorX += 6;
  }
}
/*
* this function draws a shadow for the text
*/
void drawString5x7_shadow(int x, int y, const char* textString, uint16_t foregroundColour, uint16_t shadowColour){
  drawString5x7(x+1, y+1, textString, shadowColour); // draw text FIRST in SHADOW colour at offset to original x.y position
  drawString5x7(x, y, textString, foregroundColour); // draw text again in FORGROUND colour at original x,y position
}

// =============== Answers ===============
const char* ANSWERS[] = {
  "IT IS CERTAIN","IT IS DECIDEDLY SO","WITHOUT A DOUBT","YES DEFINITELY",
  "YOU MAY RELY ON IT","AS I SEE IT YES","MOST LIKELY","OUTLOOK GOOD",
  "YES","SIGNS POINT TO YES","REPLY HAZY TRY AGAIN","ASK AGAIN LATER",
  "BETTER NOT TELL YOU NOW","CANNOT PREDICT NOW","CONCENTRATE AND ASK AGAIN",
  "DON'T COUNT ON IT","MY REPLY IS NO","MY SOURCES SAY NO",
  "OUTLOOK NOT SO GOOD","VERY DOUBTFUL"
};
const uint8_t NUM_ANS = sizeof(ANSWERS)/sizeof(ANSWERS[0]);

// =============== State ===============
bool shaking=false;
uint32_t shakeStart=0;
uint16_t shakeDur=800;
uint8_t ansIdx=0;

// =============== Fast primitives ===============

/*
* this function draws a circle outline using Bresenham's circle algorithm (faster than drawing with sin/cos)
*/
void ringFast(int centerx, int centery, int radius, uint16_t colour){
  int offsetx = radius;
  int offsety = 0; 
  int errorValue = 1 - radius;
  
  while (offsetx >= offsety){
    display.drawPixel(centerx + offsetx, centery + offsety, colour); // Bottom-Right
    display.drawPixel(centerx + offsety, centery + offsetx, colour); // Bottom-Right (swapped)
    display.drawPixel(centerx - offsety, centery + offsetx, colour); // Bottom-Left
    display.drawPixel(centerx - offsetx, centery + offsety, colour); // Bottom-Left (swapped)
    display.drawPixel(centerx - offsetx, centery - offsety, colour); // Top-Left
    display.drawPixel(centerx - offsety, centery - offsetx, colour); // Top-Left (swapped)
    display.drawPixel(centerx + offsety, centery - offsetx, colour); // Top-Right
    display.drawPixel(centerx + offsetx, centery - offsety, colour); // Top-Right (swapped)

    offsety++;

    if (errorValue < 0) errorValue += 2*offsety + 1;
    else {
      offsetx--; 
      errorValue += 2*(offsety - offsetx) + 1; 
      }
  }
}

/*
* helper function for fillTriangleFast
*/
static inline int calculateXOnLine(int x1,int y1,int x2,int y2,int y){
  if (y2 == y1) return x1;
  return x1 + (int)((int32_t)(x2 - x1) * (y - y1) / (y2 - y1));
}

/*
* this function draws a solid filled-in triangle
* first finds what point is at the top of the triangle
* then goes down row by row, identifying the left and right edge of the triangle for that row
* and draws a fast horizontal line across, between those two edges
*/
void fillTriangleFast(int point1_x, int point1_y, int point2_x, int point2_y, int point3_x, int point3_y, uint16_t colour){
  // sorts points by highest to lowest
  if (point1_y > point2_y) { 
    int tempY = point1_y; point1_y = point2_y; point2_y = tempY; 
    int tempX = point1_x; point1_x = point2_x; point2_x = tempX; 
  }
  if (point2_y > point3_y) { 
    int tempY = point2_y; point2_y = point3_y; point3_y = tempY; 
    int tempX = point2_x; point2_x = point3_x; point3_x = tempX; 
  }
  if (point1_y > point2_y) { 
    int tempY = point1_y; point1_y = point2_y; point2_y = tempY; 
    int tempX = point1_x; point1_x = point2_x; point2_x = tempX; 
  }

  for (int currentY = point1_y; currentY <= point2_y; ++currentY) {
    // calculate the Left and Right edges for this specific row
    int leftEdgeX = calculateXOnLine(point1_x, point1_y, point2_x, point2_y, currentY); 
    int rightEdgeX = calculateXOnLine(point1_x, point1_y, point3_x, point3_y, currentY); 
    
    // ensure left is actually smaller than right
    if (leftEdgeX > rightEdgeX) { int temp = leftEdgeX; leftEdgeX = rightEdgeX; rightEdgeX = temp; }
    
    // draw the horizontal line
    display.drawLine(leftEdgeX, currentY, rightEdgeX, currentY, colour);
  }

  for (int currentY = point2_y; currentY <= point3_y; ++currentY) {
    int leftEdgeX = calculateXOnLine(point2_x, point2_y, point3_x, point3_y, currentY);
    int rightEdgeX = calculateXOnLine(point1_x, point1_y, point3_x, point3_y, currentY); 

    if (leftEdgeX > rightEdgeX) { int temp = leftEdgeX; leftEdgeX = rightEdgeX; rightEdgeX = temp; }
    
    display.drawLine(leftEdgeX, currentY, rightEdgeX, currentY, colour);
  }
}

/*
* this function breaks the long string into smaller lines that will fit inside the screen
*/
uint8_t wrapLines(const char* sourceText, char outputBuffer[3][22], uint8_t maxLines, int maxWidth) {
  
  char textBuffer[96]; 
  strncpy(textBuffer, sourceText, sizeof(textBuffer)-1); 
  textBuffer[sizeof(textBuffer)-1]=0; 

  uint8_t lineCount = 0;
  char currentLine[64]; 
  currentLine[0] = 0; // start with an empty line

  // strtok splits the text into words (tokens) at every space " "
  char* currentWord = strtok(textBuffer, " ");

  // Loop through every word found
  while (currentWord) {
    char testLine[64];

    if (currentLine[0] == 0) {
      strncpy(testLine, currentWord, sizeof(testLine)-1);
    } else {
      snprintf(testLine, sizeof(testLine), "%s %s", currentLine, currentWord);
    }
    
    if (textWidth5x7_fast(testLine) <= maxWidth) {
      strncpy(currentLine, testLine, sizeof(currentLine)-1);
    } else {
      if (lineCount < maxLines) {
        strncpy(outputBuffer[lineCount], currentLine, 21); 
        outputBuffer[lineCount][21] = 0;
        lineCount++;
      }
      strncpy(currentLine, currentWord, sizeof(currentLine)-1); 
    }
    
    currentWord = strtok(NULL, " ");
  }

  if (currentLine[0] && lineCount < maxLines) {
    strncpy(outputBuffer[lineCount], currentLine, 21); 
    outputBuffer[lineCount][21] = 0; 
    lineCount++;
  }

  return lineCount; 
}

// =============== Shaking Animation ===============

/*
* precalculate the values of a sine wave
*/
const int8_t SIN32[32] = {
   0,  8, 16, 23, 29, 33, 36, 38, 
  39, 38, 36, 33, 29, 23, 16,  8,
   0, -8,-16,-23,-29,-33,-36,-38,
 -39,-38,-36,-33,-29,-23,-16, -8
};

/*
* just sets the stopwatch
*/
void startShake(){
  shaking = true; shakeStart = millis();
}

/*
* main function that handles the wobble animation of the ball
*/
void updateShake() {
  // tells you how long you have been shaking
  uint32_t timeElapsed = millis() - shakeStart;

  // if we have exceeded the shake duration (800ms), stop shaking and pick an answer.
  if (timeElapsed >= shakeDur) {
    shaking = false;
    
    // pick a random answer index
    ansIdx = random(NUM_ANS); 
    
    drawAnswer(); // switch to the answer screen
    return;
  }

  // cycle through the 32-step SINE32 lookup table
  // '>> 5' is a fast way to divide by 32. 
  // '& 31' is a fast way to say mod 32 (keeps the number between 0-31).s
  const uint8_t speedMultiplier = 4; 
  uint8_t phaseIndex = (uint8_t)((timeElapsed * speedMultiplier) >> 5) & 31;

  // shake starts violent (maxAmplitude) and end gently (minAmplitude)
  const int16_t maxAmplitude = 8;  
  const int16_t minAmplitude = 2;   
  
  // this reduces amplitude as time duration increases
  int16_t currentAmp = maxAmplitude - (int32_t)(maxAmplitude - minAmplitude) * timeElapsed / shakeDur;

  // Get the sine value (-40 to 40) from our table, multiply by intensity, scale down.
  // We offset 'dy' by 8 steps in the table to make it out of sync with 'dx' (circular motion).
  int offsetX = (currentAmp * SIN32[phaseIndex]) / 40;
  int offsetY = (currentAmp * SIN32[(phaseIndex + 8) & 31]) / 40;


  // to make it look like liquid, the inner '8' moves in the OPPOSITE direction
  // and slightly less distance than the outer ring.
  int innerOffsetX = -offsetX / 2;
  int innerOffsetY = -offsetY / 2;

  // clear frame
  display.clearScreen(); 

  // draw outer ring
  ringFast(48 + offsetX, 32 + offsetY, 26, COL_RING_DIM);

  // draw Inner Ring/Window (Moved opposite way)
  // determine color based on theme
  uint16_t windowColor = (theme == Classic) ? COL_TRI_CLASSIC : COL_TRI_INVERT;
  ringFast(48 + innerOffsetX, 26 + innerOffsetY, 11, windowColor);

  // Draw the '8' inside the window
  uint16_t eightColor = (theme == Classic) ? COL_TEXT_LIGHT : COL_TEXT_DARK;
  drawChar5x7(48 - 3 + innerOffsetX, 26 - 3 + innerOffsetY, '8', eightColor);
}

// -------------- Draw screens --------------
void drawIdle(){
  display.clearScreen();
  ringFast(48,32,26, COL_RING_DIM);
  ringFast(48,26,11, (theme==Classic)? RGB565(210,220,235) : RGB565(40,40,40));
  uint16_t eightCol = (theme==Classic)? COL_TEXT_LIGHT : COL_TEXT_DARK;
  drawChar5x7(48-3, 26-3, '8', eightCol);
}

void drawAnswer(){
  display.clearScreen();

  // outer ring
  ringFast(48,32,26, COL_RING_DIM);

  // triangle points
  int ax=48, ay=16;          // top
  int bx=24, by=48;          // bottom-left
  int cx=72, cy=48;          // bottom-right

  // theme-aware triangle fill and outline
  uint16_t triCol     = (theme==Classic)? COL_TRI_CLASSIC : COL_TRI_INVERT;
  uint16_t triOutline = (theme==Classic)? COL_WHITE      : COL_TEXT_DARK;

  fillTriangleFast(ax,ay,bx,by,cx,cy, triCol);
  display.drawLine(ax,ay,bx,by,triOutline);
  display.drawLine(ax,ay,cx,cy,triOutline);
  display.drawLine(bx,by,cx,cy,triOutline);

  // wrap and draw text with shadow for readability
  char lines[3][22]; for (int i=0;i<3;i++) lines[i][0]=0;
  uint8_t L = wrapLines(ANSWERS[ansIdx], lines, 3, 60);

  int blockH = L * 8; // 7 px glyph + 1 px gap
  int y0 = 32 - blockH/2;

  uint16_t textFG = (theme==Classic)? COL_TEXT_LIGHT : COL_TEXT_DARK;
  uint16_t textSH = (theme==Classic)? COL_SHADOW_DARK: COL_SHADOW_LIGHT;

  for (uint8_t i=0; i<L; ++i){
    int w = textWidth5x7_fast(lines[i]);
    int x = 48 - w/2;
    int y = y0 + i*8;
    drawString5x7_shadow(x, y, lines[i], textFG, textSH);
  }
}

// -------------- Buttons --------------
#define BTN_UL TSButtonUpperLeft
#define BTN_UR TSButtonUpperRight
#define BTN_LL TSButtonLowerLeft
#define BTN_LR TSButtonLowerRight

uint8_t read8BallButtons(){ 
  return display.getButtons(); 
  }

// -------------- Arduino --------------
void setup8Ball(TinyScreen &display){
  Wire.begin();
  display.begin();
  display.setFlip(0);
  display.setBrightness(12);   // lower avoids flicker
  randomSeed(analogRead(0));
  drawIdle();
}

void run8Ball(TinyScreen &display, bool &exitToMenu){
  uint8_t b = read8BallButtons();

  if (b & BTN_UR){           // shake
    if (!shaking){ startShake(); }
  }
  if (b & BTN_UL){           // reset
    shaking=false; drawIdle();
  }
  if (b & BTN_LR){           // Theme toggle
    theme = (theme==Classic)? Inverted : Classic;
    if (!shaking) drawIdle();
    delay(200); // debounce
  }
  
  if (b & BTN_LL){           // return to app menu
    exitToMenu = true;  // signal to exit to main menu

  }

  if (shaking) updateShake();

  delay(16); // ~60 fps pacing
}