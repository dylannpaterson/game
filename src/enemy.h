#ifndef ENEMY_H
#define ENEMY_H

#include <SDL.h>
#include "character.h"

class Enemy {
    public:
    int x; // Target tile X
    int y; // Target tile Y
    int health;
    int width;
    int height;

    float visualX; // Current visual x position (floating point)
    float visualY; // Current visual y position (floating point)

    bool isMoving;
    int startTileX;
    int startTileY;
    int targetTileX;
    int targetTileY;
    float moveProgress;
    float moveDuration;
    float moveTimer;

    bool hasTakenActionThisTurn;

    Enemy(int startX, int startY);
    Enemy(int startX, int startY, int initialHealth, int enemyWidth, int enemyHeight);
    void takeAction(const class Level& level, PlayerCharacter& player);
    void render(SDL_Renderer* renderer, int cameraX, int cameraY, int tileWidth, int tileHeight, float visibilityAlpha) const;
    void update(float deltaTime, int tileWidth, int tileHeight); // New update function
    void startMove(int targetX, int targetY);
};

#endif // ENEMY_H