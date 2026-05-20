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

// ─── init / shutdown ─────────────────────────────────────────────────────────

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

// ─── drawRoom ────────────────────────────────────────────────────────────────

void Renderer::drawRoom(const World& w, int roomId) {
    const Room* const room = w.roomById(roomId);
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
            mvaddch(MAP_ORIG_Y + y, MAP_ORIG_X + x, ch);
            attroff(COLOR_PAIR(cp) | attr);
        }
    }

    // Border
    attron(COLOR_PAIR(Color::UI));
    for (int x = 0; x <= ROOM_W + 1; ++x) {
        mvaddch(MAP_ORIG_Y - 1,      MAP_ORIG_X - 1 + x, ACS_HLINE);
        mvaddch(MAP_ORIG_Y + ROOM_H, MAP_ORIG_X - 1 + x, ACS_HLINE);
    }
    for (int y = 0; y <= ROOM_H + 1; ++y) {
        mvaddch(MAP_ORIG_Y - 1 + y, MAP_ORIG_X - 1,      ACS_VLINE);
        mvaddch(MAP_ORIG_Y - 1 + y, MAP_ORIG_X + ROOM_W, ACS_VLINE);
    }
    mvaddch(MAP_ORIG_Y - 1,      MAP_ORIG_X - 1,      ACS_ULCORNER);
    mvaddch(MAP_ORIG_Y - 1,      MAP_ORIG_X + ROOM_W, ACS_URCORNER);
    mvaddch(MAP_ORIG_Y + ROOM_H, MAP_ORIG_X - 1,      ACS_LLCORNER);
    mvaddch(MAP_ORIG_Y + ROOM_H, MAP_ORIG_X + ROOM_W, ACS_LRCORNER);
    attron(A_BOLD);
    mvprintw(MAP_ORIG_Y - 1, MAP_ORIG_X + 1, " %s ", room->name);
    attroff(A_BOLD | COLOR_PAIR(Color::UI));
}

// ─── drawEntities ────────────────────────────────────────────────────────────

void Renderer::drawEntities(const Player& p, const EnemyPool& ep,
                             const ItemPool& ip, int roomId)
{
    // Items
    const Item* const iend = ip.items + ip.count;
    for (const Item* it = ip.items; it != iend; ++it) {
        if (!it->active || it->roomId != roomId) continue;
        attron(COLOR_PAIR(Color::Item) | A_BOLD);
        mvaddch(MAP_ORIG_Y + it->y, MAP_ORIG_X + it->x, it->glyph());
        attroff(COLOR_PAIR(Color::Item) | A_BOLD);
    }

    // Enemies
    const Enemy* const eend = ep.enemies + ep.count;
    for (const Enemy* e = ep.enemies; e != eend; ++e) {
        if (!e->active || e->roomId != roomId) continue;

        const int cp = (e->type == EnemyType::Boss)   ? Color::Boss
                     : (e->type == EnemyType::Patrol) ? Color::Enemy2
                                                       : Color::Enemy1;
        attron(COLOR_PAIR(cp) | A_BOLD);
        mvaddch(MAP_ORIG_Y + e->y, MAP_ORIG_X + e->x, e->glyph());
        attroff(COLOR_PAIR(cp) | A_BOLD);

        // HP pips above enemy (only if damaged)
        if (e->hp < e->maxHp) {
            const int ey = MAP_ORIG_Y + e->y - 1;
            const int ex = MAP_ORIG_X + e->x - e->maxHp / 2;
            if (ey >= MAP_ORIG_Y) {
                for (int h = 0; h < e->maxHp; ++h) {
                    const int cpH = (h < e->hp) ? Color::Enemy1 : Color::UI;
                    attron(COLOR_PAIR(cpH));
                    mvaddch(ey, ex + h, (h < e->hp) ? '*' : '-');
                    attroff(COLOR_PAIR(cpH));
                }
            }
        }
    }

    // Player
    attron(COLOR_PAIR(Color::Player) | A_BOLD);
    mvaddch(MAP_ORIG_Y + p.y, MAP_ORIG_X + p.x, '@');
    attroff(COLOR_PAIR(Color::Player) | A_BOLD);
}

// ─── drawMiniMap ─────────────────────────────────────────────────────────────

void Renderer::drawMiniMap(const World& w, const Player& p,
                            const bool visited[MAX_ROOMS])
{
    const int baseX = MAP_ORIG_X;
    const int baseY = MAP_ORIG_Y + ROOM_H + 2;

    attron(COLOR_PAIR(Color::UI) | A_BOLD);
    mvprintw(baseY - 1, baseX, "[ MAPA ] M = mapa completo");
    attroff(A_BOLD | COLOR_PAIR(Color::UI));

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            const int roomId = row * 3 + col;
            const int sx = baseX + col * MINI_CELL_W;
            const int sy = baseY + row * MINI_CELL_H;
            int cp;
            char lbl;
            if      (roomId == p.roomId)   { cp = Color::MiniCur;   lbl = '@'; }
            else if (visited[roomId])      { cp = Color::MiniVisit; lbl = static_cast<char>('0'+roomId); }
            else                           { cp = Color::MiniHide;  lbl = ' '; }

            for (int dy = 0; dy < MINI_CELL_H; ++dy) {
                attron(COLOR_PAIR(cp));
                for (int dx = 0; dx < MINI_CELL_W - 1; ++dx) {
                    char ch = (visited[roomId] || roomId == p.roomId) ? '.' : ' ';
                    if (dy == 1 && dx == 1) ch = lbl;
                    mvaddch(sy + dy, sx + dx, ch);
                }
                attroff(COLOR_PAIR(cp));
                attron(COLOR_PAIR(Color::UI));
                mvaddch(sy + dy, sx + MINI_CELL_W - 1, ACS_VLINE);
                attroff(COLOR_PAIR(Color::UI));
            }
            attron(COLOR_PAIR(Color::UI));
            for (int dx = 0; dx < MINI_CELL_W - 1; ++dx)
                mvaddch(sy + MINI_CELL_H, sx + dx, ACS_HLINE);
            mvaddch(sy + MINI_CELL_H, sx + MINI_CELL_W - 1, ACS_PLUS);
            attroff(COLOR_PAIR(Color::UI));
        }
    }
}

// ─── drawFullMap ─────────────────────────────────────────────────────────────

void Renderer::drawFullMap(const World& world, const Player& player,
                            const bool visited[MAX_ROOMS])
{
    clear();
    constexpr int CW = 10, CH = 4;
    const int startX = (termCols - CW * 3 - 1) / 2;
    const int startY = (termRows - (CH * 3 + 1)) / 2;

    attron(COLOR_PAIR(Color::Item) | A_BOLD);
    mvprintw(startY - 2, startX, "=== MAPA DEL DUNGEON (M para cerrar) ===");
    attroff(A_BOLD | COLOR_PAIR(Color::Item));

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            const int roomId = row * 3 + col;
            const int sx = startX + col * CW;
            const int sy = startY + row * CH;
            const int cp = (roomId == player.roomId) ? Color::MiniCur
                         : visited[roomId]           ? Color::MiniVisit
                                                     : Color::MiniHide;
            const Room* const room = world.roomById(roomId);
            const bool vis = visited[roomId] || roomId == player.roomId;

            for (int dy = 0; dy < CH; ++dy) {
                attron(COLOR_PAIR(cp));
                for (int dx = 0; dx < CW - 1; ++dx) {
                    char ch = ' ';
                    if (vis) {
                        if (dy == 0 || dy == CH-1 || dx == 0 || dx == CW-2) ch = '.';
                        if (dy == CH/2) {
                            if      (dx == CW/2 - 1 && roomId == player.roomId) ch = '@';
                            else if (dx == CW/2 - 1 && room && room->hasExit)   ch = 'X';
                            else if (dx == CW/2 - 1 && room && room->locked)    ch = '+';
                            if (dx == CW/2) ch = static_cast<char>('0' + roomId);
                        }
                    } else {
                        ch = (dy % 2 == 0 && dx % 2 == 0) ? '?' : ' ';
                    }
                    mvaddch(sy + dy, sx + dx, ch);
                }
                attroff(COLOR_PAIR(cp));
                attron(COLOR_PAIR(Color::UI));
                mvaddch(sy + dy, sx + CW - 1, ACS_VLINE);
                attroff(COLOR_PAIR(Color::UI));
            }
            attron(COLOR_PAIR(Color::UI));
            for (int dx = 0; dx < CW - 1; ++dx)
                mvaddch(sy + CH, sx + dx, ACS_HLINE);
            mvaddch(sy + CH, sx + CW - 1, ACS_PLUS);
            if (vis && room) {
                attron(COLOR_PAIR(Color::Door) | A_BOLD);
                if (room->north >= 0) mvaddch(sy,        sx + CW/2,     '^');
                if (room->south >= 0) mvaddch(sy + CH,   sx + CW/2,     'v');
                if (room->east  >= 0) mvaddch(sy + CH/2, sx + CW - 1,   '>');
                if (room->west  >= 0) mvaddch(sy + CH/2, sx,            '<');
                attroff(A_BOLD | COLOR_PAIR(Color::Door));
            }
            attroff(COLOR_PAIR(Color::UI));
        }
    }
    attron(COLOR_PAIR(Color::UI));
    mvprintw(startY + CH * 3 + 2, startX, "Presiona M para volver al juego");
    attroff(COLOR_PAIR(Color::UI));
    refresh();
}

// ─── drawUI ──────────────────────────────────────────────────────────────────

void Renderer::drawUI(const Player& p, const EnemyPool& ep,
                      const char* message, int roomId)
{
    int cx = UI_ORIG_X;
    int cy = UI_ORIG_Y;

    // ── Título ────────────────────────────────────────────────────────────────
    attron(COLOR_PAIR(Color::Item) | A_BOLD);
    mvprintw(cy++, cx, "DUNGEON CRAWLER");
    attroff(A_BOLD | COLOR_PAIR(Color::Item));

    attron(COLOR_PAIR(Color::UI));
    mvprintw(cy++, cx, "Sala %d", roomId);
    cy++;

    // ── HP ────────────────────────────────────────────────────────────────────
    mvprintw(cy, cx, "HP:");
    for (int i = 0; i < p.maxHp; ++i) {
        if (i < p.hp) {
            attron(COLOR_PAIR(Color::Enemy1) | A_BOLD);
            mvaddch(cy, cx + 4 + i, '*');
            attroff(COLOR_PAIR(Color::Enemy1) | A_BOLD);
        } else {
            attron(COLOR_PAIR(Color::UI) | A_DIM);
            mvaddch(cy, cx + 4 + i, '-');
            attroff(COLOR_PAIR(Color::UI) | A_DIM);
        }
    }
    ++cy; ++cy;

    // ── Item ──────────────────────────────────────────────────────────────────
    attron(COLOR_PAIR(Color::Item) | A_BOLD);
    const char* const itemName = p.heldItem ? p.heldItem->name() : "[nada]";
    mvprintw(cy++, cx, "Inv: %s", itemName);
    attroff(A_BOLD | COLOR_PAIR(Color::Item));
    ++cy;

    // ── Mensaje ───────────────────────────────────────────────────────────────
    attron(COLOR_PAIR(Color::Item));
    mvprintw(cy++, cx, "%-22.22s", message ? message : "");
    attroff(COLOR_PAIR(Color::Item));
    ++cy;

    // ── Panel COMBATE ─────────────────────────────────────────────────────────
    attron(COLOR_PAIR(Color::Enemy1) | A_BOLD);
    mvprintw(cy++, cx, "COMBATE:");
    attroff(A_BOLD | COLOR_PAIR(Color::Enemy1));
    attron(COLOR_PAIR(Color::UI));
    mvprintw(cy++, cx, " Empuja a un enemigo");
    mvprintw(cy++, cx, " para atacarlo");
    attron(A_DIM);
    mvprintw(cy++, cx, " Espada = x2 dano");
    mvprintw(cy++, cx, " Jefe suelta la LLAVE");
    attroff(A_DIM | COLOR_PAIR(Color::UI));
    ++cy;

    // ── Controles ─────────────────────────────────────────────────────────────
    attron(COLOR_PAIR(Color::UI) | A_BOLD);
    mvprintw(cy++, cx, "Controles:");
    attroff(A_BOLD);
    mvprintw(cy++, cx, " WASD/flechas mover");
    mvprintw(cy++, cx, " F/Espacio    recoger");
    mvprintw(cy++, cx, " M            mapa");
    mvprintw(cy++, cx, " Q            salir");
    ++cy;

    // ── Leyenda ───────────────────────────────────────────────────────────────
    attron(A_BOLD);
    mvprintw(cy++, cx, "Leyenda:");
    attroff(A_BOLD);

    struct Legend { int cp; char ch; const char* desc; };
    static constexpr Legend legend[] = {
        { Color::Trap,   '^', " trampa de puas"  },
        { Color::Plate,  'o', " placa presion"   },
        { Color::Gate,   '#', " puerta secreta"  },
        { Color::Boss,   'B', " Jefe (llave!)"   },
    };
    for (const Legend* l = legend; l != legend + 4; ++l) {
        attron(COLOR_PAIR(l->cp) | A_BOLD);
        mvprintw(cy, cx, " %c", l->ch);
        attroff(COLOR_PAIR(l->cp) | A_BOLD);
        attron(COLOR_PAIR(Color::UI));
        mvprintw(cy++, cx + 2, "%s", l->desc);
        attroff(COLOR_PAIR(Color::UI));
    }
    ++cy;

    // ── Enemigos en esta sala ─────────────────────────────────────────────────
    bool header = false;
    const Enemy* const eend = ep.enemies + ep.count;
    for (const Enemy* e = ep.enemies; e != eend; ++e) {
        if (!e->active || e->roomId != roomId) continue;
        if (!header) {
            attron(COLOR_PAIR(Color::UI) | A_BOLD);
            mvprintw(cy++, cx, "Enemigos:");
            attroff(A_BOLD | COLOR_PAIR(Color::UI));
            header = true;
        }
        const int cp = (e->type == EnemyType::Boss)   ? Color::Boss
                     : (e->type == EnemyType::Patrol) ? Color::Enemy2
                                                       : Color::Enemy1;
        attron(COLOR_PAIR(cp) | A_BOLD);
        mvprintw(cy, cx, " %c", e->glyph());
        attroff(COLOR_PAIR(cp) | A_BOLD);
        attron(COLOR_PAIR(Color::UI));
        // HP bar inline
        mvprintw(cy, cx + 2, " ");
        for (int h = 0; h < e->maxHp; ++h) {
            if (h < e->hp) {
                attron(COLOR_PAIR(Color::Enemy1));
                mvaddch(cy, cx + 3 + h, '*');
                attroff(COLOR_PAIR(Color::Enemy1));
            } else {
                attron(COLOR_PAIR(Color::UI) | A_DIM);
                mvaddch(cy, cx + 3 + h, '-');
                attroff(COLOR_PAIR(Color::UI) | A_DIM);
            }
        }
        attron(COLOR_PAIR(Color::UI));
        mvprintw(cy++, cx + 3 + e->maxHp + 1, "%s", e->name());
        attroff(COLOR_PAIR(Color::UI));
    }
}

// ─── draw ────────────────────────────────────────────────────────────────────

void Renderer::draw(const World& world, const Player& player,
                    const EnemyPool& enemies, const ItemPool& items,
                    const char* message, const bool visited[MAX_ROOMS])
{
    clear();
    drawRoom(world, player.roomId);
    drawEntities(player, enemies, items, player.roomId);
    drawUI(player, enemies, message, player.roomId);
    drawMiniMap(world, player, visited);
    refresh();
}

// ─── drawGameOver ────────────────────────────────────────────────────────────

void Renderer::drawGameOver(bool won) {
    clear();
    const int cy = termRows / 2 - 2;
    const int cx = termCols / 2 - 13;
    attron(COLOR_PAIR(won ? Color::Item : Color::Enemy1) | A_BOLD);
    if (won) {
        mvprintw(cy,     cx, "  *** ESCAPASTE DEL DUNGEON! ***");
        mvprintw(cy + 1, cx, "     El Jefe fue derrotado.    ");
    } else {
        mvprintw(cy,     cx, "  *** HAS MUERTO... ***        ");
        mvprintw(cy + 1, cx, "  El dungeon te reclamo.       ");
    }
    attroff(A_BOLD);
    mvprintw(cy + 3, cx, "  Presiona cualquier tecla...");
    refresh();
    nodelay(stdscr, FALSE);
    getch();
}
