#include "ui.h"
#include "asset_manager.h"
#include "game_data.h" // Include game_data for PlayerCharacter definition within renderCharacterSheet
#include "utils.h"     // Include utils for the original renderText declaration

// **FIX: Add necessary includes BEFORE the helper function**
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <algorithm> // For std::min

// **FIX: Rename the helper function to avoid conflict**
// Helper function to render text at a specific position
void renderTextAtPos(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color, int x, int y) {
    if (!font || !renderer) { // Safety check
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "renderTextAtPos Error: Null font or renderer.");
        return;
    }
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_RenderText_Solid Error for sheet: %s", TTF_GetError());
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateTextureFromSurface Error for sheet: %s", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect destRect = { x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, nullptr, &destRect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}


// Definition for renderUI (Uses the original renderText from utils.h/cpp)
void renderUI(GameData &gameData, AssetManager &assets) {
  SDL_Color textColor = {255, 255, 255, 255}; // White color
  TTF_Font* font = assets.getFont("main_font"); // Get the font once

  if (!font) {
      SDL_Log("UI Error: Font 'main_font' not loaded!");
      return; // Cannot proceed without font
  }

  // --- Health Display ---
  SDL_Rect healthRect = {10, 10, 0, 0}; // Position at top-left
  std::string healthText =
      "Health: " + std::to_string(gameData.currentGamePlayer.health) + "/" +
      std::to_string(gameData.currentGamePlayer.maxHealth);
  // **Use the original renderText from utils.h/cpp**
  SDL_Texture *healthTexture = renderText(
      gameData.renderer, font, healthText, textColor);
  if (healthTexture != nullptr) {
      SDL_QueryTexture(healthTexture, nullptr, nullptr, &healthRect.w, &healthRect.h);
      SDL_RenderCopy(gameData.renderer, healthTexture, nullptr, &healthRect);
      SDL_DestroyTexture(healthTexture);
  } else {
       SDL_Log("Failed to render health text.");
  }


  // --- Mana Display ---
  SDL_Rect manaRect = {10, healthRect.y + healthRect.h + 5, 0, 0};
  std::string manaText =
      "Mana: " + std::to_string(gameData.currentGamePlayer.mana) + "/" +
      std::to_string(gameData.currentGamePlayer.maxMana);
  // **Use the original renderText from utils.h/cpp**
  SDL_Texture *manaTexture = renderText(
      gameData.renderer, font, manaText, textColor);
  if (manaTexture != nullptr) {
      SDL_QueryTexture(manaTexture, nullptr, nullptr, &manaRect.w, &manaRect.h);
      SDL_RenderCopy(gameData.renderer, manaTexture, nullptr, &manaRect);
      SDL_DestroyTexture(manaTexture);
  } else {
       SDL_Log("Failed to render mana text.");
  }


  // --- Arcana Display ---
  SDL_Rect arcanaRect = {10, manaRect.y + manaRect.h + 5, 0, 0};
  std::string arcanaText = "Arcana: " + std::to_string(gameData.currentGamePlayer.currentArcana);
   // **Use the original renderText from utils.h/cpp**
  SDL_Texture *arcanaTexture = renderText(
      gameData.renderer, font, arcanaText, textColor);
  if (arcanaTexture != nullptr) {
       SDL_QueryTexture(arcanaTexture, nullptr, nullptr, &arcanaRect.w, &arcanaRect.h);
       SDL_RenderCopy(gameData.renderer, arcanaTexture, nullptr, &arcanaRect);
       SDL_DestroyTexture(arcanaTexture);
   } else {
       SDL_Log("Failed to render Arcana text.");
   }


  // --- Level Display ---
  std::string levelText =
      "Level: " + std::to_string(gameData.currentGamePlayer.level);
   // **Use the original renderText from utils.h/cpp**
  SDL_Texture *levelTexture = renderText(
      gameData.renderer, font, levelText, textColor);
  if (levelTexture != nullptr) {
      SDL_Rect levelRect = {gameData.windowWidth - 150, 10, 0, 0};
      SDL_QueryTexture(levelTexture, nullptr, nullptr, &levelRect.w, &levelRect.h);
      levelRect.x = gameData.windowWidth - levelRect.w - 10; // Right-align
      SDL_RenderCopy(gameData.renderer, levelTexture, nullptr, &levelRect);
      SDL_DestroyTexture(levelTexture);
  } else {
       SDL_Log("Failed to render level text.");
  }

  // --- Floor Display ---
   SDL_Rect floorRect = {0, 0, 0, 0};
   std::string floorText =
       "Floor: " + std::to_string(gameData.currentLevelIndex);
    // **Use the original renderText from utils.h/cpp**
   SDL_Texture *floorTexture = renderText(
       gameData.renderer, font, floorText, textColor);
   if (floorTexture != nullptr) {
       SDL_QueryTexture(floorTexture, nullptr, nullptr, &floorRect.w, &floorRect.h);
       floorRect.x = gameData.windowWidth - floorRect.w - 10; // Right-align

       // Need levelRect height to position below it
       SDL_Rect tempLevelRect = {0,0,0,0};
       SDL_Texture *tempLevelTexture = renderText( gameData.renderer, font, levelText, textColor);
       if (tempLevelTexture) {
           SDL_QueryTexture(tempLevelTexture, nullptr, nullptr, &tempLevelRect.w, &tempLevelRect.h);
           SDL_DestroyTexture(tempLevelTexture);
           floorRect.y = 10 + tempLevelRect.h + 5; // Position below level text (Y=10 + level_height + padding)
       } else {
          floorRect.y = 40; // Fallback position
       }

       SDL_RenderCopy(gameData.renderer, floorTexture, nullptr, &floorRect);
       SDL_DestroyTexture(floorTexture);
   } else {
       SDL_Log("Failed to render floor text.");
   }
}

// --- Updated renderSpellMenu using original utils::renderText ---
void renderSpellMenu(SDL_Renderer *renderer, TTF_Font *font,
                     const PlayerCharacter &player, int currentSelectionIndex,
                     int windowWidth, int windowHeight) {
  if (player.knownSpells.empty())
    return; // Don't draw if no spells

  // SDL_Log("renderSpellMenu called."); // Reduce log spam

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
    std::string spellText =
        spell.name + " (" + std::to_string(spell.manaCost) + " Mana)";

    SDL_Color textColor;
    if (i == currentSelectionIndex) {
      textColor = selectedColor;
    } else {
      textColor = normalColor;
    }
    if (player.mana < spell.manaCost) {
      if (i == currentSelectionIndex) {
        textColor = cannotCastColor;
      } else {
        textColor = {100, 100, 100, 255};
      }
    }

    // **Use the original renderText from utils.h/cpp**
    SDL_Texture *textTexture =
        renderText(renderer, font, spellText, textColor);

    if (textTexture) {
      int textW, textH;
      SDL_QueryTexture(textTexture, nullptr, nullptr, &textW, &textH);
      SDL_Rect dstRect = {menuX + 15, menuY + textOffsetY + (i * itemHeight),
                          textW, textH};
      SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);
      SDL_DestroyTexture(textTexture);
    }
  }
}

// --- renderSpellBar using original utils::renderText ---
void renderSpellBar(GameData &gameData, AssetManager &assets) {
  const int MAX_BAR_SLOTS = 5; // For QWERT
  const std::vector<std::string> keyLabels = {"Q", "W", "E", "R", "T"};

  SDL_Renderer *renderer = gameData.renderer;
  TTF_Font* font = assets.getFont("spellbar_font"); // Use the smaller font
  const auto &spells = gameData.currentGamePlayer.knownSpells;

  if (!font || spells.empty())
    return;

  int slotSize = 70;
  int padding = 10;
  int totalBarWidth =
      (slotSize * MAX_BAR_SLOTS) + (padding * (MAX_BAR_SLOTS - 1));
  int barHeight = slotSize + 2 * padding;
  int barX = (gameData.windowWidth - totalBarWidth) / 2;
  int barY = gameData.windowHeight - barHeight - 10;

  SDL_Color barBgColor = {20, 20, 40, 180};
  SDL_Color slotBgColor = {40, 40, 60, 200};
  SDL_Color slotBorderColor = {100, 100, 120, 255};
  SDL_Color keyTextColor = {220, 220, 220, 255};
  SDL_Color cannotCastColor = {150, 50, 50, 255};

  SDL_Rect barRect = {barX, barY, totalBarWidth, barHeight};
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, barBgColor.r, barBgColor.g, barBgColor.b,
                         barBgColor.a);
  SDL_RenderFillRect(renderer, &barRect);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  int currentSlotX = barX + padding;
  for (int i = 0; i < MAX_BAR_SLOTS && i < spells.size(); ++i) {
    const Spell &spell = spells[i];
    bool canCast = gameData.currentGamePlayer.mana >= spell.manaCost;

    SDL_Rect slotRect = {currentSlotX, barY + padding, slotSize, slotSize};

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, slotBgColor.r, slotBgColor.g,
                           slotBgColor.b, slotBgColor.a);
    SDL_RenderFillRect(renderer, &slotRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, slotBorderColor.r, slotBorderColor.g,
                           slotBorderColor.b, slotBorderColor.a);
    SDL_RenderDrawRect(renderer, &slotRect);

    SDL_Texture* iconTex = assets.getTexture(spell.iconName);
    if (iconTex) {
        int iconPadding = 2;
        SDL_Rect iconDestRect = {
            slotRect.x + iconPadding,
            slotRect.y + iconPadding,
            slotRect.w - (2 * iconPadding),
            slotRect.h - (2 * iconPadding)
        };
        SDL_RenderCopy(gameData.renderer, iconTex, nullptr, &iconDestRect);
    } else {
        SDL_Rect iconPlaceholderRect = { slotRect.x + 5, slotRect.y + 5, slotRect.w - 10, slotRect.h - 10 };
        SDL_SetRenderDrawColor(gameData.renderer, 40, 40, 40, 255);
        SDL_RenderFillRect(gameData.renderer, &iconPlaceholderRect);
    }

    // **Use the original renderText from utils.h/cpp**
    SDL_Texture* keyTexture = nullptr;
    int keyW = 0, keyH = 0;
    SDL_Color currentKeyColor = keyTextColor;
    keyTexture = renderText(renderer, font, keyLabels[i], currentKeyColor);
     if (keyTexture) {
         SDL_QueryTexture(keyTexture, nullptr, nullptr, &keyW, &keyH);
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


    if (!canCast) {
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(renderer, cannotCastColor.r, cannotCastColor.g,
                             cannotCastColor.b,
                             100); // Semi-transparent red overlay
      SDL_RenderFillRect(renderer, &slotRect);
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    currentSlotX += slotSize + padding;
  }
}


// --- NEW: renderCharacterSheet Implementation ---
void renderCharacterSheet(GameData& gameData, AssetManager& assets) {
    SDL_Renderer* renderer = gameData.renderer;
    TTF_Font* font = assets.getFont("main_font"); // Or a specific font for the sheet
    TTF_Font* smallFont = assets.getFont("spellbar_font"); // For smaller text
    PlayerCharacter& player = gameData.currentGamePlayer;

    if (!font || !smallFont) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Character Sheet Error: Fonts not loaded!");
        return;
    }

    // --- Sheet Appearance ---
    int sheetWidth = 600; // Adjust as needed
    int sheetHeight = 700; // Adjust as needed
    int sheetX = (gameData.windowWidth - sheetWidth) / 2;
    int sheetY = (gameData.windowHeight - sheetHeight) / 2;
    SDL_Color bgColor = {10, 20, 40, 220}; // Dark blue, semi-transparent
    SDL_Color textColor = {220, 220, 220, 255}; // Light gray
    SDL_Color statColor = {200, 200, 255, 255}; // Light blue for stats
    SDL_Color spellColor = {200, 255, 200, 255}; // Light green for spells

    // --- Draw Background ---
    SDL_Rect bgRect = {sheetX, sheetY, sheetWidth, sheetHeight};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderFillRect(renderer, &bgRect);
    SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255); // Border color
    SDL_RenderDrawRect(renderer, &bgRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    int currentY = sheetY + 20; // Starting Y position inside the sheet with padding
    int leftColX = sheetX + 20;
    int rightColX = sheetX + sheetWidth / 2 + 10; // Start of right column
    int sectionPadding = 15;
    int lineSpacing = 5; // Extra space between lines within a section

    // --- Column 1: Portrait and Basic Info ---
    SDL_Texture* portraitTexture = nullptr;
    std::string portraitKey;
     if (player.type == CharacterType::FemaleMage) {
         portraitKey = "female_mage_portrait";
     } else {
         portraitKey = "male_mage_portrait";
     }
    portraitTexture = assets.getTexture(portraitKey);

    int portraitSize = 128;
    if (portraitTexture) {
        SDL_Rect portraitRect = {leftColX, currentY, portraitSize, portraitSize};
        SDL_RenderCopy(renderer, portraitTexture, nullptr, &portraitRect);
        currentY += portraitSize + sectionPadding;
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Character portrait '%s' not found.", portraitKey.c_str());
         SDL_Rect placeholderRect = {leftColX, currentY, portraitSize, portraitSize};
         SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
         SDL_RenderFillRect(renderer, &placeholderRect);
         SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
         SDL_RenderDrawRect(renderer, &placeholderRect);
         currentY += portraitSize + sectionPadding;
    }

    // Level
    std::string levelText = "Level: " + std::to_string(player.level);
    // **FIX: Use the renamed helper function**
    renderTextAtPos(renderer, font, levelText, textColor, leftColX, currentY);
    currentY += TTF_FontHeight(font) + lineSpacing;

    // Arcana (XP)
    std::string arcanaText = "Arcana: " + std::to_string(player.currentArcana) + " / " + std::to_string(player.level * ARCANA_PER_LEVEL);
    // **FIX: Use the renamed helper function**
    renderTextAtPos(renderer, smallFont, arcanaText, textColor, leftColX, currentY);
    currentY += TTF_FontHeight(smallFont) + sectionPadding;

    // Health
    std::string healthText = "Health: " + std::to_string(player.health) + " / " + std::to_string(player.maxHealth);
    // **FIX: Use the renamed helper function**
    renderTextAtPos(renderer, font, healthText, {255, 100, 100, 255}, leftColX, currentY); // Reddish for HP
    currentY += TTF_FontHeight(font) + lineSpacing;

    // Mana
    std::string manaText = "Mana: " + std::to_string(player.mana) + " / " + std::to_string(player.maxMana);
    // **FIX: Use the renamed helper function**
    renderTextAtPos(renderer, font, manaText, {100, 100, 255, 255}, leftColX, currentY); // Bluish for MP
    currentY += TTF_FontHeight(font) + sectionPadding;

    // Stats Section Title
    // **FIX: Use the renamed helper function**
    renderTextAtPos(renderer, font, "Attributes", textColor, leftColX, currentY);
    currentY += TTF_FontHeight(font) + lineSpacing;

    // Core Stats
    std::string vitText = "Vitality: " + std::to_string(player.GetEffectiveVitality());
    // **FIX: Use the renamed helper function**
    renderTextAtPos(renderer, smallFont, vitText, statColor, leftColX, currentY); currentY += TTF_FontHeight(smallFont);

    std::string intText = "Intelligence: " + std::to_string(player.GetEffectiveIntelligence());
    // **FIX: Use the renamed helper function**
    renderTextAtPos(renderer, smallFont, intText, statColor, leftColX, currentY); currentY += TTF_FontHeight(smallFont);

    std::string spiText = "Spirit: " + std::to_string(player.GetEffectiveSpirit());
    // **FIX: Use the renamed helper function**
    renderTextAtPos(renderer, smallFont, spiText, statColor, leftColX, currentY); currentY += TTF_FontHeight(smallFont);

    std::string agiText = "Agility: " + std::to_string(player.GetEffectiveAgility());
    // **FIX: Use the renamed helper function**
    renderTextAtPos(renderer, smallFont, agiText, statColor, leftColX, currentY); currentY += TTF_FontHeight(smallFont) + sectionPadding;


    // --- Column 2: Spells ---
    currentY = sheetY + 20; // Reset Y for the right column

    // Spells Section Title
    // **FIX: Use the renamed helper function**
    renderTextAtPos(renderer, font, "Known Spells", textColor, rightColX, currentY);
    currentY += TTF_FontHeight(font) + lineSpacing;

    int iconSize = 32; // Size for spell icons
    int spellPadding = 5;

    for (const auto& spell : player.knownSpells) {
         if (currentY + iconSize > sheetY + sheetHeight - 20) break; // Prevent overflow

        // Render Icon
        SDL_Texture* iconTex = assets.getTexture(spell.iconName);
        if (iconTex) {
            SDL_Rect iconRect = {rightColX, currentY, iconSize, iconSize};
            SDL_RenderCopy(renderer, iconTex, nullptr, &iconRect);
        } else {
             SDL_Rect placeholderRect = {rightColX, currentY, iconSize, iconSize};
             SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
             SDL_RenderFillRect(renderer, &placeholderRect);
        }

        // Render Spell Name & Cost
        std::string spellInfo = spell.name + " (" + std::to_string(spell.manaCost) + " MP)";
        // **FIX: Use the renamed helper function**
        renderTextAtPos(renderer, smallFont, spellInfo, spellColor, rightColX + iconSize + spellPadding, currentY + (iconSize / 2 - TTF_FontHeight(smallFont) / 2) ); // Align text vertically with icon center

        currentY += iconSize + spellPadding; // Move down for next spell
    }

    // --- Instructions ---
    int instructionY = sheetY + sheetHeight - TTF_FontHeight(smallFont) - 10;
    // **FIX: Use the renamed helper function**
    renderTextAtPos(renderer, smallFont, "Press 'I' to close", textColor, sheetX + sheetWidth - 150, instructionY);
}

