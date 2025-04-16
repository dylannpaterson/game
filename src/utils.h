#ifndef UTILS_H
#define UTILS_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h> // Include SDL_image here as well
#include <string>

SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color);
bool isWithinBounds(int x, int y, int width, int height);

struct SDL_Context {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
};

SDL_Context initializeSDL(int width, int height);
void cleanupSDL(SDL_Context& context); // Function to handle SDL cleanup

#endif // UTILS_H