#include "enemy.h"
#include <cmath>

char Enemy::glyph() const {
    switch (type) {
        case EnemyType::Chaser: return 'C';
        case EnemyType::Patrol: return 'P';
        case EnemyType::Boss:   return 'B';
    }
    return '?';
}

const char* Enemy::name() const {
    switch (type) {
        case EnemyType::Chaser: return "Cazador";
        case EnemyType::Patrol: return "Centinela";
        case EnemyType::Boss:   return "Jefe Oscuro";
    }
    return "???";
}

// tiles que un enemigo puede pisar (son inmunes a trampas)
static inline bool walkable(char t) {
    return t == static_cast<char>(Tile::Floor)
        || t == static_cast<char>(Tile::Exit)
        || t == static_cast<char>(Tile::Trap)
        || t == static_cast<char>(Tile::Plate)
        || t == static_cast<char>(Tile::PlateOn);
}

bool Enemy::tryMove(int nx, int ny, const char tiles[][ROOM_W]) {
    if (nx < 0 || nx >= ROOM_W || ny < 0 || ny >= ROOM_H) return false;
    if (!walkable(tiles[ny][nx])) return false;
    x = nx;
    y = ny;
    return true;
}

void Enemy::moveChaser(int px, int py, const char tiles[][ROOM_W]) {
    const int dx = px - x;
    const int dy = py - y;
    if (std::abs(dx) >= std::abs(dy)) {
        if (dx && tryMove(x + (dx > 0 ? 1 : -1), y, tiles)) return;
        if (dy)      tryMove(x, y + (dy > 0 ? 1 : -1), tiles);
    } else {
        if (dy && tryMove(x, y + (dy > 0 ? 1 : -1), tiles)) return;
        if (dx)      tryMove(x + (dx > 0 ? 1 : -1), y, tiles);
    }
}

void Enemy::movePatrol(int px, int py, const char tiles[][ROOM_W]) {
    if (std::abs(px - x) + std::abs(py - y) <= 5) {
        moveChaser(px, py, tiles);
        return;
    }
    const int tx = patrolX[patrolIdx];
    const int ty = patrolY[patrolIdx];
    if (x == tx && y == ty) {
        if (--patrolTimer <= 0) {
            patrolIdx   = (patrolIdx + 1) % 4;
            patrolTimer = 4;
        }
        return;
    }
    const int dx = tx - x;
    const int dy = ty - y;
    if (std::abs(dx) >= std::abs(dy)) {
        if (dx && tryMove(x + (dx > 0 ? 1 : -1), y, tiles)) return;
        if (dy)      tryMove(x, y + (dy > 0 ? 1 : -1), tiles);
    } else {
        if (dy && tryMove(x, y + (dy > 0 ? 1 : -1), tiles)) return;
        if (dx)      tryMove(x + (dx > 0 ? 1 : -1), y, tiles);
    }
}

void Enemy::moveBoss(int px, int py, const char tiles[][ROOM_W]) {
    moveChaser(px, py, tiles);
}

bool Enemy::update(int px, int py, int playerRoom, const char tiles[][ROOM_W]) {
    if (!active || roomId != playerRoom) return false;
    if (++moveTick < moveDelay) return false;
    moveTick = 0;

    switch (type) {
        case EnemyType::Chaser: moveChaser(px, py, tiles); break;
        case EnemyType::Patrol: movePatrol(px, py, tiles); break;
        case EnemyType::Boss:   moveBoss  (px, py, tiles); break;
    }
    return (x == px && y == py);
}

Enemy* EnemyPool::spawn(EnemyType t, int roomId, int x, int y) {
    if (count >= CAP) return nullptr;
    Enemy* const e = &enemies[count++];
    e->type        = t;
    e->roomId      = roomId;
    e->x           = x;
    e->y           = y;
    e->active      = true;
    e->dropKey     = false;
    e->patrolTimer = 4;
    e->patrolIdx   = 0;
    e->moveTick    = 0;

    switch (t) {
        case EnemyType::Chaser:
            e->hp = e->maxHp = 2; e->moveDelay = 2; break;
        case EnemyType::Patrol:
            e->hp = e->maxHp = 3; e->moveDelay = 3; break;
        case EnemyType::Boss:
            e->hp = e->maxHp = 6; e->moveDelay = 4; break;
    }
    return e;
}

void EnemyPool::setupPatrolWaypoints(Enemy* e,
    int x0,int y0, int x1,int y1,
    int x2,int y2, int x3,int y3)
{
    e->patrolX[0]=x0; e->patrolY[0]=y0;
    e->patrolX[1]=x1; e->patrolY[1]=y1;
    e->patrolX[2]=x2; e->patrolY[2]=y2;
    e->patrolX[3]=x3; e->patrolY[3]=y3;
}

Enemy* EnemyPool::at(int roomId, int ex, int ey) {
    Enemy* const end = enemies + count;
    for (Enemy* e = enemies; e != end; ++e)
        if (e->active && e->roomId == roomId && e->x == ex && e->y == ey)
            return e;
    return nullptr;
}
