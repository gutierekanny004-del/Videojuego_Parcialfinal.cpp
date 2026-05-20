#include "room.h"
#include <cstring>

// ─── Room helpers ─────────────────────────────────────────────────────────────

void Room::setTile(int x, int y, Tile t) {
    if (x >= 0 && x < ROOM_W && y >= 0 && y < ROOM_H)
        tiles[y][x] = static_cast<char>(t);
}
void Room::placePillar(int x, int y) { setTile(x, y, Tile::Pillar); }
void Room::placeTrap  (int x, int y) { setTile(x, y, Tile::Trap);   }

void Room::build() {
    // Fill walls, carve floor
    for (int y = 0; y < ROOM_H; ++y)
        for (int x = 0; x < ROOM_W; ++x)
            tiles[y][x] = static_cast<char>(y == 0 || y == ROOM_H-1 ||
                                            x == 0 || x == ROOM_W-1
                                            ? Tile::Wall : Tile::Floor);

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

// ─── World helpers ────────────────────────────────────────────────────────────

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
    // Place the tile in the room
    if (Room* r = roomById(pRoom)) r->setTile(px, py, Tile::Plate);
    // Place the gate tile
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
    // Place the gate tile
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

// ══════════════════════════════════════════════════════════════════════════════
//  World::init  — 3x3 grid layout:
//
//   [0 Tesoro] ── [1 Guardian] ── [2 Sellada/EXIT]
//       |               |                 |
//   [3 Enigma] ── [4 Cruce]  ── [5 Trampas]
//       |               |                 |
//   [6 Profunda] ─ [7 Sombras] ─ [8 Abismo]
//
//  Puzzle A (cross-room, needs 2 plates → boss gate):
//    Plate 1: Room 5 (14,10)  +  Plate 2: Room 7 (24,11)
//    → opens Gate at Room 6 (10,7)
//
//  Puzzle B (double-plate inside Room 3):
//    Plate 1: Room 3 (15,3)  +  Plate 2: Room 3 (24,10)
//    → opens Gate at Room 3 (8,7)
//
//  Puzzle C (standalone, Room 8):
//    Plate: Room 8 (22,6)  → Gate at Room 8 (6,6)  (bonus potion inside)
// ══════════════════════════════════════════════════════════════════════════════

void World::init() {
    // ── Connections ───────────────────────────────────────────────────────────
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

    // ── Names ────────────────────────────────────────────────────────────────
    const char* names[MAX_ROOMS] = {
        "Sala del Tesoro",      "Camara del Guardian",  "Camara Sellada",
        "Sala del Enigma",      "Cruce Central",        "Galeria de Trampas",
        "Mazmorra Profunda",    "Sala de las Sombras",  "El Abismo"
    };
    for (int i = 0; i < MAX_ROOMS; ++i)
        strncpy(rooms[i].name, names[i], sizeof(rooms[i].name)-1);

    // ── Base geometry ─────────────────────────────────────────────────────────
    for (int i = 0; i < MAX_ROOMS; ++i) rooms[i].build();

    // ══════════════════════════════════════════════════════════════════════════
    // ROOM 0 — Sala del Tesoro
    // Nested pillar maze: two concentric rings with offset gaps force zigzag
    // ══════════════════════════════════════════════════════════════════════════
    {
        Room& r = rooms[0];
        // Outer ring (with N/S/E/W gaps)
        for (int x = 4; x <= 23; ++x) {
            if (x != 14) r.placePillar(x, 3);
            if (x != 14) r.placePillar(x, 10);
        }
        for (int y = 4; y <= 9; ++y) {
            r.placePillar(4,  y);
            r.placePillar(23, y);
        }
        // Inner vault ring (offset gaps: enter from corners, not center)
        for (int x = 8; x <= 19; ++x) {
            if (x != 8 && x != 19) r.placePillar(x, 5);
            if (x != 8 && x != 19) r.placePillar(x, 8);
        }
        r.placePillar(8,  5); r.placePillar(8,  8);
        r.placePillar(19, 5); r.placePillar(19, 8);
        for (int y = 5; y <= 8; ++y) {
            r.placePillar(8,  y);
            r.placePillar(19, y);
        }
        // Vault gaps (corners only — forces player to go around inner ring)
        r.setTile(8,  5, Tile::Floor); r.setTile(19, 5, Tile::Floor);
        r.setTile(8,  8, Tile::Floor); r.setTile(19, 8, Tile::Floor);
        // Outer ring corridor passages
        r.setTile(4,  6, Tile::Floor); r.setTile(4,  7, Tile::Floor);
        r.setTile(23, 6, Tile::Floor); r.setTile(23, 7, Tile::Floor);
        // Sword in vault center
        // (spawned as Item in game.cpp)
    }

    // ══════════════════════════════════════════════════════════════════════════
    // ROOM 1 — Camara del Guardian
    // Two pillar walls create a gauntlet corridor; traps punish rushing
    // ══════════════════════════════════════════════════════════════════════════
    {
        Room& r = rooms[1];
        // West gauntlet wall: x=9, full height except y=4..5 (top passage) and y=8..9
        for (int y = 1; y <= 12; ++y) {
            if (y < 4 || y > 5) r.placePillar(9, y);
        }
        // East gauntlet wall: x=18, full height except y=7..8 (bottom passage)
        for (int y = 1; y <= 12; ++y) {
            if (y < 7 || y > 8) r.placePillar(18, y);
        }
        // Top bridge (x=10..17 at y=4): blocks direct shortcut through top gap
        r.placePillar(12, 4); r.placePillar(13, 4);
        r.placePillar(14, 4); r.placePillar(15, 4);
        // Bottom bridge (x=10..17 at y=9)
        r.placePillar(12, 9); r.placePillar(13, 9);
        r.placePillar(14, 9); r.placePillar(15, 9);
        // Spike traps in the central corridor (between the walls)
        r.placeTrap(11, 3);  r.placeTrap(16, 3);
        r.placeTrap(11, 10); r.placeTrap(16, 10);
        r.placeTrap(13, 6);  r.placeTrap(15, 6);
        // Corner cover pillars
        r.placePillar(3, 2); r.placePillar(3, 11);
        r.placePillar(24,2); r.placePillar(24,11);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // ROOM 2 — Camara Sellada  (EXIT, locked)
    // Concentric pillar rings surround exit; trap moat flanks approach
    // ══════════════════════════════════════════════════════════════════════════
    {
        Room& r = rooms[2];
        // Outer ring with 4 passages (N/S/E/W of ring)
        for (int x = 6; x <= 21; ++x) {
            if (x != 13 && x != 14) r.placePillar(x, 3);
            if (x != 13 && x != 14) r.placePillar(x, 10);
        }
        for (int y = 4; y <= 9; ++y) {
            if (y != 6 && y != 7) r.placePillar(6,  y);
            if (y != 6 && y != 7) r.placePillar(21, y);
        }
        // Inner ring — only 1-wide gap at each cardinal (forces navigation)
        r.placePillar(10,5); r.placePillar(11,5);
        r.placePillar(17,5); r.placePillar(18,5);
        r.placePillar(10,8); r.placePillar(11,8);
        r.placePillar(17,8); r.placePillar(18,8);
        for (int y = 5; y <= 8; ++y) r.placePillar(10,y), r.placePillar(18,y);
        // Inner ring passages: only the E passage (y=6) leads cleanly to exit
        r.setTile(10,6, Tile::Floor); r.setTile(10,7, Tile::Floor);
        r.setTile(18,6, Tile::Floor); r.setTile(18,7, Tile::Floor);
        // Trap moat: between outer and inner rings
        r.placeTrap(8,4);  r.placeTrap(8,6);  r.placeTrap(8,9);
        r.placeTrap(19,4); r.placeTrap(19,6); r.placeTrap(19,9);
        r.placeTrap(13,4); r.placeTrap(14,4);
        r.placeTrap(13,9); r.placeTrap(14,9);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // ROOM 3 — Sala del Enigma
    // PUZZLE B (2-plate): step on BOTH plates to open the gate
    //   Plate 1: (15,3) — near entrance, obvious
    //   Plate 2: (24,10) — hidden corner behind traps
    //   Gate: (8,7) — divides room; reward (potion) on left side
    // ══════════════════════════════════════════════════════════════════════════
    {
        Room& r = rooms[3];
        // Dividing pillar wall at x=8
        for (int y = 1; y <= 12; ++y)
            if (y != 7) r.placePillar(8, y);
        // Right-side maze: pillars guide player past both plates
        r.placePillar(13,2); r.placePillar(19,2); r.placePillar(25,2);
        r.placePillar(13,5); r.placePillar(19,5);
        r.placePillar(13,9); r.placePillar(19,9); r.placePillar(25,9);
        // Trap punishing the direct shortcut at y=7 (centre)
        r.placeTrap(11,7); r.placeTrap(12,7);
        r.placeTrap(22,4); r.placeTrap(22,8);
        // Left-side decorative pillars (visible as reward "bait")
        r.placePillar(4,3); r.placePillar(4,10);
        r.placePillar(6,5); r.placePillar(6,9);
        // Puzzle B (2-plate group)
        const int grpB = addGroup(3, 8, 7, 2);
        addGroupPlate(grpB, 3, 15, 3);
        addGroupPlate(grpB, 3, 24, 10);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // ROOM 4 — Cruce Central  (START)
    // Cross-shaped pillar channels force the player to choose a lane
    // ══════════════════════════════════════════════════════════════════════════
    {
        Room& r = rooms[4];
        // Horizontal pillar bar (y=5 and y=8) — creates N/S lanes
        for (int x = 4; x <= 11; ++x) { r.placePillar(x, 5); r.placePillar(x, 8); }
        for (int x = 16; x <= 23; ++x){ r.placePillar(x, 5); r.placePillar(x, 8); }
        // Vertical pillar bar (x=6 and x=21) — creates E/W lanes
        for (int y = 2; y <= 4; ++y) { r.placePillar(6, y); r.placePillar(21,y); }
        for (int y = 9; y <= 11; ++y){ r.placePillar(6, y); r.placePillar(21,y); }
        // Open the center cross (doors must stay reachable)
        r.setTile(14,5, Tile::Floor); r.setTile(14,8, Tile::Floor);
        r.setTile(6, 6, Tile::Floor); r.setTile(6, 7, Tile::Floor);
        r.setTile(21,6, Tile::Floor); r.setTile(21,7, Tile::Floor);
        // Corner accent pillars
        r.placePillar(3,2);  r.placePillar(3,11);
        r.placePillar(24,2); r.placePillar(24,11);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // ROOM 5 — Galeria de Trampas
    // PUZZLE A (cross-room, plate 1 of 2): Plate at (14,10)
    //   → opens Boss Gate in Room 6 (together with plate in Room 7)
    // Dense trap gauntlet with narrow safe corridors
    // ══════════════════════════════════════════════════════════════════════════
    {
        Room& r = rooms[5];
        // Pillar barriers create 3 lanes
        for (int y = 2; y <= 11; ++y) {
            if (y != 6 && y != 7) r.placePillar(9,  y);
            if (y != 6 && y != 7) r.placePillar(18, y);
        }
        // Left lane (x=2..8): spike gauntlet
        r.placeTrap(3,3);  r.placeTrap(5,3);  r.placeTrap(7,3);
        r.placeTrap(4,5);  r.placeTrap(6,5);
        r.placeTrap(3,8);  r.placeTrap(5,8);  r.placeTrap(7,8);
        r.placeTrap(4,10); r.placeTrap(6,10);
        // Right lane (x=19..26): more traps
        r.placeTrap(20,3); r.placeTrap(22,3); r.placeTrap(24,3);
        r.placeTrap(21,5); r.placeTrap(23,5);
        r.placeTrap(20,8); r.placeTrap(22,8); r.placeTrap(24,8);
        r.placeTrap(21,10);r.placeTrap(23,10);
        // Centre lane (x=10..17): narrower but safer — plate at south end
        r.placeTrap(11,4); r.placeTrap(16,4);
        r.placeTrap(11,9); r.placeTrap(16,9);
        // Cover pillars at lane junctions
        r.placePillar(12,2); r.placePillar(15,2);
        r.placePillar(12,11);r.placePillar(15,11);
        // Cross-room puzzle A plate 1 — placed AFTER traps (dangerous to reach)
        // (addGroup called in Room 6 setup; plate added here referencing that group)
    }

    // ══════════════════════════════════════════════════════════════════════════
    // ROOM 6 — Mazmorra Profunda  (Boss arena)
    // PUZZLE A gate here; left arena accessed only after both cross-room plates
    // Boss arena: pillar ring creates fighting space
    // ══════════════════════════════════════════════════════════════════════════
    {
        Room& r = rooms[6];
        // Dividing wall at x=10 (same as before)
        for (int y = 1; y <= 12; ++y)
            if (y != 7) r.placePillar(10, y);
        // PUZZLE A: cross-room group (2 plates needed)
        const int grpA = addGroup(6, 10, 7, 2);
        // Plate 1 in Room 5
        addGroupPlate(grpA, 5, 14, 10);
        // Plate 2 in Room 7
        addGroupPlate(grpA, 7, 24, 11);

        // Left arena: pillar ring creates enclosed combat space
        r.placePillar(3,2); r.placePillar(7,2);
        r.placePillar(3,5);                       r.placePillar(7,5);
        r.placePillar(3,9);                       r.placePillar(7,9);
        r.placePillar(3,11);r.placePillar(7,11);
        // Trap moat inside the arena entrance (punishes rushing in)
        r.placeTrap(4,3);   r.placeTrap(6,3);
        r.placeTrap(4,10);  r.placeTrap(6,10);
        // Right side decorative pillars
        r.placePillar(15,3); r.placePillar(24,3);
        r.placePillar(15,10);r.placePillar(24,10);
        r.placePillar(19,6); r.placePillar(19,8);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // ROOM 7 — Sala de las Sombras
    // PUZZLE A (cross-room, plate 2 of 2): Plate at (24,11) — guarded by Chasers
    // Horizontal pillar wall splits room N/S; enemies guard the south plate
    // ══════════════════════════════════════════════════════════════════════════
    {
        Room& r = rooms[7];
        // Horizontal dividing wall at y=6 (gap at x=14 for door alignment)
        for (int x = 2; x <= 11; ++x) r.placePillar(x, 6);
        for (int x = 16; x <= 25; ++x)r.placePillar(x, 6);
        // N half: pillar clusters for cover
        r.placePillar(5,3);  r.placePillar(10,3);
        r.placePillar(17,3); r.placePillar(22,3);
        // S half: traps around the plate to punish careless approach
        r.placeTrap(20,9);  r.placeTrap(22,9);
        r.placeTrap(20,11); r.placeTrap(22,11);
        // S half: extra pillar clusters
        r.placePillar(5,9);  r.placePillar(8,9);
        r.placePillar(5,11); r.placePillar(8,11);
        // Note: Plate (24,11) is added via addGroupPlate in Room 6 setup above
    }

    // ══════════════════════════════════════════════════════════════════════════
    // ROOM 8 — El Abismo
    // PUZZLE C (standalone): Plate at (22,6) opens Gate at (6,6)
    //   Left pocket holds bonus potion
    // Concentric trap rings — hardest navigation in the dungeon
    // ══════════════════════════════════════════════════════════════════════════
    {
        Room& r = rooms[8];
        // Outer pillar ring
        r.placePillar(5,2);  r.placePillar(14,2); r.placePillar(22,2);
        r.placePillar(5,11); r.placePillar(14,11);r.placePillar(22,11);
        // Outer trap ring
        r.placeTrap(8,3);  r.placeTrap(11,3); r.placeTrap(17,3); r.placeTrap(20,3);
        r.placeTrap(8,10); r.placeTrap(11,10);r.placeTrap(17,10);r.placeTrap(20,10);
        r.placeTrap(3,5);  r.placeTrap(3,8);
        r.placeTrap(24,5); r.placeTrap(24,8);
        // Inner trap ring (tighter)
        r.placeTrap(10,5); r.placeTrap(14,5); r.placeTrap(18,5);
        r.placeTrap(10,8); r.placeTrap(14,8); r.placeTrap(18,8);
        r.placeTrap(7,6);  r.placeTrap(7,7);
        r.placeTrap(21,6); r.placeTrap(21,7);
        // Left pocket wall: x=5 pillar column (gate at y=6)
        for (int y = 1; y <= 12; ++y)
            if (y != 6) r.placePillar(5, y);
        // Standalone Puzzle C
        addPlate(8, 22, 6, 8, 5, 6);
        // Note: bonus potion at (3,6) spawned in game.cpp
    }
}
