#include "room.h"
#include <cstring>

void Room::setTile(int x, int y, Tile t) {
    if (x >= 0 && x < ROOM_W && y >= 0 && y < ROOM_H)
        tiles[y][x] = static_cast<char>(t);
}
void Room::placePillar(int x, int y) { setTile(x, y, Tile::Pillar); }
void Room::placeTrap  (int x, int y) { setTile(x, y, Tile::Trap);   }

void Room::build() {
    for (int y = 0; y < ROOM_H; ++y) {
        char* const row = tiles[y];
        const bool border = (y == 0 || y == ROOM_H - 1);
        for (int x = 0; x < ROOM_W; ++x)
            row[x] = static_cast<char>(
                border || x == 0 || x == ROOM_W - 1 ? Tile::Wall : Tile::Floor);
    }

    const char door = locked ? static_cast<char>(Tile::Locked)
                             : static_cast<char>(Tile::Floor);
    if (north >= 0) tiles[0]       [ROOM_W/2] = door;
    if (south >= 0) tiles[ROOM_H-1][ROOM_W/2] = door;
    if (east  >= 0) tiles[ROOM_H/2][ROOM_W-1] = door;
    if (west  >= 0) tiles[ROOM_H/2][0]        = door;

    if (hasExit)
        tiles[ROOM_H/2][ROOM_W/2] = static_cast<char>(Tile::Exit);
}

bool Room::isWalkable(int x, int y) const {
    if (x < 0 || x >= ROOM_W || y < 0 || y >= ROOM_H) return false;
    const char t = tiles[y][x];
    return t != static_cast<char>(Tile::Wall)
        && t != static_cast<char>(Tile::Locked)
        && t != static_cast<char>(Tile::Pillar)
        && t != static_cast<char>(Tile::Gate);
}

Dir Room::doorAt(int x, int y) const {
    if (y == 0        && north >= 0 && x == ROOM_W/2) return Dir::North;
    if (y == ROOM_H-1 && south >= 0 && x == ROOM_W/2) return Dir::South;
    if (x == ROOM_W-1 && east  >= 0 && y == ROOM_H/2) return Dir::East;
    if (x == 0        && west  >= 0 && y == ROOM_H/2) return Dir::West;
    return Dir::None;
}

Room* World::roomById(int id) {
    return (id >= 0 && id < MAX_ROOMS) ? &rooms[id] : nullptr;
}
const Room* World::roomById(int id) const {
    return (id >= 0 && id < MAX_ROOMS) ? &rooms[id] : nullptr;
}

TriggerPlate* World::addPlate(int pRoom, int px, int py,
                               int gRoom, int gx, int gy)
{
    if (plateCount >= MAX_PLATES) return nullptr;
    TriggerPlate* const pl = &plates[plateCount++];
    pl->plateRoomId = pRoom; pl->plateX = px; pl->plateY = py;
    pl->gateRoomId  = gRoom; pl->gateX  = gx; pl->gateY  = gy;
    pl->groupId   = -1;
    pl->exists    = true;
    pl->triggered = false;
    if (Room* r = roomById(pRoom)) r->setTile(px, py, Tile::Plate);
    if (Room* r = roomById(gRoom)) r->setTile(gx, gy, Tile::Gate);
    return pl;
}

int World::addGroup(int gRoom, int gx, int gy, int needed) {
    if (groupCount >= MAX_GROUPS) return -1;
    const int id = groupCount++;
    PuzzleGroup* const g = &groups[id];
    g->gateRoomId = gRoom; g->gateX = gx; g->gateY = gy;
    g->needed     = needed;
    g->triggered  = 0;
    g->opened     = false;
    g->exists     = true;
    if (Room* r = roomById(gRoom)) r->setTile(gx, gy, Tile::Gate);
    return id;
}

TriggerPlate* World::addGroupPlate(int groupId, int pRoom, int px, int py) {
    if (plateCount >= MAX_PLATES) return nullptr;
    TriggerPlate* const pl = &plates[plateCount++];
    pl->plateRoomId = pRoom; pl->plateX = px; pl->plateY = py;
    pl->gateRoomId  = -1;    pl->gateX  = 0;  pl->gateY  = 0;
    pl->groupId   = groupId;
    pl->exists    = true;
    pl->triggered = false;
    if (Room* r = roomById(pRoom)) r->setTile(px, py, Tile::Plate);
    return pl;
}

/*
 * Layout del dungeon (grilla 3x3):
 *
 *   [0 Tesoro] -- [1 Guardian] -- [2 Sellada/EXIT]
 *       |               |                |
 *   [3 Enigma] -- [4 Cruce]   -- [5 Trampas]
 *       |               |                |
 *   [6 Profunda] - [7 Sombras] - [8 Abismo]
 *
 * Puzzle A (cruzado, 2 placas -> puerta del Jefe en sala 6):
 *   placa sala 5 (14,10) + placa sala 7 (24,11)  ->  gate sala 6 (10,7)
 *
 * Puzzle B (doble placa, sala 3):
 *   placa (15,3) + placa (24,10)  ->  gate (8,7)
 *
 * Puzzle C (standalone, sala 8):
 *   placa (22,6)  ->  gate (5,6)
 */
void World::init() {
    for (int i = 0; i < MAX_ROOMS; ++i) {
        rooms[i].id = i;
        const int row = i / COLS, col = i % COLS;
        rooms[i].north = (row > 0)        ? i - COLS : -1;
        rooms[i].south = (row < ROWS - 1) ? i + COLS : -1;
        rooms[i].west  = (col > 0)        ? i - 1    : -1;
        rooms[i].east  = (col < COLS - 1) ? i + 1    : -1;
    }
    rooms[2].locked  = true;
    rooms[2].hasExit = true;

    const char* names[MAX_ROOMS] = {
        "Sala del Tesoro",     "Camara del Guardian",  "Camara Sellada",
        "Sala del Enigma",     "Cruce Central",        "Galeria de Trampas",
        "Mazmorra Profunda",   "Sala de las Sombras",  "El Abismo"
    };
    for (int i = 0; i < MAX_ROOMS; ++i)
        strncpy(rooms[i].name, names[i], sizeof(rooms[i].name)-1);

    for (int i = 0; i < MAX_ROOMS; ++i) rooms[i].build();

    // sala 5 tambien tiene la puerta norte bloqueada (la llave la abre en ambos lados)
    rooms[5].tiles[0][ROOM_W / 2] = static_cast<char>(Tile::Locked);

    // Sala 0 - anillos concéntricos de pilares con huecos desalineados
    {
        Room& r = rooms[0];
        for (int x = 4; x <= 23; ++x) {
            if (x != 14) r.placePillar(x, 3);
            if (x != 14) r.placePillar(x, 10);
        }
        for (int y = 4; y <= 9; ++y) {
            r.placePillar(4,  y);
            r.placePillar(23, y);
        }
        for (int x = 8; x <= 19; ++x) {
            r.placePillar(x, 5);
            r.placePillar(x, 8);
        }
        for (int y = 5; y <= 8; ++y) {
            r.placePillar(8,  y);
            r.placePillar(19, y);
        }
        // esquinas abiertas del anillo interno para forzar zigzag
        r.setTile(8,  5, Tile::Floor); r.setTile(19, 5, Tile::Floor);
        r.setTile(8,  8, Tile::Floor); r.setTile(19, 8, Tile::Floor);
        r.setTile(4,  6, Tile::Floor); r.setTile(4,  7, Tile::Floor);
        r.setTile(23, 6, Tile::Floor); r.setTile(23, 7, Tile::Floor);
    }

    // Sala 1 - dos muros con huecos opuestos forman un gauntlet en S
    {
        Room& r = rooms[1];
        for (int y = 1; y <= 12; ++y) {
            if (y < 4 || y > 5) r.placePillar(9, y);
        }
        for (int y = 1; y <= 12; ++y) {
            if (y < 7 || y > 8) r.placePillar(18, y);
        }
        r.placePillar(12,4); r.placePillar(13,4); r.placePillar(14,4); r.placePillar(15,4);
        r.placePillar(12,9); r.placePillar(13,9); r.placePillar(14,9); r.placePillar(15,9);
        r.placeTrap(11,3); r.placeTrap(16,3);
        r.placeTrap(11,10);r.placeTrap(16,10);
        r.placeTrap(13,6); r.placeTrap(15,6);
        r.placePillar(3,2); r.placePillar(3,11);
        r.placePillar(24,2);r.placePillar(24,11);
    }

    // Sala 2 - anillos alrededor de la X, foso de trampas externo
    {
        Room& r = rooms[2];
        for (int x = 6; x <= 21; ++x) {
            if (x != 13 && x != 14) r.placePillar(x, 3);
            if (x != 13 && x != 14) r.placePillar(x, 10);
        }
        for (int y = 4; y <= 9; ++y) {
            if (y != 6 && y != 7) r.placePillar(6,  y);
            if (y != 6 && y != 7) r.placePillar(21, y);
        }
        r.placePillar(10,5); r.placePillar(11,5); r.placePillar(17,5); r.placePillar(18,5);
        r.placePillar(10,8); r.placePillar(11,8); r.placePillar(17,8); r.placePillar(18,8);
        for (int y = 5; y <= 8; ++y) { r.placePillar(10,y); r.placePillar(18,y); }
        r.setTile(10,6, Tile::Floor); r.setTile(10,7, Tile::Floor);
        r.setTile(18,6, Tile::Floor); r.setTile(18,7, Tile::Floor);
        r.placeTrap(8,4);  r.placeTrap(8,6);  r.placeTrap(8,9);
        r.placeTrap(19,4); r.placeTrap(19,6); r.placeTrap(19,9);
        r.placeTrap(13,4); r.placeTrap(14,4);
        r.placeTrap(13,9); r.placeTrap(14,9);
    }

    // Sala 3 - Puzzle B: pared divisoria con puerta que necesita 2 placas
    {
        Room& r = rooms[3];
        for (int y = 1; y <= 12; ++y)
            if (y != 7) r.placePillar(8, y);
        r.placePillar(13,2); r.placePillar(19,2); r.placePillar(25,2);
        r.placePillar(13,5); r.placePillar(19,5);
        r.placePillar(13,9); r.placePillar(19,9); r.placePillar(25,9);
        r.placeTrap(11,7); r.placeTrap(12,7);
        r.placeTrap(22,4); r.placeTrap(22,8);
        r.placePillar(4,3); r.placePillar(4,10);
        r.placePillar(6,5); r.placePillar(6,9);
        const int grpB = addGroup(3, 8, 7, 2);
        addGroupPlate(grpB, 3, 15, 3);
        addGroupPlate(grpB, 3, 24, 10);
    }

    // Sala 4 - sala de inicio, canales en cruz obligan a elegir carril
    {
        Room& r = rooms[4];
        for (int x = 4; x <= 11; ++x) { r.placePillar(x,5); r.placePillar(x,8); }
        for (int x = 16; x <= 23; ++x){ r.placePillar(x,5); r.placePillar(x,8); }
        for (int y = 2; y <= 4; ++y)  { r.placePillar(6,y); r.placePillar(21,y); }
        for (int y = 9; y <= 11; ++y) { r.placePillar(6,y); r.placePillar(21,y); }
        r.setTile(14,5, Tile::Floor); r.setTile(14,8, Tile::Floor);
        r.setTile(6, 6, Tile::Floor); r.setTile(6, 7, Tile::Floor);
        r.setTile(21,6, Tile::Floor); r.setTile(21,7, Tile::Floor);
        r.placePillar(3,2);  r.placePillar(3,11);
        r.placePillar(24,2); r.placePillar(24,11);
    }

    // Sala 5 - galería de trampas, contiene placa 1 del Puzzle A
    {
        Room& r = rooms[5];
        for (int y = 2; y <= 11; ++y) {
            if (y != 6 && y != 7) r.placePillar(9,  y);
            if (y != 6 && y != 7) r.placePillar(18, y);
        }
        r.placeTrap(3,3);  r.placeTrap(5,3);  r.placeTrap(7,3);
        r.placeTrap(4,5);  r.placeTrap(6,5);
        r.placeTrap(3,8);  r.placeTrap(5,8);  r.placeTrap(7,8);
        r.placeTrap(4,10); r.placeTrap(6,10);
        r.placeTrap(20,3); r.placeTrap(22,3); r.placeTrap(24,3);
        r.placeTrap(21,5); r.placeTrap(23,5);
        r.placeTrap(20,8); r.placeTrap(22,8); r.placeTrap(24,8);
        r.placeTrap(21,10);r.placeTrap(23,10);
        r.placeTrap(11,4); r.placeTrap(16,4);
        r.placeTrap(11,9); r.placeTrap(16,9);
        r.placePillar(12,2); r.placePillar(15,2);
        r.placePillar(12,11);r.placePillar(15,11);
        // la placa se registra en el bloque de sala 6 junto al grupo
    }

    // Sala 6 - arena del Jefe, bloqueada por Puzzle A (2 placas en salas distintas)
    {
        Room& r = rooms[6];
        for (int y = 1; y <= 12; ++y)
            if (y != 7) r.placePillar(10, y);

        const int grpA = addGroup(6, 10, 7, 2);
        addGroupPlate(grpA, 5, 14, 10);
        addGroupPlate(grpA, 7, 24, 11);

        r.placePillar(3,2);  r.placePillar(7,2);
        r.placePillar(3,5);  r.placePillar(7,5);
        r.placePillar(3,9);  r.placePillar(7,9);
        r.placePillar(3,11); r.placePillar(7,11);
        r.placeTrap(4,3);   r.placeTrap(6,3);
        r.placeTrap(4,10);  r.placeTrap(6,10);
        r.placePillar(15,3); r.placePillar(24,3);
        r.placePillar(15,10);r.placePillar(24,10);
        r.placePillar(19,6); r.placePillar(19,8);
    }

    // Sala 7 - placa 2 del Puzzle A en la esquina sur, custodiada por cazadores
    {
        Room& r = rooms[7];
        for (int x = 2;  x <= 11; ++x) r.placePillar(x,  6);
        for (int x = 16; x <= 25; ++x) r.placePillar(x,  6);
        r.placePillar(5,3);  r.placePillar(10,3);
        r.placePillar(17,3); r.placePillar(22,3);
        r.placeTrap(20,9);  r.placeTrap(22,9);
        r.placeTrap(20,11); r.placeTrap(22,11);
        r.placePillar(5,9);  r.placePillar(8,9);
        r.placePillar(5,11); r.placePillar(8,11);
    }

    // Sala 8 - Puzzle C standalone, bolsillo con pocion al oeste de la puerta
    {
        Room& r = rooms[8];
        r.placePillar(5,2);  r.placePillar(14,2); r.placePillar(22,2);
        r.placePillar(5,11); r.placePillar(14,11);r.placePillar(22,11);
        r.placeTrap(8,3);  r.placeTrap(11,3); r.placeTrap(17,3); r.placeTrap(20,3);
        r.placeTrap(8,10); r.placeTrap(11,10);r.placeTrap(17,10);r.placeTrap(20,10);
        r.placeTrap(3,5);  r.placeTrap(3,8);
        r.placeTrap(24,5); r.placeTrap(24,8);
        r.placeTrap(10,5); r.placeTrap(14,5); r.placeTrap(18,5);
        r.placeTrap(10,8); r.placeTrap(14,8); r.placeTrap(18,8);
        r.placeTrap(7,6);  r.placeTrap(7,7);
        r.placeTrap(21,6); r.placeTrap(21,7);
        for (int y = 1; y <= 12; ++y)
            if (y != 6) r.placePillar(5, y);
        addPlate(8, 22, 6, 8, 5, 6);
    }
}
