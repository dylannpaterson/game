#ifndef ORBITAL_MISSILE_H
#define ORBITAL_MISSILE_H

#include <string>
#include <SDL.h> // For SDL_Renderer

// Forward Declarations
struct GameData;
class AssetManager; // Needed for rendering

enum class OrbitalState {
    Waiting,    // Lingering, scanning
    Launched    // Target acquired, projectile spawned, self should be removed
};

struct OrbitalMissile {
    // Removed ownerPlayerId - accesses player via GameData
    float visualX, visualY; // Current position in the world
    float lifetimeRemaining; // Seconds until it disappears
    float scanTimer;         // Timer to control scan frequency
    int formationIndex;    // 0, 1, 2... for horizontal spacing

    // Base stats inherited from spell
    int acquisitionRangeSq; // Store squared range for efficiency
    int damageNumDice;
    int damageDieType;
    int damageBonus;
    std::string projectileTextureKey; // Texture for the *launched* projectile
    float projectileSpeed;

    OrbitalState currentState = OrbitalState::Waiting;
    bool markedForRemoval = false; // Flag for cleanup

    // Constructor
    OrbitalMissile(float startX, float startY, float lifetime,
                   int acqRange, int dmgNumDice, int dmgDie, int dmgBonus,
                   const std::string& projTexKey, float projSpeed,
                   int formIndex); // Removed ownerId

    // Update function called each frame/turn
    // Needs access to GameData to find player position and enemies
    // Returns true if the orbital should be removed (launched or expired)
    bool update(float deltaTime, GameData& gameData, AssetManager& assets);

    // Render function
    void render(SDL_Renderer* renderer, AssetManager& assets, int cameraX, int cameraY) const;
};

#endif // ORBITAL_MISSILE_H