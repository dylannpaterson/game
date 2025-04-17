// ui.h
#ifndef UI_H
#define UI_H

#include <SDL.h>
#include <SDL_ttf.h>
#include "character.h" // Needed for PlayerCharacter type in function signatures
#include "asset_manager.h"
#include "game_data.h"

struct GameData;
class AssetManager;

// Assuming renderText declaration is in utils.h, which ui.cpp includes, or include utils.h here too
#include "utils.h"     // Make sure this is included if ui.cpp needs renderText directly

// Existing renderUI declaration
void renderUI(GameData& gameData, AssetManager& assets);

// --- Add declaration for renderSpellMenu ---
void renderSpellMenu(SDL_Renderer* renderer, TTF_Font* font, const PlayerCharacter& player, int currentSelectionIndex, int windowWidth, int windowHeight);
// --------------------------------------------

void renderSpellBar(GameData& gameData, AssetManager& assets);

#endif // UI_H