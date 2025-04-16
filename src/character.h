#ifndef CHARACTER_H
#define CHARACTER_H
#include <iostream>

enum class CharacterType {
    FemaleMage,
    MaleMage
};

struct PlayerCharacter {
    CharacterType type;
    int health;
    int maxHealth;
    int mana;
    int maxMana;
    int level;

    // --- Lerping Movement Variables ---
    float x; // Current visual x position (floating point)
    float y; // Current visual y position (floating point)

    bool isMoving;
    int startTileX;
    int startTileY;
    int targetTileX;
    int targetTileY;
    float moveProgress;
    float moveDuration;
    float moveTimer;

    // Constructor to initialize lerping variables
    PlayerCharacter(CharacterType t, int h, int mh, int m, int mm, int lvl, int startX, int startY, int tileWidth, int tileHeight)
        : type(t), health(h), maxHealth(mh), mana(m), maxMana(mm), level(lvl),
          x(startX * tileWidth + tileWidth / 2.0f),
          y(startY * tileHeight + tileHeight / 2.0f),
          isMoving(false), startTileX(startX), startTileY(startY), targetTileX(startX), targetTileY(startY),
          moveProgress(0.0f), moveDuration(0.2f), moveTimer(0.0f) {}

    // Function to initiate movement
    void startMove(int targetX, int targetY) {
        if (!isMoving && (targetX != targetTileX || targetY != targetTileY)) {
            isMoving = true;
            startTileX = targetTileX;
            startTileY = targetTileY;
            this->targetTileX = targetX;
            this->targetTileY = targetY;
            moveProgress = 0.0f;
            moveTimer = 0.0f;
        }
    }

    // Function to update the player's position during movement
    void update(float deltaTime, int tileWidth, int tileHeight) {
        if (isMoving) {
            moveTimer += deltaTime;
            moveProgress = moveTimer / moveDuration;
            if (moveProgress >= 1.0f) {
                moveProgress = 1.0f;
                isMoving = false;
                startTileX = targetTileX;
                startTileY = targetTileY;
            }
            x = startTileX * tileWidth + (targetTileX - startTileX) * tileWidth * moveProgress + tileWidth / 2.0f;
            y = startTileY * tileHeight + (targetTileY - startTileY) * tileHeight * moveProgress + tileHeight / 2.0f;
        }
    }
};

#endif // CHARACTER_H