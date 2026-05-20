#pragma once
#include "types.h"
#include "player.h"
#include "enemy.h"
#include "item.h"
#include "room.h"
#include "renderer.h"

struct Game {
    World      world;
    Player     player;
    EnemyPool  enemies;
    ItemPool   items;
    Renderer   renderer;
    GameState  state     = GameState::Playing;
    int        tickCount = 0;

    bool visited[MAX_ROOMS] = {};
    bool showMap = false;

    char message[64] = "Bienvenido al dungeon!";

    void init();
    void run();

private:
    void handleInput(int key);
    void update();
    void tryMove(int dx, int dy);
    void tryPickupOrDrop();
    void checkRoomTransition();
    void checkWinLose();
    void checkTrap();
    void checkPlate();
    void setMessage(const char* fmt, ...);
    void markVisited();
};
