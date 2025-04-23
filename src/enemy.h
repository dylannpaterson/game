#ifndef ENEMY_H
#define ENEMY_H

#include <SDL.h>

struct PlayerCharacter; // Forward declare PlayerCharacter
struct Level;           // Forward declare Level
struct GameData;      // Forward declare GameData

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

    int tileWidth;  // Store the tile context
    int tileHeight;

    bool hasTakenActionThisTurn;

    int arcanaValue = 10; // Default Arcana value if none provided

    Enemy(int startX, int startY, int initialHealth, int enemyW, int enemyH, int tileW, int tileH, int arcValue = 10); // Add tile dims
    void takeAction(const Level& level, PlayerCharacter& player, GameData& gameData);
    void update(float deltaTime, GameData& gameData);
    void render(SDL_Renderer* renderer, int cameraX, int cameraY, float visibilityAlpha) const;
    void startMove(int targetX, int targetY);
    void takeDamage(int amount);
};

#endif // ENEMY_H