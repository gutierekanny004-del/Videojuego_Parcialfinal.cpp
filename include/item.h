#pragma once
#include "types.h"

enum class ItemType { None, Key, Sword, Potion };

struct Item {
    ItemType type   = ItemType::None;
    int      roomId = -1;   // Cada item esta en una sala
    int      x      = 0;
    int      y      = 0;
    bool     active = false;

    char     glyph()  const;
    const char* name() const;
};

// Pool de items Estatico
struct ItemPool {
    static constexpr int CAP = MAX_ITEMS;
    Item items[CAP] = {};
    int  count      = 0;

    Item* spawn(ItemType t, int roomId, int x, int y);
    // Puntero al item en la posicion (x,y) de la sala roomId; nullptr si no hay
    Item* at(int roomId, int x, int y);
};
