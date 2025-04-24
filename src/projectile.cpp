// projectile.cpp
#include "projectile.h"
#include <cmath> // For sqrt, atan2 (optional), or just normalization
#include <SDL.h> // For SDL_Log if debugging

Projectile::Projectile(ProjectileType pType, SDL_Texture* tex, int w, int h,
                       float sX, float sY, float tX, float tY, float spd, int dmg)
    : type(pType),
      isActive(true), // Start active
      startX(sX), startY(sY),
      targetX(tX), targetY(tY),
      currentX(sX), currentY(sY), // Start at the beginning
      speed(spd),
      damage(dmg),
      dx(0.0f), dy(0.0f), // Initialize direction
      texture(tex),       // Store pointer to existing texture
      width(w), height(h)
{
    if (!texture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Projectile created with null texture!");
        isActive = false; // Immediately deactivate if no texture
    }
    calculateDirection(); // Calculate dx, dy based on start/target
}

void Projectile::calculateDirection() {
    float vecX = targetX - startX;
    float vecY = targetY - startY;
    float magnitude = std::sqrt(vecX * vecX + vecY * vecY);

    if (magnitude > 0.0001f) { // Avoid division by zero
        dx = vecX / magnitude;
        dy = vecY / magnitude;
    } else {
        // Target is same as start? Or very close. Don't move.
        dx = 0.0f;
        dy = 0.0f;
        // Or maybe set isActive = false immediately? Depends on desired behavior.
        isActive = false;
         SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Projectile target is same as start.");
    }
}

// Updates position, returns true if target reached this frame
bool Projectile::update(float deltaTime) {
    if (!isActive) {
        return false; // Don't update inactive projectiles
    }

    // Calculate distance moved this frame
    float distanceMoved = speed * deltaTime;

    // Calculate remaining distance to target
    float remainingX = targetX - currentX;
    float remainingY = targetY - currentY;
    float remainingDistSq = remainingX * remainingX + remainingY * remainingY; // Use squared distance for check

    // Check if the movement this frame will overshoot the target
    if (distanceMoved * distanceMoved >= remainingDistSq) {
        // Reached target (or very close)
        currentX = targetX; // Snap to target
        currentY = targetY;
        isActive = false; // Mark for removal
        SDL_Log("Projectile reached target.");
        return true; // Indicate target reached
    } else {
        // Move along the direction vector
        currentX += dx * distanceMoved;
        currentY += dy * distanceMoved;
        return false; // Still moving
    }
}

void Projectile::render(SDL_Renderer* renderer, int cameraX, int cameraY) const {
    if (!isActive || !texture) {
        return; // Don't render inactive or textureless projectiles
    }

    SDL_Rect destRect;
    // Center the projectile sprite on its currentX/Y
    destRect.x = static_cast<int>(currentX - width / 2.0f) - cameraX;
    destRect.y = static_cast<int>(currentY - height / 2.0f) - cameraY;
    destRect.w = width;
    destRect.h = height;

    // TODO: Add rotation based on dx, dy if desired (using SDL_RenderCopyEx)
    // double angle = atan2(dy, dx) * 180.0 / M_PI;
    // SDL_RenderCopyEx(renderer, texture, nullptr, &destRect, angle, nullptr, SDL_FLIP_NONE);

    SDL_RenderCopy(renderer, texture, nullptr, &destRect); // Simple render without rotation
}