// src/enemy.cpp

#include "enemy.h"
#include "asset_manager.h"
#include "character.h" // Include character.h for PlayerCharacter definition
#include "game_data.h" // Include game_data.h for GameData and IntendedAction
#include "level.h"
#include "utils.h"
#include <SDL.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

// Initialize static ID counter
int Enemy::nextId = 0;

// --- Constructor Implementation (Assign ID) ---
Enemy::Enemy(int uniqueId, EnemyType eType, int startX, int startY, int tileW,
             int tileH)
    : id(uniqueId), // Assign the unique ID
      type(eType), x(startX), y(startY), isMoving(false), startTileX(startX),
      startTileY(startY), targetTileX(startX), targetTileY(startY),
      moveProgress(0.0f), moveTimer(0.0f),
      isAttacking(false),
      idleAnimationTimer(0.0f),
      attackStartX(0.0f), attackStartY(0.0f),
      attackTargetX(0.0f), attackTargetY(0.0f),
      lungeDistanceRatio(0.4f),
      currentIdleFrame(0), tileWidth(tileW), tileHeight(tileH),
      // +++ Initialize facing direction +++
      currentFacingDirection(FacingDirection::Left) // Default to Left
      // +++++++++++++++++++++++++++++++++++++
       {
  // Initialize visual position
  visualX = startX * tileWidth + tileWidth / 2.0f;
  visualY = startY * tileHeight + tileHeight / 2.0f;

  // Set attributes based on EnemyType
  switch (type) {
  case EnemyType::SLIME:
    health = 20;
    maxHealth = 20;
    width = static_cast<int>(tileW * 0.8); // Ensure width/height are int
    height = static_cast<int>(tileH * 0.8);
    arcanaValue = 12;
    textureName = "slime_texture";
    moveDuration = 0.7f;
    baseAttackDamage = 8;
    idleAnimationSpeed = 8.0f; // Slimes might animate slowly
    walkAnimationSpeed = 8.0f;
    attackAnimationSpeed = 16.0f;
    lungeDistanceRatio = 0.6f; // Slime might make smaller lunges

    // --- ADDED: Initialize Slime Idle Frames ---
    idleFrameTextureNames = {
        "slime_idle_1", "slime_idle_2", "slime_idle_3",
        "slime_idle_4", "slime_idle_5", "slime_idle_6",
        "slime_idle_7", "slime_idle_8", "slime_idle_9"}; // Example names

    // --- ADDED: Initialize Slime Walk Frames (9 frames) ---
    walkFrameTextureNames = {"slime_walk_1", "slime_walk_2", "slime_walk_3",
                             "slime_walk_4", "slime_walk_5", "slime_walk_6",
                             "slime_walk_7", "slime_walk_8", "slime_walk_9"};

    // --- ADDED: Initialize Slime Attack Frames (9 frames) ---
    attackFrameTextureNames = {
        "slime_attack_1", "slime_attack_2", "slime_attack_3",
        "slime_attack_4", "slime_attack_5", "slime_attack_6",
        "slime_attack_7", "slime_attack_8", "slime_attack_9"}; // Example names
    // -----------------------------------------

    // Calculate total duration for one loop of attack animation
    if (attackAnimationSpeed > 0 && !attackFrameTextureNames.empty()) {
      attackAnimationDuration =
          (1.0f / attackAnimationSpeed) * attackFrameTextureNames.size();
    } else {
      attackAnimationDuration =
          0.5f; // Default duration if speed/frames invalid
    }

    break;
  default:
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Unknown EnemyType encountered in constructor!");
    health = 10;
    maxHealth = 10;
    width = tileW / 2;
    height = tileH / 2;
    arcanaValue = 5;
    textureName = "";
    moveDuration = 0.3f;
    baseAttackDamage = 10;
    break;
  }
  SDL_Log("Enemy %d created: Type %d, HP %d, Pos (%d, %d), Texture '%s'", id,
          (int)type, health, x, y, textureName.c_str());
}
// ------------------------------------------

// --- NEW: planAction Implementation ---
// This function decides what the enemy *wants* to do, returning the plan.
IntendedAction Enemy::planAction(const Level &levelData,
                                 const PlayerCharacter &player,
                                 const GameData &gameData) const {
    IntendedAction plannedAction; // Start with ActionType::None
    plannedAction.enemyId = this->id; // Store the enemy's ID

    // Don't plan if already moving or attacking (shouldn't happen if called correctly, but safe)
    if (isMoving || isAttacking) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy %d planAction called while moving/attacking. Returning None.", id);
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
            plannedAction.targetX = playerTileX; // Store player tile coords
            plannedAction.targetY = playerTileY;
            SDL_Log("Enemy %d [%d,%d] plans ATTACK on player at [%d,%d]", id, x, y, playerTileX, playerTileY);
        } else {
            // Not Adjacent: Plan to Move Towards Player
            int moveX = 0, moveY = 0;
            if (std::abs(dx) > std::abs(dy)) {
                moveX = (dx > 0) ? 1 : -1;
            } else if (dy != 0) {
                moveY = (dy > 0) ? 1 : -1;
            } else if (dx != 0) { // Only move horizontally if dy is 0
                moveX = (dx > 0) ? 1 : -1;
            }

            int nextX = x + moveX;
            int nextY = y + moveY;

            // Check primary direction validity (using gameData.occupationGrid)
            // IMPORTANT: Use the *current* occupation grid for planning.
            bool primaryMoveValid =
                isWithinBounds(nextX, nextY, levelData.width, levelData.height) &&
                levelData.tiles[nextY][nextX] != '#' &&
                !gameData.occupationGrid[nextY][nextX]; // Check CURRENT occupation

            if (primaryMoveValid) {
                plannedAction.type = ActionType::Move;
                plannedAction.targetX = nextX;
                plannedAction.targetY = nextY;
                SDL_Log("Enemy %d [%d,%d] plans MOVE to [%d,%d] (Primary)", id, x, y, nextX, nextY);
            } else {
                // Try alternative axis if primary failed
                int altMoveX = 0, altMoveY = 0;
                if (moveX != 0) { // Primary was horizontal, try vertical
                    if (dy != 0) altMoveY = (dy > 0) ? 1 : -1;
                } else if (moveY != 0) { // Primary was vertical, try horizontal
                     if (dx != 0) altMoveX = (dx > 0) ? 1 : -1;
                }
                // If primary was zero (shouldn't happen unless dx=dy=0), stay put

                int altNextX = x + altMoveX;
                int altNextY = y + altMoveY;

                if (altMoveX != 0 || altMoveY != 0) { // Check if alternative is possible
                    bool altMoveValid =
                        isWithinBounds(altNextX, altNextY, levelData.width, levelData.height) &&
                        levelData.tiles[altNextY][altNextX] != '#' &&
                        !gameData.occupationGrid[altNextY][altNextX]; // Check CURRENT occupation
                    if (altMoveValid) {
                        plannedAction.type = ActionType::Move;
                        plannedAction.targetX = altNextX;
                        plannedAction.targetY = altNextY;
                        SDL_Log("Enemy %d [%d,%d] plans MOVE to [%d,%d] (Alt)", id, x, y, altNextX, altNextY);
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

        bool isValidMove =
            isWithinBounds(nextX, nextY, levelData.width, levelData.height) &&
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

// --- update Implementation (With added movement debugging) ---
void Enemy::update(float deltaTime, GameData& gameData) {

    if (isAttacking) {
        attackAnimationTimer += deltaTime;
        // Ensure duration is positive before division
        float currentAttackDuration = attackAnimationDuration > 0.0f ? attackAnimationDuration : 1.0f; // Avoid div by zero
        float attackProgress = std::min(attackAnimationTimer / currentAttackDuration, 1.0f); // Overall progress 0-1


        // --- Calculate Lunge/Retreat Interpolation ---
        float lungePhaseDuration = currentAttackDuration * 0.5f; // Lunge takes first half
        float retreatPhaseDuration = currentAttackDuration * 0.5f; // Retreat takes second half

        // Safety check for duration
        if (lungePhaseDuration <= 0.0f || retreatPhaseDuration <= 0.0f) {
             SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Enemy %d attack animation duration is zero or negative! Aborting attack update.", id);
             isAttacking = false; // Force exit attacking state
             visualX = attackStartX; // Snap back
             visualY = attackStartY;
             attackAnimationTimer = 0.0f; currentAttackFrame = 0;
             return;
        }

        float currentLungeTargetX = attackStartX + (attackTargetX - attackStartX) * lungeDistanceRatio;
        float currentLungeTargetY = attackStartY + (attackTargetY - attackStartY) * lungeDistanceRatio;

        if (attackAnimationTimer <= lungePhaseDuration) {
            // --- Lunge Phase ---
            float lungeProgress = attackAnimationTimer / lungePhaseDuration; // Progress within lunge phase (0-1)
            // Non-linear interpolation for abruptness (e.g., quadratic ease-out: 1 - (1-p)^2 )
            float easedProgress = 1.0f - (1.0f - lungeProgress) * (1.0f - lungeProgress);
            visualX = attackStartX + (currentLungeTargetX - attackStartX) * easedProgress;
            visualY = attackStartY + (currentLungeTargetY - attackStartY) * easedProgress;

        } else {
            // --- Retreat Phase ---
            float retreatTimer = attackAnimationTimer - lungePhaseDuration; // Time elapsed in retreat phase
            float retreatProgress = retreatTimer / retreatPhaseDuration; // Progress within retreat phase (0-1)
            // Linear interpolation for slower retreat
            visualX = currentLungeTargetX + (attackStartX - currentLungeTargetX) * retreatProgress;
            visualY = currentLungeTargetY + (attackStartY - currentLungeTargetY) * retreatProgress;
        }


        // --- Update Attack Animation Frame ---
        if (!attackFrameTextureNames.empty() && attackAnimationSpeed > 0) {
            float frameDuration = 1.0f / attackAnimationSpeed;
             if (frameDuration > 0) { // Safety check
                currentAttackFrame = static_cast<int>(floor(attackAnimationTimer / frameDuration));
                // Ensure frame index stays within bounds, especially at the very end
                currentAttackFrame = std::min(currentAttackFrame, (int)attackFrameTextureNames.size() - 1);
             } else {
                 currentAttackFrame = 0; // Default if speed is invalid
             }
        }

        // --- Check for Animation Completion ---
        if (attackProgress >= 1.0f) {
            isAttacking = false; // Animation finished
            // Snap visual position back precisely to the starting tile center
            visualX = attackStartX;
            visualY = attackStartY;
            // Reset all animation states
            attackAnimationTimer = 0.0f; currentAttackFrame = 0;
            idleAnimationTimer = 0.0f; currentIdleFrame = 0;
            walkAnimationTimer = 0.0f; currentWalkFrame = 0;
             SDL_Log("Enemy %d finished attack animation.", id);
        }
        // While attacking, don't process movement or idle animation updates below
        return;
    }

    // --- Update Movement (Only if not attacking) ---
    if (isMoving) {
        // +++ MOVEMENT DEBUG LOGGING START +++
        moveTimer += deltaTime;

        // Safety check for moveDuration
        if (moveDuration <= 0.0f) {
             SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Enemy %d moveDuration is zero or negative (%.4f)! Aborting move.", id, moveDuration);
             isMoving = false; // Force exit moving state
             // Snap to target tile logically and visually
             x = targetTileX;
             y = targetTileY;
             visualX = targetTileX * tileWidth + tileWidth / 2.0f;
             visualY = targetTileY * tileHeight + tileHeight / 2.0f;
             moveTimer = 0.0f;
             moveProgress = 0.0f;
             walkAnimationTimer = 0.0f; currentWalkFrame = 0;
             return; // Stop further processing this frame
        }

        moveProgress = moveTimer / moveDuration;

        //SDL_Log("DEBUG_MOVE: Enemy %d Moving | Timer: %.4f | Duration: %.4f | Progress: %.4f",
        //        id, moveTimer, moveDuration, moveProgress); // Reduce log spam
        // +++ MOVEMENT DEBUG LOGGING END +++


        // Update Walk Animation
        if (!walkFrameTextureNames.empty() && walkAnimationSpeed > 0) {
            walkAnimationTimer += deltaTime;
            float walkFrameDuration = 1.0f / walkAnimationSpeed;
            if (walkFrameDuration > 0 && walkAnimationTimer >= walkFrameDuration) {
                 walkAnimationTimer -= walkFrameDuration;
                 currentWalkFrame = (currentWalkFrame + 1) % walkFrameTextureNames.size();
            }
        } else {
             currentWalkFrame = 0; // Default if no frames/speed
        }
        // Reset idle state
        idleAnimationTimer = 0.0f; currentIdleFrame = 0;

        // +++ MOVEMENT DEBUG LOGGING START +++
        //SDL_Log("DEBUG_MOVE: Enemy %d Checking Completion | Progress: %.4f", id, moveProgress); // Reduce log spam
        // +++ MOVEMENT DEBUG LOGGING END +++

        if (moveProgress >= 1.0f) {
             // +++ MOVEMENT DEBUG LOGGING START +++
             SDL_Log("DEBUG_MOVE: Enemy %d COMPLETING Move | Old Logical: [%d,%d] | New Logical: [%d,%d]",
                     id, startTileX, startTileY, targetTileX, targetTileY);
             // +++ MOVEMENT DEBUG LOGGING END +++

             // Snap logical position
             x = targetTileX;
             y = targetTileY;
             // Snap visual position exactly
             visualX = targetTileX * tileWidth + tileWidth / 2.0f;
             visualY = targetTileY * tileHeight + tileHeight / 2.0f;

             // Reset movement state
             isMoving = false;
             moveProgress = 0.0f; // Reset progress
             moveTimer = 0.0f;    // Reset timer
             walkAnimationTimer = 0.0f; currentWalkFrame = 0; // Reset walk anim
             idleAnimationTimer = 0.0f; currentIdleFrame = 0; // Reset idle anim

             // +++ MOVEMENT DEBUG LOGGING START +++
             SDL_Log("DEBUG_MOVE: Enemy %d Move COMPLETE. isMoving = false.", id);
             // +++ MOVEMENT DEBUG LOGGING END +++

        } else {
             // +++ MOVEMENT DEBUG LOGGING START +++
            // SDL_Log("DEBUG_MOVE: Enemy %d Interpolating | Progress: %.4f", id, moveProgress); // Can be spammy
             // +++ MOVEMENT DEBUG LOGGING END +++

            // Interpolate visual position
            float startVisualX = startTileX * tileWidth + tileWidth / 2.0f;
            float startVisualY = startTileY * tileHeight + tileHeight / 2.0f;
            float targetVisualX = targetTileX * tileWidth + tileWidth / 2.0f;
            float targetVisualY = targetTileY * tileHeight + tileHeight / 2.0f;

            visualX = startVisualX + (targetVisualX - startVisualX) * moveProgress;
            visualY = startVisualY + (targetVisualY - startVisualY) * moveProgress;
        }
    }
    // --- Update Idle Animation (Only if not attacking AND not moving) ---
    else { // Enemy is Idle
        // Update Idle Animation
        if (!idleFrameTextureNames.empty() && idleAnimationSpeed > 0) {
             idleAnimationTimer += deltaTime;
             float idleFrameDuration = 1.0f / idleAnimationSpeed;
             if (idleFrameDuration > 0 && idleAnimationTimer >= idleFrameDuration) {
                 idleAnimationTimer -= idleFrameDuration;
                 currentIdleFrame = (currentIdleFrame + 1) % idleFrameTextureNames.size();
             }
        } else {
             currentIdleFrame = 0; // Default if no frames/speed
        }

        // Reset other animation states
        walkAnimationTimer = 0.0f; currentWalkFrame = 0;
        attackAnimationTimer = 0.0f; currentAttackFrame = 0;
        // Ensure visual matches logical when idle
        visualX = x * tileWidth + tileWidth / 2.0f;
        visualY = y * tileHeight + tileHeight / 2.0f;
    }
}


void Enemy::startAttackAnimation(const GameData& gameData) { // Now requires GameData
    if (!isAttacking && !isMoving) {
        SDL_Log("Enemy %d starting attack animation.", id);
        isAttacking = true;
        attackAnimationTimer = 0.0f;
        currentAttackFrame = 0;

        // --- Capture Start and Target Positions ---
        attackStartX = visualX; // Current visual position is the start
        attackStartY = visualY;
        // Target the player's current visual position
        attackTargetX = gameData.currentGamePlayer.x;
        attackTargetY = gameData.currentGamePlayer.y;
        SDL_Log("   Attack Start: [%.1f, %.1f], Target: [%.1f, %.1f]", attackStartX, attackStartY, attackTargetX, attackTargetY);
        // ------------------------------------------

        // Reset other animations
        idleAnimationTimer = 0.0f; currentIdleFrame = 0;
        walkAnimationTimer = 0.0f; currentWalkFrame = 0;
    } else {
         SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy %d failed to start attack animation (already attacking/moving?).", id);
    }
}

// --- startMove Implementation (Update facing direction) ---
void Enemy::startMove(int targetX, int targetY) {
  // Only start if not already moving and target is different
  if (!isMoving && (targetX != x || targetY != y)) {
    startTileX = x; // Current logical pos is the start
    startTileY = y;
    targetTileX = targetX; // Target logical pos
    targetTileY = targetY;
    isMoving = true;
    moveProgress = 0.0f;
    moveTimer = 0.0f;

    // +++ Update Facing Direction +++
    if (targetTileX > startTileX) {
        currentFacingDirection = FacingDirection::Right;
    } else if (targetTileX < startTileX) {
        currentFacingDirection = FacingDirection::Left;
    }
    // If targetTileX == startTileX, direction remains unchanged (vertical move)
    // +++++++++++++++++++++++++++++++

    SDL_Log("Enemy %d starting move animation from [%d,%d] to [%d,%d] (Facing: %s)", id,
            startTileX, startTileY, targetTileX, targetTileY,
            (currentFacingDirection == FacingDirection::Right ? "Right" : "Left"));
  } else if (isMoving) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Enemy %d told to startMove while already moving.", id);
  } else {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Enemy %d told to startMove to its current location [%d,%d].",
                id, x, y);
  }
}

// --- Render Implementation (Use SDL_RenderCopyEx for flipping) ---
void Enemy::render(SDL_Renderer *renderer, AssetManager &assets, int cameraX,
                   int cameraY, float visibilityAlpha) const {

  SDL_Texture *textureToRender = nullptr;
  std::string keyToUse;

  // Determine which texture to use: Attack -> Walk -> Idle -> Base
  if (isAttacking && !attackFrameTextureNames.empty()) {
    // Use current attack frame if attacking and frames available
    if (currentAttackFrame >= 0 &&
        currentAttackFrame < attackFrameTextureNames.size()) {
      keyToUse = attackFrameTextureNames[currentAttackFrame];
    } else {
      keyToUse = textureName; // Fallback
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "Enemy %d: Invalid attack frame index %d.", id,
                  currentAttackFrame);
    }
  } else if (isMoving && !walkFrameTextureNames.empty()) {
    // Use current walk frame if moving and frames available
    if (currentWalkFrame >= 0 &&
        currentWalkFrame < walkFrameTextureNames.size()) {
      keyToUse = walkFrameTextureNames[currentWalkFrame];
    } else {
      keyToUse = textureName; // Fallback
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "Enemy %d: Invalid walk frame index %d.", id,
                  currentWalkFrame);
    }
  } else if (!isMoving && !idleFrameTextureNames.empty()) {
    // Use current idle frame if idle and frames available
    if (currentIdleFrame >= 0 &&
        currentIdleFrame < idleFrameTextureNames.size()) {
      keyToUse = idleFrameTextureNames[currentIdleFrame];
    } else {
      keyToUse = textureName; // Fallback
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "Enemy %d: Invalid idle frame index %d.", id,
                  currentIdleFrame);
    }
  } else {
    // Use base texture if no specific animation applies or frames are missing
    keyToUse = textureName;
  }

  // Get the texture from the asset manager
  if (!keyToUse.empty()) {
    textureToRender = assets.getTexture(keyToUse);
  }

  // Calculate destination rectangle
  SDL_Rect destRect;
  destRect.w = width;
  destRect.h = height;
  // Center the texture on the visual position
  destRect.x = static_cast<int>(visualX - destRect.w / 2.0f) - cameraX;
  destRect.y = static_cast<int>(visualY - destRect.h / 2.0f) - cameraY;

  // Render the chosen texture or fallback
  if (textureToRender) {
    Uint8 alpha = static_cast<Uint8>(visibilityAlpha * 255);
    SDL_SetTextureAlphaMod(textureToRender, alpha);
    SDL_SetTextureBlendMode(textureToRender, SDL_BLENDMODE_BLEND);

    // +++ Determine Flip based on Facing Direction +++
    SDL_RendererFlip flip = SDL_FLIP_NONE; // Default: no flip (faces left)
    if (currentFacingDirection == FacingDirection::Right) {
        flip = SDL_FLIP_HORIZONTAL; // Flip horizontally if facing right
    }
    // ++++++++++++++++++++++++++++++++++++++++++++++++

    // +++ Use SDL_RenderCopyEx with Flip +++
    SDL_RenderCopyEx(renderer, textureToRender,
                     nullptr,     // Source rect (entire texture)
                     &destRect,   // Destination rect
                     0.0,         // Angle (no rotation)
                     nullptr,     // Center of rotation (not needed)
                     flip);       // Apply flip
    // ++++++++++++++++++++++++++++++++++++++

  } else {
    // Fallback rendering
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Enemy texture key '%s' not found for enemy %d, drawing "
                "fallback rectangle.",
                keyToUse.c_str(), id);
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
  } else {
      SDL_Log("Enemy %d took %d damage. Health: %d/%d", id, amount, health, maxHealth);
  }
}

int Enemy::GetAttackDamage() const {
  // For now, just return the base damage set in the constructor based on type
  return baseAttackDamage;
}

void Enemy::applyFloorScaling(int currentFloorIndex, float scalingFactorPerFloor) {
  if (currentFloorIndex <= 1) {
      // No scaling on floor 1 or invalid floors
      return;
  }

  // Calculate the multiplier: 1.0 + (floors above 1) * scaling_factor
  // Example: Floor 2 -> 1.0 + (2-1)*0.05 = 1.05
  // Example: Floor 3 -> 1.0 + (3-1)*0.05 = 1.10
  float multiplier = pow(1.0f + scalingFactorPerFloor,static_cast<float>(currentFloorIndex - 1));

  // Store original values for logging if needed
  int oldMaxHealth = maxHealth;
  int oldDamage = baseAttackDamage;
  int oldArcana = arcanaValue;

  // Scale stats (round to nearest integer for health/damage/arcana)
  maxHealth = static_cast<int>(std::round(static_cast<float>(maxHealth) * multiplier));
  baseAttackDamage = static_cast<int>(std::round(static_cast<float>(baseAttackDamage) * multiplier));
  arcanaValue = static_cast<int>(std::round(static_cast<float>(arcanaValue) * multiplier));

  // Ensure stats don't become zero if they were non-zero before scaling very slightly down due to rounding
  if (oldMaxHealth > 0 && maxHealth <= 0) maxHealth = 1;
  if (oldDamage > 0 && baseAttackDamage <= 0) baseAttackDamage = 1;
  if (oldArcana > 0 && arcanaValue <= 0) arcanaValue = 1;


  // Reset current health to the new max health after scaling
  health = maxHealth;

  SDL_Log("INFO: Enemy %d scaled for Floor %d (Multiplier: %.2f). HP: %d -> %d, DMG: %d -> %d, Arcana: %d -> %d",
          id, currentFloorIndex, multiplier, oldMaxHealth, maxHealth, oldDamage, baseAttackDamage, oldArcana, arcanaValue);
}
