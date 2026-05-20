#include "game.h"
#include "intro.h"
#include <cstdio>

int main() {
    showIntro();   // historia + mapa + controles (ncurses ya queda iniciado)

    Game game;
    game.init();
    game.run();
    return 0;
}
