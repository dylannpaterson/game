// asset_manager.h
#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <string>
#include <map> // To store assets by name
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
// #include <SDL_mixer.h> // For future sound/music

class AssetManager {
public:
    // Constructor: Needs the renderer to create textures
    AssetManager(SDL_Renderer* renderer);
    // Destructor: Cleans up all loaded assets
    ~AssetManager();

    // --- Loading Functions ---
    // Loads a texture, stores it under 'name'. Returns true on success.
    bool loadTexture(const std::string& name, const std::string& path);
    // Loads a font, stores it under 'name'. Returns true on success.
    bool loadFont(const std::string& name, const std::string& path, int pointSize);
    // Add loadSound, loadMusic later...

    // --- Accessor Functions ---
    // Gets a previously loaded texture. Returns nullptr if not found.
    SDL_Texture* getTexture(const std::string& name);
    // Gets a previously loaded font. Returns nullptr if not found.
    TTF_Font* getFont(const std::string& name);
    // Add getSound, getMusic later...

private:
    // Prevent copying (optional but good practice)
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // Pointer to the main renderer (doesn't own it)
    SDL_Renderer* rendererRef;

    // Storage for assets
    std::map<std::string, SDL_Texture*> textures;
    std::map<std::string, TTF_Font*> fonts;
    // std::map<std::string, Mix_Chunk*> sounds;
    // std::map<std::string, Mix_Music*> music;

    // Helper for cleanup
    void clearAssets();
};

#endif // ASSET_MANAGER_H