// projectile.h
#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <SDL.h>
#include <string>

// Forward declare GameData if needed for update signature (it is needed)
struct GameData;

enum class ProjectileType {
    Firebolt,
    IceShard,
    // Add more as needed
};

class Projectile {
public:
    // --- Identification & State ---
    ProjectileType type;
    bool isActive;

    // --- NEW: Target Tracking ---
    int targetEnemyId = -1; // ID of the enemy being targeted (-1 if none/tile target)

    // --- Position & Movement ---
    float startX, startY;   // Visual world coordinates where it originated
    float targetX, targetY; // Initial visual world coordinates of the target center (still useful for non-homing or if target dies)
    float currentX, currentY; // Current visual world coordinates
    float speed;          // Pixels per second
    float dx, dy;           // Normalized direction vector components (-1.0 to 1.0)
    int damage;

    // --- Visuals ---
    SDL_Texture* texture; // Pointer to the texture to render (MUST NOT be null)
    int width, height;    // Dimensions for rendering

    // --- Constructor (Updated) ---
    Projectile(ProjectileType type, SDL_Texture* tex, int w, int h,
               float startX, float startY, float targetX, float targetY, float speed, int damage,
               int targetId = -1); // Added optional targetId parameter

    // --- Methods (Updated) ---
    // Updates position, returns true if target reached/hit this frame, false otherwise
    // Now requires GameData to find the target enemy by ID
    bool update(float deltaTime, const GameData& gameData);
    void render(SDL_Renderer* renderer, int cameraX, int cameraY) const;

private:
    // Calculate the normalized direction vector towards a specific point
    void calculateDirection(float toX, float toY);
};

#endif // PROJECTILE_H
