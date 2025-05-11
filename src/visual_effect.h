// src/visual_effect.h
#ifndef VISUAL_EFFECT_H
#define VISUAL_EFFECT_H

#include <SDL.h>
#include <string>
#include <utility> // For std::pair
#include <vector>

// Include necessary headers for declarations used in the header
#include "asset_manager.h" // Include AssetManager declaration

// Forward Declarations (sufficient for pointer/reference members)
struct GameData; // Still need forward declaration as GameData is used by
                 // pointer
// isWithinBounds is a free function, its declaration is in utils.h,
// which will be included in the .cpp file where the constructor is defined.

struct VisualEffect {
  // --- Position & Size ---
  float visualX;    // Center X coordinate in world space
  float visualY;    // Center Y coordinate in world space
  int renderWidth;  // Width for rendering the effect
  int renderHeight; // Height for rendering the effect

  // --- Rotation ---
  double angleDegrees;      // Rotation angle in degrees
  SDL_Point rotationOrigin; // The point around which the texture is rotated,
                            // relative to its top-left corner

  // --- Animation State ---
  std::vector<std::string> frameTextureKeys; // Keys for animation frames
  float animationTimer = 0.0f;
  int currentFrame = 0;
  float animationSpeed = 10.0f; // Frames per second (adjust per effect)
  bool loop = false;            // Should the animation loop?
  float duration = 0.0f; // Total time the effect should last (in seconds). If
                         // 0, plays animation once.
  float timeElapsed = 0.0f; // Time since the effect started

  // --- Visibility Control ---
  GameData *gameDataRef =
      nullptr; // Pointer to game data for visibility map access
  // Store affected tiles and their *original* visibility values
  std::vector<std::pair<std::pair<int, int>, float>>
      affectedTilesWithOriginalVisibility;
  float visibilityFadeStartTimeRatio =
      0.5f; // Animation progress ratio where visibility fade starts
  float currentVisibility =
      0.0f; // Current visibility value applied by this effect

  // --- State ---
  bool markedForRemoval = false;
  bool visibilityReset = false; // Flag to ensure visibility is reset only once

  // --- Constructor for standard effects ---
  // Declaration only - implementation is in visual_effect.cpp
  VisualEffect(float x, float y, int w, int h,
               const std::vector<std::string> &frameKeys, float speed,
               float lifeDuration, bool loops = false);

  // --- Constructor for linear/rotated effects with visibility control ---
  // Declaration only - implementation is in visual_effect.cpp
  VisualEffect(float x, float y, int w, int h,
               const std::vector<std::string> &frameKeys, float speed,
               float lifeDuration, bool loops, double angle, SDL_Point origin,
               GameData *gameDataPtr,
               const std::vector<std::pair<int, int>> &tilesToAffect,
               float visibilityFadeStartRatio);

  // --- Methods ---
  // Declaration only - implementation is in visual_effect.cpp
  void update(float deltaTime);
  void render(SDL_Renderer *renderer, AssetManager &assets, int cameraX,
              int cameraY) const;

  // --- Helper to update visibility map ---
  // Declaration only - implementation is in visual_effect.cpp
  void updateAffectedTilesVisibility();
};

#endif // VISUAL_EFFECT_H
