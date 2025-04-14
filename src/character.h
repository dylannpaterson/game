#ifndef CHARACTER_H
#define CHARACTER_H

enum class CharacterType {
    FemaleMage,
    MaleMage
};

struct PlayerCharacter {
    CharacterType type;
    int health;
    int maxHealth;
    int mana;
    int maxMana;
    int level;
};

#endif // CHARACTER_H