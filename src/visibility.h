#ifndef VISIBILITY_H
#define VISIBILITY_H

#include <vector>
#include <SDL.h>

// Assuming Level is defined in 'level.h'
#include "level.h"

void updateVisibility(const Level& level, const std::vector<SDL_Rect>& rooms, int playerX, int playerY, int hallwayVisibilityDistance, std::vector<std::vector<float>>& visibilityMap);

#endif // VISIBILITY_H