#include "enemy.h"
#include "character.h" // Needed for takeAction & PlayerCharacter type
#include "level.h"     // Needed for takeAction map data
#include "game_data.h" // Needed for visibilityMap, occupationGrid, counter
#include <SDL.h>       // For SDL_Log
#include <cmath>       // For std::abs
#include <cstdlib>     // For rand()
#include <iostream>    // Or use SDL_Log consistently

// --- Constructor(s) ---
// Ensure all necessary members are initialized, especially maxHealth if used later.
// This constructor matches the one used in dynamic spawning and level generation (assuming default enemyW/H)
Enemy::Enemy(int startX, int startY, int initialHealth, int enemyW, int enemyH,
             int tileW, int tileH, int arcValue)
    : x(startX), y(startY), health(initialHealth), // Store maxHealth
      width(enemyW), height(enemyH), arcanaValue(arcValue),
      isMoving(false), startTileX(startX), startTileY(startY),
      targetTileX(startX), targetTileY(startY), moveProgress(0.0f),
      moveDuration(0.2f), // Default duration, adjust as needed
      moveTimer(0.0f), hasTakenActionThisTurn(false),
      tileWidth(tileW), tileHeight(tileH)
{
    visualX = startX * tileWidth + tileWidth / 2.0f;
    visualY = startY * tileHeight + tileHeight / 2.0f;
    // Add any other necessary initialization
}

// --- takeAction Implementation (Corrected) ---
// Accepts non-const GameData& to modify the counter
void Enemy::takeAction(const Level &levelData, PlayerCharacter &player, GameData& gameData) {
    if (hasTakenActionThisTurn || isMoving) {
        return; // Already acted or currently moving
    }

    // --- Check Visibility ---
    float visibility = 0.0f;
    // Bounds check for map access
    if (x >= 0 && x < gameData.currentLevel.width && y >= 0 && y < gameData.currentLevel.height &&
        y < gameData.visibilityMap.size() && x < gameData.visibilityMap[y].size()) {
         visibility = gameData.visibilityMap[y][x];
    } else {
         SDL_Log("Warning: Enemy at (%d, %d) out of bounds for visibility check.", x, y);
    }
    bool isVisible = (visibility > 0.0f);

    bool performedPhysicalAction = false; // Flag if attacked or moved
    bool actionIsInstant = true;          // Assume instant (wait) unless move starts

    if (isVisible) {
        // --- Visible Logic: Attack or Move ---
        int playerTileX = player.targetTileX;
        int playerTileY = player.targetTileY;
        int dx = playerTileX - x;
        int dy = playerTileY - y;

        // Check Adjacency for Attack
        if (std::abs(dx) <= 1 && std::abs(dy) <= 1 && (dx != 0 || dy != 0)) {
            int damage = 10; // Example damage
            player.takeDamage(damage);
            // SDL_Log("Enemy at (%d, %d) attacked player! Player health: %d", x, y, player.health); // Using WARN level, might be hidden
            performedPhysicalAction = true;
            actionIsInstant = true; // Attack is instant
        } else {
            // Not Adjacent: Try to Move Towards Player
            int moveX = 0, moveY = 0;
            // Simple greedy move logic (same as before)
            if (std::abs(dx) > std::abs(dy)) { moveX = (dx > 0) ? 1 : -1; }
            else if (dy != 0) { moveY = (dy > 0) ? 1 : -1; }
            else if (dx != 0) { moveX = (dx > 0) ? 1 : -1; }

            int nextX = x + moveX;
            int nextY = y + moveY;

            // Check primary direction validity
            bool primaryMoveValid = (nextX >= 0 && nextX < levelData.width && nextY >= 0 && nextY < levelData.height &&
                                     levelData.tiles[nextY][nextX] != '#' &&
                                     !gameData.occupationGrid[nextY][nextX]);

            if (primaryMoveValid) {
                startMove(nextX, nextY);
                performedPhysicalAction = true;
                actionIsInstant = false; // Movement takes time
            } else {
                // Try alternative axis if primary failed
                if (moveX != 0 && dy != 0) { // Tried X, try Y
                    moveX = 0; moveY = (dy > 0) ? 1 : -1;
                } else if (moveY != 0 && dx != 0) { // Tried Y, try X
                    moveY = 0; moveX = (dx > 0) ? 1 : -1;
                } else { // Cannot move in primary or secondary axis
                    moveX = 0; moveY = 0;
                }
                nextX = x + moveX;
                nextY = y + moveY;

                if (moveX != 0 || moveY != 0) { // If alternative direction exists
                     bool altMoveValid = (nextX >= 0 && nextX < levelData.width && nextY >= 0 && nextY < levelData.height &&
                                         levelData.tiles[nextY][nextX] != '#' &&
                                         !gameData.occupationGrid[nextY][nextX]);
                     if (altMoveValid) {
                         startMove(nextX, nextY);
                         performedPhysicalAction = true;
                         actionIsInstant = false;
                     } else { actionIsInstant = true; /* Wait */ }
                 } else { actionIsInstant = true; /* Wait */ }
            }
        } // End Move Attempt block
    } else {
        // --- Invisible Logic: Random Walk ---
        int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
        int randIndex = rand() % 4;
        int nextX = x + directions[randIndex][0];
        int nextY = y + directions[randIndex][1];

        // Check Validity
        if (nextX >= 0 && nextX < levelData.width && nextY >= 0 && nextY < levelData.height &&
            levelData.tiles[nextY][nextX] != '#' &&
            !gameData.occupationGrid[nextY][nextX])
        {
            startMove(nextX, nextY);
            performedPhysicalAction = true;
            actionIsInstant = false; // Movement takes time
        } else {
             actionIsInstant = true; // Wait if random move failed
        }
    } // End if(isVisible)

    // Mark that AI logic has run for this turn cycle
    hasTakenActionThisTurn = true;

    // Decrement counter ONLY if the action completed instantly
    if (actionIsInstant) {
        if(gameData.enemiesActingThisTurn > 0) {
             gameData.enemiesActingThisTurn--;
             // SDL_Log("Enemy at (%d, %d) finished INSTANT action. Remaining actors: %d", x, y, gameData.enemiesActingThisTurn); // Debug Level
        } else {
             SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy at (%d, %d) finished instant action, but counter already <= 0.", x, y);
        }
    }
    // If movement was initiated, counter decreases in update() upon completion.
}

// --- update Implementation (Corrected) ---
// Accepts non-const GameData& for grid update and counter decrement
void Enemy::update(float deltaTime, GameData& gameData) {
    bool wasMoving = isMoving; // Store state *before* potentially changing it

    if (isMoving) {
        moveTimer += deltaTime;
        moveProgress = moveTimer / moveDuration;

        if (moveProgress >= 1.0f) {
            // --- Movement Complete ---
            moveProgress = 1.0f; // Clamp

            int oldTileX = startTileX; // Store position *before* snapping
            int oldTileY = startTileY;

            // Snap logical position to the target *now*
            x = targetTileX;
            y = targetTileY;

            isMoving = false; // Update state AFTER snapping logical position

            // Snap visual position to final destination
            visualX = x * tileWidth + tileWidth / 2.0f;
            visualY = y * tileHeight + tileHeight / 2.0f;

            // Update Occupation Grid
            // Clear old position (use stored oldTileX/Y)
            if (oldTileX >= 0 && oldTileX < gameData.currentLevel.width && oldTileY >= 0 && oldTileY < gameData.currentLevel.height) {
                 gameData.occupationGrid[oldTileY][oldTileX] = false;
            }
            // Set new position (use final logical x/y)
            if (x >= 0 && x < gameData.currentLevel.width && y >= 0 && y < gameData.currentLevel.height) {
                gameData.occupationGrid[y][x] = true;
            } else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy moved out of bounds to (%d, %d)? Grid not updated.", x, y);
            }

            // --- Decrement Counter IF this move completed the turn's action ---
            // Check if it WAS moving, ISN'T now, and HAD decided an action this cycle
            if (wasMoving && hasTakenActionThisTurn) {
                 if(gameData.enemiesActingThisTurn > 0) {
                     gameData.enemiesActingThisTurn--;
                     // SDL_Log("Enemy at (%d, %d) finished MOVE action. Remaining actors: %d", x, y, gameData.enemiesActingThisTurn); // Debug Level
                 } else {
                     SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy at (%d, %d) finished move action, but counter already <= 0.", x, y);
                 }
                 // Note: hasTakenActionThisTurn remains true until reset globally at the start of the next EnemyTurn cycle.
            }
            // --- End Counter Decrement ---

        } else {
            // Check visibility FIRST
            float visibility = 0.0f;
            // Use logical coords (x,y) for visibility check - these represent the current tile
             if (x >= 0 && x < gameData.currentLevel.width && y >= 0 && y < gameData.currentLevel.height &&
                 y < gameData.visibilityMap.size() && x < gameData.visibilityMap[y].size()) {
                  visibility = gameData.visibilityMap[y][x];
             }
             // Alternative: check visibility at startTileX/Y? Or targetTileX/Y? Current tile seems most logical.
            bool isVisible = (visibility > 0.0f);

            // --- NEW: Conditional Interpolation ---
            if (isVisible) {
                // Enemy is visible: Perform smooth interpolation (Lerp)
                float startVisualX = startTileX * tileWidth + tileWidth / 2.0f;
                float startVisualY = startTileY * tileHeight + tileHeight / 2.0f;
                float targetVisualX = targetTileX * tileWidth + tileWidth / 2.0f;
                float targetVisualY = targetTileY * tileHeight + tileHeight / 2.0f;
                visualX = startVisualX + (targetVisualX - startVisualX) * moveProgress;
                visualY = startVisualY + (targetVisualY - startVisualY) * moveProgress;
            } else {
                // Enemy is NOT visible: Skip lerp calculation.
                // Snap visual position directly to the *destination* tile's center.
                // This saves calculation and ensures correct position if revealed upon arrival.
                visualX = targetTileX * tileWidth + tileWidth / 2.0f;
                visualY = targetTileY * tileHeight + tileHeight / 2.0f;
            }
             // --- End Conditional Interpolation ---
        }
    } else {
         // Ensure visual position matches logical if not moving
         visualX = x * tileWidth + tileWidth / 2.0f;
         visualY = y * tileHeight + tileHeight / 2.0f;
     }
}


// --- startMove Implementation (Corrected) ---
void Enemy::startMove(int targetX, int targetY) {
  // Check if already moving or target is the same as current logical position
  if (!isMoving && (targetX != x || targetY != y)) {
    startTileX = x; // Current logical position is the start
    startTileY = y;
    targetTileX = targetX; // Store the intended destination tile
    targetTileY = targetY;
    isMoving = true; // Set moving flag
    moveProgress = 0.0f;
    moveTimer = 0.0f;
    // DO NOT update logical x, y here. It changes only when move completes in update().
  }
}


// --- render Implementation (Unchanged from previous version) ---
void Enemy::render(SDL_Renderer *renderer, int cameraX, int cameraY,
                   float visibilityAlpha) const {
    SDL_Rect enemyRect;
    enemyRect.w = width;
    enemyRect.h = height;
    enemyRect.x = static_cast<int>(visualX - width / 2.0f) - cameraX;
    enemyRect.y = static_cast<int>(visualY - height / 2.0f) - cameraY;

    Uint8 alpha = static_cast<Uint8>(visibilityAlpha * 255);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, alpha); // Red with variable alpha
    SDL_RenderFillRect(renderer, &enemyRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// --- takeDamage Implementation (Unchanged from previous version) ---
void Enemy::takeDamage(int amount) {
    if (health <= 0) {
        return;
    }
    health -= amount;
    // SDL_Log("Enemy at (%d, %d) took %d damage.", x, y, amount); // Using WARN, may be hidden

    if (health <= 0) {
        health = 0;
        SDL_Log("Enemy at (%d, %d) has been vanquished!", x, y); // INFO level, might show depending on setting
    } else {
        // SDL_Log("Enemy health remaining: %d", health); // Using default INFO level
    }
}