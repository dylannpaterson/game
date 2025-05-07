// src/spell.h
#ifndef SPELL_H
#define SPELL_H

#include "status_effect.h"
#include <SDL.h>
#include <string>
#include <vector>

// Forward declarations
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
  Buff,
  Debuff,
  Summon,
  ApplyShield,
  SummonOrbital,
  AreaDamage // <<< ADDED: More explicit type for AoE damage
};

struct Spell {
  std::string name;
  int baseManaCost;
  int baseRange; // Range to the *center* of the AoE

  SpellTargetType targetType;
  SpellEffectType effectType;

  int numDamageDice = 0;
  int damageDieType = 0;
  int baseDamageBonus = 0;
  float baseHealAmount = 0.0f;

  int areaOfEffectRadius = 0; // <<< IMPORTANT for Blizzard
  std::string iconName;

  float baseDistanceDamageBonusPercent = 0.0f;
  float shieldDecayPercent = 0.0f;

  int numOrbitals = 0;
  int orbitalAcquisitionRange = 0;
  float orbitalLifetime = 0.0f;
  std::string orbitalProjectileTextureKey = "";
  float orbitalProjectileSpeed = 600.0f;

  // --- Status Effect Application --- // <<< ADDED MEMBERS
  StatusEffectType statusEffectApplied = StatusEffectType::None;
  int statusEffectDuration = 0; // In turns
  // Optional future additions:
  // int statusEffectChance = 100;
  // float statusEffectMagnitude = 0.0f;
  // --- END ADDED MEMBERS ---

  // --- Constructor for Damage Spells (Tile/Enemy Target) ---
  Spell(std::string n, int cost, int rng, SpellTargetType tt,
        SpellEffectType et, int numDice, int dieType, int dmgBonus,
        float distBonusPct, std::string iconKey, int aoe = 0,
        StatusEffectType statusType = StatusEffectType::None,
        int statusDuration = 0)
      : name(std::move(n)), baseManaCost(cost), baseRange(rng), targetType(tt),
        effectType(et), numDamageDice(numDice), damageDieType(dieType),
        baseDamageBonus(dmgBonus), baseHealAmount(0.0f),
        areaOfEffectRadius(aoe), // <<< Initialize AoE here
        iconName(std::move(iconKey)),
        baseDistanceDamageBonusPercent(distBonusPct),
        statusEffectApplied(statusType),     // <<< INITIALIZE
        statusEffectDuration(statusDuration) // <<< INITIALIZE
  // Zero out other irrelevant fields explicitly if desired
  {}

  // Constructor for shield spells
  Spell(std::string n, int cost, SpellTargetType tt, SpellEffectType et,
        float shieldMagnitude, // Use baseHealAmount conceptually for magnitude
        float decayPercent,    // Pass in the decay rate
        std::string iconKey)
      : name(std::move(n)), baseManaCost(cost), baseRange(0), targetType(tt),
        effectType(et), numDamageDice(0), damageDieType(0),
        baseDamageBonus(0),              // Zero damage dice
        baseHealAmount(shieldMagnitude), // Store magnitude here
        areaOfEffectRadius(0),           // Ensure AoE is 0
        iconName(std::move(iconKey)),
        baseDistanceDamageBonusPercent(0.0f), // Zero distance bonus
        shieldDecayPercent(decayPercent)      // <<< Store decay rate
  {
    if (effectType != SpellEffectType::ApplyShield) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "Shield constructor used for non-shield spell: %s",
                   n.c_str());
    }
  }

  // Constructor for SummonOrbital
  Spell(std::string n, int cost, SpellTargetType tt, SpellEffectType et,
        int count, int acqRange, float lifetime, int numDice, int dieType,
        int dmgBonus, const std::string &projTexKey, float projSpeed,
        std::string iconKey)
      : name(std::move(n)), baseManaCost(cost), baseRange(0), targetType(tt),
        effectType(et), numDamageDice(numDice), damageDieType(dieType),
        baseDamageBonus(dmgBonus), iconName(std::move(iconKey)),
        numOrbitals(count), orbitalAcquisitionRange(acqRange),
        orbitalLifetime(lifetime), orbitalProjectileTextureKey(projTexKey),
        orbitalProjectileSpeed(projSpeed),
        statusEffectApplied(StatusEffectType::None), statusEffectDuration(0) {
    if (effectType != SpellEffectType::SummonOrbital) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "SummonOrbital constructor used for non-orbital spell: %s",
                   n.c_str());
    }
  }

  // Constructor for Area Damage Spells (like Blizzard)
  Spell(std::string n, int cost, int rng, SpellTargetType tt,
        SpellEffectType et, int numDice, int dieType, int dmgBonus,
        std::string iconKey, int aoe,
        StatusEffectType statusType = StatusEffectType::None,
        int statusDuration = 0) // <<< ADDED PARAMS
      : name(std::move(n)), baseManaCost(cost), baseRange(rng), targetType(tt),
        effectType(et), numDamageDice(numDice), damageDieType(dieType),
        baseDamageBonus(dmgBonus), areaOfEffectRadius(aoe),
        iconName(std::move(iconKey)), statusEffectApplied(statusType),
        statusEffectDuration(statusDuration) // <<< INITIALIZE
  { /* Validation */ }

  // Potentially add a function to apply the effect, though this might be better
  // handled elsewhere void applyEffect(PlayerCharacter& caster, /* target(s) */
  // );
};

#endif // SPELL_H