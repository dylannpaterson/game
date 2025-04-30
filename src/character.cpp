// In character.cpp
#include "character.h"
#include "asset_manager.h"
#include "enemy.h"
#include "game_data.h"
#include "projectile.h"
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
    knownSpells.emplace_back("Fireball", 7, 3, SpellTargetType::Enemy,
                             SpellEffectType::Damage, 6, 6, 0,
                             0.1, // numDice=6, dieType=6, bonus=0 -> 6d6
                             "fireball_icon");
    knownSpells.emplace_back(
        "Ward", 20, SpellTargetType::Self, SpellEffectType::ApplyShield,
        50.0f, // Shield Magnitude (using baseHealAmount field)
        0.20f, // Decay 20% of max per turn
        "ward_icon");
    // ---> ADD Magic Missiles <---
    knownSpells.emplace_back(
        "Magic Missiles", 15, SpellTargetType::Self,
        SpellEffectType::SummonOrbital, 3, 6,
        500.0f,  // Summon 3 orbitals, 6 tile range, 3 sec lifetime
        2, 6, 0, // Payload: 1d4+1 damage
        "magic_missile_launched",
        700.0f,                 // Launched projectile texture key & speed
        "magic_missiles_icon"); // Icon for spell bar/menu
  }

  // CRITICAL: Calculate initial stats based on starting level and base stats
  RecalculateStats();

  // Set current health/mana to max initially
  health = maxHealth;
  mana = maxMana;

  // Idle animation frames
  // Initialize the vector of texture names
  if (type == CharacterType::FemaleMage) {
    idleFrameTextureNames = {"female_mage_idle_1", "female_mage_idle_2",
                             "female_mage_idle_3", "female_mage_idle_4",
                             "female_mage_idle_5"};

    walkFrameTextureNames = {
        "female_mage_walk_1",  "female_mage_walk_2", "female_mage_walk_3",
        "female_mage_walk_4",  "female_mage_walk_5", "female_mage_walk_6",
        "female_mage_walk_7",  "female_mage_walk_8", "female_mage_walk_9",
        "female_mage_walk_10", "female_mage_walk_11"};
    targetingFrameTextureNames = {
        "female_mage_target_1", "female_mage_target_2", "female_mage_target_3",
        "female_mage_target_4", "female_mage_target_5"};

    for (int i = 1; i <= 9; ++i) {
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

// --- Existing Method Modifications (Examples - NEEDED LATER) ---

/*
// Modify takeDamage to use calculated maxHealth
void PlayerCharacter::takeDamage(int amount) {
     health -= amount;
     std::cout << "Player took " << amount << " damage. Health: " << health <<
"/" << maxHealth << std::endl; // maxHealth is now dynamic if (health <= 0) {
          health = 0;
          std::cout << "Player has been defeated!" << std::endl;
     }
}

// Modify castSpell to use calculated spellDamageModifier and check calculated
maxMana bool PlayerCharacter::castSpell(int spellIndex, int castTargetX, int
castTargetY, std::vector<Enemy>& enemies, std::vector<Projectile>& projectiles,
    SDL_Texture* projectileTexture) {

    // ... (Check spellIndex validity) ...
    const Spell& spell = knownSpells[spellIndex];

    // 1. Check Mana Cost (against current mana)
    if (mana < spell.manaCost) {
         // ... (fail message) ...
        return false;
    }

    // ... (Check range etc.) ...

    // --- Apply Spell Effect ---
    bool castSuccess = false;
    switch (spell.targetType) {
        case SpellTargetType::Self:
            if (spell.effectType == SpellEffectType::Heal) {
                int healAmount = static_cast<int>(spell.value *
spellDamageModifier); // Apply modifier maybe? Or only to damage? health +=
healAmount; health = std::min(health, maxHealth); // Use calculated maxHealth
                 // ... (log message) ...
                castSuccess = true;
            }
            break;
        case SpellTargetType::Enemy:
             // ... (check valid target) ...
            if (spell.effectType == SpellEffectType::Damage) {
                 // ... (calculate projectile start/target) ...
                // Damage calculation will happen *later* when projectile hits,
                // BUT the projectile should store the damage amount calculated
NOW int damageAmount = static_cast<int>(spell.value * spellDamageModifier); //
Apply modifier HERE

                // Launch projectile, passing calculated damageAmount
                 projectiles.emplace_back(..., damageAmount); // Need to add
damage to Projectile struct/constructor castSuccess = true;
            }
            break;
         // ... (other cases) ...
    } // End switch

    // 4. Deduct Mana if Cast Was Successful
    if (castSuccess) {
        mana -= spell.manaCost;
         // ... (log mana remaining) ...
    }
    return castSuccess;
}
*/

// --- Other existing methods (startMove, update for movement) likely unchanged
// for now --- Placeholder implementations for existing methods if needed
void PlayerCharacter::startMove(int targetX, int targetY) {
  if (!isMoving && (targetX != targetTileX || targetY != targetTileY)) {
    isMoving = true;
    startTileX = targetTileX;
    startTileY = targetTileY;
    targetTileX = targetX;
    targetTileY = targetY;
    moveProgress = 0.0f;
    moveTimer = 0.0f;
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
      updateVisibility(gameData.currentLevel, gameData.levelRooms,
                       targetTileX, // Use final logical X
                       targetTileY, // Use final logical Y
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
    x = targetTileX * tileWidth + tileWidth / 2.0f;
    y = targetTileY * tileHeight + tileHeight / 2.0f;

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
    int distance = std::abs(targetTileX - castTargetX) +
                   std::abs(targetTileY - castTargetY);
    if (distance > effectiveRange) {
      SDL_Log("CastSpell: Target [%d,%d] out of effective range for '%s' "
              "(Range: %d, Dist: %d).",
              castTargetX, castTargetY, spell.name.c_str(), effectiveRange,
              distance);
      return false; // Target out of range
    }
  }

  // 4. Deduct Mana Cost (use effective cost)
  mana -= effectiveManaCost;
  SDL_Log("CastSpell: Spent %d mana for '%s'. Remaining: %d/%d",
          effectiveManaCost, spell.name.c_str(), mana, maxMana);

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

void PlayerCharacter::ApplyTurnEndEffects() {
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

int PlayerCharacter::calculateSpellDamage(int numDice, int dieType, int bonus, int targetTileX, int targetTileY, const Enemy* target) const {
  // 1. Roll Base Dice
  int currentDamage = rollDice(numDice, dieType, bonus);
  int initialRoll = currentDamage; // Store for logging

  // 2. Apply Player Modifiers (Intelligence, etc.)
  // Use the spellDamageModifier calculated in RecalculateStats()
  currentDamage = static_cast<int>(std::round(currentDamage * this->spellDamageModifier));
  SDL_Log("DEBUG [CalcDmg]: Base Roll (%dd%d+%d) = %d. After Player Mod (x%.2f) = %d",
          numDice, dieType, bonus, initialRoll, this->spellDamageModifier, currentDamage);


  // 3. Apply Distance Bonus (Placeholder - Requires Spell Struct access or parameters)
  // NOTE: This overload currently CANNOT apply spell-specific distance bonuses
  // because it doesn't know which spell is being calculated.
  // If distance bonus needs to apply to orbital payloads, the orbital needs
  // to pass more info OR this function needs the spell reference again.
  // For now, distance bonus is only applied via the spellIndex overload.
  // Consider adding spell reference parameter if distance bonus needed here.


  // 4. Apply Spell Upgrades (Future Placeholder)
  // currentDamage = applyUpgrades(currentDamage, spellIndex); // Needs spell context

  // 5. Apply Target Modifiers (Resistances/Vulnerabilities - Future Placeholder)
  // if (target != nullptr) { currentDamage = applyTargetResistances(currentDamage, target, /* needs damage type */); }

  // Ensure damage isn't negative
  int finalDamage = std::max(0, currentDamage);
  SDL_Log("DEBUG [CalcDmg]: Final Damage = %d", finalDamage);
  return finalDamage;
}