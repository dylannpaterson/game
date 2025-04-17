// game_data.h
#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <vector>
#include <string>
#include <map> // If AssetManager uses maps and is included here, or forward declare

// Include headers for types used AS MEMBERS in GameData
#include <SDL.h>          // For SDL_Renderer*, SDL_Texture* etc.
#include <SDL_ttf.h>      // For TTF_Font*
#include "character.h"    // For PlayerCharacter
#include "enemy.h"        // For std::vector<Enemy>
#include "projectile.h"   // For std::vector<Projectile>
#include "level.h"        // For Level


enum class GameState {
    MainMenu,
    CharacterSelect,
    // GamePlay, // Can likely be removed
    PlayerTurn,
    PlayerTargeting, // New state for spell targeting
    ProjectileResolution,
    EnemyTurn,
    GameOver
  };

enum class GameMenu {
    None,     // No menu overlay active
    SpellMenu // Spell selection menu is active
  };
  
  struct GameData {
    // --- Rendering Context ---
    SDL_Renderer *renderer = nullptr;
  
    // --- Game State & Control ---
    GameState currentGameState = GameState::MainMenu;
    GameMenu currentMenu = GameMenu::None;
    int spellSelectIndex = 0;
    int currentSpellIndex = -1;
  
    // --- Entities & Level ---
    // Note: PlayerCharacter needs default constructor or careful initialization
    // later
    PlayerCharacter currentGamePlayer{
        CharacterType::FemaleMage, 100, 100, 150, 150, 1, 0, 0, 64, 64};
    std::vector<Enemy> enemies;
    std::vector<Projectile> activeProjectiles;
    Level currentLevel;
    std::vector<SDL_Rect> levelRooms;
    std::vector<std::vector<float>> visibilityMap;
  
    // --- UI / Menu State ---
    std::vector<std::string> menuItems = {"Start Game", "Options", "Exit"};
    int selectedIndex = 0; // Main menu index
    int selectedCharacterIndex = 0;
    bool isPanning = false;
    int splashPanOffset = 456;
    int panCounter = 0;
    bool isCharacterSelectFadingIn = false;
    Uint8 characterSelectAlpha = 0;
    bool hasCharacterSelectStartedFading = false;
    int targetIndicatorX = 0;
    int targetIndicatorY = 0;
  
    // --- Camera ---
    int cameraX = 0;
    int cameraY = 0;
  
    // --- Settings ---
    int windowWidth = 1920;
    int windowHeight = 1080;
    int tileWidth = 128;
    int tileHeight = 128;
    int levelWidth = 120;
    int levelHeight = 75;
    int levelMaxRooms = 15;
    int levelMinRoomSize = 8;
    int levelMaxRoomSize = 15;
    int hallwayVisibilityDistance = 5;
    int currentLevelIndex = 1;
  
    // --- Frame Input Flags --- (These can move inside handleEvents/updateLogic
    // if preferred)
    bool menuUpThisFrame = false;
    bool menuDownThisFrame = false;
    bool menuSelectThisFrame = false;
    bool menuCancelThisFrame = false;
  
    // --- Destructor for Cleanup ---
    // Automatically cleans up textures when GameData goes out of scope
    ~GameData() { SDL_Log("GameData destructor: "); }
  };

#endif // GAME_DATA_H