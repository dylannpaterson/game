// src/enemy.cpp

#include "enemy.h"
#include "character.h" // Include character.h for PlayerCharacter definition
#include "level.h"
#include "game_data.h" // Include game_data.h for GameData and IntendedAction
#include "asset_manager.h"
#include "utils.h"
#include <SDL.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

// Initialize static ID counter
int Enemy::nextId = 0;

// --- Constructor Implementation (Assign ID) ---
Enemy::Enemy(int uniqueId, EnemyType eType, int startX, int startY, int tileW, int tileH)
    : id(uniqueId), // Assign the unique ID
      type(eType),
      x(startX), y(startY),
      isMoving(false), startTileX(startX), startTileY(startY),
      targetTileX(startX), targetTileY(startY), moveProgress(0.0f),
      moveTimer(0.0f), // hasTakenActionThisTurn removed
      tileWidth(tileW), tileHeight(tileH)
{
    // Initialize visual position
    visualX = startX * tileWidth + tileWidth / 2.0f;
    visualY = startY * tileHeight + tileHeight / 2.0f;

    // Set attributes based on EnemyType
    switch (type) {
        case EnemyType::SLIME:
            health = 15; maxHealth = 15;
            width = tileW * 0.8; height = tileH * 0.8;
            arcanaValue = 12; textureName = "slime_texture";
            moveDuration = 0.18f;
            baseAttackDamage = 8;
            break;
        default:
             SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unknown EnemyType encountered in constructor!");
             health = 10; maxHealth = 10; width = tileW/2; height = tileH/2; arcanaValue = 5; textureName = ""; moveDuration = 0.3f; baseAttackDamage = 10;
            break;
    }
     SDL_Log("Enemy %d created: Type %d, HP %d, Pos (%d, %d), Texture '%s'", id, (int)type, health, x, y, textureName.c_str());
}
// ------------------------------------------

// --- NEW: planAction Implementation ---
// This function decides what the enemy *wants* to do, returning the plan.
IntendedAction Enemy::planAction(const Level &levelData, const PlayerCharacter &player, const GameData& gameData) const {
  IntendedAction plannedAction; // Start with ActionType::None

  // Don't plan if already moving (shouldn't happen if called correctly, but safe)
  if (isMoving) {
      return plannedAction; // Return ActionType::None
  }

  // --- Check Visibility (using const gameData) ---
  float visibility = 0.0f;
  if (isWithinBounds(x, y, gameData.currentLevel.width, gameData.currentLevel.height) &&
      y < (int)gameData.visibilityMap.size() && x < (int)gameData.visibilityMap[y].size()) {
       visibility = gameData.visibilityMap[y][x];
  }
  bool isVisible = (visibility > 0.0f);

  // --- AI Logic (Similar to takeAction, but returns intent) ---
  if (isVisible) {
      // Visible Logic: Plan Attack or Move
      int playerTileX = player.targetTileX; // Use player's logical position
      int playerTileY = player.targetTileY;
      int dx = playerTileX - x;
      int dy = playerTileY - y;

      // Check Adjacency for Attack Plan
      if (std::abs(dx) <= 1 && std::abs(dy) <= 1 && (dx != 0 || dy != 0)) {
          plannedAction.type = ActionType::Attack;
          // Target could be player's ID if implemented, or coords for now
          plannedAction.targetX = playerTileX;
          plannedAction.targetY = playerTileY;
          SDL_Log("Enemy %d [%d,%d] plans ATTACK on player at [%d,%d]", id, x, y, playerTileX, playerTileY);
      } else {
          // Not Adjacent: Plan to Move Towards Player
          int moveX = 0, moveY = 0;
          if (std::abs(dx) > std::abs(dy)) { moveX = (dx > 0) ? 1 : -1; }
          else if (dy != 0) { moveY = (dy > 0) ? 1 : -1; }
          else if (dx != 0) { moveX = (dx > 0) ? 1 : -1; }

          int nextX = x + moveX;
          int nextY = y + moveY;

          // Check primary direction validity (using gameData.occupationGrid)
          // IMPORTANT: Use the *current* occupation grid for planning.
          // The sequential planning in main.cpp will update this grid *after* each enemy plans.
          bool primaryMoveValid = isWithinBounds(nextX, nextY, levelData.width, levelData.height) &&
                                   levelData.tiles[nextY][nextX] != '#' &&
                                   !gameData.occupationGrid[nextY][nextX]; // Check CURRENT occupation

          if (primaryMoveValid) {
              plannedAction.type = ActionType::Move;
              plannedAction.targetX = nextX;
              plannedAction.targetY = nextY;
               SDL_Log("Enemy %d [%d,%d] plans MOVE to [%d,%d] (Primary)", id, x, y, nextX, nextY);
          } else {
              // Try alternative axis
              if (moveX != 0 && dy != 0) { moveX = 0; moveY = (dy > 0) ? 1 : -1; }
              else if (moveY != 0 && dx != 0) { moveY = 0; moveX = (dx > 0) ? 1 : -1; }
              else { moveX = 0; moveY = 0; }
              nextX = x + moveX;
              nextY = y + moveY;

              if (moveX != 0 || moveY != 0) {
                   bool altMoveValid = isWithinBounds(nextX, nextY, levelData.width, levelData.height) &&
                                       levelData.tiles[nextY][nextX] != '#' &&
                                       !gameData.occupationGrid[nextY][nextX]; // Check CURRENT occupation
                   if (altMoveValid) {
                       plannedAction.type = ActionType::Move;
                       plannedAction.targetX = nextX;
                       plannedAction.targetY = nextY;
                        SDL_Log("Enemy %d [%d,%d] plans MOVE to [%d,%d] (Alt)", id, x, y, nextX, nextY);
                   } else {
                       plannedAction.type = ActionType::Wait; // Blocked
                        SDL_Log("Enemy %d [%d,%d] plans WAIT (Blocked Alt)", id, x, y);
                   }
               } else {
                   plannedAction.type = ActionType::Wait; // Blocked
                   SDL_Log("Enemy %d [%d,%d] plans WAIT (Blocked Primary, No Alt)", id, x, y);
               }
          }
      }
  } else {
      // Invisible Logic: Plan Random Walk
      int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
      int randIndex = rand() % 4;
      int nextX = x + directions[randIndex][0];
      int nextY = y + directions[randIndex][1];

      bool isValidMove = isWithinBounds(nextX, nextY, levelData.width, levelData.height) &&
                         levelData.tiles[nextY][nextX] != '#' &&
                         !gameData.occupationGrid[nextY][nextX]; // Check CURRENT occupation

      if (isValidMove) {
          plannedAction.type = ActionType::Move;
          plannedAction.targetX = nextX;
          plannedAction.targetY = nextY;
           SDL_Log("Enemy %d [%d,%d] plans INVISIBLE MOVE to [%d,%d]", id, x, y, nextX, nextY);
      } else {
          plannedAction.type = ActionType::Wait; // Cannot move randomly
           SDL_Log("Enemy %d [%d,%d] plans INVISIBLE WAIT (Blocked)", id, x, y);
      }
  }

  return plannedAction;
}
// ----------------------------------

// --- update Implementation (Keep for visual movement interpolation) ---
void Enemy::update(float deltaTime, GameData& gameData) {
  // This function now ONLY handles the visual interpolation of movement
  // initiated during the Resolution_Start phase.
 bool wasMoving = isMoving; // Store state *before* potentially changing it

 if (isMoving) {
     moveTimer += deltaTime;
     moveProgress = moveTimer / moveDuration;

     if (moveProgress >= 1.0f) {
         // --- Movement Complete ---
         moveProgress = 1.0f;

         // Snap logical position to the target (SHOULD already be there, but safe)
         // Note: Occupation grid was updated when move was PLANNED/STARTED.
         x = targetTileX;
         y = targetTileY;

         isMoving = false; // Stop moving

         // Snap visual position
         visualX = x * tileWidth + tileWidth / 2.0f;
         visualY = y * tileHeight + tileHeight / 2.0f;

         // No counter decrement here anymore. Resolution phase tracks completion.
         SDL_Log("Enemy %d finished move animation at [%d,%d]", id, x, y);

     } else {
         // Interpolate visual position
         float startVisualX = startTileX * tileWidth + tileWidth / 2.0f;
         float startVisualY = startTileY * tileHeight + tileHeight / 2.0f;
         float targetVisualX = targetTileX * tileWidth + tileWidth / 2.0f;
         float targetVisualY = targetTileY * tileHeight + tileHeight / 2.0f;
         visualX = startVisualX + (targetVisualX - startVisualX) * moveProgress;
         visualY = startVisualY + (targetVisualY - startVisualY) * moveProgress;
     }
 } else {
      // Ensure visual matches logical if not moving
      visualX = x * tileWidth + tileWidth / 2.0f;
      visualY = y * tileHeight + tileHeight / 2.0f;
  }
}


// --- startMove Implementation (Keep - Called during Resolution_Start) ---
void Enemy::startMove(int targetX, int targetY) {
  // Only start if not already moving and target is different
  // Note: Occupation grid should have been checked/updated BEFORE calling this
  if (!isMoving && (targetX != x || targetY != y)) {
    startTileX = x; // Current logical pos is the start
    startTileY = y;
    targetTileX = targetX; // Target logical pos
    targetTileY = targetY;
    isMoving = true;
    moveProgress = 0.0f;
    moveTimer = 0.0f;
    // Visuals will start interpolating in the next update() call
    SDL_Log("Enemy %d starting move animation from [%d,%d] to [%d,%d]", id, startTileX, startTileY, targetTileX, targetTileY);
  } else if (isMoving) {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy %d told to startMove while already moving.", id);
  } else {
       SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy %d told to startMove to its current location [%d,%d].", id, x, y);
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

int Enemy::GetAttackDamage() const {
  // For now, just return the base damage set in the constructor based on type
  return baseAttackDamage;
}