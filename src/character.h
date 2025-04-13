#ifndef CHARACTER_H
#define CHARACTER_H

enum class CharacterType {
    FemaleMage,
    MaleMage
};

struct PlayerCharacter {
    CharacterType type;
    // We can add more attributes later, such as name, level, stats, etc.
};

#endif // CHARACTER_H