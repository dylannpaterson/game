// main.cpp
#include <algorithm> // For std::remove_if
#include <cmath>     // For std::abs, std::max, std::min
#include <cstdlib>   // For rand() and srand()
#include <ctime>     // For time()
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "asset_manager.h"
#include "character.h" // Includes PlayerCharacter definition (ensure it includes spell.h)
#include "character_select.h"
#include "enemy.h" // Includes Enemy definition (ensure it includes takeDamage declaration)
#include "game_data.h"
#include "level.h"
#include "menu.h"
#include "projectile.h"
#include "ui.h"
#include "utils.h"
#include "visibility.h"
// No need to include spell.h here if character.h already does

#ifdef _WIN32
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "SDL2_ttf.lib")
#pragma comment(lib, "SDL2_image.lib")
// Remove duplicate SDL2_image.lib if present
#endif

// --- Forward Declarations for Game Loop Functions ---
void handleEvents(GameData &gameData, AssetManager &assets, bool &running);
void updateLogic(GameData &gameData, float deltaTime);
void renderScene(GameData &gameData, AssetManager &assets);

// --- Main Function ---
int main(int argc, char *argv[]) {
  srand(static_cast<unsigned int>(time(0))); // Seed random number generator

  GameData gameData; // Instantiate the single GameData object

  // --- SDL Initialization ---
  SDL_Context sdlContext =
      initializeSDL(gameData.windowWidth, gameData.windowHeight);
  if (!sdlContext.window || !sdlContext.renderer) {
    cleanupSDL(sdlContext); // Still use context for SDL cleanup
    return 1;
  }

  // Assign essential context to gameData
  gameData.renderer = sdlContext.renderer;

  // --- Create Asset Manager ---
  AssetManager assetManager(gameData.renderer); // <-- CREATE MANAGER

  // --- Initialize Player Character Correctly ---
  // Use the tileWidth/Height now stored in gameData
  gameData.currentGamePlayer =
      PlayerCharacter(CharacterType::FemaleMage, 100, 100, 150, 150, 1, 0, 0,
                      gameData.tileWidth, gameData.tileHeight);

  // --- Load Assets VIA MANAGER ---
  if (!assetManager.loadTexture(
          "splash", "../assets/splash/splash.png")) { /* Handle error */
  }
  if (!assetManager.loadTexture(
          "start_tile",
          "../assets/sprites/start_tile.png")) { /* Handle error */
  }
  if (!assetManager.loadTexture(
          "exit_tile",
          "../assets/sprites/exit_tile.png")) { /* Handle error */
  }
  if (!assetManager.loadTexture(
          "reticle",
          "../assets/sprites/target_reticle.png")) { /* Handle error */
  }
  if (!assetManager.loadTexture(
          "fireball",
          "../assets/sprites/fireball.PNG")) { /* Handle error */
  }
  if (!assetManager.loadTexture(
          "fireball_icon",
          "../assets/sprites/fireball_icon.PNG")) { /* Handle error */
  }
  if (!assetManager.loadTexture(
          "minor_heal_icon",
          "../assets/sprites/minor_heal_icon.PNG")) { /* Handle error */
  }
  if (!assetManager.loadFont("main_font", "../assets/fonts/LUMOS.TTF",
                             36)) { /* Handle error */
  } // Load font here
  if (!assetManager.loadFont("spellbar_font", "../assets/fonts/LUMOS.TTF",
                             18)) { /* Handle error */
  } // Load font here

  // Set blend modes using the manager
  SDL_Texture *reticleTex = assetManager.getTexture("reticle");
  if (reticleTex)
    SDL_SetTextureBlendMode(reticleTex, SDL_BLENDMODE_BLEND);
  SDL_Texture *splashTex = assetManager.getTexture("splash");
  if (splashTex)
    SDL_SetTextureBlendMode(splashTex, SDL_BLENDMODE_BLEND);

  bool running = true;
  Uint32 lastFrameTime = SDL_GetTicks();

  // --- Main Game Loop ---
  while (running) {
    Uint32 currentFrameTime = SDL_GetTicks();
    float deltaTime =
        static_cast<float>(currentFrameTime - lastFrameTime) / 1000.0f;
    // Prevent huge deltaTime steps if debugging/paused
    if (deltaTime > 0.1f) {
      deltaTime = 0.1f;
    }
    lastFrameTime = currentFrameTime;

    // --- Event Handling ---
    handleEvents(
        gameData, assetManager,
        running); // Handles input events, updates game state based on input

    // --- Update Logic ---
    updateLogic(
        gameData,
        deltaTime); // Updates game state, entities, camera based on flags/state

    // --- Rendering ---
    renderScene(gameData, assetManager); // Draws everything

    // Frame Delay (Optional, prevents maxing out CPU)
    SDL_Delay(10); // Delay for ~10ms
                   //
  } // End Main Game Loop

  // --- Cleanup ---
  cleanupSDL(sdlContext); // Cleans up window, renderer, font, subsystems

  SDL_Log("Exiting gracefully. Farewell, Mortal.");
  return 0;
}

void handleEvents(GameData &gameData, AssetManager &assets, bool &running) {

  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      running = false;
    } else if (event.type == SDL_WINDOWEVENT &&
               event.window.event == SDL_WINDOWEVENT_RESIZED) {
      // Handle window resize (maintain aspect ratio if desired)
      // Your existing resize logic here...
    }

    // State-Specific Input Handling
    switch (gameData.currentGameState) {
    case GameState::MainMenu:
      if (!gameData.isPanning) { // Only handle menu input if not panning
        if (event.type == SDL_KEYDOWN) {
          switch (event.key.keysym.sym) {
          case SDLK_UP:
            gameData.selectedIndex = (gameData.selectedIndex > 0)
                                         ? gameData.selectedIndex - 1
                                         : gameData.menuItems.size() - 1;
            break;
          case SDLK_DOWN:
            gameData.selectedIndex =
                (gameData.selectedIndex < gameData.menuItems.size() - 1)
                    ? gameData.selectedIndex + 1
                    : 0;
            break;
          case SDLK_RETURN:
            if (gameData.selectedIndex == 0) { // Start Game
              gameData.isPanning = true;
              gameData.panCounter = 0; // Start panning animation
              // Transition to CharacterSelect happens *after* panning finishes
              // (see Update Logic section)
            } else if (gameData.selectedIndex == 1) {
              SDL_Log("Options not implemented.");
            } else if (gameData.selectedIndex == 2) { // Exit
              running = false;
            }
            break;
          case SDLK_ESCAPE:
            running = false;
            break;
          }
        }
      }
      break; // End MainMenu case

    case GameState::CharacterSelect:
      if (!gameData
               .isCharacterSelectFadingIn) { // Only handle input after fade-in
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
          case SDLK_RETURN: // Character Chosen
          {                 // Scope for variable initialization
            CharacterType chosenType = (gameData.selectedCharacterIndex == 0)
                                           ? CharacterType::FemaleMage
                                           : CharacterType::MaleMage;
            // Re-initialize player with chosen stats/spells (use the proper
            // constructor)
            if (chosenType == CharacterType::FemaleMage) {
              gameData.currentGamePlayer =
                  PlayerCharacter(chosenType, 100, 100, 150, 150, 1, 0, 0,
                                  gameData.tileWidth, gameData.tileHeight);
            } else {
              gameData.currentGamePlayer = PlayerCharacter(
                  chosenType, 120, 120, 130, 130, 1, 0, 0, gameData.tileWidth,
                  gameData.tileHeight); // Example different stats
            }

            // Generate Level 1
            gameData.enemies
                .clear(); // Clear enemies from previous game (if any)
            gameData.currentLevelIndex = 1;
            gameData.currentLevel = generateLevel(
                gameData.levelWidth, gameData.levelHeight,
                gameData.levelMaxRooms, gameData.levelMinRoomSize,
                gameData.levelMaxRoomSize, gameData.enemies, gameData.tileWidth,
                gameData.tileHeight); // Generate level and populate enemies
            gameData.levelRooms = gameData.currentLevel.rooms;

            // Place player at start position
            gameData.currentGamePlayer.targetTileX =
                gameData.currentLevel.startCol;
            gameData.currentGamePlayer.targetTileY =
                gameData.currentLevel.startRow;
            gameData.currentGamePlayer.x =
                gameData.currentGamePlayer.targetTileX * gameData.tileWidth +
                gameData.tileWidth / 2.0f; // Snap visual position
            gameData.currentGamePlayer.y =
                gameData.currentGamePlayer.targetTileY * gameData.tileHeight +
                gameData.tileHeight / 2.0f;
            gameData.currentGamePlayer.startTileX =
                gameData.currentGamePlayer
                    .targetTileX; // Ensure startTile is correct
            gameData.currentGamePlayer.startTileY =
                gameData.currentGamePlayer.targetTileY;

            // Initialize visibility map
            gameData.visibilityMap.assign(
                gameData.currentLevel.height,
                std::vector<float>(gameData.currentLevel.width,
                                   0.0f)); // Resize and zero-fill
            updateVisibility(gameData.currentLevel, gameData.levelRooms,
                             gameData.currentGamePlayer.targetTileX,
                             gameData.currentGamePlayer.targetTileY,
                             gameData.hallwayVisibilityDistance,
                             gameData.visibilityMap);

            // Transition to Gameplay
            gameData.currentGameState = GameState::PlayerTurn;
            gameData.isCharacterSelectFadingIn =
                false; // Ensure fade flag is reset
            gameData.hasCharacterSelectStartedFading = false;
            gameData.characterSelectAlpha = 0;
          } // End scope
          break; // End Enter Key case
          case SDLK_ESCAPE:
            running = false;
            break; // Optional: Quit from char select
          } // End keydown switch
        } // End keydown check
      } // End !isFadingIn check
      break; // End CharacterSelect case

    case GameState::PlayerTurn:
      if (!gameData.currentGamePlayer
               .isMoving) { // Player can only act when not moving
        // --- Spell Menu Input Handling (ONLY if SpellMenu is active) ---
        if (event.type == SDL_KEYDOWN) {
          if (gameData.currentMenu == GameMenu::SpellMenu) {
            switch (event.key.keysym.sym) {
            case SDLK_UP:
              if (event.key.repeat == 0 &&
                  !gameData.currentGamePlayer.knownSpells.empty()) {
                gameData.spellSelectIndex =
                    (gameData.spellSelectIndex > 0)
                        ? gameData.spellSelectIndex - 1
                        : gameData.currentGamePlayer.knownSpells.size() - 1;
              }
              break;
            case SDLK_DOWN:
              if (event.key.repeat == 0 &&
                  !gameData.currentGamePlayer.knownSpells.empty()) {
                gameData.spellSelectIndex =
                    (gameData.spellSelectIndex <
                     gameData.currentGamePlayer.knownSpells.size() - 1)
                        ? gameData.spellSelectIndex + 1
                        : 0;
              }
              break;
            case SDLK_RETURN: // Select the highlighted spell
              if (!gameData.currentGamePlayer.knownSpells.empty() &&
                  gameData.spellSelectIndex >= 0 &&
                  gameData.spellSelectIndex <
                      gameData.currentGamePlayer.knownSpells.size()) {
                gameData.currentSpellIndex =
                    gameData.spellSelectIndex; // Set the spell to be
                                               // potentially cast
                const Spell &spellToCast = gameData.currentGamePlayer.getSpell(
                    gameData.currentSpellIndex);

                if (gameData.currentGamePlayer.canCastSpell(
                        gameData.currentSpellIndex)) {
                  if (spellToCast.targetType == SpellTargetType::Self) {
                    // Cast self-targeted spell immediately & end turn
                    if (gameData.currentGamePlayer.castSpell(
                            gameData.currentSpellIndex,
                            gameData.currentGamePlayer.targetTileX,
                            gameData.currentGamePlayer.targetTileY,
                            gameData.enemies, gameData.activeProjectiles,
                            nullptr)) {
                      gameData.currentGameState =
                          GameState::EnemyTurn; // actionTaken handled by state
                                                // change
                      for (auto &enemy : gameData.enemies) {
                        enemy.hasTakenActionThisTurn = false;
                      } // Reset enemy flags
                    } else {
                    } // Should ideally not happen if canCastSpell was true
                  } else {
                    // Enter targeting mode for non-self spells
                    gameData.targetIndicatorX =
                        gameData.currentGamePlayer.targetTileX;
                    gameData.targetIndicatorY =
                        gameData.currentGamePlayer.targetTileY;
                    gameData.currentGameState = GameState::PlayerTargeting;
                  }
                } else {
                  // Optionally add a sound effect or UI message here
                }
              }
              gameData.currentMenu =
                  GameMenu::None; // Close menu after selection attempt
              break;
            case SDLK_ESCAPE: // Cancel spell selection
              gameData.currentMenu = GameMenu::None; // Close the menu
              gameData.spellSelectIndex = 0;         // Reset highlight
              gameData.currentSpellIndex = -1;
              break;
            default:
              // Allow closing menu with 'c' again? Or ignore other keys?
              // if (event.key.keysym.sym == SDLK_c) { currentMenu =
              // GameMenu::None; }
              break;
            }
          } // End Spell Menu Input Handling

          // --- Normal Player Turn Input Handling (ONLY if NO menu is active)
          // ---
          else if (gameData.currentMenu == GameMenu::None) {
            int moveX = 0;
            int moveY = 0;
            bool actionTaken = false; // Only relevant for *instant* actions now

            switch (event.key.keysym.sym) {
            // --- Movement ---
            case SDLK_UP:
              moveY = -1;
              break;
            case SDLK_DOWN:
              moveY = 1;
              break;
            case SDLK_LEFT:
              moveX = -1;
              break;
            case SDLK_RIGHT:
              moveX = 1;
              break;

            // --- Spellcasting INITIATION ---
            case SDLK_c:
              if (!gameData.currentGamePlayer.knownSpells.empty()) {
                gameData.spellSelectIndex =
                    0; // Start highlighting the first spell
                gameData.currentMenu = GameMenu::SpellMenu; // Open the menu
              } else {
              }
              // Note: Opening the menu doesn't consume the turn itself
              break; // End SDLK_c case

            case SDLK_q:
            case SDLK_w:
            case SDLK_e:
            case SDLK_r:
            case SDLK_t: { // Scope for variables
              int spellIndex = -1;
              if (event.key.keysym.sym == SDLK_q)
                spellIndex = 0;
              else if (event.key.keysym.sym == SDLK_w)
                spellIndex = 1;
              else if (event.key.keysym.sym == SDLK_e)
                spellIndex = 2;
              else if (event.key.keysym.sym == SDLK_r)
                spellIndex = 3;
              else if (event.key.keysym.sym == SDLK_t)
                spellIndex = 4;

              if (spellIndex != -1 &&
                  spellIndex < gameData.currentGamePlayer.knownSpells.size()) {
                gameData.currentSpellIndex =
                    spellIndex; // Store which spell is being attempted
                const Spell &spellToCast =
                    gameData.currentGamePlayer.getSpell(spellIndex);

                if (gameData.currentGamePlayer.canCastSpell(spellIndex)) {
                  if (spellToCast.targetType == SpellTargetType::Self) {
                    if (gameData.currentGamePlayer.castSpell(
                            spellIndex, gameData.currentGamePlayer.targetTileX,
                            gameData.currentGamePlayer.targetTileY,
                            gameData.enemies, gameData.activeProjectiles,
                            nullptr)) {
                      actionTaken =
                          true; // Mark that an instant action occurred
                    } else {
                    }
                  } else { // Needs targeting
                    gameData.targetIndicatorX =
                        gameData.currentGamePlayer.targetTileX;
                    gameData.targetIndicatorY =
                        gameData.currentGamePlayer.targetTileY;
                    gameData.currentGameState =
                        GameState::PlayerTargeting; // Change state
                    // Turn ends upon confirmation in targeting state
                  }
                } else {
                }
              } else {
              }
            } // End scope for hotkey cases
            break; // End hotkey cases block

            default:
              break;
            } // End keydown switch (Normal Turn)

            // --- Process Movement ---
            if (!actionTaken &&
                gameData.currentGameState == GameState::PlayerTurn &&
                (moveX != 0 || moveY != 0)) { // Check we haven't switched state
              int newPlayerTargetX =
                  gameData.currentGamePlayer.targetTileX + moveX;
              int newPlayerTargetY =
                  gameData.currentGamePlayer.targetTileY + moveY;

              // Check level bounds and walkability
              if (isWithinBounds(newPlayerTargetX, newPlayerTargetY,
                                 gameData.currentLevel.width,
                                 gameData.currentLevel.height) &&
                  gameData.currentLevel
                          .tiles[newPlayerTargetY][newPlayerTargetX] != '#') {
                // Check for enemy collision
                bool collision = false;
                for (const auto &enemy : gameData.enemies) {
                  if (enemy.x == newPlayerTargetX &&
                      enemy.y == newPlayerTargetY && enemy.health > 0) {
                    collision = true;
                    // TODO: Implement attacking enemy by bumping into them?
                    // For now, bumping into enemy also ends the turn without
                    // moving
                    actionTaken = true;
                    break;
                  }
                }

                if (!collision) { // If no wall and no enemy, start moving
                  gameData.currentGamePlayer.startMove(newPlayerTargetX,
                                                       newPlayerTargetY);
                  // Movement itself is the action, turn transition happens
                  // after animation. actionTaken remains false, letting the
                  // update loop handle turn end. Update visibility based on the
                  // *target* tile immediately for responsiveness
                  updateVisibility(gameData.currentLevel, gameData.levelRooms,
                                   newPlayerTargetX, newPlayerTargetY,
                                   gameData.hallwayVisibilityDistance,
                                   gameData.visibilityMap);
                }
              } else {
              }
            } // End movement processing

            // --- End Turn if an Instant Action Occurred ---
            if (actionTaken) {
              // This handles instant spells, waiting, bumping into enemies etc.
              gameData.currentGameState = GameState::EnemyTurn;
              // Reset enemy turn flags immediately
              for (auto &enemy : gameData.enemies) {
                enemy.hasTakenActionThisTurn = false;
              }
            }

          } // End keydown check
        }
      } // End !isMoving check
      break; // End PlayerTurn case

    case GameState::PlayerTargeting:
      if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_UP:
          gameData.targetIndicatorY--;
          break;
        case SDLK_DOWN:
          gameData.targetIndicatorY++;
          break;
        case SDLK_LEFT:
          gameData.targetIndicatorX--;
          break;
        case SDLK_RIGHT:
          gameData.targetIndicatorX++;
          break;

        case SDLK_RETURN: // Confirm Spell Target
        {                 // Scope needed
          const Spell &spell = gameData.currentGamePlayer.getSpell(
              gameData.currentSpellIndex); // Get selected spell info
          int distance = std::abs(gameData.currentGamePlayer.targetTileX -
                                  gameData.targetIndicatorX) +
                         std::abs(gameData.currentGamePlayer.targetTileY -
                                  gameData.targetIndicatorY);

          if (distance <= spell.range) { // Check range again
            // Attempt to cast the spell
            SDL_Texture *texToUse = nullptr;
            // Determine which texture based on the selected spell index
            // This assumes you know which spell corresponds to which index
            // Example: Assuming index 0 is Firebolt, index 1 is Fireball (if
            // you add it)
            if (gameData.currentSpellIndex ==
                0) { // Assuming Firebolt is index 0
              texToUse =
                  assets.getTexture("fireball"); // Use the texture you loaded
              // If you rename fireballTexture to fireboltTexture, use that here
            }
            // else if (currentSpellIndex == 1) { texToUse = fireballTexture; }
            // else if (currentSpellIndex == ...) { texToUse = someOtherTexture;
            // }

            if (!texToUse) {

            }

            if (gameData.currentGamePlayer.castSpell(
                    gameData.currentSpellIndex, gameData.targetIndicatorX,
                    gameData.targetIndicatorY, gameData.enemies,
                    gameData.activeProjectiles, texToUse)) {
              // Spell cast SUCCESS! End turn.
              gameData.currentGameState = GameState::ProjectileResolution;
              gameData.currentSpellIndex = -1; // Reset selected spell
              // Reset enemy turn flags
              for (auto &enemy : gameData.enemies) {
                enemy.hasTakenActionThisTurn = false;
              }
            } else {
              // Casting failed (e.g., no valid target found by castSpell
              // implementation, though mana/range checked)
              gameData.currentGameState =
                  GameState::PlayerTurn; // Go back to player turn
              gameData.currentSpellIndex = -1;
            }
          } else {
            // Stay in targeting mode
          }
        } // End scope
        break; // End SDLK_RETURN case

        case SDLK_ESCAPE: // Cancel Targeting
          gameData.currentGameState =
              GameState::PlayerTurn;       // Go back to player turn
          gameData.currentSpellIndex = -1; // Reset selected spell
          break;                           // End SDLK_ESCAPE case

        default:
          break;
        } // End keydown switch

        // Clamp target indicator within level bounds
        gameData.targetIndicatorX =
            std::max(0, std::min(gameData.currentLevel.width - 1,
                                 gameData.targetIndicatorX));
        gameData.targetIndicatorY =
            std::max(0, std::min(gameData.currentLevel.height - 1,
                                 gameData.targetIndicatorY));

      } // End keydown check
      break; // End PlayerTargeting case

    case GameState::EnemyTurn:
      // No player input during enemy turn
      break; // End EnemyTurn case

    case GameState::ProjectileResolution:
      // Ignore player input while projectiles are flying
      if (event.type == SDL_KEYDOWN) {
      }
      break; // End ProjectileResolution case

    case GameState::GameOver:
      // Input to restart or quit?
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN) {
          // Restart - go back to main menu or character select?
          gameData.currentGameState = GameState::MainMenu;
          // Reset any game variables needed for restart
        } else if (event.key.keysym.sym == SDLK_ESCAPE) {
          running = false;
        }
      }
      break; // End GameOver case

    } // End GameState switch (within event polling)
  } // End SDL_PollEvent loop
}

void updateLogic(GameData &gameData, float deltaTime) {

  // Update Panning/Fades (Menu -> Character Select)
  if (gameData.isPanning) {
    gameData.panCounter += 10;           // Adjust speed as needed
    gameData.splashPanOffset -= 10;      // Assuming splash pans upwards
    if (gameData.splashPanOffset <= 0) { // Panning finished
      gameData.splashPanOffset = 0;
      gameData.isPanning = false;
      gameData.currentGameState =
          GameState::CharacterSelect;            // Now transition state
      gameData.isCharacterSelectFadingIn = true; // Start fade in
      gameData.characterSelectAlpha = 0;
      gameData.hasCharacterSelectStartedFading = true;
    }
  }
  if (gameData.isCharacterSelectFadingIn) {
    // Need safe alpha increment
    int newAlpha = static_cast<int>(gameData.characterSelectAlpha) +
                   10; // Adjust fade speed
    if (newAlpha >= 255) {
      gameData.characterSelectAlpha = 255;
      gameData.isCharacterSelectFadingIn = false; // Fade-in complete
    } else {
      gameData.characterSelectAlpha = static_cast<Uint8>(newAlpha);
    }
  }

  // Update Game Entities (Only during gameplay states)
  switch (gameData.currentGameState) {
  case GameState::PlayerTurn:
    // Only update player movement if they are moving
    if (gameData.currentGamePlayer.isMoving) {
      bool playerWasMoving = true; // Assume they were if in this state
      gameData.currentGamePlayer.update(deltaTime, gameData.tileWidth,
                                        gameData.tileHeight);
      bool playerIsNowIdle = !gameData.currentGamePlayer.isMoving;

      // Check if movement finished AND menu is closed
      if (playerWasMoving && playerIsNowIdle &&
          gameData.currentMenu == GameMenu::None) {
        gameData.currentGameState = GameState::EnemyTurn;
        for (auto &enemy : gameData.enemies) {
          enemy.hasTakenActionThisTurn = false;
        }
      }
    }
    // Do NOT update projectiles or enemies here
    break;

  case GameState::ProjectileResolution: { // Scope for variables
    // --- Update Projectiles & Apply Hits ---
    bool hitOccurred = false; // Track if any hit happened this frame
    // Iterate using index to allow safe removal inside loop is complex,
    // so let's update all, apply hits, then remove dead ones after.
    std::vector<std::pair<int, int>>
        hitLocations; // Store XY tile coords of hits

    for (auto &proj : gameData.activeProjectiles) {
      if (proj.isActive &&
          proj.update(deltaTime)) { // proj.update returns true on hit
        hitOccurred = true;
        int hitX = static_cast<int>(proj.targetX / gameData.tileWidth);
        int hitY = static_cast<int>(proj.targetY / gameData.tileHeight);
        hitLocations.push_back({hitX, hitY});
        // proj.isActive is set to false inside proj.update()
      }
    }

    // Apply damage for all hits that occurred this frame
    if (hitOccurred) {
      for (const auto &hitPos : hitLocations) {
        int hitX = hitPos.first;
        int hitY = hitPos.second;
        bool enemyFoundAndDamaged = false;
        for (auto &enemy : gameData.enemies) {
          if (enemy.x == hitX && enemy.y == hitY && enemy.health > 0) {
            int damageAmount = 15; // TODO: Get damage from projectile/spell
            enemy.takeDamage(damageAmount);
            enemyFoundAndDamaged = true;
            // Hit only one enemy per projectile? break; might be needed if so.
            break;
          }
        }
        if (!enemyFoundAndDamaged) {
        }
      }
    }

    // --- Remove Inactive Projectiles ---
    gameData.activeProjectiles.erase(
        std::remove_if(gameData.activeProjectiles.begin(),
                       gameData.activeProjectiles.end(),
                       [](const Projectile &p) { return !p.isActive; }),
        gameData.activeProjectiles.end());

    // --- Check if Resolution Finished ---
    if (gameData.activeProjectiles.empty()) {
      gameData.currentGameState = GameState::EnemyTurn;
      // Reset enemy action flags NOW before enemies act
      for (auto &enemy : gameData.enemies) {
        enemy.hasTakenActionThisTurn = false;
      }
      // --- Check for Player Death (after potential hits) ---
      // It's possible player died from reflected damage etc. Check here? Or
      // after enemy turn? Let's check after enemy turn for now.
    }
  } // End scope for ProjectileResolution case
  break; // End ProjectileResolution case

  case GameState::EnemyTurn: {
    // --- Update Enemies & Handle AI/Turn ---
    bool allEnemiesActed = true;
    bool allEnemiesIdle = true;
    bool actionAttemptedThisFrame = false;

    for (auto &enemy : gameData.enemies) {
      if (enemy.health > 0) {
        // Update enemy animation/movement first
        enemy.update(deltaTime);

        // Then check AI / Take Action
        if (!enemy.hasTakenActionThisTurn) {
          allEnemiesActed = false;
          if (!enemy.isMoving && !actionAttemptedThisFrame) {
            enemy.takeAction(gameData.currentLevel, gameData.currentGamePlayer);
            actionAttemptedThisFrame = true;
          }
        }
        if (enemy.isMoving) {
          allEnemiesIdle = false;
        }
      }
    }

    // Transition back ONLY if all living enemies have acted AND finished moving
    if (allEnemiesActed && allEnemiesIdle) {
      // --- Check for Player Death ---
      if (gameData.currentGamePlayer.health <= 0) {
        gameData.currentGameState = GameState::GameOver;
        SDL_Log("--- Game Over ---");
      } else {
        // --- Start Player Turn ---
        gameData.currentGameState = GameState::PlayerTurn;
        updateVisibility(gameData.currentLevel, gameData.levelRooms,
                         gameData.currentGamePlayer.targetTileX,
                         gameData.currentGamePlayer.targetTileY,
                         gameData.hallwayVisibilityDistance,
                         gameData.visibilityMap);
      }
    }
    // Do NOT update player or projectiles here
    break; // End EnemyTurn case
  }
  // PlayerTargeting and GameOver don't have active entity updates in this model
  case GameState::PlayerTargeting:
  case GameState::GameOver:
  case GameState::MainMenu:
  case GameState::CharacterSelect:
    // No entity updates needed in these states
    break;

  } // End Gameplay State Update block

  gameData.enemies.erase(
      std::remove_if(gameData.enemies.begin(), gameData.enemies.end(),
                     [](const Enemy &e) { return e.health <= 0; }),
      gameData.enemies.end());

  // Update Camera (Should run in most gameplay states to follow player)
  if (gameData.currentGameState == GameState::PlayerTurn ||
      gameData.currentGameState == GameState::PlayerTargeting ||
      gameData.currentGameState ==
          GameState::ProjectileResolution || // Follow player even during
                                             // resolution
      gameData.currentGameState ==
          GameState::EnemyTurn) // Follow player even during enemy turn
  {
    int halfWidth = gameData.windowWidth / 2;
    int halfHeight = gameData.windowHeight / 2;
    // Center on the player's *current visual position*
    int idealCameraX =
        static_cast<int>(gameData.currentGamePlayer.x) - halfWidth;
    int idealCameraY =
        static_cast<int>(gameData.currentGamePlayer.y) - halfHeight;

    // Clamp camera to level boundaries
    int maxCameraX = (gameData.currentLevel.width * gameData.tileWidth) -
                     gameData.windowWidth;
    int maxCameraY = (gameData.currentLevel.height * gameData.tileHeight) -
                     gameData.windowHeight;
    gameData.cameraX = std::max(0, std::min(idealCameraX, maxCameraX));
    gameData.cameraY = std::max(0, std::min(idealCameraY, maxCameraY));
    // Ensure maxCameraX/Y aren't negative if level is smaller than screen
    if (maxCameraX < 0)
      gameData.cameraX = 0;
    if (maxCameraY < 0)
      gameData.cameraY = 0;
  }
}

void renderScene(GameData &gameData, AssetManager &assets) {

  SDL_SetRenderDrawColor(gameData.renderer, 0, 0, 0, 255); // Black background
  SDL_RenderClear(gameData.renderer);

  // Render based on Game State
  switch (gameData.currentGameState) {
  case GameState::MainMenu:
    displayMenu(gameData.renderer, assets.getFont("main_font"),
                assets.getTexture("splash"), gameData.menuItems,
                gameData.selectedIndex, gameData.isPanning,
                gameData.splashPanOffset, 456, gameData.windowWidth,
                gameData.windowHeight); // Adjust Y offset as needed
    break;

  case GameState::CharacterSelect:
    // Render a background if desired (e.g., black or a static image)
    // SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    // SDL_RenderClear(renderer);
    displayCharacterSelect(gameData.renderer, assets.getFont("main_font"),
                           gameData.selectedCharacterIndex,
                           gameData.windowWidth, gameData.windowHeight,
                           gameData.characterSelectAlpha);
    break;

  case GameState::PlayerTurn:
  case GameState::PlayerTargeting: // Targeting shares rendering with PlayerTurn
  case GameState::ProjectileResolution:
  case GameState::EnemyTurn:
    // --- Render Level ---
    if (!gameData.currentLevel.tiles.empty()) {
      for (int y = 0; y < gameData.currentLevel.height; ++y) {
        for (int x = 0; x < gameData.currentLevel.width; ++x) {
          SDL_Rect tileRect = {(x * gameData.tileWidth) - gameData.cameraX,
                               (y * gameData.tileHeight) - gameData.cameraY,
                               gameData.tileWidth, gameData.tileHeight};
          float visibility = (isWithinBounds(x, y, gameData.currentLevel.width,
                                             gameData.currentLevel.height))
                                 ? gameData.visibilityMap[y][x]
                                 : 0.0f;

          if (visibility > 0.0f) { // Only render visible tiles
                                   // Choose color/texture based on tile type
            if (y == gameData.currentLevel.startRow &&
                x == gameData.currentLevel.startCol &&
                assets.getTexture("start_tile")) {
              SDL_RenderCopy(gameData.renderer, assets.getTexture("start_tile"),
                             nullptr, &tileRect);
            } else if (y == gameData.currentLevel.endRow &&
                       x == gameData.currentLevel.endCol &&
                       assets.getTexture("exit_tile")) {
              SDL_RenderCopy(gameData.renderer, assets.getTexture("exit_tile"),
                             nullptr, &tileRect);
            } else if (gameData.currentLevel.tiles[y][x] == '#') {
              SDL_SetRenderDrawColor(gameData.renderer, 139, 69, 19,
                                     255); // Brown Wall
              SDL_RenderFillRect(gameData.renderer, &tileRect);
            } else if (gameData.currentLevel.tiles[y][x] == '.') {
              SDL_SetRenderDrawColor(gameData.renderer, 200, 200, 200,
                                     255); // Light Gray Floor
              SDL_RenderFillRect(gameData.renderer, &tileRect);
            }
            // Add other tile types ('V' for void/vision blocker?)

            // Apply visibility dimming overlay
            Uint8 alpha = static_cast<Uint8>(
                (1.0f - visibility) * 200); // Adjust dimming intensity (e.g., *
                                            // 200 for less total darkness)
            SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(gameData.renderer, 0, 0, 0,
                                   alpha); // Black overlay
            SDL_RenderFillRect(gameData.renderer, &tileRect);
            SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_NONE);

          } else {
            // Optionally render previously explored tiles dimly (Fog of War)
            // Or just render black for unseen tiles
            SDL_SetRenderDrawColor(gameData.renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(gameData.renderer, &tileRect);
          }
        } // End x loop
      } // End y loop
    } // End level rendering

    // --- Render Enemies ---
    for (const auto &enemy : gameData.enemies) {
      if (enemy.health > 0) {     // Only render alive enemies
        int enemyTileX = enemy.x; // Use logical position for visibility check
        int enemyTileY = enemy.y;
        float visibility = 0.0f;
        if (isWithinBounds(enemyTileX, enemyTileY, gameData.currentLevel.width,
                           gameData.currentLevel.height)) {
          visibility = gameData.visibilityMap[enemyTileY][enemyTileX];
        }

        if (visibility > 0.0f) {
          enemy.render(gameData.renderer, gameData.cameraX, gameData.cameraY,
                       visibility); // Pass visibility for alpha
        }
      }
    }

    // --- Render Player ---
    SDL_Rect playerRect;
    playerRect.w =
        gameData.tileWidth / 2; // Make player smaller? Adjust as needed
    playerRect.h = gameData.tileHeight / 2;
    playerRect.x =
        static_cast<int>(gameData.currentGamePlayer.x - playerRect.w / 2.0f) -
        gameData.cameraX; // Center based on visual pos
    playerRect.y =
        static_cast<int>(gameData.currentGamePlayer.y - playerRect.h / 2.0f) -
        gameData.cameraY;
    SDL_SetRenderDrawColor(gameData.renderer, 0, 255, 0,
                           255); // Green for player
    SDL_RenderFillRect(gameData.renderer, &playerRect);

    // --- >> ADD Projectile Rendering << ---
    for (const auto &proj : gameData.activeProjectiles) {
      // Optionally add visibility check? Should projectiles be visible in FoW?
      // For now, render all active.
      proj.render(gameData.renderer, gameData.cameraX, gameData.cameraY);
    }

    // --- Render Spell Menu Overlay (If Active) ---
    if (gameData.currentGameState == GameState::PlayerTurn &&
        gameData.currentMenu == GameMenu::SpellMenu) {
      renderSpellMenu(gameData.renderer, assets.getFont("main_font"),
                      gameData.currentGamePlayer, gameData.spellSelectIndex,
                      gameData.windowWidth, gameData.windowHeight);
    }

    // --- Render Targeting Reticle (if targeting) ---
    if (gameData.currentGameState == GameState::PlayerTargeting) {
      // Check if the target tile is visible before rendering reticle
      if (isWithinBounds(gameData.targetIndicatorX, gameData.targetIndicatorY,
                         gameData.currentLevel.width,
                         gameData.currentLevel.height) &&
          gameData.visibilityMap[gameData.targetIndicatorY]
                                [gameData.targetIndicatorX] > 0.0f) {
        SDL_Rect reticleRect = {
            (gameData.targetIndicatorX * gameData.tileWidth) - gameData.cameraX,
            (gameData.targetIndicatorY * gameData.tileHeight) -
                gameData.cameraY,
            gameData.tileWidth, gameData.tileHeight};
        const Spell &spell =
            gameData.currentGamePlayer.getSpell(gameData.currentSpellIndex);
        int distance =
            std::sqrt(std::pow(gameData.currentGamePlayer.targetTileX -
                                   gameData.targetIndicatorX,
                               2) +
                      std::pow(gameData.currentGamePlayer.targetTileY -
                                   gameData.targetIndicatorY,
                               2));
        Uint8 reticleAlpha = 180; // Base alpha for reticle

        // Set color/texture based on range validity
        if (distance <= spell.range) {
          // White or Green if in range
          if (assets.getTexture("reticle"))
            SDL_SetTextureColorMod(assets.getTexture("reticle"), 255, 255,
                                   255); // White tint
          SDL_SetRenderDrawColor(gameData.renderer, 255, 255, 255,
                                 reticleAlpha);
        } else {
          // Red if out of range
          if (assets.getTexture("reticle"))
            SDL_SetTextureColorMod(assets.getTexture("reticle"), 255, 100,
                                   100); // Red tint
          SDL_SetRenderDrawColor(gameData.renderer, 255, 0, 0, reticleAlpha);
        }

        if (assets.getTexture("reticle")) {
          SDL_SetTextureAlphaMod(assets.getTexture("reticle"), reticleAlpha);
          SDL_RenderCopy(gameData.renderer, assets.getTexture("reticle"),
                         nullptr, &reticleRect);
        } else {
          // Fallback: Draw a simple box
          SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_BLEND);
          SDL_RenderDrawRect(gameData.renderer, &reticleRect);
          SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_NONE);
        }
      } // End visibility check for reticle
    } // End rendering reticle

    // --- Render UI ---
    renderSpellBar(gameData, assets);
    renderUI(gameData, assets);
    break; // End gameplay rendering states

  case GameState::GameOver:
    // Render "Game Over" screen
    // TODO: Implement displayGameOver function or similar
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surface =
        TTF_RenderText_Solid(assets.getFont("main_font"),
                             "Game Over - Press Enter to Restart", white);
    if (surface) {
      SDL_Texture *texture =
          SDL_CreateTextureFromSurface(gameData.renderer, surface);
      if (texture) {
        SDL_Rect textRect;
        textRect.w = surface->w;
        textRect.h = surface->h;
        textRect.x = (gameData.windowWidth - textRect.w) / 2;
        textRect.y = (gameData.windowHeight - textRect.h) / 2;
        SDL_RenderCopy(gameData.renderer, texture, nullptr, &textRect);
        SDL_DestroyTexture(texture);
      }
      SDL_FreeSurface(surface);
    }
    break; // End GameOver case

  } // End GameState rendering switch

  SDL_RenderPresent(gameData.renderer);
}