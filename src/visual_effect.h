// src/visual_effect.h
#ifndef VISUAL_EFFECT_H
#define VISUAL_EFFECT_H

#include <SDL.h>
#include <string>
#include <vector>

// Forward Declarations if needed
class AssetManager;

struct VisualEffect {
    // --- Position & Size ---
    float visualX;        // Center X coordinate in world space
    float visualY;        // Center Y coordinate in world space
    int renderWidth;      // Width for rendering the effect
    int renderHeight;     // Height for rendering the effect

    // --- Animation State ---
    std::vector<std::string> frameTextureKeys; // Keys for animation frames
    float animationTimer = 0.0f;
    int currentFrame = 0;
    float animationSpeed = 10.0f; // Frames per second (adjust per effect)
    bool loop = false;            // Should the animation loop?
    float duration = 0.0f;        // Total time the effect should last (in seconds). If 0, plays animation once.
    float timeElapsed = 0.0f;     // Time since the effect started

    // --- State ---
    bool markedForRemoval = false;

    // --- Constructor ---
    VisualEffect(float x, float y, int w, int h,
                 const std::vector<std::string>& frameKeys,
                 float speed, float lifeDuration, bool loops = false)
        : visualX(x), visualY(y), renderWidth(w), renderHeight(h),
          frameTextureKeys(frameKeys), animationSpeed(speed),
          loop(loops), duration(lifeDuration)
    {
        if (frameKeys.empty()) {
             SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "VisualEffect created with no frame keys!");
             markedForRemoval = true; // Mark for removal if no animation
        }
        // If duration is 0, calculate it based on non-looping animation length
        if (duration <= 0.0f && !loop && animationSpeed > 0.0f && !frameKeys.empty()) {
            duration = (1.0f / animationSpeed) * frameKeys.size();
        } else if (duration <= 0.0f && loop) {
            duration = 1.0f; // Default loop duration if not specified? Or mark error?
             SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Looping VisualEffect created with zero duration, defaulting to 1s.");
        } else if (animationSpeed <= 0.0f) {
             SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "VisualEffect created with zero animation speed!");
             markedForRemoval = true; // Cannot animate
        }

    }

    // --- Methods ---
    void update(float deltaTime);
    void render(SDL_Renderer* renderer, AssetManager& assets, int cameraX, int cameraY) const;
};

#endif // VISUAL_EFFECT_H