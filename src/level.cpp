#include "level.h"
#include "enemy.h" // Make sure Enemy is included
#include "utils.h" // For isWithinBounds function
#include <fstream>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <set>
#include <limits>
#include <cmath> // For std::min


// Helper function to calculate Manhattan distance between two rooms
int manhattanDistance(const SDL_Rect& room1, const SDL_Rect& room2) {
    int x1 = room1.x + room1.w / 2;
    int y1 = room1.y + room1.h / 2;
    int x2 = room2.x + room2.w / 2;
    int y2 = room2.y + room2.h / 2;
    return std::abs(x1 - x2) + std::abs(y1 - y2);
}


Level generateLevel(int width, int height, int maxRooms, int minRoomSize, int maxRoomSize, std::vector<Enemy>& enemies, int tileW, int tileH,
    std::optional<SDL_Point>& outPedestalPos) {
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
            // Add buffer to overlap check to prevent walls touching
            if (newRoom.x < existingRoom.x + existingRoom.w + 1 &&
                newRoom.x + newRoom.w + 1 > existingRoom.x &&
                newRoom.y < existingRoom.y + existingRoom.h + 1 &&
                newRoom.y + newRoom.h + 1 > existingRoom.y) {
                overlaps = true;
                break;
            }
        }

        if (!overlaps) {
            rooms.push_back(newRoom);
            // 3. Carve floor inside the room
            for (int y = roomY + 1; y < roomY + roomHeight + 1; ++y) {
                for (int x = roomX + 1; x < roomX + roomWidth + 1; ++x) {
                    if (isWithinBounds(x, y, width, height)) // Check bounds
                        level.tiles[y][x] = '.'; // Inner area is floor
                }
            }
            // Create walls around the floor (redundant with later pass, but safe)
            // for (int x = roomX; x < roomX + roomWidth + 2; ++x) {
            //     if (isWithinBounds(x, roomY, width, height)) level.tiles[roomY][x] = '#'; // Top wall
            //     if (isWithinBounds(x, roomY + roomHeight + 1, width, height)) level.tiles[roomY + roomHeight + 1][x] = '#'; // Bottom wall
            // }
            // for (int y = roomY; y < roomY + roomHeight + 2; ++y) {
            //     if (isWithinBounds(roomX, y, width, height)) level.tiles[y][roomX] = '#'; // Left wall
            //     if (isWithinBounds(roomX + roomWidth + 1, y, width, height)) level.tiles[y][roomX + roomWidth + 1] = '#'; // Right wall
            // }
        }
    }

    int numRooms = rooms.size();
    if (numRooms > 1) {
        // 4. Connect rooms with hallways (MST logic)
        std::vector<int> parent(numRooms);
        std::vector<int> key(numRooms, std::numeric_limits<int>::max());
        std::vector<bool> mstSet(numRooms, false);

        key[0] = 0;
        parent[0] = -1; // First node is root

        for (int count = 0; count < numRooms - 1; ++count) {
            int u = -1;
            int minKey = std::numeric_limits<int>::max(); // Find min key value vertex
            for (int v = 0; v < numRooms; ++v) {
                if (!mstSet[v] && key[v] < minKey) {
                    minKey = key[v];
                    u = v;
                }
            }

            if (u == -1) break; // No more reachable vertices

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


        // Carve hallways based on MST result
        for (int i = 1; i < numRooms; ++i) {
             if (parent[i] != -1) { // Ensure parent exists
                int room1Index = i;
                int room2Index = parent[i];

                int x1 = rooms[room1Index].x + rooms[room1Index].w / 2;
                int y1 = rooms[room1Index].y + rooms[room1Index].h / 2;
                int x2 = rooms[room2Index].x + rooms[room2Index].w / 2;
                int y2 = rooms[room2Index].y + rooms[room2Index].h / 2;

                // Carve horizontal then vertical (or vice versa)
                int currentX = x1;
                int currentY = y1;
                 // Ensure start/end points are valid before carving
                if (!isWithinBounds(x1,y1,width,height) || !isWithinBounds(x2,y2,width,height)) continue;

                while (currentX != x2) {
                     if (isWithinBounds(currentX, currentY, width, height)) {
                         level.tiles[currentY][currentX] = '.';
                     }
                    currentX += (currentX < x2) ? 1 : -1;
                }
                 while (currentY != y2) {
                     if (isWithinBounds(currentX, currentY, width, height)) {
                         level.tiles[currentY][currentX] = '.';
                     }
                    currentY += (currentY < y2) ? 1 : -1;
                }
                 // Ensure final tile is carved
                 if (isWithinBounds(currentX, currentY, width, height)) {
                     level.tiles[currentY][currentX] = '.';
                 }
             }
        }

    } // End if(numRooms > 1)

    // Add walls around all floor areas (final pass)
    std::vector<std::string> tempTiles = level.tiles; // Work on a copy
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (level.tiles[y][x] == 'V') { // If it's void
                // Check neighbors
                bool adjacentToFloor = false;
                int dx[] = {0, 0, 1, -1, 1, 1, -1, -1}; // Check 8 directions
                int dy[] = {1, -1, 0, 0, 1, -1, 1, -1};
                for (int i = 0; i < 8; ++i) {
                    int nx = x + dx[i];
                    int ny = y + dy[i];
                    if (isWithinBounds(nx, ny, width, height) && level.tiles[ny][nx] == '.') {
                        adjacentToFloor = true;
                        break;
                    }
                }
                if (adjacentToFloor) {
                    tempTiles[y][x] = '#'; // Turn void next to floor into a wall
                }
            }
        }
    }
    level.tiles = tempTiles; // Apply the changes


    level.rooms = rooms; // Store the room rectangles

    // Place start and end points within valid rooms
    if (!rooms.empty()) {
        std::uniform_int_distribution<> roomDist(0, rooms.size() - 1);
        int startRoomIndex = roomDist(gen);
        // Place start in a random valid floor tile within the start room
        int startAttempts = 0;
        do {
            level.startCol = rooms[startRoomIndex].x + 1 + (rand() % std::max(1, rooms[startRoomIndex].w - 2)); // Use max(1,..) to avoid modulo by zero/negative
            level.startRow = rooms[startRoomIndex].y + 1 + (rand() % std::max(1, rooms[startRoomIndex].h - 2));
            startAttempts++;
        } while ((!isWithinBounds(level.startCol, level.startRow, width, height) || level.tiles[level.startRow][level.startCol] != '.') && startAttempts < 100);


        if (rooms.size() > 1) {
            int endRoomIndex;
            do {
                endRoomIndex = roomDist(gen);
            } while (endRoomIndex == startRoomIndex); // Ensure start and end are in different rooms
             // Place end in a random valid floor tile within the end room
             int endAttempts = 0;
            do {
                level.endCol = rooms[endRoomIndex].x + 1 + (rand() % std::max(1, rooms[endRoomIndex].w - 2));
                level.endRow = rooms[endRoomIndex].y + 1 + (rand() % std::max(1, rooms[endRoomIndex].h - 2));
                 endAttempts++;
            } while ((!isWithinBounds(level.endCol, level.endRow, width, height) || level.tiles[level.endRow][level.endCol] != '.') && endAttempts < 100);

        } else { // Only one room, place end somewhere else in the same room
             int endAttempts = 0;
             do {
                level.endCol = rooms[startRoomIndex].x + 1 + (rand() % std::max(1, rooms[startRoomIndex].w - 2));
                level.endRow = rooms[startRoomIndex].y + 1 + (rand() % std::max(1, rooms[startRoomIndex].h - 2));
                 endAttempts++;
            } while (((level.endCol == level.startCol && level.endRow == level.startRow) || !isWithinBounds(level.endCol, level.endRow, width, height) || level.tiles[level.endRow][level.endCol] != '.') && endAttempts < 100);
        }
         // Handle cases where placement failed after attempts
         if (!isWithinBounds(level.startCol, level.startRow, width, height) || level.tiles[level.startRow][level.startCol] != '.') {
             SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to place start point in a valid floor tile!");
             // Fallback: place somewhere generic?
             level.startCol = width / 2; level.startRow = height / 2;
             if(isWithinBounds(level.startCol, level.startRow, width, height)) level.tiles[level.startRow][level.startCol] = '.';
         }
          if (!isWithinBounds(level.endCol, level.endRow, width, height) || level.tiles[level.endRow][level.endCol] != '.') {
             SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to place end point in a valid floor tile!");
              level.endCol = level.startCol + 1; level.endRow = level.startRow;
             if(isWithinBounds(level.endCol, level.endRow, width, height)) level.tiles[level.endRow][level.endCol] = '.';
         }


    } else {
        // Handle case with no rooms
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Level generated with no rooms!");
        level.startRow = height / 2;
        level.startCol = width / 2;
        level.endRow = height / 2;
        level.endCol = width / 2 + 1;
        if(isWithinBounds(level.startCol, level.startRow, width, height)) level.tiles[level.startRow][level.startCol] = '.';
        if(isWithinBounds(level.endCol, level.endRow, width, height)) level.tiles[level.endRow][level.endCol] = '.';
    }

        // --- *** NEW: Place Rune Pedestal *** ---
        if (!rooms.empty()) {
            int pedestalRoomIndex = rand() % rooms.size(); // Pick a random room
            SDL_Point pedestalPos = {-1, -1};
            int pedestalAttempts = 0;
            const int MAX_PEDESTAL_ATTEMPTS = 100; // Prevent infinite loop
    
            do {
                pedestalAttempts++;
                // Try placing in the center first, then randomly if center fails
                if (pedestalAttempts == 1) {
                    pedestalPos.x = rooms[pedestalRoomIndex].x + rooms[pedestalRoomIndex].w / 2;
                    pedestalPos.y = rooms[pedestalRoomIndex].y + rooms[pedestalRoomIndex].h / 2;
                } else {
                     // Random position within the room's floor area
                    pedestalPos.x = rooms[pedestalRoomIndex].x + 1 + (rand() % std::max(1, rooms[pedestalRoomIndex].w - 2));
                    pedestalPos.y = rooms[pedestalRoomIndex].y + 1 + (rand() % std::max(1, rooms[pedestalRoomIndex].h - 2));
                }
    
                // Check if the chosen spot is valid:
                // 1. Within bounds
                // 2. Is a floor tile '.'
                // 3. Not the player start tile
                // 4. Not the level end tile
                if (isWithinBounds(pedestalPos.x, pedestalPos.y, width, height) &&
                    level.tiles[pedestalPos.y][pedestalPos.x] == '.' &&
                    !(pedestalPos.y == level.startRow && pedestalPos.x == level.startCol) &&
                    !(pedestalPos.y == level.endRow && pedestalPos.x == level.endCol))
                {
                    // Valid spot found!
                    outPedestalPos = pedestalPos; // Assign to the output parameter
                    SDL_Log("INFO: Placed Rune Pedestal at [%d, %d].", pedestalPos.x, pedestalPos.y);
                    // Optional: Mark the tile differently? Or handle via GameData.currentPedestal presence.
                    // level.tiles[pedestalPos.y][pedestalPos.x] = 'P'; // Example marker
                    break; // Exit the placement loop
                } else {
                    pedestalPos = {-1,-1}; // Reset if invalid
                }
    
            } while (pedestalAttempts < MAX_PEDESTAL_ATTEMPTS);
    
            if (!outPedestalPos.has_value()) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to place Rune Pedestal after %d attempts!", MAX_PEDESTAL_ATTEMPTS);
                // Handle failure case - maybe try another room or skip placement for this level?
            }
        } else {
             SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot place pedestal - level has no rooms!");
        }
        // --- *** END Place Rune Pedestal *** ---


    // 5. Spawn enemies
    int numEnemiesToSpawn = 3 + level.rooms.size() / 2; // Example: Scale with number of rooms
    numEnemiesToSpawn = std::min(numEnemiesToSpawn, 12); // Cap at max enemy count (adjust as needed)

    int spawnedCount = 0;
    int spawnAttempts = 0;
    int maxSpawnAttemptsTotal = width * height; // Safety limit

    while(spawnedCount < numEnemiesToSpawn && spawnAttempts < maxSpawnAttemptsTotal) {
        spawnAttempts++;
        int spawnX = rand() % width;
        int spawnY = rand() % height;

        // Check if tile is floor and not start/end
        if (isWithinBounds(spawnX, spawnY, width, height) && // Check bounds before accessing tiles
            level.tiles[spawnY][spawnX] == '.' &&
            !(spawnY == level.startRow && spawnX == level.startCol) &&
            !(spawnY == level.endRow && spawnX == level.endCol))
        {
            // Check if already occupied by another enemy being placed this loop
            bool occupied = false;
            for(const auto& existingEnemy : enemies) {
                if (existingEnemy.x == spawnX && existingEnemy.y == spawnY) {
                    occupied = true;
                    break;
                }
            }

            if (!occupied) {
                // *** MODIFIED: Assign unique ID using static member ***
                int newId = Enemy::getNextId(); // Get next ID using the public static method
                enemies.emplace_back(newId, EnemyType::SLIME, spawnX, spawnY, tileW, tileH);
                // *****************************************************
                spawnedCount++;
            }
        }
    }
     if (spawnedCount < numEnemiesToSpawn) {
         SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could only spawn %d out of %d requested enemies.", spawnedCount, numEnemiesToSpawn);
     }

    return level;
}