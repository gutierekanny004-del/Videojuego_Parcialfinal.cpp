#include "intro.h"

#ifdef _WIN32
#  include <curses.h>
#else
#  include <ncurses.h>
#endif

#include <cstring>

static void centered(int y, const char* text, int attr = 0) {
    int cols = getmaxx(stdscr);
    int x = (cols - (int)strlen(text)) / 2;
    if (x < 0) x = 0;
    if (attr) attron(attr);
    mvprintw(y, x, "%s", text);
    if (attr) attroff(attr);
}

static void drawBorder() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    for (int x = 0; x < cols; ++x) {
        mvaddch(0,        x, ACS_HLINE);
        mvaddch(rows - 1, x, ACS_HLINE);
    }
    for (int y = 0; y < rows; ++y) {
        mvaddch(y, 0,        ACS_VLINE);
        mvaddch(y, cols - 1, ACS_VLINE);
    }
    mvaddch(0,        0,        ACS_ULCORNER);
    mvaddch(0,        cols - 1, ACS_URCORNER);
    mvaddch(rows - 1, 0,        ACS_LLCORNER);
    mvaddch(rows - 1, cols - 1, ACS_LRCORNER);
}

// ─── Pagina 1: Historia ───────────────────────────────────────────────────────

static void pageStory() {
    clear();
    drawBorder();
    int r = 2;
    centered(r++, "* * *  T H E   D U N G E O N   C R A W L E R  * * *",
             COLOR_PAIR(3) | A_BOLD);
    r++;
    centered(r++, "Bajo las ruinas del Castillo Oscuro yace la Camara Sellada.");
    centered(r++, "Dentro: un artefacto de poder incalculable.");
    r++;
    centered(r++, "Para llegar necesitas una llave escondida en las profundidades.");
    centered(r++, "Los pasillos estan llenos de guardianes... y de trampas.");
    r++;
    centered(r++, "Los mecanismos antiguos pueden abrirte camino.");
    centered(r++, "Aprende a usarlos o muere en el intento.");
    r += 2;
    centered(r++, "Suerte, aventurero.", A_DIM);
    r += 2;
    centered(r, "[ ENTER: continuar ]", A_DIM);
    refresh();
    int k;
    do { k = getch(); } while (k != '\n' && k != KEY_ENTER && k != ' ');
}

// ─── Pagina 2: Controles y mecanicas ─────────────────────────────────────────

static void pageControls() {
    clear();
    drawBorder();
    int r = 2;
    centered(r++, "=== CONTROLES Y MECANICAS ===", COLOR_PAIR(6) | A_BOLD);
    r++;
    int cx = getmaxx(stdscr) / 2 - 20;

    attron(A_BOLD); mvprintw(r++, cx, "Movimiento:"); attroff(A_BOLD);
    mvprintw(r++, cx, "  W/A/S/D  o  flechas  ->  mover");
    r++;

    attron(A_BOLD); mvprintw(r++, cx, "Combate:"); attroff(A_BOLD);
    mvprintw(r++, cx, "  Muevete HACIA un enemigo para atacarlo");
    attron(COLOR_PAIR(4)); mvprintw(r, cx, "  C"); attroff(COLOR_PAIR(4));
    mvprintw(r++, cx + 3, " = Chaser  (2 HP)  se acerca rapido");
    attron(COLOR_PAIR(5)); mvprintw(r, cx, "  P"); attroff(COLOR_PAIR(5));
    mvprintw(r++, cx + 3, " = Patrol  (3 HP)  ronda y ataca");
    mvprintw(r++, cx, "  Espada en mano: haces 2 de dano en vez de 1");
    r++;

    attron(A_BOLD); mvprintw(r++, cx, "Trampas y puzzles:"); attroff(A_BOLD);
    attron(COLOR_PAIR(4)); mvprintw(r, cx, "  ^"); attroff(COLOR_PAIR(4));
    mvprintw(r++, cx + 3, " = trampa de puas   (-1 HP al pisar)");
    attron(COLOR_PAIR(6)); mvprintw(r, cx, "  o"); attroff(COLOR_PAIR(6));
    mvprintw(r++, cx + 3, " = placa de presion (pisala para abrir una puerta)");
    attron(COLOR_PAIR(8)); mvprintw(r, cx, "  #"); attroff(COLOR_PAIR(8));
    mvprintw(r++, cx + 3, " = puerta secreta   (se abre con la placa correcta)");
    r++;

    attron(A_BOLD); mvprintw(r++, cx, "Otros:"); attroff(A_BOLD);
    mvprintw(r++, cx, "  F / Espacio  ->  recoger / soltar objeto");
    mvprintw(r++, cx, "  M            ->  ver mapa (solo zonas exploradas)");
    mvprintw(r++, cx, "  Q            ->  salir");
    r++;

    attron(A_BOLD); mvprintw(r++, cx, "Objetivo:"); attroff(A_BOLD);
    mvprintw(r++, cx, "  1. Explora el dungeon (9 salas)");
    mvprintw(r++, cx, "  2. Resuelve el puzzle para conseguir la LLAVE");
    mvprintw(r++, cx, "  3. Abre la Camara Sellada y pisa la X para ganar");
    r += 2;

    centered(r, "[ ENTER: entrar al dungeon ]", COLOR_PAIR(3) | A_BOLD);
    refresh();
    int k;
    do { k = getch(); } while (k != '\n' && k != KEY_ENTER && k != ' ');
}

// ─── showIntro ────────────────────────────────────────────────────────────────

bool showIntro() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_WHITE,   -1);
        init_pair(2, COLOR_BLACK,   -1);
        init_pair(3, COLOR_CYAN,    -1);
        init_pair(4, COLOR_RED,     -1);
        init_pair(5, COLOR_MAGENTA, -1);
        init_pair(6, COLOR_YELLOW,  -1);
        init_pair(7, COLOR_WHITE,   -1);
        init_pair(8, COLOR_GREEN,   -1);
    }
    pageStory();
    pageControls();
    return true;
}
