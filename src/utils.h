#ifndef UTILS_H
#define UTILS_H

#include "game_data.h"
#include <SDL.h>
#include <SDL_image.h> // Include SDL_image here as well
#include <SDL_ttf.h>
#include <string>

// Forward-declare RuneType to avoid circular dependency if game_data.h includes
// utils.h
enum class RuneType;

SDL_Texture *renderText(SDL_Renderer *renderer, TTF_Font *font,
                        const std::string &text, SDL_Color color);
bool isWithinBounds(int x, int y, int width, int height);

struct SDL_Context {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
};

SDL_Context initializeSDL(int width, int height);
void cleanupSDL(SDL_Context &context); // Function to handle SDL cleanup
bool findNearestValidTarget(const GameData &gameData, int spellIndex,
                            SDL_Point &outTargetPos);
int rollDice(int numDice, int dieType, int bonus);
std::vector<std::pair<int, int>> getLineTiles(int x0, int y0, int x1, int y1);

int getRandomInt(int min,
                 int max); // Assuming this or similar exists or you'll add it
RuneType getRandomRune();

#endif // UTILS_H