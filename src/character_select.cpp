#include "character_select.h"
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

void displayCharacterSelect(SDL_Renderer* renderer, TTF_Font* font, int selectedIndex, int windowWidth, int windowHeight) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, SDL_ALPHA_OPAQUE); // Dark gray background
    SDL_RenderClear(renderer);

    SDL_Color textColor = {255, 255, 255}; // White text
    SDL_Color highlightColor = {255, 255, 0}; // Yellow highlight

    // Load the male mage avatar
    SDL_Texture* maleMageTexture = nullptr;
    SDL_Surface* maleSurface = IMG_Load("../assets/sprites/male_mage.png"); // Adjust path as needed
    if (maleSurface) {
        maleMageTexture = SDL_CreateTextureFromSurface(renderer, maleSurface);
        SDL_FreeSurface(maleSurface);
    }

    // Load the female mage avatar
    SDL_Texture* femaleMageTexture = nullptr;
    SDL_Surface* femaleSurface = IMG_Load("../assets/sprites/female_mage.png"); // Adjust path as needed
    if (femaleSurface) {
        femaleMageTexture = SDL_CreateTextureFromSurface(renderer, femaleSurface);
        SDL_FreeSurface(femaleSurface);
    }

    // Calculate scaled dimensions for the avatars
    int desiredHeight = windowHeight / 2; // Example: One-half of the window height
    int scaledWidth = static_cast<int>(static_cast<float>(desiredHeight) * 1024.0f / 1536.0f); // Maintain aspect ratio

    // Calculate the horizontal center of the screen
    int centerX = windowWidth / 2;
    int verticalPosition = windowHeight / 4; // Keep the vertical position the same for now

    // Determine the horizontal spacing between the centers of the avatars
    int horizontalSpacing = scaledWidth * 1.2; // Adjust this multiplier to control the gap

    // Position the female mage (index 0)
    if (femaleMageTexture != nullptr) {
        SDL_Rect femaleRect;
        femaleRect.w = scaledWidth;
        femaleRect.h = desiredHeight;
        femaleRect.x = centerX - horizontalSpacing / 2 - scaledWidth / 2; // Center the left sprite
        femaleRect.y = verticalPosition;
        SDL_RenderCopy(renderer, femaleMageTexture, nullptr, &femaleRect);

        // Highlight if selected
        if (selectedIndex == 0) {
            SDL_SetRenderDrawColor(renderer, highlightColor.r, highlightColor.g, highlightColor.b, SDL_ALPHA_OPAQUE);
            SDL_Rect highlightRect = {femaleRect.x - 10, femaleRect.y - 10, femaleRect.w + 20, femaleRect.h + 20};
            SDL_RenderDrawRect(renderer, &highlightRect);
        }
    }

    // Position the male mage avatar (index 1)
    if (maleMageTexture != nullptr) {
        SDL_Rect maleRect;
        maleRect.w = scaledWidth;
        maleRect.h = desiredHeight;
        maleRect.x = centerX + horizontalSpacing / 2 - scaledWidth / 2; // Center the right sprite
        maleRect.y = verticalPosition;
        SDL_RenderCopy(renderer, maleMageTexture, nullptr, &maleRect);

        // Highlight if selected
        if (selectedIndex == 1) {
            SDL_SetRenderDrawColor(renderer, highlightColor.r, highlightColor.g, highlightColor.b, SDL_ALPHA_OPAQUE);
            SDL_Rect highlightRect = {maleRect.x - 10, maleRect.y - 10, maleRect.w + 20, maleRect.h + 20};
            SDL_RenderDrawRect(renderer, &highlightRect);
        }
    }

    // Display a message
    std::string message = "Choose Your Hero (Use Left/Right Arrows, Press Enter to Select)";
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, message.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect;

    // Calculate the horizontal center of the window
    int centerXText = windowWidth / 2;

    // Calculate the x-coordinate to center the text
    textRect.x = centerXText - textSurface->w / 2;
    textRect.y = windowHeight - 100; // Position text near the bottom
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