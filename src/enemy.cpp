#include "enemy.h"
#include "character.h"
#include <SDL.h>
#include <cmath>
#include <cstdlib>
#include "level.h"

Enemy::Enemy(int startX, int startY)
    : x(startX), y(startY), health(20), width(64), height(64),
      visualX(startX * 64 + 32.0f),
      visualY(startY * 64 + 32.0f),
      isMoving(false), startTileX(startX), startTileY(startY), targetTileX(startX), targetTileY(startY),
      moveProgress(0.0f), moveDuration(0.2f), moveTimer(0.0f), hasTakenActionThisTurn(false) {} // Initialize here

Enemy::Enemy(int startX, int startY, int initialHealth, int enemyWidth, int enemyHeight)
    : x(startX), y(startY), health(initialHealth), width(enemyWidth), height(enemyHeight),
      visualX(startX * 64 + 32.0f),
      visualY(startY * 64 + 32.0f),
      isMoving(false), startTileX(startX), startTileY(startY), targetTileX(startX), targetTileY(startY),
      moveProgress(0.0f), moveDuration(0.2f), moveTimer(0.0f), hasTakenActionThisTurn(false) {} // Initialize here

      void Enemy::takeAction(const Level& level, PlayerCharacter& player) {
        if (hasTakenActionThisTurn) {
            return; // Enemy has already acted this turn
        }
        // Get the player's current tile coordinates
        int playerTileX = static_cast<int>(player.x / 64);
        int playerTileY = static_cast<int>(player.y / 64);

        bool performedAction = false;

        // Check if the enemy is adjacent to the player's current tile
        int dx = playerTileX - x;
        int dy = playerTileY - y;

        if (std::abs(dx) <= 1 && std::abs(dy) <= 1 && (dx != 0 || dy != 0)) {
            // Enemy is adjacent, perform attack
            int damage = 10;
            player.health -= damage;
            SDL_Log("Enemy attacked player! Player health: %d", player.health);
            performedAction = true;
        } else if (std::abs(dx) + std::abs(dy) <= 5) {
            int moveX = 0;
            int moveY = 0;

            // Prioritize horizontal movement
            if (std::abs(dx) > 0) {
                if (dx > 0 && level.tiles[y][x + 1] == '.') moveX = 1;
                else if (dx < 0 && level.tiles[y][x - 1] == '.') moveX = -1;
            }

            // If no horizontal movement, or if vertical difference is greater/equal, try vertical
            if (moveX == 0 && std::abs(dy) > 0) {
                if (dy > 0 && level.tiles[y + 1][x] == '.') moveY = 1;
                else if (dy < 0 && level.tiles[y - 1][x] == '.') moveY = -1;
            }

            if (moveX != 0 || moveY != 0) {
                startMove(x + moveX, y + moveY);
                performedAction = true;
            }
        }

        if (performedAction) {
            hasTakenActionThisTurn = true; // Mark as acted
        } else {
            // If no action was taken (e.g., player is too far), still end the turn
            hasTakenActionThisTurn = true;
        }
    }

void Enemy::startMove(int targetX, int targetY) {
    if (!isMoving && (targetX != this->targetTileX || targetY != this->targetTileY)) {
        isMoving = true;
        startTileX = x; // Use the current tile coordinates as the start
        startTileY = y;
        this->targetTileX = targetX;
        this->targetTileY = targetY;
        moveProgress = 0.0f;
        moveTimer = 0.0f;
        x = targetX; // Update the target tile immediately
        y = targetY;
    }
}

void Enemy::update(float deltaTime, int tileWidth, int tileHeight) {
    if (isMoving) {
        moveTimer += deltaTime;
        moveProgress = moveTimer / moveDuration;
        if (moveProgress >= 1.0f) {
            moveProgress = 1.0f;
            isMoving = false;
            startTileX = targetTileX;
            startTileY = targetTileY;
        }
        visualX = startTileX * tileWidth + (targetTileX - startTileX) * tileWidth * moveProgress + tileWidth / 2.0f;
        visualY = startTileY * tileHeight + (targetTileY - startTileY) * tileHeight * moveProgress + tileHeight / 2.0f;
    } else {
        visualX = x * tileWidth + tileWidth / 2.0f;
        visualY = y * tileHeight + tileHeight / 2.0f;
    }
}

void Enemy::render(SDL_Renderer* renderer, int cameraX, int cameraY, int tileWidth, int tileHeight, float visibilityAlpha) const {
    SDL_Rect enemyRect;
    enemyRect.w = width;
    enemyRect.h = height;
    enemyRect.x = static_cast<int>(visualX - width / 2.0f) - cameraX;
    enemyRect.y = static_cast<int>(visualY - height / 2.0f) - cameraY;

    // Convert the float alpha (0.0 to 1.0) to an 8-bit alpha (0 to 255)
    Uint8 alpha = static_cast<Uint8>(visibilityAlpha * 255);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, alpha); // Red with variable alpha
    SDL_RenderFillRect(renderer, &enemyRect);
}