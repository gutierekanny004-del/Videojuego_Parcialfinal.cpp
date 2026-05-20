#pragma once
#include "types.h"

// ─── Pressure plate ───────────────────────────────────────────────────────────
// A plate can live in a different room than the gate it controls.
// groupId >= 0  → belongs to a PuzzleGroup (all plates in group must trigger)
// groupId == -1 → standalone: directly opens gate on trigger
struct TriggerPlate {
    int  plateX     = 0,  plateY     = 0,  plateRoomId = -1;
    int  gateX      = 0,  gateY      = 0,  gateRoomId  = -1;
    int  groupId    = -1;   // -1 = standalone
    bool exists     = false;
    bool triggered  = false;
};

// ─── Puzzle group ─────────────────────────────────────────────────────────────
// All 'needed' plates must be triggered before the gate opens.
struct PuzzleGroup {
    int  gateX      = 0,  gateY      = 0,  gateRoomId  = -1;
    int  needed     = 1;
    int  triggered  = 0;
    bool opened     = false;
    bool exists     = false;
};

// ─── Room ─────────────────────────────────────────────────────────────────────
struct Room {
    int  id       = -1;
    bool locked   = false;
    bool hasExit  = false;

    char name[40] = "Habitacion desconocida";

    int north = -1, south = -1, east = -1, west = -1;

    char tiles[ROOM_H][ROOM_W] = {};

    void build();
    bool isWalkable(int x, int y) const;
    Dir  doorAt   (int x, int y) const;

    void setTile    (int x, int y, Tile t);
    void placePillar(int x, int y);
    void placeTrap  (int x, int y);
};

// ─── World ───────────────────────────────────────────────────────────────────
struct World {
    static constexpr int COLS       = 3;
    static constexpr int ROWS       = 3;
    static constexpr int MAX_PLATES = 16;
    static constexpr int MAX_GROUPS = 8;

    Room         rooms [MAX_ROOMS];
    TriggerPlate plates[MAX_PLATES];
    PuzzleGroup  groups[MAX_GROUPS];
    int          plateCount = 0;
    int          groupCount = 0;

    void init();

    Room*       roomById(int id);
    const Room* roomById(int id) const;

    // Register a standalone plate (single plate → single gate in any room)
    TriggerPlate* addPlate(int pRoom, int px, int py,
                            int gRoom, int gx, int gy);

    // Register a group; returns groupId
    int addGroup(int gRoom, int gx, int gy, int needed);

    // Register a plate that is part of a group
    TriggerPlate* addGroupPlate(int groupId, int pRoom, int px, int py);
};
