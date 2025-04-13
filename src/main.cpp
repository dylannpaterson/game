#include <iostream>
#include "character.h"
#include "menu.h"
#include "character_select.h"
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
#endif

void clearScreen() {
    // No need for this anymore as we'll be clearing the SDL renderer
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
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_image could not initialize! IMG_Error: %s\n", SDL_GetError());
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

    TTF_Font* font = TTF_OpenFont("../assets/fonts/StarlightRune.ttf", 36);
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
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create texture from image! SDL_Error: %s\n", SDL_GetError());
        } else{
            SDL_SetTextureBlendMode(splashTexture, SDL_BLENDMODE_BLEND);
        }
        SDL_FreeSurface(loadedSurface);
    }


    SDL_Event event;
    std::vector<std::string> menuItems = {"Start Game", "Options", "Exit"};
    int selectedIndex = 0;
    bool running = true;
    bool gameStarted = false;
    bool inCharacterSelect = false;
    int selectedCharacterIndex = 0; // Add this line
    bool isPanning = false;
    int splashPanOffset = 1536 - windowHeight; // Initial offset to show the bottom

    int panCounter = 0; // Add this line before the while loop

    while (running) {
        if (!gameStarted) {
            displayMenu(renderer, font, splashTexture, menuItems, selectedIndex, isPanning, splashPanOffset,456, windowWidth, windowHeight);
        } else if (isPanning) { // Continue calling displayMenu while panning
            displayMenu(renderer, font, splashTexture, menuItems, selectedIndex, isPanning, splashPanOffset,456, windowWidth, windowHeight);
        } else if (inCharacterSelect) {
            displayCharacterSelect(renderer, font, selectedCharacterIndex, windowWidth, windowHeight); // Pass the selected index
        } else {
            // Game loop (to be implemented)
        }

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
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        if (!gameStarted && !isPanning) {
                            selectedIndex = (selectedIndex > 0) ? selectedIndex - 1 : menuItems.size() - 1;
                        }
                        break;
                    case SDLK_DOWN:
                        if (!gameStarted && !isPanning) {
                            selectedIndex = (selectedIndex < menuItems.size() - 1) ? selectedIndex + 1 : 0;
                        }
                        break;
                    case SDLK_LEFT:
                        if (inCharacterSelect) {
                            selectedCharacterIndex = (selectedCharacterIndex > 0) ? selectedCharacterIndex - 1 : 1; // Cycle between 0 and 1
                        }
                        break;
                    case SDLK_RIGHT:
                        if (inCharacterSelect) {
                            selectedCharacterIndex = (selectedCharacterIndex < 1) ? selectedCharacterIndex + 1 : 0; // Cycle between 0 and 1
                        }
                        break;
                    case SDLK_RETURN: // Enter key
                        if (!gameStarted && !isPanning && selectedIndex == 0) {
                            std::cout << "\nEmbarking on the adventure (SDL with Image)..." << std::endl;
                            gameStarted = true;
                            isPanning = true; // Start panning
                        } else if (!gameStarted && !isPanning && selectedIndex == 1) {
                            std::cout << "\nOptions menu not implemented yet (SDL with Image)." << std::endl;
                        } else if (!gameStarted && !isPanning && selectedIndex == 2) {
                            std::cout << "\nExiting game (SDL with Image). Farewell, Mortal." << std::endl;
                            running = false;
                        } else if (gameStarted && inCharacterSelect) {
                            PlayerCharacter player; // Create an instance of our structure
                            if (selectedCharacterIndex == 0) {
                                player.type = CharacterType::FemaleMage;
                                std::cout << "\nCharacter Selected: Female Mage" << std::endl;
                            } else if (selectedCharacterIndex == 1) {
                                player.type = CharacterType::MaleMage;
                                std::cout << "\nCharacter Selected: Male Mage" << std::endl;
                            }
                            gameStarted = false; // Exit character select
                            inCharacterSelect = false;
                            // Next, we would transition to the game loop, possibly passing this 'player' object
                            std::cout << "\n--- Preparing to Enter the Game Loop ---" << std::endl;
                            running = false; // For now, exit the main loop after selection
                        }
                        break;
                    case SDLK_ESCAPE: // Optional: Handle Escape key to quit
                        running = false;
                        break;
                }
            }
        }

        if (isPanning) {
            splashPanOffset = 456 - panCounter; // Set offset based on counter
            panCounter += 5; // Adjust speed here
            if (splashPanOffset <= 0) {
                splashPanOffset = 0;
                isPanning = false;
                inCharacterSelect = true; // Transition to character select after pan
                panCounter = 0; // Reset counter
            }
        }

        SDL_Delay(10);
    }

    // Here is where the game loop will begin if gameStarted is true
    if (gameStarted) {
        std::cout << "\n--- Entering the Game Loop ---" << std::endl;
        // You will add your game loop code here in our next steps
    }

    SDL_DestroyTexture(splashTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}