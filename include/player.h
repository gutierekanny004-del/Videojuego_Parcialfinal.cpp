#pragma once
#include "types.h"
#include "item.h"

struct Player {
    int  x      = ROOM_W / 2;
    int  y      = ROOM_H / 2;
    int  roomId = 4;          // starts in centre room (index 4 of 3x3 grid)
    int  hp     = 5;
    int  maxHp  = 5;

    Item* heldItem = nullptr; // single-slot inventory (pointer into ItemPool)

    bool isAlive()    const { return hp > 0; }
    bool hasItem()    const { return heldItem != nullptr; }
    bool hasKey()     const { return heldItem && heldItem->type == ItemType::Key; }

    void takeDamage(int amount);
    void heal(int amount);

    // Try to pick up item at current position; returns true on success
    bool tryPickup(Item* item);
    // Drop held item back into the world
    void dropItem(int roomId);
};
