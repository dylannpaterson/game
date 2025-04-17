#include "ui.h"
#include "asset_manager.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>

// Assuming renderText is defined in 'utils.cpp' (after being declared in
// 'utils.h') If renderText is still in main.cpp, you might need to move it to
// utils.cpp and include utils.h here as well.

void renderUI(GameData &gameData, AssetManager &assets) {
  SDL_Color textColor = {255, 255, 255, 255}; // White color

  std::string healthText =
      "Health: " + std::to_string(gameData.currentGamePlayer.health) + "/" +
      std::to_string(gameData.currentGamePlayer.maxHealth);
  SDL_Texture *healthTexture = renderText(
      gameData.renderer, assets.getFont("main_font"), healthText, textColor);
  if (healthTexture != nullptr) {
    SDL_Rect healthRect = {10, 10, 0, 0}; // Position at top-left
    SDL_QueryTexture(healthTexture, nullptr, nullptr, &healthRect.w,
                     &healthRect.h);
    SDL_RenderCopy(gameData.renderer, healthTexture, nullptr, &healthRect);
    SDL_DestroyTexture(healthTexture);
  }

  std::string manaText =
      "Mana: " + std::to_string(gameData.currentGamePlayer.mana) + "/" +
      std::to_string(gameData.currentGamePlayer.maxMana);
  SDL_Texture *manaTexture = renderText(
      gameData.renderer, assets.getFont("main_font"), manaText, textColor);
  if (manaTexture != nullptr) {
    SDL_Rect manaRect = {10, 40, 0, 0}; // Position below health
    SDL_QueryTexture(manaTexture, nullptr, nullptr, &manaRect.w, &manaRect.h);
    SDL_RenderCopy(gameData.renderer, manaTexture, nullptr, &manaRect);
    SDL_DestroyTexture(manaTexture);
  }

  std::string levelText =
      "Level: " + std::to_string(gameData.currentGamePlayer.level);
  SDL_Texture *levelTexture = renderText(
      gameData.renderer, assets.getFont("main_font"), levelText, textColor);
  if (levelTexture != nullptr) {
    SDL_Rect levelRect = {gameData.windowWidth - 150, 10, 0,
                          0}; // Position at top-right
    SDL_QueryTexture(levelTexture, nullptr, nullptr, &levelRect.w,
                     &levelRect.h);
    SDL_RenderCopy(gameData.renderer, levelTexture, nullptr, &levelRect);
    SDL_DestroyTexture(levelTexture);
  }

  std::string floorText =
      "Floor: " + std::to_string(gameData.currentLevelIndex);
  SDL_Texture *floorTexture = renderText(
      gameData.renderer, assets.getFont("main_font"), floorText, textColor);
  if (floorTexture != nullptr) {
    SDL_Rect floorRect = {gameData.windowWidth - 150, 40, 0,
                          0}; // Position below level
    SDL_QueryTexture(floorTexture, nullptr, nullptr, &floorRect.w,
                     &floorRect.h);
    SDL_RenderCopy(gameData.renderer, floorTexture, nullptr, &floorRect);
    SDL_DestroyTexture(floorTexture);
  }
}

// --- Updated renderSpellMenu using existing utils::renderText ---
void renderSpellMenu(SDL_Renderer *renderer, TTF_Font *font,
                     const PlayerCharacter &player, int currentSelectionIndex,
                     int windowWidth, int windowHeight) {
  if (player.knownSpells.empty())
    return; // Don't draw if no spells

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
  SDL_SetRenderDrawBlendMode(renderer,
                             SDL_BLENDMODE_BLEND); // Enable alpha blending
  SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
  SDL_RenderFillRect(renderer, &bgRect);
  SDL_SetRenderDrawBlendMode(renderer,
                             SDL_BLENDMODE_NONE); // Disable alpha blending

  // --- Draw Spell Items ---
  int textOffsetY = 10; // Padding at the top
  for (int i = 0; i < player.knownSpells.size(); ++i) {
    const Spell &spell = player.knownSpells[i];
    // Combine spell name and mana cost into one string
    std::string spellText =
        spell.name + " (" + std::to_string(spell.manaCost) + " Mana)";

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
        textColor = {100, 100, 100,
                     255}; // Darker gray if not selected and unaffordable
      }
    }

    // --- Use the renderText function from utils.h/.cpp ---
    SDL_Texture *textTexture =
        renderText(renderer, font, spellText,
                   textColor); // Your function returns a texture

    if (textTexture) {
      int textW, textH;
      // Get the dimensions of the created texture
      SDL_QueryTexture(textTexture, nullptr, nullptr, &textW, &textH);

      // Calculate the destination rectangle for rendering the texture
      SDL_Rect dstRect = {menuX + 15, menuY + textOffsetY + (i * itemHeight),
                          textW, textH}; // Position with padding

      // Render the texture to the screen
      SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);

      // *** IMPORTANT: Destroy the texture immediately after rendering ***
      // Since you create a new texture for each item each frame, you MUST
      // destroy it.
      SDL_DestroyTexture(textTexture);
    }
    // Note: Error logging for texture creation failure should be handled within
    // your renderText function.
  }
}

void renderSpellBar(GameData &gameData, AssetManager &assets) {
  const int MAX_BAR_SLOTS = 5; // For QWERT
  const std::vector<std::string> keyLabels = {"Q", "W", "E", "R", "T"};

  // Get needed resources
  SDL_Renderer *renderer = gameData.renderer;
  TTF_Font *font = assets.getFont("spellbar_font"); // Or a smaller font if desired
  const auto &spells = gameData.currentGamePlayer.knownSpells;

  if (!font || spells.empty())
    return; // Can't render without font or spells

  // --- Bar Geometry & Appearance ---
  int slotSize = 70; // Width/Height of each square slot
  int padding = 10;  // Padding between slots
  int totalBarWidth =
      (slotSize * MAX_BAR_SLOTS) + (padding * (MAX_BAR_SLOTS - 1));
  int barHeight = slotSize + 2 * padding; // Height including padding
  int barX = (gameData.windowWidth - totalBarWidth) / 2; // Center horizontally
  int barY = gameData.windowHeight - barHeight - 10;     // Position near bottom

  SDL_Color barBgColor = {20, 20, 40, 180}; // Dark blue, semi-transparent
  SDL_Color slotBgColor = {40, 40, 60, 200};
  SDL_Color slotBorderColor = {100, 100, 120, 255};
  SDL_Color keyTextColor = {220, 220, 220, 255}; // Light gray for key label
  SDL_Color cannotCastColor = {150, 50, 50,
                               255}; // Dim red overlay/text if cannot cast

  // --- Draw Bar Background ---
  SDL_Rect barRect = {barX, barY, totalBarWidth, barHeight};
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, barBgColor.r, barBgColor.g, barBgColor.b,
                         barBgColor.a);
  SDL_RenderFillRect(renderer, &barRect);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  // --- Draw Slots ---
  int currentSlotX = barX + padding;
  for (int i = 0; i < MAX_BAR_SLOTS && i < spells.size(); ++i) {
    const Spell &spell = spells[i];
    bool canCast = gameData.currentGamePlayer.mana >= spell.manaCost;

    SDL_Rect slotRect = {currentSlotX, barY + padding, slotSize, slotSize};

    // Draw Slot BG and Border
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, slotBgColor.r, slotBgColor.g,
                           slotBgColor.b, slotBgColor.a);
    SDL_RenderFillRect(renderer, &slotRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, slotBorderColor.r, slotBorderColor.g,
                           slotBorderColor.b, slotBorderColor.a);
    SDL_RenderDrawRect(renderer, &slotRect);

    // Draw Spell Icon (Placeholder - replace with actual icon rendering later)
    SDL_Texture* iconTex = assets.getTexture(spell.iconName);
    if (iconTex) {
        int iconPadding = 2; // Padding inside the slot for the icon
        SDL_Rect iconDestRect = {
            slotRect.x + iconPadding,
            slotRect.y + iconPadding,
            slotRect.w - (2 * iconPadding), // Make icon slightly smaller than slot
            slotRect.h - (2 * iconPadding)
        };
        SDL_RenderCopy(gameData.renderer, iconTex, nullptr, &iconDestRect);
    } else {
        // Fallback: Draw placeholder if texture wasn't loaded or name is wrong
        SDL_Rect iconPlaceholderRect = { slotRect.x + 5, slotRect.y + 5, slotRect.w - 10, slotRect.h - 10 };
        SDL_SetRenderDrawColor(gameData.renderer, 40, 40, 40, 255); // Dark gray placeholder
        SDL_RenderFillRect(gameData.renderer, &iconPlaceholderRect);
    }

    SDL_Texture* keyTexture = nullptr;
    int keyW = 0, keyH = 0;

    if (font) {
         // Determine text color (Can still dim if unaffordable, or keep bright)
         SDL_Color currentKeyColor = keyTextColor;
         // Optional: Dim key text too if cannot cast
         // if (!canCast) { currentKeyColor = {120, 120, 120, 255}; }

         keyTexture = renderText(renderer, font, keyLabels[i], currentKeyColor);
         if (keyTexture) {
             SDL_QueryTexture(keyTexture, nullptr, nullptr, &keyW, &keyH);

             // Position: Bottom Left (Calculation remains the same)
             int keyPadding = 3;
             SDL_Rect keyDestRect = {
                 slotRect.x + keyPadding,
                 slotRect.y + slotRect.h - keyH - keyPadding,
                 keyW,
                 keyH
             };

             SDL_RenderCopy(renderer, keyTexture, nullptr, &keyDestRect);
             SDL_DestroyTexture(keyTexture);
        }
    }

    // Indicate "Cannot Cast" Overlay (Optional)
    if (!canCast) {
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(renderer, cannotCastColor.r, cannotCastColor.g,
                             cannotCastColor.b,
                             100); // Semi-transparent red overlay
      SDL_RenderFillRect(renderer, &slotRect);
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    // Advance X for the next slot
    currentSlotX += slotSize + padding;
  }
}