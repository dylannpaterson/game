// src/enemy.h

#ifndef ENEMY_H
#define ENEMY_H

#include <SDL.h>
#include <string>

// Forward declarations
struct PlayerCharacter;
struct Level;
struct GameData;
struct IntendedAction;
class AssetManager;

enum class EnemyType {
    SLIME
};

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

    // --- Context ---
    int tileWidth;
    int tileHeight;

    // --- Constructor ---
    Enemy(int uniqueId, EnemyType type, int startX, int startY, int tileW, int tileH);

    // --- Planning Function ---
    IntendedAction planAction(const Level& levelData, const PlayerCharacter& player, const GameData& gameData) const;

    // --- Action Execution & Update ---
    void update(float deltaTime, GameData& gameData);
    void render(SDL_Renderer* renderer, AssetManager& assets, int cameraX, int cameraY, float visibilityAlpha) const;
    void startMove(int targetX, int targetY);
    void takeDamage(int amount);
    int GetAttackDamage() const;

    // --- NEW: Public Static Method to Reset ID Counter ---
    static void resetIdCounter() { nextId = 0; }
    // --- Static method to get next ID during creation ---
    static int getNextId() { return nextId++; }


private:
    // --- Static ID counter (remains private) ---
    static int nextId;
};

#endif // ENEMY_H
