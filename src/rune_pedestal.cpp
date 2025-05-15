// src/rune_pedestal.cpp
#include "asset_manager.h" // For AssetManager (used in render)
#include "character.h" // For PlayerCharacter definition (if game_data.h only forward declares)
#include "game_data.h" // For RunePedestal, PlayerCharacter, GameData, RuneType
#include "utils.h"     // For getRandomRune()
// game_data.h includes character.h, so this should be fine.
#include <SDL.h> // For SDL_Log, SDL_Renderer, SDL_Rect etc.

// Make sure getRandomRune is declared in utils.h and defined in utils.cpp
// RuneType getRandomRune(); // This would be in utils.h

void RunePedestal::update(float deltaTime) {
  if (isDeactivating) {
    if (deactivationFrameTextureNames.empty()) {
      isDeactivating =
          false; // No animation to play, instantly finish deactivating
      if (!isActive &&
          currentFrame ==
              0) { // If it was set to deactivating but has no frames
        currentFrame =
            0; // Or an "off" frame index if you had one not tied to anim lists
      }
      return;
    }

    animationTimer += deltaTime;
    float frameDuration = 1.0f / deactivationAnimationSpeed;
    if (animationTimer >= frameDuration) {
      animationTimer -= frameDuration;
      // Check if it was already on the last frame
      if (currentFrame >=
          static_cast<int>(deactivationFrameTextureNames.size()) - 1) {
        // Animation finished
        isDeactivating = false;
        // Keep currentFrame at the last frame index
        currentFrame = deactivationFrameTextureNames.size() - 1;
        SDL_Log(
            "RunePedestal at [%d,%d] fully deactivated (animation finished).",
            x, y);
      } else {
        // Advance to the next frame
        currentFrame++;
      }
    }
  } else if (isActive) {
    if (idleFrameTextureNames.empty())
      return; // No animation to play

    animationTimer += deltaTime;
    float frameDuration = 1.0f / idleAnimationSpeed;
    if (animationTimer >= frameDuration) {
      animationTimer -= frameDuration;
      currentFrame = (currentFrame + 1) % idleFrameTextureNames.size();
    }
  }
  // If !isActive and !isDeactivating, it's static (off), currentFrame should be
  // last deactivation frame.
}

void RunePedestal::render(SDL_Renderer *renderer, AssetManager &assetManager,
                          int cameraX, int cameraY,
                          const GameData &gameData) const {
  std::string textureKey;
  int frameToRender = currentFrame;

  if (isDeactivating) {
    if (!deactivationFrameTextureNames.empty()) {
      // Ensure frameToRender is within bounds for deactivation animation
      if (frameToRender < 0 ||
          frameToRender >=
              static_cast<int>(deactivationFrameTextureNames.size())) {
        frameToRender = deactivationFrameTextureNames.size() -
                        1; // Default to last frame if out of bounds
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "RunePedestal::render: Invalid frame %d during "
                    "deactivation. Using last frame.",
                    currentFrame);
      }
      textureKey = deactivationFrameTextureNames[frameToRender];
    } else {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "RunePedestal::render: isDeactivating is true, but no "
                  "deactivation frames.");
      // Optionally render a generic "off" texture or nothing
    }
  } else if (isActive) {
    if (!idleFrameTextureNames.empty()) {
      // Ensure frameToRender is within bounds for idle animation
      if (frameToRender < 0 ||
          frameToRender >= static_cast<int>(idleFrameTextureNames.size())) {
        frameToRender = 0; // Default to first frame if out of bounds
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "RunePedestal::render: Invalid frame %d during idle. Using "
                    "first frame.",
                    currentFrame);
      }
      textureKey = idleFrameTextureNames[frameToRender];
    } else {
      SDL_LogWarn(
          SDL_LOG_CATEGORY_APPLICATION,
          "RunePedestal::render: isActive is true, but no idle frames.");
    }
  } else { // Not active and not deactivating (fully off)
    if (!deactivationFrameTextureNames.empty()) {
      textureKey = deactivationFrameTextureNames
                       .back(); // Show last frame of deactivation
    } else if (!idleFrameTextureNames.empty()) {
      // Fallback if no deactivation frames: show first idle frame (or a
      // specific "off" frame if you add one) This state implies it was
      // deactivated without an animation.
      textureKey = idleFrameTextureNames.front();
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "RunePedestal::render: Deactivated, no deactivation frames. "
                  "Showing first idle frame as fallback.");
    } else {
      // No frames at all
      return;
    }
  }

  if (textureKey.empty()) {
    // SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "RunePedestal::render: No
    // textureKey determined for pedestal at [%d,%d].", x, y);
    return; // Nothing to render
  }

  // Check visibility using gameData
  float visibility = 0.0f;
  if (y >= 0 && y < static_cast<int>(gameData.visibilityMap.size()) && x >= 0 &&
      x < static_cast<int>(gameData.visibilityMap[y].size())) {
    visibility = gameData.visibilityMap[y][x];
  }

  if (visibility <= 0.0f) {
    return; // Not visible
  }

  SDL_Texture *tex = assetManager.getTexture(textureKey);
  if (!tex) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "RunePedestal::render: Texture not found for key '%s'",
                textureKey.c_str());
    return;
  }

  SDL_Rect destRect;
  destRect.w = gameData.tileWidth;
  destRect.h = gameData.tileHeight;
  destRect.x = x * destRect.w - cameraX;
  destRect.y = y * destRect.h - cameraY;

  Uint8 alpha = static_cast<Uint8>(visibility * 255);
  SDL_SetTextureAlphaMod(tex, alpha);
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

  SDL_RenderCopy(renderer, tex, nullptr, &destRect);

  SDL_SetTextureAlphaMod(tex, 255); // Reset alpha for texture's general use
  // SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE); // Only if you
  // changed renderer's blend mode
}

void RunePedestal::activateReward(PlayerCharacter &player) {
  if (!isActive) {
    SDL_Log("RunePedestal::activateReward: Pedestal at [%d,%d] is not active. "
            "Cannot activate.",
            x, y);
    return;
  }

  RuneType runeWon = getRandomRune();
  player.addRune(runeWon, 1);

  // It would be good to have a RuneTypeToString function for better logging.
  SDL_Log("Player activated RunePedestal at [%d,%d] and received Rune Type %d!",
          x, y, static_cast<int>(runeWon));

  isActive = false;
  isDeactivating = true;
  currentFrame = 0;
  animationTimer = 0.0f;

  // Consider adding a sound effect call here via an AssetManager instance if
  // available/passed assetManager.playSound("rune_obtained_sound");
}