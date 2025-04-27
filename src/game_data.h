// src/game_data.h
#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <map>
#include <string>
#include <vector>

// Include headers for types used AS MEMBERS in GameData
#include "character.h"  // For PlayerCharacter
#include "enemy.h"      // For std::vector<Enemy>
#include "level.h"      // For Level
#include "projectile.h" // For std::vector<Projectile>
#include <SDL.h>        // For SDL_Renderer*, SDL_Texture* etc.
#include <SDL_ttf.h>    // For TTF_Font*

// --- NEW: Action Types ---
// Defines the possible actions an actor can intend to perform during the planning phase.
enum class ActionType {
    None,       // No action planned (or default state)
    Wait,       // Explicitly choose to do nothing
    Move,       // Intend to move to a target tile
    Attack,     // Intend to perform a melee attack (e.g., bump) - Can be expanded
    CastSpell   // Intend to cast a specific spell
};

// --- NEW: Structure to hold planned actions ---
// Stores the details of an action decided during the planning phase,
// before it is executed in the resolution phase.
struct IntendedAction {
    ActionType type = ActionType::None; // The type of action planned
    int targetX = -1;       // Target tile X coordinate (used for Move, Tile-Targeted Spells)
    int targetY = -1;       // Target tile Y coordinate (used for Move, Tile-Targeted Spells)
    int targetEntityID = -1;// Placeholder for targeting specific entities (Requires unique IDs on Player/Enemy)
    int spellIndex = -1;    // Index of the spell intended to be cast (from the caster's knownSpells)
    int enemyId = -1; 
    // Consider adding caster information if needed later (e.g., casterID or pointer)
};


// --- NEW: Turn Phases ---
// Replaces the old GameState enum to manage the distinct stages of a simultaneous turn.
enum class TurnPhase {
    // Planning Phase: Actors decide what they want to do.
    Planning_PlayerInput, // Waiting for player input to determine their IntendedAction.
    Planning_EnemyAI,     // Enemies sequentially determine their IntendedActions.

    // Action Resolution Phase: Execute the planned actions concurrently.
    // This might span multiple frames for animations.
    Resolution_Start,     // Initiate all planned moves, attacks, spell casts. Create projectiles.
    Resolution_Update,    // Update ongoing animations (movement interpolation), projectile movement.

    // Cleanup Phase: Apply results and prepare for the next turn.
    TurnEnd_ApplyEffects, // Apply damage from attacks/spells that resolved this phase. Handle healing, status effects.
    TurnEnd_Cleanup       // Check for deaths, remove dead entities, reset flags, update visibility, regen mana, etc.
                          // Finally, transition back to Planning_PlayerInput for the next turn.
};

// Forward declaration for AssetManager if needed, though not directly stored here now.
// class AssetManager;

// Enum for UI Overlays (remains useful)
enum class GameMenu {
  None,     // No menu overlay active
  SpellMenu, // Spell selection menu is active
  CharacterSheet // Character info sheet is active
};


// --- Main Game Data Structure ---
struct GameData {
    // --- Rendering Context ---
    SDL_Renderer *renderer = nullptr; // Pointer to the main SDL renderer

    // --- NEW: Turn Phase & Control ---
    TurnPhase currentPhase = TurnPhase::Planning_PlayerInput; // Start by waiting for player input
    int currentEnemyPlanningIndex = 0; // Used to iterate through enemies during Planning_EnemyAI phase

    // --- Entities & Level ---
    // PlayerCharacter needs default constructor or initialization in main.cpp
    PlayerCharacter currentGamePlayer{CharacterType::FemaleMage, 0, 0, 64, 64}; // Example initialization
    std::vector<Enemy> enemies;                 // Stores all enemies currently in the level
    std::vector<Projectile> activeProjectiles;  // Stores projectiles currently in flight
    Level currentLevel;                         // Holds the current level layout (tiles, dimensions)
    std::vector<SDL_Rect> levelRooms;           // Stores the generated room rectangles
    std::vector<std::vector<float>> visibilityMap; // Stores visibility level (0.0 to 1.0) for each tile
    std::vector<std::vector<bool>> occupationGrid; // Represents the CURRENT occupied state of tiles (walls, entities)
    // Optional: A separate grid could track *intended* occupation during Planning_EnemyAI
    // std::vector<std::vector<bool>> intendedOccupationGrid;

    // --- NEW: Stored Intended Actions ---
    IntendedAction playerIntendedAction;        // Stores the action the player decides on
    std::vector<IntendedAction> enemyIntendedActions; // Stores actions planned by enemies (resized each turn)


    // --- UI / Menu State (Keep relevant parts) ---
    GameMenu currentMenu = GameMenu::None;      // Tracks active overlay menus (Spell, Character Sheet)
    int spellSelectIndex = 0;                   // Index for highlighting in the spell menu
    int currentSpellIndex = -1;                 // Index of spell selected by player for casting/targeting
    int targetIndicatorX = 0;                   // X coordinate for player's targeting reticle
    int targetIndicatorY = 0;                   // Y coordinate for player's targeting reticle
    bool showTargetingReticle = false;          // Controls rendering of the targeting indicator

    // Main Menu/Character Select state (Keep for game start flow)
    std::vector<std::string> menuItems = {"Start Game", "Options", "Exit"};
    int selectedIndex = 0; // Main menu index
    int selectedCharacterIndex = 0;
    bool isPanning = false;
    int splashPanOffset = 456;
    int panCounter = 0;
    bool isCharacterSelectFadingIn = false;
    Uint8 characterSelectAlpha = 0;
    bool hasCharacterSelectStartedFading = false;
    // Note: MainMenu and CharacterSelect might need to become separate `GameState` values
    // outside the main `TurnPhase` loop, or handled carefully at the start.

    // --- Camera ---
    int cameraX = 0; // Camera's top-left X coordinate relative to the level
    int cameraY = 0; // Camera's top-left Y coordinate relative to the level

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
    int maxEnemyCount = 100;
    int spawnChancePercent = 15;


    // --- OBSOLETE STATE (Marked for removal/ignore in new system) ---
    /*
    GameState currentGameState = GameState::MainMenu; // Replaced by currentPhase for gameplay loop
    int enemiesActingThisTurn = 0; // Replaced by the Resolution phase logic determining completion
    int currentEnemyUpdateIndex = 0; // Replaced by currentEnemyPlanningIndex for AI planning phase
    const int ENEMY_UPDATES_PER_FRAME = 10; // No longer needed for batching AI decisions this way
    */

    // --- Frame Input Flags --- (Can still be useful in handleEvents)
    bool menuUpThisFrame = false;
    bool menuDownThisFrame = false;
    bool menuSelectThisFrame = false;
    bool menuCancelThisFrame = false;


    // --- Destructor ---
    // No dynamic memory owned directly by GameData members currently shown,
    // but good practice to have if textures/fonts were managed here.
    ~GameData() {
        SDL_Log("GameData destructor called.");
        // Cleanup logic would go here if GameData owned resources directly.
    }
};

#endif // GAME_DATA_H
