// src/enemy.cpp

#include "enemy.h"
#include "character.h"
#include "level.h"
#include "game_data.h"
#include "asset_manager.h" // Include AssetManager header
#include "utils.h"
#include <SDL.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept> // For std::runtime_error

// --- MODIFIED: Constructor Implementation ---
Enemy::Enemy(EnemyType eType, int startX, int startY, int tileW, int tileH)
    : type(eType), // Store the type
      x(startX), y(startY),
      isMoving(false), startTileX(startX), startTileY(startY),
      targetTileX(startX), targetTileY(startY), moveProgress(0.0f),
      moveTimer(0.0f), hasTakenActionThisTurn(false),
      tileWidth(tileW), tileHeight(tileH)
{
    // Initialize visual position based on logical start position
    visualX = startX * tileWidth + tileWidth / 2.0f;
    visualY = startY * tileHeight + tileHeight / 2.0f;

    // *** Set attributes based on EnemyType ***
    // (Consider moving this data to a map or data file later for better organization)
    switch (type) {
        case EnemyType::SLIME:
            // --- Slime Specific Stats ---
            health = 15;        // Example health
            maxHealth = 15;
            width = tileW * 0.8; // Slime slightly smaller than tile
            height = tileH * 0.8;
            arcanaValue = 12;
            textureName = "slime_texture"; // Make sure this matches asset key
            moveDuration = 0.25f; // Slimes might be slightly slower
            // --------------------------
            break;
        // case EnemyType::SKELETON:
        //     health = 40;
        //     maxHealth = 40;
        //     width = tileW * 0.7;
        //     height = tileH * 0.9;
        //     arcanaValue = 20;
        //     textureName = "skeleton_texture"; // Load this texture in main!
        //     moveDuration = 0.2f;
        //     break;
        // // Add cases for other enemy types here...
        default:
            // Throw error or default to a basic enemy if type is unknown
             SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unknown EnemyType encountered in constructor!");
             // Default fallback (or throw exception)
             health = 10; maxHealth = 10; width = tileW/2; height = tileH/2; arcanaValue = 5; textureName = ""; moveDuration = 0.3f;
            // Optionally: throw std::runtime_error("Unknown EnemyType");
            break;
    }
     SDL_Log("Enemy created: Type %d, HP %d, Pos (%d, %d), Texture '%s'", (int)type, health, x, y, textureName.c_str());
}
// ------------------------------------------

// --- takeAction Implementation (No changes needed for this refactor) ---
void Enemy::takeAction(const Level &levelData, PlayerCharacter &player, GameData& gameData) {
  if (hasTakenActionThisTurn || isMoving) {
      return; // Already acted or currently moving (visible move in progress)
  }

  // --- Check Visibility ---
  float visibility = 0.0f;
  if (isWithinBounds(x, y, gameData.currentLevel.width, gameData.currentLevel.height) &&
      y < (int)gameData.visibilityMap.size() && x < (int)gameData.visibilityMap[y].size()) {
       visibility = gameData.visibilityMap[y][x];
  } else {
       SDL_Log("Warning: Enemy at (%d, %d) out of bounds for visibility check.", x, y);
  }
  bool isVisible = (visibility > 0.0f);

  bool performedPhysicalAction = false; // Flag if attacked or moved (visibly)
  bool actionIsInstant = true;          // Assume instant unless a VISIBLE move starts

  bool isValidMove = false;

  if (isVisible) {
      // --- Visible Logic: Attack or Move (Keep Existing Logic Here) ---
      int playerTileX = player.targetTileX;
      int playerTileY = player.targetTileY;
      int dx = playerTileX - x;
      int dy = playerTileY - y;

      // Check Adjacency for Attack
      if (std::abs(dx) <= 1 && std::abs(dy) <= 1 && (dx != 0 || dy != 0)) {
          int damage = 10; // Example damage
          player.takeDamage(damage);
          SDL_Log("Visible Enemy at (%d, %d) attacked player! Player health: %d", x, y, player.health);
          performedPhysicalAction = true;
          actionIsInstant = true; // Attack is instant
      } else {
          // Not Adjacent: Try to Move Towards Player (Visible Movement)
          int moveX = 0, moveY = 0;
          if (std::abs(dx) > std::abs(dy)) { moveX = (dx > 0) ? 1 : -1; }
          else if (dy != 0) { moveY = (dy > 0) ? 1 : -1; }
          else if (dx != 0) { moveX = (dx > 0) ? 1 : -1; }

          int nextX = x + moveX;
          int nextY = y + moveY;

          bool primaryMoveValid = isWithinBounds(nextX, nextY, levelData.width, levelData.height) &&
                                   levelData.tiles[nextY][nextX] != '#' &&
                                   !gameData.occupationGrid[nextY][nextX];

          if (primaryMoveValid) {
              startMove(nextX, nextY); // <<< VISIBLE move starts here
              performedPhysicalAction = true;
              actionIsInstant = false; // Movement takes time WHEN VISIBLE
          } else {
               // Try alternative axis... (Existing logic for visible movement attempt)
               if (moveX != 0 && dy != 0) { moveX = 0; moveY = (dy > 0) ? 1 : -1; }
               else if (moveY != 0 && dx != 0) { moveY = 0; moveX = (dx > 0) ? 1 : -1; }
               else { moveX = 0; moveY = 0; }
               nextX = x + moveX;
               nextY = y + moveY;

               if (moveX != 0 || moveY != 0) {
                    bool altMoveValid = isWithinBounds(nextX, nextY, levelData.width, levelData.height) &&
                                        levelData.tiles[nextY][nextX] != '#' &&
                                        !gameData.occupationGrid[nextY][nextX];
                    if (altMoveValid) {
                        startMove(nextX, nextY); // <<< VISIBLE move starts here
                        performedPhysicalAction = true;
                        actionIsInstant = false; // Movement takes time WHEN VISIBLE
                    } else { actionIsInstant = true; /* Wait */ }
                } else { actionIsInstant = true; /* Wait */ }
          }
      } // End Visible Move Attempt block
  } else {
      // --- Invisible Logic: Random Walk (MODIFIED FOR INSTANT RESOLUTION) ---
      int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
      int randIndex = rand() % 4;
      int nextX = x + directions[randIndex][0];
      int nextY = y + directions[randIndex][1];

      // Check Validity (including occupation grid bounds)
      bool isValidMove = isWithinBounds(nextX, nextY, levelData.width, levelData.height) &&
                         levelData.tiles[nextY][nextX] != '#' &&
                         !gameData.occupationGrid[nextY][nextX];

      if (isValidMove)
      {
          // *** INSTANT MOVE LOGIC ***
          int oldX = x;
          int oldY = y;

          // 1. Update logical position directly
          x = nextX;
          y = nextY;

          visualX = x * tileWidth + tileWidth / 2.0f;
          visualY = y * tileHeight + tileHeight / 2.0f;

          // 2. Update Occupation Grid (with bounds checks)
          if (isWithinBounds(oldX, oldY, gameData.currentLevel.width, gameData.currentLevel.height)) {
               gameData.occupationGrid[oldY][oldX] = false;
          }
          if (isWithinBounds(x, y, gameData.currentLevel.width, gameData.currentLevel.height)) {
              gameData.occupationGrid[y][x] = true;
          } else {
               SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invisible enemy moved OOB to (%d, %d)? Grid not updated.", x, y);
          }

          // 3. Mark action as complete for turn logic
          hasTakenActionThisTurn = true; // Action completed now

          // 4. Decrement counter immediately
          if(gameData.enemiesActingThisTurn > 0) {
               gameData.enemiesActingThisTurn--;
               SDL_Log("Invisible enemy at (%d, %d) moved instantly. Remaining actors: %d", x, y, gameData.enemiesActingThisTurn);
          } else {
               SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invisible enemy at (%d, %d) moved instantly, but counter already <= 0.", x, y);
          }

          // 5. Ensure actionIsInstant remains true (no timed movement)
          actionIsInstant = true; // Confirming the action was instant
          // DO NOT CALL startMove() here!
      } else {
           // Cannot move randomly, treat as an instant "wait" action
           actionIsInstant = true; // Waiting is instant
      }
  } // End if(isVisible)

  // Mark that AI logic has run for this turn cycle (if not already marked by instant move)
  if (!hasTakenActionThisTurn) { // Only set if not already set by instant invisible move
       hasTakenActionThisTurn = true;
  }


  // Decrement counter ONLY if the action completed instantly AND WAS NOT the instant invisible move (which decremented already)
  // This now primarily applies to visible attacks/waits, or failed invisible moves.
  if (actionIsInstant && !(isVisible == false && isValidMove == true)) { // Check it wasn't the instant invisible move case
      if(gameData.enemiesActingThisTurn > 0) {
           gameData.enemiesActingThisTurn--;
           SDL_Log("Enemy at (%d, %d) finished INSTANT action (Attack/Wait/Blocked Invisible). Remaining actors: %d", x, y, gameData.enemiesActingThisTurn);
      } else {
           SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy at (%d, %d) finished instant action, but counter already <= 0.", x, y);
      }
  }
  // Note: The counter decrement for VISIBLE moves happens in Enemy::update when isMoving becomes false.
}

// --- update Implementation (No changes needed for this refactor) ---
void Enemy::update(float deltaTime, GameData& gameData) {
    // ... (Existing update logic remains the same) ...
    // It uses member variables (isMoving, moveDuration, visualX/Y etc.)
    // which are now set by the constructor based on type.
        bool wasMoving = isMoving; // Store state *before* potentially changing it

    if (isMoving) {
        moveTimer += deltaTime;
        moveProgress = moveTimer / moveDuration; // Uses moveDuration set by type

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
            if (isWithinBounds(oldTileX, oldTileY, gameData.currentLevel.width, gameData.currentLevel.height)) {
                 gameData.occupationGrid[oldTileY][oldTileX] = false;
            }
            // Set new position (use final logical x/y)
            if (isWithinBounds(x, y, gameData.currentLevel.width, gameData.currentLevel.height)) {
                gameData.occupationGrid[y][x] = true;
            } else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy moved out of bounds to (%d, %d)? Grid not updated.", x, y);
            }

            // --- Decrement Counter IF this move completed the turn's action ---
            if (wasMoving && hasTakenActionThisTurn) {
                 if(gameData.enemiesActingThisTurn > 0) {
                     gameData.enemiesActingThisTurn--;
                     SDL_Log("Enemy at (%d, %d) finished MOVE action. Remaining actors: %d", x, y, gameData.enemiesActingThisTurn);
                 } else {
                     SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy at (%d, %d) finished move action, but counter already <= 0.", x, y);
                 }
            }
            // --- End Counter Decrement ---

        } else {
            // Check visibility FIRST
            float visibility = 0.0f;
             if (isWithinBounds(x, y, gameData.currentLevel.width, gameData.currentLevel.height) &&
                 y < (int)gameData.visibilityMap.size() && x < (int)gameData.visibilityMap[y].size()) {
                  visibility = gameData.visibilityMap[y][x];
             }
            bool isVisible = (visibility > 0.0f);

            // Conditional Interpolation
            if (isVisible) {
                float startVisualX = startTileX * tileWidth + tileWidth / 2.0f;
                float startVisualY = startTileY * tileHeight + tileHeight / 2.0f;
                float targetVisualX = targetTileX * tileWidth + tileWidth / 2.0f;
                float targetVisualY = targetTileY * tileHeight + tileHeight / 2.0f;
                visualX = startVisualX + (targetVisualX - startVisualX) * moveProgress;
                visualY = startVisualY + (targetVisualY - startVisualY) * moveProgress;
            } else {
                visualX = targetTileX * tileWidth + tileWidth / 2.0f;
                visualY = targetTileY * tileHeight + tileHeight / 2.0f;
            }
        }
    } else {
         visualX = x * tileWidth + tileWidth / 2.0f;
         visualY = y * tileHeight + tileHeight / 2.0f;
     }
}


// --- startMove Implementation (No changes needed for this refactor) ---
void Enemy::startMove(int targetX, int targetY) {
  if (!isMoving && (targetX != x || targetY != y)) {
    startTileX = x;
    startTileY = y;
    targetTileX = targetX;
    targetTileY = targetY;
    isMoving = true;
    moveProgress = 0.0f;
    moveTimer = 0.0f;
  }
}


// --- Render Implementation (No changes needed for this refactor) ---
void Enemy::render(SDL_Renderer *renderer, AssetManager& assets, int cameraX, int cameraY,
                   float visibilityAlpha) const {
    // It already uses textureName, width, and height set by the constructor.
    SDL_Texture* enemyTexture = assets.getTexture(textureName);
    SDL_Rect destRect;
    destRect.w = width;
    destRect.h = height;
    destRect.x = static_cast<int>(visualX - destRect.w / 2.0f) - cameraX;
    destRect.y = static_cast<int>(visualY - destRect.h / 2.0f) - cameraY;

    if (enemyTexture) {
        Uint8 alpha = static_cast<Uint8>(visibilityAlpha * 255);
        SDL_SetTextureAlphaMod(enemyTexture, alpha);
        SDL_SetTextureBlendMode(enemyTexture, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(renderer, enemyTexture, nullptr, &destRect);
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy texture '%s' not found, drawing fallback rectangle.", textureName.c_str());
        Uint8 alpha = static_cast<Uint8>(visibilityAlpha * 255);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, alpha); // Red fallback
        SDL_RenderFillRect(renderer, &destRect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}


// --- takeDamage Implementation (No changes needed for this refactor) ---
void Enemy::takeDamage(int amount) {
    if (health <= 0) {
        return;
    }
    health -= amount;
    if (health <= 0) {
        health = 0;
        SDL_Log("Enemy at (%d, %d) has been vanquished!", x, y);
    }
}