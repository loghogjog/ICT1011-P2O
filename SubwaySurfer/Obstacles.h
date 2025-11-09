#ifndef OBSTACLES_H
#define OBSTACLES_H

struct Obstacle {
    int lane;
    int y;
    bool active;
    int type;
    int trainHeight;
};

extern const int MAX_OBS;
extern Obstacle obstacles[];

void drawObstacle(int lane, int y, int type, int trainHeight);
void updateObstacles();
void checkCollision();

#endif