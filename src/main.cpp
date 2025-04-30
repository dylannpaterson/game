// main.cpp
#include <algorithm> // For std::remove_if, std::max, std::min
#include <cmath>     // For std::abs
#include <cstdlib>   // For rand() and srand()
#include <ctime>     // For time()
#include <memory>    // For std::make_unique if needed (not currently used)
#include <string>    // For std::string
#include <vector>    // For std::vector

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

// Include project headers
#include "asset_manager.h"    // Include AssetManager header
#include "character.h"        // Includes PlayerCharacter definition
#include "character_select.h" // For character selection screen function
#include "enemy.h"            // Includes Enemy definition and planAction
#include "game_data.h" // Includes TurnPhase, IntendedAction, GameData struct etc.
#include "level.h"     // For Level struct and generateLevel function
#include "menu.h"      // For main menu function
#include "orbital_missile.h"
#include "projectile.h" // For Projectile struct
#include "ui.h"         // For rendering UI elements
#include "utils.h"      // Includes SDL_Context, helper functions
#include "visibility.h" // For updateVisibility function

#ifdef _WIN32
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "SDL2_ttf.lib")
#pragma comment(lib, "SDL2_image.lib")
#endif

// --- Forward Declarations ---
// Main loop functions
void handleEvents(GameData &gameData, AssetManager &assets, bool &running,
                  SDL_Context &context);
void updateLogic(GameData &gameData, AssetManager &assets, float deltaTime);
void renderScene(GameData &gameData, AssetManager &assets);
// Helper function for checking action resolution completion
bool isResolutionComplete(const GameData &gameData);

// --- Global Application State (Temporary) ---
// IMPORTANT: Replace this global with proper state management (pass AppState or
// put it in GameData)
enum class AppState { MainMenu, CharacterSelect, Gameplay, Quitting };
AppState currentAppState = AppState::MainMenu;
// --------------------------------------------

// --- Helper function to check if the Resolution phase is complete ---
bool isResolutionComplete(const GameData &gameData) {
  if (gameData.currentGamePlayer.isMoving)
    return false;
  for (const auto &enemy : gameData.enemies) {
    // Include check for enemy attack animation completion if necessary
    if (enemy.health > 0 && (enemy.isMoving || enemy.isAttacking))
      return false;
  }
  // Include check for pedestal deactivation animation completion
  if (gameData.currentPedestal.has_value() &&
      gameData.currentPedestal.value().isDeactivating) {
    return false;
  }
  // Projectile check remains the same
  for (const auto &proj : gameData.activeProjectiles) {
    if (proj.isActive)
      return false;
  }
  return true; // All animations/projectiles finished
}

// --- Main Function ---
int main(int argc, char *argv[]) {
  srand(static_cast<unsigned int>(time(0)));
  GameData gameData;
  SDL_Context sdlContext =
      initializeSDL(gameData.windowWidth, gameData.windowHeight);
  if (!sdlContext.window || !sdlContext.renderer) {
    cleanupSDL(sdlContext);
    return 1;
  }

  SDL_Renderer *renderer = sdlContext.renderer; // Or gameData.renderer
  int logicalWidth =
      gameData.windowWidth; // Your game's design resolution width
  int logicalHeight =
      gameData.windowHeight; // Your game's design resolution height
  if (SDL_RenderSetLogicalSize(renderer, logicalWidth, logicalHeight) != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to set logical size! SDL_Error: %s", SDL_GetError());
    // Handle error appropriately
  }

  // *** Set SDL logging level ***
  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE); // Show all messages
  // SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO); // Show info, warnings,
  // SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);    // Show warnings,
  // SDL_LogSetAllPriority(SDL_LOG_PRIORITY_ERROR); // Show only errors
  // To hide *all* standard logs (including errors), set priority higher than
  // critical: SDL_LogSetAllPriority(SDL_NUM_LOG_PRIORITIES); // Effectively
  // disable standard logging

  gameData.renderer = sdlContext.renderer;
  {
    AssetManager assetManager(
        gameData.renderer); // Create AssetManager instance

    // --- Player Initialization ---
    // Player initialized in GameData constructor

    // --- Load Assets ---
    bool loadSuccess = true;
    loadSuccess &=
        assetManager.loadTexture("splash", "../assets/splash/splash.png");
    loadSuccess &= assetManager.loadTexture("start_tile",
                                            "../assets/sprites/start_tile.png");
    loadSuccess &= assetManager.loadTexture("exit_tile",
                                            "../assets/sprites/exit_tile.png");
    loadSuccess &= assetManager.loadTexture(
        "reticle", "../assets/sprites/target_reticle.png");
    loadSuccess &=
        assetManager.loadTexture("fireball", "../assets/sprites/fireball.PNG");
    loadSuccess &= assetManager.loadTexture(
        "fireball_icon", "../assets/sprites/fireball_icon.PNG");
    loadSuccess &= assetManager.loadTexture("ward_icon",
                                            "../assets/sprites/ward_icon.PNG");
    loadSuccess &= assetManager.loadTexture("wall_texture",
                                            "../assets/sprites/wall_1.PNG");
    loadSuccess &=
        assetManager.loadTexture("floor_1", "../assets/sprites/floor_1.PNG");
    loadSuccess &=
        assetManager.loadTexture("floor_2", "../assets/sprites/floor_2.PNG");
    loadSuccess &=
        assetManager.loadFont("main_font", "../assets/fonts/LUMOS.TTF", 36);
    loadSuccess &=
        assetManager.loadFont("spellbar_font", "../assets/fonts/LUMOS.TTF", 18);
    loadSuccess &= assetManager.loadTexture(
        "female_mage_portrait", "../assets/sprites/female_mage_portrait.PNG");
    loadSuccess &= assetManager.loadTexture(
        "male_mage_portrait", "../assets/sprites/male_mage_portrait.PNG");
    loadSuccess &= assetManager.loadTexture("slime_texture",
                                            "../assets/sprites/slime.PNG");
    // Idle animation frames
    loadSuccess &= assetManager.loadTexture(
        "female_mage_idle_1", "../assets/sprites/animations/female_mage/idle/"
                              "female_mage_idle_0001.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_idle_2", "../assets/sprites/animations/female_mage/idle/"
                              "female_mage_idle_0002.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_idle_3", "../assets/sprites/animations/female_mage/idle/"
                              "female_mage_idle_0003.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_idle_4", "../assets/sprites/animations/female_mage/idle/"
                              "female_mage_idle_0004.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_idle_5", "../assets/sprites/animations/female_mage/idle/"
                              "female_mage_idle_0005.png");

    // --- ADDED: Load Walking Animation Frames ---
    loadSuccess &= assetManager.loadTexture(
        "female_mage_walk_1",
        "../assets/sprites/animations/female_mage/walk/" // Ensure path is
                                                         // correct
        "female_mage_walk_0001.png");                    // Example filename
    loadSuccess &= assetManager.loadTexture(
        "female_mage_walk_2", "../assets/sprites/animations/female_mage/walk/"
                              "female_mage_walk_0002.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_walk_3", "../assets/sprites/animations/female_mage/walk/"
                              "female_mage_walk_0003.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_walk_4", "../assets/sprites/animations/female_mage/walk/"
                              "female_mage_walk_0004.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_walk_5", "../assets/sprites/animations/female_mage/walk/"
                              "female_mage_walk_0005.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_walk_6", "../assets/sprites/animations/female_mage/walk/"
                              "female_mage_walk_0006.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_walk_7", "../assets/sprites/animations/female_mage/walk/"
                              "female_mage_walk_0007.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_walk_8", "../assets/sprites/animations/female_mage/walk/"
                              "female_mage_walk_0008.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_walk_9", "../assets/sprites/animations/female_mage/walk/"
                              "female_mage_walk_0009.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_walk_10", "../assets/sprites/animations/female_mage/walk/"
                               "female_mage_walk_0010.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_walk_11", "../assets/sprites/animations/female_mage/walk/"
                               "female_mage_walk_0011.png");

    // --- END Load Walking Animation Frames ---

    // --- ADDED: Load Targeting Animation Frames ---
    loadSuccess &= assetManager.loadTexture(
        "female_mage_target_1",
        "../assets/sprites/animations/female_mage/targetting/" // Ensure path is
                                                               // correct
        "female_mage_targetting_0001.png"); // Example filename
    loadSuccess &= assetManager.loadTexture(
        "female_mage_target_2",
        "../assets/sprites/animations/female_mage/targetting/"
        "female_mage_targetting_0002.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_target_3",
        "../assets/sprites/animations/female_mage/targetting/"
        "female_mage_targetting_0003.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_target_4",
        "../assets/sprites/animations/female_mage/targetting/"
        "female_mage_targetting_0004.png");
    loadSuccess &= assetManager.loadTexture(
        "female_mage_target_5",
        "../assets/sprites/animations/female_mage/targetting/"
        "female_mage_targetting_0005.png");
    // --- END Load Targeting Animation Frames ---

    // Load slime idle animation assests
    loadSuccess &= assetManager.loadTexture(
        "slime_idle_1", "../assets/sprites/animations/enemies/slime/idle/"
                        "slime_idle_0001.png"); // Example path/name
    loadSuccess &= assetManager.loadTexture(
        "slime_idle_2",
        "../assets/sprites/animations/enemies/slime/idle/slime_idle_0002.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_idle_3",
        "../assets/sprites/animations/enemies/slime/idle/slime_idle_0003.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_idle_4",
        "../assets/sprites/animations/enemies/slime/idle/slime_idle_0004.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_idle_5",
        "../assets/sprites/animations/enemies/slime/idle/slime_idle_0005.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_idle_6",
        "../assets/sprites/animations/enemies/slime/idle/slime_idle_0006.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_idle_7",
        "../assets/sprites/animations/enemies/slime/idle/slime_idle_0007.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_idle_8",
        "../assets/sprites/animations/enemies/slime/idle/slime_idle_0008.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_idle_9",
        "../assets/sprites/animations/enemies/slime/idle/slime_idle_0009.png");

    // Load slime walk animation assets
    loadSuccess &= assetManager.loadTexture(
        "slime_walk_1", "../assets/sprites/animations/enemies/slime/walk/"
                        "slime_walk_0001.png"); // Example path/name
    loadSuccess &= assetManager.loadTexture(
        "slime_walk_2",
        "../assets/sprites/animations/enemies/slime/walk/slime_walk_0002.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_walk_3",
        "../assets/sprites/animations/enemies/slime/walk/slime_walk_0003.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_walk_4",
        "../assets/sprites/animations/enemies/slime/walk/slime_walk_0004.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_walk_5",
        "../assets/sprites/animations/enemies/slime/walk/slime_walk_0005.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_walk_6",
        "../assets/sprites/animations/enemies/slime/walk/slime_walk_0006.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_walk_7",
        "../assets/sprites/animations/enemies/slime/walk/slime_walk_0007.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_walk_8",
        "../assets/sprites/animations/enemies/slime/walk/slime_walk_0008.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_walk_9",
        "../assets/sprites/animations/enemies/slime/walk/slime_walk_0009.png");

    // Load slime attack animation assets
    loadSuccess &= assetManager.loadTexture(
        "slime_attack_1", "../assets/sprites/animations/enemies/slime/attack/"
                          "slime_attack_0001.png"); // Example path/name
    loadSuccess &= assetManager.loadTexture(
        "slime_attack_2", "../assets/sprites/animations/enemies/slime/attack/"
                          "slime_attack_0002.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_attack_3", "../assets/sprites/animations/enemies/slime/attack/"
                          "slime_attack_0003.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_attack_4", "../assets/sprites/animations/enemies/slime/attack/"
                          "slime_attack_0004.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_attack_5", "../assets/sprites/animations/enemies/slime/attack/"
                          "slime_attack_0005.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_attack_6", "../assets/sprites/animations/enemies/slime/attack/"
                          "slime_attack_0006.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_attack_7", "../assets/sprites/animations/enemies/slime/attack/"
                          "slime_attack_0007.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_attack_8", "../assets/sprites/animations/enemies/slime/attack/"
                          "slime_attack_0008.png");
    loadSuccess &= assetManager.loadTexture(
        "slime_attack_9", "../assets/sprites/animations/enemies/slime/attack/"
                          "slime_attack_0009.png");

    loadSuccess &= assetManager.loadTexture(
        "magic_missiles_icon",
        "../assets/sprites/animations/spells/magic_missile/magic_missile_icon.png"); // Spell icon
    loadSuccess &= assetManager.loadTexture(
        "magic_missile_orbiting",
        "../assets/sprites/animations/spells/magic_missile/magic_missile_launched.png"); // Visual for waiting
    loadSuccess &= assetManager.loadTexture(
        "magic_missile_launched",
        "../assets/sprites/animations/spells/magic_missile/magic_missile_launched.png"); // Visual for

    // Load Crystal Textures ***
    //  Replace paths with your actual crystal image files
    loadSuccess &= assetManager.loadTexture(
        "health_crystal_texture",
        "../assets/sprites/health_crystal.png"); // Example path
    loadSuccess &= assetManager.loadTexture(
        "mana_crystal_texture",
        "../assets/sprites/mana_crystal.png"); // Example path

    // *** NEW: Load Rune Pedestal Animation Frames ***
    for (int i = 1; i <= 10; ++i) {
      std::string key = "rune_pedestal_" + std::to_string(i);
      // Adjust path and filename pattern as needed
      std::string path = "../assets/sprites/animations/environment/"
                         "rune_pedestal/rune_pedestal_" +
                         std::to_string(i) + ".png";
      loadSuccess &= assetManager.loadTexture(key, path);
      if (!loadSuccess) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Failed to load pedestal frame: %s", path.c_str());
        // Decide if you want to break or just log if a frame fails
      }
    }

    // *** NEW: Load Rune Pedestal Deactivation Animation Frames ***
    // Adjust the loop range (1 to 5 here) and filename pattern to match your
    // assets
    for (int i = 1; i <= 9; ++i) {
      std::string key = "rune_pedestal_off_" + std::to_string(i);
      std::string path =
          "../assets/sprites/animations/environment/"
          "rune_pedestal/rune_pedestal_deactivating_" + // Ensure correct
                                                        // path/prefix
          std::to_string(i) +
          ".png";
      loadSuccess &= assetManager.loadTexture(key, path);
      if (!loadSuccess) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Failed to load pedestal deactivation frame: %s",
                    path.c_str());
      }
    }
    // *** END Load Deactivation Frames ***

    // *** NEW: Load Ward Animation Frames ***
    for (int i = 1; i <= 9; ++i) {
      std::string key = "ward_active_" + std::to_string(i);
      // Adjust path and filename pattern as needed
      std::string path = "../assets/sprites/animations/spells/"
                         "ward/ward_active_000" +
                         std::to_string(i) + ".png";
      loadSuccess &= assetManager.loadTexture(key, path);
      if (!loadSuccess) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to load ward: %s",
                    path.c_str());
        // Decide if you want to break or just log if a frame fails
      }
    }

    if (!loadSuccess) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "Asset loading failed!"); /* Handle error */
    }
    SDL_Texture *reticleTex = assetManager.getTexture("reticle");
    if (reticleTex)
      SDL_SetTextureBlendMode(reticleTex, SDL_BLENDMODE_BLEND);
    SDL_Texture *splashTex = assetManager.getTexture("splash");
    if (splashTex)
      SDL_SetTextureBlendMode(splashTex, SDL_BLENDMODE_BLEND);

    bool running = true;
    Uint32 lastFrameTime = SDL_GetTicks();

    // --- FPS Counter Variables ---
    Uint32 fpsTimer = SDL_GetTicks(); // Initialize timer for FPS calculation
    Uint32 frameCount = 0;
    // ---

    // --- Main Application Loop ---
    while (currentAppState != AppState::Quitting) {
      Uint32 currentFrameTime = SDL_GetTicks();
      float deltaTime =
          static_cast<float>(currentFrameTime - lastFrameTime) / 1000.0f;
      if (deltaTime > 0.1f) // Cap delta time
        deltaTime = 0.1f;
      lastFrameTime = currentFrameTime;

      // --- FPS Calculation ---
      frameCount++;
      if (currentFrameTime - fpsTimer >= 1000) { // If 1 second has passed
        // Use SDL_Log instead of printf for consistency
        SDL_Log("INFO: FPS: %u", frameCount);
        frameCount = 0;              // Reset counter
        fpsTimer = currentFrameTime; // Reset timer
      }
      // ---

      handleEvents(gameData, assetManager, running, sdlContext);
      if (!running) {
        currentAppState = AppState::Quitting;
      }

      switch (currentAppState) {
      case AppState::MainMenu:
        if (gameData.isPanning) { /* ... menu panning ... */
          gameData.panCounter += 10;
          gameData.splashPanOffset -= 10;
          if (gameData.splashPanOffset <= 0) {
            gameData.splashPanOffset = 0;
            gameData.isPanning = false;
            currentAppState = AppState::CharacterSelect;
            gameData.isCharacterSelectFadingIn = true;
            gameData.characterSelectAlpha = 0;
            gameData.hasCharacterSelectStartedFading = true;
            SDL_Log("Panning finished, entering Character Select AppState.");
          }
        }
        break;
      case AppState::CharacterSelect:
        if (gameData.isCharacterSelectFadingIn) { /* ... fade-in ... */
          int newAlpha = static_cast<int>(gameData.characterSelectAlpha) + 20;
          if (newAlpha >= 255) {
            gameData.characterSelectAlpha = 255;
            gameData.isCharacterSelectFadingIn = false;
            SDL_Log("Character Select fade-in complete.");
          } else {
            gameData.characterSelectAlpha = static_cast<Uint8>(newAlpha);
          }
        }
        break;
      case AppState::Gameplay:
        updateLogic(gameData, assetManager, deltaTime);
        break;
      case AppState::Quitting:
        break;
      }

      SDL_SetRenderDrawColor(gameData.renderer, 0, 0, 0, 255);
      SDL_RenderClear(gameData.renderer);
      switch (currentAppState) {
      case AppState::MainMenu:
        displayMenu(gameData.renderer, assetManager.getFont("main_font"),
                    assetManager.getTexture("splash"), gameData.menuItems,
                    gameData.selectedIndex, gameData.isPanning,
                    gameData.splashPanOffset, 456, gameData.windowWidth,
                    gameData.windowHeight);
        break;
      case AppState::CharacterSelect:
        displayCharacterSelect(
            gameData.renderer, assetManager.getFont("main_font"),
            gameData.selectedCharacterIndex, gameData.windowWidth,
            gameData.windowHeight, gameData.characterSelectAlpha);
        break;
      case AppState::Gameplay:
        renderScene(gameData, assetManager);
        break; // Calls the full renderScene below
      case AppState::Quitting:
        break;
      }
      SDL_RenderPresent(gameData.renderer);
      SDL_Delay(1);
    } // End Main Application Loop
  }
  cleanupSDL(sdlContext);
  SDL_Log("Exiting gracefully. Farewell, Mortal.");
  return 0;
}

// --- Rewritten handleEvents Function ---
void handleEvents(GameData &gameData, AssetManager &assets, bool &running,
                  SDL_Context &context) {
  SDL_Event event;
  extern AppState currentAppState; // Using temporary global

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      running = false;
      return;
    } else if (event.type == SDL_WINDOWEVENT &&
               event.window.event == SDL_WINDOWEVENT_RESIZED) {
      gameData.windowWidth = event.window.data1;
      gameData.windowHeight = event.window.data2;
      SDL_Log("Window resized to %d x %d", gameData.windowWidth,
              gameData.windowHeight);
    } else if (event.type == SDL_MOUSEMOTION) {
      if (currentAppState == AppState::Gameplay &&
          gameData.showTargetingReticle) {
        // Translate screen coordinates to world coordinates
        int worldX = event.motion.x + gameData.cameraX;
        int worldY = event.motion.y + gameData.cameraY;

        // Translate world coordinates to tile coordinates
        int tileX = worldX / gameData.tileWidth;
        int tileY = worldY / gameData.tileHeight;

        // Clamp to level bounds
        tileX = std::max(0, std::min(gameData.currentLevel.width - 1, tileX));
        tileY = std::max(0, std::min(gameData.currentLevel.height - 1, tileY));

        // Update the reticle position if it's different from the current one
        if (tileX != gameData.targetIndicatorX ||
            tileY != gameData.targetIndicatorY) {
          gameData.targetIndicatorX = tileX;
          gameData.targetIndicatorY = tileY;
          SDL_Log("DEBUG: Reticle moved via mouse to [%d, %d]",
                  gameData.targetIndicatorX,
                  gameData.targetIndicatorY); // Optional log
        }
      }

    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
      if (currentAppState == AppState::Gameplay &&
          gameData.showTargetingReticle) {
        if (event.button.button ==
            SDL_BUTTON_LEFT) { // Left mouse button confirms
          SDL_Log("DEBUG: Left mouse click detected during targeting.");
          // --- Begin Confirmation Logic (Adapted from your hotkey logic) ---
          if (gameData.currentSpellIndex != -1) {
            const Spell &spell =
                gameData.currentGamePlayer.getSpell(gameData.currentSpellIndex);
            // Calculate distance using MANHATTAN distance for consistency with
            // range checks
            int effectiveRange =
                gameData.currentGamePlayer.GetEffectiveSpellRange(
                    gameData.currentSpellIndex);
            int distance = std::abs(gameData.currentGamePlayer.targetTileX -
                                    gameData.targetIndicatorX) +
                           std::abs(gameData.currentGamePlayer.targetTileY -
                                    gameData.targetIndicatorY);

            if (distance <= effectiveRange) { // Check if target is within range
              gameData.playerIntendedAction.type = ActionType::CastSpell;
              gameData.playerIntendedAction.spellIndex =
                  gameData.currentSpellIndex;
              gameData.playerIntendedAction.targetX =
                  gameData.targetIndicatorX; // Use reticle's current X
              gameData.playerIntendedAction.targetY =
                  gameData.targetIndicatorY; // Use reticle's current Y

              // --- CRITICAL: Advance Game State ---
              // This assumes 'actionPlanned' is used outside the keydown block
              // to advance the phase. We need similar logic here.
              // Find where 'actionPlanned = true;' is set after keyboard
              // targeting and replicate the phase transition logic. A
              // simplified approach for now:
              gameData.currentPhase = TurnPhase::Planning_EnemyAI;
              gameData.currentEnemyPlanningIndex = 0;
              gameData.enemyIntendedActions.clear();
              if (!gameData.enemies.empty()) {
                gameData.enemyIntendedActions.resize(gameData.enemies.size());
              }
              SDL_Log("Player plans TARGETED CAST spell %d at [%d,%d] "
                      "(Confirmed via Mouse Click). Advancing phase.",
                      gameData.currentSpellIndex, gameData.targetIndicatorX,
                      gameData.targetIndicatorY);
              // --- End Phase Advancement ---

              gameData.showTargetingReticle = false; // Exit targeting mode
              gameData.currentSpellIndex = -1; // Reset selected spell index
              // NOTE: If 'actionPlanned' is checked later, ensure it's set
              // here. actionPlanned = true; // Set this if your logic relies on
              // it after the event loop

            } else {
              SDL_Log(
                  "Target out of range (Dist: %d, Range: %d). Click ignored.",
                  distance, effectiveRange);
              // Maybe add a visual/audio cue for out-of-range click
            }
          } else {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "Target confirmed via mouse click with no spell selected!");
            gameData.showTargetingReticle = false; // Exit targeting mode anyway
          }
          // --- End Confirmation Logic ---

        } else if (event.button.button ==
                   SDL_BUTTON_RIGHT) { // Optional: Right mouse button cancels
          SDL_Log("DEBUG: Right mouse click detected during targeting. "
                  "Cancelling.");
          gameData.showTargetingReticle = false;
          gameData.currentSpellIndex = -1; // Reset state
        }
      }
      // Add else-if blocks here later to handle clicks when *not* targeting
      // (e.g., map movement, UI interaction)
    }

    switch (currentAppState) {
    case AppState::MainMenu:
      if (!gameData.isPanning) {
        if (event.type == SDL_KEYDOWN) {
          switch (event.key.keysym.sym) {
          case SDLK_UP:
            gameData.selectedIndex = (gameData.selectedIndex > 0)
                                         ? gameData.selectedIndex - 1
                                         : gameData.menuItems.size() - 1;
            break;
          case SDLK_DOWN:
            gameData.selectedIndex =
                (gameData.selectedIndex < (int)gameData.menuItems.size() - 1)
                    ? gameData.selectedIndex + 1
                    : 0;
            break;
          case SDLK_RETURN:
            if (gameData.selectedIndex == 0) {
              gameData.isPanning = true;
              gameData.panCounter = 0;
            } else if (gameData.selectedIndex == 2) {
              running = false;
            }
            break;
          case SDLK_ESCAPE:
            running = false;
            break;
          }
        }
      }
      break;

    case AppState::CharacterSelect:
      if (!gameData.isCharacterSelectFadingIn) {
        if (event.type == SDL_KEYDOWN) {
          switch (event.key.keysym.sym) {
          case SDLK_LEFT:
            gameData.selectedCharacterIndex =
                (gameData.selectedCharacterIndex > 0)
                    ? gameData.selectedCharacterIndex - 1
                    : 1;
            break;
          case SDLK_RIGHT:
            gameData.selectedCharacterIndex =
                (gameData.selectedCharacterIndex < 1)
                    ? gameData.selectedCharacterIndex + 1
                    : 0;
            break;
          case SDLK_RETURN: { // Character Chosen
            CharacterType chosenType = (gameData.selectedCharacterIndex == 0)
                                           ? CharacterType::FemaleMage
                                           : CharacterType::MaleMage;
            gameData.currentGamePlayer = PlayerCharacter(
                chosenType, 0, 0, gameData.tileWidth, gameData.tileHeight);
            gameData.enemies.clear();
            gameData.activeProjectiles.clear();
            gameData.currentLevelIndex = 1;
            Enemy::resetIdCounter(); // Use static method
            // *** MODIFIED CALL to generateLevel ***
            std::optional<SDL_Point>
                pedestalPosOpt; // Variable to receive position
            gameData.currentLevel =
                generateLevel(gameData.levelWidth, gameData.levelHeight,
                              gameData.levelMaxRooms, gameData.levelMinRoomSize,
                              gameData.levelMaxRoomSize, gameData.enemies,
                              gameData.tileWidth, gameData.tileHeight,
                              pedestalPosOpt); // Pass the optional Point

            // *** NEW: Create pedestal object if position was found ***
            if (pedestalPosOpt.has_value()) {
              gameData.currentPedestal.emplace(pedestalPosOpt.value().x,
                                               pedestalPosOpt.value().y);
            } else {
              gameData.currentPedestal
                  .reset(); // Ensure no pedestal if placement failed
            }
            // *** END NEW ***
            gameData.levelRooms = gameData.currentLevel.rooms;
            // --- NEW: Apply enemy scaling based on floor ---
            SDL_Log("Applying enemy scaling for floor %d...",
                    gameData.currentLevelIndex);
            for (auto &enemy : gameData.enemies) {
              enemy.applyFloorScaling(gameData.currentLevelIndex,
                                      gameData.enemyStatScalingPerFloor);
            }
            // --- END Apply enemy scaling ---
            // Init Occupation Grid
            gameData.occupationGrid.assign(
                gameData.currentLevel.height,
                std::vector<bool>(gameData.currentLevel.width, false));
            for (int y = 0; y < gameData.currentLevel.height; ++y)
              for (int x = 0; x < gameData.currentLevel.width; ++x)
                if (gameData.currentLevel.tiles[y][x] == '#')
                  gameData.occupationGrid[y][x] = true;
            // Place player & mark grid
            gameData.currentGamePlayer.targetTileX =
                gameData.currentLevel.startCol;
            gameData.currentGamePlayer.targetTileY =
                gameData.currentLevel.startRow;
            gameData.currentGamePlayer.x =
                gameData.currentGamePlayer.targetTileX * gameData.tileWidth +
                gameData.tileWidth / 2.0f;
            gameData.currentGamePlayer.y =
                gameData.currentGamePlayer.targetTileY * gameData.tileHeight +
                gameData.tileHeight / 2.0f;
            gameData.currentGamePlayer.startTileX =
                gameData.currentGamePlayer.targetTileX;
            gameData.currentGamePlayer.startTileY =
                gameData.currentGamePlayer.targetTileY;
            gameData.currentGamePlayer.isMoving = false;
            if (isWithinBounds(gameData.currentGamePlayer.targetTileX,
                               gameData.currentGamePlayer.targetTileY,
                               gameData.currentLevel.width,
                               gameData.currentLevel.height))
              gameData.occupationGrid[gameData.currentGamePlayer.targetTileY]
                                     [gameData.currentGamePlayer.targetTileX] =
                  true;
            // Mark initial enemy positions on grid
            for (const auto &enemy : gameData.enemies) {
              if (isWithinBounds(enemy.x, enemy.y, gameData.currentLevel.width,
                                 gameData.currentLevel.height)) {
                if (!gameData.occupationGrid[enemy.y][enemy.x]) {
                  gameData.occupationGrid[enemy.y][enemy.x] = true;
                } else {
                  SDL_LogWarn(
                      SDL_LOG_CATEGORY_APPLICATION,
                      "Enemy %d spawn location [%d,%d] was already occupied.",
                      enemy.id, enemy.x, enemy.y);
                }
              }
            }

            // --- Mark Pedestal Tile as Occupied (Correct Placement!) ---
            if (gameData.currentPedestal.has_value()) {
              int pedestalX = gameData.currentPedestal.value().x;
              int pedestalY = gameData.currentPedestal.value().y;
              if (isWithinBounds(pedestalX, pedestalY,
                                 gameData.currentLevel.width,
                                 gameData.currentLevel.height)) {
                if (gameData.occupationGrid[pedestalY][pedestalX]) {
                  SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                              "Pedestal location [%d,%d] was already marked "
                              "occupied (player/enemy spawn?)!",
                              pedestalX, pedestalY);
                }
                gameData.occupationGrid[pedestalY][pedestalX] =
                    true; // Mark the initialized grid
                SDL_Log("Marked pedestal location [%d,%d] as occupied on grid.",
                        pedestalX, pedestalY);
              } else { /* Log warning */
              }
            }
            // --- END Mark Pedestal Tile ---

            // Init Visibility
            gameData.visibilityMap.assign(
                gameData.currentLevel.height,
                std::vector<float>(gameData.currentLevel.width, 0.0f));
            updateVisibility(gameData.currentLevel, gameData.levelRooms,
                             gameData.currentGamePlayer.targetTileX,
                             gameData.currentGamePlayer.targetTileY,
                             gameData.hallwayVisibilityDistance,
                             gameData.visibilityMap);
            // Reset gameplay state
            gameData.playerIntendedAction = {};
            gameData.enemyIntendedActions.clear();
            gameData.currentPhase = TurnPhase::Planning_PlayerInput;
            gameData.currentMenu = GameMenu::None;
            gameData.showTargetingReticle = false;
            gameData.currentEnemyPlanningIndex = 0;
            gameData.cameraX = 0;
            gameData.cameraY = 0;
            updateLogic(gameData, assets, 0.0f); // Pass assets
            currentAppState = AppState::Gameplay;
            SDL_Log("Character selected, entering Gameplay AppState.");
            gameData.isCharacterSelectFadingIn = false;
            gameData.hasCharacterSelectStartedFading = false;
            gameData.characterSelectAlpha = 0;
          } break; // End Enter Key case
          case SDLK_ESCAPE:
            currentAppState = AppState::MainMenu;
            gameData.isCharacterSelectFadingIn = false;
            gameData.hasCharacterSelectStartedFading = false;
            gameData.characterSelectAlpha = 0;
            gameData.isPanning = false;
            gameData.splashPanOffset = 456;
            gameData.selectedIndex = 0;
            break;
          }
        }
      }
      break; // End CharacterSelect input

    case AppState::Gameplay:
      /*SDL_Log("DEBUG: handleEvents - AppState=Gameplay, CurrentPhase=%d",
              (int)gameData.currentPhase);*/
      if (gameData.currentPhase == TurnPhase::Planning_PlayerInput) {
        /*SDL_Log("DEBUG: handleEvents - Phase=Planning_PlayerInput, Player "
                "isMoving=%s, CurrentMenu=%d",
                gameData.currentGamePlayer.isMoving ? "true" : "false",
                (int)gameData.currentMenu);*/
        if (!gameData.currentGamePlayer.isMoving) {
          if (gameData.currentMenu == GameMenu::SpellMenu) {
            // --- Spell Menu Input ---
            if (event.type == SDL_KEYDOWN) {
              switch (event.key.keysym.sym) {
              case SDLK_w:
                if (!gameData.currentGamePlayer.knownSpells.empty()) {
                  gameData.spellSelectIndex =
                      (gameData.spellSelectIndex > 0)
                          ? gameData.spellSelectIndex - 1
                          : gameData.currentGamePlayer.knownSpells.size() - 1;
                }
                break;
              case SDLK_s:
                if (!gameData.currentGamePlayer.knownSpells.empty()) {
                  gameData.spellSelectIndex =
                      (gameData.spellSelectIndex <
                       (int)gameData.currentGamePlayer.knownSpells.size() - 1)
                          ? gameData.spellSelectIndex + 1
                          : 0;
                }
                break;
              case SDLK_RETURN: {
                if (!gameData.currentGamePlayer.knownSpells.empty() &&
                    gameData.spellSelectIndex >= 0 &&
                    gameData.spellSelectIndex <
                        gameData.currentGamePlayer.knownSpells.size()) {
                  gameData.currentSpellIndex = gameData.spellSelectIndex;
                  const Spell &spellToCast =
                      gameData.currentGamePlayer.getSpell(
                          gameData.currentSpellIndex);
                  if (gameData.currentGamePlayer.canCastSpell(
                          gameData.currentSpellIndex)) {
                    if (spellToCast.targetType == SpellTargetType::Self) {
                      gameData.playerIntendedAction.type =
                          ActionType::CastSpell;
                      gameData.playerIntendedAction.spellIndex =
                          gameData.currentSpellIndex;
                      gameData.currentMenu = GameMenu::None;
                      gameData.currentPhase = TurnPhase::Planning_EnemyAI;
                      gameData.currentEnemyPlanningIndex = 0;
                      gameData.enemyIntendedActions.clear();
                      gameData.enemyIntendedActions.resize(
                          gameData.enemies.size());
                      SDL_Log("Player plans SELF CAST spell %d. Advancing to "
                              "Enemy Planning.",
                              gameData.currentSpellIndex);
                    } else {
                      gameData.targetIndicatorX =
                          gameData.currentGamePlayer.targetTileX;
                      gameData.targetIndicatorY =
                          gameData.currentGamePlayer.targetTileY;
                      gameData.showTargetingReticle = true;
                      gameData.currentMenu = GameMenu::None;
                      SDL_Log("Player enters TARGETING for spell %d.",
                              gameData.currentSpellIndex);
                    }
                  } else {
                    SDL_Log("Cannot cast spell %d (Not enough mana?)",
                            gameData.currentSpellIndex);
                  }
                } else {
                  SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                              "Invalid spell selection index: %d",
                              gameData.spellSelectIndex);
                  gameData.currentMenu = GameMenu::None;
                }
              } break;
              case SDLK_ESCAPE:
              case SDLK_c:
                gameData.currentMenu = GameMenu::None;
                gameData.spellSelectIndex = 0;
                gameData.currentSpellIndex = -1;
                gameData.showTargetingReticle = false;
                break;
              }
            }
          } else if (gameData.currentMenu == GameMenu::CharacterSheet) {
            // --- Character Sheet Input ---
            if (event.type == SDL_KEYDOWN) {
              switch (event.key.keysym.sym) {
              case SDLK_i:
              case SDLK_ESCAPE:
                gameData.currentMenu = GameMenu::None;
                break;
              }
            }
          } else {
            // --- No Menu Active: Handle Core Gameplay Actions ---
            if (event.type == SDL_KEYDOWN) {
              SDL_Log("DEBUG: Keydown detected in Planning_PlayerInput (No "
                      "Menu). Key: %s",
                      SDL_GetKeyName(event.key.keysym.sym));
              bool actionPlanned = false;
              SDL_Keycode keycode = event.key.keysym.sym;

              // --- Targeting Input ---
              if (gameData.showTargetingReticle) {
                int targetMoveX = 0;
                int targetMoveY = 0;
                bool confirmTarget = false;
                bool cancelTarget = false;
                int confirmingSpellIndex =
                    -1; // Check if pressed key matches current spell hotkey

                // Map pressed key to potential spell index for confirmation
                if (keycode == SDLK_1)
                  confirmingSpellIndex = 0;
                else if (keycode == SDLK_2)
                  confirmingSpellIndex = 1;
                else if (keycode == SDLK_3)
                  confirmingSpellIndex = 2;
                else if (keycode == SDLK_4)
                  confirmingSpellIndex = 3;
                else if (keycode == SDLK_5)
                  confirmingSpellIndex = 4;

                // Check for Confirmation via Hotkey
                if (confirmingSpellIndex != -1 &&
                    confirmingSpellIndex == gameData.currentSpellIndex) {
                  confirmTarget = true;
                  SDL_Log("DEBUG: Confirming target via Hotkey %s.",
                          SDL_GetKeyName(keycode));
                }
                // Check for Cancellation
                else if (keycode == SDLK_ESCAPE) {
                  cancelTarget = true;
                }
                // Check for Reticle Movement (if not confirming or cancelling)
                else if (confirmingSpellIndex ==
                         -1) { // Only move if it wasn't a confirmation/cancel
                               // key
                  switch (keycode) {
                  case SDLK_UP:
                  case SDLK_w:
                  case SDLK_KP_8:
                    targetMoveY = -1;
                    break;
                  case SDLK_DOWN:
                  case SDLK_s:
                  case SDLK_KP_2:
                    targetMoveY = 1;
                    break;
                  case SDLK_LEFT:
                  case SDLK_a:
                  case SDLK_KP_4:
                    targetMoveX = -1;
                    break;
                  case SDLK_RIGHT:
                  case SDLK_d:
                  case SDLK_KP_6:
                    targetMoveX = 1;
                    break;
                  case SDLK_KP_7:
                    targetMoveX = -1;
                    targetMoveY = -1;
                    break;
                  case SDLK_KP_9:
                    targetMoveX = 1;
                    targetMoveY = -1;
                    break;
                  case SDLK_KP_1:
                    targetMoveX = -1;
                    targetMoveY = 1;
                    break;
                  case SDLK_KP_3:
                    targetMoveX = 1;
                    targetMoveY = 1;
                    break;
                    // Removed SDLK_RETURN confirmation here
                  }
                }

                // Apply Reticle Movement
                if (targetMoveX != 0 || targetMoveY != 0) {
                  gameData.targetIndicatorX += targetMoveX;
                  gameData.targetIndicatorY += targetMoveY;
                  // Clamp indicator within bounds
                  gameData.targetIndicatorX =
                      std::max(0, std::min(gameData.currentLevel.width - 1,
                                           gameData.targetIndicatorX));
                  gameData.targetIndicatorY =
                      std::max(0, std::min(gameData.currentLevel.height - 1,
                                           gameData.targetIndicatorY));
                  SDL_Log("DEBUG: Reticle moved to [%d, %d]",
                          gameData.targetIndicatorX, gameData.targetIndicatorY);
                }
                // Apply Confirmation (Now triggered by matching hotkey)
                if (confirmTarget) {
                  if (gameData.currentSpellIndex != -1) {
                    const Spell &spell = gameData.currentGamePlayer.getSpell(
                        gameData.currentSpellIndex);
                    int effectiveRange =
                        gameData.currentGamePlayer.GetEffectiveSpellRange(
                            gameData.currentSpellIndex);
                    int distance =
                        std::abs(gameData.currentGamePlayer.targetTileX -
                                 gameData.targetIndicatorX) +
                        std::abs(gameData.currentGamePlayer.targetTileY -
                                 gameData.targetIndicatorY);
                    if (distance <= effectiveRange) {
                      gameData.playerIntendedAction.type =
                          ActionType::CastSpell;
                      gameData.playerIntendedAction.spellIndex =
                          gameData.currentSpellIndex;
                      gameData.playerIntendedAction.targetX =
                          gameData.targetIndicatorX;
                      gameData.playerIntendedAction.targetY =
                          gameData.targetIndicatorY;
                      actionPlanned = true; // Mark action planned
                      SDL_Log("Player plans TARGETED CAST spell %d at [%d,%d] "
                              "(Confirmed via Hotkey).",
                              gameData.currentSpellIndex,
                              gameData.targetIndicatorX,
                              gameData.targetIndicatorY);
                    } else {
                      SDL_Log("Target out of range.");
                    }
                  } else {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                 "Target confirmed with no spell selected!");
                  }
                  gameData.showTargetingReticle = false;
                  gameData.currentSpellIndex = -1; // Reset state
                }
                // Apply Cancellation
                if (cancelTarget) {
                  SDL_Log("Targeting cancelled.");
                  gameData.showTargetingReticle = false;
                  gameData.currentSpellIndex = -1; // Reset state
                }
              } else {
                int moveX = 0;
                int moveY = 0;
                switch (event.key.keysym.sym) {
                case SDLK_UP:
                case SDLK_w:
                case SDLK_KP_8:
                  SDL_Log("DEBUG: UP key processed.");
                  moveY = -1;
                  break;
                case SDLK_DOWN:
                case SDLK_s:
                case SDLK_KP_2:
                  SDL_Log("DEBUG: DOWN key processed.");
                  moveY = 1;
                  break;
                case SDLK_LEFT:
                case SDLK_a:
                case SDLK_KP_4:
                  SDL_Log("DEBUG: LEFT key processed.");
                  moveX = -1;
                  gameData.currentGamePlayer.currentFacingDirection =
                      PlayerCharacter::FacingDirection::Left;
                  break;
                case SDLK_RIGHT:
                case SDLK_d:
                case SDLK_KP_6:
                  SDL_Log("DEBUG: RIGHT key processed.");
                  moveX = 1;
                  gameData.currentGamePlayer.currentFacingDirection =
                      PlayerCharacter::FacingDirection::Right;
                  break;
                case SDLK_KP_7:
                  SDL_Log("DEBUG: UP-LEFT key processed.");
                  moveX = -1;
                  moveY = -1;
                  break;
                case SDLK_KP_9:
                  SDL_Log("DEBUG: UP-RIGHT key processed.");
                  moveX = 1;
                  moveY = -1;
                  break;
                case SDLK_KP_1:
                  SDL_Log("DEBUG: DOWN-LEFT key processed.");
                  moveX = -1;
                  moveY = 1;
                  break;
                case SDLK_KP_3:
                  SDL_Log("DEBUG: DOWN-RIGHT key processed.");
                  moveX = 1;
                  moveY = 1;
                  break;
                case SDLK_KP_5:
                  SDL_Log("DEBUG: WAIT key processed.");
                  gameData.playerIntendedAction.type = ActionType::Wait;
                  actionPlanned = true;
                  SDL_Log("Player plans WAIT.");
                  break;
                case SDLK_e: // Use 'E' for interaction (or choose another key)
                  SDL_Log("DEBUG: Interact key 'E' processed.");
                  if (gameData.currentPedestal.has_value() &&
                      gameData.currentPedestal.value().isActive) {
                    const RunePedestal &pedestal =
                        gameData.currentPedestal.value();
                    int playerX = gameData.currentGamePlayer.targetTileX;
                    int playerY = gameData.currentGamePlayer.targetTileY;

                    // Check adjacency (Manhattan distance <= 1)
                    int distance = std::abs(playerX - pedestal.x) +
                                   std::abs(playerY - pedestal.y);

                    if (distance <= 1) {
                      SDL_Log("Player plans INTERACT with pedestal at [%d,%d].",
                              pedestal.x, pedestal.y);
                      gameData.playerIntendedAction.type = ActionType::Interact;
                      // Store target coords in case needed later, though
                      // adjacency check might be enough
                      gameData.playerIntendedAction.targetX = pedestal.x;
                      gameData.playerIntendedAction.targetY = pedestal.y;
                      actionPlanned = true; // Mark action as planned
                    } else {
                      SDL_Log("Player attempted interaction, but pedestal at "
                              "[%d,%d] is not adjacent (Dist: %d).",
                              pedestal.x, pedestal.y, distance);
                    }
                  } else {
                    SDL_Log("Player attempted interaction, but no active "
                            "pedestal found.");
                  }
                  break; // End Interaction case
                case SDLK_c:
                  SDL_Log("DEBUG: SPELL MENU key processed.");
                  if (event.key.repeat == 0 &&
                      !gameData.currentGamePlayer.knownSpells.empty()) {
                    gameData.currentMenu = GameMenu::SpellMenu;
                    gameData.spellSelectIndex = 0;
                  } else if (gameData.currentGamePlayer.knownSpells.empty()) {
                    SDL_Log("Player knows no spells!");
                  }
                  break;
                case SDLK_i:
                  SDL_Log("DEBUG: CHAR SHEET key processed.");
                  if (event.key.repeat == 0)
                    gameData.currentMenu = GameMenu::CharacterSheet;
                  break;

                // --- Hotkey Logic with Enhanced Logging ---
                case SDLK_1:
                case SDLK_2:
                case SDLK_3:
                case SDLK_4:
                case SDLK_5: {
                  SDL_Log("DEBUG: Hotkey %s processed.",
                          SDL_GetKeyName(event.key.keysym.sym));
                  int spellIndex = -1;
                  if (event.key.keysym.sym == SDLK_1)
                    spellIndex = 0;
                  else if (event.key.keysym.sym == SDLK_2)
                    spellIndex = 1;
                  else if (event.key.keysym.sym == SDLK_3)
                    spellIndex = 2;
                  else if (event.key.keysym.sym == SDLK_4)
                    spellIndex = 3;
                  else if (event.key.keysym.sym == SDLK_5)
                    spellIndex = 4;

                  SDL_Log("DEBUG: Hotkey determined spellIndex = %d",
                          spellIndex);

                  // Check 1: Is the index valid for the known spells?
                  if (spellIndex != -1 &&
                      spellIndex <
                          gameData.currentGamePlayer.knownSpells.size()) {
                    SDL_Log("DEBUG: Hotkey spell index %d is valid (Known "
                            "Spells: %zu).",
                            spellIndex,
                            gameData.currentGamePlayer.knownSpells.size());
                    // Check 2: Can the player afford the mana?
                    if (gameData.currentGamePlayer.canCastSpell(spellIndex)) {
                      SDL_Log("DEBUG: Player CAN cast spell %d.", spellIndex);
                      const Spell &spellToCast =
                          gameData.currentGamePlayer.getSpell(spellIndex);
                      // Check 3: Is it self-cast or targeted?
                      if (spellToCast.targetType == SpellTargetType::Self) {
                        SDL_Log("DEBUG: Hotkey spell %d is Self-Targeted. "
                                "Planning action.",
                                spellIndex);
                        gameData.playerIntendedAction.type =
                            ActionType::CastSpell;
                        gameData.playerIntendedAction.spellIndex = spellIndex;
                        actionPlanned =
                            true; // <<<< Mark action as planned HERE
                        SDL_Log("Player plans HOTKEY SELF CAST spell %d.",
                                spellIndex);
                      } else { // Enter Targeting Mode
                        SDL_Log("DEBUG: Hotkey spell %d needs targeting. "
                                "Finding nearest...",
                                spellIndex);
                        gameData.currentSpellIndex =
                            spellIndex; // Store the spell index
                        SDL_Point nearestTargetPos = {-1, -1};
                        bool targetFound = findNearestValidTarget(
                            gameData, spellIndex, nearestTargetPos);

                        if (targetFound) {
                          SDL_Log("DEBUG: Nearest target found at [%d, %d]. "
                                  "Setting reticle.",
                                  nearestTargetPos.x, nearestTargetPos.y);
                          gameData.targetIndicatorX = nearestTargetPos.x;
                          gameData.targetIndicatorY = nearestTargetPos.y;
                        } else {
                          SDL_Log("DEBUG: No nearest target found. Reticle "
                                  "starts at player [%d, %d].",
                                  gameData.currentGamePlayer.targetTileX,
                                  gameData.currentGamePlayer.targetTileY);
                          gameData.targetIndicatorX =
                              gameData.currentGamePlayer.targetTileX;
                          gameData.targetIndicatorY =
                              gameData.currentGamePlayer.targetTileY;
                        }
                        gameData.showTargetingReticle =
                            true; // Enter targeting mode
                        SDL_Log(
                            "Player enters TARGETING via hotkey for spell %d.",
                            spellIndex);
                        // DO NOT set actionPlanned = true here.
                      }
                    } else {
                      SDL_Log("DEBUG: Player CANNOT cast spell %d (Not enough "
                              "mana?).",
                              spellIndex);
                    }
                  } else {
                    SDL_Log("DEBUG: Hotkey spell index %d is INVALID (Index: "
                            "%d, Known: %zu).",
                            spellIndex, spellIndex,
                            gameData.currentGamePlayer.knownSpells.size());
                  }
                } break; // End hotkey block
                         // --- End Hotkey Logic ---

                } // End normal action switch

                SDL_Log("DEBUG: moveX=%d, moveY=%d, actionPlanned (before move "
                        "check)=%s",
                        moveX, moveY, actionPlanned ? "true" : "false");

                // --- Process Planned Movement ---
                if (!actionPlanned && (moveX != 0 || moveY != 0)) {
                  int newPlayerTargetX =
                      gameData.currentGamePlayer.targetTileX + moveX;
                  int newPlayerTargetY =
                      gameData.currentGamePlayer.targetTileY + moveY;

                  if (isWithinBounds(newPlayerTargetX, newPlayerTargetY,
                                     gameData.currentLevel.width,
                                     gameData.currentLevel.height)) {
                    // 2. Check if the target tile is a wall
                    if (gameData.currentLevel
                            .tiles[newPlayerTargetY][newPlayerTargetX] != '#') {
                      // 3. Check if the target tile is ALREADY OCCUPIED on the
                      // grid
                      //    (by wall duplicate check, enemy, pedestal, etc.)
                      if (!gameData.occupationGrid[newPlayerTargetY]
                                                  [newPlayerTargetX]) {
                        // --- TARGET TILE IS CLEAR ---
                        SDL_Log("Player intends move to [%d,%d]. Starting "
                                "animation immediately.",
                                newPlayerTargetX, newPlayerTargetY);

                        // 1. Store old logical position
                        int oldPlayerTileX =
                            gameData.currentGamePlayer.targetTileX;
                        int oldPlayerTileY =
                            gameData.currentGamePlayer.targetTileY;

                        // 2. Start the move animation
                        gameData.currentGamePlayer.startMove(newPlayerTargetX,
                                                             newPlayerTargetY);

                        // 3. Update occupation grid immediately
                        if (isWithinBounds(oldPlayerTileX, oldPlayerTileY,
                                           gameData.currentLevel.width,
                                           gameData.currentLevel.height)) {
                          gameData
                              .occupationGrid[oldPlayerTileY][oldPlayerTileX] =
                              false;
                          SDL_Log(
                              "Cleared player occupation at old pos [%d,%d]",
                              oldPlayerTileX, oldPlayerTileY);
                        }
                        if (isWithinBounds(newPlayerTargetX, newPlayerTargetY,
                                           gameData.currentLevel.width,
                                           gameData.currentLevel.height)) {
                          gameData.occupationGrid[newPlayerTargetY]
                                                 [newPlayerTargetX] = true;
                          SDL_Log("Set player occupation at new pos [%d,%d]",
                                  newPlayerTargetX, newPlayerTargetY);
                        } else {
                          // Log error if somehow the new target became out of
                          // bounds after the initial check
                          SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                       "New player target [%d,%d] out of "
                                       "bounds during grid marking!",
                                       newPlayerTargetX, newPlayerTargetY);
                        }

                        // 4. Set intended action and flag
                        gameData.playerIntendedAction.type = ActionType::Move;
                        gameData.playerIntendedAction.targetX =
                            newPlayerTargetX;
                        gameData.playerIntendedAction.targetY =
                            newPlayerTargetY;
                        actionPlanned = true;
                        SDL_Log("DEBUG: Move Action Planned & Animation "
                                "Started. actionPlanned is now true.");
                        // --- END TARGET TILE IS CLEAR ---

                      } else {
                        // Target tile is occupied (wall duplicate check, enemy,
                        // pedestal, etc.)
                        SDL_Log("Player move to [%d,%d] blocked by occupation "
                                "grid.",
                                newPlayerTargetX, newPlayerTargetY);
                      }
                    } else {
                      // Target tile is a wall '#'
                      SDL_Log("Player move to [%d,%d] blocked by wall.",
                              newPlayerTargetX, newPlayerTargetY);
                    }
                  } else {
                    // Target tile is out of bounds
                    SDL_Log("Player move to [%d,%d] blocked by OOB.",
                            newPlayerTargetX, newPlayerTargetY);
                  }
                } // End movement processing
              } // End Normal Action Input block

              SDL_Log("DEBUG: Checking actionPlanned flag: %s",
                      actionPlanned ? "true" : "false");
              if (actionPlanned) {
                SDL_Log(
                    "DEBUG: [HandleEvents] Action planned. Advancing Phase.");
                gameData.currentPhase = TurnPhase::Planning_EnemyAI;
                gameData.currentEnemyPlanningIndex = 0;
                gameData.enemyIntendedActions.clear();
                if (!gameData.enemies.empty()) {
                  gameData.enemyIntendedActions.resize(gameData.enemies.size());
                }
                SDL_Log("--- Player Planning Complete. Transitioning to Enemy "
                        "Planning ---");
                // Reset targeting state just in case
                gameData.showTargetingReticle = false;
                gameData.currentSpellIndex = -1;
                // Break out of the inner keydown processing? Not strictly
                // necessary but prevents processing other keys in the same
                // frame after action decided. return; // Optionally exit
                // handleEvents immediately after planning action
              }
            } // End keydown check
          } // End No Menu Active block
        } // End !isMoving check
      } // End Planning_PlayerInput phase check
      break; // End Gameplay input

    case AppState::Quitting:
      break; // Ignore input

    } // End AppState switch
  } // End SDL_PollEvent loop
}

// --- Rewritten updateLogic Function ---
void updateLogic(GameData &gameData, AssetManager &assets, float deltaTime) {

  // SDL_Log("DEBUG: [UpdateLogic Start] Current Phase: %d",
  //         (int)gameData.currentPhase);

  // --- Update Pedestal Animation *** ---
  if (gameData.currentPedestal.has_value()) {
    RunePedestal &pedestal = gameData.currentPedestal.value(); // Get reference

    if (pedestal.isDeactivating) {
      // --- Handle Deactivation Animation ---
      if (!pedestal.deactivationFrameTextureNames.empty()) {
        pedestal.animationTimer += deltaTime;
        float frameDuration = 1.0f / pedestal.deactivationAnimationSpeed;
        if (pedestal.animationTimer >= frameDuration) {
          pedestal.animationTimer -= frameDuration;
          // Check if it was already on the last frame
          if (pedestal.currentFrame >=
              pedestal.deactivationFrameTextureNames.size() - 1) {
            // Animation finished
            pedestal.isDeactivating = false;
            // Keep currentFrame at the last frame index
            pedestal.currentFrame =
                pedestal.deactivationFrameTextureNames.size() - 1;
            SDL_Log("Pedestal deactivation animation finished.");
          } else {
            // Advance to the next frame
            pedestal.currentFrame++;
          }
        }
      } else {
        // No deactivation frames defined, instantly deactivate
        pedestal.isDeactivating = false;
        pedestal.currentFrame =
            0; // Or set to a specific "off" frame index if you have one
      }
    } else if (pedestal.isActive) {
      // --- Handle Idle Animation (Only if active and not deactivating) ---
      if (!pedestal.idleFrameTextureNames.empty()) {
        pedestal.animationTimer += deltaTime;
        float frameDuration = 1.0f / pedestal.idleAnimationSpeed;
        if (pedestal.animationTimer >= frameDuration) {
          pedestal.animationTimer -= frameDuration;
          pedestal.currentFrame = (pedestal.currentFrame + 1) %
                                  pedestal.idleFrameTextureNames.size();
        }
      }
    }
    // --- *** END Update Pedestal Animation *** ---

    // update player
    gameData.currentGamePlayer.update(deltaTime, gameData);

    for (auto &enemy : gameData.enemies) {
      if (enemy.health > 0) { // Only update living enemies
        enemy.update(deltaTime, gameData);
      }
    }

    // --- Update Camera ---
    if (gameData.currentLevel.width > 0 && gameData.currentLevel.height > 0 &&
        gameData.tileWidth > 0 && gameData.tileHeight > 0 &&
        gameData.logicalWidth > 0 &&
        gameData.logicalHeight > 0) { // Added checks for logical dims

      // Center point based on LOGICAL viewport dimensions
      int logicalHalfWidth = gameData.logicalWidth / 2;
      int logicalHalfHeight = gameData.logicalHeight / 2;
      int idealCameraX =
          static_cast<int>(gameData.currentGamePlayer.x) - logicalHalfWidth;
      int idealCameraY =
          static_cast<int>(gameData.currentGamePlayer.y) - logicalHalfHeight;

      // Max limits depend on tile size AND LOGICAL viewport size
      int maxCameraX = (gameData.currentLevel.width * gameData.tileWidth) -
                       gameData.logicalWidth;
      int maxCameraY = (gameData.currentLevel.height * gameData.tileHeight) -
                       gameData.logicalHeight;

      // Clamp camera position based on calculated max limits
      gameData.cameraX = std::max(0, std::min(idealCameraX, maxCameraX));
      gameData.cameraY = std::max(0, std::min(idealCameraY, maxCameraY));

      // Prevent negative camera position if logical size > level size in pixels
      if (maxCameraX < 0)
        gameData.cameraX = 0;
      if (maxCameraY < 0)
        gameData.cameraY = 0;
    } else {
      // Set camera to 0,0 if level/tile/logical dimensions are invalid
      gameData.cameraX = 0;
      gameData.cameraY = 0;
    } //

    // --- Timing variables ---
    static Uint32 planningWallClockStartTime =
        0; // For overall phase wall-clock time
    static Uint32 totalEnemyPlanningCpuTime =
        0; // Accumulator for CPU time this turn
    static Uint32 resolutionStartTime = 0;
    Uint32 phaseEndTime = 0;
    Uint32 elapsedMs = 0;

    // --- Turn Phase State Machine ---

    // SDL_Log("DEBUG: [UpdateLogic] Entering phase switch. Current Phase: %d",
    //         (int)gameData.currentPhase);
    TurnPhase previousPhase =
        gameData.currentPhase; // Track phase changes for logging

    switch (gameData.currentPhase) {
    case TurnPhase::Planning_PlayerInput: {

      // *** MOVED LEVEL TRANSITION CHECK HERE ***
      // Check at the START of the player's turn if they are on the exit tile
      PlayerCharacter &player = gameData.currentGamePlayer; // Alias for clarity
      // Ensure the player is NOT currently moving (should be true in this
      // phase, but good check)
      if (!player.isMoving &&
          player.targetTileX == gameData.currentLevel.endCol &&
          player.targetTileY == gameData.currentLevel.endRow) {
        SDL_Log(
            "Player is starting turn on exit tile! Advancing to next level.");
        gameData.currentLevelIndex++;
        gameData.enemies.clear();           // Clear enemies
        gameData.activeProjectiles.clear(); // Clear projectiles
        gameData.playerIntendedAction = {}; // Clear intent
        gameData.enemyIntendedActions.clear();

        // Generate New Level
        Enemy::resetIdCounter(); // Reset IDs for new level
        // *** MODIFIED CALL to generateLevel ***
        std::optional<SDL_Point> pedestalPosOpt; // Variable to receive position
        gameData.currentLevel = generateLevel(
            gameData.levelWidth, gameData.levelHeight, gameData.levelMaxRooms,
            gameData.levelMinRoomSize, gameData.levelMaxRoomSize,
            gameData.enemies, gameData.tileWidth, gameData.tileHeight,
            pedestalPosOpt); // Pass the optional Point

        // *** NEW: Create pedestal object if position was found ***
        if (pedestalPosOpt.has_value()) {
          gameData.currentPedestal.emplace(pedestalPosOpt.value().x,
                                           pedestalPosOpt.value().y);

        } else {
          gameData.currentPedestal
              .reset(); // Ensure no pedestal if placement failed
        }
        // *** END NEW ***
        gameData.levelRooms = gameData.currentLevel.rooms;

        // --- Apply enemy scaling based on floor ---
        SDL_Log("Applying enemy scaling for floor %d...",
                gameData.currentLevelIndex);
        for (auto &enemy : gameData.enemies) {
          enemy.applyFloorScaling(gameData.currentLevelIndex,
                                  gameData.enemyStatScalingPerFloor);
        }
        // --- END Apply enemy scaling ---

        // Re-Initialize Occupation Grid
        gameData.occupationGrid.assign(
            gameData.currentLevel.height,
            std::vector<bool>(gameData.currentLevel.width, false));
        for (int y = 0; y < gameData.currentLevel.height; ++y)
          for (int x = 0; x < gameData.currentLevel.width; ++x)
            if (gameData.currentLevel.tiles[y][x] == '#')
              gameData.occupationGrid[y][x] = true;

        // Reset Player Position to New Start
        player.targetTileX = gameData.currentLevel.startCol;
        player.targetTileY = gameData.currentLevel.startRow;
        player.x =
            player.targetTileX * gameData.tileWidth + gameData.tileWidth / 2.0f;
        player.y = player.targetTileY * gameData.tileHeight +
                   gameData.tileHeight / 2.0f;
        player.startTileX = player.targetTileX;
        player.startTileY = player.targetTileY;
        player.isMoving = false; // Ensure player is not moving
        // Mark new player position on grid
        if (isWithinBounds(player.targetTileX, player.targetTileY,
                           gameData.currentLevel.width,
                           gameData.currentLevel.height)) {
          gameData.occupationGrid[player.targetTileY][player.targetTileX] =
              true;
        }

        // Mark initial enemy positions on grid for new level
        for (const auto &enemy : gameData.enemies) {
          if (isWithinBounds(enemy.x, enemy.y, gameData.currentLevel.width,
                             gameData.currentLevel.height)) {
            if (!gameData.occupationGrid[enemy.y][enemy.x]) {
              gameData.occupationGrid[enemy.y][enemy.x] = true;
            } else {
              SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                          "New level enemy %d spawn location [%d,%d] was "
                          "already occupied.",
                          enemy.id, enemy.x, enemy.y);
            }
          }
        }

        // --- Mark Pedestal Tile as Occupied (Correct Placement!) ---
        if (gameData.currentPedestal.has_value()) {
          int pedestalX = gameData.currentPedestal.value().x;
          int pedestalY = gameData.currentPedestal.value().y;
          if (isWithinBounds(pedestalX, pedestalY, gameData.currentLevel.width,
                             gameData.currentLevel.height)) {
            if (gameData.occupationGrid[pedestalY][pedestalX]) {
              SDL_LogWarn(
                  SDL_LOG_CATEGORY_APPLICATION,
                  "Pedestal location [%d,%d] was already marked occupied "
                  "(player/enemy spawn?)!",
                  pedestalX, pedestalY);
            }
            gameData.occupationGrid[pedestalY][pedestalX] =
                true; // Mark the initialized grid
            SDL_Log("Marked pedestal location [%d,%d] as occupied on grid.",
                    pedestalX, pedestalY);
          } else { /* Log warning */
          }
        }
        // --- END Mark Pedestal Tile ---

        // Reset Visibility
        gameData.visibilityMap.assign(
            gameData.currentLevel.height,
            std::vector<float>(gameData.currentLevel.width, 0.0f));
        updateVisibility(gameData.currentLevel, gameData.levelRooms,
                         player.targetTileX, player.targetTileY,
                         gameData.hallwayVisibilityDistance,
                         gameData.visibilityMap);

        // Transition directly to the start of the *next* turn's planning
        // (Effectively skipping the rest of the current frame's update logic
        // for this phase)
        SDL_Log(
            "New level %d generated. Restarting Planning_PlayerInput phase.",
            gameData.currentLevelIndex);
        // No phase change needed, we are already in Planning_PlayerInput, just
        // exit updateLogic early.
        return; // <<<< EXIT updateLogic early after level transition

      } // End if player is on exit tile check
      // SDL_Log("DEBUG: [UpdateLogic] In Planning_PlayerInput phase."); //
      // Usually not needed

      if (!player.isMoving) { // Only check if player is not currently mid-move
        // Use std::find_if with an index to allow removal
        auto it = std::find_if(gameData.droppedItems.begin(),
                               gameData.droppedItems.end(),
                               [&](const ItemDrop &item) {
                                 return item.x == player.targetTileX &&
                                        item.y == player.targetTileY;
                               });

        if (it != gameData.droppedItems.end()) {
          // Found an item at the player's location
          ItemDrop &itemToPickup = *it; // Get reference to the item

          // Apply effect based on type
          if (itemToPickup.type == ItemType::HealthCrystal) {
            int healthToRestore =
                static_cast<int>(player.maxHealth * 0.30f); // 30% of max health
            int oldHealth = player.health;
            player.health = std::min(player.health + healthToRestore,
                                     player.maxHealth); // Add and clamp
            SDL_Log(
                "INFO: Picked up Health Crystal. Restored %d HP (%d -> %d).",
                healthToRestore, oldHealth, player.health);
          } else if (itemToPickup.type == ItemType::ManaCrystal) {
            int manaToRestore =
                static_cast<int>(player.maxMana * 0.30f); // 30% of max mana
            int oldMana = player.mana;
            player.mana = std::min(player.mana + manaToRestore,
                                   player.maxMana); // Add and clamp
            SDL_Log("INFO: Picked up Mana Crystal. Restored %d MP (%d -> %d).",
                    manaToRestore, oldMana, player.mana);
          }

          // Remove the item from the dropped items list
          gameData.droppedItems.erase(it); // Erase the found item
        }
      }
      planningWallClockStartTime = 0; // Reset timers when in player input phase
      resolutionStartTime = 0;
      totalEnemyPlanningCpuTime = 0;

      break; // Waiting for input
    }
    case TurnPhase::Planning_EnemyAI: { // Scope for variables
      // *** START TIMING Planning_EnemyAI (Wall Clock) ***
      SDL_Log(
          "DEBUG: [UpdateLogic] Entering Planning_EnemyAI phase."); // Log phase
                                                                    // entry
      if (previousPhase ==
          TurnPhase::Planning_PlayerInput) { // Only start timer on first frame
                                             // entering this phase
        planningWallClockStartTime = SDL_GetTicks();
        totalEnemyPlanningCpuTime =
            0; // Ensure accumulator is reset at start of phase
      }
      // ***

      // Update player animation during this phase (if they started moving)
      if (gameData.currentGamePlayer.isMoving) {
        gameData.currentGamePlayer.update(deltaTime, gameData);
      }

      // --- Process ALL Enemies in a single loop ---
      // Ensure intended actions vector is sized correctly first
      if (gameData.enemyIntendedActions.size() != gameData.enemies.size()) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "Resizing enemyIntendedActions from %zu to %zu during planning.",
            gameData.enemyIntendedActions.size(), gameData.enemies.size());
        gameData.enemyIntendedActions.resize(gameData.enemies.size());
      }

      // --- Main Planning Loop for All Enemies ---
      SDL_Log(
          "DEBUG: [UpdateLogic] Starting enemy planning loop (%zu enemies)...",
          gameData.enemies.size());
      for (int i = 0; i < gameData.enemies.size(); ++i) {
        // Safety check for vector index just in case resize failed or size
        // changed unexpectedly
        if (i >= gameData.enemies.size() ||
            i >= gameData.enemyIntendedActions.size()) {
          SDL_LogError(
              SDL_LOG_CATEGORY_APPLICATION,
              "Index %d out of bounds during enemy planning loop (Enemy "
              "Size: %zu, Action Size: %zu). Skipping.",
              i, gameData.enemies.size(), gameData.enemyIntendedActions.size());
          continue; // Skip this iteration
        }

        Enemy &currentEnemy = gameData.enemies[i];

        // *** START TIMING Single Enemy CPU Time ***
        Uint32 singleEnemyStartTime = SDL_GetTicks();
        // ***

        if (currentEnemy.health > 0 && !currentEnemy.isMoving) {
          // Enemy is alive and not currently animating a move, plan its action
          IntendedAction plan = currentEnemy.planAction(
              gameData.currentLevel, gameData.currentGamePlayer, gameData);
          gameData.enemyIntendedActions[i] =
              plan; // Store the plan using loop index 'i'

          SDL_Log("DEBUG: [UpdateLogic] Enemy %d planned action type: %d",
                  currentEnemy.id, (int)plan.type);

          // Handle tentative occupation and instant invisible moves logic
          // immediately after planning
          if (plan.type == ActionType::Move) {
            if (isWithinBounds(plan.targetX, plan.targetY,
                               gameData.currentLevel.width,
                               gameData.currentLevel.height)) {
              // Check if the target tile is *currently* free in the grid
              if (!gameData.occupationGrid[plan.targetY][plan.targetX]) {
                // Mark it as tentatively occupied for subsequent enemies in
                // *this* planning pass
                gameData.occupationGrid[plan.targetY][plan.targetX] = true;
                // SDL_Log("INFO: Enemy %d tentatively occupies [%d,%d] for
                // planning.", currentEnemy.id, plan.targetX, plan.targetY); //
                // Optional Log

                // Check visibility to handle instant invisible moves
                float visibility = 0.0f;
                if (isWithinBounds(currentEnemy.x, currentEnemy.y,
                                   gameData.currentLevel.width,
                                   gameData.currentLevel.height) &&
                    currentEnemy.y < gameData.visibilityMap.size() &&
                    currentEnemy.x <
                        gameData.visibilityMap[currentEnemy.y].size()) {
                  visibility =
                      gameData.visibilityMap[currentEnemy.y][currentEnemy.x];
                }
                bool isVisible = (visibility > 0.0f);

                // If move is planned AND enemy is invisible, snap
                // visual/logical coords now
                if (!isVisible) {
                  currentEnemy.visualX = plan.targetX * gameData.tileWidth +
                                         gameData.tileWidth / 2.0f;
                  currentEnemy.visualY = plan.targetY * gameData.tileHeight +
                                         gameData.tileHeight / 2.0f;

                  // Update logical coords instantly too, clearing old spot on
                  // grid *if different*
                  if (isWithinBounds(currentEnemy.x, currentEnemy.y,
                                     gameData.currentLevel.width,
                                     gameData.currentLevel.height)) {
                    if (currentEnemy.x != plan.targetX ||
                        currentEnemy.y != plan.targetY) {
                      gameData.occupationGrid[currentEnemy.y][currentEnemy.x] =
                          false;
                    }
                  }
                  currentEnemy.x = plan.targetX;
                  currentEnemy.y = plan.targetY;
                  // The target spot was already marked occupied above for the
                  // tentative logic SDL_Log("INFO: Enemy %d (invisible)
                  // instantly updated logic/visual to planned move [%d,%d]",
                  // currentEnemy.id, plan.targetX, plan.targetY); // Optional
                  // Log
                }
                // End of invisible enemy instant update logic

              } else { // Target tile was already occupied (by player or
                       // previously planned enemy)
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Enemy %d planned move to [%d,%d] but it was "
                            "occupied during planning phase. Forcing WAIT.",
                            currentEnemy.id, plan.targetX, plan.targetY);
                gameData.enemyIntendedActions[i].type =
                    ActionType::Wait; // Force Wait
              }
            } else { // Target tile is out of bounds
              SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                          "Enemy %d planned move out of bounds to [%d,%d]. "
                          "Forcing WAIT.",
                          currentEnemy.id, plan.targetX, plan.targetY);
              gameData.enemyIntendedActions[i].type =
                  ActionType::Wait; // Force Wait
            }
          } // End if plan type is Move
        } else { // Enemy is dead or already moving (shouldn't happen for moving
          // if called correctly)
          SDL_Log("DEBUG: [UpdateLogic] Enemy %d skipped planning (Dead or "
                  "Moving). Health: %d, Moving: %s",
                  currentEnemy.id, currentEnemy.health,
                  currentEnemy.isMoving ? "true" : "false");
          gameData.enemyIntendedActions[i].type =
              ActionType::None; // Ensure no action if dead/moving
          if (currentEnemy.isMoving) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Enemy %d was already moving during planning phase? "
                        "Setting action to None.",
                        currentEnemy.id);
          }
        }

        // *** END TIMING Single Enemy CPU Time & Accumulate ***
        Uint32 singleEnemyEndTime = SDL_GetTicks();
        totalEnemyPlanningCpuTime +=
            (singleEnemyEndTime - singleEnemyStartTime);
        SDL_Log("DEBUG: [UpdateLogic] Finished planning for enemy index %d.",
                i);
        // ***

      } // --- End FOR loop processing all enemies ---

      // --- Cleanup Tentative Enemy Occupation Marks (After ALL enemies have
      // planned) ---
      for (int i = 0; i < gameData.enemies.size(); ++i) {
        if (i >= gameData.enemyIntendedActions.size())
          continue; // Safety check

        const IntendedAction &plan = gameData.enemyIntendedActions[i];

        // Only need to revert tentative marks for VISIBLE enemies who planned a
        // move. Invisible enemies already committed their grid changes.
        if (plan.type == ActionType::Move) {
          // Need the enemy object to check its visibility status *now*
          const Enemy &enemy = gameData.enemies[i]; // Get corresponding enemy
          float visibility = 0.0f;
          if (isWithinBounds(enemy.x, enemy.y, gameData.currentLevel.width,
                             gameData.currentLevel.height) &&
              enemy.y < gameData.visibilityMap.size() &&
              enemy.x < gameData.visibilityMap[enemy.y].size()) {
            visibility = gameData.visibilityMap[enemy.y][enemy.x];
          }
          bool isVisible = (visibility > 0.0f);

          if (isVisible && isWithinBounds(plan.targetX, plan.targetY,
                                          gameData.currentLevel.width,
                                          gameData.currentLevel.height)) {
            // Check if the grid is still marked as occupied by *this tentative
            // plan* We also need to ensure the player didn't end up logically
            // on that tile due to their concurrent move finishing.
            if (gameData.occupationGrid[plan.targetY][plan.targetX] &&
                !(gameData.currentGamePlayer.targetTileX == plan.targetX &&
                  gameData.currentGamePlayer.targetTileY == plan.targetY)) {
              // It was marked (likely by this enemy or another earlier in the
              // loop) and player isn't there, so clear it. This prepares the
              // grid for the Resolution phase where moves are actually
              // initiated.
              gameData.occupationGrid[plan.targetY][plan.targetX] = false;
              // SDL_Log("INFO: Clearing tentative *visible enemy* occupation
              // mark at [%d,%d].", plan.targetX, plan.targetY); // Optional Log
            }
          }
        }
      }
      // --- End Cleanup of Tentative Marks ---

      // *** LOG FINAL TIMING DATA ***
      if (planningWallClockStartTime >
          0) { // Ensure wall clock timer was started
        phaseEndTime = SDL_GetTicks();
        elapsedMs = phaseEndTime - planningWallClockStartTime;
        SDL_Log("INFO: --- Phase Planning_EnemyAI (Wall Clock) Took: %u ms ---",
                elapsedMs);
        planningWallClockStartTime = 0; // Reset wall clock timer
      }
      SDL_Log("INFO: --- Total Enemy Planning CPU Time This Turn: %u ms ---",
              totalEnemyPlanningCpuTime);
      // ***

      // Transition phase AFTER processing all enemies and cleaning up marks
      // SDL_Log("INFO: --- Enemy Planning Complete. Transitioning to Resolution
      // Start ---"); // Optional Log
      gameData.currentPhase = TurnPhase::Resolution_Start;
      SDL_Log("DEBUG: [UpdateLogic] Finished enemy planning loop.");

    } break; // End Planning_EnemyAI Case

    case TurnPhase::Resolution_Start: { // Scope
      SDL_Log("--- Phase: Resolution Start ---");
      SDL_Log(
          "DEBUG: [UpdateLogic] Entering Resolution_Start phase."); // Log phase
                                                                    // entry
      // Initiate Player Action
      IntendedAction pAction = gameData.playerIntendedAction;
      PlayerCharacter &player = gameData.currentGamePlayer;
      if (pAction.type == ActionType::Move) {
        if (isWithinBounds(pAction.targetX, pAction.targetY,
                           gameData.currentLevel.width,
                           gameData.currentLevel.height) &&
            !gameData.occupationGrid[pAction.targetY][pAction.targetX]) {
          if (isWithinBounds(player.targetTileX, player.targetTileY,
                             gameData.currentLevel.width,
                             gameData.currentLevel.height))
            gameData.occupationGrid[player.targetTileY][player.targetTileX] =
                false;
          gameData.occupationGrid[pAction.targetY][pAction.targetX] = true;
          player.startMove(pAction.targetX, pAction.targetY);
        } else {
          SDL_Log("Player planned move to [%d,%d] blocked at resolution.",
                  pAction.targetX, pAction.targetY);
        }
      }
      // Player Bump Attack Logic Removed
      else if (pAction.type == ActionType::CastSpell) {
        SDL_Log("Player resolves CAST SPELL %d.", pAction.spellIndex);
        player.castSpell(pAction.spellIndex, pAction.targetX, pAction.targetY,
                         gameData, &assets);
      } else if (pAction.type == ActionType::Wait) {
        SDL_Log("Player resolves WAIT.");
      } else if (pAction.type == ActionType::Interact) {
        SDL_Log("Player resolves INTERACT.");
        if (gameData.currentPedestal.has_value() &&
            gameData.currentPedestal.value().isActive) {
          // Double-check target coordinates match the pedestal (optional,
          // adjacency check in input might be enough)
          if (pAction.targetX == gameData.currentPedestal.value().x &&
              pAction.targetY == gameData.currentPedestal.value().y) {
            RunePedestal &pedestal =
                gameData.currentPedestal.value(); // Get mutable reference
            SDL_Log("Player interacts with active pedestal at [%d,%d]. "
                    "Starting deactivation.",
                    pedestal.x, pedestal.y);
            pedestal.isActive = false;      // Mark as inactive
            pedestal.isDeactivating = true; // Start deactivation animation
            pedestal.currentFrame = 0;      // Start animation from first frame
            pedestal.animationTimer = 0.0f; // Reset animation timer

            // --- ADD EFFECTS OF INTERACTION HERE ---
            // Example: Give player some mana/health, spawn an item, trigger an
            // event, etc.
            int manaReward = 50; // Example reward
            player.mana = std::min(player.mana + manaReward, player.maxMana);
            SDL_Log("Pedestal interaction granted %d mana. Player Mana: %d/%d",
                    manaReward, player.mana, player.maxMana);
            // -----------------------------------------

          } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Player interaction action targeted [%d,%d], but "
                        "pedestal is at [%d,%d]. Interaction failed.",
                        pAction.targetX, pAction.targetY,
                        gameData.currentPedestal.value().x,
                        gameData.currentPedestal.value().y);
          }
        } else {
          SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                      "Player interaction action resolved, but no active "
                      "pedestal found.");
        }
      }

      SDL_Log("DEBUG: [UpdateLogic] Starting enemy action resolution loop (%zu "
              "enemies)...",
              gameData.enemies.size());
      // Initiate Enemy Actions
      for (const auto &eAction : gameData.enemyIntendedActions) {
        // Skip actions with no assigned enemy ID (shouldn't happen if planning
        // is correct)
        if (eAction.enemyId == -1)
          continue;

        // Find the enemy associated with this action by ID
        Enemy *enemyPtr = nullptr;
        for (auto &potentialEnemy : gameData.enemies) {
          if (potentialEnemy.id == eAction.enemyId) {
            enemyPtr = &potentialEnemy;
            break;
          }
        }

        // Check if enemy was found AND is alive
        if (enemyPtr != nullptr && enemyPtr->health > 0) {
          Enemy &enemy = *enemyPtr; // Dereference the pointer for easier access

          SDL_Log("DEBUG: [UpdateLogic] Resolving action for Enemy ID %d "
                  "(Type: %d)",
                  enemy.id, (int)eAction.type);

          // Now resolve the action for this specific, living enemy
          switch (eAction.type) {
          case ActionType::Move: {
            // Check if target tile is valid and unoccupied
            if (isWithinBounds(eAction.targetX, eAction.targetY,
                               gameData.currentLevel.width,
                               gameData.currentLevel.height) &&
                !gameData.occupationGrid[eAction.targetY][eAction.targetX]) {
              // Update occupation grid: Clear old, set new
              if (isWithinBounds(enemy.x, enemy.y, gameData.currentLevel.width,
                                 gameData.currentLevel.height)) {
                gameData.occupationGrid[enemy.y][enemy.x] = false;
                SDL_Log("DEBUG: Enemy %d cleared occupation at [%d,%d]",
                        enemy.id, enemy.x, enemy.y);
              }
              gameData.occupationGrid[eAction.targetY][eAction.targetX] = true;
              SDL_Log("DEBUG: Enemy %d set occupation at [%d,%d]", enemy.id,
                      eAction.targetX, eAction.targetY);

              // Start the move animation
              enemy.startMove(eAction.targetX, eAction.targetY);
              SDL_Log("INFO: Enemy %d initiated MOVE to [%d,%d]", enemy.id,
                      eAction.targetX, eAction.targetY);
            } else {
              SDL_Log("WARN: Enemy %d planned move to [%d,%d] blocked at "
                      "resolution.",
                      enemy.id, eAction.targetX, eAction.targetY);
              // Enemy does nothing this turn if move is blocked at resolution
            }
            break;
          }
          case ActionType::Attack: {
            SDL_Log("INFO: Enemy %d resolves ATTACK on player at [%d,%d].",
                    enemy.id, eAction.targetX, eAction.targetY);
            // Check if player is still at the target location (optional, but
            // good practice)
            if (player.targetTileX == eAction.targetX &&
                player.targetTileY == eAction.targetY) {
              int damage = enemy.GetAttackDamage();
              enemy.startAttackAnimation(gameData); // Start animation
              player.takeDamage(damage);            // Apply damage
            } else {
              SDL_Log("INFO: Enemy %d attack whiffed! Player not at target "
                      "[%d,%d].",
                      enemy.id, eAction.targetX, eAction.targetY);
              // Optionally start a whiff animation or just wait
            }
            break;
          }
          case ActionType::CastSpell: {
            SDL_Log("INFO: Enemy %d resolves CAST SPELL %d.", enemy.id,
                    eAction.spellIndex);
            // Implement enemy spell casting logic here if needed
            break;
          }
          case ActionType::Wait: {
            SDL_Log("INFO: Enemy %d resolves WAIT.", enemy.id);
            // No action needed
            break;
          }
          case ActionType::None:
          default:
            // No action planned or unknown action
            break;
          } // End switch eAction.type

        } else if (enemyPtr == nullptr) {
          SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                      "Resolution_Start: Could not find enemy with ID %d for "
                      "planned action.",
                      eAction.enemyId);
        } else { // enemyPtr != nullptr && enemyPtr->health <= 0
          SDL_Log("DEBUG: [UpdateLogic] Skipping action resolution for dead "
                  "Enemy ID %d.",
                  enemyPtr->id);
        }
      }

      SDL_Log("DEBUG: [UpdateLogic] Finished enemy action resolution loop.");
      gameData.currentPhase = TurnPhase::Resolution_Update;

      SDL_Log("--- Transitioning to Resolution Update ---");
    } break;

    case TurnPhase::Resolution_Update: { // Scope

      // *** START TIMING Resolution_Update (Implicit Wait) ***
      if (previousPhase ==
          TurnPhase::Resolution_Start) { // Only log start time on first entry
        resolutionStartTime = SDL_GetTicks();
        // SDL_Log("INFO: --- Entering Phase: Resolution Update ---");
      }
      // ***

      bool playerFinishedMovingThisFrame = false;

      if (gameData.currentGamePlayer.isMoving) {
        bool wasMoving = true;
        gameData.currentGamePlayer.update(deltaTime, gameData);
        if (wasMoving && !gameData.currentGamePlayer.isMoving) {
          playerFinishedMovingThisFrame = true; // Player's move completed
        }
      }
      for (auto &enemy : gameData.enemies) {
        if (enemy.health > 0 && enemy.isMoving) {
          enemy.update(deltaTime, gameData);
        }
      }
      // Iterate using index to allow modification (finding enemy by ID)
      for (int i = 0; i < gameData.activeProjectiles.size(); ++i) {
        Projectile &proj = gameData.activeProjectiles[i]; // Use reference

        if (proj.isActive) {
          // Update projectile movement & check for hit
          if (proj.update(deltaTime, gameData)) { // Pass gameData for homing
            // --- PROJECTILE HIT ---
            proj.isActive = false; // Mark as inactive FIRST
            SDL_Log("Projectile arrived/hit target (EnemyID: %d). Applying "
                    "damage...",
                    proj.targetEnemyId);

            // Find the target enemy
            Enemy *targetEnemy = nullptr;
            // Prioritize homing target ID
            if (proj.targetEnemyId != -1) {
              for (auto &potentialTarget : gameData.enemies) {
                if (potentialTarget.id == proj.targetEnemyId &&
                    potentialTarget.health > 0) {
                  targetEnemy = &potentialTarget;
                  break;
                }
              }
              if (!targetEnemy) {
                SDL_Log("...Target Enemy ID %d not found or dead.",
                        proj.targetEnemyId);
              }
            }

            // If no homing target or homing target lost, check hit location
            if (!targetEnemy) {
              int hitTileX =
                  static_cast<int>(floor(proj.currentX / gameData.tileWidth));
              int hitTileY =
                  static_cast<int>(floor(proj.currentY / gameData.tileHeight));
              SDL_Log("...Checking for enemy at impact tile [%d,%d].", hitTileX,
                      hitTileY);
              for (auto &potentialTarget : gameData.enemies) {
                if (potentialTarget.health > 0 &&
                    potentialTarget.x == hitTileX &&
                    potentialTarget.y == hitTileY) {
                  targetEnemy = &potentialTarget;
                  SDL_Log("...Found enemy %d at impact tile.", targetEnemy->id);
                  break;
                }
              }
            }

            // Apply damage if a living target was found
            if (targetEnemy != nullptr) { // Check if pointer is valid
              targetEnemy->takeDamage(proj.damage);
              // Log is now inside Enemy::takeDamage
            } else {
              SDL_Log("...No living enemy found at impact point/target ID. "
                      "Damage not applied.");
            }
            // Projectile is now inactive and damage (if any) applied.
            // It will be removed later.
          }
          // else: Projectile still moving, do nothing else this frame
        } // End if proj.isActive
      } // End projectile update loop
      gameData.activeProjectiles.erase(
          std::remove_if(gameData.activeProjectiles.begin(),
                         gameData.activeProjectiles.end(),
                         [](const Projectile &p) { return !p.isActive; }),
          gameData.activeProjectiles.end());

      // ---> ADD Orbital Update Loop <---
      // Iterate backwards for safe removal while iterating
      for (int i = gameData.activeOrbitals.size() - 1; i >= 0; --i) {
        OrbitalMissile &orbital = gameData.activeOrbitals[i];

        // Update returns true if orbital launched or expired
        if (orbital.update(deltaTime, gameData,assets)) {
          // Orbital is markedForRemoval within its update if needed
        }
      }
      // Remove orbitals marked for removal
      gameData.activeOrbitals.erase(
          std::remove_if(
              gameData.activeOrbitals.begin(), gameData.activeOrbitals.end(),
              [](const OrbitalMissile &o) { return o.markedForRemoval; }),
          gameData.activeOrbitals.end());
      // --- END Orbital Update Loop ---
      SDL_Log("DEBUG: Entered Resolution_Update Check");

      if (isResolutionComplete(gameData)) {
        SDL_Log("DEBUG: [UpdateLogic] Movement Resolution Complete. "
                "Transitioning to TurnEnd_ApplyEffects.");

        SDL_Log("DEBUG: Resolution IS Complete");

        // *** END TIMING Resolution_Update ***
        if (resolutionStartTime > 0) { // Ensure timer was started
          phaseEndTime = SDL_GetTicks();
          elapsedMs = phaseEndTime - resolutionStartTime;
          SDL_Log("INFO: --- Phase Resolution_Update Took: %u ms ---",
                  elapsedMs);
          resolutionStartTime = 0; // Reset timer
        }
        // ***

        SDL_Log("--- Resolution Complete. Transitioning to Turn End Apply "
                "Effects ---");
        gameData.currentPhase = TurnPhase::TurnEnd_ApplyEffects;
      }
    } break;

    case TurnPhase::TurnEnd_ApplyEffects:
      SDL_Log("--- Phase: Turn End Apply Effects ---");
      SDL_Log("DEBUG: [UpdateLogic] Transitioning from TurnEnd_ApplyEffects to "
              "TurnEnd_Cleanup.");
      gameData.currentPhase = TurnPhase::TurnEnd_Cleanup;
      SDL_Log("--- Transitioning to Turn End Cleanup ---");
      break;

    case TurnPhase::TurnEnd_Cleanup:
      SDL_Log("--- Phase: Turn End Cleanup ---");
      SDL_Log("DEBUG: [UpdateLogic] Entering TurnEnd_Cleanup phase.");
      bool playerDied = gameData.currentGamePlayer.health <= 0;
      int arcanaGained = 0;
      gameData.enemies.erase(
          std::remove_if(
              gameData.enemies.begin(), gameData.enemies.end(),
              [&](const Enemy &e) {
                if (e.health <= 0) {
                  SDL_Log("Cleaning up dead enemy %d at [%d,%d].", e.id, e.x,
                          e.y);
                  arcanaGained += e.arcanaValue;

                  // --- *** NEW: Crystal Drop Logic *** ---
                  if ((rand() % 100) < gameData.crystalDropChancePercent) {
                    ItemType dropType;
                    std::string textureKey;
                    // Determine crystal type (Health or Mana)
                    if ((rand() % 100) < gameData.healthCrystalChancePercent) {
                      dropType = ItemType::HealthCrystal;
                      textureKey =
                          "health_crystal_texture"; // Use consistent key
                      SDL_Log(
                          "INFO: Enemy %d dropped a Health Crystal at [%d,%d].",
                          e.id, e.x, e.y);
                    } else {
                      dropType = ItemType::ManaCrystal;
                      textureKey = "mana_crystal_texture"; // Use consistent key
                      SDL_Log(
                          "INFO: Enemy %d dropped a Mana Crystal at [%d,%d].",
                          e.id, e.x, e.y);
                    }

                    // Create the ItemDrop object
                    ItemDrop newItem;
                    newItem.x = e.x; // Drop at enemy's location
                    newItem.y = e.y;
                    newItem.type = dropType;
                    newItem.textureName = textureKey;

                    // Add to the game's list of dropped items
                    gameData.droppedItems.push_back(newItem);
                  }
                  // --- *** END Crystal Drop Logic *** ---

                  // --- CORRECTED Grid Clearing ---
                  // 1. Clear the enemy's current logical position
                  if (isWithinBounds(e.x, e.y, gameData.currentLevel.width,
                                     gameData.currentLevel.height)) {
                    if (gameData.occupationGrid[e.y][e.x]) {
                      gameData.occupationGrid[e.y][e.x] = false;
                      SDL_Log("Cleared occupation grid at dead enemy's logical "
                              "pos [%d,%d]",
                              e.x, e.y);
                    } else {
                      // This might happen if the enemy died *before* its move
                      // resolution started properly
                      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                  "Dead enemy %d's logical pos [%d,%d] was "
                                  "already clear on grid.",
                                  e.id, e.x, e.y);
                    }
                  }

                  // 2. If the enemy was moving when it died, ALSO clear its
                  // target tile
                  if (e.isMoving) {
                    SDL_Log("Dead enemy %d was moving. Checking target tile "
                            "[%d,%d].",
                            e.id, e.targetTileX, e.targetTileY);
                    if (isWithinBounds(e.targetTileX, e.targetTileY,
                                       gameData.currentLevel.width,
                                       gameData.currentLevel.height)) {
                      if (gameData
                              .occupationGrid[e.targetTileY][e.targetTileX]) {
                        gameData.occupationGrid[e.targetTileY][e.targetTileX] =
                            false;
                        SDL_Log(
                            "Cleared occupation grid at dead enemy's target "
                            "pos [%d,%d]",
                            e.targetTileX, e.targetTileY);
                      } else {
                        // This could happen if the move was blocked at
                        // resolution start, but enemy died before cleanup
                        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                    "Dead moving enemy %d's target pos [%d,%d] "
                                    "was already clear on grid.",
                                    e.id, e.targetTileX, e.targetTileY);
                      }
                    }
                  }
                  return true;
                }
                return false;
              }),
          gameData.enemies.end());
      if (arcanaGained > 0)
        gameData.currentGamePlayer.GainArcana(arcanaGained);
      gameData.activeProjectiles.erase(
          std::remove_if(gameData.activeProjectiles.begin(),
                         gameData.activeProjectiles.end(),
                         [](const Projectile &p) { return !p.isActive; }),
          gameData.activeProjectiles.end());
      if (playerDied) {
        SDL_Log("--- Game Over ---");
        currentAppState = AppState::MainMenu;
        break;
      } // Go back to menu on death
      // *** INSERT REINFORCEMENT LOGIC HERE ***
      if (gameData.enemies.size() < gameData.maxEnemyCount &&
          gameData.spawnChancePercent > 0 && !gameData.levelRooms.empty()) {
        if ((rand() % 100) < gameData.spawnChancePercent) {
          SDL_Log("Attempting to spawn reinforcement...");
          std::pair<int, int> spawnPos = {-1, -1};
          int maxSpawnAttempts =
              gameData.levelRooms.size() * 3; // Increase attempts slightly

          for (int attempt = 0; attempt < maxSpawnAttempts; ++attempt) {
            int roomIndex = rand() % gameData.levelRooms.size();
            const SDL_Rect &room = gameData.levelRooms[roomIndex];
            if (room.w <= 2 || room.h <= 2)
              continue;

            int potentialX = room.x + 1 + (rand() % std::max(1, room.w - 2));
            int potentialY = room.y + 1 + (rand() % std::max(1, room.h - 2));

            // Check bounds, occupation, player pos, and visibility
            if (isWithinBounds(potentialX, potentialY,
                               gameData.currentLevel.width,
                               gameData.currentLevel.height)) {
              bool occupied = gameData.occupationGrid[potentialY][potentialX];
              bool isPlayerPos =
                  (potentialX == gameData.currentGamePlayer.targetTileX &&
                   potentialY == gameData.currentGamePlayer.targetTileY);
              float visibility =
                  (potentialY < gameData.visibilityMap.size() &&
                   potentialX < gameData.visibilityMap[potentialY].size())
                      ? gameData.visibilityMap[potentialY][potentialX]
                      : 0.0f;
              bool isVisible = (visibility > 0.0f);

              if (!occupied && !isPlayerPos &&
                  !isVisible) { // Spawn only if unoccupied, not player pos, and
                                // not visible
                spawnPos = {potentialX, potentialY};
                break; // Found a valid spot
              }
            }
          } // End spawn attempt loop

          if (spawnPos.first != -1) { // Valid spot found
            int spawnX = spawnPos.first;
            int spawnY = spawnPos.second;
            int newId = Enemy::getNextId(); // Get unique ID
            // Add new enemy (e.g., always Slime for now)
            gameData.enemies.emplace_back(newId, EnemyType::SLIME, spawnX,
                                          spawnY, gameData.tileWidth,
                                          gameData.tileHeight);
            // Mark occupation grid immediately
            gameData.occupationGrid[spawnY][spawnX] = true;

            if (!gameData.enemies.empty()) {
              gameData.enemies.back().applyFloorScaling(
                  gameData.currentLevelIndex,
                  gameData.enemyStatScalingPerFloor);
              SDL_Log("Applied floor scaling to reinforcement Enemy ID %d.",
                      newId);
            } else {
              // This case should theoretically not happen if emplace_back
              // succeeded, but defensive check
              SDL_LogError(
                  SDL_LOG_CATEGORY_APPLICATION,
                  "Failed to access reinforcement enemy after emplace_back!");
            }

            SDL_Log("Reinforcement (Enemy %d) spawned at [%d, %d]. Total "
                    "enemies: %zu",
                    newId, spawnX, spawnY, gameData.enemies.size());
            // Resize intended actions vector for the new enemy next turn
            gameData.enemyIntendedActions.resize(gameData.enemies.size());
          } else {
            SDL_Log("No valid spawn location found for reinforcement after %d "
                    "attempts.",
                    maxSpawnAttempts);
          }
        } // End spawn chance check
      } // End reinforcement condition check
      // *****************************************
      gameData.playerIntendedAction = {};
      gameData.enemyIntendedActions.clear();
      gameData.currentGamePlayer
          .ApplyTurnEndEffects(); // Handles decay, regen, etc.
      SDL_Log("--- Turn End. Transitioning to Player Input for Next Turn ---");
      SDL_Log("DEBUG: [UpdateLogic] Transitioning from TurnEnd_Cleanup to "
              "Planning_PlayerInput.");
      gameData.currentPhase = TurnPhase::Planning_PlayerInput;
      break;

    } // End TurnPhase switch
    // SDL_Log("DEBUG: [UpdateLogic End] Exiting phase switch. Current Phase:
    // %d",
    //         (int)gameData.currentPhase);
  }
}
// --- Rewritten renderScene Function ---
void renderScene(GameData &gameData, AssetManager &assets) {
  // --- Render Level Tiles ---
  if (gameData.currentLevel.width > 0 && gameData.currentLevel.height > 0 &&
      !gameData.currentLevel.tiles.empty() && gameData.tileWidth > 0 &&
      gameData.tileHeight > 0) {
    SDL_Texture *wallTexture = assets.getTexture("wall_texture");
    SDL_Texture *startTexture = assets.getTexture("start_tile");
    SDL_Texture *exitTexture = assets.getTexture("exit_tile");
    std::vector<SDL_Texture *> floorTextures = {assets.getTexture("floor_1"),
                                                assets.getTexture("floor_2")};
    floorTextures.erase(
        std::remove(floorTextures.begin(), floorTextures.end(), nullptr),
        floorTextures.end());
    std::vector<double> floorWeights = {3.0, 7.0};
    if (floorWeights.size() != floorTextures.size() && !floorTextures.empty()) {
      floorWeights.assign(floorTextures.size(), 1.0);
    }
    double totalWeight = 0;
    for (double w : floorWeights)
      totalWeight += w;
    std::vector<double> cumulativeWeights;
    double currentCumulative = 0;
    if (totalWeight > 0) {
      for (size_t i = 0; i < floorWeights.size(); ++i) {
        currentCumulative += floorWeights[i];
        cumulativeWeights.push_back(currentCumulative / totalWeight);
      }
    }
    int startTileX = std::max(0, gameData.cameraX / gameData.tileWidth);
    int startTileY = std::max(0, gameData.cameraY / gameData.tileHeight);
    // End calculation uses LOGICAL viewport size
    int endTileX = std::min(
        gameData.currentLevel.width,
        (gameData.cameraX + gameData.logicalWidth) / gameData.tileWidth + 1);
    int endTileY = std::min(
        gameData.currentLevel.height,
        (gameData.cameraY + gameData.logicalHeight) / gameData.tileHeight + 1);
    for (int y = startTileY; y < endTileY; ++y) {
      for (int x = startTileX; x < endTileX; ++x) {
        if (!isWithinBounds(x, y, gameData.currentLevel.width,
                            gameData.currentLevel.height) ||
            y >= (int)gameData.visibilityMap.size() ||
            x >= (int)gameData.visibilityMap[y].size())
          continue;
        SDL_Rect tileRect = {(x * gameData.tileWidth) - gameData.cameraX,
                             (y * gameData.tileHeight) - gameData.cameraY,
                             gameData.tileWidth, gameData.tileHeight};
        float visibility = gameData.visibilityMap[y][x];
        if (visibility > 0.0f) {
          SDL_Texture *textureToRender = nullptr;
          bool isFloor = false;
          if (y >= (int)gameData.currentLevel.tiles.size() ||
              x >= (int)gameData.currentLevel.tiles[y].size())
            continue;
          char currentTileType = gameData.currentLevel.tiles[y][x];
          if (y == gameData.currentLevel.startRow &&
              x == gameData.currentLevel.startCol && startTexture)
            textureToRender = startTexture;
          else if (y == gameData.currentLevel.endRow &&
                   x == gameData.currentLevel.endCol && exitTexture)
            textureToRender = exitTexture;
          else if (currentTileType == '#' && wallTexture)
            textureToRender = wallTexture;
          else if (currentTileType == '.')
            isFloor = true;
          if (isFloor && !floorTextures.empty() && totalWeight > 0 &&
              !cumulativeWeights.empty()) {
            unsigned int hash = ((unsigned int)x * 2654435761u) ^
                                ((unsigned int)y * 3063691763u);
            double hashValue = (double)(hash % 10000) / 10000.0;
            for (size_t i = 0; i < cumulativeWeights.size(); ++i)
              if (hashValue <= cumulativeWeights[i]) {
                textureToRender = floorTextures[i];
                break;
              }
            if (!textureToRender)
              textureToRender = floorTextures[0];
          } else if (isFloor)
            textureToRender = nullptr;
          if (textureToRender != nullptr)
            SDL_RenderCopy(gameData.renderer, textureToRender, nullptr,
                           &tileRect);
          else {
            Uint8 r = 50, g = 50, b = 50;
            if (currentTileType == '#') {
              r = 139;
              g = 69;
              b = 19;
            } else if (currentTileType == '.') {
              r = 100;
              g = 100;
              b = 100;
            }
            SDL_SetRenderDrawColor(gameData.renderer, r, g, b, 255);
            SDL_RenderFillRect(gameData.renderer, &tileRect);
          }
          Uint8 alpha = static_cast<Uint8>((1.0f - visibility) * 200);
          SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_BLEND);
          SDL_SetRenderDrawColor(gameData.renderer, 0, 0, 0, alpha);
          SDL_RenderFillRect(gameData.renderer, &tileRect);
          SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_NONE);
        } else {
          SDL_SetRenderDrawColor(gameData.renderer, 0, 0, 0, 255);
          SDL_RenderFillRect(gameData.renderer, &tileRect);
        }
      }
    } // End x, y loops
  } // End level rendering check

  // --- *** NEW: Render Dropped Items *** ---
  for (const auto &item : gameData.droppedItems) {
    // Check visibility of the item's tile
    float visibility = 0.0f;
    if (isWithinBounds(item.x, item.y, gameData.currentLevel.width,
                       gameData.currentLevel.height) &&
        item.y < gameData.visibilityMap.size() &&
        item.x < gameData.visibilityMap[item.y].size()) {
      visibility = gameData.visibilityMap[item.y][item.x];
    }

    if (visibility > 0.0f) { // Only render if the tile is visible
      SDL_Texture *itemTexture = assets.getTexture(item.textureName);
      if (itemTexture) {
        SDL_Rect itemRect = {
            (item.x * gameData.tileWidth) - gameData.cameraX,
            (item.y * gameData.tileHeight) - gameData.cameraY,
            gameData.tileWidth / 2, // Smaller size for item? Adjust as needed
            gameData.tileHeight / 2 // Smaller size for item? Adjust as needed
        };
        // Center the smaller item texture within the tile visually
        itemRect.x += gameData.tileWidth / 4;
        itemRect.y += gameData.tileHeight / 4;

        // Apply visibility alpha
        Uint8 alpha = static_cast<Uint8>(visibility * 255);
        SDL_SetTextureAlphaMod(itemTexture, alpha);
        SDL_SetTextureBlendMode(itemTexture,
                                SDL_BLENDMODE_BLEND); // Ensure blending

        SDL_RenderCopy(gameData.renderer, itemTexture, nullptr, &itemRect);

        // Reset alpha/blend mode if other things rely on defaults
        SDL_SetTextureAlphaMod(itemTexture, 255); // Reset alpha for next use
        // SDL_SetTextureBlendMode(itemTexture, SDL_BLENDMODE_NONE); //
        // Usually not needed to reset blend

      } else {
        // Optional: Render a fallback if texture is missing
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Item texture '%s' not found!", item.textureName.c_str());
        SDL_Rect fallbackRect = {(item.x * gameData.tileWidth) -
                                     gameData.cameraX + gameData.tileWidth / 4,
                                 (item.y * gameData.tileHeight) -
                                     gameData.cameraY + gameData.tileHeight / 4,
                                 gameData.tileWidth / 2,
                                 gameData.tileHeight / 2};
        Uint8 r = 255, g = 255, b = 0; // Yellow fallback
        if (item.type == ItemType::HealthCrystal) {
          r = 255;
          g = 0;
          b = 0;
        } // Red
        else if (item.type == ItemType::ManaCrystal) {
          r = 0;
          g = 0;
          b = 255;
        } // Blue

        Uint8 alpha =
            static_cast<Uint8>(visibility * 128); // Dimmer fallback alpha
        SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(gameData.renderer, r, g, b, alpha);
        SDL_RenderFillRect(gameData.renderer, &fallbackRect);
        SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_NONE);
      }
    }
  }
  // --- *** END Render Dropped Items *** ---

  // --- *** NEW: Render Rune Pedestal *** ---
  if (gameData.currentPedestal.has_value()) {
    const RunePedestal &pedestal =
        gameData.currentPedestal.value(); // Use const ref

    // Check visibility
    float visibility = 0.0f;
    if (isWithinBounds(pedestal.x, pedestal.y, gameData.currentLevel.width,
                       gameData.currentLevel.height) &&
        pedestal.y < gameData.visibilityMap.size() &&
        pedestal.x < gameData.visibilityMap[pedestal.y].size()) {
      visibility = gameData.visibilityMap[pedestal.y][pedestal.x];
    }

    if (visibility > 0.0f) { // Only render if visible tile
      SDL_Texture *pedestalTexture = nullptr;
      std::string textureKey;

      if (pedestal.isDeactivating) {
        // --- Render Deactivation Animation ---
        if (!pedestal.deactivationFrameTextureNames.empty() &&
            pedestal.currentFrame >= 0 &&
            pedestal.currentFrame <
                pedestal.deactivationFrameTextureNames.size()) {
          textureKey =
              pedestal.deactivationFrameTextureNames[pedestal.currentFrame];
        } else if (!pedestal.deactivationFrameTextureNames.empty()) {
          // Fallback to last frame if index is somehow invalid during
          // deactivation
          textureKey = pedestal.deactivationFrameTextureNames.back();
          SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                      "Invalid frame index %d during pedestal deactivation, "
                      "using last frame.",
                      pedestal.currentFrame);
        }
      } else if (pedestal.isActive) {
        // --- Render Idle Animation ---
        if (!pedestal.idleFrameTextureNames.empty() &&
            pedestal.currentFrame >= 0 &&
            pedestal.currentFrame < pedestal.idleFrameTextureNames.size()) {
          textureKey = pedestal.idleFrameTextureNames[pedestal.currentFrame];
        } else if (!pedestal.idleFrameTextureNames.empty()) {
          // Fallback to first idle frame if index invalid
          textureKey = pedestal.idleFrameTextureNames.front();
          SDL_LogWarn(
              SDL_LOG_CATEGORY_APPLICATION,
              "Invalid frame index %d during pedestal idle, using first frame.",
              pedestal.currentFrame);
        }
      } else {
        // --- Render Deactivated State (Last frame of deactivation) ---
        if (!pedestal.deactivationFrameTextureNames.empty()) {
          textureKey = pedestal.deactivationFrameTextureNames
                           .back(); // Use the last frame
        } else {
          SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                      "Cannot render deactivated pedestal: No deactivation "
                      "frames defined.");
          // Optionally render a fallback placeholder here if textureKey remains
          // empty
        }
      }

      // Get and render the chosen texture
      if (!textureKey.empty()) {
        pedestalTexture = assets.getTexture(textureKey);
      }

      if (pedestalTexture) {
        SDL_Rect pedestalRect = {
            /* ... calculate rect ... */
            (pedestal.x * gameData.tileWidth) - gameData.cameraX,
            (pedestal.y * gameData.tileHeight) - gameData.cameraY,
            gameData.tileWidth, gameData.tileHeight};
        Uint8 alpha = static_cast<Uint8>(visibility * 255);
        SDL_SetTextureAlphaMod(pedestalTexture, alpha);
        SDL_SetTextureBlendMode(pedestalTexture, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(gameData.renderer, pedestalTexture, nullptr,
                       &pedestalRect);
        SDL_SetTextureAlphaMod(pedestalTexture, 255); // Reset alpha
      } else {
        // Optional: Render placeholder if texture failed to load or key was
        // empty
        if (!textureKey.empty()) {
          SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                      "Pedestal texture '%s' not found!", textureKey.c_str());
        }
        // Draw simple placeholder (e.g., gray square)
        SDL_Rect fallbackRect = {
            (pedestal.x * gameData.tileWidth) - gameData.cameraX,
            (pedestal.y * gameData.tileHeight) - gameData.cameraY,
            gameData.tileWidth, gameData.tileHeight};
        Uint8 alpha = static_cast<Uint8>(visibility * 128);
        SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(gameData.renderer, 100, 100, 100,
                               alpha); // Gray fallback
        SDL_RenderFillRect(gameData.renderer, &fallbackRect);
        SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_NONE);
      }
    }
  }

  // --- Render Entities ---
  for (const auto &enemy : gameData.enemies) {
    if (enemy.health > 0) {
      int ex = enemy.x;
      int ey = enemy.y;
      float vis = 0.0f;
      if (isWithinBounds(ex, ey, gameData.currentLevel.width,
                         gameData.currentLevel.height) &&
          ey < (int)gameData.visibilityMap.size() &&
          ex < (int)gameData.visibilityMap[ey].size()) {
        vis = gameData.visibilityMap[ey][ex];
      }
      if (vis > 0.0f) {
        enemy.render(gameData.renderer, assets, gameData.cameraX,
                     gameData.cameraY, vis);
      }
    }
  }

  // --- Render Player ---
  SDL_Texture *playerTexture = nullptr;
  std::string textureKeyToUse;
  bool isTargeting = gameData.showTargetingReticle; // Cache flags
  bool isPlayerMoving = gameData.currentGamePlayer.isMoving;

  // --- MODIFIED: Select Animation Frame ---
  if (isTargeting) {
    // --- Use Targeting Animation ---
    SDL_Log("DEBUG: [RenderPlayer] Player IS targeting. TargetFrames=%zu, "
            "CurrentTargetFrame=%d",
            gameData.currentGamePlayer.targetingFrameTextureNames.size(),
            gameData.currentGamePlayer.currentTargetingFrame);
    if (!gameData.currentGamePlayer.targetingFrameTextureNames.empty() &&
        gameData.currentGamePlayer.currentTargetingFrame <
            gameData.currentGamePlayer.targetingFrameTextureNames.size()) {
      textureKeyToUse = gameData.currentGamePlayer.targetingFrameTextureNames
                            [gameData.currentGamePlayer.currentTargetingFrame];
      SDL_Log("DEBUG: [RenderPlayer] Using TARGETING key: %s",
              textureKeyToUse.c_str());
    } else if (!gameData.currentGamePlayer.idleFrameTextureNames
                    .empty()) { // Fallback to idle if targeting frames
                                // missing
      textureKeyToUse = gameData.currentGamePlayer.idleFrameTextureNames[0];
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "[RenderPlayer] Targeting frame invalid/missing, fallback to "
                  "IDLE key: %s",
                  textureKeyToUse.c_str());
    }

  } else if (isPlayerMoving) {
    // Use Walking Animation
    if (!gameData.currentGamePlayer.walkFrameTextureNames.empty() &&
        gameData.currentGamePlayer.currentWalkFrame <
            gameData.currentGamePlayer.walkFrameTextureNames.size()) {
      textureKeyToUse = gameData.currentGamePlayer.walkFrameTextureNames
                            [gameData.currentGamePlayer.currentWalkFrame];
    } else if (!gameData.currentGamePlayer.idleFrameTextureNames.empty()) {
      // Fallback to first idle frame if walk frames are missing/invalid
      textureKeyToUse = gameData.currentGamePlayer.idleFrameTextureNames[0];
      if (gameData.currentGamePlayer.walkFrameTextureNames.empty()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Player is moving but has no walk frames defined!");
      }
    }
  } else {
    // Use Idle Animation (existing logic)
    bool shouldAnimateIdle =
        (currentAppState == AppState::Gameplay &&
         gameData.currentPhase == TurnPhase::Planning_PlayerInput &&
         !gameData.showTargetingReticle); // Check idle conditions
    if (shouldAnimateIdle &&
        !gameData.currentGamePlayer.idleFrameTextureNames.empty() &&
        gameData.currentGamePlayer.currentIdleFrame <
            gameData.currentGamePlayer.idleFrameTextureNames.size()) {
      textureKeyToUse = gameData.currentGamePlayer.idleFrameTextureNames
                            [gameData.currentGamePlayer.currentIdleFrame];
    } else if (!gameData.currentGamePlayer.idleFrameTextureNames.empty()) {
      // Default to first idle frame if not animating idle or frames missing
      textureKeyToUse = gameData.currentGamePlayer.idleFrameTextureNames[0];
      if (gameData.currentGamePlayer.idleFrameTextureNames.empty()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Player is idle but has no idle frames defined!");
      }
    }
  }

  // Get the texture using the determined key
  if (!textureKeyToUse.empty()) {
    playerTexture = assets.getTexture(textureKeyToUse);
  }
  // --- END MODIFIED Frame Selection ---

  // Render the texture if found, otherwise use fallback
  if (playerTexture) {
    SDL_Rect playerRect;
    int texW, texH;
    SDL_QueryTexture(playerTexture, NULL, NULL, &texW, &texH);
    playerRect.w = 0.8 * gameData.tileWidth;  // Example fixed width
    playerRect.h = 0.8 * gameData.tileHeight; // Example fixed height
    playerRect.x = static_cast<int>(std::round(gameData.currentGamePlayer.x -
                                               playerRect.w / 2.0f)) -
                   gameData.cameraX;
    playerRect.y = static_cast<int>(std::round(gameData.currentGamePlayer.y -
                                               playerRect.h / 2.0f)) -
                   gameData.cameraY;

    // <<< DETERMINE FLIP BASED ON FACING DIRECTION >>>
    SDL_RendererFlip flip = SDL_FLIP_NONE; // Default: no flip (facing left)
    if (gameData.currentGamePlayer.currentFacingDirection ==
        PlayerCharacter::FacingDirection::Right) {
      flip = SDL_FLIP_HORIZONTAL; // Flip horizontally if facing left
    }

    // <<< USE SDL_RenderCopyEx WITH FLIP >>>
    SDL_RenderCopyEx(gameData.renderer, playerTexture,
                     nullptr,     // Source rect (entire texture)
                     &playerRect, // Destination rect
                     0.0,         // Angle (no rotation)
                     nullptr,     // Center of rotation (not needed)
                     flip);       // Apply flip

  } else {
    // Fallback green rectangle
    SDL_Rect playerRect;
    playerRect.w = gameData.tileWidth / 2;
    playerRect.h = gameData.tileHeight / 2;
    playerRect.x =
        static_cast<int>(gameData.currentGamePlayer.x - playerRect.w / 2.0f) -
        gameData.cameraX;
    playerRect.y =
        static_cast<int>(gameData.currentGamePlayer.y - playerRect.h / 2.0f) -
        gameData.cameraY;
    SDL_SetRenderDrawColor(gameData.renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(gameData.renderer, &playerRect);
    // Logging for missing textures...
    if (!gameData.currentGamePlayer.idleFrameTextureNames.empty() &&
        !textureKeyToUse.empty()) {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "Player texture '%s' not found!", textureKeyToUse.c_str());
    } else if (gameData.currentGamePlayer.idleFrameTextureNames.empty()) {
      SDL_LogWarn(
          SDL_LOG_CATEGORY_APPLICATION,
          "Player has no idle frame textures defined in character.cpp!");
    }
  }
  // --- End Player Rendering ---
  PlayerCharacter &player =
      gameData.currentGamePlayer; // Get reference for convenience
  if (player.currentShield > 0 && !player.wardFrameTextureKeys.empty()) {
    // Get the correct texture key for the current frame
    int frameIndex = player.currentWardFrame;
    // Basic bounds check for safety
    if (frameIndex >= 0 && frameIndex < player.wardFrameTextureKeys.size()) {
      std::string wardTextureKey = player.wardFrameTextureKeys[frameIndex];
      SDL_Texture *wardTexture = assets.getTexture(wardTextureKey);

      if (wardTexture) {
        // Calculate destination rectangle for the ward effect
        // Let's make it slightly larger than the player's render rect
        // (Assuming playerRect is still in scope from player rendering above,
        // otherwise recalculate player render size/pos)
        // Note: We might need playerRect calculation from earlier, let's assume
        // a size for now
        int playerRenderWidth = static_cast<int>(
            gameData.tileWidth * 0.8); // Use consistent size if possible
        int playerRenderHeight = static_cast<int>(gameData.tileHeight * 0.8);
        int wardWidth =
            static_cast<int>(playerRenderWidth * 1.2f); // Slightly larger
        int wardHeight =
            static_cast<int>(playerRenderHeight * 1.2f); // Slightly larger

        SDL_Rect wardRect;
        wardRect.w = wardWidth;
        wardRect.h = wardHeight;
        // Center it precisely on player's visual coordinates
        wardRect.x =
            static_cast<int>(std::round(player.x - wardRect.w / 2.0f)) -
            gameData.cameraX;
        wardRect.y =
            static_cast<int>(std::round(player.y - wardRect.h / 2.0f)) -
            gameData.cameraY;

        // Set transparency (alpha value 0-255)
        Uint8 wardAlpha = 70; // Adjust for desired transparency
        SDL_SetTextureAlphaMod(wardTexture, wardAlpha);
        SDL_SetTextureBlendMode(
            wardTexture, SDL_BLENDMODE_BLEND); // Crucial for transparency

        // Render the current ward frame
        SDL_RenderCopy(gameData.renderer, wardTexture, nullptr, &wardRect);

        // It's good practice to reset alpha/blend for the texture if it might
        // be reused differently, though getting it via AssetManager might
        // mitigate this need. SDL_SetTextureAlphaMod(wardTexture, 255);

      } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Ward texture key '%s' not found!", wardTextureKey.c_str());
      }
    } else {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid ward frame index: %d",
                  frameIndex);
    }
  }
  // --- END WARD EFFECT RENDERING ---

       // ---> ADD Orbital Rendering <---
       for (const auto& orbital : gameData.activeOrbitals) {
        // Pass necessary arguments to orbital's render method
        orbital.render(gameData.renderer, assets, gameData.cameraX, gameData.cameraY);
   }
   // --- END Orbital Rendering ---

  // --- Render Projectiles ---
  for (const auto &proj : gameData.activeProjectiles) {
    if (proj.isActive) {
      proj.render(gameData.renderer, gameData.cameraX, gameData.cameraY);
    }
  }

  // --- Render Targeting Reticle ---
  if (gameData.currentPhase == TurnPhase::Planning_PlayerInput &&
      gameData.showTargetingReticle) {
    if (isWithinBounds(gameData.targetIndicatorX, gameData.targetIndicatorY,
                       gameData.currentLevel.width,
                       gameData.currentLevel.height) &&
        gameData.targetIndicatorY < (int)gameData.visibilityMap.size() &&
        gameData.targetIndicatorX <
            (int)gameData.visibilityMap[gameData.targetIndicatorY].size() &&
        gameData.visibilityMap[gameData.targetIndicatorY]
                              [gameData.targetIndicatorX] > 0.0f) {
      SDL_Rect reticleRect = {
          (gameData.targetIndicatorX * gameData.tileWidth) - gameData.cameraX,
          (gameData.targetIndicatorY * gameData.tileHeight) - gameData.cameraY,
          gameData.tileWidth, gameData.tileHeight};
      SDL_Texture *reticleTexture = assets.getTexture("reticle");
      Uint8 reticleAlpha = 180;
      if (gameData.currentSpellIndex != -1) {
        const Spell &spell =
            gameData.currentGamePlayer.getSpell(gameData.currentSpellIndex);
        int distance = std::abs(gameData.currentGamePlayer.targetTileX -
                                gameData.targetIndicatorX) +
                       std::abs(gameData.currentGamePlayer.targetTileY -
                                gameData.targetIndicatorY);
        int effectiveRange = gameData.currentGamePlayer.GetEffectiveSpellRange(
            gameData.currentSpellIndex);
        if (distance <= effectiveRange) {
          if (reticleTexture)
            SDL_SetTextureColorMod(reticleTexture, 255, 255, 255);
          SDL_SetRenderDrawColor(gameData.renderer, 255, 255, 255,
                                 reticleAlpha);
        } else {
          if (reticleTexture)
            SDL_SetTextureColorMod(reticleTexture, 255, 100, 100);
          SDL_SetRenderDrawColor(gameData.renderer, 255, 0, 0, reticleAlpha);
        }
      } else {
        if (reticleTexture)
          SDL_SetTextureColorMod(reticleTexture, 255, 255, 255);
        SDL_SetRenderDrawColor(gameData.renderer, 255, 255, 255, reticleAlpha);
      }
      if (reticleTexture) {
        SDL_SetTextureAlphaMod(reticleTexture, reticleAlpha);
        SDL_SetTextureBlendMode(reticleTexture, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(gameData.renderer, reticleTexture, nullptr,
                       &reticleRect);
      } else {
        SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderDrawRect(gameData.renderer, &reticleRect);
        SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_NONE);
      }
    }
  }

  // --- Render UI Overlays ---
  renderSpellBar(gameData, assets);
  renderUI(gameData, assets);
  if (gameData.currentMenu == GameMenu::SpellMenu) {
    renderSpellMenu(gameData.renderer, assets.getFont("main_font"),
                    gameData.currentGamePlayer, gameData.spellSelectIndex,
                    gameData.windowWidth, gameData.windowHeight);
  } else if (gameData.currentMenu == GameMenu::CharacterSheet) {
    renderCharacterSheet(gameData, assets);
  }
}
