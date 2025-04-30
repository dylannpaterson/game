#include "orbital_missile.h"
#include "asset_manager.h" // Include for rendering
#include "character.h"     // Include for PlayerCharacter members (x, y)
#include "enemy.h"         // Include for Enemy members
#include "game_data.h"     // Include full definition for GameData members
#include "projectile.h"    // Include for creating launched projectiles
#include "utils.h"         // For isWithinBounds, potentially distance calcs
#include <cmath>           // For std::round, std::abs, std::sqrt, cos, sin
#include <limits>          // For numeric_limits
#include <vector>

// Constructor Implementation
OrbitalMissile::OrbitalMissile(float startX, float startY, float lifetime,
                               int acqRange, int dmgNumDice, int dmgDie,
                               int dmgBonus, const std::string &projTexKey,
                               float projSpeed, int formIndex)
    : visualX(startX), visualY(startY), lifetimeRemaining(lifetime),
      scanTimer(0.1f), // Start scan quickly
      formationIndex(formIndex),
      acquisitionRangeSq(acqRange * acqRange), // Store squared range
      damageNumDice(dmgNumDice), damageDieType(dmgDie), damageBonus(dmgBonus),
      projectileTextureKey(projTexKey), projectileSpeed(projSpeed),
      currentState(OrbitalState::Waiting), markedForRemoval(false) {}

// Update Implementation (Needs careful implementation of scanning logic)
bool OrbitalMissile::update(float deltaTime, GameData &gameData,
                            AssetManager &assets) {
  if (markedForRemoval)
    return true;

  // 1. Update lifetime
  lifetimeRemaining -= deltaTime;
  if (lifetimeRemaining <= 0.0f) {
    SDL_Log("DEBUG: Orbital %d lifetime expired.", formationIndex);
    this->markedForRemoval = true;
    return true;
  }

  // 2. Update Position (Float above player)
  const PlayerCharacter &owner = gameData.currentGamePlayer;
  float ownerX = owner.x;
  float ownerY = owner.y;

  float verticalOffset = -64.0f;   // Adjust as needed
  float horizontalSpacing = 32.0f; // Adjust as needed
  // --- More robust centering logic requires knowing total orbitals ---
  // Find total active orbitals for *this player* (currently only 1 player)
  int totalOrbitals = 0;
  for (const auto &orb : gameData.activeOrbitals) {
    // In single player, all orbitals belong to player. In multiplayer, check
    // ownerId if added back.
    if (!orb.markedForRemoval && orb.currentState == OrbitalState::Waiting) {
      totalOrbitals++;
    }
  }
  if (totalOrbitals <= 0)
    totalOrbitals = 1; // Avoid division by zero if this is the last one
  // Calculate centered offset: (index - (total - 1) / 2.0) * spacing
  float centeredHorizontalOffset =
      (static_cast<float>(formationIndex) -
       (static_cast<float>(totalOrbitals - 1) / 2.0f)) *
      horizontalSpacing;

  this->visualX = ownerX + centeredHorizontalOffset;
  this->visualY = ownerY + verticalOffset;
  // --- End Position Update ---

  // 3. Scan for Targets (if Waiting)
  scanTimer -= deltaTime;
  if (currentState == OrbitalState::Waiting && scanTimer <= 0.0f) {
    scanTimer = 0.25f; // Reset scan timer

    const Enemy *nearestEnemy = nullptr;
    int nearestDistSq = acquisitionRangeSq; // Max range is initial nearest

    // <<< ADDED: Log scan initiation >>>
    SDL_Log(
        "DEBUG: Orbital %d scanning (RangeSq: %d). Player pos: (%.1f, %.1f)",
        formationIndex, acquisitionRangeSq, owner.x, owner.y);

    // Scan enemies relative to the *owner's* position
    for (const auto &enemy : gameData.enemies) {
      if (enemy.health <= 0)
        continue; // Skip dead enemies

      // <<< ADDED: Log enemy being considered >>>
      SDL_Log("DEBUG: Orbital %d considering Enemy %d at visual (%.1f, %.1f), "
              "logical [%d,%d]",
              formationIndex, enemy.id, enemy.visualX, enemy.visualY, enemy.x,
              enemy.y);

      // Check visibility? Optional, missiles might ignore line of sight
      float visibility = 0.0f;
      if (isWithinBounds(enemy.x, enemy.y, gameData.currentLevel.width,
                         gameData.currentLevel.height) &&
          enemy.y < gameData.visibilityMap.size() &&
          enemy.x < gameData.visibilityMap[enemy.y].size()) {
        visibility = gameData.visibilityMap[enemy.y][enemy.x];
      }
      if (visibility <= 0.0f)
        continue; // Skip unseen enemies (design choice)

      // <<< ADDED: Log visibility result >>>
      SDL_Log("DEBUG:  -> Visibility: %.2f", visibility);
      if (visibility <= 0.0f) {
        // <<< ADDED: Log skip reason >>>
        SDL_Log("DEBUG:  -> Skipping Enemy %d (Not Visible)", enemy.id);
        continue; // Skip unseen enemies (design choice)
      }

      // <<< MODIFIED: Calculate distance using TILE coordinates >>>
      int dx_tile = enemy.x - owner.targetTileX; // Difference in tile X
      int dy_tile = enemy.y - owner.targetTileY; // Difference in tile Y
      int distSq_tile =
          dx_tile * dx_tile + dy_tile * dy_tile; // Squared distance in tiles

      SDL_Log("DEBUG:  -> DistSq (Tiles) from Player: %d",
              distSq_tile); // Log tile distance

      // <<< Compare TILE distance squared with TILE range squared >>>
      if (distSq_tile < nearestDistSq) {
        SDL_Log("DEBUG:  -> Closer than current nearest (%d). Potential "
                "Target: Enemy %d",
                nearestDistSq, enemy.id);
        nearestDistSq =
            distSq_tile; // Update nearest with TILE distance squared
        nearestEnemy = &enemy;
      } else {
        SDL_Log("DEBUG:  -> Not closer than current nearest (%d).",
                nearestDistSq);
      }
    }

    // <<< ADDED: Log scan completion >>>
    if (nearestEnemy != nullptr) {
      SDL_Log("DEBUG: Orbital %d finished scan. Nearest Target FOUND: Enemy %d",
              formationIndex, nearestEnemy->id);
    } else {
      SDL_Log("DEBUG: Orbital %d finished scan. Nearest Target NOT FOUND.",
              formationIndex);
    }

    // 4. Launch if Target Acquired
    if (nearestEnemy != nullptr) {
      SDL_Log("DEBUG: Orbital %d acquired target Enemy %d", formationIndex,
              nearestEnemy->id);

      // --- Create Launched Projectile ---

      // <<< CALL Player's calculateSpellDamage using stored dice parameters >>>
      int finalDamage = owner.calculateSpellDamage(
          this->damageNumDice, // Use orbital's stored dice info
          this->damageDieType, this->damageBonus,
          nearestEnemy->x, // Target X tile coord
          nearestEnemy->y, // Target Y tile coord
          nearestEnemy     // Pass target pointer
      );

      // <<< Get the texture using AssetManager >>>
      SDL_Texture *projTexture = assets.getTexture(this->projectileTextureKey);

      // Check if texture was loaded successfully
      if (!projTexture) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Failed to get projectile texture '%s' for Orbital %d launch!",
            this->projectileTextureKey.c_str(), this->formationIndex);
        // Decide how to handle: skip launch, use fallback? Skipping for now.
        // return false; // Don't launch, don't mark for removal yet, maybe
        // retry next frame?
      } else {
        // <<< Add to main projectile list in GameData (MODIFIED) >>>
        gameData.activeProjectiles.emplace_back(
            ProjectileType::MagicMissile, // <<< USE CORRECT TYPE >>>
            projTexture,                  // <<< PASS THE TEXTURE POINTER >>>
            16, 16, // Placeholder size (Consider making this configurable)
            this->visualX, this->visualY, // Start from orbital's current pos
            nearestEnemy->visualX, nearestEnemy->visualY, // Initial target pos
            this->projectileSpeed, finalDamage,
            nearestEnemy->id // Homing target ID
        );

        // --- End Create Launched Projectile ---

        this->currentState = OrbitalState::Launched;
        this->markedForRemoval = true; // Remove after launching
        return true;                   // Indicate removal needed
      }
    } // End scan check
  }
  return this->markedForRemoval;
}

// Render Implementation
void OrbitalMissile::render(SDL_Renderer *renderer, AssetManager &assets,
                            int cameraX, int cameraY) const {
  if (currentState == OrbitalState::Waiting) { // Only render waiting ones
    SDL_Texture *orbitalTexture =
        assets.getTexture("magic_missile_orbiting"); // Use appropriate key
    if (orbitalTexture) {
      SDL_Rect orbitalRect;
      int orbitalSize = 32; // Example size
      SDL_QueryTexture(orbitalTexture, NULL, NULL, &orbitalRect.w,
                       &orbitalRect.h); // Use texture size?
      orbitalRect.w = orbitalSize;      // Or force size
      orbitalRect.h = orbitalSize;
      orbitalRect.x =
          static_cast<int>(std::round(visualX - orbitalRect.w / 2.0f)) -
          cameraX;
      orbitalRect.y =
          static_cast<int>(std::round(visualY - orbitalRect.h / 2.0f)) -
          cameraY;
      SDL_RenderCopy(renderer, orbitalTexture, nullptr, &orbitalRect);
    } else {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "Orbital texture 'magic_missile_orbiting' not found!");
    }
  }
}