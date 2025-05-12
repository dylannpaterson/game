#include "visibility.h"
#include "level.h" // Make sure level.h is included here as well
#include "utils.h"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <vector>

void updateVisibility(const Level &level, const std::vector<SDL_Rect> &rooms,
                      int playerX, int playerY, int hallwayVisibilityDistance,
                      std::vector<std::vector<float>> &visibilityMap) {
  int width = level.width;
  int height = level.height;
  int brightRadius = 4;
  int dimRadius = 9;
  int rayThickness = 1;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      visibilityMap[y][x] = 0.0f; // Initialize with float
    }
  }

  auto isWithinBoundsFunc = [&](int x, int y, int w, int h) {
    return x >= 0 && x < w && y >= 0 && y < h;
  };

  auto canPass = [&](int x, int y) {
    return isWithinBoundsFunc(x, y, width, height) && level.tiles[y][x] != '#';
  };

  auto castRay = [&](int startX, int startY, int endX, int endY,
                     float brightness) {
    for (int offsetX = -rayThickness; offsetX <= rayThickness; ++offsetX) {
      for (int offsetY = -rayThickness; offsetY <= rayThickness; ++offsetY) {
        int x0 = startX + offsetX;
        int y0 = startY + offsetY;
        int x1 = endX;
        int y1 = endY;

        if (isWithinBoundsFunc(x0, y0, width, height) &&
            level.tiles[y0][x0] != '#') {
          int dx_ray = std::abs(x1 - x0);
          int dy_ray = std::abs(y1 - y0);
          int sx = (x0 < x1) ? 1 : -1;
          int sy = (y0 < y1) ? 1 : -1;
          int err = dx_ray - dy_ray;
          int currentX = x0;
          int currentY = y0;
          bool blocked = false;

          while (true) {
            if (isWithinBoundsFunc(currentX, currentY, width, height)) {
              if (currentX == endX && currentY == endY) {
                visibilityMap[currentY][currentX] =
                    std::max(visibilityMap[currentY][currentX], brightness);
                return true;
              }
              if ((level.tiles[currentY][currentX] == '#' ||
                   level.tiles[currentY][currentX] == 'V') &&
                  (currentX != x0 || currentY != y0)) {
                blocked = true;
                break;
              }
            } else {
              break;
            }

            int e2 = 2 * err;
            if (e2 > -dy_ray) {
              err -= dy_ray;
              currentX += sx;
            }
            if (e2 < dx_ray) {
              err += dx_ray;
              currentY += sy;
            }
          }
          if (!blocked && (currentX == endX && currentY == endY))
            return true;
        }
      }
    }
    return false;
  };

  for (int dy = -dimRadius; dy <= dimRadius; ++dy) {
    for (int dx = -dimRadius; dx <= dimRadius; ++dx) {
      int targetX = playerX + dx;
      int targetY = playerY + dy;

      if (!isWithinBoundsFunc(targetX, targetY, width, height) ||
          level.tiles[targetY][targetX] == 'V')
        continue;

      float distance = std::sqrt(dx * dx + dy * dy);
      float brightness = 0.0f;

      if (distance < brightRadius) {
        brightness = 1.0f; // 100% bright
      } else if (distance >= brightRadius && distance < dimRadius) {
        // Linear falloff from 100% to 0%
        brightness =
            1.0f - (distance - brightRadius) / (dimRadius - brightRadius);
        brightness =
            std::max(0.0f, std::min(1.0f, brightness)); // Ensure within 0-1
      }

      if (brightness > 0.0f) {
        castRay(playerX, playerY, targetX, targetY, brightness);
      }
    }
  }
}