// projectile.cpp
#include "projectile.h"
#include "enemy.h"     // Include Enemy to access its members (x, y, health)
#include "game_data.h" // Include GameData to access enemy list in update
#include <SDL.h>       // For SDL_Log if debugging
#include <cmath>       // For sqrt, atan2 (optional), or just normalization

// --- Updated Constructor ---
Projectile::Projectile(ProjectileType pType, SDL_Texture *tex, int w, int h,
                       float sX, float sY, float tX, float tY, float spd,
                       int dmg,
                       int spellIdx,            // <<< ADDED parameter
                       int targetId /* = -1 */) // Added targetId
    : type(pType), isActive(true),
      sourceSpellIndex(spellIdx), // <<< INITIALIZE member
      targetEnemyId(targetId),    // Store the target ID
      startX(sX), startY(sY), targetX(tX),
      targetY(tY), // Store initial target coords
      currentX(sX), currentY(sY), speed(spd), damage(dmg), dx(0.0f), dy(0.0f),
      texture(tex), width(w), height(h) {
  if (!texture) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Projectile created with null texture!");
    isActive = false;
  }
  // Calculate initial direction towards the initial target point
  calculateDirection(targetX, targetY);
}

// --- Updated calculateDirection Helper ---
// Calculates direction towards a specific point (toX, toY)
void Projectile::calculateDirection(float toX, float toY) {
  float vecX = toX - currentX; // Use currentX as the origin
  float vecY = toY - currentY; // Use currentY as the origin
  float magnitude = std::sqrt(vecX * vecX + vecY * vecY);

  if (magnitude > 0.0001f) { // Avoid division by zero
    dx = vecX / magnitude;
    dy = vecY / magnitude;
  } else {
    // Target is same as current? Stop moving in this direction.
    dx = 0.0f;
    dy = 0.0f;
    // Optionally deactivate if direction becomes zero?
    // isActive = false;
    // SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Projectile direction
    // calculation resulted in zero magnitude.");
  }
}

// --- Updated update Method ---
// Updates position, returns true if target reached/hit this frame
bool Projectile::update(float deltaTime, const GameData &gameData) {
  if (!isActive) {
    return false; // Don't update inactive projectiles
  }

  float currentTargetX = targetX; // Default to initial target
  float currentTargetY = targetY;
  bool targetFoundAndAlive = false;

  // --- Homing Logic ---
  if (targetEnemyId != -1) {
    const Enemy *targetEnemy = nullptr;
    // Find the target enemy in the game data
    for (const auto &enemy : gameData.enemies) {
      if (enemy.id == targetEnemyId) {
        targetEnemy = &enemy;
        break;
      }
    }

    // Check if found and alive
    if (targetEnemy != nullptr && targetEnemy->health > 0) {
      // Update target coordinates to the enemy's current visual position
      currentTargetX = targetEnemy->visualX;
      currentTargetY = targetEnemy->visualY;
      targetFoundAndAlive = true;
      // Recalculate direction towards the moving target each frame
      calculateDirection(currentTargetX, currentTargetY);
    } else {
      // Target died or disappeared while projectile was in flight
      SDL_Log("Projectile target Enemy ID %d not found or dead. Deactivating.",
              targetEnemyId);
      isActive = false;
      return false; // Deactivate and stop processing this frame
    }
  }
  // --- End Homing Logic ---

  // Calculate distance moved this frame
  float distanceMoved = speed * deltaTime;

  // Calculate remaining distance to the *current* target position
  float remainingX = currentTargetX - currentX;
  float remainingY = currentTargetY - currentY;
  float remainingDistSq = remainingX * remainingX + remainingY * remainingY;

  // Check for arrival/hit
  // Use a small threshold for hitting moving targets
  float hitThresholdSq =
      10.0f * 10.0f; // Example: Hit if within 10 pixels squared

  // Check if we will reach or pass the target this frame OR if we are already
  // very close
  if ((distanceMoved * distanceMoved >= remainingDistSq) ||
      (remainingDistSq < hitThresholdSq)) {
    // Reached target (or close enough)
    currentX = currentTargetX; // Snap to final target position
    currentY = currentTargetY;
    isActive = false; // Mark for removal / effect application
    SDL_Log("Projectile reached/hit target (EnemyID: %d).", targetEnemyId);
    return true; // Indicate target reached/hit
  } else {
    // Move along the (potentially updated) direction vector
    currentX += dx * distanceMoved;
    currentY += dy * distanceMoved;
    return false; // Still moving
  }
}

// --- render Method (Unchanged) ---
void Projectile::render(SDL_Renderer *renderer, int cameraX,
                        int cameraY) const {
  if (!isActive || !texture) {
    return;
  }
  SDL_Rect destRect;
  destRect.x = static_cast<int>(currentX - width / 2.0f) - cameraX;
  destRect.y = static_cast<int>(currentY - height / 2.0f) - cameraY;
  destRect.w = width;
  destRect.h = height;
  SDL_RenderCopy(renderer, texture, nullptr, &destRect);
}
