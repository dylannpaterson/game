#ifndef LEVEL_H
#define LEVEL_H

#include <vector>
#include <string>
#include <SDL.h>

struct Level {
    int width;
    int height;
    std::vector<std::string> tiles;
    int startRow;
    int startCol;
    int endRow;
    int endCol;
    std::vector<SDL_Rect> rooms; // Add this line
};

Level generateLevel(int width, int height, int maxRooms, int minRoomSize, int maxRoomSize);

#endif // LEVEL_H