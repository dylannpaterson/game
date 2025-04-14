#include "character_select.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <string>

void displayCharacterSelect(SDL_Renderer* renderer, TTF_Font* font, int selectedIndex, int windowWidth, int windowHeight, Uint8 alpha) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, alpha); // Apply alpha to background
    SDL_RenderClear(renderer);

    SDL_Color textColor = {255, 255, 255};
    SDL_Color highlightColor = {255, 255, 0};

    // Apply alpha to text color
    SDL_Color textColorWithAlpha = {textColor.r, textColor.g, textColor.b, alpha};
    SDL_Color highlightColorWithAlpha = {highlightColor.r, highlightColor.g, highlightColor.b, alpha};

    // Load the male mage avatar
    SDL_Texture* maleMageTexture = nullptr;
    SDL_Surface* maleSurface = IMG_Load("../assets/sprites/male_mage.png"); // Adjust path as needed
    if (maleSurface) {
        maleMageTexture = SDL_CreateTextureFromSurface(renderer, maleSurface);
        SDL_FreeSurface(maleSurface);
        if (maleMageTexture) {
            SDL_SetTextureAlphaMod(maleMageTexture, alpha); // Apply alpha to texture
            SDL_SetTextureBlendMode(maleMageTexture, SDL_BLENDMODE_BLEND); // Set blend mode
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateTextureFromSurface failed for male_mage.png! SDL_Error: %s\n", SDL_GetError());
        }
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "IMG_Load failed for male_mage.png! IMG_Error: %s\n", IMG_GetError());
    }

    // Load the female mage avatar
    SDL_Texture* femaleMageTexture = nullptr;
    SDL_Surface* femaleSurface = IMG_Load("../assets/sprites/female_mage.png"); // Adjust path as needed
    if (femaleSurface) {
        femaleMageTexture = SDL_CreateTextureFromSurface(renderer, femaleSurface);
        SDL_FreeSurface(femaleSurface);
        if (femaleMageTexture) {
            SDL_SetTextureAlphaMod(femaleMageTexture, alpha); // Apply alpha to texture
            SDL_SetTextureBlendMode(femaleMageTexture, SDL_BLENDMODE_BLEND); // Set blend mode
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateTextureFromSurface failed for female_mage.png! SDL_Error: %s\n", SDL_GetError());
        }
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "IMG_Load failed for female_mage.png! IMG_Error: %s\n", IMG_GetError());
    }

    // Calculate scaled dimensions for the avatars
    int desiredHeight = windowHeight / 2;
    int scaledWidth = static_cast<int>(static_cast<float>(desiredHeight) * 1024.0f / 1536.0f);

    int centerX = windowWidth / 2;
    int verticalPosition = windowHeight / 4;

    int horizontalSpacing = scaledWidth * 1.2;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); // Set blend mode for the renderer

    // Position the female mage (index 0)
    if (femaleMageTexture != nullptr) {
        SDL_Rect femaleRect;
        femaleRect.w = scaledWidth;
        femaleRect.h = desiredHeight;
        femaleRect.x = centerX - horizontalSpacing / 2 - scaledWidth / 2;
        femaleRect.y = verticalPosition;
        SDL_RenderCopy(renderer, femaleMageTexture, nullptr, &femaleRect);

        // Highlight if selected
        if (selectedIndex == 0) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, alpha); // Set alpha directly
            SDL_Rect highlightRect = {femaleRect.x - 10, femaleRect.y - 10, femaleRect.w + 20, femaleRect.h + 20};
            SDL_RenderDrawRect(renderer, &highlightRect);
        }
    }

    // Position the male mage avatar (index 1)
    if (maleMageTexture != nullptr) {
        SDL_Rect maleRect;
        maleRect.w = scaledWidth;
        maleRect.h = desiredHeight;
        maleRect.x = centerX + horizontalSpacing / 2 - scaledWidth / 2;
        maleRect.y = verticalPosition;
        SDL_RenderCopy(renderer, maleMageTexture, nullptr, &maleRect);

        // Highlight if selected
        if (selectedIndex == 1) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, alpha); // Set alpha directly
            SDL_Rect highlightRect = {maleRect.x - 10, maleRect.y - 10, maleRect.w + 20, maleRect.h + 20};
            SDL_RenderDrawRect(renderer, &highlightRect);
        }
    }

    // Display a message
    std::string message = "Choose Your Hero";
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, message.c_str(), textColorWithAlpha); // Apply alpha to text
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_SetTextureAlphaMod(textTexture, alpha); // Apply alpha to texture
    SDL_SetTextureBlendMode(textTexture, SDL_BLENDMODE_BLEND); // Ensure text blending
    SDL_Rect textRect;

    int centerXText = windowWidth / 2;

    textRect.x = centerXText - textSurface->w / 2;
    textRect.y = windowHeight - 100;
    textRect.w = textSurface->w;
    textRect.h = textSurface->h;
    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    SDL_RenderPresent(renderer);

    // Clean up textures (we'll do this properly later)
    // SDL_DestroyTexture(maleMageTexture);
    // SDL_DestroyTexture(femaleMageTexture);
}