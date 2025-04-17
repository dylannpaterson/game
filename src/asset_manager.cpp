// asset_manager.cpp
#include "asset_manager.h"
#include <iostream> // For error messages if needed (or use SDL_Log)

AssetManager::AssetManager(SDL_Renderer* renderer) : rendererRef(renderer) {
    if (!rendererRef) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AssetManager created with null renderer!");
        // Potentially throw an exception or handle error appropriately
    }
    SDL_Log("AssetManager created.");
}

AssetManager::~AssetManager() {
    SDL_Log("AssetManager destroying assets...");
    clearAssets();
    SDL_Log("AssetManager finished cleanup.");
}

// --- Loading ---

bool AssetManager::loadTexture(const std::string& name, const std::string& path) {
    if (!rendererRef) return false; // Cannot load without renderer

    SDL_Texture* texture = IMG_LoadTexture(rendererRef, path.c_str());
    if (!texture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture '%s' from path '%s': %s", name.c_str(), path.c_str(), IMG_GetError());
        return false;
    }

    // Optional: Check if name already exists and handle (e.g., log warning, don't overwrite)
    if (textures.count(name)) {
         SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Texture name '%s' already exists. Overwriting.", name.c_str());
         SDL_DestroyTexture(textures[name]); // Destroy old one before replacing
    }

    textures[name] = texture;
    SDL_Log("Loaded Texture '%s' from '%s'", name.c_str(), path.c_str());
    return true;
}

bool AssetManager::loadFont(const std::string& name, const std::string& path, int pointSize) {
    TTF_Font* font = TTF_OpenFont(path.c_str(), pointSize);
    if (!font) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load font '%s' from path '%s': %s", name.c_str(), path.c_str(), TTF_GetError());
        return false;
    }

    if (fonts.count(name)) {
         SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Font name '%s' already exists. Overwriting.", name.c_str());
         TTF_CloseFont(fonts[name]);
    }

    fonts[name] = font;
    SDL_Log("Loaded Font '%s' from '%s' (%dpt)", name.c_str(), path.c_str(), pointSize);
    return true;
}

// --- Accessors ---

SDL_Texture* AssetManager::getTexture(const std::string& name) {
    // Use find for efficiency and to avoid inserting if not found
    auto it = textures.find(name);
    if (it != textures.end()) {
        return it->second; // Return pointer to the found texture
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture '%s' not found in AssetManager!", name.c_str());
        return nullptr;
    }
}

TTF_Font* AssetManager::getFont(const std::string& name) {
    auto it = fonts.find(name);
    if (it != fonts.end()) {
        return it->second;
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Font '%s' not found in AssetManager!", name.c_str());
        return nullptr;
    }
}

// --- Cleanup ---

void AssetManager::clearAssets() {
    // Destroy textures
    for (auto const& [name, texture] : textures) {
        if (texture) {
            SDL_DestroyTexture(texture);
            // SDL_Log("Destroyed texture '%s'", name.c_str()); // Verbose logging if needed
        }
    }
    textures.clear();

    // Close fonts
    for (auto const& [name, font] : fonts) {
        if (font) {
            TTF_CloseFont(font);
             // SDL_Log("Closed font '%s'", name.c_str()); // Verbose logging if needed
        }
    }
    fonts.clear();

    // Add sound/music cleanup here later
}