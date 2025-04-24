// src/enemy.h

#ifndef ENEMY_H
#define ENEMY_H

#include <SDL.h>
#include <string> // Needed for std::string

// Forward declarations
struct PlayerCharacter;
struct Level;
struct GameData;
class AssetManager; // Forward declare AssetManager

// *** NEW: Define Enemy Types ***
enum class EnemyType {
    SLIME // Add other types like SKELETON, GOBLIN etc. later
};
// -----------------------------

class Enemy {
    public:
    // --- Core Attributes (Set by Type) ---
    EnemyType type;
    int health;
    int maxHealth; // Good practice to store max health
    int width;     // Visual width
    int height;    // Visual height
    int arcanaValue;
    std::string textureName;
    float moveDuration;

    // --- Positional & State ---
    int x; // Logical tile X
    int y; // Logical tile Y
    float visualX; // Current visual x position (floating point)
    float visualY; // Current visual y position (floating point)
    bool isMoving;
    int startTileX;
    int startTileY;
    int targetTileX;
    int targetTileY;
    float moveProgress;
    float moveTimer;
    bool hasTakenActionThisTurn;

    // --- Context ---
    int tileWidth;  // Store the tile context (used for visual positioning)
    int tileHeight;


    // *** MODIFIED: Constructor signature takes type and position ***
    Enemy(EnemyType type, int startX, int startY, int tileW, int tileH);
    // -------------------------------------------------------------

    void takeAction(const Level& level, PlayerCharacter& player, GameData& gameData);
    void update(float deltaTime, GameData& gameData);
    void render(SDL_Renderer* renderer, AssetManager& assets, int cameraX, int cameraY, float visibilityAlpha) const;
    void startMove(int targetX, int targetY);
    void takeDamage(int amount);
};

#endif // ENEMY_H