// character.cpp
#include "character.h" // Include the header file for the PlayerCharacter declaration
#include "enemy.h"     // Include enemy definition for castSpell implementation
#include <cmath>       // For std::abs in castSpell range check and lerp
#include <iostream>    // For debugging output (optional)
#include "projectile.h"

// --- Constructor Implementation ---
PlayerCharacter::PlayerCharacter(CharacterType t, int hp, int maxHp, int mp, int maxMp, int lvl,
                                 int initialTileX, int initialTileY, int tileWidth, int tileHeight, float moveDur)
    : type(t),
      health(hp), maxHealth(maxHp),
      mana(mp), maxMana(maxMp),   // Initialize mana
      level(lvl),
      x(initialTileX * tileWidth + tileWidth / 2.0f),   // Center visual X
      y(initialTileY * tileHeight + tileHeight / 2.0f), // Center visual Y
      targetTileX(initialTileX),  // Initial logical position
      targetTileY(initialTileY),
      isMoving(false),
      startTileX(initialTileX),   // Start is same as target initially
      startTileY(initialTileY),
      moveProgress(0.0f),
      moveDuration(moveDur),      // Use the provided move duration
      moveTimer(0.0f),
      tileWidth(tileWidth),    // Store the passed tile width
      tileHeight(tileHeight)   // Store the passed tile height
{
    // --- Initialize Spellbook (Example) ---
    // Grant starting spells based on type - just like before
    if (type == CharacterType::FemaleMage || type == CharacterType::MaleMage) {
        knownSpells.emplace_back("Fireball", 10, 5, SpellTargetType::Enemy, SpellEffectType::Damage, 25.0f);
        knownSpells.emplace_back("Minor Heal", 15, 0, SpellTargetType::Self, SpellEffectType::Heal, 30.0f);
         // Add more default spells here
    }
    // Add spells for other classes if you introduce them
}

// --- Method Implementations ---

void PlayerCharacter::startMove(int targetX, int targetY) {
    if (!isMoving && (targetX != targetTileX || targetY != targetTileY)) {
        isMoving = true;
        startTileX = targetTileX; // Where the move starts (current logical tile)
        startTileY = targetTileY;
        targetTileX = targetX;    // The destination logical tile
        targetTileY = targetY;
        moveProgress = 0.0f;
        moveTimer = 0.0f; // Reset timer for this specific move

        // Visual position 'x' and 'y' will update in the 'update' function
    }
}

void PlayerCharacter::update(float deltaTime, int tileWidth, int tileHeight) {
    if (isMoving) {
        moveTimer += deltaTime;
        moveProgress = moveTimer / moveDuration;

        if (moveProgress >= 1.0f) {
            // --- Movement Complete ---
            moveProgress = 1.0f; // Clamp progress
            isMoving = false;    // Stop moving state

            // Snap visual position exactly to the center of the target tile
            x = targetTileX * tileWidth + tileWidth / 2.0f;
            y = targetTileY * tileHeight + tileHeight / 2.0f;

            // Update starting point for potential next move (no longer strictly needed here, but doesn't hurt)
            // startTileX = targetTileX;
            // startTileY = targetTileY;
        } else {
            // --- Interpolate Visual Position ---
            // Linear interpolation (Lerp)
            float startVisualX = startTileX * tileWidth + tileWidth / 2.0f;
            float startVisualY = startTileY * tileHeight + tileHeight / 2.0f;
            float targetVisualX = targetTileX * tileWidth + tileWidth / 2.0f;
            float targetVisualY = targetTileY * tileHeight + tileHeight / 2.0f;

            x = startVisualX + (targetVisualX - startVisualX) * moveProgress;
            y = startVisualY + (targetVisualY - startVisualY) * moveProgress;
        }
    }
     // Note: targetTileX/Y represents the logical grid position.
     // 'x'/'y' represents the smoothed visual position for rendering.
}

bool PlayerCharacter::canCastSpell(int spellIndex) const {
    if (spellIndex < 0 || spellIndex >= knownSpells.size()) {
        std::cerr << "Error: Invalid spell index " << spellIndex << " requested." << std::endl;
        return false; // Invalid index
    }
    const Spell& spell = knownSpells[spellIndex];
    if (mana >= spell.manaCost) {
        return true; // Has enough mana
    } else {
        std::cout << "Not enough mana for " << spell.name << ". Needs " << spell.manaCost << ", has " << mana << "." << std::endl;
        return false; // Not enough mana
    }
}

const Spell& PlayerCharacter::getSpell(int spellIndex) const {
    // Consider adding bounds checking here for robustness, or ensure calling code is safe
    if (spellIndex < 0 || spellIndex >= knownSpells.size()) {
         std::cerr << "FATAL ERROR: Attempted to get invalid spell index " << spellIndex << std::endl;
         // Returning a reference requires a valid object. This is tricky.
         // Option 1: Throw an exception (better C++)
         throw std::out_of_range("Invalid spell index in getSpell");
         // Option 2: Return a static dummy spell (less ideal)
         // static Spell dummySpell("Error",0,0,SpellTargetType::Self, SpellEffectType::Damage, 0);
         // return dummySpell;
         // Option 3: Crash (what might happen implicitly if bounds check fails)
    }
    return knownSpells[spellIndex];
}


bool PlayerCharacter::castSpell(int spellIndex, int castTargetX, int castTargetY,
    std::vector<Enemy>& enemies, std::vector<Projectile>& projectiles, // <-- Add projectile list
    SDL_Texture* projectileTexture) {
    if (spellIndex < 0 || spellIndex >= knownSpells.size()) {
         std::cerr << "Error: Attempted to cast invalid spell index " << spellIndex << "." << std::endl;
        return false; // Invalid index
    }

    const Spell& spell = knownSpells[spellIndex]; // Get the spell details

    // 1. Check Mana Cost
    if (mana < spell.manaCost) {
        std::cout << "Failed to cast " << spell.name << ": Not enough mana." << std::endl;
        return false; // Not enough mana
    }

    // 2. Check Range (if not self-targeted)
    if (spell.targetType != SpellTargetType::Self) {
        // Use Manhattan distance for grid-based games
        int distance = std::abs(targetTileX - castTargetX) + std::abs(targetTileY - castTargetY);
        if (distance > spell.range) {
            std::cout << "Failed to cast " << spell.name << ": Target (" << castTargetX << "," << castTargetY << ") is out of range (" << distance << "/" << spell.range << ")." << std::endl;
            return false; // Out of range
        }
    }

    // 3. Apply Spell Effect
    bool castSuccess = false;
    switch (spell.targetType) {
        case SpellTargetType::Self:
            if (spell.effectType == SpellEffectType::Heal) {
                int healAmount = static_cast<int>(spell.value);
                health += healAmount;
                if (health > maxHealth) health = maxHealth; // Cap healing at max health
                std::cout << "Cast " << spell.name << " on self, healing " << healAmount << ". Health: " << health << "/" << maxHealth << std::endl;
                castSuccess = true;
            }
            // Add other self-targeted effects (buffs?) here
            else {
                 std::cout << "Warning: Unsupported effect type for self-targeted spell '" << spell.name << "'." << std::endl;
            }
            break;

            case SpellTargetType::Enemy:
            {
                // Find the enemy at the target tile (Check needed to ensure target is valid for spell)
                bool validEnemyTarget = false;
                for (const auto& enemy : enemies) {
                     if (enemy.x == castTargetX && enemy.y == castTargetY && enemy.health > 0) {
                         validEnemyTarget = true;
                         break;
                     }
                }
                 // You might also allow casting at an empty tile within range? Or require an enemy? Let's require an enemy for now.
                 if (!validEnemyTarget) {
                     SDL_Log("Failed to cast %s: No living enemy at target tile (%d, %d).", spell.name.c_str(), castTargetX, castTargetY);
                     // Do not set castSuccess = true;
                     // Do not deduct mana
                     break; // Exit case
                 }
    
    
                if (spell.effectType == SpellEffectType::Damage) {
                    // --- >> CHANGE: Instead of dealing damage, launch projectile << ---
    
                    // Calculate start and target positions (visual coordinates, center of tiles)
                    float startVisualX = this->x; // Player's current visual center
                    float startVisualY = this->y;
                    float targetVisualX = castTargetX * tileWidth + tileWidth / 2.0f;
                    float targetVisualY = castTargetY * tileHeight + tileHeight / 2.0f;
                    float projectileSpeed = 450.0f; // Adjust speed as needed
                    int projWidth = 16, projHeight = 16; // Example size
    
                    // TODO: Select correct texture based on spell.name or type
                    SDL_Texture* texToUse = projectileTexture; // Use the passed texture for now
    
                    if (texToUse) {
                         ProjectileType pType = ProjectileType::Firebolt; // TODO: Map from spell
                         projectiles.emplace_back(pType, texToUse, projWidth, projHeight,
                                                  startVisualX, startVisualY, targetVisualX, targetVisualY,
                                                  projectileSpeed);
                         SDL_Log("Launched %s projectile towards (%d, %d)", spell.name.c_str(), castTargetX, castTargetY);
                         castSuccess = true; // Mark as successfully cast (mana will be deducted)
                    } else {
                         SDL_Log("Cannot launch %s: Projectile texture is missing!", spell.name.c_str());
                         // castSuccess remains false, mana not deducted
                    }
    
                    // --- >> REMOVE direct damage application here << ---
                    // targetEnemy->takeDamage(damageAmount); // No longer applied here
    
                } else {
                    std::cout << "Warning: Unsupported effect type for enemy-targeted projectile spell '" << spell.name << "'." << std::endl;
                }
            } // End scope for Enemy case
            break; // End of Enemy case

        case SpellTargetType::Tile:
             // Apply effect to a tile (e.g., create fire, ice) - requires Level modification
             std::cout << "Spell TargetType::Tile not implemented yet for '" << spell.name << "'." << std::endl;
             // castSuccess = true; // Potentially successful even if effect not implemented
             break;

        case SpellTargetType::Area:
            // Apply effect to tiles/entities in an area around the target tile
             std::cout << "Spell TargetType::Area not implemented yet for '" << spell.name << "'." << std::endl;
             // castSuccess = true; // Potentially successful
             break;

        default:
             std::cout << "Error: Unknown SpellTargetType encountered for '" << spell.name << "'." << std::endl;
             break;
    }

    // 4. Deduct Mana if Cast Was Successful
    if (castSuccess) {
        mana -= spell.manaCost;
        std::cout << "Mana remaining: " << mana << "/" << maxMana << std::endl;
    }

    return castSuccess; // Return true if mana was spent and effect (attempted to be) applied
}


// Example takeDamage implementation (add to PlayerCharacter if needed)
void PlayerCharacter::takeDamage(int amount) {
     health -= amount;
     std::cout << "Player took " << amount << " damage. Health: " << health << "/" << maxHealth << std::endl;
     if (health <= 0) {
          health = 0;
          std::cout << "Player has been defeated!" << std::endl;
          // TODO: Set a 'isDead' flag or trigger GameOver state in main.cpp
     }
}