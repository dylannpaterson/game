#include "ui.h"
#include "asset_manager.h"
#include "game_data.h" // Include game_data for PlayerCharacter definition within renderCharacterSheet
#include "utils.h"     // Include utils for the original renderText declaration

// **FIX: Add necessary includes BEFORE the helper function**
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <algorithm> // For std::min
#include <cmath>

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
    SDL_Renderer *renderer = gameData.renderer;
    PlayerCharacter &player = gameData.currentGamePlayer;
    TTF_Font* font = assets.getFont("main_font"); // Font for labels/numbers if kept
    TTF_Font* numberFont = assets.getFont("spellbar_font");

    if (!renderer) return; // Safety check

    // --- Bar Appearance ---
       // --- Bar Appearance ---
    // Define base dimensions for Mana bar
    int manaBarWidth = 400;
    int manaBarHeight = 30;

    // Calculate relative dimensions
    int healthBarWidth = static_cast<int>(std::round(manaBarWidth * 1.2));
    int healthBarHeight = static_cast<int>(std::round(manaBarHeight * 1.2));
    int arcanaBarWidth = static_cast<int>(std::round(manaBarWidth * 0.8));
    int arcanaBarHeight = static_cast<int>(std::round(manaBarHeight * 0.8));

    int barPadding = 10; // Vertical space between bars
    int barX = 15;       // X position for the bars
    int currentY = 15;   // Starting Y position

    // Colors (remain the same)
    SDL_Color healthBgColor = {50, 0, 0, 255};
    SDL_Color healthFgColor = {200, 0, 0, 255};
    SDL_Color manaBgColor = {0, 0, 50, 255};
    SDL_Color manaFgColor = {0, 100, 200, 255};
    SDL_Color arcanaBgColor = {50, 0, 50, 255};
    SDL_Color arcanaFgColor = {200, 0, 200, 255};
    SDL_Color textColor = {220, 220, 220, 255};

    // --- Health Bar (Largest) ---
    // Label Removed:
    // if (font) { renderTextAtPos(renderer, font, "Health", textColor, barX, currentY); currentY += TTF_FontHeight(font) + 2; }

    SDL_Rect healthBgRect = {barX, currentY, healthBarWidth, healthBarHeight};
    SDL_SetRenderDrawColor(renderer, healthBgColor.r, healthBgColor.g, healthBgColor.b, healthBgColor.a);
    SDL_RenderFillRect(renderer, &healthBgRect);

    float healthRatio = (player.maxHealth > 0) ? static_cast<float>(player.health) / player.maxHealth : 0.0f;
    healthRatio = std::max(0.0f, std::min(healthRatio, 1.0f));

    SDL_Rect healthFgRect = {barX, currentY, static_cast<int>(healthBarWidth * healthRatio), healthBarHeight};
    SDL_SetRenderDrawColor(renderer, healthFgColor.r, healthFgColor.g, healthFgColor.b, healthFgColor.a);
    SDL_RenderFillRect(renderer, &healthFgRect);

    // Render Numerical Text
    std::string healthText = std::to_string(player.health) + " / " + std::to_string(player.maxHealth);
    int textW, textH;
    TTF_SizeText(numberFont, healthText.c_str(), &textW, &textH);
    renderTextAtPos(renderer, numberFont, healthText, textColor,
                    barX + (healthBarWidth - textW) / 2, // Center horizontally
                    currentY + (healthBarHeight - textH) / 2); // Center vertically

    currentY += healthBarHeight + barPadding; // Move down for the next bar

    // --- Mana Bar (Base Size) ---
    // Label Removed:
    // if (font) { renderTextAtPos(renderer, font, "Mana", textColor, barX, currentY); currentY += TTF_FontHeight(font) + 2; }

    SDL_Rect manaBgRect = {barX, currentY, manaBarWidth, manaBarHeight};
    SDL_SetRenderDrawColor(renderer, manaBgColor.r, manaBgColor.g, manaBgColor.b, manaBgColor.a);
    SDL_RenderFillRect(renderer, &manaBgRect);

    float manaRatio = (player.maxMana > 0) ? static_cast<float>(player.mana) / player.maxMana : 0.0f;
    manaRatio = std::max(0.0f, std::min(manaRatio, 1.0f));

    SDL_Rect manaFgRect = {barX, currentY, static_cast<int>(manaBarWidth * manaRatio), manaBarHeight};
    SDL_SetRenderDrawColor(renderer, manaFgColor.r, manaFgColor.g, manaFgColor.b, manaFgColor.a);
    SDL_RenderFillRect(renderer, &manaFgRect);

    // Render Numerical Text
    std::string manaText = std::to_string(player.mana) + " / " + std::to_string(player.maxMana);
    TTF_SizeText(numberFont, manaText.c_str(), &textW, &textH);
    renderTextAtPos(renderer, numberFont, manaText, textColor,
                    barX + (manaBarWidth - textW) / 2,
                    currentY + (manaBarHeight - textH) / 2);

    currentY += manaBarHeight + barPadding;

    // --- Arcana Bar (Smallest) ---
    // Label Removed:
    // if (font) { renderTextAtPos(renderer, font, "Arcana", textColor, barX, currentY); currentY += TTF_FontHeight(font) + 2; }

    SDL_Rect arcanaBgRect = {barX, currentY, arcanaBarWidth, arcanaBarHeight};
    SDL_SetRenderDrawColor(renderer, arcanaBgColor.r, arcanaBgColor.g, arcanaBgColor.b, arcanaBgColor.a);
    SDL_RenderFillRect(renderer, &arcanaBgRect);

    // --- MODIFIED CALCULATION ---
    float arcanaRatio = 0.0f;
    // Calculate the base arcana for the current 100-block (e.g., 121 -> 100, 250 -> 200)
    int baseArcanaForCurrentBlock = (player.currentArcana / 100) * 100;
    // Determine the next multiple of 100 (e.g., 121 -> 200, 250 -> 300)
    int nextMultipleOf100 = baseArcanaForCurrentBlock + 100;
    // Calculate the arcana gained *within* the current 100-block (e.g., 121 -> 21, 250 -> 50)
    int arcanaProgressInBlock = player.currentArcana - baseArcanaForCurrentBlock;
    // The amount needed to reach the next multiple is always 100 (unless max level?)
    int arcanaNeededForNextBlock = 100;

    if (arcanaNeededForNextBlock > 0) {
         // The ratio is the progress within the current block divided by the size of the block (100)
         arcanaRatio = static_cast<float>(arcanaProgressInBlock) / arcanaNeededForNextBlock;
    }
    arcanaRatio = std::max(0.0f, std::min(arcanaRatio, 1.0f));
    // --- END MODIFIED CALCULATION ---


    SDL_Rect arcanaFgRect = {barX, currentY, static_cast<int>(arcanaBarWidth * arcanaRatio), arcanaBarHeight};
    SDL_SetRenderDrawColor(renderer, arcanaFgColor.r, arcanaFgColor.g, arcanaFgColor.b, arcanaFgColor.a);
    SDL_RenderFillRect(renderer, &arcanaFgRect);

    // --- MODIFIED TEXT DISPLAY ---
    // Render Numerical Text (Current Arcana / Next Multiple of 100)
    std::string arcanaText = std::to_string(player.currentArcana) + " / " + std::to_string(nextMultipleOf100);
    // --- END MODIFIED TEXT DISPLAY ---

    TTF_SizeText(numberFont, arcanaText.c_str(), &textW, &textH);
    renderTextAtPos(renderer, numberFont, arcanaText, textColor,
                    barX + (arcanaBarWidth - textW) / 2,
                    currentY + (arcanaBarHeight - textH) / 2);

    // currentY += arcanaBarHeight + barPadding; // Update if more elements follow below


  // --- Level Display ---
  std::string levelText =
      "Level: " + std::to_string(gameData.currentGamePlayer.level);
   // **Use the original renderText from utils.h/cpp**
  SDL_Texture *levelTexture = renderText(
      gameData.renderer, font, levelText, textColor);
  if (levelTexture != nullptr) {
      SDL_Rect levelRect = {gameData.windowWidth - 150, 10, 0, 0};
      SDL_QueryTexture(levelTexture, nullptr, nullptr, &levelRect.w, &levelRect.h);
      levelRect.x = gameData.logicalWidth - levelRect.w - 10; // Right-align
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
       floorRect.x = gameData.logicalWidth - floorRect.w - 10; // Right-align

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
    int effectiveCost = player.GetEffectiveManaCost(i);
    std::string spellText =
        spell.name + " (" + std::to_string(effectiveCost) + " Mana)";

    SDL_Color textColor;
    if (i == currentSelectionIndex) {
      textColor = selectedColor;
    } else {
      textColor = normalColor;
    }
    if (player.mana < effectiveCost) {
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
  const std::vector<std::string> keyLabels = {"1", "2", "3", "4", "5"};

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
  int barX = (gameData.logicalWidth - totalBarWidth) / 2; // Center based on LOGICAL width
  int barY = gameData.logicalHeight - barHeight - 10;   // Align to bottom of LOGICAL height

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
    int effectiveCost = gameData.currentGamePlayer.GetEffectiveManaCost(i);
    bool canCast = gameData.currentGamePlayer.mana >= effectiveCost;

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
    int sheetX = (gameData.logicalWidth - sheetWidth) / 2;  // Center based on LOGICAL width
    int sheetY = (gameData.logicalHeight - sheetHeight) / 2; // Center based on LOGICAL height
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
        int effectiveCost = player.GetEffectiveManaCost(&spell - &player.knownSpells[0]); // Get index of spell
        std::string spellInfo = spell.name + " (" + std::to_string(effectiveCost) + " MP)";
        // **FIX: Use the renamed helper function**
        renderTextAtPos(renderer, smallFont, spellInfo, spellColor, rightColX + iconSize + spellPadding, currentY + (iconSize / 2 - TTF_FontHeight(smallFont) / 2) ); // Align text vertically with icon center

        currentY += iconSize + spellPadding; // Move down for next spell
    }

    // --- Instructions ---
    int instructionY = sheetY + sheetHeight - TTF_FontHeight(smallFont) - 10; // Y position is fine relative to sheetY + sheetHeight
    int instructionX = sheetX + sheetWidth - 150; // Relative to sheet right edge is probably intended and fine

    // **FIX: Use the renamed helper function**
    renderTextAtPos(renderer, smallFont, "Press 'I' to close", textColor, instructionX, instructionY);
}

