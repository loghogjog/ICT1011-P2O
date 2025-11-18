#include "spinthewheel.h"
#include <Wire.h>
#include <SPI.h>
#include <TinyScreen.h>

// ======== Colors (RGB565) ========
// colour converter from standard to 565
static inline uint16_t RGB565(uint8_t red, uint8_t green, uint8_t blue){
  return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}
const uint16_t COLOUR_BACKGROUND = RGB565(10,12,20);
const uint16_t COLOUR_FRONTGROUND = RGB565(255,255,255);
const uint16_t COLOUR_HAND = RGB565(255,220,70);

// ======== Postitions ========
const uint8_t  SCREEN_WIDTH = 96, SCREEN_HEIGHT = 64;
const uint8_t  centerX = SCREEN_WIDTH/2, centerY = SCREEN_HEIGHT/2; 
const uint8_t  OUTER_RADIUS = 20, INNER_RADIUS = 12, RADIUS_LABEL = 26;

// two pi
static inline float TAU(){
  return 6.28318530718f; 
  }

// how big each slice is
static inline float stepSize(uint8_t N){
  return TAU()/N; 
  }

// finds the angle remaining by modding with 360deg/2pi
static inline float normAngle(float a){
  float t=fmodf(a,TAU()); return t<0?t+TAU():t; 
  }

// finding angle to get to the "winner" number
static inline float thetaForSectorAtTop(uint8_t k, uint8_t N){
  return -((float)k - 0.5f) * stepSize(N);
}

static inline uint8_t sectorIndexAtPointer(float theta, uint8_t N){
  float s = stepSize(N);
  float m = normAngle(-theta) / s;
  uint8_t k = (uint8_t)floorf(m) + 1;  // 1..N
  if (k > N) k = 1;
  return k;
}

// ======== Buttons ========
#define BTN_UPPER_LEFT   TSButtonUpperLeft
#define BTN_UPPER_RIGHT  TSButtonUpperRight
#define BTN_LOWER_LEFT   TSButtonLowerLeft
#define BTN_LOWER_RIGHT  TSButtonLowerRight

// ======== State ========
uint8_t  maxN=10, value=1;
bool     spinning=false, skipSpin=false;
float    wheelAngle=0.0f;
uint32_t spinStartMs=0, spinDurationMs=0;
float    spinStartAngle=0, spinTotal=0;

// record current state so that we can reset later
float    oldWheelAngle = 0.0f;
uint8_t  oldMaxN = 10;
int8_t   oldShowIdx = -1;
bool     oldWasSpinning = false; 

// ======== Tiny 3×5 digits ========
const uint8_t DIGIT[10][5] = {
  {0b111,0b101,0b101,0b101,0b111}, //0
  {0b010,0b110,0b010,0b010,0b111}, //1
  {0b111,0b001,0b111,0b100,0b111}, //2
  {0b111,0b001,0b111,0b001,0b111}, //3
  {0b101,0b101,0b111,0b001,0b001}, //4
  {0b111,0b100,0b111,0b001,0b111}, //5
  {0b111,0b100,0b111,0b101,0b111}, //6
  {0b111,0b001,0b001,0b001,0b001}, //7
  {0b111,0b101,0b111,0b101,0b111}, //8
  {0b111,0b101,0b111,0b001,0b111}  //9
};

/*
* draw the digits
*/
void drawDigit3x5(uint8_t dx, uint8_t dy, uint8_t d, uint16_t col){
  if(d>9) d=9;
  for(uint8_t y=0;y<5;y++){
    uint8_t row = DIGIT[d][y];
    // optimisation that instead of drawing three pixels one by one if its a straight line it just draws a rectangle
    if (row == 0b111) {
        display.drawRect(dx, dy + y*2, 6, 2, 1, col);
    } else {
        for(uint8_t x=0;x<3;x++){
          if(row & (0b100>>x)) display.drawRect(dx+ x*2, dy+ y*2, 2, 2, 1, col);
        }
    }
  }
}

/*
* this function draws the number in the very center of the screen 
*/
void drawNumberCentered(uint8_t n, uint16_t col){
  // single digit
  if(n<10){
    uint8_t w = 3*2, h = 5*2;
    drawDigit3x5(centerX - w/2, centerY - h/2, n, col);
  // double digit
  }else{ 
    uint8_t w = (3*2)*2 + 2;
    uint8_t x = centerX - w/2;
    drawDigit3x5(x,           centerY - 5, 1, col);
    drawDigit3x5(x + 3*2 + 2, centerY - 5, 0, col);
  }
}

// ======== Drawing helpers ========
/*
* function to draw the pointer
*/
void drawHandTop(){
  float a = -TAU()/4; // up
  int x = (int)(centerX + cosf(a)*(INNER_RADIUS-1));
  int y = (int)(centerY + sinf(a)*(INNER_RADIUS-1));
  display.drawLine(centerX, centerY-6, centerX, y, COLOUR_HAND);
  display.drawLine(x, y, x-1, y+3, COLOUR_HAND);
  display.drawLine(x, y, x+1, y+3, COLOUR_HAND);
}

/*
* function to clear screen
*/
void clearScreen(uint16_t col){
  display.drawRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, 1, col);
}

// helper function that draws the wheel 
void drawWheelState(float angle, uint8_t N, int8_t showIdx, uint16_t col, bool drawLabels) {
  int8_t ticks[20][4];
  int8_t labels[20][2];
  uint8_t labelNums[20];


  for(uint8_t i=0;i<N;i++){
    float aTick = -TAU()/4 + i*(TAU()/N) + angle;
    ticks[i][0] = (int)(centerX + cosf(aTick)*INNER_RADIUS);
    ticks[i][1] = (int)(centerY + sinf(aTick)*INNER_RADIUS);
    ticks[i][2] = (int)(centerX + cosf(aTick)*OUTER_RADIUS);
    ticks[i][3] = (int)(centerY + sinf(aTick)*OUTER_RADIUS);

    if (drawLabels) {
      float aCenter = -TAU()/4 + (i+0.5f)*(TAU()/N) + angle;
      labels[i][0] = (int)(centerX + cosf(aCenter)*RADIUS_LABEL);
      labels[i][1] = (int)(centerY + sinf(aCenter)*RADIUS_LABEL);
      labelNums[i] = i + 1;
    }
  }

  uint8_t idx = sectorIndexAtPointer(angle, N);
  uint8_t centerNum = (showIdx>0) ? (uint8_t)showIdx : idx;

  for(uint8_t i=0; i<N; i++){
     display.drawLine(ticks[i][0], ticks[i][1], ticks[i][2], ticks[i][3], col);
     
     if (drawLabels) {
       int lx = labels[i][0];
       int ly = labels[i][1];
       if(labelNums[i]<10){
         drawDigit3x5(lx-3, ly-3, labelNums[i], col);
       } else {
         drawDigit3x5(lx-5, ly-3, 1, col);
         drawDigit3x5(lx+1,  ly-3, 0, col);
       }
     }
  }
  drawNumberCentered(centerNum, col);
}

/*
* MAIN DRAWING FUNCTION
*/
void drawWheel(uint8_t N, int8_t showIdx){
  
  if (wheelAngle == oldWheelAngle && N == oldMaxN && 
      showIdx == oldShowIdx && spinning == oldWasSpinning) {
    return;
  }
  // if spinning, Hide labels. If stopped, Show labels.
  bool currentDrawLabels = !spinning;
  // erase old wheel
  bool oldDrawLabels = !oldWasSpinning;
  drawWheelState(oldWheelAngle, oldMaxN, oldShowIdx, COLOUR_BACKGROUND, oldDrawLabels);
  // draw new wheel
  drawWheelState(wheelAngle, N, showIdx, COLOUR_FRONTGROUND, currentDrawLabels); 
  // redraw the hand on top
  drawHandTop();

  // save current state
  oldWheelAngle = wheelAngle;
  oldMaxN = N;
  oldShowIdx = showIdx;
  oldWasSpinning = spinning; 
}

// ======== Spin logic ========
void beginSpin(){
  spinning = true; skipSpin = false;
  value = 1 + (uint8_t)(rand() % maxN);
  float s = stepSize(maxN);
  float margin = s * 0.08f;
  float maxOff = s*0.5f - margin;
  float offset = ((float)rand()/RAND_MAX)*2.0f*maxOff - maxOff;
  float targetTheta = thetaForSectorAtTop(value, maxN) + offset;
  spinStartAngle = wheelAngle;
  float cur = normAngle(wheelAngle);
  float desired = normAngle(targetTheta);
  float delta = desired - cur; if(delta<0) delta += TAU();
  uint8_t turns = 2 + (rand()%3);
  spinTotal = turns*TAU() + delta;
  spinDurationMs = 1500 + turns*300;
  spinStartMs = millis();
}

void updateSpin(){
  if(!spinning) return;
  if(skipSpin){ wheelAngle = normAngle(spinStartAngle + spinTotal); spinning=false; return; }
  uint32_t elapsed = millis() - spinStartMs;
  if(elapsed >= spinDurationMs){ wheelAngle = normAngle(spinStartAngle + spinTotal); spinning=false; return; }
  float t = (float)elapsed / (float)spinDurationMs;
  float ease = 1.0f - powf(1.0f - t, 3.0f);
  wheelAngle = normAngle(spinStartAngle + spinTotal * ease);
}

// ======== Buttons ========
struct BtnState { uint8_t last=0; uint32_t tUL=0, tUR=0; } btn;
uint8_t readButtons(){ return display.getButtons(); }

void handleButtons(bool &quitGame){
  uint8_t m = readButtons();
  uint32_t now = millis();
  bool ulPrev = btn.last & BTN_UPPER_LEFT;
  bool urPrev = btn.last & BTN_UPPER_RIGHT;
  bool ul = m & BTN_UPPER_LEFT;
  bool ur = m & BTN_UPPER_RIGHT;

  if(!ulPrev && ul) btn.tUL = now;
  if(ulPrev && !ul){
    int delta = (now - btn.tUL > 500) ? -5 : -1;
    if(!spinning){
      // Force redraw to update numbers (Erase old with labels, Draw new with labels)
      drawWheelState(wheelAngle, maxN, -1, COLOUR_BACKGROUND, true);
      
      int v = (int)maxN + delta;
      while(v<1) v += 10; while(v>10) v -= 10;
      maxN = (uint8_t)v;
      if(value>maxN) value=maxN;
      wheelAngle = thetaForSectorAtTop(value, maxN);
      
      oldMaxN = maxN; 
      oldWheelAngle = wheelAngle;
      oldWasSpinning = false; // Not spinning, so labels are ON
      
      drawWheelState(wheelAngle, maxN, -1, COLOUR_FRONTGROUND, true);
    }
  }
  if(!urPrev && ur) btn.tUR = now;
  if(urPrev && !ur){
    int delta = (now - btn.tUR > 500) ? +5 : +1;
    if(!spinning){
      drawWheelState(wheelAngle, maxN, -1, COLOUR_BACKGROUND, true);

      int v = (int)maxN + delta;
      while(v<1) v += 10; while(v>10) v -= 10;
      maxN = (uint8_t)v;
      if(value>maxN) value=maxN;
      wheelAngle = thetaForSectorAtTop(value, maxN);
      
      oldMaxN = maxN;
      oldWheelAngle = wheelAngle;
      oldWasSpinning = false;
      
      drawWheelState(wheelAngle, maxN, -1, COLOUR_FRONTGROUND, true);
    }
  }

  static bool latchLL = false;
  static bool latchLR = false;

  if (m & BTN_LOWER_LEFT) {
      if (!latchLL) {
          latchLL = true;
          quitGame = true;
      }
  } else latchLL = false;

  if (m & BTN_LOWER_RIGHT) {
      if (!latchLR) {
          latchLR = true;
          if (!spinning) beginSpin();
          else skipSpin = true;
      }
  } else latchLR = false;

  btn.last = m;
}

// ======== Arduino ========
void setupSpinTheWheel(TinyScreen &display){
  Wire.begin();
  display.begin();
  display.setFlip(0);
  display.setBrightness(10); 
  randomSeed(analogRead(0));

  // black screen
  display.drawRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, 1, COLOUR_BACKGROUND);

  wheelAngle = thetaForSectorAtTop(value, maxN);
  
  oldWheelAngle = wheelAngle;
  oldMaxN = maxN;
  oldWasSpinning = false; // start 
  
  // draw initial state
  drawWheelState(wheelAngle, maxN, -1, COLOUR_FRONTGROUND, true);
}

void runSpinTheWheel(TinyScreen &display, bool &quitGame){
  handleButtons(quitGame);
  updateSpin();
  int8_t showIdx = spinning ? (int8_t)sectorIndexAtPointer(wheelAngle, maxN) : (int8_t)value;
  drawWheel(maxN, showIdx);
}