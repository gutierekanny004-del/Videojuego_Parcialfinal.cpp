#pragma once
#include "types.h"
#include "item.h"

struct Player {
    int  x      = ROOM_W / 2;
    int  y      = ROOM_H / 2;
    int  roomId = 4;          // Comienza en la sala central
    int  hp     = 5;
    int  maxHp  = 5;

    Item* heldItem = nullptr; // Inventario de un solo slot

    bool isAlive()    const { return hp > 0; }
    bool hasItem()    const { return heldItem != nullptr; }
    bool hasKey()     const { return heldItem && heldItem->type == ItemType::Key; }

    void takeDamage(int amount);
    void heal(int amount);

    // Intenta recoger el item en la posicion (x,y) de la sala roomId
    bool tryPickup(Item* item);
    // Suelta el item en la posicion (x,y) de la sala roomId
    void dropItem(int roomId);
};
