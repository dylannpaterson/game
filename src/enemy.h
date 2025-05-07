// src/enemy.h

#ifndef ENEMY_H
#define ENEMY_H

#include "status_effect.h"
#include <SDL.h>
#include <string>
#include <vector> // Added for animation frame names

// Forward declarations
struct PlayerCharacter;
struct Level;
struct GameData;
struct IntendedAction;
class AssetManager;

enum class EnemyType { SLIME };

class Enemy {
public:
  // --- Unique ID ---
  int id; // Unique ID for targeting

  // --- Core Attributes ---
  EnemyType type;
  int health;
  int maxHealth;
  int width;
  int height;
  int arcanaValue;
  std::string textureName;
  float moveDuration;
  int baseAttackDamage;

  // --- Positional & State ---
  int x; // Logical tile X
  int y; // Logical tile Y
  float visualX;
  float visualY;
  bool isMoving;
  int startTileX;
  int startTileY;
  int targetTileX;
  int targetTileY;
  float moveProgress;
  float moveTimer;
  bool isAttacking = false;

  // --- Context ---
  int tileWidth;
  int tileHeight;

  // --- Status Effects ---
  std::vector<StatusEffect> activeStatusEffects; // <<< ADDED

  // --- Facing Direction ---
  enum class FacingDirection { Left, Right };
  FacingDirection currentFacingDirection;

  // --- ADDED: Idle Animation ---
  std::vector<std::string> idleFrameTextureNames; // Keys for idle frames
  float idleAnimationTimer = 0.0f;
  int currentIdleFrame = 0;
  float idleAnimationSpeed =
      4.0f; // Frames per second for idle (adjust per enemy type)

  // --- ADDED: Walk Animation ---
  std::vector<std::string> walkFrameTextureNames; // Keys for walk frames
  float walkAnimationTimer = 0.0f;
  int currentWalkFrame = 0;
  float walkAnimationSpeed =
      4.0f; // Frames per second for walking (adjust per enemy type)

  // --- ADDED: Attack Animation ---
  std::vector<std::string> attackFrameTextureNames; // Keys for attack frames
  float attackAnimationTimer = 0.0f;
  int currentAttackFrame = 0;
  float attackAnimationSpeed =
      10.0f; // Attack might be faster (adjust per enemy type)
  float attackAnimationDuration =
      0.0f; // Will be calculated based on speed and frame count
            // --- ADDED: Attack Movement Variables ---
  float attackStartX = 0.0f;  // Visual X position when attack started
  float attackStartY = 0.0f;  // Visual Y position when attack started
  float attackTargetX = 0.0f; // Target visual X for the lunge (player pos)
  float attackTargetY = 0.0f; // Target visual Y for the lunge (player pos)
  float lungeDistanceRatio =
      0.4f; // How far towards the target to lunge (e.g., 40%)
  // --- END ADDED ---

  // --- Constructor ---
  Enemy(int uniqueId, EnemyType type, int startX, int startY, int tileW,
        int tileH);

  // --- Planning Function ---
  IntendedAction planAction(const Level &levelData,
                            const PlayerCharacter &player,
                            const GameData &gameData) const;

  // --- Action Execution & Update ---
  void update(float deltaTime, GameData &gameData);
  void render(SDL_Renderer *renderer, AssetManager &assets, int cameraX,
              int cameraY, float visibilityAlpha) const;
  void startMove(int targetX, int targetY);
  void startAttackAnimation(const GameData &gameData);
  void takeDamage(int amount);
  int GetAttackDamage() const;
  void applyFloorScaling(int currentFloorIndex, float scalingFactorPerFloor);

  // --- NEW: Public Static Method to Reset ID Counter ---
  static void resetIdCounter() { nextId = 0; }
  // --- Static method to get next ID during creation ---
  static int getNextId() { return nextId++; }

  void AddStatusEffect(StatusEffectType type, int duration);
  void RemoveStatusEffect(
      StatusEffectType type); // Optional: For dispel effects later
  bool HasStatusEffect(StatusEffectType type) const;
  void UpdateStatusEffectDurations(); // Called each turn end

private:
  // --- Static ID counter (remains private) ---
  static int nextId;
};

#endif // ENEMY_H
