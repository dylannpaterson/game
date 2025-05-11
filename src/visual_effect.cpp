// src/visual_effect.cpp
#include "visual_effect.h"
#include "asset_manager.h" // Include for rendering
#include "game_data.h"     // Include for GameData definition
#include "utils.h"         // Include for isWithinBounds
#include <algorithm>       // For std::max/min
#include <cmath>           // For std::floor, std::round
#include <utility>         // For std::make_pair

// --- Constructor Implementation for standard effects ---
VisualEffect::VisualEffect(float x, float y, int w, int h,
                           const std::vector<std::string> &frameKeys,
                           float speed, float lifeDuration, bool loops)
    : visualX(x), visualY(y), renderWidth(w), renderHeight(h),
      angleDegrees(0.0),              // Default to no rotation
      rotationOrigin({w / 2, h / 2}), // Default origin to center
      frameTextureKeys(frameKeys), animationSpeed(speed), loop(loops),
      duration(lifeDuration) {
  if (frameKeys.empty()) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "VisualEffect created with no frame keys!");
    markedForRemoval = true; // Mark for removal if no animation
  }
  // If duration is 0, calculate it based on non-looping animation length
  if (duration <= 0.0f && !loop && animationSpeed > 0.0f &&
      !frameTextureKeys.empty()) {
    duration = (1.0f / animationSpeed) * frameTextureKeys.size();
  } else if (duration <= 0.0f && loop) {
    duration = 1.0f; // Default loop duration if not specified? Or mark error?
    SDL_LogWarn(
        SDL_LOG_CATEGORY_APPLICATION,
        "Looping VisualEffect created with zero duration, defaulting to 1s.");
  } else if (animationSpeed <= 0.0f) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "VisualEffect created with zero animation speed!");
    markedForRemoval = true; // Cannot animate
  }
}

// --- Constructor Implementation for linear/rotated effects with visibility
// control ---
VisualEffect::VisualEffect(
    float x, float y, int w, int h, const std::vector<std::string> &frameKeys,
    float speed, float lifeDuration, bool loops, double angle, SDL_Point origin,
    GameData *gameDataPtr,
    const std::vector<std::pair<int, int>> &tilesToAffect,
    float visibilityFadeStartRatio)
    : visualX(x), visualY(y), renderWidth(w), renderHeight(h),
      angleDegrees(angle), rotationOrigin(origin), frameTextureKeys(frameKeys),
      animationSpeed(speed), loop(loops), duration(lifeDuration),
      gameDataRef(gameDataPtr),
      visibilityFadeStartTimeRatio(visibilityFadeStartRatio) {
  if (frameKeys.empty()) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "VisualEffect created with no frame keys!");
    markedForRemoval = true; // Mark for removal if no animation
  }
  // If duration is 0, calculate it based on non-looping animation length
  if (duration <= 0.0f && !loop && animationSpeed > 0.0f &&
      !frameTextureKeys.empty()) {
    duration = (1.0f / animationSpeed) * frameTextureKeys.size();
  } else if (duration <= 0.0f && loop) {
    duration = 1.0f; // Default loop duration if not specified? Or mark error?
    SDL_LogWarn(
        SDL_LOG_CATEGORY_APPLICATION,
        "Looping VisualEffect created with zero duration, defaulting to 1s.");
  } else if (animationSpeed <= 0.0f) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "VisualEffect created with zero animation speed!");
    markedForRemoval = true; // Cannot animate
  }

  // Capture original visibility for affected tiles upon creation
  if (gameDataRef) {
    for (const auto &tile : tilesToAffect) {
      if (isWithinBounds(tile.first, tile.second,
                         gameDataRef->currentLevel.width,
                         gameDataRef->currentLevel.height)) {
        // Correctly construct the pair for push_back
        affectedTilesWithOriginalVisibility.push_back(std::make_pair(
            tile, gameDataRef->visibilityMap[tile.second][tile.first]));
      }
    }
  }
}

void VisualEffect::update(float deltaTime) {
  // Check if marked for removal and visibility hasn't been reset yet
  if (markedForRemoval) {
    if (!visibilityReset && gameDataRef &&
        !affectedTilesWithOriginalVisibility.empty()) {
      // Restore original visibility
      for (const auto &tile_pair : affectedTilesWithOriginalVisibility) {
        int tileX = tile_pair.first.first;
        int tileY = tile_pair.first.second;
        float originalVis = tile_pair.second;
        if (gameDataRef &&
            isWithinBounds(tileX, tileY, gameDataRef->currentLevel.width,
                           gameDataRef->currentLevel.height)) {
          gameDataRef->visibilityMap[tileY][tileX] = originalVis;
        }
      }
      visibilityReset = true; // Mark visibility as reset
    }
    return; // Don't update if already finished and visibility reset
  }

  if (frameTextureKeys.empty() || animationSpeed <= 0.0f) {
    // If invalid state, mark for removal and handle visibility reset
    markedForRemoval = true;
    if (!visibilityReset && gameDataRef &&
        !affectedTilesWithOriginalVisibility.empty()) {
      // Restore original visibility
      for (const auto &tile_pair : affectedTilesWithOriginalVisibility) {
        int tileX = tile_pair.first.first;
        int tileY = tile_pair.first.second;
        float originalVis = tile_pair.second;
        if (gameDataRef &&
            isWithinBounds(tileX, tileY, gameDataRef->currentLevel.width,
                           gameDataRef->currentLevel.height)) {
          gameDataRef->visibilityMap[tileY][tileX] = originalVis;
        }
      }
      visibilityReset = true; // Mark visibility as reset
    }
    return;
  }

  timeElapsed += deltaTime;

  // Check duration-based removal first
  if (duration > 0.0f && timeElapsed >= duration) {
    markedForRemoval = true;
    // Before returning, if we were managing visibility, ensure it's fully off
    if (!visibilityReset && gameDataRef &&
        !affectedTilesWithOriginalVisibility.empty()) {
      // Restore original visibility
      for (const auto &tile_pair : affectedTilesWithOriginalVisibility) {
        int tileX = tile_pair.first.first;
        int tileY = tile_pair.first.second;
        float originalVis = tile_pair.second;
        if (gameDataRef &&
            isWithinBounds(tileX, tileY, gameDataRef->currentLevel.width,
                           gameDataRef->currentLevel.height)) {
          gameDataRef->visibilityMap[tileY][tileX] = originalVis;
        }
      }
      visibilityReset = true; // Mark visibility as reset
    }
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
        // Before returning, if we were managing visibility, ensure it's fully
        // off
        if (!visibilityReset && gameDataRef &&
            !affectedTilesWithOriginalVisibility.empty()) {
          // Restore original visibility
          for (const auto &tile_pair : affectedTilesWithOriginalVisibility) {
            int tileX = tile_pair.first.first;
            int tileY = tile_pair.first.second;
            float originalVis = tile_pair.second;
            if (gameDataRef &&
                isWithinBounds(tileX, tileY, gameDataRef->currentLevel.width,
                               gameDataRef->currentLevel.height)) {
              gameDataRef->visibilityMap[tileY][tileX] = originalVis;
            }
          }
          visibilityReset = true; // Mark visibility as reset
        }
        return;
      }
    }
  }

  // --- Update Visibility based on Animation Progress ---
  if (gameDataRef && !affectedTilesWithOriginalVisibility.empty()) {
    float animationProgress = 0.0f;
    if (duration > 0.0f) { // If duration is set, use time elapsed
      animationProgress = std::min(timeElapsed / duration, 1.0f);
    } else if (animationSpeed > 0.0f &&
               !frameTextureKeys
                    .empty()) { // If no duration, use frame progress
      // Calculate progress based on time elapsed relative to total animation
      // time
      float totalAnimationDuration =
          (1.0f / animationSpeed) * frameTextureKeys.size();
      if (totalAnimationDuration > 0.0f) {
        animationProgress =
            std::min(timeElapsed / totalAnimationDuration, 1.0f);
      }
    }

    // Define the point in animation progress where fading starts
    float fadeStartTimeRatio =
        visibilityFadeStartTimeRatio; // Use the member variable

    if (animationProgress >= fadeStartTimeRatio) {
      // Calculate fade progress from 0.0 to 1.0 during the fade portion
      float fadeDurationRatio = 1.0f - fadeStartTimeRatio;
      if (fadeDurationRatio > 0.0f) { // Avoid division by zero
        float fadeProgress =
            (animationProgress - fadeStartTimeRatio) / fadeDurationRatio;
        fadeProgress = std::min(fadeProgress, 1.0f); // Clamp to 1.0

        // Visibility goes from 1.0 (at fadeStartTimeRatio) to 0.0 (at 1.0
        // progress)
        currentVisibility = 1.0f - fadeProgress;
      } else {
        currentVisibility = 1.0f; // If fadeDurationRatio is 0, stay fully
                                  // visible during the animation
      }
    } else {
      currentVisibility = 1.0f; // Fully visible before the fade starts
    }

    // Ensure visibility is within valid range [0.0, 1.0]
    currentVisibility = std::max(0.0f, std::min(currentVisibility, 1.0f));

    // Apply the calculated visibility to the affected tiles *every frame*
    updateAffectedTilesVisibility();
  }
  // --- End Visibility Update ---
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

    // Calculate the top-left corner of the destination rectangle
    // The visualX and visualY represent the desired center of the effect,
    // but SDL_RenderCopyEx positions based on the top-left corner.
    // We also need to account for the rotation origin.
    // If the origin is (0,0), the top-left is simply (visualX - cameraX,
    // visualY - cameraY). If the origin is (w/2, h/2) (center), the top-left is
    // (visualX - w/2 - cameraX, visualY - h/2 - cameraY). For a custom origin
    // (ox, oy), the top-left is (visualX - ox - cameraX, visualY - oy -
    // cameraY).

    destRect.x =
        static_cast<int>(std::round(visualX - rotationOrigin.x)) - cameraX;
    destRect.y =
        static_cast<int>(std::round(visualY - rotationOrigin.y)) - cameraY;

    // --- Apply Alpha/Blending based on the calculated visual alpha ---
    // The visual alpha is now separate from the visibility alpha applied to the
    // map. We can still fade the visual effect itself if desired, perhaps
    // faster than the visibility. For now, let's use the calculated visibility
    // alpha for the visual effect's alpha as well, to keep them in sync
    // visually. If you want independent visual fading, modify this.
    Uint8 visualAlpha = static_cast<Uint8>(currentVisibility * 255.0f);

    SDL_SetTextureAlphaMod(texture, visualAlpha);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    // --- End Alpha/Blending ---

    // Use SDL_RenderCopyEx to apply rotation and render
    SDL_RenderCopyEx(
        renderer, texture,
        nullptr,         // Source rectangle (nullptr for entire texture)
        &destRect,       // Destination rectangle
        angleDegrees,    // Angle in degrees
        &rotationOrigin, // Rotation origin relative to destRect top-left
        SDL_FLIP_NONE);  // No flipping needed for rotation

    // Reset texture mods if necessary
    SDL_SetTextureAlphaMod(texture, 255);

  } else {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "VisualEffect texture '%s' not found!", currentKey.c_str());
    // Optional: Draw a fallback placeholder rectangle
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); // Magenta fallback
    SDL_Rect fallbackRect = {
        static_cast<int>(std::round(visualX - renderWidth / 2.0f)) - cameraX,
        static_cast<int>(std::round(visualY - renderHeight / 2.0f)) - cameraY,
        renderWidth, renderHeight};
    SDL_RenderFillRect(renderer, &fallbackRect);
  }
}

// --- Implementation of updateAffectedTilesVisibility ---
void VisualEffect::updateAffectedTilesVisibility() {
  if (!gameDataRef)
    return; // Cannot update visibility without game data

  // Iterate through the stored affected tiles and their original visibility
  for (const auto &tile_pair : affectedTilesWithOriginalVisibility) {
    int tileX = tile_pair.first.first;
    int tileY = tile_pair.first.second;
    float originalVis = tile_pair.second;

    // Check if the tile is within level bounds before updating
    if (gameDataRef &&
        isWithinBounds(tileX, tileY, gameDataRef->currentLevel.width,
                       gameDataRef->currentLevel.height)) {
      // Update the visibility value in the game data's visibility map
      // We take the maximum of the original visibility and the effect's current
      // visibility. This ensures the tile is at least as visible as it was
      // before the effect, and potentially more visible if the effect's current
      // visibility is higher.
      gameDataRef->visibilityMap[tileY][tileX] =
          std::max(originalVis, currentVisibility);
    }
  }
}
// --- End of updateAffectedTilesVisibility Implementation ---
