#include "player.h"

void Player::takeDamage(int amount) {
    hp -= amount;
    if (hp < 0) hp = 0;
}

void Player::heal(int amount) {
    hp += amount;
    if (hp > maxHp) hp = maxHp;
}

bool Player::tryPickup(Item* item) {
    if (heldItem != nullptr) return false;   // inventory full
    if (!item || !item->active) return false;
    heldItem       = item;
    item->active   = false;   // remove from room
    item->roomId   = -1;
    return true;
}

void Player::dropItem(int roomId) {
    if (!heldItem) return;
    heldItem->x      = x;
    heldItem->y      = y;
    heldItem->roomId = roomId;
    heldItem->active = true;
    heldItem         = nullptr;
}
