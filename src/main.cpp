// main.cpp (Updated for Spellcasting)

#include <iostream>
#include <vector>
#include <string>
#include <cmath>     // For std::abs, std::max, std::min
#include <cstdlib>   // For rand() and srand()
#include <ctime>     // For time()
#include <random>
#include <algorithm> // For std::remove_if

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

#include "character.h" // Includes PlayerCharacter definition (ensure it includes spell.h)
#include "enemy.h"     // Includes Enemy definition (ensure it includes takeDamage declaration)
#include "menu.h"
#include "character_select.h"
#include "level.h"
#include "utils.h"
#include "visibility.h"
#include "ui.h"
#include "projectile.h"
// No need to include spell.h here if character.h already does

#ifdef _WIN32
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "SDL2_ttf.lib")
#pragma comment(lib, "SDL2_image.lib")
// Remove duplicate SDL2_image.lib if present
#endif

enum class GameState {
    MainMenu,
    CharacterSelect,
    // GamePlay, // Can likely be removed
    PlayerTurn,
    PlayerTargeting, // New state for spell targeting
    ProjectileResolution,
    EnemyTurn,
    GameOver
};

enum class GameMenu {
    None, // No menu overlay active
    SpellMenu // Spell selection menu is active
};



// --- Global Game Variables ---
GameState currentGameState = GameState::MainMenu;
std::vector<Enemy> enemies; // Global list of enemies
PlayerCharacter currentGamePlayer(CharacterType::FemaleMage, 100, 100, 150, 150, 1, 0, 0, 64, 64); // Default player, replace in char select
Level currentLevel;
std::vector<SDL_Rect> levelRooms;
std::vector<std::vector<float>> visibilityMap;
GameMenu currentMenu = GameMenu::None; // Track active menu overlay
int spellSelectIndex = 0; // Index of the currently highlighted spell in the menu
std::vector<Projectile> activeProjectiles;

// Game Settings / Constants
int windowWidth = 1920;
int windowHeight = 1080;
int tileWidth = 64;
int tileHeight = 64;
int levelWidth = 120;
int levelHeight = 75;
int levelMaxRooms = 15;
int levelMinRoomSize = 8;
int levelMaxRoomSize = 15;
int hallwayVisibilityDistance = 5;
int currentLevelIndex = 1;

// Camera
int cameraX = 0;
int cameraY = 0;

// Spell Targeting State
bool playerIsTargeting = false; // Should be controlled by GameState now, but can keep as sanity check? Better to rely on state.
int currentSpellIndex = -1;     // Which spell is being targeted
int targetIndicatorX = 0;       // Tile X for targeting reticle
int targetIndicatorY = 0;       // Tile Y for targeting reticle
SDL_Texture* targetingReticleTexture = nullptr; // Optional: Texture for targeting reticle

// --- Main Function ---
int main(int argc, char* argv[]) {
    srand(static_cast<unsigned int>(time(0))); // Seed random number generator

    // --- SDL Initialization ---
    SDL_Context sdlContext = initializeSDL(windowWidth, windowHeight);
    if (sdlContext.window == nullptr || sdlContext.renderer == nullptr || sdlContext.font == nullptr) {
        cleanupSDL(sdlContext);
        return 1;
    }
    SDL_Window* window = sdlContext.window;
    SDL_Renderer* renderer = sdlContext.renderer;
    TTF_Font* font = sdlContext.font; // Use font from context

    // Adjust logical size if needed (optional, but good practice)
    // SDL_RenderSetLogicalSize(renderer, windowWidth, windowHeight);

    // --- Load Assets ---
    // Splash, Menu textures, etc.
    SDL_Texture* splashTexture = IMG_LoadTexture(renderer, "../assets/splash/splash.png");
    SDL_Texture* startTexture = IMG_LoadTexture(renderer, "../assets/textures/start_placeholder.png");
    SDL_Texture* exitTexture = IMG_LoadTexture(renderer, "../assets/textures/exit_placeholder.png");
    targetingReticleTexture = IMG_LoadTexture(renderer, "../assets/sprites/target_reticle.png"); // Load reticle

    SDL_Texture* fireballTexture = IMG_LoadTexture(renderer, "../assets/sprites/fireball_placeholder.png"); // Provide a real path


    // Error checking for loaded textures
    if (!splashTexture) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load splash texture: %s", IMG_GetError());
    if (!startTexture) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load start texture: %s", IMG_GetError());
    if (!exitTexture) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load exit texture: %s", IMG_GetError());
    if (!targetingReticleTexture) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load targeting reticle texture: %s", IMG_GetError()); // Optional error

    // Set blend modes if needed (e.g., for alpha)
    if (splashTexture) SDL_SetTextureBlendMode(splashTexture, SDL_BLENDMODE_BLEND);
    if (targetingReticleTexture) SDL_SetTextureBlendMode(targetingReticleTexture, SDL_BLENDMODE_BLEND);


    // --- Game Loop Variables ---
    SDL_Event event;
    std::vector<std::string> menuItems = {"Start Game", "Options", "Exit"};
    int selectedIndex = 0;
    bool running = true;
    // bool gameStarted = false; // Replaced by GameState checks
    // bool inCharacterSelect = false; // Replaced by GameState checks
    int selectedCharacterIndex = 0;
    bool isPanning = false;
    int splashPanOffset = 456; // Adjust as needed based on your splash image dimensions vs window height
    int panCounter = 0;
    bool isCharacterSelectFadingIn = false;
    Uint8 characterSelectAlpha = 0;
    bool hasCharacterSelectStartedFading = false;

    Uint32 lastFrameTime = SDL_GetTicks();

    // --- Main Game Loop ---
    while (running) {
        Uint32 currentFrameTime = SDL_GetTicks();
        float deltaTime = static_cast<float>(currentFrameTime - lastFrameTime) / 1000.0f;
        lastFrameTime = currentFrameTime;

        bool menuUpThisFrame = false;
        bool menuDownThisFrame = false;
        bool menuSelectThisFrame = false;
        bool menuCancelThisFrame = false;

        // --- Event Handling ---
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                // Handle window resize (maintain aspect ratio if desired)
                // Your existing resize logic here...
            }

            // State-Specific Input Handling
            switch (currentGameState) {
                case GameState::MainMenu:
                    if (!isPanning) { // Only handle menu input if not panning
                        if (event.type == SDL_KEYDOWN) {
                            switch (event.key.keysym.sym) {
                                case SDLK_UP: selectedIndex = (selectedIndex > 0) ? selectedIndex - 1 : menuItems.size() - 1; break;
                                case SDLK_DOWN: selectedIndex = (selectedIndex < menuItems.size() - 1) ? selectedIndex + 1 : 0; break;
                                case SDLK_RETURN:
                                    if (selectedIndex == 0) { // Start Game
                                        isPanning = true; panCounter = 0; // Start panning animation
                                        // Transition to CharacterSelect happens *after* panning finishes (see Update Logic section)
                                    } else if (selectedIndex == 1) {
                                        SDL_Log("Options not implemented.");
                                    } else if (selectedIndex == 2) { // Exit
                                        running = false;
                                    }
                                    break;
                                case SDLK_ESCAPE: running = false; break;
                            }
                        }
                    }
                    break; // End MainMenu case

                case GameState::CharacterSelect:
                    if (!isCharacterSelectFadingIn) { // Only handle input after fade-in
                         if (event.type == SDL_KEYDOWN) {
                             switch (event.key.keysym.sym) {
                                 case SDLK_LEFT: selectedCharacterIndex = (selectedCharacterIndex > 0) ? selectedCharacterIndex - 1 : 1; break;
                                 case SDLK_RIGHT: selectedCharacterIndex = (selectedCharacterIndex < 1) ? selectedCharacterIndex + 1 : 0; break;
                                 case SDLK_RETURN: // Character Chosen
                                    { // Scope for variable initialization
                                        CharacterType chosenType = (selectedCharacterIndex == 0) ? CharacterType::FemaleMage : CharacterType::MaleMage;
                                        // Re-initialize player with chosen stats/spells (use the proper constructor)
                                        if (chosenType == CharacterType::FemaleMage) {
                                            currentGamePlayer = PlayerCharacter(chosenType, 100, 100, 150, 150, 1, 0, 0, tileWidth, tileHeight);
                                            SDL_Log("Character Selected: Female Mage");
                                        } else {
                                            currentGamePlayer = PlayerCharacter(chosenType, 120, 120, 130, 130, 1, 0, 0, tileWidth, tileHeight); // Example different stats
                                            SDL_Log("Character Selected: Male Mage");
                                        }

                                        // Generate Level 1
                                        enemies.clear(); // Clear enemies from previous game (if any)
                                        currentLevelIndex = 1;
                                        currentLevel = generateLevel(levelWidth, levelHeight, levelMaxRooms, levelMinRoomSize, levelMaxRoomSize, enemies); // Generate level and populate enemies
                                        levelRooms = currentLevel.rooms;

                                        // Place player at start position
                                        currentGamePlayer.targetTileX = currentLevel.startCol;
                                        currentGamePlayer.targetTileY = currentLevel.startRow;
                                        currentGamePlayer.x = currentGamePlayer.targetTileX * tileWidth + tileWidth / 2.0f; // Snap visual position
                                        currentGamePlayer.y = currentGamePlayer.targetTileY * tileHeight + tileHeight / 2.0f;
                                        currentGamePlayer.startTileX = currentGamePlayer.targetTileX; // Ensure startTile is correct
                                        currentGamePlayer.startTileY = currentGamePlayer.targetTileY;


                                        // Initialize visibility map
                                        visibilityMap.assign(currentLevel.height, std::vector<float>(currentLevel.width, 0.0f)); // Resize and zero-fill
                                        updateVisibility(currentLevel, levelRooms, currentGamePlayer.targetTileX, currentGamePlayer.targetTileY, hallwayVisibilityDistance, visibilityMap);

                                        // Transition to Gameplay
                                        currentGameState = GameState::PlayerTurn;
                                        isCharacterSelectFadingIn = false; // Ensure fade flag is reset
                                        hasCharacterSelectStartedFading = false;
                                        characterSelectAlpha = 0;
                                        SDL_Log("--- Entering Level 1 ---");
                                    } // End scope
                                    break; // End Enter Key case
                                case SDLK_ESCAPE: running = false; break; // Optional: Quit from char select
                             } // End keydown switch
                         } // End keydown check
                    } // End !isFadingIn check
                    break; // End CharacterSelect case

                case GameState::PlayerTurn:
                    if (!currentGamePlayer.isMoving) { // Player can only act when not moving
                        // --- Spell Menu Input Handling (ONLY if SpellMenu is active) ---
                        if (event.type == SDL_KEYDOWN) {
                            if (currentMenu == GameMenu::SpellMenu) {
                                switch (event.key.keysym.sym) {
                                    case SDLK_UP:
                                        if (event.key.repeat == 0 && !currentGamePlayer.knownSpells.empty()) {
                                            spellSelectIndex = (spellSelectIndex > 0) ? spellSelectIndex - 1 : currentGamePlayer.knownSpells.size() - 1;
                                            SDL_Log("Menu Up: spellSelectIndex is now %d", spellSelectIndex);
                                        }
                                        break;
                                    case SDLK_DOWN:
                                        if (event.key.repeat == 0 && !currentGamePlayer.knownSpells.empty()) {
                                            spellSelectIndex = (spellSelectIndex < currentGamePlayer.knownSpells.size() - 1) ? spellSelectIndex + 1 : 0;
                                            SDL_Log("Menu Down: spellSelectIndex is now %d", spellSelectIndex);
                                        }
                                        break;
                                    case SDLK_RETURN: // Select the highlighted spell
                                        if (!currentGamePlayer.knownSpells.empty() && spellSelectIndex >= 0 && spellSelectIndex < currentGamePlayer.knownSpells.size()) {
                                            currentSpellIndex = spellSelectIndex; // Set the spell to be potentially cast
                                            const Spell& spellToCast = currentGamePlayer.getSpell(currentSpellIndex);

                                            if (currentGamePlayer.canCastSpell(currentSpellIndex)) {
                                                if (spellToCast.targetType == SpellTargetType::Self) {
                                                    // Cast self-targeted spell immediately & end turn
                                                    if (currentGamePlayer.castSpell(currentSpellIndex, currentGamePlayer.targetTileX, currentGamePlayer.targetTileY, enemies, activeProjectiles, nullptr)) {
                                                        currentGameState = GameState::EnemyTurn; // actionTaken handled by state change
                                                        for (auto& enemy : enemies) { enemy.hasTakenActionThisTurn = false; } // Reset enemy flags
                                                        SDL_Log("Player turn ended (Self-Cast). Transitioning to Enemy Turn.");
                                                    } else { SDL_Log("Self-cast failed unexpectedly."); } // Should ideally not happen if canCastSpell was true
                                                } else {
                                                    // Enter targeting mode for non-self spells
                                                    targetIndicatorX = currentGamePlayer.targetTileX;
                                                    targetIndicatorY = currentGamePlayer.targetTileY;
                                                    currentGameState = GameState::PlayerTargeting;
                                                    SDL_Log("Entering targeting mode for: %s", spellToCast.name.c_str());
                                                }
                                            } else {
                                                SDL_Log("Cannot cast %s: Not enough mana.", spellToCast.name.c_str());
                                                // Optionally add a sound effect or UI message here
                                            }
                                        }
                                        currentMenu = GameMenu::None; // Close menu after selection attempt
                                        break;
                                    case SDLK_ESCAPE: // Cancel spell selection
                                        SDL_Log("Spell selection cancelled.");
                                        currentMenu = GameMenu::None; // Close the menu
                                        spellSelectIndex = 0; // Reset highlight
                                        currentSpellIndex = -1;
                                        break;
                                    default:
                                        // Allow closing menu with 'c' again? Or ignore other keys?
                                        // if (event.key.keysym.sym == SDLK_c) { currentMenu = GameMenu::None; }
                                        break;
                                }
                            } // End Spell Menu Input Handling

                            // --- Normal Player Turn Input Handling (ONLY if NO menu is active) ---
                            else if (currentMenu == GameMenu::None) {
                                int moveX = 0;
                                int moveY = 0;
                                bool actionTaken = false; // Only relevant for *instant* actions now

                                switch (event.key.keysym.sym) {
                                    // --- Movement ---
                                    case SDLK_UP: moveY = -1; break;
                                    case SDLK_DOWN: moveY = 1; break;
                                    case SDLK_LEFT: moveX = -1; break;
                                    case SDLK_RIGHT: moveX = 1; break;

                                    // --- Spellcasting INITIATION ---
                                    case SDLK_c:
                                        if (!currentGamePlayer.knownSpells.empty()) {
                                            spellSelectIndex = 0; // Start highlighting the first spell
                                            currentMenu = GameMenu::SpellMenu; // Open the menu
                                            SDL_Log("Opening Spell Menu...");
                                        } else {
                                            SDL_Log("No spells known!");
                                        }
                                        // Note: Opening the menu doesn't consume the turn itself
                                        break; // End SDLK_c case

                                    // --- Other Actions (Wait, etc.) ---
                                    // case SDLK_SPACE: actionTaken = true; SDL_Log("Player waits."); break;

                                    default: break;
                                } // End keydown switch (Normal Turn)

                             // --- Process Movement ---
                             if (!actionTaken && currentGameState == GameState::PlayerTurn && (moveX != 0 || moveY != 0)) { // Check we haven't switched state
                                 int newPlayerTargetX = currentGamePlayer.targetTileX + moveX;
                                 int newPlayerTargetY = currentGamePlayer.targetTileY + moveY;

                                 // Check level bounds and walkability
                                 if (isWithinBounds(newPlayerTargetX, newPlayerTargetY, currentLevel.width, currentLevel.height) &&
                                     currentLevel.tiles[newPlayerTargetY][newPlayerTargetX] != '#')
                                 {
                                     // Check for enemy collision
                                     bool collision = false;
                                     for (const auto& enemy : enemies) {
                                         if (enemy.x == newPlayerTargetX && enemy.y == newPlayerTargetY && enemy.health > 0) {
                                             collision = true;
                                             // TODO: Implement attacking enemy by bumping into them?
                                             SDL_Log("Player bumps into enemy at (%d, %d)!", newPlayerTargetX, newPlayerTargetY);
                                             // For now, bumping into enemy also ends the turn without moving
                                             actionTaken = true;
                                             break;
                                         }
                                     }

                                     if (!collision) { // If no wall and no enemy, start moving
                                         currentGamePlayer.startMove(newPlayerTargetX, newPlayerTargetY);
                                         // Movement itself is the action, turn transition happens after animation.
                                         // actionTaken remains false, letting the update loop handle turn end.
                                         // Update visibility based on the *target* tile immediately for responsiveness
                                          updateVisibility(currentLevel, levelRooms, newPlayerTargetX, newPlayerTargetY, hallwayVisibilityDistance, visibilityMap);
                                     }
                                 } else {
                                      SDL_Log("Cannot move into wall or outside bounds.");
                                 }
                             } // End movement processing

                             // --- End Turn if an Instant Action Occurred ---
                             if (actionTaken) {
                                 // This handles instant spells, waiting, bumping into enemies etc.
                                 currentGameState = GameState::EnemyTurn;
                                 // Reset enemy turn flags immediately
                                 for (auto& enemy : enemies) {
                                     enemy.hasTakenActionThisTurn = false;
                                 }
                                 SDL_Log("Player turn ended (Instant Action). Transitioning to Enemy Turn.");
                             }

                         } // End keydown check
                        }
                     } // End !isMoving check
                     break; // End PlayerTurn case

                case GameState::PlayerTargeting:
                    if (event.type == SDL_KEYDOWN) {
                        switch (event.key.keysym.sym) {
                            case SDLK_UP:    targetIndicatorY--; break;
                            case SDLK_DOWN:  targetIndicatorY++; break;
                            case SDLK_LEFT:  targetIndicatorX--; break;
                            case SDLK_RIGHT: targetIndicatorX++; break;

                            case SDLK_RETURN: // Confirm Spell Target
                                { // Scope needed
                                    const Spell& spell = currentGamePlayer.getSpell(currentSpellIndex); // Get selected spell info
                                    int distance = std::abs(currentGamePlayer.targetTileX - targetIndicatorX) + std::abs(currentGamePlayer.targetTileY - targetIndicatorY);

                                    if (distance <= spell.range) { // Check range again
                                        // Attempt to cast the spell
                                        SDL_Texture* texToUse = nullptr;
                                        // Determine which texture based on the selected spell index
                                        // This assumes you know which spell corresponds to which index
                                        // Example: Assuming index 0 is Firebolt, index 1 is Fireball (if you add it)
                                        if (currentSpellIndex == 0) { // Assuming Firebolt is index 0
                                             texToUse = fireballTexture; // Use the texture you loaded
                                             // If you rename fireballTexture to fireboltTexture, use that here
                                        }
                                        // else if (currentSpellIndex == 1) { texToUse = fireballTexture; }
                                        // else if (currentSpellIndex == ...) { texToUse = someOtherTexture; }

                                        if (!texToUse) {
                                             SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No projectile texture found for spell index %d!", currentSpellIndex);
                                        }

                                        if (currentGamePlayer.castSpell(currentSpellIndex, targetIndicatorX, targetIndicatorY, enemies, activeProjectiles, texToUse)){
                                            // Spell cast SUCCESS! End turn.
                                            currentGameState = GameState::ProjectileResolution;
                                            currentSpellIndex = -1; // Reset selected spell
                                            // Reset enemy turn flags
                                            for (auto& enemy : enemies) {
                                                enemy.hasTakenActionThisTurn = false;
                                            }
                                            SDL_Log("Player turn ended (Spell Cast). Transitioning to Enemy Turn.");
                                        } else {
                                            // Casting failed (e.g., no valid target found by castSpell implementation, though mana/range checked)
                                            SDL_Log("Spell casting failed (Invalid target?). Returning to player turn.");
                                            currentGameState = GameState::PlayerTurn; // Go back to player turn
                                            currentSpellIndex = -1;
                                        }
                                    } else {
                                        SDL_Log("Target out of range! Select a different target.");
                                        // Stay in targeting mode
                                    }
                                } // End scope
                                break; // End SDLK_RETURN case

                            case SDLK_ESCAPE: // Cancel Targeting
                                SDL_Log("Targeting cancelled.");
                                currentGameState = GameState::PlayerTurn; // Go back to player turn
                                currentSpellIndex = -1; // Reset selected spell
                                break; // End SDLK_ESCAPE case

                            default: break;
                        } // End keydown switch

                        // Clamp target indicator within level bounds
                        targetIndicatorX = std::max(0, std::min(currentLevel.width - 1, targetIndicatorX));
                        targetIndicatorY = std::max(0, std::min(currentLevel.height - 1, targetIndicatorY));

                    } // End keydown check
                    break; // End PlayerTargeting case

                case GameState::EnemyTurn:
                    // No player input during enemy turn
                    break; // End EnemyTurn case

                case GameState::ProjectileResolution:
                    // Ignore player input while projectiles are flying
                    if (event.type == SDL_KEYDOWN) {
                            SDL_Log("Input ignored during ProjectileResolution.");
                    }
                    break; // End ProjectileResolution case

                case GameState::GameOver:
                    // Input to restart or quit?
                     if (event.type == SDL_KEYDOWN) {
                         if (event.key.keysym.sym == SDLK_RETURN) {
                              // Restart - go back to main menu or character select?
                              currentGameState = GameState::MainMenu;
                              // Reset any game variables needed for restart
                         } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                              running = false;
                         }
                     }
                    break; // End GameOver case

            } // End GameState switch (within event polling)
        } // End SDL_PollEvent loop


        // --- Update Logic ---

        // Update Panning/Fades (Menu -> Character Select)
        if (isPanning) {
            panCounter += 10; // Adjust speed as needed
            splashPanOffset -= 10; // Assuming splash pans upwards
            if (splashPanOffset <= 0) { // Panning finished
                splashPanOffset = 0;
                isPanning = false;
                currentGameState = GameState::CharacterSelect; // Now transition state
                isCharacterSelectFadingIn = true; // Start fade in
                characterSelectAlpha = 0;
                hasCharacterSelectStartedFading = true;
            }
        }
        if (isCharacterSelectFadingIn) {
            // Need safe alpha increment
            int newAlpha = static_cast<int>(characterSelectAlpha) + 10; // Adjust fade speed
            if (newAlpha >= 255) {
                characterSelectAlpha = 255;
                isCharacterSelectFadingIn = false; // Fade-in complete
            } else {
                characterSelectAlpha = static_cast<Uint8>(newAlpha);
            }
        }

        // Update Game Entities (Only during gameplay states)
        switch (currentGameState) {
            case GameState::PlayerTurn:
                // Only update player movement if they are moving
                if (currentGamePlayer.isMoving) {
                    bool playerWasMoving = true; // Assume they were if in this state
                    currentGamePlayer.update(deltaTime, tileWidth, tileHeight);
                    bool playerIsNowIdle = !currentGamePlayer.isMoving;

                    // Check if movement finished AND menu is closed
                    if (playerWasMoving && playerIsNowIdle && currentMenu == GameMenu::None) {
                         currentGameState = GameState::EnemyTurn;
                         for (auto& enemy : enemies) { enemy.hasTakenActionThisTurn = false; }
                         SDL_Log("Player turn ended (Movement Complete). Transitioning to Enemy Turn.");
                    }
                }
                // Do NOT update projectiles or enemies here
                break;

            case GameState::ProjectileResolution:
                { // Scope for variables
                    // --- Update Projectiles & Apply Hits ---
                    bool hitOccurred = false; // Track if any hit happened this frame
                    // Iterate using index to allow safe removal inside loop is complex,
                    // so let's update all, apply hits, then remove dead ones after.
                    std::vector<std::pair<int, int>> hitLocations; // Store XY tile coords of hits

                    for (auto& proj : activeProjectiles) {
                        if (proj.isActive && proj.update(deltaTime)) { // proj.update returns true on hit
                            hitOccurred = true;
                            int hitX = static_cast<int>(proj.targetX / tileWidth);
                            int hitY = static_cast<int>(proj.targetY / tileHeight);
                            hitLocations.push_back({hitX, hitY});
                            SDL_Log("Projectile hit detected at tile (%d, %d)", hitX, hitY);
                            // proj.isActive is set to false inside proj.update()
                        }
                    }

                    // Apply damage for all hits that occurred this frame
                    if (hitOccurred) {
                         for (const auto& hitPos : hitLocations) {
                             int hitX = hitPos.first;
                             int hitY = hitPos.second;
                              SDL_Log("Checking for enemy at hit location (%d, %d)...", hitX, hitY);
                              bool enemyFoundAndDamaged = false;
                             for (auto& enemy : enemies) {
                                 SDL_Log("  Comparing with enemy at (%d, %d), health=%d", enemy.x, enemy.y, enemy.health);
                                 if (enemy.x == hitX && enemy.y == hitY && enemy.health > 0) {
                                     int damageAmount = 15; // TODO: Get damage from projectile/spell
                                     SDL_Log("  MATCH FOUND! Applying projectile damage (%d) to enemy at (%d, %d)", damageAmount, hitX, hitY);
                                     enemy.takeDamage(damageAmount);
                                     enemyFoundAndDamaged = true;
                                     // Hit only one enemy per projectile? break; might be needed if so.
                                     break;
                                 }
                             }
                              if (!enemyFoundAndDamaged) { SDL_Log("...No living enemy found at hit location (%d, %d).", hitX, hitY); }
                         }
                    }

                    // --- Remove Inactive Projectiles ---
                    activeProjectiles.erase(std::remove_if(activeProjectiles.begin(), activeProjectiles.end(),
                                                            [](const Projectile& p){ return !p.isActive; }),
                                            activeProjectiles.end());

                    // --- Check if Resolution Finished ---
                    if (activeProjectiles.empty()) {
                        SDL_Log("All projectiles resolved. Transitioning to Enemy Turn.");
                        currentGameState = GameState::EnemyTurn;
                        // Reset enemy action flags NOW before enemies act
                        for (auto& enemy : enemies) {
                            enemy.hasTakenActionThisTurn = false;
                        }
                        // --- Check for Player Death (after potential hits) ---
                        // It's possible player died from reflected damage etc. Check here? Or after enemy turn?
                        // Let's check after enemy turn for now.
                    }
                } // End scope for ProjectileResolution case
                break; // End ProjectileResolution case

            case GameState::EnemyTurn:
            {
                // --- Update Enemies & Handle AI/Turn ---
                 bool allEnemiesActed = true;
                 bool allEnemiesIdle = true;
                 bool actionAttemptedThisFrame = false;

                 for (auto& enemy : enemies) {
                     if (enemy.health > 0) {
                         // Update enemy animation/movement first
                         enemy.update(deltaTime, tileWidth, tileHeight);

                         // Then check AI / Take Action
                         if (!enemy.hasTakenActionThisTurn) {
                             allEnemiesActed = false;
                             if (!enemy.isMoving && !actionAttemptedThisFrame) {
                                 enemy.takeAction(currentLevel, currentGamePlayer);
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
                     if (currentGamePlayer.health <= 0) {
                          currentGameState = GameState::GameOver;
                          SDL_Log("--- Game Over ---");
                     } else {
                          // --- Start Player Turn ---
                          currentGameState = GameState::PlayerTurn;
                          updateVisibility(currentLevel, levelRooms, currentGamePlayer.targetTileX, currentGamePlayer.targetTileY, hallwayVisibilityDistance, visibilityMap);
                          SDL_Log("Enemy turn ended. Transitioning to Player Turn.");
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

        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const Enemy& e) { return e.health <= 0; }),
enemies.end());


// --- >> ADD THIS CAMERA UPDATE BLOCK BACK << ---
// Update Camera (Should run in most gameplay states to follow player)
if (currentGameState == GameState::PlayerTurn ||
currentGameState == GameState::PlayerTargeting ||
currentGameState == GameState::ProjectileResolution || // Follow player even during resolution
currentGameState == GameState::EnemyTurn)               // Follow player even during enemy turn
{
int halfWidth = windowWidth / 2;
int halfHeight = windowHeight / 2;
// Center on the player's *current visual position*
int idealCameraX = static_cast<int>(currentGamePlayer.x) - halfWidth;
int idealCameraY = static_cast<int>(currentGamePlayer.y) - halfHeight;

// Clamp camera to level boundaries
int maxCameraX = (currentLevel.width * tileWidth) - windowWidth;
int maxCameraY = (currentLevel.height * tileHeight) - windowHeight;
cameraX = std::max(0, std::min(idealCameraX, maxCameraX));
cameraY = std::max(0, std::min(idealCameraY, maxCameraY));
// Ensure maxCameraX/Y aren't negative if level is smaller than screen
if (maxCameraX < 0) cameraX = 0;
if (maxCameraY < 0) cameraY = 0;
}


        // --- Rendering ---
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);

        // Render based on Game State
        switch (currentGameState) {
            case GameState::MainMenu:
                displayMenu(renderer, font, splashTexture, menuItems, selectedIndex, isPanning, splashPanOffset, 456, windowWidth, windowHeight); // Adjust Y offset as needed
                break;

            case GameState::CharacterSelect:
                 // Render a background if desired (e.g., black or a static image)
                 // SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
                 // SDL_RenderClear(renderer);
                 displayCharacterSelect(renderer, font, selectedCharacterIndex, windowWidth, windowHeight, characterSelectAlpha);
                 break;

            case GameState::PlayerTurn:
            case GameState::PlayerTargeting: // Targeting shares rendering with PlayerTurn
            case GameState::ProjectileResolution:
            case GameState::EnemyTurn:
                // --- Render Level ---
                if (!currentLevel.tiles.empty()) {
                    for (int y = 0; y < currentLevel.height; ++y) {
                        for (int x = 0; x < currentLevel.width; ++x) {
                            SDL_Rect tileRect = {(x * tileWidth) - cameraX, (y * tileHeight) - cameraY, tileWidth, tileHeight};
                            float visibility = (isWithinBounds(x,y, currentLevel.width, currentLevel.height)) ? visibilityMap[y][x] : 0.0f;

                            if (visibility > 0.0f) { // Only render visible tiles
                                 // Choose color/texture based on tile type
                                if (y == currentLevel.startRow && x == currentLevel.startCol && startTexture) {
                                      SDL_RenderCopy(renderer, startTexture, nullptr, &tileRect);
                                } else if (y == currentLevel.endRow && x == currentLevel.endCol && exitTexture) {
                                     SDL_RenderCopy(renderer, exitTexture, nullptr, &tileRect);
                                } else if (currentLevel.tiles[y][x] == '#') {
                                     SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); // Brown Wall
                                     SDL_RenderFillRect(renderer, &tileRect);
                                } else if (currentLevel.tiles[y][x] == '.') {
                                     SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Light Gray Floor
                                     SDL_RenderFillRect(renderer, &tileRect);
                                }
                                // Add other tile types ('V' for void/vision blocker?)

                                // Apply visibility dimming overlay
                                Uint8 alpha = static_cast<Uint8>((1.0f - visibility) * 200); // Adjust dimming intensity (e.g., * 200 for less total darkness)
                                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                                SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha); // Black overlay
                                SDL_RenderFillRect(renderer, &tileRect);
                                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                            } else {
                                // Optionally render previously explored tiles dimly (Fog of War)
                                // Or just render black for unseen tiles
                                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                                SDL_RenderFillRect(renderer, &tileRect);
                            }
                        } // End x loop
                    } // End y loop
                } // End level rendering

                // --- Render Enemies ---
                for (const auto& enemy : enemies) {
                     if (enemy.health > 0) { // Only render alive enemies
                         int enemyTileX = enemy.x; // Use logical position for visibility check
                         int enemyTileY = enemy.y;
                         float visibility = 0.0f;
                         if (isWithinBounds(enemyTileX, enemyTileY, currentLevel.width, currentLevel.height)) {
                            visibility = visibilityMap[enemyTileY][enemyTileX];
                         }

                         if (visibility > 0.0f) {
                            enemy.render(renderer, cameraX, cameraY, tileWidth, tileHeight, visibility); // Pass visibility for alpha
                         }
                     }
                }

                // --- Render Player ---
                SDL_Rect playerRect;
                playerRect.w = tileWidth / 2; // Make player smaller? Adjust as needed
                playerRect.h = tileHeight / 2;
                playerRect.x = static_cast<int>(currentGamePlayer.x - playerRect.w / 2.0f) - cameraX; // Center based on visual pos
                playerRect.y = static_cast<int>(currentGamePlayer.y - playerRect.h / 2.0f) - cameraY;
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green for player
                SDL_RenderFillRect(renderer, &playerRect);

                // --- >> ADD Projectile Rendering << ---
                for (const auto& proj : activeProjectiles) {
                    // Optionally add visibility check? Should projectiles be visible in FoW? For now, render all active.
                    proj.render(renderer, cameraX, cameraY);
                }

                // --- Render Spell Menu Overlay (If Active) ---
                if (currentGameState == GameState::PlayerTurn && currentMenu == GameMenu::SpellMenu) {
                    SDL_Log("Rendering menu: Passing spellSelectIndex = %d", spellSelectIndex);
                    renderSpellMenu(renderer, font, currentGamePlayer, spellSelectIndex, windowWidth, windowHeight);
                }

                // --- Render Targeting Reticle (if targeting) ---
                if (currentGameState == GameState::PlayerTargeting) {
                    // Check if the target tile is visible before rendering reticle
                    if (isWithinBounds(targetIndicatorX, targetIndicatorY, currentLevel.width, currentLevel.height) &&
                        visibilityMap[targetIndicatorY][targetIndicatorX] > 0.0f)
                    {
                        SDL_Rect reticleRect = { (targetIndicatorX * tileWidth) - cameraX, (targetIndicatorY * tileHeight) - cameraY, tileWidth, tileHeight };
                        const Spell& spell = currentGamePlayer.getSpell(currentSpellIndex);
                        int distance = std::abs(currentGamePlayer.targetTileX - targetIndicatorX) + std::abs(currentGamePlayer.targetTileY - targetIndicatorY);
                        Uint8 reticleAlpha = 180; // Base alpha for reticle

                        // Set color/texture based on range validity
                        if (distance <= spell.range) {
                             // White or Green if in range
                            if (targetingReticleTexture) SDL_SetTextureColorMod(targetingReticleTexture, 255, 255, 255); // White tint
                            SDL_SetRenderDrawColor(renderer, 255, 255, 255, reticleAlpha);
                        } else {
                             // Red if out of range
                            if (targetingReticleTexture) SDL_SetTextureColorMod(targetingReticleTexture, 255, 100, 100); // Red tint
                            SDL_SetRenderDrawColor(renderer, 255, 0, 0, reticleAlpha);
                        }

                        if (targetingReticleTexture) {
                             SDL_SetTextureAlphaMod(targetingReticleTexture, reticleAlpha);
                             SDL_RenderCopy(renderer, targetingReticleTexture, nullptr, &reticleRect);
                        } else {
                             // Fallback: Draw a simple box
                             SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                             SDL_RenderDrawRect(renderer, &reticleRect);
                             SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                        }
                    } // End visibility check for reticle
                } // End rendering reticle


                // --- Render UI ---
                renderUI(renderer, font, currentGamePlayer, currentLevelIndex, windowWidth);
                break; // End gameplay rendering states

            case GameState::GameOver:
                 // Render "Game Over" screen
                 // TODO: Implement displayGameOver function or similar
                 SDL_Color white = {255, 255, 255, 255};
                 SDL_Surface* surface = TTF_RenderText_Solid(font, "Game Over - Press Enter to Restart", white);
                 if (surface) {
                     SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                     if (texture) {
                          SDL_Rect textRect;
                          textRect.w = surface->w;
                          textRect.h = surface->h;
                          textRect.x = (windowWidth - textRect.w) / 2;
                          textRect.y = (windowHeight - textRect.h) / 2;
                          SDL_RenderCopy(renderer, texture, nullptr, &textRect);
                          SDL_DestroyTexture(texture);
                     }
                     SDL_FreeSurface(surface);
                 }
                 break; // End GameOver case

        } // End GameState rendering switch

        SDL_RenderPresent(renderer);

        // Frame Delay (Optional, prevents maxing out CPU)
        SDL_Delay(10); // Delay for ~10ms

    } // End Main Game Loop


    // --- Cleanup ---
    SDL_DestroyTexture(splashTexture);
    SDL_DestroyTexture(startTexture);
    SDL_DestroyTexture(exitTexture);
    SDL_DestroyTexture(targetingReticleTexture);
    SDL_DestroyTexture(fireballTexture);
    cleanupSDL(sdlContext); // Cleans up window, renderer, font, subsystems

    SDL_Log("Exiting gracefully. Farewell, Mortal.");
    return 0;
}