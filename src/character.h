// character.h
#ifndef CHARACTER_H
#define CHARACTER_H

#include "projectile.h"    // Include projectile definition for casting spells
#include "spell.h"         // Include your spell definitions
#include "status_effect.h" // Include for StatusEffect and EffectMagnitude
#include <SDL.h> // For SDL_Texture* forward declaration if needed, or include fully
#include <array> // For std::array
#include <map>
#include <optional> // Potentially for spell bar slots if allowing empty
#include <string> // For spell names eventually, if not already included by spell.h
#include <vector> // For std::vector

// Forward declaration to avoid circular dependency if AssetManager is only used
// for pointer/ref in header
class AssetManager;
class Enemy;         // Forward declare Enemy
struct GameData;     // Forward declare GameData
enum class RuneType; // Forward-declare RuneType from game_data.h to break
                     // circular dependency

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

const int MAX_SPELL_BAR_SLOTS =
    5; // Define this constant, should match GameData::MAX_HOTKEY_SPELLS

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

  // --- Status Effects ---
  std::vector<StatusEffect> activeStatusEffects; // <<< ADDED

  // --- Position & Movement ---
  int tileWidth;
  int tileHeight;
  float x;
  float y;
  int targetTileX;
  int targetTileY;
  int logicalTileX; // <<< NEW: Tile player definitively occupies logically
  int logicalTileY; // <<< NEW: Tile player definitively occupies logically
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

  // --- NEW: Void Infusion Animation Data ---
  std::vector<std::string>
      voidInfusionFrameTextureNames; // Keys for Void Infusion frames
  float voidInfusionAnimationTimer = 0.0f;
  int currentVoidInfusionFrame = 0;
  float voidInfusionAnimationSpeed = 10.0f; // Adjust speed as needed
  // --- END NEW ---

  // ADDED: Enum for facing direction INSIDE the struct
  enum class FacingDirection { Right, Left };
  // ADDED: Member variable to store current direction
  FacingDirection currentFacingDirection =
      FacingDirection::Left; // Default direction

  std::array<std::string, MAX_SPELL_BAR_SLOTS> spellBarSlots;

  // --- Constructor ---
  PlayerCharacter(CharacterType t, int initialTileX, int initialTileY,
                  int tileW, int tileH);

  // --- Methods ---
  void startMove(int targetX, int targetY);
  void update(float deltaTime,
              GameData &gameData); // Pass GameData for occupation grid updates
  void render(SDL_Renderer *renderer, AssetManager &assets, int cameraX,
              int cameraY) const; // Modified to use buff animation

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
  const Spell *getKnownSpellByIndex(int knownSpellIndex)
      const; // Returns pointer to allow checking for nullptr
  const Spell *getKnownSpellByName(const std::string &spellName) const;
  int getKnownSpellIndexByName(
      const std::string &spellName) const; // Helper to get index from name

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

  // --- Status Effect Methods --- // <<< ADDED
  void AddStatusEffect(StatusEffectType type, int duration,
                       EffectMagnitude magnitude); // Updated signature
  void RemoveStatusEffect(
      StatusEffectType type); // Optional: For dispel effects later
  bool HasStatusEffect(StatusEffectType type) const;
  void UpdateStatusEffectDurations(); // Called each turn end

  // --- Other Methods ---
  void takeDamage(int amount);
  void RegenerateMana(float timeStep);
  void ApplyTurnEndEffects();

  // --- NEW: Rune Inventory ---
  std::map<RuneType, int> runes;

  void addRune(RuneType type, int count = 1);
  int getRuneCount(RuneType type) const;
  bool canSpendRunes(RuneType type, int count) const; // For later use
  bool spendRunes(RuneType type, int count);          // For later use

  // Spellbook
  bool attemptToUnlockSpell(const std::string &spellNameToUnlock,
                            const GameData &gameData);
  bool hasSpellUnlocked(const std::string &spellName) const;
  std::pair<int, int> getTheoreticalSpellDamageRange(const Spell &spell) const;

  // --- NEW: Spell Bar Management ---
  void assignSpellToBar(int slotIndex, const std::string &spellName);
  void clearSpellBarSlot(int slotIndex);

}; // End of PlayerCharacter struct declaration

#endif // CHARACTER_H