#include "item.h"
#include <cstring>

char Item::glyph() const {
    switch (type) {
        case ItemType::Key:    return 'k';
        case ItemType::Sword:  return 's';
        case ItemType::Potion: return 'p';
        default:               return '?';
    }
}

const char* Item::name() const {
    switch (type) {
        case ItemType::Key:    return "Key";
        case ItemType::Sword:  return "Sword";
        case ItemType::Potion: return "Potion";
        default:               return "???";
    }
}

Item* ItemPool::spawn(ItemType t, int roomId, int x, int y) {
    if (count >= CAP) return nullptr;
    Item* const it = &items[count++];
    it->type   = t;
    it->roomId = roomId;
    it->x      = x;
    it->y      = y;
    it->active = true;
    return it;
}

Item* ItemPool::at(int roomId, int x, int y) {
    Item* const end = items + count;
    for (Item* it = items; it != end; ++it)
        if (it->active && it->roomId == roomId && it->x == x && it->y == y)
            return it;
    return nullptr;
}
