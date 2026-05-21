#include "renderer.h"
#include "room.h"
#include "player.h"
#include "enemy.h"
#include "item.h"

#ifdef _WIN32
#  include <curses.h>
#else
#  include <ncurses.h>
#endif

#include <cstring>
#include <cstdio>
#include <cstdarg>

void Renderer::init() {
    if (!stdscr) initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(Color::Wall,      COLOR_WHITE,   COLOR_WHITE);
        init_pair(Color::Floor,     COLOR_BLACK,   -1);
        init_pair(Color::Player,    COLOR_CYAN,    -1);
        init_pair(Color::Enemy1,    COLOR_RED,     -1);
        init_pair(Color::Enemy2,    COLOR_MAGENTA, -1);
        init_pair(Color::Item,      COLOR_YELLOW,  -1);
        init_pair(Color::UI,        COLOR_WHITE,   -1);
        init_pair(Color::Door,      COLOR_GREEN,   -1);
        init_pair(Color::MiniVisit, COLOR_GREEN,   COLOR_BLACK);
        init_pair(Color::MiniCur,   COLOR_BLACK,   COLOR_CYAN);
        init_pair(Color::MiniHide,  COLOR_BLACK,   COLOR_BLACK);
        init_pair(Color::FloorDark, COLOR_CYAN,    COLOR_BLACK);
        init_pair(Color::ExitGlow,  COLOR_WHITE,   COLOR_RED);
        init_pair(Color::Pillar,    COLOR_BLACK,   COLOR_WHITE);
        init_pair(Color::Trap,      COLOR_RED,     COLOR_BLACK);
        init_pair(Color::Plate,     COLOR_YELLOW,  COLOR_BLACK);
        init_pair(Color::Gate,      COLOR_GREEN,   COLOR_BLACK);
        init_pair(Color::Boss,      COLOR_BLACK,   COLOR_RED);
    }
    getmaxyx(stdscr, termRows, termCols);
}

void Renderer::shutdown() { endwin(); }

// wrappers que no dibujan si la coordenada cae fuera del terminal
void Renderer::safe_mvaddch(int y, int x, chtype ch) {
    if (x >= 0 && x < termCols && y >= 0 && y < termRows)
        mvaddch(y, x, ch);
}

void Renderer::safe_mvprintw(int y, int x, const char* fmt, ...) {
    if (y < 0 || y >= termRows || x < 0 || x >= termCols) return;
    va_list args;
    va_start(args, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    const int maxLen = termCols - x;
    for (int i = 0; buf[i] != '\0' && i < maxLen; ++i)
        mvaddch(y, x + i, static_cast<unsigned char>(buf[i]));
}

void Renderer::drawRoom(const Room* const room) {
    if (!room) return;

    for (int y = 0; y < ROOM_H; ++y) {
        const char* const row = room->tiles[y];
        for (int x = 0; x < ROOM_W; ++x) {
            const char t = row[x];
            int    cp;
            chtype ch;
            int    attr = 0;

            if      (t == static_cast<char>(Tile::Wall))    { cp = Color::Wall;      ch = ' ';         attr = A_BOLD; }
            else if (t == static_cast<char>(Tile::Pillar))  { cp = Color::Pillar;    ch = ACS_DIAMOND; attr = A_BOLD; }
            else if (t == static_cast<char>(Tile::Locked))  { cp = Color::Door;      ch = '+';         attr = A_BOLD; }
            else if (t == static_cast<char>(Tile::Exit))    { cp = Color::ExitGlow;  ch = 'X';         attr = A_BOLD | A_BLINK; }
            else if (t == static_cast<char>(Tile::Trap))    { cp = Color::Trap;      ch = '^';         attr = A_BOLD; }
            else if (t == static_cast<char>(Tile::Plate))   { cp = Color::Plate;     ch = 'o';         attr = A_BOLD; }
            else if (t == static_cast<char>(Tile::PlateOn)) { cp = Color::Plate;     ch = 'O';         attr = A_DIM;  }
            else if (t == static_cast<char>(Tile::Gate))    { cp = Color::Gate;      ch = '#';         attr = A_BOLD; }
            else                                             { cp = Color::FloorDark; ch = '.';         attr = A_DIM;  }

            attron(COLOR_PAIR(cp) | attr);
            safe_mvaddch(MAP_ORIG_Y + y, MAP_ORIG_X + x, ch);
            attroff(COLOR_PAIR(cp) | attr);
        }
    }

    attron(COLOR_PAIR(Color::UI));
    for (int x = 0; x <= ROOM_W + 1; ++x) {
        safe_mvaddch(MAP_ORIG_Y - 1,      MAP_ORIG_X - 1 + x, ACS_HLINE);
        safe_mvaddch(MAP_ORIG_Y + ROOM_H, MAP_ORIG_X - 1 + x, ACS_HLINE);
    }
    for (int y = 0; y <= ROOM_H + 1; ++y) {
        safe_mvaddch(MAP_ORIG_Y - 1 + y, MAP_ORIG_X - 1,      ACS_VLINE);
        safe_mvaddch(MAP_ORIG_Y - 1 + y, MAP_ORIG_X + ROOM_W, ACS_VLINE);
    }
    safe_mvaddch(MAP_ORIG_Y - 1,      MAP_ORIG_X - 1,      ACS_ULCORNER);
    safe_mvaddch(MAP_ORIG_Y - 1,      MAP_ORIG_X + ROOM_W, ACS_URCORNER);
    safe_mvaddch(MAP_ORIG_Y + ROOM_H, MAP_ORIG_X - 1,      ACS_LLCORNER);
    safe_mvaddch(MAP_ORIG_Y + ROOM_H, MAP_ORIG_X + ROOM_W, ACS_LRCORNER);
    attron(A_BOLD);
    safe_mvprintw(MAP_ORIG_Y - 1, MAP_ORIG_X + 1, " %s ", room->name);
    attroff(A_BOLD | COLOR_PAIR(Color::UI));
}

void Renderer::drawEntities(const Player*    const p,
                             const EnemyPool* const ep,
                             const ItemPool*  const ip,
                             int roomId)
{
    if (!p || !ep || !ip) return;

    const Item* const iend = ip->items + ip->count;
    for (const Item* it = ip->items; it != iend; ++it) {
        if (!it->active || it->roomId != roomId) continue;
        attron(COLOR_PAIR(Color::Item) | A_BOLD);
        safe_mvaddch(MAP_ORIG_Y + it->y, MAP_ORIG_X + it->x, it->glyph());
        attroff(COLOR_PAIR(Color::Item) | A_BOLD);
    }

    const Enemy* const eend = ep->enemies + ep->count;
    for (const Enemy* e = ep->enemies; e != eend; ++e) {
        if (!e->active || e->roomId != roomId) continue;

        const int cp = (e->type == EnemyType::Boss)   ? Color::Boss
                     : (e->type == EnemyType::Patrol) ? Color::Enemy2
                                                      : Color::Enemy1;
        attron(COLOR_PAIR(cp) | A_BOLD);
        safe_mvaddch(MAP_ORIG_Y + e->y, MAP_ORIG_X + e->x, e->glyph());
        attroff(COLOR_PAIR(cp) | A_BOLD);

        // barra de vida encima del sprite, solo si ya recibio daño
        if (e->hp < e->maxHp) {
            const int ey  = MAP_ORIG_Y + e->y - 1;
            const int exs = MAP_ORIG_X + e->x - e->maxHp / 2;
            if (ey >= MAP_ORIG_Y) {
                for (int h = 0; h < e->maxHp; ++h) {
                    const int bx = exs + h;
                    if (bx < 0 || bx >= termCols) continue;
                    const int cpH = (h < e->hp) ? Color::Enemy1 : Color::UI;
                    attron(COLOR_PAIR(cpH));
                    safe_mvaddch(ey, bx, (h < e->hp) ? '*' : '-');
                    attroff(COLOR_PAIR(cpH));
                }
            }
        }
    }

    attron(COLOR_PAIR(Color::Player) | A_BOLD);
    safe_mvaddch(MAP_ORIG_Y + p->y, MAP_ORIG_X + p->x, '@');
    attroff(COLOR_PAIR(Color::Player) | A_BOLD);
}

void Renderer::drawMiniMap(const World*  const w,
                            const Player* const p,
                            const bool visited[MAX_ROOMS])
{
    if (!w || !p) return;
    const int baseX = MAP_ORIG_X;
    const int baseY = MAP_ORIG_Y + ROOM_H + 2;

    attron(COLOR_PAIR(Color::UI) | A_BOLD);
    safe_mvprintw(baseY - 1, baseX, "[ MAPA ] M = mapa completo");
    attroff(A_BOLD | COLOR_PAIR(Color::UI));

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            const int roomId = row * 3 + col;
            const int sx = baseX + col * MINI_CELL_W;
            const int sy = baseY + row * MINI_CELL_H;
            int  cp;
            char lbl;
            if      (roomId == p->roomId)   { cp = Color::MiniCur;   lbl = '@'; }
            else if (visited[roomId])        { cp = Color::MiniVisit; lbl = static_cast<char>('0' + roomId); }
            else                             { cp = Color::MiniHide;  lbl = ' '; }

            for (int dy = 0; dy < MINI_CELL_H; ++dy) {
                attron(COLOR_PAIR(cp));
                for (int dx = 0; dx < MINI_CELL_W - 1; ++dx) {
                    char ch = (visited[roomId] || roomId == p->roomId) ? '.' : ' ';
                    if (dy == 1 && dx == 1) ch = lbl;
                    safe_mvaddch(sy + dy, sx + dx, ch);
                }
                attroff(COLOR_PAIR(cp));
                attron(COLOR_PAIR(Color::UI));
                safe_mvaddch(sy + dy, sx + MINI_CELL_W - 1, ACS_VLINE);
                attroff(COLOR_PAIR(Color::UI));
            }
            attron(COLOR_PAIR(Color::UI));
            for (int dx = 0; dx < MINI_CELL_W - 1; ++dx)
                safe_mvaddch(sy + MINI_CELL_H, sx + dx, ACS_HLINE);
            safe_mvaddch(sy + MINI_CELL_H, sx + MINI_CELL_W - 1, ACS_PLUS);
            attroff(COLOR_PAIR(Color::UI));
        }
    }
}

void Renderer::drawUI(const Player*    const p,
                       const EnemyPool* const ep,
                       const char* message,
                       int roomId)
{
    if (!p || !ep) return;
    int cx = UI_ORIG_X;
    int cy = UI_ORIG_Y;

    attron(COLOR_PAIR(Color::Item) | A_BOLD);
    safe_mvprintw(cy++, cx, "DUNGEON CRAWLER");
    attroff(A_BOLD | COLOR_PAIR(Color::Item));

    attron(COLOR_PAIR(Color::UI));
    safe_mvprintw(cy++, cx, "Sala %d", roomId);
    ++cy;

    safe_mvprintw(cy, cx, "HP:");
    for (int i = 0; i < p->maxHp; ++i) {
        if (i < p->hp) {
            attron(COLOR_PAIR(Color::Enemy1) | A_BOLD);
            safe_mvaddch(cy, cx + 4 + i, '*');
            attroff(COLOR_PAIR(Color::Enemy1) | A_BOLD);
        } else {
            attron(COLOR_PAIR(Color::UI) | A_DIM);
            safe_mvaddch(cy, cx + 4 + i, '-');
            attroff(COLOR_PAIR(Color::UI) | A_DIM);
        }
    }
    cy += 2;

    attron(COLOR_PAIR(Color::Item) | A_BOLD);
    safe_mvprintw(cy++, cx, "Inv: %s", p->heldItem ? p->heldItem->name() : "[nada]");
    attroff(A_BOLD | COLOR_PAIR(Color::Item));
    ++cy;

    attron(COLOR_PAIR(Color::Item));
    safe_mvprintw(cy++, cx, "%-22.22s", message ? message : "");
    attroff(COLOR_PAIR(Color::Item));
    ++cy;

    attron(COLOR_PAIR(Color::Enemy1) | A_BOLD);
    safe_mvprintw(cy++, cx, "COMBATE:");
    attroff(A_BOLD | COLOR_PAIR(Color::Enemy1));
    attron(COLOR_PAIR(Color::UI));
    safe_mvprintw(cy++, cx, " Empuja a un enemigo");
    safe_mvprintw(cy++, cx, " para atacarlo");
    attron(A_DIM);
    safe_mvprintw(cy++, cx, " Espada = x2 dano");
    safe_mvprintw(cy++, cx, " Jefe suelta la LLAVE");
    attroff(A_DIM | COLOR_PAIR(Color::UI));
    ++cy;

    attron(COLOR_PAIR(Color::UI) | A_BOLD);
    safe_mvprintw(cy++, cx, "Controles:");
    attroff(A_BOLD);
    safe_mvprintw(cy++, cx, " WASD/flechas mover");
    safe_mvprintw(cy++, cx, " F/Espacio    recoger");
    safe_mvprintw(cy++, cx, " M            mapa");
    safe_mvprintw(cy++, cx, " Q            salir");
    ++cy;

    attron(A_BOLD);
    safe_mvprintw(cy++, cx, "Leyenda:");
    attroff(A_BOLD);

    struct Legend { int cp; char ch; const char* desc; };
    static constexpr Legend legend[] = {
        { Color::Trap,   '^', " trampa de puas"  },
        { Color::Plate,  'o', " placa presion"   },
        { Color::Gate,   '#', " puerta secreta"  },
        { Color::Boss,   'B', " Jefe (llave!)"   },
    };
    for (const Legend* l = legend; l != legend + 4; ++l) {
        if (cy >= termRows) break;
        attron(COLOR_PAIR(l->cp) | A_BOLD);
        safe_mvprintw(cy, cx, " %c", l->ch);
        attroff(COLOR_PAIR(l->cp) | A_BOLD);
        attron(COLOR_PAIR(Color::UI));
        safe_mvprintw(cy++, cx + 2, "%s", l->desc);
        attroff(COLOR_PAIR(Color::UI));
    }
    ++cy;

    bool header = false;
    const Enemy* const eend = ep->enemies + ep->count;
    for (const Enemy* e = ep->enemies; e != eend; ++e) {
        if (!e->active || e->roomId != roomId) continue;
        if (cy >= termRows - 1) break;
        if (!header) {
            attron(COLOR_PAIR(Color::UI) | A_BOLD);
            safe_mvprintw(cy++, cx, "Enemigos:");
            attroff(A_BOLD | COLOR_PAIR(Color::UI));
            header = true;
        }
        if (cy >= termRows - 1) break;
        const int cp = (e->type == EnemyType::Boss)   ? Color::Boss
                     : (e->type == EnemyType::Patrol) ? Color::Enemy2
                                                      : Color::Enemy1;
        attron(COLOR_PAIR(cp) | A_BOLD);
        safe_mvprintw(cy, cx, " %c", e->glyph());
        attroff(COLOR_PAIR(cp) | A_BOLD);
        attron(COLOR_PAIR(Color::UI));
        safe_mvprintw(cy, cx + 2, " ");
        for (int h = 0; h < e->maxHp; ++h) {
            const int bx = cx + 3 + h;
            if (bx >= termCols) break;
            if (h < e->hp) {
                attron(COLOR_PAIR(Color::Enemy1));
                safe_mvaddch(cy, bx, '*');
                attroff(COLOR_PAIR(Color::Enemy1));
            } else {
                attron(COLOR_PAIR(Color::UI) | A_DIM);
                safe_mvaddch(cy, bx, '-');
                attroff(COLOR_PAIR(Color::UI) | A_DIM);
            }
        }
        attron(COLOR_PAIR(Color::UI));
        safe_mvprintw(cy++, cx + 3 + e->maxHp + 1, "%s", e->name());
        attroff(COLOR_PAIR(Color::UI));
    }
}

void Renderer::draw(const World*     const world,
                     const Player*   const player,
                     const EnemyPool* const enemies,
                     const ItemPool*  const items,
                     const char* message,
                     const bool visited[MAX_ROOMS])
{
    getmaxyx(stdscr, termRows, termCols);
    clear();
    if (!world || !player || !enemies || !items) { refresh(); return; }

    const Room* const room = world->roomById(player->roomId);
    drawRoom(room);
    drawEntities(player, enemies, items, player->roomId);
    drawUI(player, enemies, message, player->roomId);
    drawMiniMap(world, player, visited);
    refresh();
}

void Renderer::drawFullMap(const World*  const world,
                            const Player* const player,
                            const bool visited[MAX_ROOMS])
{
    if (!world || !player) return;
    clear();
    constexpr int CW = 10, CH = 4;
    const int startX = (termCols - CW * 3 - 1) / 2;
    const int startY = (termRows - (CH * 3 + 1)) / 2;

    attron(COLOR_PAIR(Color::Item) | A_BOLD);
    safe_mvprintw(startY - 2, startX, "=== MAPA DEL DUNGEON (M para cerrar) ===");
    attroff(A_BOLD | COLOR_PAIR(Color::Item));

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            const int roomId = row * 3 + col;
            const int sx = startX + col * CW;
            const int sy = startY + row * CH;
            const int cp = (roomId == player->roomId) ? Color::MiniCur
                         : visited[roomId]             ? Color::MiniVisit
                                                       : Color::MiniHide;
            const Room* const room = world->roomById(roomId);
            const bool vis = visited[roomId] || roomId == player->roomId;

            for (int dy = 0; dy < CH; ++dy) {
                attron(COLOR_PAIR(cp));
                for (int dx = 0; dx < CW - 1; ++dx) {
                    char ch = ' ';
                    if (vis) {
                        if (dy == 0 || dy == CH-1 || dx == 0 || dx == CW-2) ch = '.';
                        if (dy == CH/2) {
                            if      (dx == CW/2 - 1 && roomId == player->roomId) ch = '@';
                            else if (dx == CW/2 - 1 && room && room->hasExit)   ch = 'X';
                            else if (dx == CW/2 - 1 && room && room->locked)    ch = '+';
                            if (dx == CW/2) ch = static_cast<char>('0' + roomId);
                        }
                    } else {
                        ch = (dy % 2 == 0 && dx % 2 == 0) ? '?' : ' ';
                    }
                    safe_mvaddch(sy + dy, sx + dx, ch);
                }
                attroff(COLOR_PAIR(cp));
                attron(COLOR_PAIR(Color::UI));
                safe_mvaddch(sy + dy, sx + CW - 1, ACS_VLINE);
                attroff(COLOR_PAIR(Color::UI));
            }
            attron(COLOR_PAIR(Color::UI));
            for (int dx = 0; dx < CW - 1; ++dx)
                safe_mvaddch(sy + CH, sx + dx, ACS_HLINE);
            safe_mvaddch(sy + CH, sx + CW - 1, ACS_PLUS);
            if (vis && room) {
                attron(COLOR_PAIR(Color::Door) | A_BOLD);
                if (room->north >= 0) safe_mvaddch(sy,        sx + CW/2,     '^');
                if (room->south >= 0) safe_mvaddch(sy + CH,   sx + CW/2,     'v');
                if (room->east  >= 0) safe_mvaddch(sy + CH/2, sx + CW - 1,   '>');
                if (room->west  >= 0) safe_mvaddch(sy + CH/2, sx,            '<');
                attroff(A_BOLD | COLOR_PAIR(Color::Door));
            }
            attroff(COLOR_PAIR(Color::UI));
        }
    }
    attron(COLOR_PAIR(Color::UI));
    safe_mvprintw(startY + CH * 3 + 2, startX, "Presiona M para volver al juego");
    attroff(COLOR_PAIR(Color::UI));
    refresh();
}

void Renderer::drawGameOver(bool won,
                             int  tickCount,
                             int  playerHp,
                             int  playerMaxHp,
                             const bool visited[MAX_ROOMS])
{
    getmaxyx(stdscr, termRows, termCols);
    clear();
    nodelay(stdscr, FALSE);

    if (won) {
        static const char* const art[] = {
            "  ____   ____  _______  ______  ____   ____  _____   _____  ",
            " |_  _| |_  _||_   __ \\|_   _ \\|_  _| |_  _||_   _| |_   _|",
            "   \\ \\   / /    | |__) | | |_) | \\ \\   / /    | |     | |  ",
            "    \\ \\ / /     |  ___/  |  __'.  \\ \\ / /     | |     | |  ",
            "     \\ ' /     _| |_    _| |__) |  \\ ' /     _| |_   _| |_ ",
            "      \\_/     |_____|  |_______/    \\_/     |_____| |_____|",
        };
        static constexpr int ART_LINES = 6;
        const int artW  = 60;
        const int cy0   = termRows / 2 - ART_LINES - 4;
        const int cxArt = (termCols - artW) / 2;

        attron(COLOR_PAIR(Color::Item) | A_BOLD);
        for (int x = 2; x < termCols - 2; ++x)
            safe_mvaddch(cy0 - 2, x, ACS_HLINE);
        safe_mvaddch(cy0 - 2, 2,            ACS_ULCORNER);
        safe_mvaddch(cy0 - 2, termCols - 3, ACS_URCORNER);
        attroff(A_BOLD | COLOR_PAIR(Color::Item));

        for (int i = 0; i < ART_LINES; ++i) {
            attron(COLOR_PAIR(Color::Item) | A_BOLD);
            safe_mvprintw(cy0 + i, cxArt, "%s", art[i]);
            attroff(A_BOLD | COLOR_PAIR(Color::Item));
        }

        const int subY = cy0 + ART_LINES + 1;
        attron(COLOR_PAIR(Color::Door) | A_BOLD);
        safe_mvprintw(subY, (termCols - 36) / 2,
                      "** El Jefe Oscuro ha sido derrotado! **");
        attroff(A_BOLD | COLOR_PAIR(Color::Door));

        attron(COLOR_PAIR(Color::UI));
        for (int x = (termCols / 2) - 20; x <= (termCols / 2) + 20; ++x)
            safe_mvaddch(subY + 2, x, ACS_HLINE);
        attroff(COLOR_PAIR(Color::UI));

        const int stY = subY + 3;
        const int stX = (termCols - 38) / 2;

        attron(COLOR_PAIR(Color::UI) | A_BOLD);
        safe_mvprintw(stY, stX, "Turnos totales  : ");
        attroff(A_BOLD | COLOR_PAIR(Color::UI));
        attron(COLOR_PAIR(Color::Item));
        safe_mvprintw(stY, stX + 18, "%d", tickCount);
        attroff(COLOR_PAIR(Color::Item));

        attron(COLOR_PAIR(Color::UI) | A_BOLD);
        safe_mvprintw(stY + 1, stX, "Vida restante   : ");
        attroff(A_BOLD | COLOR_PAIR(Color::UI));
        for (int i = 0; i < playerMaxHp; ++i) {
            const int bx = stX + 18 + i;
            if (i < playerHp) {
                attron(COLOR_PAIR(Color::Enemy1) | A_BOLD);
                safe_mvaddch(stY + 1, bx, '*');
                attroff(COLOR_PAIR(Color::Enemy1) | A_BOLD);
            } else {
                attron(COLOR_PAIR(Color::UI) | A_DIM);
                safe_mvaddch(stY + 1, bx, '-');
                attroff(COLOR_PAIR(Color::UI) | A_DIM);
            }
        }
        safe_mvprintw(stY + 1, stX + 18 + playerMaxHp + 1,
                      "(%d/%d)", playerHp, playerMaxHp);

        int exploredCount = 0;
        for (int i = 0; i < MAX_ROOMS; ++i)
            if (visited[i]) ++exploredCount;
        const int pct = (exploredCount * 100) / MAX_ROOMS;
        attron(COLOR_PAIR(Color::UI) | A_BOLD);
        safe_mvprintw(stY + 2, stX, "Salas exploradas: ");
        attroff(A_BOLD | COLOR_PAIR(Color::UI));
        attron(COLOR_PAIR(Color::Door));
        safe_mvprintw(stY + 2, stX + 18, "%d/%d  (%d%%)",
                      exploredCount, MAX_ROOMS, pct);
        attroff(COLOR_PAIR(Color::Door));

        attron(COLOR_PAIR(Color::Item) | A_BOLD);
        for (int x = 2; x < termCols - 2; ++x)
            safe_mvaddch(stY + 4, x, ACS_HLINE);
        safe_mvaddch(stY + 4, 2,            ACS_LLCORNER);
        safe_mvaddch(stY + 4, termCols - 3, ACS_LRCORNER);
        attroff(A_BOLD | COLOR_PAIR(Color::Item));

        attron(COLOR_PAIR(Color::UI));
        safe_mvprintw(stY + 6, (termCols - 30) / 2,
                      "  Presiona cualquier tecla...  ");
        attroff(COLOR_PAIR(Color::UI));

    } else {
        const int cy = termRows / 2 - 2;
        const int cx = (termCols - 32) / 2;
        attron(COLOR_PAIR(Color::Enemy1) | A_BOLD);
        safe_mvprintw(cy,     cx, "  *** HAS MUERTO... ***        ");
        safe_mvprintw(cy + 1, cx, "  El dungeon te reclamo.       ");
        attroff(A_BOLD | COLOR_PAIR(Color::Enemy1));
        attron(COLOR_PAIR(Color::UI));
        safe_mvprintw(cy + 3, cx, "  Presiona cualquier tecla...");
        attroff(COLOR_PAIR(Color::UI));
    }

    refresh();
    getch();
}
