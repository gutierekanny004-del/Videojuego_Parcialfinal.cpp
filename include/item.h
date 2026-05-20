#pragma once
#include "types.h"

enum class ItemType { None, Key, Sword, Potion };

struct Item {
    ItemType type   = ItemType::None;
    int      roomId = -1;   // which room this item is in (-1 = carried/gone)
    int      x      = 0;
    int      y      = 0;
    bool     active = false;

    char     glyph()  const;
    const char* name() const;
};

// Static pool — no heap allocation allowed
struct ItemPool {
    static constexpr int CAP = MAX_ITEMS;
    Item items[CAP] = {};
    int  count      = 0;

    Item* spawn(ItemType t, int roomId, int x, int y);
    // Returns pointer into static array — valid for program lifetime
};
