#include "enemy.h"
#include "character.h"
#include "level.h"
#include <SDL.h>
#include <cmath>
#include <cstdlib>

Enemy::Enemy(int startX, int startY, int tileW, int tileH)
    : x(startX), y(startY), health(20), width(tileW),
      height(tileH), // Default visual size to tile size
      isMoving(false), startTileX(startX), startTileY(startY),
      targetTileX(startX), targetTileY(startY), moveProgress(0.0f),
      moveDuration(0.2f), moveTimer(0.0f), hasTakenActionThisTurn(false),
      tileWidth(tileW), tileHeight(tileH) // Initialize members
{
  // Calculate initial visual position using member tileWidth/Height
  visualX = startX * tileWidth + tileWidth / 2.0f;
  visualY = startY * tileHeight + tileHeight / 2.0f;
} // Initialize here

Enemy::Enemy(int startX, int startY, int initialHealth, int enemyW, int enemyH,
             int tileW, int tileH)
    : x(startX), y(startY), health(initialHealth), width(enemyW),
      height(enemyH), // Use specific enemy visual size
      isMoving(false), startTileX(startX), startTileY(startY),
      targetTileX(startX), targetTileY(startY), moveProgress(0.0f),
      moveDuration(0.2f), moveTimer(0.0f), hasTakenActionThisTurn(false),
      tileWidth(tileW), tileHeight(tileH) // Initialize members
{
  // Calculate initial visual position using member tileWidth/Height
  visualX = startX * tileWidth + tileWidth / 2.0f;
  visualY = startY * tileHeight + tileHeight / 2.0f;
} // Initialize here

void Enemy::takeAction(const Level &level, PlayerCharacter &player) {
  if (hasTakenActionThisTurn) {
    return; // Enemy has already acted this turn
  }
  // Get the player's current tile coordinates
  int playerTileX = static_cast<int>(player.x / player.tileWidth);
  int playerTileY = static_cast<int>(player.y / player.tileHeight);

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
      if (dx > 0 && level.tiles[y][x + 1] == '.')
        moveX = 1;
      else if (dx < 0 && level.tiles[y][x - 1] == '.')
        moveX = -1;
    }

    // If no horizontal movement, or if vertical difference is greater/equal,
    // try vertical
    if (moveX == 0 && std::abs(dy) > 0) {
      if (dy > 0 && level.tiles[y + 1][x] == '.')
        moveY = 1;
      else if (dy < 0 && level.tiles[y - 1][x] == '.')
        moveY = -1;
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
  if (!isMoving &&
      (targetX != this->targetTileX || targetY != this->targetTileY)) {
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

void Enemy::update(float deltaTime) {
  if (isMoving) {
    moveTimer += deltaTime;
    moveProgress = moveTimer / moveDuration;
    if (moveProgress >= 1.0f) {
      moveProgress = 1.0f;
      isMoving = false;
      // Update logical position ONLY when move completes fully
      // x = targetTileX; // This was likely wrong in startMove, logical pos
      // shouldn't change until move ends y = targetTileY;
      startTileX = targetTileX; // Prepare for next potential move
      startTileY = targetTileY;
    }
    // Calculate visual position using MEMBER tileWidth/Height
    visualX = startTileX * tileWidth +
              (targetTileX - startTileX) * tileWidth * moveProgress +
              tileWidth / 2.0f;
    visualY = startTileY * tileHeight +
              (targetTileY - startTileY) * tileHeight * moveProgress +
              tileHeight / 2.0f;

    // If movement just finished, snap visual to final logical center
    if (!isMoving) {
      visualX = targetTileX * tileWidth + tileWidth / 2.0f;
      visualY = targetTileY * tileHeight + tileHeight / 2.0f;
    }

  } else {
    // Ensure visual position matches logical position if not moving
    // This uses MEMBER tileWidth/Height now
    visualX = x * tileWidth + tileWidth / 2.0f;
    visualY = y * tileHeight + tileHeight / 2.0f;
  }
}

void Enemy::render(SDL_Renderer *renderer, int cameraX, int cameraY,
                   float visibilityAlpha) const {
  SDL_Rect enemyRect;
  enemyRect.w = width; // Use MEMBER width/height for visual size
  enemyRect.h = height;
  enemyRect.x = static_cast<int>(visualX - width / 2.0f) - cameraX;
  enemyRect.y = static_cast<int>(visualY - height / 2.0f) - cameraY;

  Uint8 alpha = static_cast<Uint8>(visibilityAlpha * 255);
  SDL_SetRenderDrawBlendMode(
      renderer, SDL_BLENDMODE_BLEND); // Enable alpha for visibility
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, alpha); // Red with variable alpha
  SDL_RenderFillRect(renderer, &enemyRect);
  SDL_SetRenderDrawBlendMode(renderer,
                             SDL_BLENDMODE_NONE); // Disable alpha blending
}

void Enemy::takeDamage(int amount) {
  if (health <= 0) {
    return; // Already defeated, cannot take more damage
  }

  health -= amount;
  SDL_Log("Enemy at (%d, %d) took %d damage.", x, y,
          amount); // Use SDL_Log for consistency if you prefer

  if (health <= 0) {
    health = 0; // Ensure health doesn't go negative
    SDL_Log("Enemy at (%d, %d) has been vanquished!", x, y);
    // --- Important Note on Enemy Death ---
    // DO NOT try to delete the enemy or remove it from a vector *here*.
    // This function only changes the enemy's state (health).
    // The main game loop (in main.cpp) should be responsible for checking
    // enemy health each turn (or after actions) and removing enemies
    // with health <= 0 from the 'enemies' vector.
    // You might add an 'isDead' boolean flag here if it helps your main loop
    // logic. For example: isDead = true;
  } else {
    SDL_Log("Enemy health remaining: %d", health);
  }
}