#pragma once

constexpr int ROOM_W      = 28;
constexpr int ROOM_H      = 14;
constexpr int MAX_ENEMIES = 16;
constexpr int MAX_ITEMS   = 16;
constexpr int MAX_ROOMS   = 9;

enum class Tile : char {
    Floor   = '.',
    Wall    = '#',
    DoorN   = '^',
    DoorS   = 'v',
    DoorE   = '>',
    DoorW   = '<',
    Locked  = '+',
    Exit    = 'X',
    Pillar  = '%',
    Trap    = '~',
    Plate   = 'o',
    PlateOn = 'O',
    Gate    = '!'
};

enum class Dir { None, North, South, East, West };

enum class GameState { Playing, Won, Lost };

namespace Color {
    constexpr int Wall      = 1;
    constexpr int Floor     = 2;
    constexpr int Player    = 3;
    constexpr int Enemy1    = 4;
    constexpr int Enemy2    = 5;
    constexpr int Item      = 6;
    constexpr int UI        = 7;
    constexpr int Door      = 8;
    constexpr int MiniVisit = 9;
    constexpr int MiniCur   = 10;
    constexpr int MiniHide  = 11;
    constexpr int FloorDark = 12;
    constexpr int ExitGlow  = 13;
    constexpr int Pillar    = 14;
    constexpr int Trap      = 15;
    constexpr int Plate     = 16;
    constexpr int Gate      = 17;
    constexpr int Boss      = 18;
}
