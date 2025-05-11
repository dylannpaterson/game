// In character.cpp
#include "character.h"
#include "asset_manager.h"
#include "enemy.h"
#include "game_data.h"
#include "projectile.h"
#include "status_effect.h"
#include "utils.h"
#include "visibility.h"
#include <algorithm> // For std::max/min
#include <cmath>
#include <iostream> // For debugging output

// --- Constructor Implementation ---
PlayerCharacter::PlayerCharacter(CharacterType t, int initialTileX,
                                 int initialTileY, int tileW, int tileH)
    : type(t), level(1), // Start at level 1
      currentArcana(0),  // Start with 0 Arcana
      baseVitality(5),   // Example starting stats
      baseIntelligence(10), baseSpirit(7), baseAgility(8),
      x(initialTileX * tileW + tileW / 2.0f), // Center visual X
      y(initialTileY * tileH + tileH / 2.0f), // Center visual Y
      logicalTileX(initialTileX),             // <<< Initialize logical position
      logicalTileY(initialTileY),             // <<< Initialize logical position
      targetTileX(initialTileX),              // Initial logical position
      targetTileY(initialTileY), isMoving(false),
      startTileX(initialTileX), // Start is same as target initially
      startTileY(initialTileY), moveProgress(0.0f),
      // moveDuration initialized later by RecalculateStats
      idleAnimationTimer(0.0f), currentIdleFrame(0), moveTimer(0.0f),
      currentFacingDirection(FacingDirection::Left), tileWidth(tileW),
      tileHeight(tileH) {
  // Initialize known spells based on type
  if (type == CharacterType::FemaleMage || type == CharacterType::MaleMage) {
    knownSpells.emplace_back("Fireball", 7, 10, SpellTargetType::Enemy,
                             SpellEffectType::Damage, 6, 6, 0, // Damage: 6d6+0
                             0.05f, // 5% bonus dmg per tile beyond adjacent
                             "fireball_icon",
                             0, // AoE Radius (0 for single target)
                             StatusEffectType::None, // <<< APPLY Stunned
                             0                       // <<< DURATION 1 turn
    );
    knownSpells.emplace_back(
        "Ward", 20, SpellTargetType::Self, SpellEffectType::ApplyShield,
        50.0f, // Shield Magnitude (using baseHealAmount field)
        0.20f, // Decay 20% of max per turn
        "ward_icon");
    // ---> ADD Magic Missiles <---
    /*knownSpells.emplace_back(
        "Magic Missiles", 15, SpellTargetType::Self,
        SpellEffectType::SummonOrbital, 3, 6,
        500.0f,  // Summon 3 orbitals, 6 tile range, 3 sec lifetime
        2, 6, 0, // Payload: 2d6 damage
        "magic_missile_launched",
        700.0f,                 // Launched projectile texture key & speed
        "magic_missiles_icon"); // Icon for spell bar/menu

    knownSpells.emplace_back(
        "Blizzard", 40,              // High Cost
        8,                           // Range to center tile
        SpellTargetType::Tile,       // Target a tile for the center
        SpellEffectType::AreaDamage, // New Effect Type
        4, 8, 0,                     // Damage: 4d8+0 (Moderate)
        0.0f,                        // No distance bonus for AoE usually
        "blizzard_icon",             // Icon key
        1                            // AoE Radius (1 = 3x3 area)
    );
    // <<< DEFINE VORTEX >>>
    knownSpells.emplace_back(
        "Vortex", 10, // Mana Cost
        0,            // Range (Self targeted, uses AoE radius)
        SpellTargetType::Self,
        SpellEffectType::AreaPushbackStun, // The new effect type
        1, 6, 0,                           // 1d6 dice damage
        "vortex_icon",                     // <<< Need to create vortex_icon.png
        2, // AoE Radius (Affects 5x5 area around player) - Adjust as desired
        StatusEffectType::Stunned, // Status to apply
        1                          // Duration (1 turn)
    );*/
    // Lightning Bolt (Linear Damage)
    knownSpells.emplace_back(
        "Lightning Bolt", 15, 8, SpellTargetType::Tile,
        SpellEffectType::LinearDamage, // <<< Use new effect type
        3, 8, 0,                       // 3d8 damage
        -0.10f,                        // 10% less damage per tile distance
        "lightning_icon", 0, StatusEffectType::None, 0);
  }

  // CRITICAL: Calculate initial stats based on starting level and base stats
  RecalculateStats();

  // Set current health/mana to max initially
  health = maxHealth;
  mana = maxMana;

  // Idle animation frames
  // Initialize the vector of texture names
  if (type == CharacterType::FemaleMage) {
    for (int i = 0; i < 8; ++i) {
      idleFrameTextureNames.push_back("mage_idle_" + std::to_string(i));
    }

    for (int i = 0; i < 8; ++i) {
      walkFrameTextureNames.push_back("mage_walk_" + std::to_string(i));
    }
    for (int i = 8; i < 8; ++i) {
      targetingFrameTextureNames.push_back("mage_target_" + std::to_string(i));
    }

    for (int i = 0; i < 8; ++i) {
      wardFrameTextureKeys.push_back("ward_active_" + std::to_string(i));
    }
  } else if (type == CharacterType::MaleMage) {
    // Add male frames here if/when you create them
    idleFrameTextureNames = {/* e.g., "male_mage_idle_1", ... */};

    for (int i = 1; i <= 9; ++i) {
      wardFrameTextureKeys.push_back("ward_active_" + std::to_string(i));
    }
  }
}

// --- NEW: Getter Methods ---
int PlayerCharacter::GetEffectiveVitality() const {
  return baseVitality + (level - 1) * VITALITY_PER_LEVEL;
}

int PlayerCharacter::GetEffectiveIntelligence() const {
  return baseIntelligence + (level - 1) * INTELLIGENCE_PER_LEVEL;
}

int PlayerCharacter::GetEffectiveSpirit() const {
  return baseSpirit + (level - 1) * SPIRIT_PER_LEVEL;
}

int PlayerCharacter::GetEffectiveAgility() const {
  return baseAgility + (level - 1) * AGILITY_PER_LEVEL;
}

// --- NEW: Stat Calculation ---
void PlayerCharacter::RecalculateStats() {
  // Calculate effective core stats (using getters for clarity, though could
  // calculate directly)
  int effVit = GetEffectiveVitality();
  int effInt = GetEffectiveIntelligence();
  int effSpr = GetEffectiveSpirit();
  int effAgi = GetEffectiveAgility();

  // Calculate derived stats
  maxHealth = effVit * HP_PER_VITALITY;     // Example calculation
  maxMana = effInt * MANA_PER_INTELLIGENCE; // Example calculation

  // Ensure current health/mana don't exceed new maxes
  health = std::min(health, maxHealth);
  mana = std::min(mana, maxMana);
  if (health <= 0)
    health = 1; // Prevent accidental death from stat recalculation if max drops
                // below current

  manaRegenRate = effSpr * MANA_REGEN_PER_SPIRIT; // Example calculation

  // Calculate spell damage modifier (e.g., 1% increase per point above 10 Int)
  // Adjust this formula as desired!
  spellDamageModifier = 1.0f + std::max(0, effInt - 10) * 0.01f;

  // Calculate move duration (e.g., base duration reduced by agility)
  // Adjust this formula as desired! Make sure moveDuration doesn't become zero
  // or negative.
  moveDuration = std::max(
      0.05f,
      0.5f - (effAgi * SPEED_MOD_PER_AGILITY)); // Example: base 0.2s, faster
                                                // with agi, min 0.05s

  std::cout << "Stats Recalculated. Level: " << level << " EffVit: " << effVit
            << " EffInt: " << effInt << " EffSpr: " << effSpr
            << " EffAgi: " << effAgi << " MaxHP: " << maxHealth
            << " MaxMana: " << maxMana << " ManaRegen: " << manaRegenRate
            << " DmgMod: " << spellDamageModifier
            << " MoveDur: " << moveDuration << std::endl;
}

// --- NEW: Arcana Management ---
void PlayerCharacter::GainArcana(int amount) {
  if (amount <= 0)
    return;

  currentArcana += amount;
  std::cout << "Gained " << amount << " Arcana. Total: " << currentArcana
            << std::endl;

  // Check for Level Up
  int potentialNewLevel = (currentArcana / ARCANA_PER_LEVEL) +
                          1; // +1 because level 1 requires 0-999 Arcana
  if (potentialNewLevel > level) {
    int oldLevel = level;
    int oldMaxHealth = maxHealth; // *** STORE old max health ***
    int oldMaxMana = maxMana;     // *** STORE old max mana ***

    level = potentialNewLevel;
    std::cout << "Level Up! " << oldLevel << " -> " << level << std::endl;
    RecalculateStats();

    int healthIncrease = std::max(
        0, maxHealth -
               oldMaxHealth); // Calculate the difference (ensure non-negative)
    int manaIncrease = std::max(
        0,
        maxMana - oldMaxMana); // Calculate the difference (ensure non-negative)

    health = std::min(health + healthIncrease,
                      maxHealth); // Add the increase, clamp to new max
    mana = std::min(mana + manaIncrease,
                    maxMana); // Add the increase, clamp to new max
  }
}

bool PlayerCharacter::CanAffordArcana(int cost) const {
  return currentArcana >= cost;
}

bool PlayerCharacter::SpendArcana(int amount) {
  if (amount <= 0)
    return false; // Cannot spend zero or negative
  if (CanAffordArcana(amount)) {
    currentArcana -= amount;
    std::cout << "Spent " << amount << " Arcana. Remaining: " << currentArcana
              << std::endl;

    // Check for De-Level
    int potentialNewLevel = (currentArcana / ARCANA_PER_LEVEL) + 1;
    if (potentialNewLevel < level) {
      int oldLevel = level;
      level =
          std::max(1, potentialNewLevel); // Ensure level doesn't drop below 1
      std::cout << "De-Leveled! " << oldLevel << " -> " << level << std::endl;
      RecalculateStats();
    }
    return true; // Spend succeeded
  } else {
    std::cout << "Cannot afford to spend " << amount
              << " Arcana. Have: " << currentArcana << std::endl;
    return false; // Spend failed
  }
}

// --- NEW: Mana Regeneration ---
void PlayerCharacter::RegenerateMana(
    float timeStep /* Currently unused, assuming per-turn */) {
  if (mana >= maxMana) {
    fractionalMana = 0.0f; // Reset fraction if already at max mana
    return;                // No need to regenerate if full
  }

  // Add the calculated regen rate to our fractional accumulator
  fractionalMana += manaRegenRate;
  // SDL_Log("Regen Check: Rate=%.2f, FracMana=%.2f", manaRegenRate,
  // fractionalMana); // Optional debug

  // Check if the accumulated fraction is enough for one or more whole points of
  // mana
  if (fractionalMana >= 1.0f) {
    int manaToAdd =
        static_cast<int>(fractionalMana); // Get the whole number part

    // Add the whole points to the actual mana pool
    mana += manaToAdd;

    // Subtract the whole points that were added from the fractional part
    fractionalMana -= static_cast<float>(manaToAdd);

    // Clamp mana to maxMana
    mana = std::min(mana, maxMana);

    SDL_Log("Regen Applied: Added=%d, NewMana=%d/%d, RemainingFrac=%.2f",
            manaToAdd, mana, maxMana, fractionalMana);
  } else {
    // SDL_Log("Regen Accumulating: FracMana=%.2f", fractionalMana); // Optional
    // debug
  }
}

// --- Other existing methods (startMove, update for movement) likely unchanged
// for now --- Placeholder implementations for existing methods if needed
void PlayerCharacter::startMove(int newTargetX, int newTargetY) {
  // Only start if not moving and target is different from *logical* position
  if (!isMoving && (newTargetX != logicalTileX || newTargetY != logicalTileY)) {
    isMoving = true;
    startTileX = logicalTileX; // Start animation from current logical tile
    startTileY = logicalTileY;
    targetTileX = newTargetX; // Set the destination target
    targetTileY = newTargetY;
    moveProgress = 0.0f;
    moveTimer = 0.0f;
    // Update facing direction based on the intended move
    if (targetTileX > startTileX) {
      currentFacingDirection = FacingDirection::Right;
    } else if (targetTileX < startTileX) {
      currentFacingDirection = FacingDirection::Left;
    }

    SDL_Log("DEBUG: Player startMove initiated. From logical [%d,%d] to target "
            "[%d,%d]",
            startTileX, startTileY, targetTileX, targetTileY);
  } else if (isMoving) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Player told to startMove while already moving.");
  } else {
    SDL_LogWarn(
        SDL_LOG_CATEGORY_APPLICATION,
        "Player told to startMove to their current logical location [%d,%d].",
        logicalTileX, logicalTileY);
  }
}

void PlayerCharacter::update(float deltaTime, GameData &gameData) {
  // ---> ADD Ward Animation Update <---
  if (currentShield > 0 && !wardFrameTextureKeys.empty()) {
    wardAnimationTimer += deltaTime;
    // Ensure speed is positive to avoid division by zero
    float effectiveWardAnimSpeed =
        std::max(0.1f, wardAnimationSpeed); // Prevent speed <= 0
    float wardFrameDuration = 1.0f / effectiveWardAnimSpeed;

    if (wardAnimationTimer >= wardFrameDuration) {
      wardAnimationTimer -=
          wardFrameDuration; // Subtract duration, don't just reset
      currentWardFrame = (currentWardFrame + 1) % wardFrameTextureKeys.size();
    }
  } else {
    // Reset ward animation if shield is gone or no frames defined
    wardAnimationTimer = 0.0f;
    currentWardFrame = 0;
  }
  // --- END Ward Animation Update ---
  if (isMoving) {
    // --- Handle Movement Interpolation ---
    moveTimer += deltaTime;
    moveProgress =
        moveTimer / moveDuration; // moveDuration is now potentially dynamic

    // Clamp progress to avoid overshooting
    moveProgress = std::min(moveProgress, 1.0f);

    // Interpolate visual position during movement
    float startVisualX = startTileX * tileWidth + tileWidth / 2.0f;
    float startVisualY = startTileY * tileHeight + tileHeight / 2.0f;
    float targetVisualX = targetTileX * tileWidth + tileWidth / 2.0f;
    float targetVisualY = targetTileY * tileHeight + tileHeight / 2.0f;

    x = startVisualX + (targetVisualX - startVisualX) * moveProgress;
    y = startVisualY + (targetVisualY - startVisualY) * moveProgress;

    // --- Update Walking Animation ---
    if (!walkFrameTextureNames.empty()) {
      walkAnimationTimer += deltaTime;
      float walkFrameDuration = 1.0f / walkAnimationSpeed;
      if (walkAnimationTimer >= walkFrameDuration) {
        walkAnimationTimer -= walkFrameDuration; // Subtract duration
        currentWalkFrame =
            (currentWalkFrame + 1) % walkFrameTextureNames.size();
        // Log walk frame change
        SDL_Log("DEBUG: [PlayerUpdate] Walk Frame Updated. isMoving=%s, "
                "NewFrame=%d",
                isMoving ? "true" : "false", currentWalkFrame);
      }
    } else {
      // Log only once if walk frames are missing
      if (walkAnimationTimer == 0.0f) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Player is moving but walkFrameTextureNames is empty!");
      }
      walkAnimationTimer +=
          deltaTime; // Still increment timer to avoid spamming log
    }
    // --- END Walking Animation Update ---

    // Reset idle animation state while moving
    idleAnimationTimer = 0.0f;
    currentIdleFrame = 0;

    // --- ADDED: Per-Frame Visibility Update ---
    // Calculate the TILE the player is currently visually over
    int currentVisualTileX = static_cast<int>(floor(x / tileWidth));
    int currentVisualTileY = static_cast<int>(floor(y / tileHeight));

    // Ensure coordinates are within bounds before updating visibility
    if (isWithinBounds(currentVisualTileX, currentVisualTileY,
                       gameData.currentLevel.width,
                       gameData.currentLevel.height)) {
      // Optional Log: Can be very spammy!
      // SDL_Log("DEBUG: [PlayerUpdate Moving] Updating visibility from visual
      // tile [%d, %d]", currentVisualTileX, currentVisualTileY);
      updateVisibility(
          gameData.currentLevel, gameData.levelRooms,
          currentVisualTileX, // Use current visual tile X
          currentVisualTileY, // Use current visual tile Y
          gameData.hallwayVisibilityDistance,
          gameData.visibilityMap); // Pass the visibility map from gameData
    }
    // --- END Per-Frame Visibility Update ---

    // --- Check for Movement Completion ---
    if (moveProgress >= 1.0f) {
      moveProgress = 1.0f; // Clamp progress

      int oldTileX = targetTileX; // Use the LOGICAL start tile for grid update
      int oldTileY =
          targetTileY; // (targetTileX/Y holds the *intended* destination)
                       // NOTE: The ORIGINAL startTileX/Y is needed if player
                       // could change direction mid-move, but for turn-based,
                       // targetTileX/Y before snapping IS the old logical
                       // position.

      // Snap visual position to target
      x = targetTileX * tileWidth + tileWidth / 2.0f;
      y = targetTileY * tileHeight + tileHeight / 2.0f;

      // <<< UPDATE LOGICAL POSITION UPON COMPLETION >>>
      logicalTileX = targetTileX;
      logicalTileY = targetTileY;
      SDL_Log("... Logical position updated to [%d,%d].", logicalTileX,
              logicalTileY);
      // <<< END LOGICAL POSITION UPDATE >>>

      // --- Update Occupation Grid ---
      // This part assumes the grid was ALREADY updated when the move *started*.
      // The purpose here might be more complex if concurrent movement required
      // re-checking occupation upon completion.
      // If the grid is only updated *after* movement, the logic would be
      // different. Based on previous context (immediate grid update on move
      // start), this might just need verification or could be removed if
      // redundant. Let's keep the original grid logic from your file for now:

      // Clear old position (check bounds) - Assuming oldTileX/Y correctly
      // represents the tile before this move finished
      if (oldTileX >= 0 && oldTileX < gameData.currentLevel.width &&
          oldTileY >= 0 && oldTileY < gameData.currentLevel.height) {
        // Check if it was *supposed* to be occupied by the player before
        // clearing This check might be overly complex depending on grid update
        // strategy. gameData.occupationGrid[oldTileY][oldTileX] = false; //
        // Simple clear
      }
      // Set new position (check bounds) - This should already be true from move
      // initiation
      if (targetTileX >= 0 && targetTileX < gameData.currentLevel.width &&
          targetTileY >= 0 && targetTileY < gameData.currentLevel.height) {
        if (!gameData.occupationGrid[targetTileY][targetTileX]) {
          SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                      "Player finished move at [%d,%d] but grid wasn't marked "
                      "occupied!",
                      targetTileX, targetTileY);
          gameData.occupationGrid[targetTileY][targetTileX] =
              true; // Ensure it's set
        }
      } else {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "Player moved outside level bounds to (%d, %d)? Grid not updated.",
            targetTileX, targetTileY);
      }
      // --- End Occupation Grid Update ---

      // Finish movement state
      isMoving = false;
      currentWalkFrame = 0;      // Reset walk animation frame
      walkAnimationTimer = 0.0f; // Reset walk timer
      // Log movement stop
      SDL_Log(
          "DEBUG: [PlayerUpdate] Movement Complete. isMoving=false at [%d,%d]",
          targetTileX, targetTileY);

      // Perform one final visibility update from the exact destination tile
      SDL_Log("DEBUG: [PlayerUpdate Completed] Final visibility update from "
              "[%d, %d]",
              targetTileX, targetTileY);
      updateVisibility(gameData.currentLevel, gameData.levelRooms, logicalTileX,
                       logicalTileY, // Use final logical position
                       gameData.hallwayVisibilityDistance,
                       gameData.visibilityMap);

    } else {
      // Interpolate visual position during movement
      float startVisualX = startTileX * tileWidth + tileWidth / 2.0f;
      float startVisualY = startTileY * tileHeight + tileHeight / 2.0f;
      float targetVisualX = targetTileX * tileWidth + tileWidth / 2.0f;
      float targetVisualY = targetTileY * tileHeight + tileHeight / 2.0f;

      x = startVisualX + (targetVisualX - startVisualX) * moveProgress;
      y = startVisualY + (targetVisualY - startVisualY) * moveProgress;
    }

  } else { // Player is NOT moving
    walkAnimationTimer = 0.0f;
    currentWalkFrame = 0;

    // --- Determine Idle vs. Targeting State ---
    if (gameData.showTargetingReticle) { // Check if player is targeting
      // --- Update Targeting Animation ---
      if (!targetingFrameTextureNames.empty()) {
        targetingAnimationTimer += deltaTime;
        float targetingFrameDuration = 1.0f / targetingAnimationSpeed;
        if (targetingAnimationTimer >= targetingFrameDuration) {
          targetingAnimationTimer -= targetingFrameDuration;
          currentTargetingFrame =
              (currentTargetingFrame + 1) % targetingFrameTextureNames.size();
          // Optional Log: Log targeting frame change
          // SDL_Log("DEBUG: [PlayerUpdate] Targeting Frame Updated. Frame=%d",
          // currentTargetingFrame);
        }
      } else {
        if (targetingAnimationTimer == 0.0f) {
          SDL_LogWarn(
              SDL_LOG_CATEGORY_APPLICATION,
              "Player is targeting but targetingFrameTextureNames is empty!");
        }
        targetingAnimationTimer += deltaTime;
      }
      // Reset idle animation state while targeting
      idleAnimationTimer = 0.0f;
      currentIdleFrame = 0;

    } else { // Player is idle (not moving, not targeting)
      // --- Update Idle Animation ---
      if (!idleFrameTextureNames.empty()) {
        idleAnimationTimer += deltaTime;
        float idleFrameDuration = 1.0f / idleAnimationSpeed;
        if (idleAnimationTimer >= idleFrameDuration) {
          idleAnimationTimer -= idleFrameDuration;
          currentIdleFrame =
              (currentIdleFrame + 1) % idleFrameTextureNames.size();
          // Optional Log: Log idle frame change
          // SDL_Log("DEBUG: [PlayerUpdate] Idle Frame Updated. Frame=%d",
          // currentIdleFrame);
        }
      } else {
        if (idleAnimationTimer == 0.0f) {
          SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                      "Player is idle but idleFrameTextureNames is empty!");
        }
        idleAnimationTimer += deltaTime;
      }
      // Reset targeting animation state while idle
      targetingAnimationTimer = 0.0f;
      currentTargetingFrame = 0;
    }
    // --- End Idle vs. Targeting ---

    // Ensure visual position matches logical position when idle
    x = logicalTileX * tileWidth + tileWidth / 2.0f;
    y = logicalTileY * tileHeight + tileHeight / 2.0f;

  } // End if(isMoving) / else block
} // End of PlayerCharacter::update

// Placeholder for canCastSpell
bool PlayerCharacter::canCastSpell(int spellIndex) const {
  if (spellIndex < 0 || spellIndex >= knownSpells.size()) {
    return false;
  }
  const Spell &spell = knownSpells[spellIndex];
  return mana >= GetEffectiveManaCost(spellIndex); // Check against current mana
}
// Placeholder for getSpell
const Spell &PlayerCharacter::getSpell(int spellIndex) const {
  if (spellIndex < 0 || spellIndex >= knownSpells.size()) {
    throw std::out_of_range("Invalid spell index in getSpell");
  }
  return knownSpells[spellIndex];
}
void PlayerCharacter::takeDamage(int amount) {
  if (amount <= 0)
    return; // No damage taken

  SDL_Log("DEBUG: Player taking %d damage. Current Shield: %d, Health: %d/%d",
          amount, currentShield, health, maxHealth);

  // Apply damage to shield first
  if (currentShield > 0) {
    int absorbedByShield = std::min(amount, currentShield);
    currentShield -= absorbedByShield;
    amount -= absorbedByShield; // Reduce damage amount

    SDL_Log(
        "DEBUG: Shield absorbed %d damage. Shield Left: %d. Remaining Dmg: %d",
        absorbedByShield, currentShield, amount);

    // If shield drops to 0, reset decay amount
    if (currentShield <= 0) {
      currentShield = 0; // Ensure it's exactly 0
      shieldDecayPerTurn = 0;
      SDL_Log("DEBUG: Shield depleted.");
    }
  }

  // Apply remaining damage to health
  if (amount > 0) {
    health -= amount;
    SDL_Log("DEBUG: Applied %d damage to health. Health: %d/%d", amount, health,
            maxHealth);
    if (health <= 0) {
      health = 0; // Prevent negative health
      SDL_Log("Player has been defeated!");
      // Handle player death logic here
    }
  }
}
// Placeholder for castSpell - NEEDS LATER MODIFICATION for damage based on Int
bool PlayerCharacter::castSpell(int spellIndex, int castTargetX,
                                int castTargetY, GameData &gameData,
                                AssetManager *assets) // Changed parameter type
{
  // 1. Validate Spell Index and Mana Cost
  if (spellIndex < 0 || spellIndex >= knownSpells.size()) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "CastSpell: Invalid spell index %d", spellIndex);
    return false;
  }
  const Spell &spell = knownSpells[spellIndex];
  if (!canCastSpell(spellIndex)) {
    SDL_Log("CastSpell: Cannot cast '%s', not enough mana (%d/%d).",
            spell.name.c_str(), mana, GetEffectiveManaCost(spellIndex));
    return false;
  }

  std::vector<Enemy> &enemies = gameData.enemies;
  std::vector<Projectile> &projectiles = gameData.activeProjectiles;

  // --- Calculate Effective Attributes (Example Platzholders) ---
  // Here you would apply modifiers based on player stats, level, buffs etc.
  // For now, we just use the base values from the spell struct.
  int effectiveManaCost = spell.baseManaCost; // - GetManaCostReduction();
  int effectiveRange = GetEffectiveSpellRange(spellIndex); // + GetRangeBonus();
  // --- End Effective Attribute Calculation ---

  // 2. Check Effective Mana Cost
  if (mana < effectiveManaCost) {
    SDL_Log("CastSpell: Cannot cast '%s', not enough mana (%d/%d).",
            spell.name.c_str(), mana, effectiveManaCost);
    return false;
  }

  // 3. Check Effective Range (if applicable)
  if (spell.targetType != SpellTargetType::Self) {
    int dx = targetTileX - castTargetX;
    int dy = targetTileY - castTargetY;
    int distSq = dx * dx + dy * dy; // L2 norm squared

    if (distSq > effectiveRange * effectiveRange) {
      SDL_Log("CastSpell: Target [%d,%d] out of effective range for '%s' "
              "(Range: %d, Dist: %d).",
              castTargetX, castTargetY, spell.name.c_str(), effectiveRange,
              distSq);
      return false; // Target out of range
    }
  }

  // 4. Deduct Mana Cost (use effective cost)
  mana -= effectiveManaCost;
  SDL_Log("CastSpell: Spent %d mana for '%s'. Remaining: %d/%d",
          effectiveManaCost, spell.name.c_str(), mana, maxMana);

  // Declare lineTiles outside the switch to avoid "jump to case label" error
  std::vector<std::pair<int, int>> lineTiles;

  // 5. Apply Spell Effect / Create Projectile
  bool effectApplied = false;
  switch (spell.effectType) {
  case SpellEffectType::Damage:
    if (spell.targetType == SpellTargetType::Enemy ||
        spell.targetType == SpellTargetType::Tile ||
        spell.targetType == SpellTargetType::Area) {
      // <<< MODIFIED: Use centralized calculation >>>
      // Find target enemy pointer (optional but useful for calc function)
      const Enemy *targetEnemyPtr = nullptr;
      int targetId = -1;
      if (spell.targetType == SpellTargetType::Enemy) {
        for (const auto &enemy : enemies) {
          if (enemy.health > 0 && enemy.x == castTargetX &&
              enemy.y == castTargetY) {
            targetEnemyPtr = &enemy;
            targetId = enemy.id; // Store ID for projectile homing
            break;
          }
        }
      }

      // Calculate final damage using the NEW method
      int finalDamage = this->calculateSpellDamage(spellIndex, castTargetX,
                                                   castTargetY, targetEnemyPtr);
      SDL_Log("DEBUG: CastSpell '%s' calculated final damage: %d",
              spell.name.c_str(), finalDamage);
      // Create Projectile - Logic unchanged, just pass rolledDamage
      std::string projectileTextureName; // = GetProjectileTextureName(spell);
                                         // // Map spell to texture
      if (spell.name == "Fireball")
        projectileTextureName = "fireball";
      else
        projectileTextureName = spell.iconName; // Fallback

      SDL_Texture *projTexture =
          assets ? assets->getTexture(projectileTextureName) : nullptr;

      if (projTexture) {
        float startVisualX = this->x;
        float startVisualY = this->y;
        float targetVisualX = castTargetX * tileWidth + tileWidth / 2.0f;
        float targetVisualY = castTargetY * tileHeight + tileHeight / 2.0f;
        float projectileSpeed = 600.0f;
        int projWidth = 32, projHeight = 32;
        ProjectileType pType = ProjectileType::Firebolt;

        // ---> Pass rolledDamage instead of calculatedDamage <---
        projectiles.emplace_back(pType, projTexture, projWidth, projHeight,
                                 startVisualX, startVisualY, targetVisualX,
                                 targetVisualY, projectileSpeed, finalDamage,
                                 spellIndex,
                                 targetId); // Use rolledDamage

        SDL_Log("CastSpell: Launched '%s' projectile towards [%d,%d] with %d "
                "potential damage.",
                spell.name.c_str(), castTargetX, castTargetY, finalDamage);
        effectApplied = true;
      } else { /* ... log texture warning ... */
      }
    } else { /* ... log target type warning ... */
    }
    break;

  case SpellEffectType::LinearDamage: // Handles linear damage like Lightning
                                      // Bolt
    SDL_Log(
        "CastSpell: Resolving Linear Damage for '%s' from [%d,%d] to [%d,%d]",
        spell.name.c_str(), logicalTileX, logicalTileY, castTargetX,
        castTargetY);

    // Get tiles along the line for damage application and visibility update
    { // Scope for line tiles calculation
      int effectiveRange = GetEffectiveSpellRange(spellIndex);
      // Calculate the endpoint at the full effective range in the direction of
      // the target
      float deltaX_tiles = static_cast<float>(castTargetX - logicalTileX);
      float deltaY_tiles = static_cast<float>(castTargetY - logicalTileY);
      float distance_to_target_tiles =
          std::sqrt(deltaX_tiles * deltaX_tiles + deltaY_tiles * deltaY_tiles);

      int endTileX_full_range = castTargetX;
      int endTileY_full_range = castTargetY;

      if (distance_to_target_tiles >
          0.001f) { // Avoid division by zero if target is on player tile
        float normalizedDeltaX = deltaX_tiles / distance_to_target_tiles;
        float normalizedDeltaY = deltaY_tiles / distance_to_target_tiles;

        endTileX_full_range = static_cast<int>(
            std::round(logicalTileX + normalizedDeltaX * effectiveRange));
        endTileY_full_range = static_cast<int>(
            std::round(logicalTileY + normalizedDeltaY * effectiveRange));
      } else {
        // If target is on the player tile, the line is just the player's tile.
        // Damage application loop will start from i=1, effectively skipping the
        // player's tile. The visual effect will still be a point at the
        // player's location.
        endTileX_full_range = logicalTileX;
        endTileY_full_range = logicalTileY;
      }

      lineTiles = getLineTiles(logicalTileX, logicalTileY, endTileX_full_range,
                               endTileY_full_range);
    } // End scope for line tiles calculation

    // Iterate through tiles along the line for damage (excluding the player's
    // tile)
    for (size_t i = 1; i < lineTiles.size(); ++i) {
      int currentTileX = lineTiles[i].first;
      int currentTileY = lineTiles[i].second;

      // Check if the tile is within level bounds
      if (!isWithinBounds(currentTileX, currentTileY,
                          gameData.currentLevel.width,
                          gameData.currentLevel.height)) {
        SDL_Log("... Tile [%d, %d] along line is out of bounds. Stopping "
                "damage application.",
                currentTileX, currentTileY);
        break; // Stop damage application if the line goes out of bounds
      }

      // --- Stop damage if it hits a wall --- // <<< ADDED
      if (gameData.currentLevel.tiles[currentTileY][currentTileX] == '#') {
        SDL_Log("... Linear Damage spell hit a wall at [%d, %d]. Stopping "
                "damage application.",
                currentTileX, currentTileY);
        // Optional: Spawn a wall impact effect here
        // gameData.activeEffects.emplace_back(...)
        lineTiles.erase(
            lineTiles.begin() + i,
            lineTiles.end()); // Truncate lineTiles for visual effect
        break;                // Stop iterating along the line if it hits a wall
      }
      // --- End wall collision check ---

      // Find if any enemy is on this tile and apply damage
      for (int enemyIdx = 0; enemyIdx < enemies.size(); ++enemyIdx) {
        Enemy &currentEnemy = enemies[enemyIdx];

        if (currentEnemy.health > 0 && currentEnemy.x == currentTileX &&
            currentEnemy.y == currentTileY) {
          // Found a living enemy on the line
          SDL_Log("... Linear Damage spell hitting Enemy %d at [%d, %d]",
                  currentEnemy.id, currentTileX, currentTileY);

          // Calculate damage for this specific enemy, applying distance penalty
          int damageToEnemy = this->calculateSpellDamage(
              spellIndex, currentTileX, currentTileY, &currentEnemy);

          SDL_Log("... Applying %d damage to Enemy %d", damageToEnemy,
                  currentEnemy.id);
          currentEnemy.takeDamage(damageToEnemy); // Apply damage

          effectApplied = true; // Mark that at least one enemy was hit
          // Continue to hit other enemies further down the line on the same
          // tile if necessary
        }
      } // End loop through enemies for one tile

      // --- Visibility Update for this tile (Handled by VisualEffect now) ---
      // The VisualEffect will manage the visibility of the tiles in its
      // 'affectedTiles' list over its lifetime. We no longer set visibility
      // here instantly.
      // --- End Visibility Update ---

    } // End loop through line tiles for damage

    // --- SPAWN VISUAL EFFECT (Lightning Bolt) ---
    { // Scope for effect variables
      std::vector<std::string> lightningFrames;
      int frameCount = 8; // User specified 8 frames
      std::string baseName =
          "lightning_bolt_effect"; // Base name for animation frames

      for (int i = 0; i < frameCount; ++i) {
        lightningFrames.push_back(baseName + "_" + std::to_string(i));
      }

      if (!lightningFrames.empty()) {
        // Calculate visual start and end points
        float startVisualX = this->x; // Player's visual center
        float startVisualY = this->y;
        // The visual effect should extend to the end of the calculated
        // lineTiles (which might be truncated by a wall)
        float endVisualX_actual = startVisualX; // Default to start if lineTiles
                                                // is empty after wall check
        float endVisualY_actual = startVisualY;

        if (!lineTiles.empty()) {
          // Use the last tile in the (potentially truncated) lineTiles for the
          // visual end point
          endVisualX_actual = lineTiles.back().first * gameData.tileWidth +
                              gameData.tileWidth / 2.0f;
          endVisualY_actual = lineTiles.back().second * gameData.tileHeight +
                              gameData.tileHeight / 2.0f;
        }

        // Calculate the direction vector from start to actual end
        float deltaX_visual = endVisualX_actual - startVisualX;
        float deltaY_visual = endVisualY_actual - startVisualY;
        float visualEffectDistance = std::sqrt(deltaX_visual * deltaX_visual +
                                               deltaY_visual * deltaY_visual);

        // Calculate the angle in radians, then convert to degrees for
        // SDL_RenderCopyEx
        float angleRadians = std::atan2(
            deltaY_visual,
            deltaX_visual); // Angle based on player to actual end point
        float angleDegrees =
            angleRadians * (180.0f / M_PI); // Convert radians to degrees

        // The visual effect will be positioned at the start point (player's
        // visual center) Its width will be the calculated visual effect
        // distance, height will be a fixed thickness
        float effectWidth = visualEffectDistance; // Length of the visual line
        float effectHeight =
            gameData.tileWidth /
            0.8f; // Fixed thickness (using your requested scaling)

        // The rotation origin should be at the left edge, vertically centered
        // of the texture This is because the texture is a horizontal line
        // animation, and we want it to rotate from the player's center.
        SDL_Point origin = {0, static_cast<int>(effectHeight / 2.0f)};

        // Define the visibility fade start time ratio for this specific effect
        float visibilityFadeStartRatio =
            0.5f; // As requested, fade starts at 50% animation progress

        // Pass only the tiles *after* the player's tile to the VisualEffect for
        // visibility
        std::vector<std::pair<int, int>> tilesForVisibility;
        if (lineTiles.size() > 1) {
          tilesForVisibility.assign(lineTiles.begin() + 1, lineTiles.end());
        }

        gameData.activeEffects.emplace_back(
            startVisualX, startVisualY, // Position at player's visual center
            static_cast<int>(effectWidth),
            static_cast<int>(effectHeight), // Size
            lightningFrames, 24.0f, 0.0f,
            false,        // Animation speed, duration, loops
            angleDegrees, // Pass the calculated angle
            origin,       // Pass the calculated origin
            &gameData,    // Pass pointer to gameData for visibility updates
            tilesForVisibility,      // Pass the list of affected tiles for
                                     // visibility
            visibilityFadeStartRatio // Pass the visibility fade start ratio
        );

        SDL_Log("... Spawned Linear Damage visual effect from (%.1f, %.1f) to "
                "(%.1f, %.1f) (actual end). Length: %.2f, Angle: %.2f degrees. "
                "Origin: {%d, %d}. Visibility Fade Start: %.2f. Affected tiles "
                "for visibility: %zu",
                startVisualX, startVisualY, endVisualX_actual,
                endVisualY_actual, visualEffectDistance, angleDegrees, origin.x,
                origin.y, visibilityFadeStartRatio, tilesForVisibility.size());

      } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Could not generate frame keys for Linear Damage effect.");
      }
    }
    // --- END SPAWN VISUAL EFFECT ---
    break; // End LinearDamage case

  case SpellEffectType::AreaDamage: // <<< NEW CASE for Blizzard
    SDL_Log("CastSpell: Resolving Area Damage for '%s' centered at [%d, %d], "
            "Radius: %d",
            spell.name.c_str(), castTargetX, castTargetY,
            spell.areaOfEffectRadius);

    // Iterate through the square area defined by the radius
    for (int dx = -spell.areaOfEffectRadius; dx <= spell.areaOfEffectRadius;
         ++dx) {
      for (int dy = -spell.areaOfEffectRadius; dy <= spell.areaOfEffectRadius;
           ++dy) {
        int currentTileX = castTargetX + dx;
        int currentTileY = castTargetY + dy;

        // Optional: Circular AoE Check (uncomment if you prefer circles)
        // if (dx*dx + dy*dy > spell.areaOfEffectRadius *
        // spell.areaOfEffectRadius) {
        //     continue; // Skip tiles outside the circle
        // }

        // Check if the tile is within level bounds
        if (!isWithinBounds(currentTileX, currentTileY,
                            gameData.currentLevel.width,
                            gameData.currentLevel.height)) {
          continue; // Skip tiles outside the map
        }

        // Find if any enemy is on this tile
        // IMPORTANT: Need to iterate through enemies vector by index to modify
        // health
        for (int enemyIdx = 0; enemyIdx < enemies.size(); ++enemyIdx) {
          Enemy &currentEnemy = enemies[enemyIdx]; // Get a mutable reference

          if (currentEnemy.health > 0 && currentEnemy.x == currentTileX &&
              currentEnemy.y == currentTileY) {
            // Found a living enemy in the AoE
            SDL_Log("... Found Enemy %d at [%d, %d]", currentEnemy.id,
                    currentTileX, currentTileY);

            // Calculate damage for this specific enemy
            // Note: Pass currentEnemy pointer to calculation function
            int damageToEnemy = this->calculateSpellDamage(
                spellIndex, currentTileX, currentTileY, &currentEnemy);

            SDL_Log("... Applying %d damage to Enemy %d", damageToEnemy,
                    currentEnemy.id);
            currentEnemy.takeDamage(damageToEnemy); // Apply damage

            effectApplied = true; // Mark that at least one enemy was hit
            // Optional: Don't break here, hit all enemies on the same tile if
            // stacking occurs
          }
        } // End loop through enemies for one tile
      } // End dy loop
    } // End dx loop

    // --- SPAWN VISUAL EFFECT ---
    { // Scope for effect variables
      // Prepare frame keys for the blizzard effect
      std::vector<std::string> blizzardFrames;
      // Assuming 12 frames named "blizzard_effect_1" to "blizzard_effect_12"
      // (adjust if padding used) IMPORTANT: Ensure frame count matches what you
      // loaded in AssetManager
      int frameCount = 10; // <<< MATCH THIS TO YOUR ACTUAL ASSETS
      for (int i = 0; i < frameCount; ++i) {
        // Construct key based on how loadAnimationSequence creates them
        blizzardFrames.push_back("blizzard_effect_" + std::to_string(i));
      }

      if (!blizzardFrames.empty()) {
        // Calculate visual center and size for the effect
        float effectCenterX =
            (castTargetX + 0.5f) * gameData.tileWidth; // Center of target tile
        float effectCenterY = (castTargetY + 0.5f) * gameData.tileHeight;
        // Size covers the whole AoE (radius 1 = 3x3 tiles)
        int effectWidth =
            gameData.tileWidth * (1 + 2 * spell.areaOfEffectRadius);
        int effectHeight =
            gameData.tileHeight * (1 + 2 * spell.areaOfEffectRadius);
        float effectSpeed = 16.0f;   // Animation speed (FPS) - Adjust!
        float effectDuration = 0.0f; // Play once based on animation length
        bool loops = false;

        gameData.activeEffects.emplace_back(
            effectCenterX, effectCenterY, effectWidth, effectHeight,
            blizzardFrames, effectSpeed, effectDuration, loops);
        SDL_Log("... Spawned Blizzard visual effect (%dx%d)", effectWidth,
                effectHeight);
      } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Could not generate frame keys for Blizzard effect.");
      }
    } // End scope for effect variables
    // --- END SPAWN VISUAL EFFECT ---

    // Add visual effect spawning here later if desired
    // Example: gameData.activeEffects.push_back(BlizzardEffect(castTargetX,
    // castTargetY, spell.areaOfEffectRadius));

    break; // End AreaDamage case

  case SpellEffectType::Heal:
    if (spell.targetType == SpellTargetType::Self) {
      // Use baseHealAmount from the struct now
      int healAmount = static_cast<int>(spell.baseHealAmount);
      // Maybe apply Spirit modifier here? Example: +1 heal per 10 Spirit?
      // healAmount += GetEffectiveSpirit() / 10;
      health = std::min(health + healAmount, maxHealth);
      SDL_Log("CastSpell: Healed self for %d. Health: %d/%d", healAmount,
              health, maxHealth);
      effectApplied = true;
    } else { /* ... log warning ... */
    }
    break;

  case SpellEffectType::ApplyShield:
    if (spell.targetType == SpellTargetType::Self) {
      // Calculate shield magnitude (can be modified by stats later)
      int shieldMagnitude = static_cast<int>(std::round(
          spell.baseHealAmount)); // Using baseHealAmount field for magnitude
      // Example modifier: Maybe Spirit increases shield strength slightly?
      // shieldMagnitude += GetEffectiveSpirit() / 5; // e.g., +1 shield per 5
      // spirit

      // Calculate the decay amount for this specific application
      int decayAmount = 0;
      if (spell.shieldDecayPercent > 0.0f && shieldMagnitude > 0) {
        decayAmount = static_cast<int>(std::round(
            static_cast<float>(shieldMagnitude) * spell.shieldDecayPercent));
        // Ensure decay is at least 1 if percentage > 0 and magnitude > 0?
        // Optional design choice. decayAmount = std::max(1, decayAmount);
      }

      SDL_Log("CastSpell: Applied '%s'. Shield Value: %d, Decay/Turn: %d",
              spell.name.c_str(), shieldMagnitude, decayAmount);

      // Set the player's shield values
      currentShield = shieldMagnitude;
      shieldDecayPerTurn = decayAmount; // Store the calculated decay amount

      effectApplied = true;
    } else {
      SDL_LogWarn(
          SDL_LOG_CATEGORY_APPLICATION,
          "ApplyShield effect currently only supports Self target type.");
    }
    break;

  case SpellEffectType::SummonOrbital:
    if (spell.targetType == SpellTargetType::Self) {
      SDL_Log("CastSpell: Summoning %d orbitals.", spell.numOrbitals);
      for (int i = 0; i < spell.numOrbitals; ++i) {
        // Calculate starting position near player
        float angle = (i == 0 && spell.numOrbitals == 1) ? 0
                                                         : // Center if only one
                          (2.0f * M_PI * static_cast<float>(i)) /
                              static_cast<float>(spell.numOrbitals);
        float spawnRadius = (float)tileWidth * 0.3f; // Closer orbit/start
        float startX = this->x + spawnRadius * cos(angle);
        float startY = this->y + spawnRadius * sin(angle);

        // Add to GameData's list
        gameData.activeOrbitals.emplace_back(
            // No ownerId needed
            startX, startY, spell.orbitalLifetime,
            spell.orbitalAcquisitionRange,
            spell.numDamageDice, // Use spell's damage dice for payload
            spell.damageDieType, spell.baseDamageBonus,
            spell
                .orbitalProjectileTextureKey, // Texture for launched projectile
            spell.orbitalProjectileSpeed,     // Speed for launched projectile
            i                                 // Formation index
        );
      }
      effectApplied = true;
    } else {
      SDL_LogWarn(
          SDL_LOG_CATEGORY_APPLICATION,
          "SummonOrbital effect currently only supports Self target type.");
    }
    break; // End SummonOrbital case

  case SpellEffectType::AreaPushbackStun:            // <<< Vortex Logic >>>
    if (spell.targetType != SpellTargetType::Self) { /* ... log warning ... */
      break;
    }

    SDL_Log("CastSpell: Resolving Area Pushback/Stun for '%s' centered on "
            "player [%d, %d], Radius: %d",
            spell.name.c_str(), this->logicalTileX, this->logicalTileY,
            spell.areaOfEffectRadius);

    // Iterate through enemies directly
    for (int enemyIdx = 0; enemyIdx < enemies.size(); ++enemyIdx) {
      Enemy &currentEnemy = enemies[enemyIdx];
      if (currentEnemy.health <= 0)
        continue;

      int dx = currentEnemy.x - this->logicalTileX;
      int dy = currentEnemy.y - this->logicalTileY;
      int distSq = dx * dx + dy * dy;
      int radiusSq = spell.areaOfEffectRadius * spell.areaOfEffectRadius;

      // Check if enemy is within the AoE
      if (distSq <= radiusSq) {
        if (dx == 0 && dy == 0)
          continue; // Skip self tile

        SDL_Log("... Vortex affects Enemy %d at [%d, %d]", currentEnemy.id,
                currentEnemy.x, currentEnemy.y);
        effectApplied = true;

        // 1. Apply Status Effect (Stunned)
        if (spell.statusEffectApplied != StatusEffectType::None &&
            spell.statusEffectDuration > 0) {
          currentEnemy.AddStatusEffect(spell.statusEffectApplied,
                                       spell.statusEffectDuration);
        }

        // --- 2. Calculate and Mark for Pushback (Revised Logic) ---
        int finalPushX = currentEnemy.x; // Default to current position
        int finalPushY = currentEnemy.y;
        bool pushTargetFound = false;

        float magnitude = std::sqrt(static_cast<float>(distSq));
        float normDx = 0.0f, normDy = 0.0f;
        if (magnitude > 0.001f) {
          normDx = static_cast<float>(dx) / magnitude;
          normDy = static_cast<float>(dy) / magnitude;
        } else {
          continue;
        } // Skip pushback if on player tile

        SDL_Log("... Calculating pushback for Enemy %d from [%d,%d] along "
                "vector (%.2f, %.2f)",
                currentEnemy.id, currentEnemy.x, currentEnemy.y, normDx,
                normDy);

        // --- Step Outwards to Find Target ---
        int lastValidX = currentEnemy.x; // Track the last valid tile checked
        int lastValidY = currentEnemy.y;
        int currentCheckX = currentEnemy.x;
        int currentCheckY = currentEnemy.y;
        int steps = 0;
        // Search a bit beyond the radius
        const int MAX_PUSH_STEPS = spell.areaOfEffectRadius + 3;

        while (steps < MAX_PUSH_STEPS) {
          steps++;
          // Calculate next tile coordinates based on normalized vector
          int nextX =
              static_cast<int>(std::round(currentEnemy.x + normDx * steps));
          int nextY =
              static_cast<int>(std::round(currentEnemy.y + normDy * steps));

          // Prevent getting stuck if rounding doesn't move
          if (nextX == currentCheckX && nextY == currentCheckY) {
            SDL_Log("... Pushback step %d resulted in no coordinate change. "
                    "Using last valid [%d, %d].",
                    steps, lastValidX, lastValidY);
            break; // Use the last valid position found
          }
          currentCheckX = nextX;
          currentCheckY = nextY;

          SDL_Log("... Checking step %d: Tile [%d, %d]", steps, currentCheckX,
                  currentCheckY);

          // Check if this tile is valid (in bounds and not a wall)
          if (!isWithinBounds(currentCheckX, currentCheckY,
                              gameData.currentLevel.width,
                              gameData.currentLevel.height) ||
              gameData.currentLevel.tiles[currentCheckY][currentCheckX] ==
                  '#') {
            SDL_Log("... Tile [%d, %d] is invalid (OOB or Wall). Stopping "
                    "search. Will use last valid [%d, %d].",
                    currentCheckX, currentCheckY, lastValidX, lastValidY);
            break; // Hit an invalid tile, stop searching outward. lastValidX/Y
                   // holds the best spot.
          }

          // Tile is valid, update last valid position
          lastValidX = currentCheckX;
          lastValidY = currentCheckY;
          SDL_Log("... Tile [%d, %d] is valid.", lastValidX, lastValidY);

          // Check if this valid tile is outside the AoE radius (using squared
          // distance)
          int checkDx = lastValidX - this->logicalTileX;
          int checkDy = lastValidY - this->logicalTileY;
          if (checkDx * checkDx + checkDy * checkDy > radiusSq) {
            SDL_Log("... Valid Tile [%d, %d] is OUTSIDE AoE radius %d. Target "
                    "found.",
                    lastValidX, lastValidY, spell.areaOfEffectRadius);
            pushTargetFound = true; // Mark that we found a spot outside
            break; // Found the first valid tile outside the AoE
          }
        } // End while loop stepping outwards

        // --- Set Final Target and Mark ---
        // Use the last valid tile found by the loop.
        // If pushTargetFound is true, it's the first valid tile outside the
        // radius. If pushTargetFound is false, it's the furthest valid tile
        // reached before hitting an obstacle or max steps.
        finalPushX = lastValidX;
        finalPushY = lastValidY;

        // Only mark for pushback if the final position is different from the
        // start.
        if (finalPushX != currentEnemy.x || finalPushY != currentEnemy.y) {
          SDL_Log("... Final pushback target for Enemy %d set to [%d, %d]. "
                  "Marking for pushback.",
                  currentEnemy.id, finalPushX, finalPushY);
          currentEnemy.needsPushback = true;
          currentEnemy.pushbackTargetX = finalPushX;
          currentEnemy.pushbackTargetY = finalPushY;
        } else {
          SDL_Log("... Enemy %d could not be pushed from [%d, %d]. No valid "
                  "destination found or already outside.",
                  currentEnemy.id, currentEnemy.x, currentEnemy.y);
          currentEnemy.ClearPushbackState(); // Ensure flags are clear
        }
        // --- END Pushback Calculation ---

      } // End if enemy is within AoE
    } // End loop through enemies

    // --- <<< SPAWN VORTEX VISUAL EFFECT >>> ---
    { // Scope for effect variables
      std::vector<std::string> vortexFrames;
      // --- Define parameters based on how assets were loaded ---
      // These should match the parameters used in the loadAnimationSequence
      // call in asset_manager.cpp (vortex_asset_load artifact) Logs show 8
      // frames were loaded (0-7), so frameCount should be 7.
      int frameCount = 7; // <<< CORRECTED: Based on logs (0 through 7)
      // int padding = 4; // Padding is NOT used in the loaded texture keys
      // based on logs
      std::string baseName =
          "vortex_effect"; // Base name from vortex_asset_load

      // --- Generate frame keys ---
      // Generate keys WITHOUT padding to match the loaded asset keys like
      // "vortex_effect_0", "vortex_effect_1", etc. Loop from 0 up to and
      // including frameCount (0 to 7 for 8 frames).
      for (int i = 0; i <= frameCount; ++i) {
        // --- CORRECTED: Generate key without padding ---
        vortexFrames.push_back(baseName + "_" + std::to_string(i));
        // Example generated key: "vortex_effect_0", "vortex_effect_1", ...,
        // "vortex_effect_7"
      }

      if (!vortexFrames.empty()) {
        // Center the effect visually on the player's *current* visual position
        float effectCenterX = this->x;
        float effectCenterY = this->y;

        // Size the effect to cover the full AoE diameter based on the spell's
        // radius
        int effectDiameterTiles =
            1 + (2 * spell.areaOfEffectRadius); // Diameter in tiles (e.g.,
                                                // radius 2 -> 5 tiles)
        int effectWidth = gameData.tileWidth * effectDiameterTiles;
        int effectHeight = gameData.tileHeight * effectDiameterTiles;

        // Animation parameters (adjust speed as desired)
        float effectSpeed = 12.0f; // Frames per second for the animation
        float effectDuration =
            0.0f; // Duration 0 means play once based on frame count/speed
        bool loops = false; // Vortex effect likely does not loop

        // Add the VisualEffect instance to the game's active effects list
        gameData.activeEffects.emplace_back(
            effectCenterX, effectCenterY, effectWidth, effectHeight,
            vortexFrames, effectSpeed, effectDuration, loops);
        SDL_Log("... Spawned Vortex visual effect (%dx%d) centered on player "
                "at (%.1f, %.1f). Using %zu frames.",
                effectWidth, effectHeight, effectCenterX, effectCenterY,
                vortexFrames.size());
      } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Could not generate frame keys for Vortex effect. Visual "
                    "effect not spawned.");
      }
    } // End scope for effect variables
    // --- <<< END SPAWN VISUAL EFFECT >>> ---

    break; // End AreaPushbackStun case

  // Add cases for Buff, Debuff, Summon, etc.
  default:
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "CastSpell: Effect type %d not yet implemented.",
                (int)spell.effectType);
    break;
  }

  return effectApplied;
}

int PlayerCharacter::GetEffectiveSpellRange(int spellIndex) const {
  // 1. Basic Validation
  if (spellIndex < 0 || spellIndex >= knownSpells.size()) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "GetEffectiveSpellRange called with invalid index: %d",
                spellIndex);
    return 0; // Return 0 range for invalid spells
  }

  const Spell &spell = knownSpells[spellIndex];
  int effectiveRange = spell.baseRange;

  // --- Apply Modifiers Here (Future Enhancement) ---
  // Example: Bonus range based on Intelligence?
  // int rangeBonusFromInt = std::max(0, GetEffectiveIntelligence() - 15) / 5;
  // // e.g., +1 range per 5 Int over 15 effectiveRange += rangeBonusFromInt;

  // Example: Modifiers from equipped items or temporary buffs?
  // effectiveRange += GetRangeBonusFromEquipment();
  // effectiveRange += GetRangeBonusFromBuffs();

  // --- End Apply Modifiers ---

  // Ensure range doesn't become negative if modifiers are subtractive
  return std::max(0, effectiveRange);
}

// --- Definition for GetEffectiveManaCost ---
int PlayerCharacter::GetEffectiveManaCost(int spellIndex) const {
  // 1. Basic Validation
  if (spellIndex < 0 || spellIndex >= knownSpells.size()) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "GetEffectiveManaCost called with invalid index: %d",
                spellIndex);
    // Return a high cost or handle error appropriately for invalid spells
    return 9999;
  }

  const Spell &spell = knownSpells[spellIndex];
  int effectiveCost = spell.baseManaCost;

  // --- Apply Modifiers Here (Future Enhancement) ---
  // Example: Cost reduction based on Spirit or a specific skill?
  // int costReductionFromSpirit = GetEffectiveSpirit() / 20; // e.g., -1 cost
  // per 20 Spirit effectiveCost -= costReductionFromSpirit;

  // Example: Modifiers from equipped items ("Ring of Mana Thriftyness")
  // effectiveCost *= GetManaCostMultiplierFromEquipment(); // e.g., 0.9 for 10%
  // reduction

  // --- End Apply Modifiers ---

  // Ensure cost doesn't go below a minimum threshold (e.g., 0 or 1)
  return std::max(0, effectiveCost); // Cannot have negative mana cost
}

void PlayerCharacter::AddStatusEffect(StatusEffectType type, int duration) {
  if (duration <= 0)
    return; // Don't add effects with no duration

  // Optional: Check for existing effect of the same type
  // Behaviour can vary: Refresh duration? Stack? Ignore? Let's REFRESH for now.
  for (auto &existingEffect : activeStatusEffects) {
    if (existingEffect.type == type) {
      existingEffect.durationTurns = std::max(
          existingEffect.durationTurns, duration); // Take the longer duration
      SDL_Log("DEBUG: Player Status Refreshed: %d duration %d turns.",
              (int)type, existingEffect.durationTurns);
      return; // Refreshed, no need to add new
    }
  }

  // If not found, add the new effect
  activeStatusEffects.emplace_back(type, duration);
  SDL_Log("DEBUG: Player Status Added: %d duration %d turns.", (int)type,
          duration);
}

void PlayerCharacter::RemoveStatusEffect(StatusEffectType type) {
  // Removes ALL instances of a given type (if stacking allowed later)
  activeStatusEffects.erase(std::remove_if(activeStatusEffects.begin(),
                                           activeStatusEffects.end(),
                                           [type](const StatusEffect &effect) {
                                             return effect.type == type;
                                           }),
                            activeStatusEffects.end());
  SDL_Log("DEBUG: Player Status Removed: %d.", (int)type);
}

bool PlayerCharacter::HasStatusEffect(StatusEffectType type) const {
  for (const auto &effect : activeStatusEffects) {
    if (effect.type == type) {
      return true;
    }
  }
  return false;
}

// This is called at the end of the turn to tick down durations
void PlayerCharacter::UpdateStatusEffectDurations() {
  bool effectRemoved = false;
  // Iterate backwards for safe removal
  for (int i = activeStatusEffects.size() - 1; i >= 0; --i) {
    activeStatusEffects[i].durationTurns--;
    if (activeStatusEffects[i].durationTurns <= 0) {
      SDL_Log("DEBUG: Player Status Expired: %d.",
              (int)activeStatusEffects[i].type);
      // Swap-and-pop or direct erase (vector erase is simpler here)
      activeStatusEffects.erase(activeStatusEffects.begin() + i);
      effectRemoved = true;
    }
  }
  // If any effects were removed, potentially recalculate stats if effects
  // modify them if (effectRemoved) { RecalculateStats(); } // Example hook
}

void PlayerCharacter::ApplyTurnEndEffects() {

  UpdateStatusEffectDurations(); // Tick down statuses first

  // 1. Shield Decay
  if (currentShield > 0) {
    int shieldBeforeDecay = currentShield;
    currentShield -= shieldDecayPerTurn; // Apply the pre-calculated decay

    if (currentShield <= 0) {
      currentShield = 0;      // Prevent negative shield
      shieldDecayPerTurn = 0; // Reset decay if shield is gone
      SDL_Log("DEBUG: Shield decayed to zero.");
    } else {
      SDL_Log("DEBUG: Shield decayed by %d. Current Shield: %d",
              shieldDecayPerTurn, currentShield);
    }
  }

  // 2. Mana Regeneration (Moved from main loop)
  RegenerateMana(1.0f); // Assuming 1.0 represents one turn step

  // 3. Add other effects later (e.g., poison damage, buff durations)
}

int PlayerCharacter::calculateSpellDamage(int spellIndex, int targetTileX,
                                          int targetTileY,
                                          const Enemy *target) const {
  if (spellIndex < 0 || spellIndex >= knownSpells.size()) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Invalid spell index %d for damage calculation.", spellIndex);
    return 0;
  }
  const Spell &spell = knownSpells[spellIndex];

  // 1. Roll Base Dice
  int currentDamage =
      rollDice(spell.numDamageDice, spell.damageDieType, spell.baseDamageBonus);

  // 2. Apply Player Modifiers (Intelligence, etc.)
  currentDamage =
      static_cast<int>(std::round(currentDamage * this->spellDamageModifier));

  // 3. Apply Distance Bonus (Now possible with target coordinates)
  if (spell.targetType != SpellTargetType::Self &&
      spell.baseDistanceDamageBonusPercent > 0.0f) {
    // Calculate Manhattan distance from caster's logical position to target
    // tile
    int distance = std::abs(this->targetTileX - targetTileX) +
                   std::abs(this->targetTileY - targetTileY);
    int tilesBeyondAdjacent = std::max(0, distance - 1);

    if (tilesBeyondAdjacent > 0) {
      float effectiveDistanceBonusPercent =
          spell.baseDistanceDamageBonusPercent;
      // Add other potential distance modifiers here later (from skills, etc.)
      float distanceMultiplier =
          1.0f + (static_cast<float>(tilesBeyondAdjacent) *
                  effectiveDistanceBonusPercent);
      int damageBeforeDistanceBonus = currentDamage;
      currentDamage = static_cast<int>(
          std::round(static_cast<float>(currentDamage) * distanceMultiplier));

      SDL_Log("DEBUG [CalcDmg]: Applied distance bonus. Dist: %d, Mult: %.2f, "
              "Dmg: %d -> %d",
              distance, distanceMultiplier, damageBeforeDistanceBonus,
              currentDamage);
    }
  }

  // 4. Apply Spell Upgrades (Future Placeholder)
  // currentDamage = applyUpgrades(currentDamage, spellIndex);

  // 5. Apply Target Modifiers (Resistances/Vulnerabilities - Future
  // Placeholder) if (target != nullptr) { currentDamage =
  // applyTargetResistances(currentDamage, target, spell.damageType); }

  // Ensure damage isn't negative
  return std::max(0, currentDamage);
}

int PlayerCharacter::calculateSpellDamage(int numDice, int dieType, int bonus,
                                          int targetTileX, int targetTileY,
                                          const Enemy *target) const {
  // 1. Roll Base Dice
  int currentDamage = rollDice(numDice, dieType, bonus);
  int initialRoll = currentDamage; // Store for logging

  // 2. Apply Player Modifiers (Intelligence, etc.)
  // Use the spellDamageModifier calculated in RecalculateStats()
  currentDamage =
      static_cast<int>(std::round(currentDamage * this->spellDamageModifier));
  SDL_Log("DEBUG [CalcDmg]: Base Roll (%dd%d+%d) = %d. After Player Mod "
          "(x%.2f) = %d",
          numDice, dieType, bonus, initialRoll, this->spellDamageModifier,
          currentDamage);

  // 3. Apply Distance Bonus (Placeholder - Requires Spell Struct access or
  // parameters) NOTE: This overload currently CANNOT apply spell-specific
  // distance bonuses because it doesn't know which spell is being calculated.
  // If distance bonus needs to apply to orbital payloads, the orbital needs
  // to pass more info OR this function needs the spell reference again.
  // For now, distance bonus is only applied via the spellIndex overload.
  // Consider adding spell reference parameter if distance bonus needed here.

  // 4. Apply Spell Upgrades (Future Placeholder)
  // currentDamage = applyUpgrades(currentDamage, spellIndex); // Needs spell
  // context

  // 5. Apply Target Modifiers (Resistances/Vulnerabilities - Future
  // Placeholder) if (target != nullptr) { currentDamage =
  // applyTargetResistances(currentDamage, target, /* needs damage type */); }

  // Ensure damage isn't negative
  int finalDamage = std::max(0, currentDamage);
  SDL_Log("DEBUG [CalcDmg]: Final Damage = %d", finalDamage);
  return finalDamage;
}