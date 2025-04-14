#include "level.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm> // For std::shuffle
#include <set>
#include <limits> // For std::numeric_limits

// Helper function to calculate Manhattan distance between two rooms
int manhattanDistance(const SDL_Rect& room1, const SDL_Rect& room2) {
    int x1 = room1.x + room1.w / 2;
    int y1 = room1.y + room1.h / 2;
    int x2 = room2.x + room2.w / 2;
    int y2 = room2.y + room2.h / 2;
    return std::abs(x1 - x2) + std::abs(y1 - y2);
}

Level generateLevel(int width, int height, int maxRooms, int minRoomSize, int maxRoomSize) {
    Level level;
    level.width = width;
    level.height = height;
    level.tiles.resize(height, std::string(width, 'V')); // 1. Make everything void

    std::vector<SDL_Rect> rooms;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> roomWidthDist(minRoomSize, maxRoomSize);
    std::uniform_int_distribution<> roomHeightDist(minRoomSize, maxRoomSize);
    std::uniform_int_distribution<> xDist(1, width - maxRoomSize - 2);
    std::uniform_int_distribution<> yDist(1, height - maxRoomSize - 2);

    // 2. Place rooms
    for (int i = 0; i < maxRooms; ++i) {
        int roomWidth = roomWidthDist(gen);
        int roomHeight = roomHeightDist(gen);
        int roomX = xDist(gen);
        int roomY = yDist(gen);

        SDL_Rect newRoom = {roomX, roomY, roomWidth + 2, roomHeight + 2}; // Increase size for walls
        bool overlaps = false;
        for (const auto& existingRoom : rooms) {
            if (newRoom.x < existingRoom.x + existingRoom.w &&
                newRoom.x + newRoom.w > existingRoom.x &&
                newRoom.y < existingRoom.y + existingRoom.h &&
                newRoom.y + newRoom.h > existingRoom.y) {
                overlaps = true;
                break;
            }
        }

        if (!overlaps) {
            rooms.push_back(newRoom);
            // 3. Line inside of rooms with walls
            for (int y = roomY + 1; y < roomY + roomHeight + 1; ++y) {
                for (int x = roomX + 1; x < roomX + roomWidth + 1; ++x) {
                    if (x > roomX && x < roomX + roomWidth + 1 && y > roomY && y < roomY + roomHeight + 1)
                        level.tiles[y][x] = '.'; // Inner area is floor
                }
            }
            // Create walls around the floor
            for (int x = roomX; x < roomX + roomWidth + 2; ++x) {
                if (roomY >= 0 && x >= 0 && x < width) level.tiles[roomY][x] = '#'; // Top wall
                if (roomY + roomHeight + 1 < height && x >= 0 && x < width) level.tiles[roomY + roomHeight + 1][x] = '#'; // Bottom wall
            }
            for (int y = roomY; y < roomY + roomHeight + 2; ++y) {
                if (roomX >= 0 && y >= 0 && y < height) level.tiles[y][roomX] = '#'; // Left wall
                if (roomX + roomWidth + 1 < width && y >= 0 && y < height) level.tiles[y][roomX + roomWidth + 1] = '#'; // Right wall
            }
        }
    }

    int numRooms = rooms.size();
    if (numRooms > 1) {
        // 4. Connect rooms with hallways
        std::vector<int> parent(numRooms);
        std::vector<int> key(numRooms, std::numeric_limits<int>::max());
        std::vector<bool> mstSet(numRooms, false);

        key[0] = 0;
        parent[0] = -1; // First node is root

        for (int count = 0; count < numRooms - 1; ++count) {
            int u = -1;
            for (int v = 0; v < numRooms; ++v) {
                if (!mstSet[v] && (u == -1 || key[v] < key[u])) {
                    u = v;
                }
            }

            if (u == -1) break;

            mstSet[u] = true;

            for (int v = 0; v < numRooms; ++v) {
                if (u != v && !mstSet[v]) {
                    int dist = manhattanDistance(rooms[u], rooms[v]);
                    if (dist < key[v]) {
                        key[v] = dist;
                        parent[v] = u;
                    }
                }
            }
        }

        for (int i = 1; i < numRooms; ++i) {
            int parentRoomIndex = parent[i];
            int room1Index = i;
            int room2Index = parentRoomIndex;

            int x1 = rooms[room1Index].x + rooms[room1Index].w / 2;
            int y1 = rooms[room1Index].y + rooms[room1Index].h / 2;
            int x2 = rooms[room2Index].x + rooms[room2Index].w / 2;
            int y2 = rooms[room2Index].y + rooms[room2Index].h / 2;

            int currentX = x1;
            int currentY = y1;

            while (currentX != x2) {
                if (currentX >= 0 && currentX < width && currentY >= 0 && currentY < height) {
                    level.tiles[currentY][currentX] = '.'; // Replace walls with floors
                }
                currentX += (currentX < x2) ? 1 : -1;
            }
            while (currentY != y2) {
                if (currentX >= 0 && currentX < width && currentY >= 0 && currentY < height) {
                    level.tiles[currentY][currentX] = '.'; // Replace walls with floors
                }
                currentY += (currentY < y2) ? 1 : -1;
            }
            if (currentX >= 0 && currentX < width && currentY >= 0 && currentY < height) {
                level.tiles[currentY][currentX] = '.'; // Replace walls with floors
            }
        }

        // Add walls to hallways
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (level.tiles[y][x] == '.') {
                    int dx[] = {0, 0, 1, -1};
                    int dy[] = {1, -1, 0, 0};
                    for (int i = 0; i < 4; ++i) {
                        int nx = x + dx[i];
                        int ny = y + dy[i];
                        if (ny >= 0 && ny < height && nx >= 0 && nx < width && level.tiles[ny][nx] == 'V') {
                            level.tiles[ny][nx] = '#';
                        }
                    }
                }
            }
        }
    }

    level.rooms = rooms;

    // Place start and end
    if (!rooms.empty()) {
        std::uniform_int_distribution<> startRoomDist(0, rooms.size() - 1);
        int startRoomIndex = startRoomDist(gen);
        level.startRow = rooms[startRoomIndex].y + 1;
        level.startCol = rooms[startRoomIndex].x + 1;
        level.tiles[level.startRow][level.startCol] = '.';

        if (rooms.size() > 1) {
            int endRoomIndex;
            do {
                endRoomIndex = startRoomDist(gen);
            } while (endRoomIndex == startRoomIndex);
            level.endRow = rooms[endRoomIndex].y + rooms[endRoomIndex].h - 2;
            level.endCol = rooms[endRoomIndex].x + rooms[endRoomIndex].w - 2;
            level.tiles[level.endRow][level.endCol] = '.';
        }
    }

    return level;
}