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
#include "orbital_missile.h" 
#include <SDL.h>        // For SDL_Renderer*, SDL_Texture* etc.
#include <SDL_ttf.h>    // For TTF_Font*

// --- NEW: Action Types ---
// Defines the possible actions an actor can intend to perform during the planning phase.
enum class ActionType {
    None,       // No action planned (or default state)
    Wait,       // Explicitly choose to do nothing
    Move,       // Intend to move to a target tile
    Attack,     // Intend to perform a melee attack (e.g., bump) - Can be expanded
    CastSpell,   // Intend to cast a specific spell
    Interact
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

enum class ItemType {
    HealthCrystal,
    ManaCrystal
};

// Structure to represent an item dropped on the ground
struct ItemDrop {
    int x; // Tile X coordinate
    int y; // Tile Y coordinate
    ItemType type; // What kind of item it is
    std::string textureName; // Key for the AssetManager
    // Add other properties if needed later (e.g., amount, identifier)
};

struct RunePedestal {
    int x; // Tile X coordinate
    int y; // Tile Y coordinate

    // --- Animation Frames ---
    std::vector<std::string> idleFrameTextureNames;     // Keys for idle animation frames
    std::vector<std::string> deactivationFrameTextureNames; // Keys for deactivation animation

    // --- Animation State ---
    float animationTimer = 0.0f;
    int currentFrame = 0;
    float idleAnimationSpeed = 4.0f;         // Frames per second for idle
    float deactivationAnimationSpeed = 8.0f; // Faster speed for deactivation? (Adjust)

    // --- Pedestal State ---
    bool isActive = true;         // Can it be interacted with?
    bool isDeactivating = false;  // Is it currently playing the deactivation animation?
    // Note: When isActive is false AND isDeactivating is false, it's fully deactivated.

    // Default constructor
    RunePedestal() : x(0), y(0), idleAnimationSpeed(4.0f), deactivationAnimationSpeed(8.0f), isActive(true), isDeactivating(false) {}

    // Constructor for placement
    RunePedestal(int posX, int posY) : x(posX), y(posY), idleAnimationSpeed(4.0f), deactivationAnimationSpeed(8.0f), isActive(true), isDeactivating(false) {
        // Populate IDLE frame names (ensure these match loaded assets)
        for (int i = 1; i <= 10; ++i) {
            idleFrameTextureNames.push_back("rune_pedestal_" + std::to_string(i));
        }
        // Populate DEACTIVATION frame names (ensure these match loaded assets)
        // Example: Assuming 9 deactivation frames named rune_pedestal_off_1 to _9
        for (int i = 1; i <= 9; ++i) {
             deactivationFrameTextureNames.push_back("rune_pedestal_off_" + std::to_string(i));
        }
        // Ensure there's at least one deactivation frame if logic relies on the last frame
        if (deactivationFrameTextureNames.empty()) {
             SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "RunePedestal created with no deactivation frames defined!");
             // Optionally add a fallback frame name here
             // deactivationFrameTextureNames.push_back("rune_pedestal_off_fallback");
        }
    }
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
    std::vector<OrbitalMissile> activeOrbitals;
    std::vector<ItemDrop> droppedItems; // *** NEW: Container for dropped items ***
    Level currentLevel;                         // Holds the current level layout (tiles, dimensions)
    std::optional<RunePedestal> currentPedestal;// Stores the single pedestal for the current level
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
    int logicalWidth = 1920;  // <-- ADD THIS (Set your desired logical width)
    int logicalHeight = 1080; // <-- ADD THIS (Set your desired logical height)
    int tileWidth = 96;
    int tileHeight = 96;
    int levelWidth = 120;
    int levelHeight = 75;
    int levelMaxRooms = 15;
    int levelMinRoomSize = 8;
    int levelMaxRoomSize = 15;
    int hallwayVisibilityDistance = 5;
    int currentLevelIndex = 1;
    float enemyStatScalingPerFloor = 0.10f;
    int crystalDropChancePercent = 30; // *** NEW: Chance (0-100) for an enemy to drop *any* crystal ***
    int healthCrystalChancePercent = 50; // *** NEW: Chance (0-100) for a dropped crystal to be RED (Health) ***
    int maxEnemyCount = 100;
    int spawnChancePercent = 15;


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
