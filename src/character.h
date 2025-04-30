// character.h
#ifndef CHARACTER_H
#define CHARACTER_H

#include "projectile.h" // Include projectile definition for casting spells
#include "spell.h"      // Include your spell definitions
#include <SDL.h> // For SDL_Texture* forward declaration if needed, or include fully
#include <string> // For spell names eventually, if not already included by spell.h
#include <vector> // For std::vector

// Forward declaration to avoid circular dependency if AssetManager is only used
// for pointer/ref in header
class AssetManager;
class Enemy;     // Forward declare Enemy
struct GameData; // Forward declare GameData

// --- Constants for Leveling (Example Placeholder Values) ---
const int ARCANA_PER_LEVEL = 100;
const int VITALITY_PER_LEVEL = 1;
const int INTELLIGENCE_PER_LEVEL = 2;
const int SPIRIT_PER_LEVEL = 1;
const int AGILITY_PER_LEVEL = 1;

// Derived stat constants (Example Placeholders)
const int HP_PER_VITALITY = 10;
const int MANA_PER_INTELLIGENCE = 5;
const float MANA_REGEN_PER_SPIRIT = 0.1f;
const float SPEED_MOD_PER_AGILITY = 0.005f;

enum class CharacterType { FemaleMage, MaleMage };

struct PlayerCharacter {
  // --- Basic Info ---
  CharacterType type;

  // --- Resources ---
  int health;
  int maxHealth;
  int mana;
  int maxMana;

  // --- Arcana & Leveling ---
  int level = 1;
  int currentArcana = 0;

  // --- Core Base Stats ---
  int baseVitality = 5;
  int baseIntelligence = 10;
  int baseSpirit = 7;
  int baseAgility = 8;

  // --- Calculated Derived Stats ---
  float fractionalMana = 0.0f;
  float manaRegenRate = 0.0f;
  float spellDamageModifier = 1.0f;
  int currentShield = 0;      // Current remaining shield points
  int shieldDecayPerTurn = 0; // Amount the shield decays each turn

  // --- Position & Movement ---
  int tileWidth;
  int tileHeight;
  float x;
  float y;
  int targetTileX;
  int targetTileY;
  bool isMoving;
  int startTileX;
  int startTileY;
  float moveProgress;
  float moveDuration = 0.1f; // Default, calculated later
  float moveTimer;

  // --- Spellcasting ---
  std::vector<Spell> knownSpells;

  // --- Animation & Orientation ---
  float idleAnimationTimer = 0.0f;
  int currentIdleFrame = 0;
  float idleAnimationSpeed =
      4.0f; // Frames per second (adjust as needed), made non-const
  std::vector<std::string>
      idleFrameTextureNames; // Holds the keys for all idle frames

  // ADDED: Walking Animation Data
  std::vector<std::string>
      walkFrameTextureNames; // Holds keys for walking frames
  float walkAnimationTimer = 0.0f;
  int currentWalkFrame = 0;
  float walkAnimationSpeed =
      8.0f; // Frames per second for walking (adjust speed as needed)

  // ADDED: Targeting Animation Data
  std::vector<std::string>
      targetingFrameTextureNames; // Holds keys for targeting frames
  float targetingAnimationTimer = 0.0f;
  int currentTargetingFrame = 0;
  float targetingAnimationSpeed =
      4.0f; // Frames per second for targeting (adjust as needed)

  // ---> ADD Ward Animation State <---
  std::vector<std::string>
      wardFrameTextureKeys; // Holds keys like "ward_frame_1"
  float wardAnimationTimer = 0.0f;
  int currentWardFrame = 0;
  float wardAnimationSpeed = 8.0f; // Frames per second (adjust speed)

  // ADDED: Enum for facing direction INSIDE the struct
  enum class FacingDirection { Right, Left };
  // ADDED: Member variable to store current direction
  FacingDirection currentFacingDirection =
      FacingDirection::Left; // Default direction

  // --- Constructor ---
  PlayerCharacter(CharacterType t, int initialTileX, int initialTileY,
                  int tileW, int tileH);

  // --- Methods ---
  void startMove(int targetX, int targetY);
  void update(float deltaTime,
              GameData &gameData); // Pass GameData for occupation grid updates

  // --- Leveling & Stat Methods ---
  void GainArcana(int amount);
  bool CanAffordArcana(int cost) const;
  bool SpendArcana(int amount);
  void RecalculateStats();

  // --- Getter Methods for Effective Stats ---
  int GetEffectiveVitality() const;
  int GetEffectiveIntelligence() const;
  int GetEffectiveSpirit() const;
  int GetEffectiveAgility() const;

  // --- Spellcasting Methods ---
  bool canCastSpell(int spellIndex) const;
  // Pass AssetManager by pointer or reference if needed for textures
  bool castSpell(int spellIndex, int targetX, int targetY, GameData &gameData,
                 AssetManager *assets); // Pass AssetManager
  const Spell &getSpell(int spellIndex) const;
  int GetEffectiveSpellRange(
      int spellIndex) const; // Calculates range including modifiers
  int GetEffectiveManaCost(
      int spellIndex) const; // Calculates cost including modifiers
  int calculateSpellDamage(int spellIndex, int targetTileX, int targetTileY,
                           const Enemy *target = nullptr) const;
  int calculateSpellDamage(int numDice, int dieType, int bonus, int targetTileX,
                           int targetTileY,
                           const Enemy *target = nullptr) const;

  // --- Other Methods ---
  void takeDamage(int amount);
  void RegenerateMana(float timeStep);
  void ApplyTurnEndEffects();

}; // End of PlayerCharacter struct declaration

#endif // CHARACTER_H
