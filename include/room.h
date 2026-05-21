#pragma once
#include "types.h"

// placa que puede estar en una sala distinta a la puerta que controla
// groupId >= 0  -> pertenece a un PuzzleGroup (todas las placas deben pisarse)
// groupId == -1 -> standalone, abre directamente su puerta al pisarse
struct TriggerPlate {
    int  plateX     = 0,  plateY     = 0,  plateRoomId = -1;
    int  gateX      = 0,  gateY      = 0,  gateRoomId  = -1;
    int  groupId    = -1;
    bool exists     = false;
    bool triggered  = false;
};

// todas las placas del grupo deben activarse antes de que abra la puerta
struct PuzzleGroup {
    int  gateX      = 0,  gateY      = 0,  gateRoomId  = -1;
    int  needed     = 1;
    int  triggered  = 0;
    bool opened     = false;
    bool exists     = false;
};

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

    // placa standalone: una placa -> una puerta en cualquier sala
    TriggerPlate* addPlate(int pRoom, int px, int py,
                            int gRoom, int gx, int gy);

    // crea un grupo; devuelve el groupId
    int addGroup(int gRoom, int gx, int gy, int needed);

    // agrega una placa al grupo indicado
    TriggerPlate* addGroupPlate(int groupId, int pRoom, int px, int py);
};
