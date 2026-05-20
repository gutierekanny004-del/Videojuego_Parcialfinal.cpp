#pragma once
#include "types.h"

enum class EnemyType { Chaser, Patrol, Boss };

struct Enemy {
    EnemyType type      = EnemyType::Chaser;
    int       x         = 0;
    int       y         = 0;
    int       roomId    = 0;
    bool      active    = true;
    bool      dropKey   = false;   // drops key on death

    int       hp        = 2;
    int       maxHp     = 2;

    int patrolX[4]  = {};
    int patrolY[4]  = {};
    int patrolIdx   = 0;
    int patrolTimer = 0;

    int moveDelay   = 2;
    int moveTick    = 0;

    char        glyph() const;
    const char* name()  const;

    // Returns true if it collided with (px,py) this tick
    bool update(int px, int py, int playerRoom,
                const char roomTiles[][ROOM_W]);
private:
    void moveChaser(int px, int py, const char tiles[][ROOM_W]);
    void movePatrol(int px, int py, const char tiles[][ROOM_W]);
    void moveBoss  (int px, int py, const char tiles[][ROOM_W]);
    bool tryMove   (int nx, int ny, const char tiles[][ROOM_W]);
};

struct EnemyPool {
    static constexpr int CAP = MAX_ENEMIES;
    Enemy enemies[CAP] = {};
    int   count        = 0;

    Enemy* spawn(EnemyType t, int roomId, int x, int y);
    void   setupPatrolWaypoints(Enemy* e,
               int x0, int y0, int x1, int y1,
               int x2, int y2, int x3, int y3);

    // Pointer to first active enemy in roomId matching (x,y), or nullptr
    Enemy* at(int roomId, int x, int y);
};
