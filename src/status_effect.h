// src/status_effect.h
#ifndef STATUS_EFFECT_H
#define STATUS_EFFECT_H

#include <string>  // Potentially for names or descriptions later
#include <variant> // Include for std::variant

// Enum defining the different types of status effects
enum class StatusEffectType {
  Stunned,      // Cannot move or attack (or cast?)
  Immobilised,  // Cannot move
  Blinded,      // Cannot attack (specifically targeted attacks? Check range
                // reduced?)
  Silenced,     // Cannot cast spells
  Disoriented,  // Forced random movement
  Burning,      // Takes damage over time (Example for future)
  Slowed,       // Movement takes longer (Example for future)
  VoidInfusion, // Buff: Increases attributes by a percentage
                // Add more as needed
  None
};

// Define a variant to hold different effect magnitudes
using EffectMagnitude = std::variant<int, float>;

// Struct to represent an active status effect on an entity
struct StatusEffect {
  StatusEffectType type;
  int durationTurns;         // How many turns the effect lasts
  EffectMagnitude magnitude; // Use variant for different types

  // Add more fields later if needed:
  // int sourceID; // e.g., ID of the entity that applied the effect
  // std::string sourceSpellName; // Name of the spell/ability that caused it

  // Constructor
  StatusEffect(StatusEffectType t, int duration,
               EffectMagnitude mag = 0) // Added magnitude parameter
      : type(t), durationTurns(duration), magnitude(mag) {}

  // Default constructor (optional, but can be useful)
  StatusEffect()
      : type(StatusEffectType::None), durationTurns(0), magnitude(0) {
  } // Initialize magnitude
};

#endif // STATUS_EFFECT_H