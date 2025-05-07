// src/status_effect.h
#ifndef STATUS_EFFECT_H
#define STATUS_EFFECT_H

#include <string> // Potentially for names or descriptions later

// Enum defining the different types of status effects
enum class StatusEffectType {
  Stunned,     // Cannot move or attack (or cast?)
  Immobilised, // Cannot move
  Blinded,     // Cannot attack (specifically targeted attacks? Check range
               // reduced?)
  Silenced,    // Cannot cast spells
  Disoriented, // Forced random movement
  Burning,     // Takes damage over time (Example for future)
  Slowed,      // Movement takes longer (Example for future)
               // Add more as needed
  None
};

// Struct to represent an active status effect on an entity
struct StatusEffect {
  StatusEffectType type;
  int durationTurns; // How many turns the effect lasts

  // Add more fields later if needed:
  // float magnitude; // e.g., for damage-over-time or slow percentage
  // int sourceID; // e.g., ID of the entity that applied the effect
  // std::string sourceSpellName; // Name of the spell/ability that caused it

  // Constructor
  StatusEffect(StatusEffectType t, int duration)
      : type(t), durationTurns(duration) {}

  // Default constructor (optional, but can be useful)
  StatusEffect() : type(StatusEffectType::Stunned), durationTurns(0) {}
};

#endif // STATUS_EFFECT_H