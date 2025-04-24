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
#include "utils.h" // Includes SDL_Context definition
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
// Ensure SDL_Context& is in the signature
void handleEvents(GameData &gameData, AssetManager &assets, bool &running,
                  SDL_Context &context);
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

  // --- SET LOGGING LEVEL ---
  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_ERROR); // Set to ERROR to reduce log
                                                 // spam, adjust as needed
  // SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE); // Or DEBUG for more info
  // --- END LOGGING LEVEL ---

  // Assign essential context to gameData (only renderer needed inside gameData)
  gameData.renderer = sdlContext.renderer;

  // --- Create Asset Manager ---
  AssetManager assetManager(gameData.renderer); // <-- CREATE MANAGER

  // --- Initialize Player Character Correctly ---
  // Use the tileWidth/Height now stored in gameData
  gameData.currentGamePlayer =
      PlayerCharacter(CharacterType::FemaleMage, 0, 0, // Placeholder X, Y
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
          "exit_tile", "../assets/sprites/exit_tile.png")) { /* Handle error */
  }
  if (!assetManager.loadTexture(
          "reticle",
          "../assets/sprites/target_reticle.png")) { /* Handle error */
  }
  if (!assetManager.loadTexture(
          "fireball", "../assets/sprites/fireball.PNG")) { /* Handle error */
  }
  if (!assetManager.loadTexture(
          "fireball_icon",
          "../assets/sprites/fireball_icon.PNG")) { /* Handle error */
  }
  if (!assetManager.loadTexture(
          "minor_heal_icon",
          "../assets/sprites/minor_heal_icon.PNG")) { /* Handle error */
  }
  if (!assetManager.loadTexture("wall_texture",
                                "../assets/sprites/wall_1.PNG")) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load wall texture!");
  }
  if (!assetManager.loadTexture("floor_1", "../assets/sprites/floor_1.PNG")) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load floor texture!");
    // Handle error appropriately
  }
  if (!assetManager.loadTexture("floor_2", "../assets/sprites/floor_2.PNG")) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load floor texture!");
    // Handle error appropriately
  }
  if (!assetManager.loadFont("main_font", "../assets/fonts/LUMOS.TTF",
                             36)) { /* Handle error */
  } // Load font here
  if (!assetManager.loadFont("spellbar_font", "../assets/fonts/LUMOS.TTF",
                             18)) { /* Handle error */
  } // Load font here
  // --- Load Character Portraits ---
  if (!assetManager.loadTexture("female_mage_portrait",
                                "../assets/sprites/female_mage_portrait.PNG")) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to load female mage portrait!");
  }
  if (!assetManager.loadTexture("male_mage_portrait",
                                "../assets/sprites/male_mage_portrait.PNG")) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to load male mage portrait!");
  }

  // Enemy sprites
  if (!assetManager.loadTexture("slime_texture",
                                "../assets/sprites/slime.PNG")) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load slime texture!");
    // Handle error appropriately - maybe fallback texture or exit?
  }

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
    // Pass sdlContext to handleEvents
    handleEvents(
        gameData, assetManager, running,
        sdlContext); // Handles input events, updates game state based on input

    // --- Update Logic ---
    updateLogic(
        gameData,
        deltaTime); // Updates game state, entities, camera based on flags/state

    // --- Rendering ---
    renderScene(gameData, assetManager); // Draws everything

    // Frame Delay (Optional, prevents maxing out CPU)
    if (gameData.currentGameState != GameState::EnemyTurn) {
      SDL_Delay(10); // Delay for ~10ms
    }

  } // End Main Game Loop

  // --- Cleanup ---
  cleanupSDL(sdlContext); // Cleans up window, renderer, font, subsystems

  SDL_Log("Exiting gracefully. Farewell, Mortal.");
  return 0;
}

// Add SDL_Context& context to function signature
void handleEvents(GameData &gameData, AssetManager &assets, bool &running,
                  SDL_Context &context) {

  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      running = false;
    } else if (event.type == SDL_WINDOWEVENT &&
               event.window.event == SDL_WINDOWEVENT_RESIZED) {
      // Handle window resize (maintain aspect ratio if desired)
      // Use context.window here
      SDL_GetWindowSize(context.window, &gameData.windowWidth,
                        &gameData.windowHeight);
      SDL_RenderSetLogicalSize(gameData.renderer, gameData.windowWidth,
                               gameData.windowHeight);
      SDL_Log("Window resized to %d x %d", gameData.windowWidth,
              gameData.windowHeight);
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
                (gameData.selectedIndex < (int)gameData.menuItems.size() -
                                              1) // Cast to int for comparison
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
            gameData.currentGamePlayer =
                PlayerCharacter(chosenType, 0, 0, // Use 0,0 - will be set below
                                gameData.tileWidth, gameData.tileHeight);

            // Generate Level 1
            gameData.enemies
                .clear(); // Clear enemies from previous game (if any)
            gameData.activeProjectiles.clear(); // Clear projectiles
            gameData.currentLevelIndex = 1;
            gameData.currentLevel = generateLevel(
                gameData.levelWidth, gameData.levelHeight,
                gameData.levelMaxRooms, gameData.levelMinRoomSize,
                gameData.levelMaxRoomSize, gameData.enemies, gameData.tileWidth,
                gameData.tileHeight); // Generate level and populate enemies
            gameData.levelRooms = gameData.currentLevel.rooms;

            // --- Initialize Occupation Grid ---
            gameData.occupationGrid.assign(
                gameData.currentLevel.height,
                std::vector<bool>(gameData.currentLevel.width,
                                  false)); // Set all to false initially

            // Mark walls as occupied
            for (int y = 0; y < gameData.currentLevel.height; ++y) {
              for (int x = 0; x < gameData.currentLevel.width; ++x) {
                if (gameData.currentLevel.tiles[y][x] == '#') {
                  gameData.occupationGrid[y][x] = true;
                }
              }
            }

            // Place player at start position (BEFORE marking grid)
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

            // Mark initial player position as occupied (AFTER setting it)
            if (isWithinBounds(gameData.currentGamePlayer.targetTileX,
                               gameData.currentGamePlayer.targetTileY,
                               gameData.currentLevel.width,
                               gameData.currentLevel.height)) {
              gameData.occupationGrid[gameData.currentGamePlayer.targetTileY]
                                     [gameData.currentGamePlayer.targetTileX] =
                  true;
            } else {
              SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                           "Initial player position (%d, %d) is out of bounds!",
                           gameData.currentGamePlayer.targetTileX,
                           gameData.currentGamePlayer.targetTileY);
            }

            // Mark initial enemy positions as occupied
            for (const auto &enemy : gameData.enemies) {
              if (isWithinBounds(enemy.x, enemy.y, gameData.currentLevel.width,
                                 gameData.currentLevel.height)) {
                gameData.occupationGrid[enemy.y][enemy.x] = true;
              } else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Initial enemy position (%d, %d) is out of bounds!",
                            enemy.x, enemy.y);
              }
            }
            SDL_Log("Occupation Grid Initialized for Level %d.",
                    gameData.currentLevelIndex);
            // --- End Occupation Grid Initialization ---

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
            gameData.currentMenu = GameMenu::None; // Ensure no menu is open
            gameData.spellSelectIndex = 0;         // Reset menu indices
            gameData.currentSpellIndex = -1;
            gameData.targetIndicatorX = 0; // Reset targeting
            gameData.targetIndicatorY = 0;
            gameData.cameraX = 0; // Reset camera
            gameData.cameraY = 0;
            // Update camera immediately based on new player position
            updateLogic(gameData, 0.0f); // Call updateLogic once to set camera

          } // End scope
          break; // End Enter Key case
          case SDLK_ESCAPE:
            // Optional: Go back to Main Menu instead of quitting?
            gameData.currentGameState = GameState::MainMenu;
            gameData.isCharacterSelectFadingIn = false; // Reset fade flags
            gameData.hasCharacterSelectStartedFading = false;
            gameData.characterSelectAlpha = 0;
            gameData.isPanning =
                false; // Ensure panning stops if escaping from char select
            gameData.splashPanOffset = 456; // Reset splash offset
            gameData.selectedIndex = 0;     // Reset main menu selection
            // running = false; // Current behavior: Quit
            break;
          } // End keydown switch
        } // End keydown check
      } // End !isFadingIn check
      break; // End CharacterSelect case

    case GameState::PlayerTurn:
      if (!gameData.currentGamePlayer
               .isMoving) { // Player can only act when not moving
        if (event.type == SDL_KEYDOWN) {
          // --- Spell Menu Input Handling (ONLY if SpellMenu is active) ---
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
                     (int)gameData.currentGamePlayer.knownSpells.size() -
                         1) // Cast to int
                        ? gameData.spellSelectIndex + 1
                        : 0;
              }
              break;
            case SDLK_RETURN: // Select the highlighted spell
              if (!gameData.currentGamePlayer.knownSpells.empty() &&
                  gameData.spellSelectIndex >= 0 &&
                  gameData.spellSelectIndex <
                      (int)gameData.currentGamePlayer.knownSpells
                          .size()) { // Cast to int
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
                            nullptr)) { // Pass nullptr for texture for
                                        // self-cast
                      // --- Start of Enemy Turn Initialization ---
                      gameData.currentGameState = GameState::EnemyTurn;
                      gameData.enemiesActingThisTurn = 0; // Start count at 0
                      for (auto &enemy : gameData.enemies) {
                        if (enemy.health > 0) { // Only count living enemies
                          enemy.hasTakenActionThisTurn =
                              false; // Reset action flag for the new turn
                          gameData
                              .enemiesActingThisTurn++; // Increment counter for
                                                        // living enemies
                        }
                      }
                      gameData.currentEnemyUpdateIndex =
                          0; // Reset update index
                      SDL_Log("Entering Enemy Turn. %d enemies acting.",
                              gameData.enemiesActingThisTurn);
                      gameData.currentMenu =
                          GameMenu::None; // Close menu after action

                    } else {
                      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                  "Self-spell cast failed despite CanCastSpell "
                                  "being true?");
                      // Maybe keep menu open if cast fails unexpectedly?
                    }
                  } else {
                    // Enter targeting mode for non-self spells
                    gameData.targetIndicatorX =
                        gameData.currentGamePlayer.targetTileX;
                    gameData.targetIndicatorY =
                        gameData.currentGamePlayer.targetTileY;
                    gameData.currentGameState = GameState::PlayerTargeting;
                    gameData.currentMenu =
                        GameMenu::None; // Close menu when entering targeting
                  }
                } else {
                  SDL_Log("Cannot cast spell %d (Not enough mana?)",
                          gameData.currentSpellIndex);
                  // Optionally add a sound effect or UI message here
                  // Keep menu open if cannot cast
                }
              } else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Invalid spell selection index: %d",
                            gameData.spellSelectIndex);
                gameData.currentMenu =
                    GameMenu::None; // Close menu on invalid selection
              }
              // gameData.currentMenu = GameMenu::None; // Moved closing menu
              // logic inside branches
              break;
            case SDLK_ESCAPE: // Cancel spell selection
            case SDLK_c:      // Allow closing with 'c' as well
              gameData.currentMenu = GameMenu::None; // Close the menu
              gameData.spellSelectIndex = 0;         // Reset highlight
              gameData.currentSpellIndex = -1;
              break;
            default:
              // Ignore other keys while spell menu is open
              break;
            }
          } // End Spell Menu Input Handling

          // --- Character Sheet Input Handling ---
          else if (gameData.currentMenu == GameMenu::CharacterSheet) {
            // Input handling when Character Sheet is open
            switch (event.key.keysym.sym) {
            case SDLK_i: // Close sheet with the same key
            case SDLK_ESCAPE:
              gameData.currentMenu = GameMenu::None;
              break;
            default:
              break; // Ignore other keys
            }
          } // End CharacterSheet Menu Input Handling

          // --- Normal Player Turn Input Handling (ONLY if NO menu is active)
          // ---
          else if (gameData.currentMenu == GameMenu::None) {
            // Enclose variable declarations in a scope
            {
              int moveX = 0;
              int moveY = 0;
              bool actionTaken =
                  false; // Only relevant for *instant* actions now

              switch (event.key.keysym.sym) {
              // --- Movement ---
              case SDLK_UP:
              case SDLK_KP_8: // Numpad 8
                moveY = -1;
                break;
              case SDLK_DOWN:
              case SDLK_KP_2: // Numpad 2
                moveY = 1;
                break;
              case SDLK_LEFT:
              case SDLK_KP_4: // Numpad 4
                moveX = -1;
                break;
              case SDLK_RIGHT:
              case SDLK_KP_6: // Numpad 6
                moveX = 1;
                break;
              case SDLK_KP_7: // Numpad 7 (Up-Left)
                moveX = -1;
                moveY = -1;
                break;
              case SDLK_KP_9: // Numpad 9 (Up-Right)
                moveX = 1;
                moveY = -1;
                break;
              case SDLK_KP_1: // Numpad 1 (Down-Left)
                moveX = -1;
                moveY = 1;
                break;
              case SDLK_KP_3: // Numpad 3 (Down-Right)
                moveX = 1;
                moveY = 1;
                break;
              case SDLK_KP_5:       // Numpad 5 (Wait)
                actionTaken = true; // Waiting is an action
                SDL_Log("Player waits...");
                break;

              // --- Spellcasting INITIATION ---
              case SDLK_c:
                if (event.key.repeat == 0) { // Prevent multiple toggles
                  if (!gameData.currentGamePlayer.knownSpells.empty()) {
                    gameData.spellSelectIndex =
                        0; // Start highlighting the first spell
                    gameData.currentMenu = GameMenu::SpellMenu; // Open the menu
                  } else {
                    SDL_Log("Player knows no spells!");
                    // Optional: Sound effect for failure
                  }
                }
                // Note: Opening the menu doesn't consume the turn itself
                break; // End SDLK_c case

              // --- Character Sheet Toggle ---
              case SDLK_i:
                if (event.key.repeat ==
                    0) { // Prevent multiple toggles from holding key
                  gameData.currentMenu =
                      GameMenu::CharacterSheet; // Open the sheet
                  // Note: Opening the sheet does not consume the turn
                }
                break; // End SDLK_i case

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
                    spellIndex < (int)gameData.currentGamePlayer.knownSpells
                                     .size()) { // Cast to int
                  gameData.currentSpellIndex =
                      spellIndex; // Store which spell is being attempted
                  const Spell &spellToCast =
                      gameData.currentGamePlayer.getSpell(spellIndex);

                  if (gameData.currentGamePlayer.canCastSpell(spellIndex)) {
                    if (spellToCast.targetType == SpellTargetType::Self) {
                      if (gameData.currentGamePlayer.castSpell(
                              spellIndex,
                              gameData.currentGamePlayer.targetTileX,
                              gameData.currentGamePlayer.targetTileY,
                              gameData.enemies, gameData.activeProjectiles,
                              nullptr)) { // Pass nullptr texture for self-cast
                        actionTaken =
                            true; // Mark that an instant action occurred
                      } else {
                        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                    "Self-spell hotkey cast failed despite "
                                    "CanCastSpell being true?");
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
                    SDL_Log("Cannot cast spell %d (Not enough mana?)",
                            spellIndex);
                    // Optional: Sound effect
                  }
                } else {
                  SDL_Log("Invalid spell hotkey index: %d", spellIndex);
                  // Optional: Sound effect
                }
              } // End scope for hotkey cases
              break; // End hotkey cases block

              default:
                break;
              } // End keydown switch (Normal Turn)

              // --- Process Movement ---
              if (!actionTaken &&
                  gameData.currentGameState == GameState::PlayerTurn &&
                  (moveX != 0 ||
                   moveY != 0)) { // Check we haven't switched state
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

                  // Check for enemy collision (using occupation grid now)
                  bool collision = false;
                  // Check occupation grid bounds before accessing
                  if (isWithinBounds(newPlayerTargetX, newPlayerTargetY,
                                     gameData.currentLevel.width,
                                     gameData.currentLevel.height) &&
                      gameData
                          .occupationGrid[newPlayerTargetY][newPlayerTargetX]) {
                    // Check if the occupant is an enemy
                    for (const auto &enemy : gameData.enemies) {
                      if (enemy.x == newPlayerTargetX &&
                          enemy.y == newPlayerTargetY && enemy.health > 0) {
                        collision = true;
                        // TODO: Implement attacking enemy by bumping into them?
                        // For now, bumping into enemy also ends the turn
                        // without moving
                        SDL_Log("Player bumps into enemy at (%d, %d)",
                                newPlayerTargetX, newPlayerTargetY);
                        actionTaken = true;
                        break;
                      }
                    }
                    // If it's occupied but not by an enemy (e.g., player
                    // somehow?) still treat as collision for movement
                    if (!collision) {
                      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                  "Player tried to move into occupied tile "
                                  "(%d, %d) - not an enemy?",
                                  newPlayerTargetX, newPlayerTargetY);
                      actionTaken = true; // Treat as blocked/wait
                    }
                  }

                  if (!collision) { // If no wall and no enemy, start moving
                    gameData.currentGamePlayer.startMove(newPlayerTargetX,
                                                         newPlayerTargetY);
                    // Movement itself is the action, turn transition happens
                    // after animation. actionTaken remains false, letting the
                    // update loop handle turn end. Update visibility based on
                    // the *target* tile immediately for responsiveness
                    updateVisibility(gameData.currentLevel, gameData.levelRooms,
                                     newPlayerTargetX, newPlayerTargetY,
                                     gameData.hallwayVisibilityDistance,
                                     gameData.visibilityMap);
                  }
                } else {
                  SDL_Log("Player move blocked (Wall or OOB) at (%d, %d)",
                          newPlayerTargetX, newPlayerTargetY);
                  // Optional: Add a 'bump' sound effect
                  // Bumping into a wall doesn't end the turn
                }
              } // End movement processing

              // --- End Turn if an Instant Action Occurred ---
              if (actionTaken) {
                // --- Start of Enemy Turn Initialization ---
                gameData.currentGameState = GameState::EnemyTurn;
                gameData.enemiesActingThisTurn = 0; // Start count at 0
                for (auto &enemy : gameData.enemies) {
                  if (enemy.health > 0) { // Only count living enemies
                    enemy.hasTakenActionThisTurn =
                        false; // Reset action flag for the new turn
                    gameData.enemiesActingThisTurn++; // Increment counter for
                                                      // living enemies
                  }
                }
                gameData.currentEnemyUpdateIndex = 0; // Reset update index
                SDL_Log("Entering Enemy Turn. %d enemies acting.",
                        gameData.enemiesActingThisTurn);
              }

            } // End of scope for variables
          } // End Normal Player Turn Input Handling block
        } // End keydown check
      } // End !isMoving check
      break; // End PlayerTurn case

    case GameState::PlayerTargeting:
      if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_UP:
        case SDLK_KP_8:
          gameData.targetIndicatorY--;
          break;
        case SDLK_DOWN:
        case SDLK_KP_2:
          gameData.targetIndicatorY++;
          break;
        case SDLK_LEFT:
        case SDLK_KP_4:
          gameData.targetIndicatorX--;
          break;
        case SDLK_RIGHT:
        case SDLK_KP_6:
          gameData.targetIndicatorX++;
          break;
        case SDLK_KP_7: // Numpad 7 (Up-Left)
          gameData.targetIndicatorX--;
          gameData.targetIndicatorY--;
          break;
        case SDLK_KP_9: // Numpad 9 (Up-Right)
          gameData.targetIndicatorX++;
          gameData.targetIndicatorY--;
          break;
        case SDLK_KP_1: // Numpad 1 (Down-Left)
          gameData.targetIndicatorX--;
          gameData.targetIndicatorY++;
          break;
        case SDLK_KP_3: // Numpad 3 (Down-Right)
          gameData.targetIndicatorX++;
          gameData.targetIndicatorY++;
          break;

        case SDLK_RETURN: // Confirm Spell Target
        {                 // Scope needed
          // Ensure a spell is actually selected
          if (gameData.currentSpellIndex < 0 ||
              gameData.currentSpellIndex >=
                  (int)gameData.currentGamePlayer.knownSpells
                      .size()) { // Cast to int
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Targeting confirmed with invalid spell index: %d",
                         gameData.currentSpellIndex);
            gameData.currentGameState = GameState::PlayerTurn; // Go back safely
            break;
          }

          const Spell &spell = gameData.currentGamePlayer.getSpell(
              gameData.currentSpellIndex); // Get selected spell info

          // Calculate distance using Manhattan distance for grid movement/range
          int distance = std::abs(gameData.currentGamePlayer.targetTileX -
                                  gameData.targetIndicatorX) +
                         std::abs(gameData.currentGamePlayer.targetTileY -
                                  gameData.targetIndicatorY);

          if (distance <= spell.range) { // Check range again
            // Attempt to cast the spell
            SDL_Texture *texToUse = nullptr;
            // Determine which texture based on the selected spell's iconName
            texToUse = assets.getTexture(
                spell.name); // Use the icon name directly (assuming projectile
                             // uses same base name)
            // Example: If icon is "fireball_icon", maybe projectile is
            // "fireball"? This might need adjustment based on your asset naming
            // convention. For now, let's assume the icon texture works as a
            // placeholder or is the right one.
            if (!texToUse) {
              // Fallback or specific projectile texture lookup
              if (spell.name == "Fireball") { // Example specific lookup
                texToUse = assets.getTexture(
                    "fireball"); // Use the dedicated projectile texture
              }
            }

            if (!texToUse && spell.targetType != SpellTargetType::Self) {
              SDL_LogWarn(
                  SDL_LOG_CATEGORY_APPLICATION,
                  "No suitable texture found for projectile of spell '%s'",
                  spell.name.c_str());
              // Proceed without texture? Or cancel cast? For now, proceed.
            }

            // Store projectile count *before* casting
            size_t projectilesBeforeCast = gameData.activeProjectiles.size();

            if (gameData.currentGamePlayer.castSpell(
                    gameData.currentSpellIndex, gameData.targetIndicatorX,
                    gameData.targetIndicatorY, gameData.enemies,
                    gameData.activeProjectiles, texToUse)) {
              // Spell cast SUCCESS!
              // Check if projectiles were *actually* added this frame
              bool projectileCreated =
                  (gameData.activeProjectiles.size() > projectilesBeforeCast);

              if (projectileCreated) {
                gameData.currentGameState = GameState::ProjectileResolution;
                SDL_Log("Entering Projectile Resolution state.");
              } else {
                // If cast was successful but no projectiles (e.g., self-heal,
                // instant area effect), go directly to EnemyTurn
                gameData.currentGameState = GameState::EnemyTurn;
                gameData.enemiesActingThisTurn = 0;
                for (auto &enemy : gameData.enemies) {
                  if (enemy.health > 0) {
                    enemy.hasTakenActionThisTurn = false;
                    gameData.enemiesActingThisTurn++;
                  }
                }
                gameData.currentEnemyUpdateIndex = 0;
                SDL_Log("Entering Enemy Turn (after instant spell). %d enemies "
                        "acting.",
                        gameData.enemiesActingThisTurn);
              }
              gameData.currentSpellIndex = -1; // Reset selected spell
            } else {
              // Casting failed (e.g., no valid target found by castSpell
              // implementation, though mana/range checked)
              SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                          "Spell cast failed in targeting mode.");
              gameData.currentGameState =
                  GameState::PlayerTurn; // Go back to player turn
              gameData.currentSpellIndex = -1;
            }
          } else {
            SDL_Log("Target out of range for spell '%s'.", spell.name.c_str());
            // Stay in targeting mode
            // Optional: Play 'fail' sound
          }
        } // End scope
        break; // End SDLK_RETURN case

        case SDLK_ESCAPE: // Cancel Targeting
          gameData.currentGameState =
              GameState::PlayerTurn;       // Go back to player turn
          gameData.currentSpellIndex = -1; // Reset selected spell
          SDL_Log("Targeting cancelled.");
          break; // End SDLK_ESCAPE case

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
        SDL_Log("Input ignored during projectile resolution.");
      }
      break; // End ProjectileResolution case

    case GameState::GameOver:
      // Input to restart or quit?
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN) {
          // Implement proper game reset logic - REMOVE assignment
          // gameData = GameData{}; // <-- REMOVED

          // 1. Reset Game State
          gameData.currentGameState =
              GameState::MainMenu; // Go back to main menu

          // 2. Reset Player State (using constructor creates a fresh player)
          gameData.currentGamePlayer = PlayerCharacter(
              CharacterType::FemaleMage, 0, 0, gameData.tileWidth,
              gameData.tileHeight); // Reset to default

          // 3. Clear dynamic game data
          gameData.enemies.clear();
          gameData.activeProjectiles.clear();
          gameData.levelRooms.clear();
          gameData.visibilityMap.clear();
          gameData.occupationGrid.clear();
          gameData.currentLevel = Level{}; // Reset level structure

          // 4. Reset UI/Menu states
          gameData.currentMenu = GameMenu::None;
          gameData.selectedIndex = 0; // Main menu index
          gameData.selectedCharacterIndex = 0;
          gameData.spellSelectIndex = 0;
          gameData.currentSpellIndex = -1;
          gameData.isPanning = false;
          gameData.splashPanOffset = 456; // Reset splash
          gameData.panCounter = 0;
          gameData.isCharacterSelectFadingIn = false;
          gameData.characterSelectAlpha = 0;
          gameData.hasCharacterSelectStartedFading = false;
          gameData.targetIndicatorX = 0;
          gameData.targetIndicatorY = 0;
          gameData.cameraX = 0; // Reset camera
          gameData.cameraY = 0;
          gameData.currentLevelIndex = 1; // Reset level index

          // 5. Reset turn counters
          gameData.enemiesActingThisTurn = 0;
          gameData.currentEnemyUpdateIndex = 0;

          // 6. Renderer is NOT reset here, it's managed by main/sdlContext
          // gameData.renderer = context.renderer; // <-- REMOVED

          SDL_Log("Restarting game... returning to Main Menu.");

        } else if (event.key.keysym.sym == SDLK_ESCAPE) {
          running = false;
        }
      }
      break; // End GameOver case

    } // End GameState switch (within event polling)
  } // End SDL_PollEvent loop
}

void updateLogic(GameData &gameData, float deltaTime) {

  // --- Update Panning/Fades (Menu -> Character Select) ---
  if (gameData.isPanning) {
    gameData.panCounter += 10;      // Adjust speed as needed
    gameData.splashPanOffset -= 10; // Adjust speed as needed
    if (gameData.splashPanOffset <= 0) {
      gameData.splashPanOffset = 0;
      gameData.isPanning = false;
      gameData.currentGameState = GameState::CharacterSelect;
      gameData.isCharacterSelectFadingIn = true;
      gameData.characterSelectAlpha = 0;
      gameData.hasCharacterSelectStartedFading = true;
      SDL_Log("Panning finished, entering Character Select.");
    }
  }
  if (gameData.isCharacterSelectFadingIn) {
    int newAlpha = static_cast<int>(gameData.characterSelectAlpha) +
                   10; // Adjust fade speed
    if (newAlpha >= 255) {
      gameData.characterSelectAlpha = 255;
      gameData.isCharacterSelectFadingIn = false;
      SDL_Log("Character Select fade-in complete.");
    } else {
      gameData.characterSelectAlpha = static_cast<Uint8>(newAlpha);
    }
  }

  // --- Main Game State Update Logic ---
  switch (gameData.currentGameState) {
  case GameState::PlayerTurn:
    // Update player movement *first*
    if (gameData.currentGamePlayer.isMoving) {
      bool playerWasMoving = true; // Check state *before* update
      gameData.currentGamePlayer.update(deltaTime, gameData); // Pass GameData
      bool playerIsNowIdle =
          !gameData.currentGamePlayer.isMoving; // Check state *after* update

      // Check if movement finished AND menu is closed
      if (playerWasMoving && playerIsNowIdle &&
          gameData.currentMenu == GameMenu::None) {

        // Check if the player landed on the exit tile
        if (gameData.currentGamePlayer.targetTileX ==
                gameData.currentLevel.endCol &&
            gameData.currentGamePlayer.targetTileY ==
                gameData.currentLevel.endRow) {
          SDL_Log("Player reached exit tile! Advancing to next level.");
          gameData.currentLevelIndex++;
          gameData.enemies.clear();           // Clear enemies for new level
          gameData.activeProjectiles.clear(); // Clear projectiles

          // Generate New Level
          gameData.currentLevel = generateLevel(
              gameData.levelWidth, gameData.levelHeight, gameData.levelMaxRooms,
              gameData.levelMinRoomSize, gameData.levelMaxRoomSize,
              gameData.enemies, gameData.tileWidth,
              gameData
                  .tileHeight); // Pass level index if needed by generateLevel
          gameData.levelRooms = gameData.currentLevel.rooms;

          // Initialize Occupation Grid for new level
          gameData.occupationGrid.assign(
              gameData.currentLevel.height,
              std::vector<bool>(gameData.currentLevel.width, false));
          for (int y = 0; y < gameData.currentLevel.height; ++y) {
            for (int x = 0; x < gameData.currentLevel.width; ++x) {
              if (gameData.currentLevel.tiles[y][x] == '#') {
                gameData.occupationGrid[y][x] = true;
              }
            }
          }

          // Reset Player Position to New Start
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

          // Mark new player position on grid (AFTER setting it)
          if (isWithinBounds(gameData.currentGamePlayer.targetTileX,
                             gameData.currentGamePlayer.targetTileY,
                             gameData.currentLevel.width,
                             gameData.currentLevel.height)) {
            gameData.occupationGrid[gameData.currentGamePlayer.targetTileY]
                                   [gameData.currentGamePlayer.targetTileX] =
                true;
          } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "New level player position (%d, %d) is out of bounds!",
                         gameData.currentGamePlayer.targetTileX,
                         gameData.currentGamePlayer.targetTileY);
          }

          // Mark initial enemy positions
          for (const auto &enemy : gameData.enemies) {
            if (isWithinBounds(enemy.x, enemy.y, gameData.currentLevel.width,
                               gameData.currentLevel.height)) {
              gameData.occupationGrid[enemy.y][enemy.x] = true;
            } else {
              SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                          "New level enemy position (%d, %d) is out of bounds!",
                          enemy.x, enemy.y);
            }
          }

          // Reset Visibility
          gameData.visibilityMap.assign(
              gameData.currentLevel.height,
              std::vector<float>(gameData.currentLevel.width, 0.0f));
          updateVisibility(gameData.currentLevel, gameData.levelRooms,
                           gameData.currentGamePlayer.targetTileX,
                           gameData.currentGamePlayer.targetTileY,
                           gameData.hallwayVisibilityDistance,
                           gameData.visibilityMap);

          // Remain in PlayerTurn state for the new level
          SDL_Log("New level %d generated. Player starting at (%d, %d).",
                  gameData.currentLevelIndex,
                  gameData.currentGamePlayer.targetTileX,
                  gameData.currentGamePlayer.targetTileY);

        } else {
          // --- Player moved, NOT on exit: Transition to Enemy Turn ---
          // --- Start of Enemy Turn Initialization ---
          gameData.currentGameState = GameState::EnemyTurn;
          gameData.enemiesActingThisTurn = 0;
          for (auto &enemy : gameData.enemies) {
            if (enemy.health > 0) {
              enemy.hasTakenActionThisTurn = false;
              gameData.enemiesActingThisTurn++;
            }
          }
          gameData.currentEnemyUpdateIndex = 0;
          SDL_Log("Entering Enemy Turn (after Player move). %d enemies acting.",
                  gameData.enemiesActingThisTurn);
          // --- End Enemy Turn Initialization ---
        }
      } // End if player finished moving
    } // End if player is moving block
    // Do NOT update projectiles or enemies in PlayerTurn state
    break; // End PlayerTurn case

  case GameState::ProjectileResolution: {
    bool projectilesStillActive = false;
    // std::vector<Enemy *> hitEnemiesThisFrame; // Track enemies hit to apply
    // damage once - simpler to apply directly for now

    for (auto &proj : gameData.activeProjectiles) {
      if (proj.isActive) {
        if (proj.update(deltaTime)) { // update returns true on hit/arrival
          // Projectile reached target position this frame
          int hitX = static_cast<int>(
              floor(proj.targetX / gameData.tileWidth)); // Use floor
          int hitY = static_cast<int>(
              floor(proj.targetY / gameData.tileHeight)); // Use floor

          SDL_Log("Projectile arrived at target tile (%d, %d).", hitX, hitY);

          // Apply damage to enemy at hit location (if any)
          bool enemyHit = false;
          for (auto &enemy : gameData.enemies) {
            // Check logical tile coordinates AND health
            if (enemy.x == hitX && enemy.y == hitY && enemy.health > 0) {
              enemy.takeDamage(proj.damage);
              SDL_Log("Enemy at (%d, %d) hit by projectile for %d damage. "
                      "Health: %d",
                      enemy.x, enemy.y, proj.damage, enemy.health);
              enemyHit = true;
              // Don't mark action as complete here, let the main loop handle
              // death checks
              break; // Assume one hit per projectile? Adjust if AoE needed
            }
          }
          if (!enemyHit) {
            SDL_Log("Projectile hit tile (%d, %d), but no enemy was there.",
                    hitX, hitY);
          }
          proj.isActive = false; // Deactivate projectile after hit/arrival
        } else {
          projectilesStillActive = true; // At least one projectile still moving
        }
      }
    }

    // Remove inactive projectiles AFTER iterating
    gameData.activeProjectiles.erase(
        std::remove_if(gameData.activeProjectiles.begin(),
                       gameData.activeProjectiles.end(),
                       [](const Projectile &p) { return !p.isActive; }),
        gameData.activeProjectiles.end());

    // Check if resolution finished (no active projectiles left)
    if (!projectilesStillActive) {
      SDL_Log("Projectile resolution finished.");
      // --- Transition to Enemy Turn ---
      // --- Start of Enemy Turn Initialization ---
      gameData.currentGameState = GameState::EnemyTurn;
      gameData.enemiesActingThisTurn = 0;
      for (auto &enemy : gameData.enemies) {
        if (enemy.health > 0) {
          enemy.hasTakenActionThisTurn = false;
          gameData.enemiesActingThisTurn++;
        }
      }
      gameData.currentEnemyUpdateIndex = 0;
      SDL_Log("Entering Enemy Turn (after projectiles). %d enemies acting.",
              gameData.enemiesActingThisTurn);
      // --- End Enemy Turn Initialization ---
    }
    break; // End ProjectileResolution case
  } // End scope for ProjectileResolution case

  case GameState::EnemyTurn: {
    // Ensure counter doesn't stay negative if something went wrong
    if (gameData.enemiesActingThisTurn < 0) {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "Enemy acting counter was %d, resetting to 0.",
                  gameData.enemiesActingThisTurn);
      gameData.enemiesActingThisTurn = 0;
    }

        // --- 1. Time-Sliced Enemy Updates (Movement Animation) ---
        // Keep this loop for smooth visuals of ongoing moves
        int N = gameData.enemies.size();
        if (N > 0) {
            for (int i = 0; i < N; ++i) {
                if (gameData.enemies[i].health > 0 && gameData.enemies[i].isMoving) {
                    gameData.enemies[i].update(deltaTime, gameData);
                }
            }
        }

        // --- 2. Process Enemy AI Actions (REVISED - Full Iteration) ---
        // Iterate through ALL enemies once per frame update during EnemyTurn
        if (N > 0) { // Only loop if enemies exist
            for (int i = 0; i < N; ++i) {
                Enemy& currentEnemy = gameData.enemies[i];

                // Check if this specific enemy needs to decide its action THIS frame
                if (currentEnemy.health > 0 && !currentEnemy.hasTakenActionThisTurn && !currentEnemy.isMoving) {
                    // Log before calling, useful for debugging
                    SDL_Log("AI Processing: Attempting action for Enemy Index %d at [%d,%d]", i, currentEnemy.x, currentEnemy.y);

                    // Call takeAction. It handles instant/timed actions & counter decrements internally.
                    currentEnemy.takeAction(
                        gameData.currentLevel, gameData.currentGamePlayer, gameData
                    );
                    // We no longer break the loop early based on the counter or an update limit.
                }
            } // End FOR loop iterating through all enemies for AI decision
        } // End if N > 0 check

    // --- 3. Process and Remove Dead Enemies, Grant Arcana ---
    // Moved inside EnemyTurn, runs every frame during enemy turn
    if (!gameData.enemies.empty()) {
      // Use remove_if idiom for cleaner removal
      int arcanaGainedThisTurn = 0;
      gameData.enemies.erase(
          std::remove_if(
              gameData.enemies.begin(), gameData.enemies.end(),
              [&](Enemy &e) { // Capture gameData by reference
                if (e.health <= 0) {
                  SDL_Log("Enemy defeated at (%d, %d), granting %d Arcana.",
                          e.x, e.y, e.arcanaValue);
                  arcanaGainedThisTurn += e.arcanaValue;

                  // Update Occupation Grid
                  if (isWithinBounds(e.x, e.y, gameData.currentLevel.width,
                                     gameData.currentLevel.height)) {
                    if (gameData.occupationGrid[e.y][e.x]) {
                      gameData.occupationGrid[e.y][e.x] = false;
                      SDL_Log("Cleared occupation grid at (%d, %d).", e.x, e.y);
                    } else {
                      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                  "Attempted to clear unoccupied grid cell "
                                  "(%d, %d) for dead enemy.",
                                  e.x, e.y);
                    }
                  } else {
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                "Dead enemy was out of bounds (%d, %d)? Grid "
                                "not updated.",
                                e.x, e.y);
                  }

                  // *** REVISED Counter Decrement Logic for Dead Enemies ***
                  // Decrement if the enemy died before its action fully
                  // resolved (i.e., it never started, OR it started a move but
                  // didn't finish)
                  bool actionResolved =
                      !e.isMoving &&
                      e.hasTakenActionThisTurn; // True if instant action done
                                                // OR move finished
                  if (!actionResolved) {
                    if (gameData.enemiesActingThisTurn > 0) {
                      gameData.enemiesActingThisTurn--;
                      SDL_Log("Dead enemy action unresolved, decrementing "
                              "counter. Remaining actors: %d",
                              gameData.enemiesActingThisTurn);
                    } else {
                      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                  "Dead enemy action unresolved, but counter "
                                  "already 0?");
                    }
                  }
                  return true; // Mark for removal
                }
                return false; // Keep alive enemies
              }),
          gameData.enemies.end());

      if (arcanaGainedThisTurn > 0) {
        gameData.currentGamePlayer.GainArcana(arcanaGainedThisTurn);
      }

      // Safety check: Ensure acting counter doesn't exceed remaining enemies
      // AFTER removal
      if (gameData.enemiesActingThisTurn > (int)gameData.enemies.size()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Acting counter %d higher than enemy count %zu after "
                    "removal/death check. Resetting.",
                    gameData.enemiesActingThisTurn, gameData.enemies.size());

        // Recalculate based on remaining living enemies who haven't acted
        int actualActing = 0;
        for (const auto &enemy : gameData.enemies) {
          if (enemy.health > 0 && !enemy.hasTakenActionThisTurn) {
            actualActing++;
          }
        }
        if (gameData.enemiesActingThisTurn != actualActing) {
          SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                      "Correcting acting counter from %d to %d",
                      gameData.enemiesActingThisTurn, actualActing);
          gameData.enemiesActingThisTurn = actualActing;
        } else {
          // If the counter matches the recalculation, maybe the issue was just
          // timing
          SDL_Log("Counter %d matches actual acting %d after recalculation.",
                  gameData.enemiesActingThisTurn, actualActing);
        }
      }
    } // End if enemies not empty

    // --- 4. Check for Turn Completion using Counter ---
    if (gameData.enemiesActingThisTurn <= 0) {
      // First, check if any enemy is STILL moving visibly
      bool anyEnemyStillMoving = false;
      if (N > 0) { // Check if there are any enemies to iterate over
        for (const auto &enemy : gameData.enemies) {
          // We only care about living enemies that might be moving
          if (enemy.health > 0 && enemy.isMoving) {
            anyEnemyStillMoving = true;
            // SDL_Log("Enemy Turn End Delay: Enemy at (%d, %d) is still
            // moving.", enemy.x, enemy.y); // Optional debug
            break; // Found one moving enemy, no need to check further
          }
        }
      } // End check N > 0
      if (!anyEnemyStillMoving) {
        SDL_Log("Enemy Turn Ends. Counter: %d. No enemies moving.", gameData.enemiesActingThisTurn);

      // Check for Player Death FIRST
      if (gameData.currentGamePlayer.health <= 0) {
        gameData.currentGameState = GameState::GameOver;
        SDL_Log("--- Game Over ---");
      } else {
        // Attempt Enemy Spawn (Optimized Approach)
        if (gameData.enemies.size() < gameData.maxEnemyCount &&
            gameData.spawnChancePercent > 0 &&
            !gameData.levelRooms.empty()) { // Check if rooms exist
          if ((rand() % 100) < gameData.spawnChancePercent) {

            std::pair<int, int> spawnPos = {-1, -1}; // Sentinel value
            int maxSpawnAttempts = gameData.levelRooms.size() *
                                   2; // Try roughly twice the number of rooms

            for (int attempt = 0; attempt < maxSpawnAttempts; ++attempt) {
              // 1. Pick a random room
              int roomIndex = rand() % gameData.levelRooms.size();
              const SDL_Rect &room = gameData.levelRooms[roomIndex];

              // 2. Pick a random tile *within* the room's floor area
              if (room.w <= 2 || room.h <= 2)
                continue; // Skip degenerate rooms
              int potentialX = room.x + 1 + (rand() % std::max(1, room.w - 2));
              int potentialY = room.y + 1 + (rand() % std::max(1, room.h - 2));

              // 3. Check if this specific tile is valid (bounds, visibility,
              // occupation, player pos)
              bool occSafe =
                  isWithinBounds(potentialX, potentialY,
                                 gameData.currentLevel.width,
                                 gameData.currentLevel.height)
                      ? !gameData.occupationGrid[potentialY][potentialX]
                      : false;
              bool visSafe =
                  isWithinBounds(potentialX, potentialY,
                                 gameData.currentLevel.width,
                                 gameData.currentLevel.height)
                      ? (potentialY < (int)gameData.visibilityMap.size() &&
                         potentialX <
                             (int)gameData.visibilityMap[potentialY].size() &&
                         gameData.visibilityMap[potentialY][potentialX] <= 0.0f)
                      : true; // Assume not visible if OOB

              if (visSafe && occSafe &&
                  (potentialX != gameData.currentGamePlayer.targetTileX ||
                   potentialY != gameData.currentGamePlayer.targetTileY)) {
                spawnPos = {potentialX, potentialY};
                break; // Found a valid spot!
              }
            } // End spawn attempt loop

            if (spawnPos.first != -1) { // Check if a valid spot was found
              int spawnX = spawnPos.first;
              int spawnY = spawnPos.second;

              // *** MODIFIED: Use new Enemy constructor with EnemyType ***
              gameData.enemies.emplace_back(EnemyType::SLIME, spawnX, spawnY,
                                            gameData.tileWidth,
                                            gameData.tileHeight);
              // ---------------------------------------------------------

              if (isWithinBounds(spawnX, spawnY, gameData.currentLevel.width,
                                 gameData.currentLevel.height)) {
                gameData.occupationGrid[spawnY][spawnX] = true;
              } else {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                             "Enemy spawned out of bounds at (%d, %d)? Grid "
                             "not updated.",
                             spawnX, spawnY);
              }
              SDL_Log("Reinforcement spawned at (%d, %d). Total: %zu", spawnX,
                      spawnY, gameData.enemies.size());

            } else {
              SDL_Log("No valid spawn location found after %d attempts.",
                      maxSpawnAttempts);
            }
          } // End spawn chance check
        }

        // --- Transition to Player Turn ---
        gameData.currentGamePlayer.RegenerateMana(
            1.0f); // Regenerate 1 turn's worth
        gameData.currentGameState = GameState::PlayerTurn;
        // Update visibility based on player's *current* position
        updateVisibility(gameData.currentLevel, gameData.levelRooms,
                         gameData.currentGamePlayer.targetTileX,
                         gameData.currentGamePlayer.targetTileY,
                         gameData.hallwayVisibilityDistance,
                         gameData.visibilityMap);
        SDL_Log("Entering Player Turn.");
      } // End else (Player is not dead)
    }else {}
    } // End turn completion check

    break; // End EnemyTurn case
  } // End scope for EnemyTurn case vars

  // Other cases (PlayerTargeting, GameOver, MainMenu, CharacterSelect)
  case GameState::PlayerTargeting:
  case GameState::GameOver:
  case GameState::MainMenu:
  case GameState::CharacterSelect:
    // No entity updates needed in these states
    break;

  } // End Gameplay State Update block (switch statement)

  // --- Update Camera ---
  // Update camera only during gameplay states
  if (gameData.currentGameState == GameState::PlayerTurn ||
      gameData.currentGameState == GameState::PlayerTargeting ||
      gameData.currentGameState == GameState::ProjectileResolution ||
      gameData.currentGameState == GameState::EnemyTurn) {
    // Ensure level dimensions are valid before calculating max camera bounds
    // Also ensure tile dimensions are positive
    if (gameData.currentLevel.width > 0 && gameData.currentLevel.height > 0 &&
        gameData.tileWidth > 0 && gameData.tileHeight > 0) {
      int halfWidth = gameData.windowWidth / 2;
      int halfHeight = gameData.windowHeight / 2;
      // Center camera on player's *visual* position for smooth following
      int idealCameraX =
          static_cast<int>(gameData.currentGamePlayer.x) - halfWidth;
      int idealCameraY =
          static_cast<int>(gameData.currentGamePlayer.y) - halfHeight;

      // Calculate maximum camera coordinates to prevent showing area outside
      // the level
      int maxCameraX = (gameData.currentLevel.width * gameData.tileWidth) -
                       gameData.windowWidth;
      int maxCameraY = (gameData.currentLevel.height * gameData.tileHeight) -
                       gameData.windowHeight;

      // Clamp camera position to stay within level boundaries
      gameData.cameraX = std::max(0, std::min(idealCameraX, maxCameraX));
      gameData.cameraY = std::max(0, std::min(idealCameraY, maxCameraY));

      // Handle cases where the level is smaller than the window
      if (maxCameraX < 0)
        gameData.cameraX = 0;
      if (maxCameraY < 0)
        gameData.cameraY = 0;

    } else {
      // Default camera position if level data is invalid or not loaded yet
      gameData.cameraX = 0;
      gameData.cameraY = 0;
      if (gameData.currentGameState != GameState::MainMenu &&
          gameData.currentGameState != GameState::CharacterSelect) {
        // Only log warning if we are supposed to be in a gameplay state
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Invalid level/tile dimensions for camera calculation. "
                    "LevelW:%d H:%d TileW:%d H:%d",
                    gameData.currentLevel.width, gameData.currentLevel.height,
                    gameData.tileWidth, gameData.tileHeight);
      }
    }
  }

} // End updateLogic function definition

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
    // Check if level dimensions and tiles are valid before rendering
    if (gameData.currentLevel.width > 0 && gameData.currentLevel.height > 0 &&
        !gameData.currentLevel.tiles.empty() && gameData.tileWidth > 0 &&
        gameData.tileHeight > 0) {

      SDL_Texture *wallTexture = assets.getTexture("wall_texture");
      SDL_Texture *startTexture =
          assets.getTexture("start_tile"); // Keep existing ones
      SDL_Texture *exitTexture = assets.getTexture("exit_tile");

      std::vector<SDL_Texture *> floorTextures = {
          assets.getTexture("floor_1"), assets.getTexture("floor_2")
          // Add more variants retrieved from assets here
      };
      // Remove nullptrs from floorTextures if loading failed
      floorTextures.erase(
          std::remove(floorTextures.begin(), floorTextures.end(), nullptr),
          floorTextures.end());

      // Define weights for each texture in the floorTextures vector
      // Example: 70% floor_1, 20% floor_2, 10% floor_dirt
      std::vector<double> floorWeights = {
          3.0, 7.0}; // Weights should match the number of *valid* textures

      // Ensure weights match texture count
      if (floorWeights.size() != floorTextures.size() &&
          !floorTextures.empty()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Floor texture/weight mismatch. Resetting weights.");
        floorWeights.assign(floorTextures.size(),
                            1.0); // Assign equal weight if mismatch
      }

      // Calculate total weight and cumulative weights for selection
      double totalWeight = 0;
      for (double w : floorWeights) {
        totalWeight += w;
      }
      std::vector<double> cumulativeWeights;
      double currentCumulative = 0;
      if (totalWeight > 0) { // Avoid division by zero if no weights/textures
        for (size_t i = 0; i < floorWeights.size(); ++i) {
          currentCumulative += floorWeights[i];
          cumulativeWeights.push_back(currentCumulative /
                                      totalWeight); // Normalize to 0-1 range
        }
      }

      // Determine visible tile range based on camera
      int startTileX = std::max(0, gameData.cameraX / gameData.tileWidth);
      int startTileY = std::max(0, gameData.cameraY / gameData.tileHeight);
      int endTileX = std::min(
          gameData.currentLevel.width,
          (gameData.cameraX + gameData.windowWidth) / gameData.tileWidth + 1);
      int endTileY = std::min(
          gameData.currentLevel.height,
          (gameData.cameraY + gameData.windowHeight) / gameData.tileHeight + 1);

      for (int y = startTileY; y < endTileY; ++y) {
        for (int x = startTileX; x < endTileX; ++x) {
          // Bounds check for safety, though loops should handle it
          if (!isWithinBounds(x, y, gameData.currentLevel.width,
                              gameData.currentLevel.height))
            continue;
          // Additional check for visibility map bounds
          if (y >= (int)gameData.visibilityMap.size() ||
              x >= (int)gameData.visibilityMap[y].size())
            continue;

          SDL_Rect tileRect = {(x * gameData.tileWidth) - gameData.cameraX,
                               (y * gameData.tileHeight) - gameData.cameraY,
                               gameData.tileWidth, gameData.tileHeight};
          float visibility =
              gameData
                  .visibilityMap[y][x]; // Access directly after bounds check

          if (visibility > 0.0f) { // Only render visible tiles
            SDL_Texture *textureToRender = nullptr;
            bool isFloor = false; // Flag to check if we need floor logic

            // --- Determine Base Texture ---
            // Check tile type bounds first
            if (y >= (int)gameData.currentLevel.tiles.size() ||
                x >= (int)gameData.currentLevel.tiles[y].size())
              continue;
            char currentTileType = gameData.currentLevel.tiles[y][x];

            if (y == gameData.currentLevel.startRow &&
                x == gameData.currentLevel.startCol &&
                startTexture) { // Check coordinates AND texture
              textureToRender = startTexture;
            } else if (y == gameData.currentLevel.endRow &&
                       x == gameData.currentLevel.endCol &&
                       exitTexture) { // Check coordinates AND texture
              textureToRender = exitTexture;
            } else if (currentTileType == '#' &&
                       wallTexture) { // Check tile type AND texture
              textureToRender = wallTexture;
            } else if (currentTileType == '.') { // Check tile type
              // It's a floor tile, flag it for variant selection below
              isFloor = true;
            }

            // --- Floor Variant Selection (if isFloor is true) ---
            if (isFloor && !floorTextures.empty() && totalWeight > 0 &&
                !cumulativeWeights.empty()) {
              // 1. Simple deterministic hash based on coordinates
              unsigned int hash = ((unsigned int)x * 2654435761u) ^
                                  ((unsigned int)y * 3063691763u);
              double hashValue = (double)(hash % 10000) /
                                 10000.0; // Scale hash to [0, 1) range

              // 2. Select texture based on cumulative weights
              for (size_t i = 0; i < cumulativeWeights.size(); ++i) {
                if (hashValue <= cumulativeWeights[i]) {
                  textureToRender = floorTextures[i]; // Select this texture
                  break;                              // Found our texture
                }
              }
              // Fallback if something went wrong (e.g., hash > 1 somehow, or no
              // cumulative weights calculated)
              if (!textureToRender) {
                textureToRender =
                    floorTextures[0]; // Fallback to first loaded floor texture
              }

            } else if (isFloor) {
              // Fallback if no floor textures loaded or weights defined
              textureToRender = nullptr; // Will trigger color fallback below
            }

            // --- Render Selected Texture or Fallback Color ---
            if (textureToRender != nullptr) {
              SDL_RenderCopy(gameData.renderer, textureToRender, nullptr,
                             &tileRect);
            } else {
              // Fallback color rendering
              Uint8 r = 50, g = 50, b = 50; // Default dark grey
              if (currentTileType == '#') {
                r = 139;
                g = 69;
                b = 19;
              } // Brown for walls
              else if (currentTileType == '.') {
                r = 100;
                g = 100;
                b = 100;
              } // Grey for floor
              else {
                r = 5;
                g = 5;
                b = 5;
              } // Very dark grey for void/other

              SDL_SetRenderDrawColor(gameData.renderer, r, g, b, 255);
              SDL_RenderFillRect(gameData.renderer, &tileRect);
            }

            // Apply visibility dimming overlay
            Uint8 alpha = static_cast<Uint8>((1.0f - visibility) *
                                             200); // Adjust dimming intensity
            SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(gameData.renderer, 0, 0, 0,
                                   alpha); // Black overlay
            SDL_RenderFillRect(gameData.renderer, &tileRect);
            SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_NONE);

          } else {
            // Optionally render previously explored tiles dimly (Fog of War)
            // For now, just render black for unseen tiles
            // Check if tile was *ever* visible (needs another map, e.g.,
            // exploredMap) If exploredMap[y][x] is true:
            //    SDL_SetRenderDrawColor(gameData.renderer, 20, 20, 20, 255); //
            //    Dim grey for explored SDL_RenderFillRect(gameData.renderer,
            //    &tileRect);
            // Else:
            SDL_SetRenderDrawColor(gameData.renderer, 0, 0, 0,
                                   255); // Black for unexplored
            SDL_RenderFillRect(gameData.renderer, &tileRect);
          }
        } // End x loop
      } // End y loop
    } else {
      // Optionally render a placeholder if level is not loaded/invalid
      if (gameData.currentGameState != GameState::MainMenu &&
          gameData.currentGameState != GameState::CharacterSelect) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Attempting to render invalid level.");
        // Render simple text indicating loading or error?
        // renderText(gameData.renderer, assets.getFont("main_font"), "Loading
        // Level...", {255,255,255,255}, 10, 10);
      }
    } // End level rendering

    // --- Render Entities (Player, Enemies, Projectiles) ---
    // Check if gameplay state before rendering entities that depend on level
    // data
    if (gameData.currentGameState != GameState::MainMenu &&
        gameData.currentGameState != GameState::CharacterSelect) {
      // --- Render Enemies ---
      for (const auto &enemy : gameData.enemies) {
        if (enemy.health > 0) {     // Only render alive enemies
          int enemyTileX = enemy.x; // Use logical position for visibility check
          int enemyTileY = enemy.y;
          float visibility = 0.0f;
          // Check bounds before accessing visibility map
          if (isWithinBounds(enemyTileX, enemyTileY,
                             gameData.currentLevel.width,
                             gameData.currentLevel.height) &&
              enemyTileY < (int)gameData.visibilityMap
                               .size() && // Check map bounds correctly
              enemyTileX < (int)gameData.visibilityMap[enemyTileY].size()) {
            visibility = gameData.visibilityMap[enemyTileY][enemyTileX];
          }

          if (visibility > 0.0f) {
            enemy.render(gameData.renderer, assets, gameData.cameraX,
                         gameData.cameraY,
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

      // --- Render Projectiles ---
      for (const auto &proj : gameData.activeProjectiles) {
        // Optionally add visibility check? Should projectiles be visible in
        // FoW? For now, render all active.
        proj.render(gameData.renderer, gameData.cameraX, gameData.cameraY);
      }
    } // End check for gameplay state before rendering entities

    // --- Render Character Sheet or Spell Menu Overlay (If Active) ---
    if (gameData.currentMenu == GameMenu::CharacterSheet) {
      // Render the character sheet if it's the active menu
      renderCharacterSheet(gameData, assets);
    } else if (gameData.currentGameState == GameState::PlayerTurn &&
               gameData.currentMenu == GameMenu::SpellMenu) {
      // Otherwise, render the spell menu only if it's the player's turn and the
      // spell menu is active
      renderSpellMenu(gameData.renderer, assets.getFont("main_font"),
                      gameData.currentGamePlayer, gameData.spellSelectIndex,
                      gameData.windowWidth, gameData.windowHeight);
    }

    // --- Render Targeting Reticle (if targeting) ---
    if (gameData.currentGameState == GameState::PlayerTargeting) {
      // Check if the target tile is visible before rendering reticle
      // Check visibility map bounds correctly
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
            (gameData.targetIndicatorY * gameData.tileHeight) -
                gameData.cameraY,
            gameData.tileWidth, gameData.tileHeight};

        // Ensure spell index is valid before accessing spell data
        if (gameData.currentSpellIndex >= 0 &&
            gameData.currentSpellIndex <
                (int)gameData.currentGamePlayer.knownSpells
                    .size()) { // Cast to int
          const Spell &spell =
              gameData.currentGamePlayer.getSpell(gameData.currentSpellIndex);
          // Use Manhattan distance for range check consistent with targeting
          // logic
          int distance = std::abs(gameData.currentGamePlayer.targetTileX -
                                  gameData.targetIndicatorX) +
                         std::abs(gameData.currentGamePlayer.targetTileY -
                                  gameData.targetIndicatorY);

          Uint8 reticleAlpha = 180; // Base alpha for reticle
          SDL_Texture *reticleTexture = assets.getTexture("reticle");

          // Set color/texture based on range validity
          if (distance <= spell.range) {
            // White or Green if in range
            if (reticleTexture)
              SDL_SetTextureColorMod(reticleTexture, 255, 255,
                                     255); // White tint
            SDL_SetRenderDrawColor(gameData.renderer, 255, 255, 255,
                                   reticleAlpha); // White fallback
          } else {
            // Red if out of range
            if (reticleTexture)
              SDL_SetTextureColorMod(reticleTexture, 255, 100, 100); // Red tint
            SDL_SetRenderDrawColor(gameData.renderer, 255, 0, 0,
                                   reticleAlpha); // Red fallback
          }

          if (reticleTexture) {
            SDL_SetTextureAlphaMod(reticleTexture, reticleAlpha);
            SDL_RenderCopy(gameData.renderer, reticleTexture, nullptr,
                           &reticleRect);
          } else {
            // Fallback: Draw a simple box
            SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderDrawRect(gameData.renderer, &reticleRect);
            SDL_SetRenderDrawBlendMode(gameData.renderer, SDL_BLENDMODE_NONE);
          }
        } else {
          SDL_LogWarn(
              SDL_LOG_CATEGORY_APPLICATION,
              "Attempting to render reticle with invalid spell index: %d",
              gameData.currentSpellIndex);
        }
      } // End visibility check for reticle
    } // End rendering reticle

    // --- Render UI (Only render if not in MainMenu or CharacterSelect) ---
    if (gameData.currentGameState != GameState::MainMenu &&
        gameData.currentGameState != GameState::CharacterSelect) {
      renderSpellBar(gameData, assets);
      renderUI(gameData, assets);
    }
    break; // End gameplay rendering states

  case GameState::GameOver:
    // Render "Game Over" screen
    // TODO: Implement displayGameOver function or similar
    { // Scope for temporary variables
      SDL_Color white = {255, 255, 255, 255};
      TTF_Font *gameOverFont =
          assets.getFont("main_font"); // Use main font or a larger one
      if (!gameOverFont) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Game Over font not loaded!");
        break; // Cannot render text
      }
      SDL_Surface *surface = TTF_RenderText_Solid(
          gameOverFont, "Game Over - Press Enter to Restart", white);
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
        } else {
          SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                       "Failed to create texture for Game Over text.");
        }
        SDL_FreeSurface(surface);
      } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Failed to render Game Over text surface.");
      }
    } // End scope
    break; // End GameOver case

  } // End GameState rendering switch

  SDL_RenderPresent(gameData.renderer);
} // End renderScene function
