#pragma once
#include "types.h"

struct World;
struct Player;
struct EnemyPool;
struct ItemPool;

struct Renderer {
    int termRows = 0;
    int termCols = 0;

    static constexpr int MAP_ORIG_X = 1;
    static constexpr int MAP_ORIG_Y = 1;
    static constexpr int UI_ORIG_X  = ROOM_W + 4;
    static constexpr int UI_ORIG_Y  = 1;

    static constexpr int MINI_CELL_W = 5;
    static constexpr int MINI_CELL_H = 3;

    void init();
    void shutdown();

    void draw(const World& world, const Player& player,
              const EnemyPool& enemies, const ItemPool& items,
              const char* message,
              const bool visited[MAX_ROOMS]);

    void drawFullMap(const World& world, const Player& player,
                     const bool visited[MAX_ROOMS]);

    void drawGameOver(bool won);

private:
    void drawRoom    (const World& w, int roomId);
    void drawEntities(const Player& p, const EnemyPool& e, const ItemPool& items, int roomId);
    void drawUI      (const Player& p, const EnemyPool& ep, const char* message, int roomId);
    void drawMiniMap (const World& w, const Player& p, const bool visited[MAX_ROOMS]);
};
