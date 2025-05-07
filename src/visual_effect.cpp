// src/visual_effect.cpp
#include "visual_effect.h"
#include "asset_manager.h" // Include for rendering
#include <algorithm>       // For std::max/min
#include <cmath>           // For std::floor

void VisualEffect::update(float deltaTime) {
  if (markedForRemoval || frameTextureKeys.empty() || animationSpeed <= 0.0f) {
    return; // Don't update if already finished or invalid
  }

  timeElapsed += deltaTime;

  // Check duration-based removal first
  if (duration > 0.0f && timeElapsed >= duration) {
    markedForRemoval = true;
    return;
  }

  // Update animation frame
  animationTimer += deltaTime;
  float frameDuration = 1.0f / animationSpeed;

  if (animationTimer >= frameDuration) {
    int framesToAdvance =
        static_cast<int>(std::floor(animationTimer / frameDuration));
    animationTimer -=
        (framesToAdvance * frameDuration); // Subtract accumulated time

    int nextFrame = currentFrame + framesToAdvance;

    if (loop) {
      currentFrame = nextFrame % frameTextureKeys.size();
    } else {
      currentFrame = std::min(nextFrame, (int)frameTextureKeys.size() - 1);
      // If duration wasn't set, check if animation finished based on frames
      if (duration <= 0.0f &&
          currentFrame >= (int)frameTextureKeys.size() - 1) {
        markedForRemoval = true;
      }
    }
  }
}

void VisualEffect::render(SDL_Renderer *renderer, AssetManager &assets,
                          int cameraX, int cameraY) const {
  if (markedForRemoval || frameTextureKeys.empty() || currentFrame < 0 ||
      currentFrame >= frameTextureKeys.size()) {
    return; // Don't render finished or invalid effects
  }

  const std::string &currentKey = frameTextureKeys[currentFrame];
  SDL_Texture *texture = assets.getTexture(currentKey);

  if (texture) {
    SDL_Rect destRect;
    destRect.w = renderWidth;
    destRect.h = renderHeight;
    destRect.x =
        static_cast<int>(std::round(visualX - renderWidth / 2.0f)) - cameraX;
    destRect.y =
        static_cast<int>(std::round(visualY - renderHeight / 2.0f)) - cameraY;

    // --- Apply Alpha/Blending (Example: Fade out based on remaining duration)
    // ---
    Uint8 alpha = 255;
    if (duration > 0.0f) {
      // Example fade: Start fading in last 20% of duration
      float fadeStartTime = duration * 0.8f;
      if (timeElapsed > fadeStartTime &&
          duration > fadeStartTime) { // Ensure duration > fadeStartTime to
                                      // avoid division by zero
        float fadeProgress =
            (timeElapsed - fadeStartTime) / (duration - fadeStartTime);
        alpha =
            static_cast<Uint8>(255.0f * (1.0f - std::min(fadeProgress, 1.0f)));
      }
    }
    SDL_SetTextureAlphaMod(texture, alpha);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    // --- End Alpha/Blending ---

    SDL_RenderCopy(renderer, texture, nullptr, &destRect);

    // Reset texture mods if necessary (though AssetManager usually handles
    // this)
    SDL_SetTextureAlphaMod(texture, 255);

  } else {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "VisualEffect texture '%s' not found!", currentKey.c_str());
    // Optional: Draw a fallback placeholder rectangle
  }
}