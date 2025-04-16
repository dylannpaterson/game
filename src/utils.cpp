#include "utils.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h> // Include SDL_image here as well
#include <iostream>

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

SDL_Context initializeSDL(int width, int height) {
    SDL_Context context;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return context; // Return empty context on error
    }

    if (TTF_Init() < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return context; // Return empty context on error
    }

    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_image could not initialize! IMG_Error: %s\n", IMG_GetError());
        TTF_Quit();
        SDL_Quit();
        return context; // Return empty context on error
    }

    context.window = SDL_CreateWindow("Wizard Roguelike", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
    if (context.window == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return context; // Return empty context on error
    }

    context.renderer = SDL_CreateRenderer(context.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (context.renderer == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(context.window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return context; // Return empty context on error
    }

    SDL_RenderSetLogicalSize(context.renderer, width, height);

    context.font = TTF_OpenFont("../assets/fonts/LUMOS.TTF", 36);
    if (context.font == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load font! TTF_Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(context.renderer);
        SDL_DestroyWindow(context.window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return context; // Return empty context on error
    }

    return context;
}

// Define the cleanup function
void cleanupSDL(SDL_Context& context) {
    if (context.font != nullptr) {
        TTF_CloseFont(context.font);
    }
    if (context.renderer != nullptr) {
        SDL_DestroyRenderer(context.renderer);
    }
    if (context.window != nullptr) {
        SDL_DestroyWindow(context.window);
    }
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}