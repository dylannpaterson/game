// ui.h
#ifndef UI_H
#define UI_H

#include <SDL.h>
#include <SDL_ttf.h>
#include "character.h" // Needed for PlayerCharacter type in function signatures
#include <vector>      // Needed for std::vector in renderSpellMenu if not included via character.h
#include <string>      // Needed for std::string in renderSpellMenu if not included via character.h

// Assuming renderText declaration is in utils.h, which ui.cpp includes, or include utils.h here too
#include "utils.h"     // Make sure this is included if ui.cpp needs renderText directly

// Existing renderUI declaration
void renderUI(SDL_Renderer* renderer, TTF_Font* font, const PlayerCharacter& player, int currentLevelIndex, int windowWidth);

// --- Add declaration for renderSpellMenu ---
void renderSpellMenu(SDL_Renderer* renderer, TTF_Font* font, const PlayerCharacter& player, int currentSelectionIndex, int windowWidth, int windowHeight);
// --------------------------------------------

#endif // UI_H