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
#include "projectile.h"    // For Projectile struct
#include "status_effect.h" //For status effects
#include "ui.h"            // For rendering UI elements
#include "utils.h"         // Includes SDL_Context, helper functions
#include "visibility.h"    // For updateVisibility function

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
  // --- ADD THIS CHECK ---
  for (const auto &effect : gameData.activeEffects) {
    // If an effect exists that hasn't yet been marked for removal
    // (meaning its time hasn't run out according to its own update logic),
    // then the resolution phase is not truly complete.
    if (!effect.markedForRemoval) {
      // SDL_Log("DEBUG: Resolution incomplete due to active Visual Effect.");
      // // Optional log
      return false; // Keep the Resolution_Update phase going
    }
  }
  // --- END ADDED CHECK ---
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
    // --- Load Assets ---

    bool assetsLoaded = loadAllAssets(assetManager);
    if (!assetsLoaded) {
      SDL_LogError(
          SDL_LOG_CATEGORY_APPLICATION,
          "Critical asset loading failed! Check asset_loader function.");
      // Decide how to handle this - maybe exit?
      cleanupSDL(sdlContext);
      return 1; // Exit if essential assets failed
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
  // No need for extern AppState here if it's passed via gameData or globally
  // accessible otherwise

  // --- Poll Events ---
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      running = false;
      return; // Exit immediately on quit
    } else if (event.type == SDL_WINDOWEVENT &&
               event.window.event == SDL_WINDOWEVENT_RESIZED) {
      // Handle Window Resize
      gameData.windowWidth = event.window.data1;
      gameData.windowHeight = event.window.data2;
      SDL_Log("Window resized to %d x %d", gameData.windowWidth,
              gameData.windowHeight);
    }
    // --- Mouse Motion for Targeting ---
    else if (event.type == SDL_MOUSEMOTION) {
      if (currentAppState == AppState::Gameplay &&
          gameData.showTargetingReticle) {
        // Translate screen coords to world coords
        int worldX = event.motion.x + gameData.cameraX;
        int worldY = event.motion.y + gameData.cameraY;
        // Translate world coords to tile coords
        int tileX = worldX / gameData.tileWidth;
        int tileY = worldY / gameData.tileHeight;
        // Clamp to level bounds
        tileX = std::max(0, std::min(gameData.currentLevel.width - 1, tileX));
        tileY = std::max(0, std::min(gameData.currentLevel.height - 1, tileY));
        // Update reticle position if different
        if (tileX != gameData.targetIndicatorX ||
            tileY != gameData.targetIndicatorY) {
          gameData.targetIndicatorX = tileX;
          gameData.targetIndicatorY = tileY;
        }
      }
    }
    // --- Mouse Click for Targeting Confirmation/Cancel ---
    else if (event.type == SDL_MOUSEBUTTONDOWN) {
      if (currentAppState == AppState::Gameplay &&
          gameData.showTargetingReticle) {
        bool actionPlanned = false; // Flag for advancing phase after click
        if (event.button.button == SDL_BUTTON_LEFT) { // Left mouse confirms
          SDL_Log("DEBUG: Left mouse click detected during targeting.");
          if (gameData.currentSpellIndex != -1) {
            // Validate range and mana before confirming
            const Spell &spell =
                gameData.currentGamePlayer.getSpell(gameData.currentSpellIndex);
            int effectiveRange =
                gameData.currentGamePlayer.GetEffectiveSpellRange(
                    gameData.currentSpellIndex);
            int dx = gameData.currentGamePlayer.targetTileX -
                     gameData.targetIndicatorX;
            int dy = gameData.currentGamePlayer.targetTileY -
                     gameData.targetIndicatorY;
            int distSq = dx * dx + dy * dy;

            if (distSq <= effectiveRange * effectiveRange &&
                gameData.currentGamePlayer.canCastSpell(
                    gameData.currentSpellIndex)) {
              // Plan the action
              gameData.playerIntendedAction.type = ActionType::CastSpell;
              gameData.playerIntendedAction.spellIndex =
                  gameData.currentSpellIndex;
              gameData.playerIntendedAction.targetX = gameData.targetIndicatorX;
              gameData.playerIntendedAction.targetY = gameData.targetIndicatorY;
              actionPlanned = true; // Mark action planned
              SDL_Log("Player plans TARGETED CAST spell %d at [%d,%d] "
                      "(Confirmed via Mouse Click).",
                      gameData.currentSpellIndex, gameData.targetIndicatorX,
                      gameData.targetIndicatorY);

            } else {
              SDL_Log("Target out of range or cannot cast. Click ignored.");
            }
          } else {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "Target confirmed via mouse click with no spell selected!");
          }
          // Exit targeting mode if action was planned or if there was an error
          if (actionPlanned || gameData.currentSpellIndex == -1) {
            gameData.showTargetingReticle = false;
            gameData.currentSpellIndex = -1;
            // Reset any held hotkey state just in case targeting was entered
            // via hold
            for (int i = 0; i < GameData::MAX_HOTKEY_SPELLS; ++i) {
              if (gameData.isHotkeyHeld[i]) {
                gameData.isHotkeyHeld[i] = false;
                gameData.hotkeyPressTime[i] = 0;
              }
            }
          }
        } else if (event.button.button ==
                   SDL_BUTTON_RIGHT) { // Right mouse cancels
          SDL_Log("DEBUG: Right mouse click detected during targeting. "
                  "Cancelling.");
          gameData.showTargetingReticle = false;
          gameData.currentSpellIndex = -1;
          // Reset any held hotkey state if cancelling targeting
          for (int i = 0; i < GameData::MAX_HOTKEY_SPELLS; ++i) {
            if (gameData
                    .isHotkeyHeld[i]) { // Only reset if it was actually held
              SDL_Log("DEBUG: Cancelling targeting also resets held state for "
                      "hotkey %d",
                      i + 1);
              gameData.isHotkeyHeld[i] = false;
              gameData.hotkeyPressTime[i] = 0;
            }
          }
        } // End button check

        // Advance phase if action was planned by the click
        if (actionPlanned) {
          // Check if still in correct phase before advancing
          if (gameData.currentPhase == TurnPhase::Planning_PlayerInput) {
            gameData.currentPhase = TurnPhase::Planning_EnemyAI;
            gameData.currentEnemyPlanningIndex = 0;
            gameData.enemyIntendedActions.clear();
            if (!gameData.enemies.empty()) {
              gameData.enemyIntendedActions.resize(gameData.enemies.size());
            }
            SDL_Log("--- Player Planning Complete (Mouse Click). Transitioning "
                    "to Enemy Planning ---");
          } else {
            SDL_LogWarn(
                SDL_LOG_CATEGORY_APPLICATION,
                "Action planned via mouse click, but phase was already %d!",
                (int)gameData.currentPhase);
          }
        }
      } // End if gameplay & targeting
      // Add other non-targeting mouse button handling here if needed (e.g., UI
      // clicks)
    } // End MouseButtonDown

    // --- Keyboard Down ---
    else if (event.type == SDL_KEYDOWN) {
      SDL_Keycode keycode = event.key.keysym.sym;
      if (event.key.repeat == 0) { // Ignore key repeats
        switch (currentAppState) {
        case AppState::MainMenu:
          if (!gameData.isPanning) {
            switch (keycode) {
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
          break; // End MainMenu KeyDown

        case AppState::CharacterSelect:
          if (!gameData.isCharacterSelectFadingIn) {
            switch (keycode) {
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
            case SDLK_RETURN: {
              // --- START OF CHARACTER SELECTION GAME SETUP ---
              CharacterType chosenType = (gameData.selectedCharacterIndex == 0)
                                             ? CharacterType::FemaleMage
                                             : CharacterType::MaleMage;
              gameData.currentGamePlayer = PlayerCharacter(
                  chosenType, 0, 0, gameData.tileWidth, gameData.tileHeight);
              gameData.enemies.clear();
              gameData.activeProjectiles.clear();
              gameData.activeOrbitals.clear();
              gameData.activeEffects.clear();
              gameData.droppedItems.clear();
              gameData.currentLevelIndex = 1;
              Enemy::resetIdCounter();
              std::optional<SDL_Point> pedestalPosOpt;
              gameData.currentLevel = generateLevel(
                  gameData.levelWidth, gameData.levelHeight,
                  gameData.levelMaxRooms, gameData.levelMinRoomSize,
                  gameData.levelMaxRoomSize, gameData.enemies,
                  gameData.tileWidth, gameData.tileHeight, pedestalPosOpt);
              if (pedestalPosOpt.has_value()) {
                gameData.currentPedestal.emplace(pedestalPosOpt.value().x,
                                                 pedestalPosOpt.value().y);
              } else {
                gameData.currentPedestal.reset();
              }
              gameData.levelRooms = gameData.currentLevel.rooms;
              for (auto &enemy : gameData.enemies) {
                enemy.applyFloorScaling(gameData.currentLevelIndex,
                                        gameData.enemyStatScalingPerFloor);
              }
              gameData.occupationGrid.assign(
                  gameData.currentLevel.height,
                  std::vector<bool>(gameData.currentLevel.width, false));
              for (int y = 0; y < gameData.currentLevel.height; ++y)
                for (int x = 0; x < gameData.currentLevel.width; ++x)
                  if (gameData.currentLevel.tiles[y][x] == '#')
                    gameData.occupationGrid[y][x] = true;
              PlayerCharacter &player = gameData.currentGamePlayer;
              player.targetTileX = gameData.currentLevel.startCol;
              player.targetTileY = gameData.currentLevel.startRow;
              player.x = player.targetTileX * gameData.tileWidth +
                         gameData.tileWidth / 2.0f;
              player.y = player.targetTileY * gameData.tileHeight +
                         gameData.tileHeight / 2.0f;
              player.startTileX = player.targetTileX;
              player.startTileY = player.targetTileY;
              player.isMoving = false;
              player.logicalTileX = player.targetTileX;
              player.logicalTileY = player.targetTileY;
              if (isWithinBounds(player.targetTileX, player.targetTileY,
                                 gameData.currentLevel.width,
                                 gameData.currentLevel.height))
                gameData
                    .occupationGrid[player.targetTileY][player.targetTileX] =
                    true;
              for (const auto &enemy : gameData.enemies) {
                if (isWithinBounds(enemy.x, enemy.y,
                                   gameData.currentLevel.width,
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
              if (gameData.currentPedestal.has_value()) {
                int pX = gameData.currentPedestal.value().x;
                int pY = gameData.currentPedestal.value().y;
                if (isWithinBounds(pX, pY, gameData.currentLevel.width,
                                   gameData.currentLevel.height)) {
                  if (gameData.occupationGrid[pY][pX]) {
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                "Pedestal location [%d,%d] was already marked "
                                "occupied!",
                                pX, pY);
                  }
                  gameData.occupationGrid[pY][pX] = true;
                  SDL_Log(
                      "Marked pedestal location [%d,%d] as occupied on grid.",
                      pX, pY);
                }
              }
              gameData.visibilityMap.assign(
                  gameData.currentLevel.height,
                  std::vector<float>(gameData.currentLevel.width, 0.0f));
              updateVisibility(gameData.currentLevel, gameData.levelRooms,
                               player.logicalTileX,
                               player.logicalTileY, // <<< Use logical position
                               gameData.hallwayVisibilityDistance,
                               gameData.visibilityMap);
              gameData.playerIntendedAction = {};
              gameData.enemyIntendedActions.clear();
              gameData.currentPhase = TurnPhase::Planning_PlayerInput;
              gameData.currentMenu = GameMenu::None;
              gameData.showTargetingReticle = false;
              gameData.currentSpellIndex = -1;
              gameData.currentEnemyPlanningIndex = 0;
              gameData.cameraX = 0;
              gameData.cameraY = 0;
              for (int i = 0; i < GameData::MAX_HOTKEY_SPELLS; ++i) {
                gameData.isHotkeyHeld[i] = false;
                gameData.hotkeyPressTime[i] = 0;
              }
              updateLogic(gameData, assets, 0.0f);
              SDL_Log("INFO: [Setup Complete] Player Logical Pos: [%d, %d]",
                      gameData.currentGamePlayer.logicalTileX,
                      gameData.currentGamePlayer.logicalTileY);
              SDL_Log("INFO: [Setup Complete] Player Visual Pos: (%.1f, %.1f)",
                      gameData.currentGamePlayer.x,
                      gameData.currentGamePlayer.y);
              SDL_Log("INFO: [Setup Complete] Initial Camera Pos: (%d, %d)",
                      gameData.cameraX, gameData.cameraY);
              SDL_Log("INFO: [Setup Complete] Visibility at Start Pos: %.2f",
                      (isWithinBounds(gameData.currentGamePlayer.logicalTileX,
                                      gameData.currentGamePlayer.logicalTileY,
                                      gameData.currentLevel.width,
                                      gameData.currentLevel.height) &&
                       gameData.currentGamePlayer.logicalTileY <
                           gameData.visibilityMap.size() &&
                       gameData.currentGamePlayer.logicalTileX <
                           gameData
                               .visibilityMap[gameData.currentGamePlayer
                                                  .logicalTileY]
                               .size())
                          ? gameData.visibilityMap
                                [gameData.currentGamePlayer.logicalTileY]
                                [gameData.currentGamePlayer.logicalTileX]
                          : -1.0f);
              SDL_Log("INFO: [Setup Complete] Visibility Map Dims: %zu x %zu",
                      gameData.visibilityMap.size(),
                      (gameData.visibilityMap.empty()
                           ? 0
                           : gameData.visibilityMap[0].size()));
              SDL_Log("INFO: [Setup Complete] Level Dims: %d x %d",
                      gameData.currentLevel.width,
                      gameData.currentLevel.height);

              // --- END OF CHARACTER SELECTION GAME SETUP ---
              currentAppState = AppState::Gameplay;
              SDL_Log("Character selected, entering Gameplay AppState.");
              gameData.isCharacterSelectFadingIn = false;
              gameData.hasCharacterSelectStartedFading = false;
              gameData.characterSelectAlpha = 0;
            } break; // End RETURN Key case
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
          break; // End CharacterSelect KeyDown

        case AppState::Gameplay:
          // Process gameplay keys only if it's player's turn and not moving
          if (gameData.currentPhase == TurnPhase::Planning_PlayerInput &&
              !gameData.currentGamePlayer.isMoving) {
            int hotkeySpellIndex = -1;
            if (keycode >= SDLK_1 && keycode <= SDLK_5) {
              hotkeySpellIndex = keycode - SDLK_1;
            }

            // --- Hotkey Press Start (Record Time) ---
            if (hotkeySpellIndex != -1) {
              if (hotkeySpellIndex < GameData::MAX_HOTKEY_SPELLS &&
                  hotkeySpellIndex <
                      gameData.currentGamePlayer.knownSpells.size()) {
                if (!gameData
                         .isHotkeyHeld[hotkeySpellIndex]) { // Only record if
                                                            // not already held
                  SDL_Log("DEBUG: Hotkey %d KeyDown detected.",
                          hotkeySpellIndex + 1);
                  gameData.hotkeyPressTime[hotkeySpellIndex] = SDL_GetTicks();
                  gameData.isHotkeyHeld[hotkeySpellIndex] = true;
                } // else: Key already held, ignore repeat KeyDown event
              } else {
                SDL_Log("DEBUG: Hotkey %d pressed, but invalid spell slot.",
                        hotkeySpellIndex + 1);
              }
            }
            // --- Movement Key Down (Set Flags) ---
            // Process these even if targeting, but movement itself will be
            // blocked later if targeting
            else { // Not a hotkey
              switch (keycode) {
              case SDLK_UP:
              case SDLK_w:
              case SDLK_KP_8:
                gameData.isMoveUpHeld = true;
                break;
              case SDLK_DOWN:
              case SDLK_s:
              case SDLK_KP_2:
                gameData.isMoveDownHeld = true;
                break;
              case SDLK_LEFT:
              case SDLK_a:
              case SDLK_KP_4:
                gameData.isMoveLeftHeld = true;
                break;
              case SDLK_RIGHT:
              case SDLK_d:
              case SDLK_KP_6:
                gameData.isMoveRightHeld = true;
                break;
              // Diagonal Aliases (set corresponding orthogonal flags)
              case SDLK_KP_7:
                gameData.isMoveUpHeld = true;
                gameData.isMoveLeftHeld = true;
                break;
              case SDLK_KP_9:
                gameData.isMoveUpHeld = true;
                gameData.isMoveRightHeld = true;
                break;
              case SDLK_KP_1:
                gameData.isMoveDownHeld = true;
                gameData.isMoveLeftHeld = true;
                break;
              case SDLK_KP_3:
                gameData.isMoveDownHeld = true;
                gameData.isMoveRightHeld = true;
                break;
              default: // Handle non-movement keys only if NOT targeting
                if (!gameData.showTargetingReticle) {
                  bool actionPlanned =
                      false; // Only for immediate actions like Wait/Interact
                  switch (keycode) {
                  case SDLK_KP_5: // Wait Key
                    if (gameData.currentPhase ==
                        TurnPhase::Planning_PlayerInput) {
                      gameData.playerIntendedAction.type = ActionType::Wait;
                      actionPlanned = true;
                      SDL_Log("Player plans WAIT.");
                    }
                    break;
                  case SDLK_e: // Interact Key
                    if (gameData.currentPedestal.has_value() &&
                        gameData.currentPedestal.value().isActive) {
                      const RunePedestal &p = gameData.currentPedestal.value();
                      int pX = gameData.currentGamePlayer.targetTileX;
                      int pY = gameData.currentGamePlayer.targetTileY;
                      if (std::abs(pX - p.x) + std::abs(pY - p.y) <= 1) {
                        gameData.playerIntendedAction.type =
                            ActionType::Interact;
                        gameData.playerIntendedAction.targetX = p.x;
                        gameData.playerIntendedAction.targetY = p.y;
                        actionPlanned = true;
                        SDL_Log("Player plans INTERACT with pedestal.");
                      } else {
                        SDL_Log("Pedestal not adjacent.");
                      }
                    } else {
                      SDL_Log("No active pedestal.");
                    }
                    break;
                  case SDLK_c: /* Toggle Spell Menu */
                    gameData.currentMenu =
                        (gameData.currentMenu == GameMenu::SpellMenu)
                            ? GameMenu::None
                            : GameMenu::SpellMenu;
                    gameData.spellSelectIndex = 0;
                    break;
                  case SDLK_i: /* Toggle Char Sheet */
                    gameData.currentMenu =
                        (gameData.currentMenu == GameMenu::CharacterSheet)
                            ? GameMenu::None
                            : GameMenu::CharacterSheet;
                    break;
                  case SDLK_ESCAPE: /* Close Menus */
                    if (gameData.currentMenu != GameMenu::None) {
                      gameData.currentMenu = GameMenu::None;
                      gameData.spellSelectIndex = 0;
                    }
                    break;
                  }
                  // Advance phase only for Wait/Interact planned here
                  if (actionPlanned && gameData.currentPhase ==
                                           TurnPhase::Planning_PlayerInput) {
                    gameData.currentPhase = TurnPhase::Planning_EnemyAI;
                    gameData.currentEnemyPlanningIndex = 0;
                    gameData.enemyIntendedActions.clear();
                    if (!gameData.enemies.empty())
                      gameData.enemyIntendedActions.resize(
                          gameData.enemies.size());
                    SDL_Log("--- Player Planning Complete (Wait/Interact). "
                            "Transitioning to Enemy Planning ---");
                  }
                } // End if not targeting check for other keys
                  // Handle targeting input (reticle move / escape cancel) if
                  // targeting
                else { // gameData.showTargetingReticle is true
                  int targetMoveX = 0;
                  int targetMoveY = 0;
                  bool cancelTarget = false;
                  switch (keycode) {
                  // Reticle Movement
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
                  // Cancel Targeting via Escape
                  case SDLK_ESCAPE:
                    cancelTarget = true;
                    break;
                  }
                  // Apply Reticle Movement
                  if (targetMoveX != 0 || targetMoveY != 0) {
                    gameData.targetIndicatorX += targetMoveX;
                    gameData.targetIndicatorY += targetMoveY;
                    gameData.targetIndicatorX =
                        std::max(0, std::min(gameData.currentLevel.width - 1,
                                             gameData.targetIndicatorX));
                    gameData.targetIndicatorY =
                        std::max(0, std::min(gameData.currentLevel.height - 1,
                                             gameData.targetIndicatorY));
                  }
                  // Apply Cancellation via Escape
                  if (cancelTarget) {
                    SDL_Log("Targeting cancelled via Escape.");
                    gameData.showTargetingReticle = false;
                    gameData.currentSpellIndex = -1;
                    for (int i = 0; i < GameData::MAX_HOTKEY_SPELLS; ++i) {
                      if (gameData.isHotkeyHeld[i]) {
                        gameData.isHotkeyHeld[i] = false;
                        gameData.hotkeyPressTime[i] = 0;
                      }
                    }
                  }
                } // End targeting input handling
                break; // Break from default case (non-hotkey)
              } // End switch(keycode)
            } // End else (not a hotkey)
          } // End Gameplay Player Turn KeyDown block
          break; // End Gameplay KeyDown

        case AppState::Quitting:
          break;
        } // End AppState switch
      } // End if(event.key.repeat == 0)
    } // End KEYDOWN Event

    // --- Key Up Event Handling ---
    else if (event.type == SDL_KEYUP) {
      SDL_Keycode keycode = event.key.keysym.sym;

      // --- Movement Key Up (Clear Flags) ---
      // This happens regardless of game state/phase
      switch (keycode) {
      case SDLK_UP:
      case SDLK_w:
      case SDLK_KP_8:
        gameData.isMoveUpHeld = false;
        break;
      case SDLK_DOWN:
      case SDLK_s:
      case SDLK_KP_2:
        gameData.isMoveDownHeld = false;
        break;
      case SDLK_LEFT:
      case SDLK_a:
      case SDLK_KP_4:
        gameData.isMoveLeftHeld = false;
        break;
      case SDLK_RIGHT:
      case SDLK_d:
      case SDLK_KP_6:
        gameData.isMoveRightHeld = false;
        break;
      case SDLK_KP_7:
        gameData.isMoveUpHeld = false;
        gameData.isMoveLeftHeld = false;
        break;
      case SDLK_KP_9:
        gameData.isMoveUpHeld = false;
        gameData.isMoveRightHeld = false;
        break;
      case SDLK_KP_1:
        gameData.isMoveDownHeld = false;
        gameData.isMoveLeftHeld = false;
        break;
      case SDLK_KP_3:
        gameData.isMoveDownHeld = false;
        gameData.isMoveRightHeld = false;
        break;
      }

      // --- Hotkey Key Up (Tap/Hold Logic - Only in Gameplay Planning Phase)
      // ---
      if (currentAppState == AppState::Gameplay &&
          gameData.currentPhase == TurnPhase::Planning_PlayerInput) {
        int hotkeySpellIndex = -1;
        if (keycode >= SDLK_1 && keycode <= SDLK_5)
          hotkeySpellIndex = keycode - SDLK_1;

        if (hotkeySpellIndex != -1 &&
            hotkeySpellIndex < GameData::MAX_HOTKEY_SPELLS) {
          if (gameData.isHotkeyHeld[hotkeySpellIndex]) { // Was this key
                                                         // actually held?
            Uint32 holdDuration =
                SDL_GetTicks() - gameData.hotkeyPressTime[hotkeySpellIndex];
            bool actionPlanned = false; // Flag specific to KeyUp logic

            // --- TAP LOGIC ---
            if (holdDuration < gameData.HOLD_THRESHOLD_MS) {
              SDL_Log("DEBUG: KeyUp - Tap detected (Duration: %u ms) for "
                      "Hotkey %d.",
                      holdDuration, hotkeySpellIndex + 1);
              if (gameData.showTargetingReticle &&
                  gameData.currentSpellIndex != hotkeySpellIndex) {
                gameData.showTargetingReticle = false;
                gameData.currentSpellIndex = -1;
                SDL_Log("... Cancelling previous target mode before Tap Cast.");
              }
              if (!gameData.showTargetingReticle) {
                if (hotkeySpellIndex <
                    gameData.currentGamePlayer.knownSpells.size()) {
                  const Spell &spell =
                      gameData.currentGamePlayer.getSpell(hotkeySpellIndex);
                  bool canCast =
                      gameData.currentGamePlayer.canCastSpell(hotkeySpellIndex);
                  SDL_Log(
                      "... Checking Tap Cast for Spell: '%s'. Mana Check: %s",
                      spell.name.c_str(), canCast ? "OK" : "FAIL");
                  if (canCast) {
                    if (spell.targetType ==
                        SpellTargetType::Self) { /* Plan Self Cast */
                      gameData.playerIntendedAction.type =
                          ActionType::CastSpell;
                      gameData.playerIntendedAction.spellIndex =
                          hotkeySpellIndex;
                      actionPlanned = true;
                      SDL_Log("... Planning SELF CAST via Tap.");
                    } else if (spell.targetType ==
                               SpellTargetType::Enemy) { /* Find Nearest, Plan
                                                            Cast */
                      SDL_Point targetPos = {-1, -1};
                      if (findNearestValidTarget(gameData, hotkeySpellIndex,
                                                 targetPos)) {
                        gameData.playerIntendedAction.type =
                            ActionType::CastSpell;
                        gameData.playerIntendedAction.spellIndex =
                            hotkeySpellIndex;
                        gameData.playerIntendedAction.targetX = targetPos.x;
                        gameData.playerIntendedAction.targetY = targetPos.y;
                        actionPlanned = true;
                        SDL_Log("... Planning AUTO CAST via Tap at nearest "
                                "enemy [%d,%d].",
                                targetPos.x, targetPos.y);
                      } else {
                        SDL_Log("... Tap cast FAILED: No valid enemy target.");
                      }
                    } else {
                      SDL_Log(
                          "... Tap ignored: Spell requires manual targeting.");
                    }
                  }
                } else {
                  SDL_Log("... Tap cast failed: Invalid spell index.");
                }
              } else {
                SDL_Log("... Tap ignored: Already targeting this spell.");
              }
            }
            // --- HOLD RELEASE LOGIC ---
            else { // holdDuration >= threshold
              SDL_Log("DEBUG: KeyUp - Hold Release detected (Duration: %u ms) "
                      "for Hotkey %d.",
                      holdDuration, hotkeySpellIndex + 1);
              if (gameData.showTargetingReticle &&
                  gameData.currentSpellIndex == hotkeySpellIndex) {
                const Spell &spell =
                    gameData.currentGamePlayer.getSpell(hotkeySpellIndex);
                int range = gameData.currentGamePlayer.GetEffectiveSpellRange(
                    hotkeySpellIndex);
                int dx = gameData.currentGamePlayer.targetTileX -
                         gameData.targetIndicatorX;
                int dy = gameData.currentGamePlayer.targetTileY -
                         gameData.targetIndicatorY;
                int distSq = dx * dx + dy * dy;
                bool canCast =
                    gameData.currentGamePlayer.canCastSpell(hotkeySpellIndex);
                bool inRange = distSq <= range * range;
                SDL_Log("... Confirming target from hold release. Validation: "
                        "InRange=%s, CanCast=%s",
                        inRange ? "Yes" : "No", canCast ? "Yes" : "No");
                if (inRange && canCast) { // Validate target
                  gameData.playerIntendedAction.type = ActionType::CastSpell;
                  gameData.playerIntendedAction.spellIndex = hotkeySpellIndex;
                  gameData.playerIntendedAction.targetX =
                      gameData.targetIndicatorX;
                  gameData.playerIntendedAction.targetY =
                      gameData.targetIndicatorY;
                  actionPlanned = true;
                  SDL_Log(
                      "... Planning TARGETED CAST via Hold Release at [%d,%d].",
                      gameData.targetIndicatorX, gameData.targetIndicatorY);
                } else {
                  SDL_Log(
                      "... Hold Release target [%d,%d] INVALID. Cancelling.",
                      gameData.targetIndicatorX, gameData.targetIndicatorY);
                }
                gameData.showTargetingReticle = false;
                gameData.currentSpellIndex = -1; // Exit targeting mode
              } else {
                SDL_Log("... Hold Release ignored: Not targeting this spell.");
                if (gameData.currentSpellIndex == hotkeySpellIndex) {
                  gameData.showTargetingReticle = false;
                  gameData.currentSpellIndex = -1;
                }
              } // Cleanup inconsistent state
            } // End Hold Release Logic

            // Advance Phase if Action Planned During KeyUp
            if (actionPlanned) {
              if (gameData.currentPhase == TurnPhase::Planning_PlayerInput) {
                gameData.currentPhase = TurnPhase::Planning_EnemyAI;
                gameData.currentEnemyPlanningIndex = 0;
                gameData.enemyIntendedActions.clear();
                if (!gameData.enemies.empty())
                  gameData.enemyIntendedActions.resize(gameData.enemies.size());
                SDL_Log("--- Player Planning Complete (Hotkey Up). "
                        "Transitioning to Enemy Planning ---");
              } else {
                SDL_LogWarn(
                    SDL_LOG_CATEGORY_APPLICATION,
                    "Action planned on KeyUp, but phase was already %d!",
                    (int)gameData.currentPhase);
              }
              gameData.showTargetingReticle = false;
              gameData.currentSpellIndex = -1; // Ensure targeting is off
            }
            // Reset Hold State for this Key
            gameData.isHotkeyHeld[hotkeySpellIndex] = false;
            gameData.hotkeyPressTime[hotkeySpellIndex] = 0;
            SDL_Log("... Reset hold state for Hotkey %d.",
                    hotkeySpellIndex + 1);
          } // End if isHotkeyHeld
        } // End if hotkeySpellIndex valid
      } // End Gameplay Player Turn KeyUp block
    } // End KEYUP Event block

  } // End SDL_PollEvent loop

  // --- Post-Event Loop Processing ---

  // --- Check for Entering Targeting Mode via Sustained Hold ---
  if (currentAppState == AppState::Gameplay &&
      gameData.currentPhase == TurnPhase::Planning_PlayerInput) {
    if (!gameData.showTargetingReticle) { // Only check if not already targeting
      for (int i = 0; i < GameData::MAX_HOTKEY_SPELLS; ++i) {
        if (gameData.isHotkeyHeld[i] && gameData.hotkeyPressTime[i] > 0) {
          Uint32 currentHoldDuration =
              SDL_GetTicks() - gameData.hotkeyPressTime[i];
          if (currentHoldDuration >= gameData.HOLD_THRESHOLD_MS) {
            if (i < gameData.currentGamePlayer.knownSpells.size()) {
              const Spell &spell = gameData.currentGamePlayer.getSpell(i);
              if (spell.targetType == SpellTargetType::Enemy ||
                  spell.targetType == SpellTargetType::Tile ||
                  spell.targetType == SpellTargetType::Area) {
                if (gameData.currentGamePlayer.canCastSpell(i)) {
                  SDL_Log("DEBUG: Hold Threshold reached for Hotkey %d. "
                          "Entering Targeting.",
                          i + 1);
                  gameData.currentSpellIndex = i;
                  gameData.showTargetingReticle = true;
                  SDL_Point targetPos = {-1, -1};
                  bool found = false;
                  if (spell.targetType == SpellTargetType::Enemy)
                    found = findNearestValidTarget(gameData, i, targetPos);
                  if (found) {
                    gameData.targetIndicatorX = targetPos.x;
                    gameData.targetIndicatorY = targetPos.y;
                    SDL_Log("... Init reticle at nearest enemy [%d,%d].",
                            targetPos.x, targetPos.y);
                  } else {
                    gameData.targetIndicatorX =
                        gameData.currentGamePlayer.targetTileX;
                    gameData.targetIndicatorY =
                        gameData.currentGamePlayer.targetTileY;
                    SDL_Log("... Init reticle at player [%d,%d].",
                            gameData.targetIndicatorX,
                            gameData.targetIndicatorY);
                  }
                  break; // Enter targeting for only one key per frame
                } else { /* Log cannot enter targeting, no mana */
                }
              } // End if target type requires targeting
            } // End if valid spell index
          } // End if hold duration sufficient
        } // End if key is held
        if (gameData.showTargetingReticle)
          break; // Exit loop once targeting mode is entered
      } // End for loop checking held keys
    } // End if not already targeting
  } // End Gameplay Player Turn check

  // --- Process Held Movement Keys (Plan Move if player can act) ---
  if (currentAppState == AppState::Gameplay &&
      gameData.currentPhase == TurnPhase::Planning_PlayerInput &&
      !gameData.currentGamePlayer
           .isMoving && // Can only plan move if not already moving
      !gameData.showTargetingReticle) // Don't move while targeting
  {
    int moveX = 0;
    int moveY = 0;
    if (gameData.isMoveUpHeld)
      moveY = -1;
    if (gameData.isMoveDownHeld)
      moveY = (moveY == -1) ? 0 : 1;
    if (gameData.isMoveLeftHeld)
      moveX = -1;
    if (gameData.isMoveRightHeld)
      moveX = (moveX == -1) ? 0 : 1;

    if (moveX != 0 || moveY != 0) { // If any movement key is held
      int newPlayerTargetX = gameData.currentGamePlayer.targetTileX + moveX;
      int newPlayerTargetY = gameData.currentGamePlayer.targetTileY + moveY;
      bool actionPlanned = false;

      // Validate Move
      if (isWithinBounds(newPlayerTargetX, newPlayerTargetY,
                         gameData.currentLevel.width,
                         gameData.currentLevel.height) &&
          gameData.currentLevel.tiles[newPlayerTargetY][newPlayerTargetX] !=
              '#' &&
          !gameData.occupationGrid[newPlayerTargetY][newPlayerTargetX]) {
        // Plan the Move Action
        gameData.playerIntendedAction.type = ActionType::Move;
        gameData.playerIntendedAction.targetX = newPlayerTargetX;
        gameData.playerIntendedAction.targetY = newPlayerTargetY;
        // Update facing direction based on planned move
        if (moveX > 0)
          gameData.currentGamePlayer.currentFacingDirection =
              PlayerCharacter::FacingDirection::Right;
        else if (moveX < 0)
          gameData.currentGamePlayer.currentFacingDirection =
              PlayerCharacter::FacingDirection::Left;
        // No change for vertical only moves
        actionPlanned = true;
        SDL_Log("DEBUG: Planning Move via Held Keys to [%d,%d].",
                newPlayerTargetX, newPlayerTargetY);
      } else { /* Optional: Log blocked move */
      }

      // Advance phase *if* a move was successfully planned
      if (actionPlanned) {
        if (gameData.currentPhase ==
            TurnPhase::Planning_PlayerInput) { // Double check phase
          gameData.currentPhase = TurnPhase::Planning_EnemyAI;
          gameData.currentEnemyPlanningIndex = 0;
          gameData.enemyIntendedActions.clear();
          if (!gameData.enemies.empty())
            gameData.enemyIntendedActions.resize(gameData.enemies.size());
          SDL_Log("--- Player Planning Complete (Held Move). Transitioning to "
                  "Enemy Planning ---");
        }
      }
    } // End if moveX != 0 || moveY != 0
  } // End Movement Key Processing block

} // End handleEvents Function

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

          // *** ADD STATUS CHECKS BEFORE EXECUTING PLANNED ACTION ***
          bool canAct = true;
          if (enemy.HasStatusEffect(StatusEffectType::Stunned)) {
            SDL_Log("... Enemy %d action prevented by Stunned.", enemy.id);
            canAct = false;
          }

          // Check specific action types against statuses
          if (canAct) {
            switch (eAction.type) {
            case ActionType::Move:
              if (enemy.HasStatusEffect(StatusEffectType::Immobilised)) {
                SDL_Log("... Enemy %d MOVE prevented by Immobilised.",
                        enemy.id);
                canAct = false;
              }
              break;
            case ActionType::Attack:
              if (enemy.HasStatusEffect(StatusEffectType::Blinded)) {
                SDL_Log("... Enemy %d ATTACK prevented by Blinded.", enemy.id);
                canAct = false;
              }
              break;
            case ActionType::CastSpell:
              if (enemy.HasStatusEffect(StatusEffectType::Silenced)) {
                SDL_Log("... Enemy %d CAST prevented by Silenced.", enemy.id);
                canAct = false;
              }
              break;
            default: // Wait, None don't need status checks here
              break;
            }
          }
          // *** END STATUS CHECKS ***

          // Now resolve the action for this specific, living enemy

          // --- Execute Action ONLY if canAct is true ---
          if (canAct) {
            switch (eAction.type) {
            case ActionType::Move: {
              // Check if target tile is valid and unoccupied
              if (isWithinBounds(eAction.targetX, eAction.targetY,
                                 gameData.currentLevel.width,
                                 gameData.currentLevel.height) &&
                  !gameData.occupationGrid[eAction.targetY][eAction.targetX]) {
                // Update occupation grid: Clear old, set new
                if (isWithinBounds(enemy.x, enemy.y,
                                   gameData.currentLevel.width,
                                   gameData.currentLevel.height)) {
                  gameData.occupationGrid[enemy.y][enemy.x] = false;
                  SDL_Log("DEBUG: Enemy %d cleared occupation at [%d,%d]",
                          enemy.id, enemy.x, enemy.y);
                }
                gameData.occupationGrid[eAction.targetY][eAction.targetX] =
                    true;
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
              // <<< USE PLAYER'S LOGICAL POSITION FOR CHECK >>>
              bool playerAtTarget = (player.logicalTileX == eAction.targetX &&
                                     player.logicalTileY == eAction.targetY);
              SDL_Log("... Checking Player Position: Player Logical is at "
                      "[%d,%d]. Target was [%d,%d]. Match: %s",
                      player.logicalTileX,
                      player.logicalTileY, // Use logical pos here
                      eAction.targetX, eAction.targetY,
                      playerAtTarget ? "Yes" : "No");
              // <<< END CHANGE >>>

              if (playerAtTarget) {
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
              // 1. Apply Damage
              targetEnemy->takeDamage(proj.damage); // Log is inside takeDamage

              // 2. Apply Status Effect (if any)
              // Retrieve the source spell definition using the index stored in
              // the projectile
              if (proj.sourceSpellIndex >= 0 &&
                  proj.sourceSpellIndex <
                      gameData.currentGamePlayer.knownSpells.size()) {
                const Spell &sourceSpell =
                    gameData.currentGamePlayer.getSpell(proj.sourceSpellIndex);

                // Check if the spell applies a status effect
                if (sourceSpell.statusEffectApplied != StatusEffectType::None &&
                    sourceSpell.statusEffectDuration > 0) {
                  // Optional: Check chance here if implemented later
                  // if ((rand() % 100) < sourceSpell.statusEffectChance) {
                  SDL_Log("... Applying Status Effect %d for %d turns from "
                          "spell '%s'.",
                          (int)sourceSpell.statusEffectApplied,
                          sourceSpell.statusEffectDuration,
                          sourceSpell.name.c_str());
                  targetEnemy->AddStatusEffect(
                      sourceSpell.statusEffectApplied,
                      sourceSpell.statusEffectDuration);
                  // } else { SDL_Log("... Status effect chance failed."); }
                } else {
                  SDL_Log(
                      "... Source spell '%s' does not apply a status effect.",
                      sourceSpell.name.c_str());
                }
              } else {
                SDL_LogWarn(
                    SDL_LOG_CATEGORY_APPLICATION,
                    "... Projectile hit, but sourceSpellIndex %d is invalid!",
                    proj.sourceSpellIndex);
              }
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
        if (orbital.update(deltaTime, gameData, assets)) {
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

      // <<< ADD VISUAL EFFECT UPDATE LOOP >>>
      for (auto &effect : gameData.activeEffects) {
        effect.update(deltaTime);
      }
      // Remove completed effects
      gameData.activeEffects.erase(
          std::remove_if(
              gameData.activeEffects.begin(), gameData.activeEffects.end(),
              [](const VisualEffect &e) { return e.markedForRemoval; }),
          gameData.activeEffects.end());
      // <<< END VISUAL EFFECT UPDATE LOOP >>>
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

      // --- Apply Enemy Effects ---
      for (auto &enemy : gameData.enemies) {
        if (enemy.health > 0) {                // Only update living enemies
          enemy.UpdateStatusEffectDurations(); // Tick down enemy statuses
          // Add other enemy turn-end effects here (e.g., poison damage)
        }
      }
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

    // +++ ADD LOGGING +++
    /*SDL_Log("DEBUG: [RenderScene Tiles] Cam=(%d,%d) Logical=(%d,%d)
Tile=(%d,%d) Level=(%d,%d)", gameData.cameraX, gameData.cameraY,
gameData.logicalWidth, gameData.logicalHeight, gameData.tileWidth,
gameData.tileHeight, gameData.currentLevel.width, gameData.currentLevel.height);
SDL_Log("DEBUG: [RenderScene Tiles] Calculated Render Bounds: X=[%d -> %d),
Y=[%d -> %d)", startTileX, endTileX, startTileY, endTileY);*/
    // +++++++++++++++++++
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

  // <<< ADD VISUAL EFFECT RENDERING >>>
  // Render *after* tiles/items/pedestal, but potentially *before* or *after*
  // entities Rendering before entities makes effects appear "on the ground"
  // Rendering after entities makes effects appear "over" them
  // Let's render them *before* entities for now (ground frost effect)
  for (const auto &effect : gameData.activeEffects) {
    // Optional: Add visibility check based on effect's center point if desired
    effect.render(gameData.renderer, assets, gameData.cameraX,
                  gameData.cameraY);
  }
  // <<< END VISUAL EFFECT RENDERING >>>

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
    playerRect.w = gameData.tileWidth;  // Example fixed width
    playerRect.h = gameData.tileHeight; // Example fixed height
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
  for (const auto &orbital : gameData.activeOrbitals) {
    // Pass necessary arguments to orbital's render method
    orbital.render(gameData.renderer, assets, gameData.cameraX,
                   gameData.cameraY);
  }
  // --- END Orbital Rendering ---

  // --- Render Projectiles ---
  for (const auto &proj : gameData.activeProjectiles) {
    if (proj.isActive) {
      proj.render(gameData.renderer, gameData.cameraX, gameData.cameraY);
    }
  }

  // --- Render Targeting Reticle ---
  // --- Render Targeting Reticle and AoE Indicator ---
  if (gameData.currentPhase == TurnPhase::Planning_PlayerInput &&
      gameData.showTargetingReticle) {
    SDL_Texture *reticleTexture = assets.getTexture("reticle");
    if (!reticleTexture) {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "Reticle texture 'reticle' not found!");
      // Optionally handle error differently, like drawing only rectangles
    }

    int centralTileX = gameData.targetIndicatorX;
    int centralTileY = gameData.targetIndicatorY;
    int spellRadius = 0; // Default to 0 for non-AoE spells

    // Get current spell details to check for AoE radius
    const Spell *currentSpellPtr = nullptr;
    if (gameData.currentSpellIndex >= 0 &&
        gameData.currentSpellIndex <
            gameData.currentGamePlayer.knownSpells.size()) {
      currentSpellPtr =
          &gameData.currentGamePlayer.knownSpells[gameData.currentSpellIndex];
      spellRadius = currentSpellPtr->areaOfEffectRadius;
    } else {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "Targeting active but currentSpellIndex (%d) is invalid!",
                   gameData.currentSpellIndex);
      // Handle error - maybe force targeting off?
      gameData.showTargetingReticle = false;
      return; // Exit rendering early if spell index is bad
    }

    // --- Determine Color/Style based on Range ---
    bool isInRange = false;
    Uint8 baseAlpha = 180; // Base transparency for indicators
    SDL_Color primaryColorMod = {255, 255, 255};    // White default (in range)
    SDL_Color secondaryColorMod = {200, 200, 255};  // Lighter blue for AoE
    SDL_Color outOfRangeColorMod = {255, 100, 100}; // Red tint (out of range)

    if (currentSpellPtr) {
      int dx_range = gameData.currentGamePlayer.targetTileX - centralTileX;
      int dy_range = gameData.currentGamePlayer.targetTileY - centralTileY;
      int distSq_range = dx_range * dx_range + dy_range * dy_range;
      int effectiveRange = gameData.currentGamePlayer.GetEffectiveSpellRange(
          gameData.currentSpellIndex);
      isInRange = (distSq_range <= effectiveRange * effectiveRange);

      if (!isInRange) {
        primaryColorMod = outOfRangeColorMod;
        secondaryColorMod =
            outOfRangeColorMod; // Make AoE red too if center is OOR
        baseAlpha = 100;        // Dimmer if out of range
      }
    }
    // --- End Color/Style Determination ---

    // --- Loop to draw indicators (center + AoE) ---
    // Loop bounds cover the full potential AoE square
    int loopRadius = spellRadius;
    for (int dx = -loopRadius; dx <= loopRadius; ++dx) {
      for (int dy = -loopRadius; dy <= loopRadius; ++dy) {

        // --- Determine if this tile is the Center or Secondary ---
        bool isCenterTile = (dx == 0 && dy == 0);
        // Skip drawing secondary indicators if the spell has no AoE radius
        if (!isCenterTile && spellRadius <= 0) {
          continue;
        }

        // Optional: Check for circular AoE distance if desired
        // if (!isCenterTile && (dx * dx + dy * dy > spellRadius * spellRadius))
        // {
        //     continue; // Skip tiles outside circle radius
        // }

        int currentTileX = centralTileX + dx;
        int currentTileY = centralTileY + dy;

        // --- Check Visibility and Bounds ---
        if (!isWithinBounds(currentTileX, currentTileY,
                            gameData.currentLevel.width,
                            gameData.currentLevel.height)) {
          continue; // Skip tiles outside map
        }
        float visibility = 0.0f;
        if (currentTileY < gameData.visibilityMap.size() &&
            currentTileX < gameData.visibilityMap[currentTileY].size()) {
          visibility = gameData.visibilityMap[currentTileY][currentTileX];
        }
        if (visibility <= 0.0f) {
          continue; // Skip tiles not visible
        }
        // --- End Visibility/Bounds Check ---

        // --- Calculate Position and Render ---
        SDL_Rect indicatorRect = {
            (currentTileX * gameData.tileWidth) - gameData.cameraX,
            (currentTileY * gameData.tileHeight) - gameData.cameraY,
            gameData.tileWidth, gameData.tileHeight};

        Uint8 finalAlpha = static_cast<Uint8>(
            baseAlpha * visibility); // Modulate base alpha by visibility
        SDL_Color currentColorMod =
            isCenterTile ? primaryColorMod : secondaryColorMod;

        if (reticleTexture) {
          // --- Render using Texture ---
          SDL_SetTextureColorMod(reticleTexture, currentColorMod.r,
                                 currentColorMod.g, currentColorMod.b);
          SDL_SetTextureAlphaMod(reticleTexture, finalAlpha);
          SDL_SetTextureBlendMode(reticleTexture, SDL_BLENDMODE_BLEND);
          SDL_RenderCopy(gameData.renderer, reticleTexture, nullptr,
                         &indicatorRect);
          // Reset texture mods for next loop iteration or rendering pass
          SDL_SetTextureColorMod(reticleTexture, 255, 255, 255);
          SDL_SetTextureAlphaMod(reticleTexture, 255);
        } else {
          // --- Render using Rectangle Fallback ---
          SDL_SetRenderDrawColor(gameData.renderer, currentColorMod.r,
                                 currentColorMod.g, currentColorMod.b,
                                 finalAlpha);
          SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_BLEND);
          SDL_RenderDrawRect(gameData.renderer, &indicatorRect); // Draw outline
          SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_NONE);
        }
      } // End dy loop
    } // End dx loop
  } // End Targeting Reticle check

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
