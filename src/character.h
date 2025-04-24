// character.h
#ifndef CHARACTER_H
#define CHARACTER_H

#include <vector>   // For std::vector
#include <string>   // For spell names eventually, if not already included by spell.h
#include "spell.h"  // Include your spell definitions
#include "projectile.h" // Include projectile definition for casting spells


// Forward declaration if Enemy methods are used only in .cpp

#include "enemy.h" // Assuming Enemy definition is needed for castSpell signature

struct GameData;

// --- Constants for Leveling (Example Placeholder Values) ---
// Consider moving these to game_data.h later if preferred, but defining
// them here keeps character-specific progression self-contained initially.
const int ARCANA_PER_LEVEL = 100;
const int VITALITY_PER_LEVEL = 1;   // e.g., +1 Vitality per level
const int INTELLIGENCE_PER_LEVEL = 2; // e.g., +2 Intelligence per level
const int SPIRIT_PER_LEVEL = 1;     // e.g., +1 Spirit per level
const int AGILITY_PER_LEVEL = 1;      // e.g., +1 Agility per level

// Derived stat constants (Example Placeholders)
const int HP_PER_VITALITY = 10;     // e.g., +10 Max HP per point of Vitality
const int MANA_PER_INTELLIGENCE = 5; // e.g., +5 Max Mana per point of Intelligence
const float MANA_REGEN_PER_SPIRIT = 0.1f; // e.g., +0.1 Mana per turn per point of Spirit
const float SPEED_MOD_PER_AGILITY = 0.005f; // e.g., Reduces moveDuration per point of Agility



enum class CharacterType {
    FemaleMage,
    MaleMage
    // Add other types as needed
};

struct PlayerCharacter {
    // --- Basic Info ---
    CharacterType type;
    // int level; // Already exists

    // --- Resources ---
    int health;    // Current health
    int maxHealth; // Current maximum health (calculated)
    int mana;      // Current mana
    int maxMana;   // Current maximum mana (calculated)

    // --- NEW: Arcana & Leveling ---
    int level = 1;             // Start at level 1
    int currentArcana = 0;   // Start with 0 Arcana

    // --- NEW: Core Base Stats (Starting values at level 1) ---
    int baseVitality = 5;
    int baseIntelligence = 10;
    int baseSpirit = 7;
    int baseAgility = 8;

    // --- Calculated Effective Stats (No need to store if calculated on the fly) ---
    // We will use GetEffective...() methods instead.

    // --- Calculated Derived Stats ---
    float fractionalMana = 0.0f;
    float manaRegenRate = 0.0f; // Mana regenerated per turn/second (calculated)
    float spellDamageModifier = 1.0f; // Multiplier or flat bonus? Let's use multiplier for now (calculated)
    // float moveDuration; // Already exists, will be calculated


    // --- Position & Movement ---
    int tileWidth; // Already exists
    int tileHeight; // Already exists
    float x;        // Current visual x position
    float y;        // Current visual y position
    int targetTileX; // Logical grid X
    int targetTileY; // Logical grid Y
    bool isMoving;   // Already exists
    int startTileX;  // Already exists
    int startTileY;  // Already exists
    float moveProgress; // Already exists
    float moveDuration = 0.1f; // Default, will be modified by Agility
    float moveTimer;    // Already exists

    // --- Spellcasting ---
    std::vector<Spell> knownSpells; // Already exists

    // --- Constructor ---
    // Updated constructor to reflect new system (remove level, hp, mp etc. if calculated)
    PlayerCharacter(CharacterType t, int initialTileX, int initialTileY, int tileW, int tileH);

    // --- Methods ---
    void startMove(int targetX, int targetY); // Existing
    void update(float deltaTime, GameData& gameData); // Existing

    // --- NEW: Leveling & Stat Methods ---
    void GainArcana(int amount);
    bool CanAffordArcana(int cost) const; // Added const
    bool SpendArcana(int amount); // Returns true if successful
    void RecalculateStats();      // Central function to update everything based on level

    // --- NEW: Getter Methods for Effective Stats ---
    // These calculate the stat based on base + level bonuses
    int GetEffectiveVitality() const;
    int GetEffectiveIntelligence() const;
    int GetEffectiveSpirit() const;
    int GetEffectiveAgility() const;

    // --- Spellcasting Methods ---
    bool canCastSpell(int spellIndex) const; // Existing - might check calculated maxMana now
    // Pass enemies by reference to allow modification (damage)
    bool castSpell(int spellIndex, int targetX, int targetY,
        std::vector<Enemy>& enemies, std::vector<Projectile>& projectiles,
        AssetManager* assets); // Existing - will need modification for damage calc
    const Spell& getSpell(int spellIndex) const; // Existing

    // --- Other Methods ---
    void takeDamage(int amount); // Existing - checks against calculated maxHealth

    // --- NEW: Mana Regeneration ---
    void RegenerateMana(float timeStep); // Called each turn/frame

}; // End of PlayerCharacter struct declaration

#endif // CHARACTER_H