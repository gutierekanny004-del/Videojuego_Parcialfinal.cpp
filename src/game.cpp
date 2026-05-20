#include "game.h"

#ifdef _WIN32
#  include <curses.h>
#else
#  include <ncurses.h>
#endif

#include <cstdio>
#include <cstdarg>
#include <cstring>

// ─── init ────────────────────────────────────────────────────────────────────

void Game::init() {
    renderer.init();
    world.init();

    player.roomId = 4;
    player.x      = ROOM_W / 2;
    player.y      = ROOM_H / 2;
    markVisited();

    // ── Items ─────────────────────────────────────────────────────────────────
    // Sword    : Room 0 vault center — rewards thorough exploration
    items.spawn(ItemType::Sword,  0, 14, 6);
    // Potion 1 : Room 7 north half — guarded by first Chaser
    items.spawn(ItemType::Potion, 7, 5, 4);
    // Potion 2 : Room 3 left side (behind Puzzle B gate)
    items.spawn(ItemType::Potion, 3, 4,  7);
    // Potion 3 : Room 8 left pocket (behind Puzzle C gate)
    items.spawn(ItemType::Potion, 8, 3,  6);

    // ── Enemies ───────────────────────────────────────────────────────────────
    // Room 1 — two Cazadores, one each side of the gauntlet
    enemies.spawn(EnemyType::Chaser, 1, 6,  6);
    enemies.spawn(EnemyType::Chaser, 1, 22, 6);

    // Room 3 — Centinela patrols the right maze
    Enemy* const p3 = enemies.spawn(EnemyType::Patrol, 3, 14, 6);
    enemies.setupPatrolWaypoints(p3, 10,2, 24,2, 24,11, 10,11);

    // Room 5 — Cazador guards the cross-room plate
    enemies.spawn(EnemyType::Chaser, 5, 14, 8);

    // Room 7 — two Cazadores guard the south plate (Puzzle A)
    enemies.spawn(EnemyType::Chaser, 7, 20, 10);
    enemies.spawn(EnemyType::Chaser, 7, 12,  9);

    // Room 8 — heavy Patrol in the concentric trap rings
    Enemy* const p8 = enemies.spawn(EnemyType::Patrol, 8, 14, 6);
    enemies.setupPatrolWaypoints(p8, 8,4, 20,4, 20,9, 8,9);
    Enemy* const p8b = enemies.spawn(EnemyType::Patrol, 8, 14, 7);
    enemies.setupPatrolWaypoints(p8b, 9,5, 19,5, 19,8, 9,8);

    // Room 6 — JEFE OSCURO guards the key inside the sealed arena
    Enemy* const boss = enemies.spawn(EnemyType::Boss, 6, 5, 7);
    boss->dropKey = true;

    setMessage("Dos placas abren la puerta del Jefe. Explora!");
}

// ─── markVisited ─────────────────────────────────────────────────────────────

void Game::markVisited() {
    if (player.roomId >= 0 && player.roomId < MAX_ROOMS)
        visited[player.roomId] = true;
}

// ─── setMessage ──────────────────────────────────────────────────────────────

void Game::setMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
}

// ─── handleInput ─────────────────────────────────────────────────────────────

void Game::handleInput(int key) {
    switch (key) {
        case 'w': case KEY_UP:    tryMove( 0, -1); break;
        case 's': case KEY_DOWN:  tryMove( 0,  1); break;
        case 'a': case KEY_LEFT:  tryMove(-1,  0); break;
        case 'd': case KEY_RIGHT: tryMove( 1,  0); break;
        case 'f': case ' ':       tryPickupOrDrop(); break;
        case 'm': case 'M':       showMap = !showMap; break;
        case 'q': case 'Q':       state = GameState::Lost; break;
        default: break;
    }
}

// ─── tryMove ─────────────────────────────────────────────────────────────────

void Game::tryMove(int dx, int dy) {
    const int nx = player.x + dx;
    const int ny = player.y + dy;

    Room* const room = world.roomById(player.roomId);
    if (!room) return;

    // ── Bump-attack ───────────────────────────────────────────────────────────
    {
        Enemy* const target = enemies.at(player.roomId, nx, ny);
        if (target) {
            const int dmg = (player.heldItem &&
                             player.heldItem->type == ItemType::Sword) ? 2 : 1;
            target->hp -= dmg;
            if (target->hp <= 0) {
                target->active = false;
                if (target->dropKey) {
                    items.spawn(ItemType::Key, target->roomId,
                                target->x, target->y);
                    setMessage("Jefe derrotado! La LLAVE cae al suelo!");
                } else {
                    setMessage("Derrotaste al %s!", target->name());
                }
            } else {
                setMessage("Golpeas al %s! (HP:%d/%d)",
                           target->name(), target->hp, target->maxHp);
            }
            return;
        }
    }

    // ── Locked door ───────────────────────────────────────────────────────────
    if (nx >= 0 && nx < ROOM_W && ny >= 0 && ny < ROOM_H) {
        if (room->tiles[ny][nx] == static_cast<char>(Tile::Locked)) {
            if (player.hasKey()) {
                room->tiles[ny][nx] = static_cast<char>(Tile::Floor);
                room->locked = false;
                Room* const r2 = world.roomById(2);
                if (r2) {
                    r2->locked = false;
                    r2->tiles[ROOM_H-1][ROOM_W/2] =
                        static_cast<char>(Tile::Floor);
                }
                setMessage("Puerta desbloqueada con la llave!");
            } else {
                setMessage("Cerrada! Derrota al Jefe para la llave.");
                return;
            }
        }
    }

    if (!room->isWalkable(nx, ny)) return;

    player.x = nx;
    player.y = ny;

    checkRoomTransition();
    checkTrap();
    checkPlate();
}

// ─── checkTrap ───────────────────────────────────────────────────────────────

void Game::checkTrap() {
    Room* const room = world.roomById(player.roomId);
    if (!room) return;
    if (room->tiles[player.y][player.x] == static_cast<char>(Tile::Trap)) {
        player.takeDamage(1);
        setMessage("Trampa de puas! -1 HP (%d/%d)", player.hp, player.maxHp);
    }
}

// ─── checkPlate ──────────────────────────────────────────────────────────────

void Game::checkPlate() {
    Room* const room = world.roomById(player.roomId);
    if (!room) return;
    if (room->tiles[player.y][player.x] != static_cast<char>(Tile::Plate))
        return;

    TriggerPlate* const pEnd = world.plates + world.plateCount;
    for (TriggerPlate* pl = world.plates; pl != pEnd; ++pl) {
        if (!pl->exists || pl->triggered)        continue;
        if (pl->plateRoomId != player.roomId)    continue;
        if (pl->plateX != player.x ||
            pl->plateY != player.y)              continue;

        pl->triggered = true;
        // Visually activate the plate tile
        Room* const pr = world.roomById(pl->plateRoomId);
        if (pr) pr->tiles[pl->plateY][pl->plateX] =
                    static_cast<char>(Tile::PlateOn);

        if (pl->groupId >= 0) {
            // ── Multi-plate group ─────────────────────────────────────────────
            PuzzleGroup* const grp = &world.groups[pl->groupId];
            grp->triggered++;
            if (grp->triggered >= grp->needed && !grp->opened) {
                grp->opened = true;
                Room* const gr = world.roomById(grp->gateRoomId);
                if (gr) gr->tiles[grp->gateY][grp->gateX] =
                            static_cast<char>(Tile::Floor);
                if (grp->gateRoomId == player.roomId)
                    setMessage("*BOOM* La puerta secreta se abre!");
                else
                    setMessage("*BOOM* Una puerta LEJANA se abre...");
            } else if (!grp->opened) {
                const int remaining = grp->needed - grp->triggered;
                setMessage("*CLIC* Faltan %d placa(s) mas!", remaining);
            }
        } else {
            // ── Standalone plate ──────────────────────────────────────────────
            Room* const gr = world.roomById(pl->gateRoomId);
            if (gr) gr->tiles[pl->gateY][pl->gateX] =
                        static_cast<char>(Tile::Floor);
            setMessage("*CLIC* Puerta secreta abierta!");
        }
        break;
    }
}

// ─── tryPickupOrDrop ─────────────────────────────────────────────────────────

void Game::tryPickupOrDrop() {
    if (player.hasItem()) {
        player.dropItem(player.roomId);
        setMessage("Item soltado.");
        return;
    }
    Item* const iEnd = items.items + items.count;
    for (Item* it = items.items; it != iEnd; ++it) {
        if (!it->active || it->roomId != player.roomId) continue;
        if (it->x != player.x  || it->y != player.y)   continue;
        if (player.tryPickup(it)) {
            if (it->type == ItemType::Potion) {
                player.heal(2);
                player.heldItem->active = false;
                player.heldItem = nullptr;
                setMessage("Pocion! +2 HP (%d/%d)", player.hp, player.maxHp);
            } else {
                setMessage("Recogiste: %s", it->name());
            }
            return;
        }
    }
    setMessage("No hay nada aqui.");
}

// ─── checkRoomTransition ─────────────────────────────────────────────────────

void Game::checkRoomTransition() {
    Room* const room = world.roomById(player.roomId);
    if (!room) return;

    bool moved = false;
    if      (player.y == 0        && room->north >= 0)
        { player.roomId = room->north; player.y = ROOM_H - 2; moved = true; }
    else if (player.y == ROOM_H-1 && room->south >= 0)
        { player.roomId = room->south; player.y = 1;           moved = true; }
    else if (player.x == ROOM_W-1 && room->east  >= 0)
        { player.roomId = room->east;  player.x = 1;           moved = true; }
    else if (player.x == 0        && room->west  >= 0)
        { player.roomId = room->west;  player.x = ROOM_W - 2;  moved = true; }

    if (moved) {
        markVisited();
        const Room* const nr = world.roomById(player.roomId);
        if (nr) setMessage("Entraste: %s", nr->name);
    }
}

// ─── checkWinLose ────────────────────────────────────────────────────────────

void Game::checkWinLose() {
    if (!player.isAlive()) { state = GameState::Lost; return; }
    const Room* const room = world.roomById(player.roomId);
    if (room && room->hasExit &&
        room->tiles[player.y][player.x] == static_cast<char>(Tile::Exit))
        state = GameState::Won;
}

// ─── update ──────────────────────────────────────────────────────────────────

void Game::update() {
    const Room* const room = world.roomById(player.roomId);
    if (!room) return;
    Enemy* const eEnd = enemies.enemies + enemies.count;
    for (Enemy* e = enemies.enemies; e != eEnd; ++e) {
        if (!e->active) continue;
        if (e->update(player.x, player.y, player.roomId, room->tiles)) {
            const int dmg = (e->type == EnemyType::Boss) ? 2 : 1;
            player.takeDamage(dmg);
            setMessage("%s te golpea! HP:%d/%d",
                       e->name(), player.hp, player.maxHp);
        }
    }
}

// ─── run ─────────────────────────────────────────────────────────────────────

void Game::run() {
    while (state == GameState::Playing) {
        if (showMap) {
            renderer.drawFullMap(world, player, visited);
            if (getch() == 'm' || getch() == 'M') showMap = false;
            showMap = false;
            continue;
        }
        renderer.draw(world, player, enemies, items, message, visited);
        handleInput(getch());
        if (state == GameState::Playing) { update(); checkWinLose(); }
        ++tickCount;
    }
    renderer.drawGameOver(state == GameState::Won);
    renderer.shutdown();
}
