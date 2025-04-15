#include <iostream>
#include "character.h"
#include "menu.h"
#include "character_select.h"
#include "level.h" // Include the level header
#include <vector>
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h> // Include SDL_image
#include <cmath> // For std::abs

#ifdef _WIN32
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "SDL2_ttf.lib")
#pragma comment(lib, "SDL2_image.lib")
#pragma comment(lib, "SDL2_image.lib")
#endif

// Function to render text to a texture
SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color) {
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (textSurface == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
        return nullptr;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    return textTexture;
}

bool isWithinBounds(int x, int y, int width, int height) {
    return x >= 0 && x < width && y >= 0 && y < height;
}

void updateVisibility(const Level& level, const std::vector<SDL_Rect>& rooms, int playerX, int playerY, int hallwayVisibilityDistance, std::vector<std::vector<float>>& visibilityMap) {
    int width = level.width;
    int height = level.height;
    int brightRadius = 7;
    int dimRadius = 10;
    int rayThickness = 1;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            visibilityMap[y][x] = 0.0f; // Initialize with float
        }
    }

    auto isWithinBoundsFunc = [&](int x, int y, int w, int h) {
        return x >= 0 && x < w && y >= 0 && y < h;
    };

    auto canPass = [&](int x, int y) {
        return isWithinBoundsFunc(x, y, width, height) && level.tiles[y][x] != '#';
    };

    auto castRay = [&](int startX, int startY, int endX, int endY, float brightness) {
        for (int offsetX = -rayThickness; offsetX <= rayThickness; ++offsetX) {
            for (int offsetY = -rayThickness; offsetY <= rayThickness; ++offsetY) {
                int x0 = startX + offsetX;
                int y0 = startY + offsetY;
                int x1 = endX;
                int y1 = endY;

                if (isWithinBoundsFunc(x0, y0, width, height) && level.tiles[y0][x0] != '#') {
                    int dx_ray = std::abs(x1 - x0);
                    int dy_ray = std::abs(y1 - y0);
                    int sx = (x0 < x1) ? 1 : -1;
                    int sy = (y0 < y1) ? 1 : -1;
                    int err = dx_ray - dy_ray;
                    int currentX = x0;
                    int currentY = y0;
                    bool blocked = false;

                    while (true) {
                        if (isWithinBoundsFunc(currentX, currentY, width, height)) {
                            if (currentX == endX && currentY == endY) {
                                visibilityMap[currentY][currentX] = std::max(visibilityMap[currentY][currentX], brightness);
                                return true;
                            }
                            if ((level.tiles[currentY][currentX] == '#' || level.tiles[currentY][currentX] == 'V') && (currentX != x0 || currentY != y0)) {
                                blocked = true;
                                break;
                            }
                        } else {
                            break;
                        }

                        int e2 = 2 * err;
                        if (e2 > -dy_ray) {
                            err -= dy_ray;
                            currentX += sx;
                        }
                        if (e2 < dx_ray) {
                            err += dx_ray;
                            currentY += sy;
                        }
                    }
                    if (!blocked && (currentX == endX && currentY == endY)) return true;
                }
            }
        }
        return false;
    };

    for (int dy = -dimRadius; dy <= dimRadius; ++dy) {
        for (int dx = -dimRadius; dx <= dimRadius; ++dx) {
            int targetX = playerX + dx;
            int targetY = playerY + dy;

            if (!isWithinBoundsFunc(targetX, targetY, width, height) || level.tiles[targetY][targetX] == 'V') continue;

            float distance = std::sqrt(dx * dx + dy * dy);
            float brightness = 0.0f;

            if (distance < brightRadius) {
                brightness = 1.0f; // 100% bright
            } else if (distance >= brightRadius && distance < dimRadius) {
                // Linear falloff from 100% to 0%
                brightness = 1.0f - (distance - brightRadius) / (dimRadius - brightRadius);
                brightness = std::max(0.0f, std::min(1.0f, brightness)); // Ensure within 0-1
            }

            if (brightness > 0.0f) {
                castRay(playerX, playerY, targetX, targetY, brightness);
            }
        }
    }
}

int main(int argc, char* argv[]) {

    int windowWidth = 1920;
    int windowHeight = 1080;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) { // Initialize only the video subsystem for now
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG; // You can add other formats like IMG_INIT_JPG
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_image could not initialize! IMG_Error: %s\n", IMG_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Wizard Roguelike", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);

    float aspectRatio = static_cast<float>(windowWidth) / windowHeight;

    if (window == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, windowWidth, windowHeight); // Set your initial resolution as the logical size

    TTF_Font* font = TTF_OpenFont("../assets/fonts/LUMOS.TTF", 36);
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
    Uint8 characterSelectAlpha = 0;         // New variable for alpha value
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
    PlayerCharacter currentGamePlayer({CharacterType::FemaleMage, 100, 100, 150, 150, 1, 0, 0, tileWidth, tileHeight}); // Initialize here with tile dimensions
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
            } else if (event.type == SDL_KEYDOWN) {
                if (!gameStarted && !isPanning) { // Menu input
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
                } else if (gameStarted && !inCharacterSelect && !currentGamePlayer.isMoving) { // Game input (movement)
                    int targetX = currentGamePlayer.targetTileX; // Use targetTileX
                    int targetY = currentGamePlayer.targetTileY; // Use targetTileY
                    switch (event.key.keysym.sym) {
                        case SDLK_UP:
                            targetY--;
                            break;
                        case SDLK_DOWN:
                            targetY++;
                            break;
                        case SDLK_LEFT:
                            targetX--;
                            break;
                        case SDLK_RIGHT:
                            targetX++;
                            break;
                    }
                    if (isWithinBounds(targetX, targetY, currentLevel.width, currentLevel.height) && currentLevel.tiles[targetY][targetX] != '#') {
                        currentGamePlayer.startMove(targetX, targetY); // Call startMove
                        updateVisibility(currentLevel, levelRooms, targetX, targetY, hallwayVisibilityDistance, visibilityMap);
                        playerX = targetX; // Update playerX and Y for visibility
                        playerY = targetY;
                    }
                } else if (inCharacterSelect) { // Character select input
                    switch (event.key.keysym.sym) {
                        case SDLK_LEFT:
                            selectedCharacterIndex = (selectedCharacterIndex > 0) ? selectedCharacterIndex - 1 : 1; // Cycle between 0 and 1
                            break;
                        case SDLK_RIGHT:
                            selectedCharacterIndex = (selectedCharacterIndex < 1) ? selectedCharacterIndex + 1 : 0; // Cycle between 0 and 1
                            break;
                        case SDLK_RETURN: // Enter key
                        { // Start of new scope to fix "jump to case label"
                            PlayerCharacter player({CharacterType::FemaleMage, 100, 100, 150, 150, 1, 0, 0, tileWidth, tileHeight}); // Create an instance
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
                            currentLevel = generateLevel(levelWidth, levelHeight, levelMaxRooms, levelMinRoomSize, levelMaxRoomSize); // Example parameters
                            levelRooms = currentLevel.rooms;

                            std::cout << "Number of rooms: " << levelRooms.size() << std::endl;
                            for (size_t i = 0; i < levelRooms.size(); ++i) {
                                std::cout << "Room " << i << ": x=" << levelRooms[i].x
                                          << ", y=" << levelRooms[i].y
                                          << ", w=" << levelRooms[i].w
                                          << ", h=" << levelRooms[i].h << std::endl;
                            }
                            std::cout << "Player starting position: x=" << playerX << ", y=" << playerY << std::endl;

                            std::cout << "Number of rooms after assignment in main.cpp: " << levelRooms.size() << std::endl; // ADD THIS LINE

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
                            break;
                        } // End of new scope
                        case SDLK_ESCAPE: // Optional: Handle Escape key to quit
                            running = false;
                            break;
                    }
                }
            }
        }

        // Update game state
        if (gameStarted && !inCharacterSelect) {
            currentGamePlayer.update(deltaTime, tileWidth, tileHeight); // Call the update function
        }

        if (!gameStarted) {
            displayMenu(renderer, font, splashTexture, menuItems, selectedIndex, isPanning, splashPanOffset,456, windowWidth, windowHeight);
        } else if (isPanning) {
            displayMenu(renderer, font, splashTexture, menuItems, selectedIndex, isPanning, splashPanOffset,456, windowWidth, windowHeight);
        } else if (inCharacterSelect) {
            displayCharacterSelect(renderer, font, selectedCharacterIndex, windowWidth, windowHeight, characterSelectAlpha); // Pass the alpha value
        } else if (gameStarted && !inCharacterSelect) {
            // Game loop

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
                cameraY = (currentLevel.height * tileHeight) - windowHeight;
            }

            // Check for exit collision AFTER player movement
            if (currentGamePlayer.targetTileY == currentLevel.endRow && currentGamePlayer.targetTileX == currentLevel.endCol && !currentGamePlayer.isMoving) { // Use targetTileX and targetTileY
                currentLevelIndex++; // Increment to the next level
                Level nextLevel = generateLevel(levelWidth, levelHeight, levelMaxRooms, levelMinRoomSize, levelMaxRoomSize); // Example parameters
                if (!nextLevel.tiles.empty()) {
                    currentLevel = nextLevel;
                    levelRooms = currentLevel.rooms;
                    currentGamePlayer.targetTileX = currentLevel.startCol; // Use targetTileX
                    currentGamePlayer.targetTileY = currentLevel.startRow; // Use targetTileY
                    currentGamePlayer.x = currentGamePlayer.targetTileX * tileWidth + tileWidth / 2.0f;
                    currentGamePlayer.y = currentGamePlayer.targetTileY * tileHeight + tileHeight / 2.0f;
                    visibilityMap.resize(currentLevel.height, std::vector<float>(currentLevel.width, 0.0f)); // Re-initialize with float
                    updateVisibility(currentLevel, levelRooms, currentGamePlayer.targetTileX, currentGamePlayer.targetTileY, hallwayVisibilityDistance, visibilityMap); // Use targetTileX and targetTileY
                    std::cout << "--- Entering Level " << currentLevelIndex << " ---" << std::endl;
                } else {
                    // If the next level file doesn't exist, perhaps we've reached the end
                    std::cout << "--- End of Levels ---" << std::endl;
                    currentLevelIndex = 1; // Loop back to the first level for now
                    currentLevel = generateLevel(levelWidth, levelHeight, levelMaxRooms, levelMinRoomSize, levelMaxRoomSize); // Example parameters
                    levelRooms = currentLevel.rooms;
                    currentGamePlayer.targetTileX = currentLevel.startCol; // Use targetTileX
                    currentGamePlayer.targetTileY = currentLevel.startRow; // Use targetTileY
                    currentGamePlayer.x = currentGamePlayer.targetTileX * tileWidth + tileWidth / 2.0f;
                    currentGamePlayer.y = currentGamePlayer.targetTileY * tileHeight + tileHeight / 2.0f;
                    visibilityMap.resize(currentLevel.height, std::vector<float>(currentLevel.width, 0.0f)); // Re-initialize with float
                    updateVisibility(currentLevel, levelRooms, currentGamePlayer.targetTileX, currentGamePlayer.targetTileY, hallwayVisibilityDistance, visibilityMap); // Use targetTileX and targetTileY
                    std::cout << "--- Returning to Level " << currentLevelIndex << " ---" << std::endl;
                }
            }

            // 3. Render the Game World
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(renderer);

            // Render the level
            if (!currentLevel.tiles.empty()) {
                for (int y = 0; y < currentLevel.height; ++y) {
                    for (int x = 0; x < currentLevel.width; ++x) {
                        SDL_Rect tileRect = {(x * tileWidth) - cameraX, (y * tileHeight) - cameraY, tileWidth, tileHeight};
                        if (visibilityMap[y][x] > 0.0f) { // Only render visible tiles
                            // Render the base tile color as before
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

            // Render the player character at the interpolated position
            SDL_Rect playerRect;
            playerRect.w = tileWidth;
            playerRect.h = tileHeight;
            playerRect.x = static_cast<int>(currentGamePlayer.x - tileWidth / 2) - cameraX; // Apply camera offset
            playerRect.y = static_cast<int>(currentGamePlayer.y - tileHeight / 2) - cameraY; // Apply camera offset
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer, &playerRect);

            // Render UI elements
            SDL_Color textColor = {255, 255, 255, 255}; // White color

            std::string healthText = "Health: " + std::to_string(currentGamePlayer.health) + "/" + std::to_string(currentGamePlayer.maxHealth);
            SDL_Texture* healthTexture = renderText(renderer, font, healthText, textColor);
            if (healthTexture != nullptr) {
                SDL_Rect healthRect = {10, 10, 0, 0}; // Position at top-left
                SDL_QueryTexture(healthTexture, nullptr, nullptr, &healthRect.w, &healthRect.h);
                SDL_RenderCopy(renderer, healthTexture, nullptr, &healthRect);
                SDL_DestroyTexture(healthTexture);
            }

            std::string manaText = "Mana: " + std::to_string(currentGamePlayer.mana) + "/" + std::to_string(currentGamePlayer.maxMana);
            SDL_Texture* manaTexture = renderText(renderer, font, manaText, textColor);
            if (manaTexture != nullptr) {
                SDL_Rect manaRect = {10, 40, 0, 0}; // Position below health
                SDL_QueryTexture(manaTexture, nullptr, nullptr, &manaRect.w, &manaRect.h);
                SDL_RenderCopy(renderer, manaTexture, nullptr, &manaRect);
                SDL_DestroyTexture(manaTexture);
            }

            std::string levelText = "Level: " + std::to_string(currentGamePlayer.level);
            SDL_Texture* levelTexture = renderText(renderer, font, levelText, textColor);
            if (levelTexture != nullptr) {
                SDL_Rect levelRect = {windowWidth - 150, 10, 0, 0}; // Position at top-right
                SDL_QueryTexture(levelTexture, nullptr, nullptr, &levelRect.w, &levelRect.h);
                SDL_RenderCopy(renderer, levelTexture, nullptr, &levelRect);
                SDL_DestroyTexture(levelTexture);
            }

            std::string floorText = "Floor: " + std::to_string(currentLevelIndex);
            SDL_Texture* floorTexture = renderText(renderer, font, floorText, textColor);
            if (floorTexture != nullptr) {
                SDL_Rect floorRect = {windowWidth - 150, 40, 0, 0}; // Position below level
                SDL_QueryTexture(floorTexture, nullptr, nullptr, &floorRect.w, &floorRect.h);
                SDL_RenderCopy(renderer, floorTexture, nullptr, &floorRect);
                SDL_DestroyTexture(floorTexture);
            }

            SDL_RenderPresent(renderer);
        }

        if (isPanning) {
            splashPanOffset = 456 - panCounter;
            panCounter += 10;
            if (splashPanOffset <= 0) {
                splashPanOffset = 0;
                isPanning = false;
                inCharacterSelect = true;
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

        SDL_Delay(10);
    }


    SDL_DestroyTexture(exitTexture);
    SDL_DestroyTexture(startTexture);
    SDL_DestroyTexture(splashTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}