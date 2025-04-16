#include "menu.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>

void displayMenu(SDL_Renderer* renderer, TTF_Font* font, SDL_Texture* splashTexture, const std::vector<std::string>& menuItems, int selectedIndex, bool isPanning, int splashPanOffset, int initialPanOffset, int windowWidth, int windowHeight) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // Temporarily comment out the splash image rendering
    
    if (splashTexture != nullptr) {
        SDL_Rect srcRect;
        SDL_Rect destRect = {0, 0, windowWidth, windowHeight};

        if (isPanning && initialPanOffset > 0) {
            srcRect.x = 0;
            srcRect.y = splashPanOffset;
            srcRect.w = 1024;
            srcRect.h = windowHeight;

            Uint8 alpha = static_cast<Uint8>((static_cast<float>(splashPanOffset) / initialPanOffset) * 255);
            SDL_SetTextureAlphaMod(splashTexture, alpha);
        } else {
            srcRect.x = 0;
            srcRect.y = 1536 - windowHeight;
            srcRect.w = 1024;
            srcRect.h = windowHeight;
            SDL_SetTextureAlphaMod(splashTexture, 255);
        }
        SDL_RenderCopy(renderer, splashTexture, &srcRect, &destRect);
    }
    

    SDL_Color textColor = {255, 255, 255}; // White text
    SDL_Color selectedColor = {255, 255, 0}; // Yellow for selected item

    int yOffset = 200; // Adjust the starting position of the menu items

    for (size_t i = 0; i < menuItems.size(); ++i) {
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, menuItems[i].c_str(), (static_cast<int>(i) == selectedIndex) ? selectedColor : textColor);
        if (textSurface == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_RenderText_Solid failed! SDL_Error: %s\n", TTF_GetError());
            continue;
        }

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateTextureFromSurface failed! SDL_Error: %s\n", TTF_GetError());
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