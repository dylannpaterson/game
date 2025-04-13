#ifndef MENU_H
#define MENU_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>

void displayMenu(SDL_Renderer* renderer, TTF_Font* font, SDL_Texture* splashTexture, const std::vector<std::string>& menuItems, int selectedIndex, bool isPanning, int splashPanOffset, int initialPanOffset, int windowWidth, int windowHeight);

#endif // MENU_H