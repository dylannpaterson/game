// projectile.h
#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <SDL.h>
#include <string> // For potential future use (e.g., effect names)

// Forward declare if needed, or include headers for dependencies
// (None strictly needed for this basic version)

// Optional: Enum to differentiate projectile types if visuals/behavior vary significantly
enum class ProjectileType {
    Firebolt,
    IceShard,
    // Add more as needed
};

class Projectile {
public:
    // --- Identification & State ---
    ProjectileType type;
    bool isActive; // Flag to mark for removal when projectile finishes

    // --- Position & Movement ---
    float startX, startY;   // Visual world coordinates where it originated
    float targetX, targetY; // Visual world coordinates of the center of the target tile
    float currentX, currentY; // Current visual world coordinates
    float speed;          // Pixels per second
    float dx, dy;           // Normalized direction vector components (-1.0 to 1.0)
    int damage;

    // --- Visuals ---
    SDL_Texture* texture; // Pointer to the texture to render (MUST NOT be null)
    int width, height;    // Dimensions for rendering

    // --- Constructor ---
    Projectile(ProjectileType type, SDL_Texture* tex, int w, int h,
               float startX, float startY, float targetX, float targetY, float speed, int damage);

    // --- Methods ---
    // Updates position, returns true if target reached this frame, false otherwise
    bool update(float deltaTime);
    void render(SDL_Renderer* renderer, int cameraX, int cameraY) const;

private:
    // Calculate the normalized direction vector
    void calculateDirection();
};

#endif // PROJECTILE_H