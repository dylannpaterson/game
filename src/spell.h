// spell.h
#ifndef SPELL_H
#define SPELL_H

#include <string>
#include <vector> // Forward declare if needed, or include headers

// Forward declarations to avoid circular dependencies if needed
class PlayerCharacter;
class Enemy;
struct Level;

enum class SpellTargetType {
    Self,
    Enemy,
    Tile,
    Area // For spells affecting multiple tiles
};

enum class SpellEffectType {
    Damage,
    Heal,
    Buff,   // e.g., Increase defense
    Debuff, // e.g., Decrease attack
    Summon  // If you dare tread that path
    // Add more as your world demands
};

struct Spell {
    std::string name;
    int manaCost;
    int range; // In tiles. 0 for self, 1 for adjacent, etc.
    SpellTargetType targetType;
    SpellEffectType effectType;
    float value; // Magnitude of the effect (e.g., damage amount, heal amount)
    int areaOfEffectRadius; // 0 for single target/tile

    // Optional: Add fields for visual effects (particle system name, sound effect name, etc.)
    // std::string visualEffectName;
    // std::string soundEffectName;

    Spell(std::string n, int cost, int rng, SpellTargetType tt, SpellEffectType et, float val, int aoe = 0)
        : name(std::move(n)), manaCost(cost), range(rng), targetType(tt), effectType(et), value(val), areaOfEffectRadius(aoe) {}

    // Potentially add a function to apply the effect, though this might be better handled elsewhere
    // void applyEffect(PlayerCharacter& caster, /* target(s) */ );
};

#endif // SPELL_H