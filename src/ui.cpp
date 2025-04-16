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



// --- Updated renderSpellMenu using existing utils::renderText ---
void renderSpellMenu(SDL_Renderer* renderer, TTF_Font* font, const PlayerCharacter& player, int currentSelectionIndex, int windowWidth, int windowHeight) {
    if (player.knownSpells.empty()) return; // Don't draw if no spells

    SDL_Log("renderSpellMenu called.");
    
    // --- Menu Appearance ---
    int menuWidth = 300;
    int itemHeight = 40; // Height per spell item
    int menuHeight = itemHeight * player.knownSpells.size() + 20; // + padding
    int menuX = (windowWidth - menuWidth) / 2;
    int menuY = (windowHeight - menuHeight) / 2;
    SDL_Color bgColor = {0, 0, 0, 180}; // Semi-transparent black background
    SDL_Color normalColor = {200, 200, 200, 255}; // Gray for non-selected spells
    SDL_Color selectedColor = {255, 255, 0, 255}; // Yellow for selected spell
    SDL_Color cannotCastColor = {150, 50, 50, 255}; // Dim red if not enough mana

    // --- Draw Background ---
    SDL_Rect bgRect = {menuX, menuY, menuWidth, menuHeight};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); // Enable alpha blending
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderFillRect(renderer, &bgRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE); // Disable alpha blending

    // --- Draw Spell Items ---
    int textOffsetY = 10; // Padding at the top
    for (int i = 0; i < player.knownSpells.size(); ++i) {
        const Spell& spell = player.knownSpells[i];
        // Combine spell name and mana cost into one string
        std::string spellText = spell.name + " (" + std::to_string(spell.manaCost) + " Mana)";

        SDL_Color textColor;
        // Determine text color based on selection and mana availability
        if (i == currentSelectionIndex) {
            textColor = selectedColor; // Highlight selected
        } else {
            textColor = normalColor; // Dimmer color for others
        }
        // Override color if player cannot cast the spell due to mana cost
        if (player.mana < spell.manaCost) {
             if (i == currentSelectionIndex) {
                 textColor = cannotCastColor; // Use dim red if selected but unaffordable
             } else {
                 textColor = {100, 100, 100, 255}; // Darker gray if not selected and unaffordable
             }
        }

        // --- Use the renderText function from utils.h/.cpp ---
        SDL_Texture* textTexture = renderText(renderer, font, spellText, textColor); // Your function returns a texture

        if (textTexture) {
            int textW, textH;
            // Get the dimensions of the created texture
            SDL_QueryTexture(textTexture, nullptr, nullptr, &textW, &textH);

            // Calculate the destination rectangle for rendering the texture
            SDL_Rect dstRect = {menuX + 15, menuY + textOffsetY + (i * itemHeight), textW, textH}; // Position with padding

            // Render the texture to the screen
            SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);

            // *** IMPORTANT: Destroy the texture immediately after rendering ***
            // Since you create a new texture for each item each frame, you MUST destroy it.
            SDL_DestroyTexture(textTexture);
        }
        // Note: Error logging for texture creation failure should be handled within your renderText function.
    }
}