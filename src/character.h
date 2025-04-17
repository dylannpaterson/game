// character.h
#ifndef CHARACTER_H
#define CHARACTER_H

#include <vector>   // For std::vector
#include <string>   // For spell names eventually, if not already included by spell.h
#include "spell.h"  // Include your spell definitions
#include "projectile.h" // Include projectile definition for casting spells

// Forward declaration if Enemy methods are used only in .cpp

#include "enemy.h" // Assuming Enemy definition is needed for castSpell signature

enum class CharacterType {
    FemaleMage,
    MaleMage
    // Add other types as needed
};

struct PlayerCharacter {
    // --- Attributes ---
    CharacterType type;
    int health;
    int maxHealth;
    int mana;       // Added for spellcasting
    int maxMana;    // Added for spellcasting
    int level;

    int tileWidth;
    int tileHeight;

    // --- Position & Movement ---
    float x;        // Current visual x position (center of tile)
    float y;        // Current visual y position (center of tile)
    int targetTileX; // Logical grid X coordinate the character occupies/is moving to
    int targetTileY; // Logical grid Y coordinate the character occupies/is moving to

    // --- Movement State (Lerping) ---
    bool isMoving;
    int startTileX;  // Tile moving FROM
    int startTileY;  // Tile moving FROM
    float moveProgress; // 0.0 to 1.0
    float moveDuration; // Time in seconds for one tile move
    float moveTimer;    // Time elapsed since starting current move

    // --- Spellcasting ---
    std::vector<Spell> knownSpells; // Character's spellbook

    // --- Constructor ---
    PlayerCharacter(CharacterType t, int hp, int maxHp, int mp, int maxMp, int lvl,
                    int initialTileX, int initialTileY, int tileWidth, int tileHeight, float moveDur = 0.2f); // Added mana, maxMana, initial tile coords

    // --- Methods ---
    void startMove(int targetX, int targetY);
    void update(float deltaTime, int tileWidth, int tileHeight); // Handles movement updates

    // Spellcasting Methods
    bool canCastSpell(int spellIndex) const; // Check if enough mana, etc.
    // Pass enemies by reference to allow modification (damage)
    bool castSpell(int spellIndex, int targetX, int targetY,
        std::vector<Enemy>& enemies, std::vector<Projectile>& projectiles, // Add vector ref
        SDL_Texture* projectileTexture);
    const Spell& getSpell(int spellIndex) const; // Safely get spell info

    // Add other methods as needed (e.g., takeDamage, gainExperience)
    void takeDamage(int amount); // Example

}; // End of PlayerCharacter struct declaration

#endif // CHARACTER_H