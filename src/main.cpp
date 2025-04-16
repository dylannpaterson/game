#include <iostream>
#include "character.h"
#include "menu.h"
#include "character_select.h"
#include "level.h" // Include the level header
#include "enemy.h" // Include the enemy header
#include"utils.h"
#include "visibility.h"
#include "ui.h"
#include <vector>
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h> // Include SDL_image
#include <cmath> // For std::abs
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()
#include <random>


#ifdef _WIN32
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "SDL2_ttf.lib")
#pragma comment(lib, "SDL2_image.lib")
#pragma comment(lib, "SDL2_image.lib")
#endif

enum class GameState {
    MainMenu,
    CharacterSelect,
    GamePlay,
    PlayerTurn,
    EnemyTurn,
    GameOver // Add this state
};

GameState currentGameState = GameState::MainMenu; // Start at the MainMenu
std::vector<Enemy> enemies;

int main(int argc, char* argv[]) {
    srand(time(0)); // Seed the random number generator

    int windowWidth = 1920;
    int windowHeight = 1080;

    // Initialize SDL
    SDL_Context sdlContext = initializeSDL(windowWidth, windowHeight);
    if (sdlContext.window == nullptr || sdlContext.renderer == nullptr || sdlContext.font == nullptr) {
        // Initialization failed, cleanup and exit
        cleanupSDL(sdlContext);
        return 1;
    }

    SDL_Window* window = sdlContext.window;
    SDL_Renderer* renderer = sdlContext.renderer;
    TTF_Font* font = sdlContext.font;

    float aspectRatio = static_cast<float>(windowWidth) / windowHeight;

    if (window == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    if (renderer == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Renderer could not be created! SDL_Error: %s\n", IMG_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, windowWidth, windowHeight); // Set your initial resolution as the logical size

    font = TTF_OpenFont("../assets/fonts/LUMOS.TTF", 36);

    if (font == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load font! TTF_Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load the splash image
    SDL_Texture* splashTexture = nullptr;
    SDL_Surface* loadedSurface = IMG_Load("../assets/splash/splash.png"); // Replace "splash.png" with your image file
    if (loadedSurface == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load image! IMG_Error: %s\n", IMG_GetError());
    } else {
        splashTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (splashTexture == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create texture from image! SDL_Error: %s\n", IMG_GetError());
        } else{
            SDL_SetTextureBlendMode(splashTexture, SDL_BLENDMODE_BLEND);
        }
        SDL_FreeSurface(loadedSurface);
    }

    SDL_Texture* startTexture = nullptr;
    loadedSurface = IMG_Load("../assets/textures/start_placeholder.png"); // Adjust path if needed
    if (loadedSurface == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load start texture! IMG_Error: %s\n", IMG_GetError());
    } else {
        startTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (startTexture == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create texture from start image! SDL_Error: %s\n", IMG_GetError());
        }
        SDL_FreeSurface(loadedSurface);
    }

    SDL_Texture* exitTexture = nullptr;
    loadedSurface = IMG_Load("../assets/textures/exit_placeholder.png"); // Adjust path if needed
    if (loadedSurface == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load exit texture! IMG_Error: %s\n", IMG_GetError());
    } else {
        exitTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (exitTexture == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create texture from exit image! SDL_Error: %s\n", IMG_GetError());
        }
        SDL_FreeSurface(loadedSurface);
    }


    SDL_Event event;
    std::vector<std::string> menuItems = {"Start Game", "Options", "Exit"};
    int selectedIndex = 0;
    bool running = true;
    bool gameStarted = false;
    bool inCharacterSelect = false;
    int selectedCharacterIndex = 0;
    bool isPanning = false;
    int splashPanOffset = 1536 - windowHeight;
    int panCounter = 0;

    bool isCharacterSelectFadingIn = false; // New variable for fade state
    Uint8 characterSelectAlpha = 0;          // New variable for alpha value
    bool hasCharacterSelectStartedFading = false; // New flag to track if fade started

    Level currentLevel; // Variable to hold the loaded level
    std::vector<SDL_Rect> levelRooms; // To store the rooms from the current level
    int playerX = 0;
    int playerY = 0;
    int cameraX = 0;
    int cameraY = 0;
    int tileWidth = 64;   // Assuming your tile size is 64x64
    int tileHeight = 64;  // Assuming your tile size is 64x64
    int levelWidth = 120;
    int levelHeight = 75;
    int levelMaxRooms = 15;
    int levelMinRoomSize = 8;
    int levelMaxRoomSize = 15;
    int currentLevelIndex = 1; // Start with level 1
    PlayerCharacter currentGamePlayer(CharacterType::FemaleMage, 100, 100, 150, 150, 1, 0, 0, tileWidth, tileHeight); // Initialize here with tile dimensions
    std::vector<std::vector<float>> visibilityMap; // Updated to float
    int hallwayVisibilityDistance = 5; // Adjust this value as needed

    Uint32 lastFrameTime = SDL_GetTicks(); // For calculating deltaTime

    while (running) {
        Uint32 currentFrameTime = SDL_GetTicks();
        float deltaTime = static_cast<float>(currentFrameTime - lastFrameTime) / 1000.0f;
        lastFrameTime = currentFrameTime;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int newWidth = event.window.data1;
                    int newHeight = event.window.data2;

                    // Calculate the correct dimensions based on the original aspect ratio
                    float currentAspectRatio = static_cast<float>(newWidth) / newHeight;

                    if (std::abs(currentAspectRatio - aspectRatio) > 0.001) { // Check if the aspect ratio has significantly changed
                        int correctWidth, correctHeight;
                        if (currentAspectRatio > aspectRatio) {
                            correctHeight = newHeight;
                            correctWidth = static_cast<int>(correctHeight * aspectRatio);
                        } else {
                            correctWidth = newWidth;
                            correctHeight = static_cast<int>(correctWidth / aspectRatio);
                        }
                        SDL_SetWindowSize(window, correctWidth, correctHeight);
                    }
                }
            }

            switch (currentGameState) {
                case GameState::MainMenu:
                    if (!gameStarted && !isPanning) { // Menu input
                        if (event.type == SDL_KEYDOWN) {
                            switch (event.key.keysym.sym) {
                                case SDLK_UP:
                                    selectedIndex = (selectedIndex > 0) ? selectedIndex - 1 : menuItems.size() - 1;
                                    break;
                                case SDLK_DOWN:
                                    selectedIndex = (selectedIndex < menuItems.size() - 1) ? selectedIndex + 1 : 0;
                                    break;
                                case SDLK_RETURN: // Enter key
                                    if (selectedIndex == 0) {
                                        std::cout << "\nEmbarking on the adventure (SDL with Image)..." << std::endl;
                                        gameStarted = true;
                                        isPanning = true; // Start panning
                                    } else if (selectedIndex == 1) {
                                        std::cout << "\nOptions menu not implemented yet (SDL with Image)." << std::endl;
                                    } else if (selectedIndex == 2) {
                                        std::cout << "\nExiting game (SDL with Image). Farewell, Mortal." << std::endl;
                                        running = false;
                                    }
                                    break;
                                case SDLK_ESCAPE: // Optional: Handle Escape key to quit
                                    running = false;
                                    break;
                            }
                        }
                    }
                    break;

                case GameState::CharacterSelect:
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
                    SDL_RenderClear(renderer); // Clear only when in character select
                    displayCharacterSelect(renderer, font, selectedCharacterIndex, windowWidth, windowHeight, characterSelectAlpha); // Pass the alpha value
                    if (inCharacterSelect) { // Character select input
                        if (event.type == SDL_KEYDOWN) {
                            switch (event.key.keysym.sym) {
                                case SDLK_LEFT:
                                    selectedCharacterIndex = (selectedCharacterIndex > 0) ? selectedCharacterIndex - 1 : 1; // Cycle between 0 and 1
                                    break;
                                case SDLK_RIGHT:
                                    selectedCharacterIndex = (selectedCharacterIndex < 1) ? selectedCharacterIndex + 1 : 0; // Cycle between 0 and 1
                                    break;
                                case SDLK_RETURN: // Enter key
                                { // Start of new scope to fix "jump to case label"
                                    PlayerCharacter player(CharacterType::FemaleMage, 100, 100, 150, 150, 1, 0, 0, tileWidth, tileHeight); // Create an instance
                                    if (selectedCharacterIndex == 0) {
                                        player.type = CharacterType::FemaleMage;
                                        std::cout << "\nCharacter Selected: Female Mage" << std::endl;
                                    } else if (selectedCharacterIndex == 1) {
                                        player.type = CharacterType::MaleMage;
                                        player.health = 120;
                                        player.maxHealth = 120;
                                        player.mana = 130;
                                        player.maxMana = 130;
                                        std::cout << "\nCharacter Selected: Male Mage" << std::endl;
                                    }
                                    // Character selected, load the level here
                                    enemies.clear(); // Clear previous enemies
                                    currentLevel = generateLevel(levelWidth, levelHeight, levelMaxRooms, levelMinRoomSize, levelMaxRoomSize, enemies); // Pass enemies vector
                                    levelRooms = currentLevel.rooms;
                                    playerX = currentLevel.startCol; // Set player X from level data
                                    playerY = currentLevel.startRow; // Set player Y from level data
                                    currentGamePlayer = player;
                                    currentGamePlayer.targetTileX = playerX; // Initialize targetTileX
                                    currentGamePlayer.targetTileY = playerY; // Initialize targetTileY
                                    currentGamePlayer.x = playerX * tileWidth + tileWidth / 2.0f; // Initialize visual x
                                    currentGamePlayer.y = playerY * tileHeight + tileHeight / 2.0f; // Initialize visual y
                                    visibilityMap.resize(currentLevel.height, std::vector<float>(currentLevel.width, 0.0f)); // Initialize with float
                                    updateVisibility(currentLevel, levelRooms, playerX, playerY, hallwayVisibilityDistance, visibilityMap);
                                    gameStarted = true; // Keep gameStarted true as we are now in the game loop
                                    inCharacterSelect = false; // Exit character select state
                                    // Next, we would transition to the game loop
                                    std::cout << "\n--- Preparing to Enter the Game Loop ---" << std::endl;
                                    currentGameState = GameState::PlayerTurn;
                                    break;
                                } // End of new scope
                                case SDLK_ESCAPE: // Optional: Handle Escape key to quit
                                    running = false;
                                    break;
                            }
                        }
                    }
                    break;

                    case GameState::PlayerTurn:
                    
                    if (!currentGamePlayer.isMoving) {
                        if (event.type == SDL_KEYDOWN) {
                            int moveX = 0;
                            int moveY = 0;
                            switch (event.key.keysym.sym) {
                                case SDLK_UP: moveY = -1; break;
                                case SDLK_DOWN: moveY = 1; break;
                                case SDLK_LEFT: moveX = -1; break;
                                case SDLK_RIGHT: moveX = 1; break;
                                default: break;
                            }
                
                            if (moveX != 0 || moveY != 0) {
                                int newPlayerTargetX = currentGamePlayer.targetTileX + moveX;
                                int newPlayerTargetY = currentGamePlayer.targetTileY + moveY;
                
                                if (isWithinBounds(newPlayerTargetX, newPlayerTargetY, currentLevel.width, currentLevel.height) && currentLevel.tiles[newPlayerTargetY][newPlayerTargetX] != '#') {
                                    bool collision = false;
                                    for (const auto& enemy : enemies) {
                                        if (enemy.x == newPlayerTargetX && enemy.y == newPlayerTargetY) {
                                            collision = true;
                                            break;
                                        }
                                    }
                
                                    if (!collision) {
                                        currentGamePlayer.startMove(newPlayerTargetX, newPlayerTargetY);
                                        updateVisibility(currentLevel, levelRooms, newPlayerTargetX, newPlayerTargetY, hallwayVisibilityDistance, visibilityMap);
                                        playerX = newPlayerTargetX;
                                        playerY = newPlayerTargetY;
                                    } else {
                                        SDL_Log("Player tried to move into an enemy!");
                                    }
                                }
                            }
                        }
                    }
                    // The "Exiting PlayerTurn state." should NOT be here.
                    break;

                    case GameState::EnemyTurn:
                    {
                        bool allEnemiesActed = true;
                        for (const auto& enemy : enemies) {
                            if (!enemy.hasTakenActionThisTurn) {
                                allEnemiesActed = false;
                                break;
                            }
                        }

                        if (!allEnemiesActed) {
                            bool foundActionThisFrame = false;
                            for (size_t i = 0; i < enemies.size(); ++i) {
                                if (!enemies[i].isMoving && !enemies[i].hasTakenActionThisTurn && !foundActionThisFrame) {
                                    enemies[i].takeAction(currentLevel, currentGamePlayer);
                                    foundActionThisFrame = true;
                                    break; // Allow one action per frame for clarity
                                }
                            }
                        } else {
                            bool allEnemiesIdle = true;
                            for (const auto& enemy : enemies) {
                                if (enemy.isMoving) {
                                    allEnemiesIdle = false;
                                    break;
                                }
                            }
                            if (allEnemiesIdle) {
                                // Reset action flags for the next turn
                                for (auto& enemy : enemies) {
                                    enemy.hasTakenActionThisTurn = false;
                                }
                                currentGameState = GameState::PlayerTurn;
                                updateVisibility(currentLevel, levelRooms, currentGamePlayer.targetTileX, currentGamePlayer.targetTileY, hallwayVisibilityDistance, visibilityMap);
                            }
                        }
                    }
                    break;

                case GameState::GameOver:
                    // Handle game over logic
                    break;
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        // Render based on game state
        if (currentGameState != GameState::MainMenu && currentGameState != GameState::CharacterSelect) {
            // Render the level
            if (!currentLevel.tiles.empty()) {
                for (int y = 0; y < currentLevel.height; ++y) {
                    for (int x = 0; x < currentLevel.width; ++x) {
                        SDL_Rect tileRect = {(x * tileWidth) - cameraX, (y * tileHeight) - cameraY, tileWidth, tileHeight};
                        if (visibilityMap[y][x] > 0.0f) { // Only render visible tiles
                            if (y == currentLevel.startRow && x == currentLevel.startCol) {
                                if (startTexture != nullptr) {
                                    SDL_RenderCopy(renderer, startTexture, nullptr, &tileRect);
                                } else {
                                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
                                    SDL_RenderFillRect(renderer, &tileRect);
                                }
                            } else if (y == currentLevel.endRow && x == currentLevel.endCol) {
                                if (exitTexture != nullptr) {
                                    SDL_RenderCopy(renderer, exitTexture, nullptr, &tileRect);
                                } else {
                                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);
                                    SDL_RenderFillRect(renderer, &tileRect);
                                }
                            } else if (currentLevel.tiles[y][x] == '#') {
                                SDL_SetRenderDrawColor(renderer, 139, 69, 19, SDL_ALPHA_OPAQUE); // Brown for walls
                                SDL_RenderFillRect(renderer, &tileRect);
                            } else if (currentLevel.tiles[y][x] == '.') {
                                SDL_SetRenderDrawColor(renderer, 200, 200, 200, SDL_ALPHA_OPAQUE); // Light grey for floor
                                SDL_RenderFillRect(renderer, &tileRect);
                            } else if (currentLevel.tiles[y][x] == 'V') {
                                SDL_SetRenderDrawColor(renderer, 100, 100, 100, SDL_ALPHA_OPAQUE);
                                SDL_RenderFillRect(renderer, &tileRect);
                            }

                            // Apply brightness overlay using a black rectangle with alpha
                            float brightness = visibilityMap[y][x];
                            int alpha = static_cast<int>((1.0f - brightness) * 255); // Invert for dimming effect (0 alpha = full bright)
                            SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha); // Use black overlay for dimming
                            SDL_RenderFillRect(renderer, &tileRect);

                        } else {
                            // Render non-visible tiles as black
                            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
                            SDL_RenderFillRect(renderer, &tileRect);
                        }
                    }
                }
            }
            

            // Render enemies
            for (const auto& enemy : enemies) {
                int enemyTileX = static_cast<int>(enemy.x);
                int enemyTileY = static_cast<int>(enemy.y);
                float visibility = 0.0f;

                if (enemyTileY >= 0 && enemyTileY < currentLevel.height && enemyTileX >= 0 && enemyTileX < currentLevel.width) {
                    visibility = visibilityMap[enemyTileY][enemyTileX];
                }

                if (visibility > 0.0f) {
                    enemy.render(renderer, cameraX, cameraY, tileWidth, tileHeight, visibility);
                }
            }
            

            // Render the player character
            SDL_Rect playerRect;
            playerRect.w = tileWidth;
            playerRect.h = tileHeight;
            playerRect.x = static_cast<int>(currentGamePlayer.x - tileWidth / 2) - cameraX; // Apply camera offset
            playerRect.y = static_cast<int>(currentGamePlayer.y - tileHeight / 2) - cameraY; // Apply camera offset
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer, &playerRect);
            

            // Render UI elements
            renderUI(renderer, font, currentGamePlayer, currentLevelIndex, windowWidth);
        } else if (currentGameState == GameState::MainMenu) {
            displayMenu(renderer, font, splashTexture, menuItems, selectedIndex, isPanning, splashPanOffset, 456, windowWidth, windowHeight);
        } else if (currentGameState == GameState::CharacterSelect) {
            displayCharacterSelect(renderer, font, selectedCharacterIndex, windowWidth, windowHeight, characterSelectAlpha); // Pass the alpha value
        }

        SDL_RenderPresent(renderer);

        if (isPanning) {
            splashPanOffset = 456 - panCounter;
            panCounter += 10;
            if (splashPanOffset <= 0) {
                splashPanOffset = 0;
                isPanning = false;
                inCharacterSelect = true;
                currentGameState = GameState::CharacterSelect; // Transition to Character Select
                panCounter = 0;
            }
        }

        if (inCharacterSelect && !isCharacterSelectFadingIn && !hasCharacterSelectStartedFading) {
            isCharacterSelectFadingIn = true;
            characterSelectAlpha = 0;
            hasCharacterSelectStartedFading = true;
        }

        if (isCharacterSelectFadingIn) {
            Uint8 previousAlpha = characterSelectAlpha;
            characterSelectAlpha += 20; // Using the problematic speed

            if (characterSelectAlpha < previousAlpha) { // Check for wrap-around
                characterSelectAlpha = 255; // Cap it at 255
                isCharacterSelectFadingIn = false; // Fade-in complete
            } else if (characterSelectAlpha == 255) {
                isCharacterSelectFadingIn = false; // Fade-in complete
            }
        }

        // Update game state (animations, etc.)
        if (currentGameState == GameState::PlayerTurn || currentGameState == GameState::EnemyTurn) {
            bool playerWasMoving = currentGamePlayer.isMoving;
            currentGamePlayer.update(deltaTime, tileWidth, tileHeight);

            bool playerIsNowIdle = !currentGamePlayer.isMoving; // Check state AFTER update

            if (currentGameState == GameState::PlayerTurn && playerWasMoving && playerIsNowIdle) {
                currentGameState = GameState::EnemyTurn;


                for (auto& enemy : enemies) {
                     enemy.hasTakenActionThisTurn = false; 
                }
            }

            for (auto& enemy : enemies) {
                enemy.update(deltaTime, tileWidth, tileHeight); // Update enemies (handles movement progress)
            }

            // --- Enemy Turn Logic Adjustment ---
            if (currentGameState == GameState::EnemyTurn) {
                // Check if ALL enemies have finished their actions AND animations
                bool allEnemiesActed = true;
                bool allEnemiesIdle = true;
                for (auto& enemy : enemies) { // Use reference to modify hasTakenActionThisTurn if needed
                    if (!enemy.hasTakenActionThisTurn) {
                        allEnemiesActed = false;
                        // Try to make an enemy act if it hasn't
                        if (!enemy.isMoving) {
                             enemy.takeAction(currentLevel, currentGamePlayer); // Let enemy decide action
                             // Assuming takeAction sets isMoving if it decides to move
                        }
                    }
                    if (enemy.isMoving) {
                        allEnemiesIdle = false; // If any enemy is still moving, wait
                    }
                }

                // Transition back to PlayerTurn ONLY if all enemies have acted AND finished moving
                if (allEnemiesActed && allEnemiesIdle) {
                    currentGameState = GameState::PlayerTurn;
                    updateVisibility(currentLevel, levelRooms, currentGamePlayer.targetTileX, currentGamePlayer.targetTileY, hallwayVisibilityDistance, visibilityMap); // Update visibility at start of player turn
                    // Enemy action flags are reset when transitioning *to* EnemyTurn, so no reset needed here.
                }
            }
            // Update camera position to center on the player's visual position
            int halfWidth = windowWidth / 2;
            int halfHeight = windowHeight / 2;

            int idealCameraX = static_cast<int>(currentGamePlayer.x) - halfWidth;
            int idealCameraY = static_cast<int>(currentGamePlayer.y) - halfHeight;

            // Constrain the camera to the level boundaries
            cameraX = idealCameraX;
            cameraY = idealCameraY;

            if (cameraX < 0) {
                cameraX = 0;
            } else if (cameraX > (currentLevel.width * tileWidth) - windowWidth) {
                cameraX = (currentLevel.width * tileWidth) - windowWidth;
            }

            if (cameraY < 0) {
                cameraY = 0;
            } else if (cameraY > (currentLevel.height * tileHeight) - windowHeight) {
                cameraY = (currentLevel.height * tileHeight) - windowHeight; // Corrected line
            }
        }

        SDL_Delay(10);
    }


    // Cleanup SDL resources
    cleanupSDL(sdlContext);

    return 0;
}