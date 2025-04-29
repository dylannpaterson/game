#include "utils.h"
#include <SDL.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <iostream>
#include <cmath>     // For sqrt/pow or just abs for Manhattan
#include <limits>    // For std::numeric_limits

// --- ADDED Includes for findNearestValidTarget ---
#include "game_data.h" // Defines GameData, PlayerCharacter, Enemy, Level, etc.
#include "character.h" // Included via game_data.h, but good practice if directly using PlayerCharacter specifics
#include "enemy.h"     // Included via game_data.h, but good practice if directly using Enemy specifics
#include "spell.h"     // Defines Spell, SpellTargetType
// Note: level.h is likely included via game_data.h as well
// --- END Added Includes ---


// Function to render text to a texture
SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color) {
    if (!font) {
         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "renderText Error: Font is null.");
         return nullptr;
    }
    if (!renderer) {
         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "renderText Error: Renderer is null.");
         return nullptr;
    }
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (textSurface == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
        return nullptr;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == nullptr) {
         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
    }
    SDL_FreeSurface(textSurface); // Free surface regardless of texture creation success
    return textTexture;
}

// Function to check if coordinates are within map bounds
bool isWithinBounds(int x, int y, int width, int height) {
    return x >= 0 && x < width && y >= 0 && y < height;
}

// Function to initialize SDL and its subsystems
SDL_Context initializeSDL(int width, int height) {
    SDL_Context context;

    // Initialize SDL Video
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return context; // Return empty context on error
    }

    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return context; // Return empty context on error
    }

    // Initialize SDL_image for PNG loading
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_image could not initialize! IMG_Error: %s\n", IMG_GetError());
        TTF_Quit();
        SDL_Quit();
        return context; // Return empty context on error
    }

    // Create Window
    context.window = SDL_CreateWindow("Wizard Roguelike", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE); // Added Resizable flag
    if (context.window == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return context; // Return empty context on error
    }

    // Create Renderer
    context.renderer = SDL_CreateRenderer(context.window, -1, SDL_RENDERER_SOFTWARE);
    if (context.renderer == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(context.window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return context; // Return empty context on error
    }

    // Optional: Set logical size for resolution independence (adjust if needed)
    // SDL_RenderSetLogicalSize(context.renderer, width, height);

    SDL_Log("SDL Initialized Successfully.");
    return context;
}

// Function to handle SDL cleanup
void cleanupSDL(SDL_Context& context) {
    if (context.renderer != nullptr) {
        SDL_DestroyRenderer(context.renderer);
        context.renderer = nullptr; // Good practice to null pointers after destroying
    }
    if (context.window != nullptr) {
        SDL_DestroyWindow(context.window);
        context.window = nullptr; // Good practice
    }
    IMG_Quit(); // Quit SDL_image
    TTF_Quit(); // Quit SDL_ttf
    SDL_Quit(); // Quit SDL
    SDL_Log("SDL Cleaned Up.");
}


// --- Implementation for findNearestValidTarget ---
// Finds the nearest valid enemy target for a given spell within range and visibility.
bool findNearestValidTarget(const GameData& gameData, int spellIndex, SDL_Point& outTargetPos) {
    const PlayerCharacter& player = gameData.currentGamePlayer; // Get reference to player

    // Validate spell index
    if (spellIndex < 0 || spellIndex >= player.knownSpells.size()) {
         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "findNearestValidTarget: Invalid spell index %d", spellIndex);
        return false;
    }
    const Spell& spell = player.knownSpells[spellIndex]; // Get the spell details

    // Check if the spell targets enemies (required for this function)
    if (spell.targetType != SpellTargetType::Enemy) {
        SDL_Log("DEBUG: [NearestTarget] Spell '%s' cannot auto-target (not Enemy target type).", spell.name.c_str());
        return false;
    }

    // Variables to track the closest target found so far
    int closestDistSq = std::numeric_limits<int>::max(); // Use max integer value as initial distance
    const Enemy* nearestEnemy = nullptr; // Pointer to the nearest enemy found

    int playerTileX = player.targetTileX; // Player's logical X position
    int playerTileY = player.targetTileY; // Player's logical Y position

    // Iterate through all enemies in the current level
    for (const auto& enemy : gameData.enemies) {
        if (enemy.health <= 0) continue; // Skip dead enemies

        // 1. Check Visibility: Is the enemy currently visible to the player?
        float visibility = 0.0f;
         if (isWithinBounds(enemy.x, enemy.y, gameData.currentLevel.width, gameData.currentLevel.height) &&
             enemy.y < gameData.visibilityMap.size() && enemy.x < gameData.visibilityMap[enemy.y].size()) {
            visibility = gameData.visibilityMap[enemy.y][enemy.x]; // Get visibility value from the map
        }
        if (visibility <= 0.0f) continue; // Skip unseen enemies

        // 2. Check Range: Is the enemy within the spell's range?
        int dx = playerTileX - enemy.x; // Difference in X
        int dy = playerTileY - enemy.y; // Difference in Y
        int distSq = dx * dx + dy * dy; // Squared Euclidean distance (avoids sqrt)

        // Compare squared distance to squared range
        if (distSq <= spell.range * spell.range) {
             // Optional: Add Line of Sight Check here if needed
             // if (hasLineOfSight(playerTileX, playerTileY, enemy.x, enemy.y, gameData.currentLevel)) { ... }

            // 3. Check if this enemy is closer than the current nearest
            if (distSq < closestDistSq) {
                closestDistSq = distSq; // Update closest distance
                nearestEnemy = &enemy; // Update nearest enemy pointer
            }
             // } // End Optional LoS check
        }
    } // End loop through enemies

    // Check if a valid target was found
    if (nearestEnemy) {
        outTargetPos.x = nearestEnemy->x; // Set output X coordinate
        outTargetPos.y = nearestEnemy->y; // Set output Y coordinate
         SDL_Log("DEBUG: [NearestTarget] Found nearest enemy %d at [%d, %d]", nearestEnemy->id, outTargetPos.x, outTargetPos.y);
        return true; // Target found
    } else {
         SDL_Log("DEBUG: [NearestTarget] No valid enemy found in range/sight for spell '%s'.", spell.name.c_str());
        return false; // No target found
    }
    // --- FIXED: Added explicit return for the case where no target is found ---
    // return false; // This line is now technically redundant due to the else block above, but ensures clarity.
}
