#ifndef CHARACTER_SELECT_H
#define CHARACTER_SELECT_H

#include <SDL.h>
#include <SDL_ttf.h>

void displayCharacterSelect(SDL_Renderer* renderer, TTF_Font* font, int selectedIndex, int windowWidth, int windowHeight, Uint8 alpha); // Added alpha parameter

#endif // CHARACTER_SELECT_H