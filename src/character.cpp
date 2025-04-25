// In character.cpp
#include "character.h"
#include "asset_manager.h"
#include "enemy.h"
#include "game_data.h"
#include "projectile.h"
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
      moveTimer(0.0f), tileWidth(tileW), tileHeight(tileH) {
  // Initialize known spells based on type
  if (type == CharacterType::FemaleMage || type == CharacterType::MaleMage) {
    knownSpells.emplace_back("Fireball", 10, 5, SpellTargetType::Enemy,
                             SpellEffectType::Damage, 25.0f, "fireball_icon");
    knownSpells.emplace_back("Minor Heal", 15, 0, SpellTargetType::Self,
                             SpellEffectType::Heal, 30.0f, "minor_heal_icon");
  }

  // CRITICAL: Calculate initial stats based on starting level and base stats
  RecalculateStats();

  // Set current health/mana to max initially
  health = maxHealth;
  mana = maxMana;

  std::cout << "Player Initialized. Level: " << level << " HP: " << health
            << "/" << maxHealth << " Mana: " << mana << "/" << maxMana
            << std::endl;
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
      0.2f - (effAgi * SPEED_MOD_PER_AGILITY)); // Example: base 0.2s, faster
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
    level = potentialNewLevel;
    std::cout << "Level Up! " << oldLevel << " -> " << level << std::endl;
    RecalculateStats();
    // Optional: Fully heal/restore mana on level up?
    health = maxHealth;
    mana = maxMana;
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
  if (isMoving) {
    moveTimer += deltaTime;
    moveProgress =
        moveTimer / moveDuration; // moveDuration is now potentially dynamic

    // *** ADD LOG: Check values JUST BEFORE the completion IF statement ***
    SDL_Log("DEBUG: [PlayerUpdate Pre-Check] Checking completion: moveProgress=%.4f (Timer=%.4f / Duration=%.4f)",
        moveProgress, moveTimer, moveDuration);
    // ***

    if (moveProgress >= 1.0f) {
      moveProgress = 1.0f;

      int oldTileX = startTileX; // Store starting tile before snapping logic
                                 // potentially changes it
      int oldTileY = startTileY;

      isMoving = false;
      x = targetTileX * tileWidth + tileWidth / 2.0f;
      y = targetTileY * tileHeight + tileHeight / 2.0f;

      // --- NEW: Update Occupation Grid ---
      // Clear old position (check bounds)
      if (oldTileX >= 0 && oldTileX < gameData.currentLevel.width &&
          oldTileY >= 0 && oldTileY < gameData.currentLevel.height) {
        gameData.occupationGrid[oldTileY][oldTileX] = false;
      }
      // Set new position (check bounds)
      if (targetTileX >= 0 && targetTileX < gameData.currentLevel.width &&
          targetTileY >= 0 && targetTileY < gameData.currentLevel.height) {
        gameData.occupationGrid[targetTileY][targetTileX] = true;
      } else {
        // Log error if moving out of bounds? Should not happen ideally.
        SDL_Log("Warning: Player moved outside level bounds to (%d, %d)? Grid "
                "not updated.",
                targetTileX, targetTileY);
      }
      // --- End Occupation Grid Update ---
    } else {
      float startVisualX = startTileX * tileWidth + tileWidth / 2.0f;
      float startVisualY = startTileY * tileHeight + tileHeight / 2.0f;
      float targetVisualX = targetTileX * tileWidth + tileWidth / 2.0f;
      float targetVisualY = targetTileY * tileHeight + tileHeight / 2.0f;

      x = startVisualX + (targetVisualX - startVisualX) * moveProgress;
      y = startVisualY + (targetVisualY - startVisualY) * moveProgress;
    }
  }
}
// Placeholder for canCastSpell
bool PlayerCharacter::canCastSpell(int spellIndex) const {
  if (spellIndex < 0 || spellIndex >= knownSpells.size()) {
    return false;
  }
  const Spell &spell = knownSpells[spellIndex];
  return mana >= spell.manaCost; // Check against current mana
}
// Placeholder for getSpell
const Spell &PlayerCharacter::getSpell(int spellIndex) const {
  if (spellIndex < 0 || spellIndex >= knownSpells.size()) {
    throw std::out_of_range("Invalid spell index in getSpell");
  }
  return knownSpells[spellIndex];
}
// Placeholder for takeDamage
void PlayerCharacter::takeDamage(int amount) {
  health -= amount;
  std::cout << "Player took " << amount << " damage. Health: " << health << "/"
            << maxHealth << std::endl;
  if (health <= 0) {
    health = 0;
    std::cout << "Player has been defeated!" << std::endl;
  }
}
// Placeholder for castSpell - NEEDS LATER MODIFICATION for damage based on Int
bool PlayerCharacter::castSpell(int spellIndex, int castTargetX,
                                int castTargetY, std::vector<Enemy> &enemies,
                                std::vector<Projectile> &projectiles,
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
            spell.name.c_str(), mana, spell.manaCost);
    return false;
  }

  // 2. Check Range (if applicable)
  if (spell.targetType != SpellTargetType::Self) {
    int distance = std::abs(targetTileX - castTargetX) +
                   std::abs(targetTileY - castTargetY);
    if (distance > spell.range) {
      SDL_Log("CastSpell: Target [%d,%d] out of range for '%s' (Range: %d, "
              "Dist: %d).",
              castTargetX, castTargetY, spell.name.c_str(), spell.range,
              distance);
      return false; // Target out of range
    }
  }

  // 3. Deduct Mana Cost (do this early)
  mana -= spell.manaCost;
  SDL_Log("CastSpell: Spent %d mana for '%s'. Remaining: %d/%d", spell.manaCost,
          spell.name.c_str(), mana, maxMana);

  // 4. Apply Spell Effect / Create Projectile
  bool effectApplied = false;
  switch (spell.effectType) {
  case SpellEffectType::Damage:
    if (spell.targetType == SpellTargetType::Enemy ||
        spell.targetType == SpellTargetType::Tile ||
        spell.targetType == SpellTargetType::Area) {

      // --- Find Target Enemy ID (for homing projectiles) ---
      int targetId = -1; // Default: -1 indicates no specific enemy target
                         // (e.g., targeting a tile)
      // Only search for an enemy ID if the spell specifically targets enemies
      if (spell.targetType == SpellTargetType::Enemy) {
        for (const auto &enemy : enemies) {
          // Check if a living enemy exists at the target logical coordinates
          if (enemy.health > 0 && enemy.x == castTargetX &&
              enemy.y == castTargetY) {
            targetId = enemy.id; // Store the ID of the found enemy
            SDL_Log("CastSpell: Found target Enemy ID %d at [%d,%d].", targetId,
                    castTargetX, castTargetY);
            break; // Stop searching once found
          }
        }
        // Log if no enemy was found at the targeted tile
        if (targetId == -1) {
          SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                      "CastSpell: Targeted enemy at [%d,%d] but no living "
                      "enemy found there.",
                      castTargetX, castTargetY);
          // Depending on game design, spell might fizzle or still launch
          // towards the tile
        }
      }
      // ---------------------------------------------------------
      // --- Create Projectile ---
      // Determine projectile texture name (e.g., based on spell name)
      std::string projectileTextureName;
      if (spell.name == "Fireball")
        projectileTextureName = "fireball"; // Example mapping
      // Add mappings for other projectile spells here...
      else
        projectileTextureName = spell.iconName; // Fallback to icon name? Risky.

      SDL_Texture *projTexture = nullptr;
      if (assets &&
          !projectileTextureName.empty()) { // Check if assets pointer is valid
        projTexture = assets->getTexture(projectileTextureName);
      }

      if (projTexture) {
        // Calculate damage based on spell value and player stats
        int calculatedDamage =
            static_cast<int>(spell.value * spellDamageModifier);

        // Calculate visual start/end points
        float startVisualX = this->x; // Player's current visual center X
        float startVisualY = this->y; // Player's current visual center Y
        float targetVisualX = castTargetX * tileWidth +
                              tileWidth / 2.0f; // Center of target tile X
        float targetVisualY = castTargetY * tileHeight +
                              tileHeight / 2.0f; // Center of target tile Y

        // Define projectile properties (speed, size)
        float projectileSpeed = 600.0f; // Example speed (pixels per second)
        int projWidth = 32, projHeight = 32; // Example size
        ProjectileType pType =
            ProjectileType::Firebolt; // Determine type based on spell if needed

        // Add the projectile to the active list
        projectiles.emplace_back(pType, projTexture, projWidth, projHeight,
                                 startVisualX, startVisualY, targetVisualX,
                                 targetVisualY, projectileSpeed,
                                 calculatedDamage, targetId);
        SDL_Log("CastSpell: Launched '%s' projectile towards [%d,%d] with %d "
                "damage.",
                spell.name.c_str(), castTargetX, castTargetY, calculatedDamage);
        effectApplied = true;
      } else {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "CastSpell: Failed to get projectile texture '%s' for spell '%s'.",
            projectileTextureName.c_str(), spell.name.c_str());
        // Spell still cost mana, but projectile wasn't created.
        // Depending on design, maybe refund mana? For now, mana is spent.
      }
    } else {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "CastSpell: Damage effect type requires Enemy, Tile, or Area "
                  "target type for projectile.");
    }
    break;

  case SpellEffectType::Heal:
    if (spell.targetType == SpellTargetType::Self) {
      int healAmount = static_cast<int>(
          spell.value); // Healing might not scale with Int/DmgMod
      health = std::min(health + healAmount,
                        maxHealth); // Apply healing, clamp to max
      SDL_Log("CastSpell: Healed self for %d. Health: %d/%d", healAmount,
              health, maxHealth);
      effectApplied = true;
    } else {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "CastSpell: Heal effect type currently only supports Self "
                  "target type.");
    }
    break;

  // Add cases for Buff, Debuff, Summon, etc.
  case SpellEffectType::Buff:
  case SpellEffectType::Debuff:
  case SpellEffectType::Summon:
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "CastSpell: Effect type %d not yet implemented.",
                (int)spell.effectType);
    break;
  }

  return effectApplied; // Return true if the spell had a resolvable effect
}