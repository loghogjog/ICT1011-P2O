#ifndef MAZE_H
#define MAZE_H

#include <TinyScreen.h>
#include <BMA250.h>

// Game states enum
enum MazeGameState {
  MAZE_MENU,
  PLAYING,
  LEVEL_COMPLETE,
  GAME_OVER,
};

// External variables (defined in Maze.cpp)
extern TinyScreen display;
extern BMA250 accel;

extern MazeGameState currentState;
extern int menuSelection;
extern const int NUM_MENU_ITEMS;

extern float ballX;
extern float ballY;
extern float ballVelX;
extern float ballVelY;
extern float prevX;
extern float prevY;

extern unsigned long levelStartTime;
extern unsigned long lastFrameTime;
extern int timeLimit;
extern int timeRemaining;

extern const uint8_t* currentMaze;

// Function prototypes:
void setupMaze(TinyScreen &display);
void runMaze(TinyScreen &display, bool &exitToMenu);
void runMenu(bool &exitToMenu);
void startLevel(int level);
void runGame();
bool checkCollision(float x, float y);
void drawMaze();
void showLevelComplete();
void showGameOver();
bool checkExitOverlap(float x, float y);

#endif
