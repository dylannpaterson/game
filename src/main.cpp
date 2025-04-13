#include <iostream>
#include <vector>
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h> // Include SDL_image

#ifdef _WIN32
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "SDL2_ttf.lib")
#pragma comment(lib, "SDL2_image.lib")
#endif

void clearScreen() {
    // No need for this anymore as we'll be clearing the SDL renderer
}

void displayMenu(SDL_Renderer* renderer, TTF_Font* font, SDL_Texture* splashTexture, const std::vector<std::string>& menuItems, int selectedIndex) {
    // Render the splash image (if loaded)
    if (splashTexture != nullptr) {
        SDL_RenderCopy(renderer, splashTexture, nullptr, nullptr); // Render it to the full screen
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); // Black background if no image
        SDL_RenderClear(renderer);
    }

    SDL_Color textColor = {255, 255, 255}; // White text
    SDL_Color selectedColor = {255, 255, 0}; // Yellow for selected item

    int yOffset = 200; // Adjust the starting position of the menu items

    for (size_t i = 0; i < menuItems.size(); ++i) {
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, menuItems[i].c_str(), (static_cast<int>(i) == selectedIndex) ? selectedColor : textColor);
        if (textSurface == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_RenderText_Solid failed! SDL_Error: %s\n", SDL_GetError());
            continue;
        }

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateTextureFromSurface failed! SDL_Error: %s\n", SDL_GetError());
            SDL_FreeSurface(textSurface);
            continue;
        }

        SDL_Rect textRect;
        textRect.x = 100;
        textRect.y = yOffset + static_cast<int>(i) * 50;
        textRect.w = textSurface->w;
        textRect.h = textSurface->h;

        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);

        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
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

    SDL_Window* window = SDL_CreateWindow("Wizard Roguelike", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
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
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load image! IMG_Error: %s\n", SDL_GetError());
    } else {
        splashTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (splashTexture == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create texture from image! SDL_Error: %s\n", SDL_GetError());
        }
        SDL_FreeSurface(loadedSurface);
    }

    SDL_Event event;
    std::vector<std::string> menuItems = {"Start Game", "Options", "Exit"};
    int selectedIndex = 0;
    bool running = true;
    bool gameStarted = false; // Add a new boolean flag

    while (running) {
        displayMenu(renderer, font, splashTexture, menuItems, selectedIndex);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
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
                            gameStarted = true; // Set the game started flag
                            running = false; // Exit the menu loop
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