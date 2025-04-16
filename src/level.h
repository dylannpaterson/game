#ifndef LEVEL_H
#define LEVEL_H

#include <vector>
#include <string>
#include <SDL.h>
#include "enemy.h" // Make sure this is included if Enemy is used in Level

struct Level {
    int width;
    int height;
    std::vector<std::string> tiles;
    std::vector<SDL_Rect> rooms;
    int startRow;
    int startCol;
    int endRow;
    int endCol;
};

// Declaration of the manhattanDistance function
int manhattanDistance(const SDL_Rect& room1, const SDL_Rect& room2);

// Declaration of the generateLevel function (CRITICAL UPDATE HERE)
Level generateLevel(int width, int height, int maxRooms, int minRoomSize, int maxRoomSize, std::vector<Enemy>& enemies);

#endif