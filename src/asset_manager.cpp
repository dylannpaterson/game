// asset_manager.cpp
#include "asset_manager.h"
#include <iostream> // For error messages if needed (or use SDL_Log)

AssetManager::AssetManager(SDL_Renderer *renderer) : rendererRef(renderer) {
  if (!rendererRef) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "AssetManager created with null renderer!");
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

bool AssetManager::loadTexture(const std::string &name,
                               const std::string &path) {
  if (!rendererRef)
    return false; // Cannot load without renderer

  SDL_Texture *texture = IMG_LoadTexture(rendererRef, path.c_str());
  if (!texture) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to load texture '%s' from path '%s': %s", name.c_str(),
                 path.c_str(), IMG_GetError());
    return false;
  }

  // Optional: Check if name already exists and handle (e.g., log warning, don't
  // overwrite)
  if (textures.count(name)) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Texture name '%s' already exists. Overwriting.", name.c_str());
    SDL_DestroyTexture(textures[name]); // Destroy old one before replacing
  }

  textures[name] = texture;
  SDL_Log("Loaded Texture '%s' from '%s'", name.c_str(), path.c_str());
  return true;
}

bool AssetManager::loadFont(const std::string &name, const std::string &path,
                            int pointSize) {
  TTF_Font *font = TTF_OpenFont(path.c_str(), pointSize);
  if (!font) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to load font '%s' from path '%s': %s", name.c_str(),
                 path.c_str(), TTF_GetError());
    return false;
  }

  if (fonts.count(name)) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Font name '%s' already exists. Overwriting.", name.c_str());
    TTF_CloseFont(fonts[name]);
  }

  fonts[name] = font;
  SDL_Log("Loaded Font '%s' from '%s' (%dpt)", name.c_str(), path.c_str(),
          pointSize);
  return true;
}

// --- Accessors ---

SDL_Texture *AssetManager::getTexture(const std::string &name) {
  // Use find for efficiency and to avoid inserting if not found
  auto it = textures.find(name);
  if (it != textures.end()) {
    return it->second; // Return pointer to the found texture
  } else {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Texture '%s' not found in AssetManager!", name.c_str());
    return nullptr;
  }
}

TTF_Font *AssetManager::getFont(const std::string &name) {
  auto it = fonts.find(name);
  if (it != fonts.end()) {
    return it->second;
  } else {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Font '%s' not found in AssetManager!", name.c_str());
    return nullptr;
  }
}

// --- Cleanup ---

void AssetManager::clearAssets() {
  // Destroy textures
  for (auto const &[name, texture] : textures) {
    if (texture) {
      SDL_DestroyTexture(texture);
      // SDL_Log("Destroyed texture '%s'", name.c_str()); // Verbose logging if
      // needed
    }
  }
  textures.clear();

  // Close fonts
  for (auto const &[name, font] : fonts) {
    if (font) {
      TTF_CloseFont(font);
      // SDL_Log("Closed font '%s'", name.c_str()); // Verbose logging if needed
    }
  }
  fonts.clear();

  // Add sound/music cleanup here later
}

bool loadAnimationSequence(
    AssetManager &assetManager, const std::string &baseName,
    const std::string &basePath, int frameCount,
    int padding =
        4) // padding = number of digits for frame number (e.g., 4 for 0001)
{
  bool success = true;
  char frameNumberStr[16];  // Buffer large enough for frame number + extension
  char filePathBuffer[256]; // Buffer for the full file path

  for (int i = 0; i < frameCount; ++i) {
    // Construct the asset key (e.g., "female_mage_idle_1")
    std::string assetKey = baseName + "_" + std::to_string(i);

    // Construct the file path with zero-padding
    // Format specifier e.g., "%s%0*d.png"
    // Arguments: basePath.c_str(), padding, i
    snprintf(frameNumberStr, sizeof(frameNumberStr), "%0*d.png", padding, i);
    snprintf(filePathBuffer, sizeof(filePathBuffer), "%s%s_%s",
             basePath.c_str(), baseName.c_str(), frameNumberStr);
    std::string filePath = filePathBuffer; // Convert buffer to string

    // Load the texture
    if (!assetManager.loadTexture(assetKey, filePath)) {
      // Log only the first failure for a sequence to avoid spam
      if (success) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Failed to load sequence frame: %s (and possibly others in "
                    "sequence)",
                    filePath.c_str());
      }
      success = false;
      // Optionally break here if one failure means the sequence is unusable
      // break;
    }
  }
  return success;
}

// --- Main Asset Loading Function ---
bool loadAllAssets(AssetManager &assetManager) {
  SDL_Log("--- Begin Asset Loading ---");
  bool loadSuccess = true;

  // --- Single Textures (Non-Animated) ---
  loadSuccess &=
      assetManager.loadTexture("splash", "../assets/splash/splash.png");
  loadSuccess &= assetManager.loadTexture("start_tile",
                                          "../assets/sprites/start_tile.png");
  loadSuccess &=
      assetManager.loadTexture("exit_tile", "../assets/sprites/exit_tile.png");
  loadSuccess &= assetManager.loadTexture(
      "reticle", "../assets/sprites/target_reticle.png");
  loadSuccess &= assetManager.loadTexture(
      "fireball",
      "../assets/sprites/animations/spells/fireball/fireball_launched.png");
  loadSuccess &= assetManager.loadTexture(
      "fireball_icon", "../assets/sprites/icons/fireball_icon.png");
  loadSuccess &= assetManager.loadTexture(
      "ward_icon", "../assets/sprites/icons/ward_icon.png");
  loadSuccess &=
      assetManager.loadTexture("wall_texture", "../assets/sprites/wall.png");
  loadSuccess &=
      assetManager.loadTexture("floor_1", "../assets/sprites/floor_1.png");
  loadSuccess &=
      assetManager.loadTexture("floor_2", "../assets/sprites/floor_2.png");
  loadSuccess &= assetManager.loadTexture(
      "female_mage_portrait", "../assets/sprites/female_mage_portrait.PNG");
  loadSuccess &= assetManager.loadTexture(
      "male_mage_portrait", "../assets/sprites/male_mage_portrait.PNG");
  loadSuccess &= assetManager.loadTexture(
      "slime_texture",
      "../assets/sprites/slime.PNG"); // Base slime texture if needed
  loadSuccess &= assetManager.loadTexture(
      "health_crystal_texture", "../assets/sprites/health_crystal.png");
  loadSuccess &= assetManager.loadTexture("mana_crystal_texture",
                                          "../assets/sprites/mana_crystal.png");
  loadSuccess &=
      assetManager.loadTexture("magic_missiles_icon", "../assets/sprites/icons/"
                                                      "magic_missile_icon.png");

  loadSuccess &= assetManager.loadTexture(
      "blizzard_icon",
      "../assets/sprites/icons/blizzard_icon.png"); // Add the actual PNG file
                                                    // here

  loadSuccess &= assetManager.loadTexture(
      "vortex_icon", "../assets/sprites/icons/vortex_icon.png");

  loadSuccess &= assetManager.loadTexture(
      "lightning_icon", "../assets/sprites/icons/lightning_bolt_icon.png");

  loadSuccess &= assetManager.loadTexture(
      "void_infusion_icon", "../assets/sprites/icons/void_infusion_icon.png");

  loadSuccess &= assetManager.loadTexture(
      "magic_missile_orbiting", "../assets/sprites/animations/spells/"
                                "magic_missile/magic_missile_launched.png");
  loadSuccess &= assetManager.loadTexture(
      "magic_missile_launched", "../assets/sprites/animations/spells/"
                                "magic_missile/magic_missile_launched.png");

  // --- Animation Sequences using Loops ---

  // Female Mage Animations
  loadSuccess &=
      loadAnimationSequence(assetManager, "mage_idle",
                            "../assets/sprites/animations/mage/idle/", 8, 4);
  loadSuccess &=
      loadAnimationSequence(assetManager, "mage_walk",
                            "../assets/sprites/animations/mage/walk/", 8, 4);
  loadSuccess &= loadAnimationSequence(
      assetManager, "mage_target", "../assets/sprites/animations/mage/target/",
      7, 4); // Assuming base name is targetting

  // Slime Animations
  loadSuccess &= loadAnimationSequence(
      assetManager, "slime_idle",
      "../assets/sprites/animations/enemies/slime/idle/", 8, 4);
  loadSuccess &= loadAnimationSequence(
      assetManager, "slime_walk",
      "../assets/sprites/animations/enemies/slime/walk/", 8, 4);
  loadSuccess &= loadAnimationSequence(
      assetManager, "slime_attack",
      "../assets/sprites/animations/enemies/slime/attack/", 8, 4);

  // Rune Pedestal Animations
  loadSuccess &= loadAnimationSequence(
      assetManager, "rune_pedestal",
      "../assets/sprites/animations/environment/rune_pedestal/", 8,
      1); // No padding needed if filenames are just rune_pedestal_1.png etc.
          // Adjust padding if needed.
  loadSuccess &= loadAnimationSequence(
      assetManager, "rune_pedestal_off",
      "../assets/sprites/animations/environment/rune_pedestal/", 8,
      1); // Assuming files are rune_pedestal_deactivating_1.png etc. Adjust
          // base path and padding if different.

  // Ward Spell Animation
  loadSuccess &= loadAnimationSequence(
      assetManager, "ward_active", "../assets/sprites/animations/spells/ward/",
      8, 4); // Assuming padding is 3 (001)

  loadSuccess &= loadAnimationSequence(
      assetManager, "blizzard_effect",                 // Base name
      "../assets/sprites/animations/spells/blizzard/", // Base path
      10, // Number of frames (adjust as needed)
      4); // Padding (e.g., 4 for 0001)
  // <<< END Blizzard Effect Load >>>

  // <<< ADD Vortex Effect Animation Sequence >>>
  // Adjust baseName, path, frameCount, and padding to match your actual files
  loadSuccess &= loadAnimationSequence(
      assetManager, "vortex_effect", // Base name (e.g., vortex_effect)
      "../assets/sprites/animations/spells/vortex/", // Base path
      8,  // <<< Number of frames in your vortex animation
      4); // <<< Padding for frame numbers (e.g., 4 for 0001)
  // <<< END Vortex Effect Load >>>

  // Adjust baseName, path, frameCount, and padding to match your actual files
  loadSuccess &= loadAnimationSequence(
      assetManager, "lightning_bolt_effect", // Base name (e.g., vortex_effect)
      "../assets/sprites/animations/spells/lightning_bolt/", // Base path
      8,  // <<< Number of frames in your vortex animation
      4); // <<< Padding for frame numbers (e.g., 4 for 0001)
  // <<< END Vortex Effect Load >>>

  // Adjust baseName, path, frameCount, and padding to match your actual files
  loadSuccess &= loadAnimationSequence(
      assetManager, "void_infusion_active", // Base name (e.g., vortex_effect)
      "../assets/sprites/animations/spells/void_infusion/", // Base path
      8,  // <<< Number of frames in your vortex animation
      4); // <<< Padding for frame numbers (e.g., 4 for 0001)
  // <<< END Vortex Effect Load >>>

  // --- Fonts ---
  loadSuccess &=
      assetManager.loadFont("main_font", "../assets/fonts/LUMOS.TTF", 36);
  loadSuccess &=
      assetManager.loadFont("spellbar_font", "../assets/fonts/LUMOS.TTF", 18);

  SDL_Log("--- End Asset Loading (%s) ---",
          loadSuccess ? "Success" : "FAILURE");
  return loadSuccess;
}