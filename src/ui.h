#ifndef UI_H
#define UI_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>

// Assuming PlayerCharacter is defined in 'player.h'
#include "character.h"

// Assuming renderText is declared in 'utils.h'
#include "utils.h"

void renderUI(SDL_Renderer* renderer, TTF_Font* font, const PlayerCharacter& player, int currentLevelIndex, int windowWidth);

#endif // UI_H