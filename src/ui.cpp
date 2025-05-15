#include "ui.h"
#include "asset_manager.h"
#include "game_data.h" // Include game_data for PlayerCharacter definition within renderCharacterSheet
#include "utils.h" // Include utils for the original renderText declaration

// **FIX: Add necessary includes BEFORE the helper function**
#include <SDL.h>
#include <SDL_ttf.h>
#include <algorithm> // For std::min
#include <cmath>
#include <string>

void renderTextAtPos(SDL_Renderer *renderer, TTF_Font *font,
                     const std::string &text, SDL_Color color, int x, int y,
                     bool centered = false) {
  if (!font || !renderer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "renderTextAtPos Error: Null font or renderer.");
    return;
  }
  SDL_Surface *surface = TTF_RenderText_Blended_Wrapped(
      font, text.c_str(), color,
      0); // Using Blended_Wrapped for potential multi-line
  if (!surface) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "TTF_RenderText_Blended_Wrapped Error: %s", TTF_GetError());
    return;
  }
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (!texture) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
    SDL_FreeSurface(surface);
    return;
  }

  SDL_Rect destRect = {x, y, surface->w, surface->h};
  if (centered) {
    destRect.x = x - surface->w / 2;
    // destRect.y = y - surface->h / 2; // Optional: center y too
  }
  SDL_RenderCopy(renderer, texture, nullptr, &destRect);

  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}

// --- Helper function to convert RuneType to String ---
std::string RuneTypeToString(RuneType runeType) {
  switch (runeType) {
  case RuneType::Fire:
    return "Fire";
  case RuneType::Ice:
    return "Ice";
  case RuneType::Lightning:
    return "Lightning";
  case RuneType::Wind:
    return "Wind";
  case RuneType::Arcane:
    return "Arcane";
  case RuneType::Shadow:
    return "Shadow";
  case RuneType::Holy:
    return "Holy";
  case RuneType::NUM_RUNE_TYPES:
    return "None"; // Or "Universal"
  default:
    return "Unknown";
  }
}

// --- Implementation for the new Spell Unlock Menu ---
// --- Implementation for the new Spell Unlock Menu ---
void renderSpellUnlockMenu(GameData &gameData, AssetManager &assets) {
  SDL_Renderer *renderer = gameData.renderer;
  PlayerCharacter &player = gameData.currentGamePlayer;
  const std::vector<Spell> &allSpells = gameData.allSpellDefinitions;
  int &selectedSpellListIndex = gameData.spellSelectIndex;

  TTF_Font *titleFont = assets.getFont("main_font");
  TTF_Font *textFont = assets.getFont("main_font");
  TTF_Font *smallFont = assets.getFont("spellbar_font");

  if (!renderer || !titleFont || !textFont || !smallFont) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Spell Unlock Menu Error: Missing renderer or fonts.");
    return;
  }

  SDL_Color bgColor = {20, 30, 50, 230};
  SDL_Color panelBgColor = {30, 45, 70, 230};
  SDL_Color borderColor = {70, 90, 130, 255};
  SDL_Color textColor = {220, 220, 240, 255};
  SDL_Color selectedColor = {255, 255, 100, 255};
  SDL_Color lockedColor = {150, 150, 170, 255};
  SDL_Color unlockedColor = {170, 255, 170, 255};
  SDL_Color costColor = {200, 180, 150, 255};
  SDL_Color affordableColor = {150, 255, 150, 255};
  SDL_Color unaffordableColor = {255, 150, 150, 255};
  SDL_Color buttonColor = {60, 80, 110, 255};
  SDL_Color buttonTextColor = {230, 230, 250, 255};
  SDL_Color damageTextColor = {255, 200, 100,
                               255}; // Orange-ish for damage text

  int screenW = gameData.logicalWidth;
  int screenH = gameData.logicalHeight;

  int menuPadding = 30;
  SDL_Rect menuRect = {menuPadding, menuPadding, screenW - 2 * menuPadding,
                       screenH - 2 * menuPadding};
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
  SDL_RenderFillRect(renderer, &menuRect);
  SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b,
                         borderColor.a);
  SDL_RenderDrawRect(renderer, &menuRect);

  renderTextAtPos(renderer, titleFont, "Arcane Library", textColor,
                  menuRect.x + menuRect.w / 2, menuRect.y + 20, true);

  int panelSpacing = 20;
  int topOffset = 80;
  int bottomOffset = 20;
  int leftPanelWidth = (menuRect.w - panelSpacing) / 3;
  SDL_Rect leftPanelRect = {menuRect.x + panelSpacing, menuRect.y + topOffset,
                            leftPanelWidth,
                            menuRect.h - topOffset - bottomOffset};
  int rightPanelX = leftPanelRect.x + leftPanelRect.w + panelSpacing;
  SDL_Rect rightPanelRect = {rightPanelX, menuRect.y + topOffset,
                             menuRect.w - leftPanelWidth - 3 * panelSpacing,
                             menuRect.h - topOffset - bottomOffset};

  SDL_SetRenderDrawColor(renderer, panelBgColor.r, panelBgColor.g,
                         panelBgColor.b, panelBgColor.a);
  SDL_RenderFillRect(renderer, &leftPanelRect);
  SDL_RenderFillRect(renderer, &rightPanelRect);
  SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b,
                         borderColor.a);
  SDL_RenderDrawRect(renderer, &leftPanelRect);
  SDL_RenderDrawRect(renderer, &rightPanelRect);

  int listItemHeight = 40;
  int iconSize = 32;
  int listPadding = 10;
  int textXOffset = listPadding + iconSize + 10;
  int visibleItems = (leftPanelRect.h - 2 * listPadding) / listItemHeight;
  int scrollOffset = 0;

  if (selectedSpellListIndex >= visibleItems) {
    scrollOffset = selectedSpellListIndex - visibleItems + 1;
  }
  if (allSpells.empty()) {
    renderTextAtPos(renderer, textFont, "No spells defined.", lockedColor,
                    leftPanelRect.x + listPadding,
                    leftPanelRect.y + listPadding);
  } else {
    for (int i = 0; i < static_cast<int>(allSpells.size()); ++i) {
      if (i < scrollOffset || i >= scrollOffset + visibleItems) {
        continue;
      }
      const Spell &spell = allSpells[i];
      int itemY =
          leftPanelRect.y + listPadding + (i - scrollOffset) * listItemHeight;
      if (i == selectedSpellListIndex) {
        SDL_Rect selectionRect = {leftPanelRect.x, itemY, leftPanelRect.w,
                                  listItemHeight};
        SDL_SetRenderDrawColor(renderer, selectedColor.r, selectedColor.g,
                               selectedColor.b, 50);
        SDL_RenderFillRect(renderer, &selectionRect);
      }
      SDL_Texture *iconTex = assets.getTexture(spell.iconName);
      if (iconTex) {
        SDL_Rect iconRect = {leftPanelRect.x + listPadding,
                             itemY + (listItemHeight - iconSize) / 2, iconSize,
                             iconSize};
        SDL_RenderCopy(renderer, iconTex, nullptr, &iconRect);
      }
      bool isKnown = player.hasSpellUnlocked(spell.name);
      SDL_Color nameColor = isKnown ? unlockedColor : lockedColor;
      if (i == selectedSpellListIndex)
        nameColor = selectedColor;
      renderTextAtPos(renderer, textFont, spell.name, nameColor,
                      leftPanelRect.x + textXOffset,
                      itemY + (listItemHeight - TTF_FontHeight(textFont)) / 2);
    }
  }

  if (selectedSpellListIndex >= 0 &&
      selectedSpellListIndex < static_cast<int>(allSpells.size())) {
    const Spell &selectedSpell = allSpells[selectedSpellListIndex];
    int currentDetailY = rightPanelRect.y + listPadding;
    int detailTextX = rightPanelRect.x + listPadding;

    SDL_Texture *largeIconTex = assets.getTexture(selectedSpell.iconName);
    if (largeIconTex) {
      int largeIconSize = 64;
      SDL_Rect largeIconRect = {detailTextX, currentDetailY, largeIconSize,
                                largeIconSize};
      SDL_RenderCopy(renderer, largeIconTex, nullptr, &largeIconRect);
      renderTextAtPos(renderer, titleFont, selectedSpell.name, textColor,
                      detailTextX + largeIconSize + 15,
                      currentDetailY +
                          (largeIconSize - TTF_FontHeight(titleFont)) / 2);
      currentDetailY += largeIconSize + 15;
    } else {
      renderTextAtPos(renderer, titleFont, selectedSpell.name, textColor,
                      detailTextX, currentDetailY);
      currentDetailY += TTF_FontHeight(titleFont) + 15;
    }

    bool isKnown = player.hasSpellUnlocked(selectedSpell.name);

    if (!isKnown) {
      renderTextAtPos(renderer, textFont, "Status: Locked", lockedColor,
                      detailTextX, currentDetailY);
      currentDetailY += TTF_FontHeight(textFont) + 10;
      renderTextAtPos(renderer, textFont, "Unlock Cost:", costColor,
                      detailTextX, currentDetailY);
      currentDetailY += TTF_FontHeight(textFont) + 5;
      std::string arcanaCostText =
          std::to_string(selectedSpell.arcanaCostToUnlock) + " Arcana";
      bool canAffordArcana =
          player.CanAffordArcana(selectedSpell.arcanaCostToUnlock);
      renderTextAtPos(renderer, textFont, arcanaCostText,
                      canAffordArcana ? affordableColor : unaffordableColor,
                      detailTextX + 20, currentDetailY);
      currentDetailY += TTF_FontHeight(textFont) + 5;
      std::string runeCostText =
          "1 x " + RuneTypeToString(selectedSpell.requiredRuneTypeToUnlock) +
          " Rune";
      bool canAffordRune =
          (selectedSpell.requiredRuneTypeToUnlock ==
           RuneType::NUM_RUNE_TYPES) ||
          (player.getRuneCount(selectedSpell.requiredRuneTypeToUnlock) >= 1);
      renderTextAtPos(renderer, textFont, runeCostText,
                      canAffordRune ? affordableColor : unaffordableColor,
                      detailTextX + 20, currentDetailY);
      currentDetailY += TTF_FontHeight(textFont) + 20;

      // --- Display Theoretical Damage Range for Locked Spells ---
      if (selectedSpell.numDamageDice > 0 &&
          (selectedSpell.effectType == SpellEffectType::Damage ||
           selectedSpell.effectType == SpellEffectType::AreaDamage ||
           selectedSpell.effectType == SpellEffectType::LinearDamage)) {
        std::pair<int, int> damageRange =
            player.getTheoreticalSpellDamageRange(selectedSpell);
        std::string damageText =
            "Potential Damage: " + std::to_string(damageRange.first) + " - " +
            std::to_string(damageRange.second);
        renderTextAtPos(renderer, textFont, damageText, damageTextColor,
                        detailTextX, currentDetailY);
        currentDetailY += TTF_FontHeight(textFont) + 10;
      }
      // --- End Damage Range Display ---

      SDL_Rect unlockButtonRect = {detailTextX, currentDetailY, 150, 40};
      bool canUnlock = canAffordArcana && canAffordRune;
      SDL_SetRenderDrawColor(
          renderer, buttonColor.r, buttonColor.g, buttonColor.b,
          canUnlock ? buttonColor.a : (Uint8)(buttonColor.a * 0.5f));
      SDL_RenderFillRect(renderer, &unlockButtonRect);
      SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g,
                             borderColor.b, 255);
      SDL_RenderDrawRect(renderer, &unlockButtonRect);
      renderTextAtPos(renderer, textFont, "Unlock",
                      canUnlock ? buttonTextColor : lockedColor,
                      unlockButtonRect.x + unlockButtonRect.w / 2,
                      unlockButtonRect.y +
                          (unlockButtonRect.h - TTF_FontHeight(textFont)) / 2,
                      true);
      currentDetailY += unlockButtonRect.h + 10;

    } else { // Spell is Known
      renderTextAtPos(renderer, textFont, "Status: Learned", unlockedColor,
                      detailTextX, currentDetailY);
      currentDetailY += TTF_FontHeight(textFont) + 10;

      renderTextAtPos(renderer, textFont,
                      "Mana Cost: " +
                          std::to_string(selectedSpell.baseManaCost),
                      textColor, detailTextX, currentDetailY);
      currentDetailY += TTF_FontHeight(textFont) + 5;
      renderTextAtPos(renderer, textFont,
                      "Range: " + std::to_string(selectedSpell.baseRange),
                      textColor, detailTextX, currentDetailY);
      currentDetailY += TTF_FontHeight(textFont) + 5;

      // --- Display Theoretical Damage Range for Known Spells ---
      if (selectedSpell.numDamageDice > 0 &&
          (selectedSpell.effectType == SpellEffectType::Damage ||
           selectedSpell.effectType == SpellEffectType::AreaDamage ||
           selectedSpell.effectType == SpellEffectType::LinearDamage)) {
        std::pair<int, int> damageRange =
            player.getTheoreticalSpellDamageRange(selectedSpell);
        std::string damageText =
            "Damage: " + std::to_string(damageRange.first) + " - " +
            std::to_string(damageRange.second);
        renderTextAtPos(renderer, textFont, damageText, damageTextColor,
                        detailTextX, currentDetailY);
        currentDetailY += TTF_FontHeight(textFont) + 5;
      }
      // --- End Damage Range Display ---

      if (selectedSpell.areaOfEffectRadius > 0) {
        renderTextAtPos(renderer, textFont,
                        "AoE Radius: " +
                            std::to_string(selectedSpell.areaOfEffectRadius),
                        textColor, detailTextX, currentDetailY);
        currentDetailY += TTF_FontHeight(textFont) + 5;
      }

      currentDetailY += 20;
      renderTextAtPos(renderer, textFont, "Upgrades coming soon...", textColor,
                      detailTextX, currentDetailY);
    }
  }
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// Definition for renderUI (Uses the original renderText from utils.h/cpp)
void renderUI(GameData &gameData, AssetManager &assets) {
  SDL_Renderer *renderer = gameData.renderer;
  PlayerCharacter &player = gameData.currentGamePlayer;
  TTF_Font *font =
      assets.getFont("main_font"); // Font for labels/numbers if kept
  TTF_Font *numberFont = assets.getFont("spellbar_font");

  if (!renderer)
    return; // Safety check

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
  // if (font) { renderTextAtPos(renderer, font, "Health", textColor, barX,
  // currentY); currentY += TTF_FontHeight(font) + 2; }

  SDL_Rect healthBgRect = {barX, currentY, healthBarWidth, healthBarHeight};
  SDL_SetRenderDrawColor(renderer, healthBgColor.r, healthBgColor.g,
                         healthBgColor.b, healthBgColor.a);
  SDL_RenderFillRect(renderer, &healthBgRect);

  float healthRatio = (player.maxHealth > 0)
                          ? static_cast<float>(player.health) / player.maxHealth
                          : 0.0f;
  healthRatio = std::max(0.0f, std::min(healthRatio, 1.0f));

  SDL_Rect healthFgRect = {barX, currentY,
                           static_cast<int>(healthBarWidth * healthRatio),
                           healthBarHeight};
  SDL_SetRenderDrawColor(renderer, healthFgColor.r, healthFgColor.g,
                         healthFgColor.b, healthFgColor.a);
  SDL_RenderFillRect(renderer, &healthFgRect);

  // Render Numerical Text
  std::string healthText =
      std::to_string(player.health) + " / " + std::to_string(player.maxHealth);
  int textW, textH;
  TTF_SizeText(numberFont, healthText.c_str(), &textW, &textH);
  renderTextAtPos(renderer, numberFont, healthText, textColor,
                  barX + (healthBarWidth - textW) / 2, // Center horizontally
                  currentY +
                      (healthBarHeight - textH) / 2); // Center vertically

  currentY += healthBarHeight + barPadding; // Move down for the next bar

  // --- Mana Bar (Base Size) ---
  // Label Removed:
  // if (font) { renderTextAtPos(renderer, font, "Mana", textColor, barX,
  // currentY); currentY += TTF_FontHeight(font) + 2; }

  SDL_Rect manaBgRect = {barX, currentY, manaBarWidth, manaBarHeight};
  SDL_SetRenderDrawColor(renderer, manaBgColor.r, manaBgColor.g, manaBgColor.b,
                         manaBgColor.a);
  SDL_RenderFillRect(renderer, &manaBgRect);

  float manaRatio = (player.maxMana > 0)
                        ? static_cast<float>(player.mana) / player.maxMana
                        : 0.0f;
  manaRatio = std::max(0.0f, std::min(manaRatio, 1.0f));

  SDL_Rect manaFgRect = {barX, currentY,
                         static_cast<int>(manaBarWidth * manaRatio),
                         manaBarHeight};
  SDL_SetRenderDrawColor(renderer, manaFgColor.r, manaFgColor.g, manaFgColor.b,
                         manaFgColor.a);
  SDL_RenderFillRect(renderer, &manaFgRect);

  // Render Numerical Text
  std::string manaText =
      std::to_string(player.mana) + " / " + std::to_string(player.maxMana);
  TTF_SizeText(numberFont, manaText.c_str(), &textW, &textH);
  renderTextAtPos(renderer, numberFont, manaText, textColor,
                  barX + (manaBarWidth - textW) / 2,
                  currentY + (manaBarHeight - textH) / 2);

  currentY += manaBarHeight + barPadding;

  // --- Arcana Bar (Smallest) ---
  // Label Removed:
  // if (font) { renderTextAtPos(renderer, font, "Arcana", textColor, barX,
  // currentY); currentY += TTF_FontHeight(font) + 2; }

  SDL_Rect arcanaBgRect = {barX, currentY, arcanaBarWidth, arcanaBarHeight};
  SDL_SetRenderDrawColor(renderer, arcanaBgColor.r, arcanaBgColor.g,
                         arcanaBgColor.b, arcanaBgColor.a);
  SDL_RenderFillRect(renderer, &arcanaBgRect);

  // --- MODIFIED CALCULATION ---
  float arcanaRatio = 0.0f;
  // Calculate the base arcana for the current 100-block (e.g., 121 -> 100, 250
  // -> 200)
  int baseArcanaForCurrentBlock = (player.currentArcana / 100) * 100;
  // Determine the next multiple of 100 (e.g., 121 -> 200, 250 -> 300)
  int nextMultipleOf100 = baseArcanaForCurrentBlock + 100;
  // Calculate the arcana gained *within* the current 100-block (e.g., 121 ->
  // 21, 250 -> 50)
  int arcanaProgressInBlock = player.currentArcana - baseArcanaForCurrentBlock;
  // The amount needed to reach the next multiple is always 100 (unless max
  // level?)
  int arcanaNeededForNextBlock = 100;

  if (arcanaNeededForNextBlock > 0) {
    // The ratio is the progress within the current block divided by the size of
    // the block (100)
    arcanaRatio =
        static_cast<float>(arcanaProgressInBlock) / arcanaNeededForNextBlock;
  }
  arcanaRatio = std::max(0.0f, std::min(arcanaRatio, 1.0f));
  // --- END MODIFIED CALCULATION ---

  SDL_Rect arcanaFgRect = {barX, currentY,
                           static_cast<int>(arcanaBarWidth * arcanaRatio),
                           arcanaBarHeight};
  SDL_SetRenderDrawColor(renderer, arcanaFgColor.r, arcanaFgColor.g,
                         arcanaFgColor.b, arcanaFgColor.a);
  SDL_RenderFillRect(renderer, &arcanaFgRect);

  // --- MODIFIED TEXT DISPLAY ---
  // Render Numerical Text (Current Arcana / Next Multiple of 100)
  std::string arcanaText = std::to_string(player.currentArcana) + " / " +
                           std::to_string(nextMultipleOf100);
  // --- END MODIFIED TEXT DISPLAY ---

  TTF_SizeText(numberFont, arcanaText.c_str(), &textW, &textH);
  renderTextAtPos(renderer, numberFont, arcanaText, textColor,
                  barX + (arcanaBarWidth - textW) / 2,
                  currentY + (arcanaBarHeight - textH) / 2);

  // currentY += arcanaBarHeight + barPadding; // Update if more elements follow
  // below

  // --- Level Display ---
  std::string levelText =
      "Level: " + std::to_string(gameData.currentGamePlayer.level);
  // **Use the original renderText from utils.h/cpp**
  SDL_Texture *levelTexture =
      renderText(gameData.renderer, font, levelText, textColor);
  if (levelTexture != nullptr) {
    SDL_Rect levelRect = {gameData.windowWidth - 150, 10, 0, 0};
    SDL_QueryTexture(levelTexture, nullptr, nullptr, &levelRect.w,
                     &levelRect.h);
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
  SDL_Texture *floorTexture =
      renderText(gameData.renderer, font, floorText, textColor);
  if (floorTexture != nullptr) {
    SDL_QueryTexture(floorTexture, nullptr, nullptr, &floorRect.w,
                     &floorRect.h);
    floorRect.x = gameData.logicalWidth - floorRect.w - 10; // Right-align

    // Need levelRect height to position below it
    SDL_Rect tempLevelRect = {0, 0, 0, 0};
    SDL_Texture *tempLevelTexture =
        renderText(gameData.renderer, font, levelText, textColor);
    if (tempLevelTexture) {
      SDL_QueryTexture(tempLevelTexture, nullptr, nullptr, &tempLevelRect.w,
                       &tempLevelRect.h);
      SDL_DestroyTexture(tempLevelTexture);
      floorRect.y =
          10 + tempLevelRect.h +
          5; // Position below level text (Y=10 + level_height + padding)
    } else {
      floorRect.y = 40; // Fallback position
    }

    SDL_RenderCopy(gameData.renderer, floorTexture, nullptr, &floorRect);
    SDL_DestroyTexture(floorTexture);
  } else {
    SDL_Log("Failed to render floor text.");
  }
}

// --- renderSpellBar using original utils::renderText ---
void renderSpellBar(GameData &gameData, AssetManager &assets) {
  SDL_Renderer *renderer = gameData.renderer;
  TTF_Font *font = assets.getFont("spellbar_font");
  const PlayerCharacter &player = gameData.currentGamePlayer;

  if (!font)
    return;

  int slotSize = 70;
  int padding = 10;
  int totalBarWidth =
      (slotSize * MAX_SPELL_BAR_SLOTS) + (padding * (MAX_SPELL_BAR_SLOTS - 1));
  int barHeight = slotSize + 2 * padding;
  int barX = (gameData.logicalWidth - totalBarWidth) / 2;
  int barY = gameData.logicalHeight - barHeight - 10;

  SDL_Color barBgColor = {20, 20, 40, 180};
  SDL_Color slotBgColor = {40, 40, 60, 200};
  SDL_Color slotBorderColor = {100, 100, 120, 255};
  SDL_Color keyTextColor = {220, 220, 220, 255};
  SDL_Color cannotCastColor = {150, 50, 50, 100};
  const std::vector<std::string> keyLabels = {
      "1", "2", "3", "4", "5"}; // Should match MAX_SPELL_BAR_SLOTS

  SDL_Rect barRect = {barX, barY, totalBarWidth, barHeight};
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, barBgColor.r, barBgColor.g, barBgColor.b,
                         barBgColor.a);
  SDL_RenderFillRect(renderer, &barRect);

  int currentSlotX = barX + padding;
  for (int i = 0; i < MAX_SPELL_BAR_SLOTS; ++i) {
    SDL_Rect slotRect = {currentSlotX, barY + padding, slotSize, slotSize};
    SDL_SetRenderDrawColor(renderer, slotBgColor.r, slotBgColor.g,
                           slotBgColor.b, slotBgColor.a);
    SDL_RenderFillRect(renderer, &slotRect);
    SDL_SetRenderDrawColor(renderer, slotBorderColor.r, slotBorderColor.g,
                           slotBorderColor.b, slotBorderColor.a);
    SDL_RenderDrawRect(renderer, &slotRect);

    const std::string &spellNameInSlot = player.spellBarSlots[i];
    if (!spellNameInSlot.empty()) {
      const Spell *spellPtr = player.getKnownSpellByName(spellNameInSlot);
      if (spellPtr) {
        SDL_Texture *iconTex = assets.getTexture(spellPtr->iconName);
        if (iconTex) {
          SDL_Rect iconDestRect = {slotRect.x + 2, slotRect.y + 2,
                                   slotRect.w - 4, slotRect.h - 4};
          SDL_RenderCopy(renderer, iconTex, nullptr, &iconDestRect);
        }
        // Check if player can cast this spell (by finding its index in
        // knownSpells)
        int knownSpellIdx = player.getKnownSpellIndexByName(spellNameInSlot);
        if (knownSpellIdx != -1 && !player.canCastSpell(knownSpellIdx)) {
          SDL_SetRenderDrawColor(renderer, cannotCastColor.r, cannotCastColor.g,
                                 cannotCastColor.b, cannotCastColor.a);
          SDL_RenderFillRect(renderer, &slotRect);
        }
      }
    }

    // Render key label
    if (i < static_cast<int>(keyLabels.size())) {
      int keyTextW, keyTextH;
      TTF_SizeText(font, keyLabels[i].c_str(), &keyTextW, &keyTextH);
      renderTextAtPos(renderer, font, keyLabels[i], keyTextColor,
                      slotRect.x + 3, slotRect.y + slotRect.h - keyTextH - 3);
    }

    currentSlotX += slotSize + padding;
  }
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
// --- NEW: renderCharacterSheet Implementation ---
void renderCharacterSheet(GameData &gameData, AssetManager &assets) {
  SDL_Renderer *renderer = gameData.renderer;
  TTF_Font *font =
      assets.getFont("main_font"); // Or a specific font for the sheet
  TTF_Font *smallFont = assets.getFont("spellbar_font"); // For smaller text
  PlayerCharacter &player = gameData.currentGamePlayer;

  if (!font || !smallFont) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Character Sheet Error: Fonts not loaded!");
    return;
  }

  // --- Sheet Appearance ---
  int sheetWidth = 600;  // Adjust as needed
  int sheetHeight = 700; // Adjust as needed
  int sheetX =
      (gameData.logicalWidth - sheetWidth) / 2; // Center based on LOGICAL width
  int sheetY = (gameData.logicalHeight - sheetHeight) /
               2;                              // Center based on LOGICAL height
  SDL_Color bgColor = {10, 20, 40, 220};       // Dark blue, semi-transparent
  SDL_Color textColor = {220, 220, 220, 255};  // Light gray
  SDL_Color statColor = {200, 200, 255, 255};  // Light blue for stats
  SDL_Color spellColor = {200, 255, 200, 255}; // Light green for spells

  // --- Draw Background ---
  SDL_Rect bgRect = {sheetX, sheetY, sheetWidth, sheetHeight};
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
  SDL_RenderFillRect(renderer, &bgRect);
  SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255); // Border color
  SDL_RenderDrawRect(renderer, &bgRect);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  int currentY =
      sheetY + 20; // Starting Y position inside the sheet with padding
  int leftColX = sheetX + 20;
  int rightColX = sheetX + sheetWidth / 2 + 10; // Start of right column
  int sectionPadding = 15;
  int lineSpacing = 5; // Extra space between lines within a section

  // --- Column 1: Portrait and Basic Info ---
  SDL_Texture *portraitTexture = nullptr;
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
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Character portrait '%s' not found.", portraitKey.c_str());
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
  std::string arcanaText = "Arcana: " + std::to_string(player.currentArcana) +
                           " / " +
                           std::to_string(player.level * ARCANA_PER_LEVEL);
  // **FIX: Use the renamed helper function**
  renderTextAtPos(renderer, smallFont, arcanaText, textColor, leftColX,
                  currentY);
  currentY += TTF_FontHeight(smallFont) + sectionPadding;

  // Health
  std::string healthText = "Health: " + std::to_string(player.health) + " / " +
                           std::to_string(player.maxHealth);
  // **FIX: Use the renamed helper function**
  renderTextAtPos(renderer, font, healthText, {255, 100, 100, 255}, leftColX,
                  currentY); // Reddish for HP
  currentY += TTF_FontHeight(font) + lineSpacing;

  // Mana
  std::string manaText = "Mana: " + std::to_string(player.mana) + " / " +
                         std::to_string(player.maxMana);
  // **FIX: Use the renamed helper function**
  renderTextAtPos(renderer, font, manaText, {100, 100, 255, 255}, leftColX,
                  currentY); // Bluish for MP
  currentY += TTF_FontHeight(font) + sectionPadding;

  // Stats Section Title
  // **FIX: Use the renamed helper function**
  renderTextAtPos(renderer, font, "Attributes", textColor, leftColX, currentY);
  currentY += TTF_FontHeight(font) + lineSpacing;

  // Core Stats
  std::string vitText =
      "Vitality: " + std::to_string(player.GetEffectiveVitality());
  // **FIX: Use the renamed helper function**
  renderTextAtPos(renderer, smallFont, vitText, statColor, leftColX, currentY);
  currentY += TTF_FontHeight(smallFont);

  std::string intText =
      "Intelligence: " + std::to_string(player.GetEffectiveIntelligence());
  // **FIX: Use the renamed helper function**
  renderTextAtPos(renderer, smallFont, intText, statColor, leftColX, currentY);
  currentY += TTF_FontHeight(smallFont);

  std::string spiText =
      "Spirit: " + std::to_string(player.GetEffectiveSpirit());
  // **FIX: Use the renamed helper function**
  renderTextAtPos(renderer, smallFont, spiText, statColor, leftColX, currentY);
  currentY += TTF_FontHeight(smallFont);

  std::string agiText =
      "Agility: " + std::to_string(player.GetEffectiveAgility());
  // **FIX: Use the renamed helper function**
  renderTextAtPos(renderer, smallFont, agiText, statColor, leftColX, currentY);
  currentY += TTF_FontHeight(smallFont) + sectionPadding;

  // --- Column 2: Spells ---
  currentY = sheetY + 20; // Reset Y for the right column

  // Spells Section Title
  // **FIX: Use the renamed helper function**
  renderTextAtPos(renderer, font, "Known Spells", textColor, rightColX,
                  currentY);
  currentY += TTF_FontHeight(font) + lineSpacing;

  int iconSize = 32; // Size for spell icons
  int spellPadding = 5;

  for (const auto &spell : player.knownSpells) {
    if (currentY + iconSize > sheetY + sheetHeight - 20)
      break; // Prevent overflow

    // Render Icon
    SDL_Texture *iconTex = assets.getTexture(spell.iconName);
    if (iconTex) {
      SDL_Rect iconRect = {rightColX, currentY, iconSize, iconSize};
      SDL_RenderCopy(renderer, iconTex, nullptr, &iconRect);
    } else {
      SDL_Rect placeholderRect = {rightColX, currentY, iconSize, iconSize};
      SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
      SDL_RenderFillRect(renderer, &placeholderRect);
    }

    // Render Spell Name & Cost
    int effectiveCost = player.GetEffectiveManaCost(
        &spell - &player.knownSpells[0]); // Get index of spell
    std::string spellInfo =
        spell.name + " (" + std::to_string(effectiveCost) + " MP)";
    // **FIX: Use the renamed helper function**
    renderTextAtPos(
        renderer, smallFont, spellInfo, spellColor,
        rightColX + iconSize + spellPadding,
        currentY +
            (iconSize / 2 - TTF_FontHeight(smallFont) /
                                2)); // Align text vertically with icon center

    currentY += iconSize + spellPadding; // Move down for next spell
  }

  // --- Instructions ---
  int instructionY = sheetY + sheetHeight - TTF_FontHeight(smallFont) -
                     10; // Y position is fine relative to sheetY + sheetHeight
  int instructionX =
      sheetX + sheetWidth -
      150; // Relative to sheet right edge is probably intended and fine

  // **FIX: Use the renamed helper function**
  renderTextAtPos(renderer, smallFont, "Press 'I' to close", textColor,
                  instructionX, instructionY);
}
