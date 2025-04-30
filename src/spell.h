// spell.h
#ifndef SPELL_H
#define SPELL_H

#include <SDL.h>
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
  Buff,       // e.g., Increase defense
  Debuff,     // e.g., Decrease attack
  Summon,     // If you dare tread that path
  ApplyShield, // <<< New Type
  SummonOrbital // <<< New Type
              // Add more as your world demands
};

struct Spell {
  std::string name;
  // --- Base Attributes (Potentially Modifiable Later) ---
  int baseManaCost;
  int baseRange;

  // --- Target & Effect Type ---
  SpellTargetType targetType;
  SpellEffectType effectType;

  // --- Dice-Based Effect Parameters (Replacing float value) ---
  int numDamageDice = 0; // How many dice to roll (e.g., 6 for 6d6)
  int damageDieType = 0; // What type of dice (e.g., 6 for d6)
  int baseDamageBonus =
      0; // Flat bonus added after rolling (e.g., +2 for XdY+2)
  // --- OR ---
  float baseHealAmount = 0.0f; // Keep separate for clarity if needed

  // --- Other Attributes ---
  int areaOfEffectRadius; // 0 for single target/tile
  std::string iconName;

  float baseDistanceDamageBonusPercent =
      0.0f; // Base bonus % per tile beyond adjacent (e.g., 0.10 for 10%)

  float shieldDecayPercent =
      0.0f; // Percentage of max shield lost per turn (e.g., 0.20 for 20%)

  // ---> ADD Parameters for Orbitals <---
  int numOrbitals = 0;
  int orbitalAcquisitionRange = 0;
  float orbitalLifetime = 0.0f;
  std::string orbitalProjectileTextureKey =
      ""; // Key for the launched projectile texture
  float orbitalProjectileSpeed = 600.0f; // Speed for the launched projectile

  // --- Updated Constructor Example ---
  // Constructor now takes dice parameters for damage spells
  Spell(std::string n, int cost, int rng, SpellTargetType tt,
        SpellEffectType et, int numDice, int dieType,
        int dmgBonus, // Dice parameters
        float distBonusPct, std::string iconKey, int aoe = 0)
      : name(std::move(n)), baseManaCost(cost), baseRange(rng), targetType(tt),
        effectType(et), numDamageDice(numDice), damageDieType(dieType),
        baseDamageBonus(dmgBonus),
        baseHealAmount(0.0f), // Ensure heal is zeroed for damage spells
        baseDistanceDamageBonusPercent(distBonusPct),
        iconName(std::move(iconKey)), areaOfEffectRadius(aoe) {
    // If it's specifically a Healing spell, you might want a different
    // constructor or set baseHealAmount based on parameters instead of dice.
    // For simplicity now, we focus on damage dice.
  }

  // --- Constructor Example for Self-Heal (if needed) ---
  Spell(std::string n, int cost, SpellTargetType tt, SpellEffectType et,
        float healVal, // Specific heal value
        std::string iconKey)
      : name(std::move(n)), baseManaCost(cost), baseRange(0), targetType(tt),
        effectType(et), numDamageDice(0), damageDieType(0),
        baseDamageBonus(0), // Zero dice for heal
        baseHealAmount(healVal), iconName(std::move(iconKey)),
        areaOfEffectRadius(0) {
    // Ensure target type is self if using this constructor signature
    if (targetType != SpellTargetType::Self ||
        effectType != SpellEffectType::Heal) {
      // Handle error - maybe log or throw?
      SDL_LogError(
          SDL_LOG_CATEGORY_APPLICATION,
          "Heal spell constructor used for non-heal/non-self spell: %s",
          n.c_str());
    }
  }

  // Constructor for shield spells
  Spell(std::string n, int cost, SpellTargetType tt, SpellEffectType et,
        float shieldMagnitude, // Use baseHealAmount conceptually for magnitude
        float decayPercent,    // Pass in the decay rate
        std::string iconKey)
      : name(std::move(n)), baseManaCost(cost), baseRange(0), targetType(tt),
        effectType(et), numDamageDice(0), damageDieType(0),
        baseDamageBonus(0),                   // Zero damage dice
        baseHealAmount(shieldMagnitude),      // Store magnitude here
        baseDistanceDamageBonusPercent(0.0f), // Zero distance bonus
        shieldDecayPercent(decayPercent),     // <<< Store decay rate
        iconName(std::move(iconKey)), areaOfEffectRadius(0) {
    // Add validation if needed (e.g., ensure type is ApplyShield)
    if (effectType != SpellEffectType::ApplyShield) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "Shield constructor used for non-shield spell: %s",
                   n.c_str());
    }
  }
  Spell(std::string n, int cost, SpellTargetType tt, SpellEffectType et,
        int count, int acqRange, float lifetime, // Orbital specific params
        int numDice, int dieType, int dmgBonus,  // Payload damage params
        const std::string &projTexKey,
        float projSpeed, // Launched projectile params
        std::string iconKey)
      : name(std::move(n)), baseManaCost(cost), baseRange(0), targetType(tt),
        effectType(et), numDamageDice(numDice), damageDieType(dieType),
        baseDamageBonus(dmgBonus), baseHealAmount(0.0f),
        baseDistanceDamageBonusPercent(0.0f),
        shieldDecayPercent(0.0f), // Zero out non-relevant params
        numOrbitals(count), orbitalAcquisitionRange(acqRange),
        orbitalLifetime(lifetime), orbitalProjectileTextureKey(projTexKey),
        orbitalProjectileSpeed(projSpeed), iconName(std::move(iconKey)),
        areaOfEffectRadius(0) {
    if (effectType != SpellEffectType::SummonOrbital) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "SummonOrbital constructor used for non-orbital spell: %s",
                   n.c_str());
    }
  }

  // Potentially add a function to apply the effect, though this might be better
  // handled elsewhere void applyEffect(PlayerCharacter& caster, /* target(s) */
  // );
};

#endif // SPELL_H