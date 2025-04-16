#include "ui.h"
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>

// Assuming renderText is defined in 'utils.cpp' (after being declared in 'utils.h')
// If renderText is still in main.cpp, you might need to move it to utils.cpp
// and include utils.h here as well.

void renderUI(SDL_Renderer* renderer, TTF_Font* font, const PlayerCharacter& player, int currentLevelIndex, int windowWidth) {
    SDL_Color textColor = {255, 255, 255, 255}; // White color

    std::string healthText = "Health: " + std::to_string(player.health) + "/" + std::to_string(player.maxHealth);
    SDL_Texture* healthTexture = renderText(renderer, font, healthText, textColor);
    if (healthTexture != nullptr) {
        SDL_Rect healthRect = {10, 10, 0, 0}; // Position at top-left
        SDL_QueryTexture(healthTexture, nullptr, nullptr, &healthRect.w, &healthRect.h);
        SDL_RenderCopy(renderer, healthTexture, nullptr, &healthRect);
        SDL_DestroyTexture(healthTexture);
    }

    std::string manaText = "Mana: " + std::to_string(player.mana) + "/" + std::to_string(player.maxMana);
    SDL_Texture* manaTexture = renderText(renderer, font, manaText, textColor);
    if (manaTexture != nullptr) {
        SDL_Rect manaRect = {10, 40, 0, 0}; // Position below health
        SDL_QueryTexture(manaTexture, nullptr, nullptr, &manaRect.w, &manaRect.h);
        SDL_RenderCopy(renderer, manaTexture, nullptr, &manaRect);
        SDL_DestroyTexture(manaTexture);
    }

    std::string levelText = "Level: " + std::to_string(player.level);
    SDL_Texture* levelTexture = renderText(renderer, font, levelText, textColor);
    if (levelTexture != nullptr) {
        SDL_Rect levelRect = {windowWidth - 150, 10, 0, 0}; // Position at top-right
        SDL_QueryTexture(levelTexture, nullptr, nullptr, &levelRect.w, &levelRect.h);
        SDL_RenderCopy(renderer, levelTexture, nullptr, &levelRect);
        SDL_DestroyTexture(levelTexture);
    }

    std::string floorText = "Floor: " + std::to_string(currentLevelIndex);
    SDL_Texture* floorTexture = renderText(renderer, font, floorText, textColor);
    if (floorTexture != nullptr) {
        SDL_Rect floorRect = {windowWidth - 150, 40, 0, 0}; // Position below level
        SDL_QueryTexture(floorTexture, nullptr, nullptr, &floorRect.w, &floorRect.h);
        SDL_RenderCopy(renderer, floorTexture, nullptr, &floorRect);
        SDL_DestroyTexture(floorTexture);
    }
}