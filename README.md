# Dungeon Crawler — Parcial Final C++

Juego de dungeon-crawler por turnos hecho en C++ con ncurses. El punto de partida fue Adventure (Atari 2600, 1980): un jugador, habitaciones conectadas, enemigos que te persiguen y un objeto que necesitás para escapar. La idea era tomar esa base y llevarla un poco más cerca de lo que espera un jugador hoy — una pantalla de intro antes de entrar al juego, un minimapa que te orienta sin revelar todo, y un sistema de combate con algo de profundidad en vez de solo chocar contra los enemigos. Todo eso sin soltar la restricción original: memoria 100% estática, sin `new` ni `delete` en el game-loop, igual que cuando la RAM era un recurso escaso.



## Integrantes

- Kanny Fernanda Gutiérrez Infante



## Desarrollos innovadores

1. Puzzles cruzados entre habitaciones con grupos de placas de presión
2. Jefe con drop de llave al morir
3. Combate por colisión (bump attack) con daño variable según ítem
4. Minimapa con niebla de guerra y mapa completo con tecla M
5. Habitaciones temáticas con layouts únicos de pilares y trampas
6. Renderizado ncurses con 18 pares de colores y barra de vida por enemigo
7. Arquitectura de memoria estática total



## Compilación

### Linux / macOS

```bash
# Instalar dependencias (Debian/Ubuntu)
sudo apt install libncurses-dev make g++

# macOS
brew install ncurses

# Con Make
make
./dungeon

# Con CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/dungeon
```

### Windows

Requiere [PDCurses](https://pdcurses.org/) y MinGW o MSVC.

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release -DPDCURSES_ROOT=C:/pdcurses
cmake --build build
.\build\dungeon.exe
```


## Estructura del proyecto

```
include/        <- interfaces (.h)
src/            <- implementaciones (.cpp)
Makefile
CMakeLists.txt
```



## Objetivo del juego

```
[0 Tesoro]──[1 Guardian]──[2 Sellada EXIT🔒]
    |             |               |
[3 Enigma]──[4 Cruce]────[5 Trampas]
    |             |               |
[6 Profunda]─[7 Sombras]──[8 Abismo]
```

Empiezas en la sala 4 (el cruce central). Para ganar tenés que:

1. Pisar las placas de presión en las salas 5 y 7 — eso abre la arena del jefe en la sala 6.
2. Derrotar al Jefe Oscuro en sala 6, que al morir suelta la llave.
3. Recoger la llave y llegar a la sala 2 para pisar la X de salida.



## Controles

| Tecla | Acción |
|-------|--------|
| WASD o flechas | Mover al jugador |
| Caminar hacia un enemigo | Atacar (bump attack) |
| F / Espacio | Recoger o soltar el ítem actual |
| M | Ver mapa completo (solo salas visitadas desbloqueadas) |
| Q | Salir del juego |



## Tiles y entidades

| Símbolo | Descripción |
|---------|-------------|
| `@` | Jugador |
| `C` | Cazador — persigue al jugador en línea recta |
| `P` | Centinela — patrulla un recorrido, ataca si te acercas |
| `B` | Jefe Oscuro — 6 HP, lento, suelta la llave al morir |
| `k` | Llave — necesaria para abrir la sala de salida |
| `s` | Espada — duplica el daño de los ataques |
| `p` | Poción — se usa automáticamente al recogerla (+2 HP) |
| `X` | Salida (bloqueada hasta tener la llave) |
| `+` | Puerta cerrada con llave |
| `^` | Trampa de púas (-1 HP al pisarla) |
| `o` / `O` | Placa de presión inactiva / activada |
| `#` | Puerta secreta (se abre al activar el grupo de placas) |



## Requisitos mínimos cumplidos

| Requisito | Estado |
|-----------|--------|
| C++ | C++17 |
| 6+ habitaciones conectadas |  9 salas en grilla 3×3 |
| Colisiones funcionales |  paredes, pilares, puertas cerradas |
| 2+ tipos de enemigos con persecución |  Cazador, Centinela y Jefe |
| Mapa visual |  ncurses con 18 pares de color |
| Inventario single-slot |  un ítem a la vez |
| Condiciones de victoria y derrota |  llegar a la X / HP = 0 |
| Módulos .h + .cpp |  8 módulos separados |
| Punteros en la lógica |  ver sección siguiente |
| Make y CMake |  ambos incluidos |
| Sin new/delete en el game-loop |  memoria 100% estática |



## Punteros en el código

```cpp
// aritmética de punteros para iterar el pool (enemy.cpp, item.cpp)
Enemy* const end = enemies + count;
for (Enemy* e = enemies; e != end; ++e) { ... }

// búsqueda directa por posición (enemy.cpp, item.cpp)
Enemy* EnemyPool::at(int roomId, int x, int y);
Item*  ItemPool::at (int roomId, int x, int y);

// modificar entidades a través del puntero devuelto (game.cpp)
Enemy* const target = enemies.at(player.roomId, nx, ny);
target->hp -= dmg;

// sala resuelta una vez y pasada como puntero a los helpers (game.cpp)
void Game::checkTrap          (Room* room);
void Game::checkPlate         (Room* room);
void Game::checkRoomTransition(Room* room);

// renderer recibe punteros const en vez de copias (renderer.cpp)
void Renderer::draw(const World*, const Player*, const EnemyPool*, const ItemPool*, ...);

// puntero de fila para tiles sin doble offset (room.cpp)
char* const row = tiles[y];
for (int x = 0; x < ROOM_W; ++x)
    row[x] = static_cast<char>(...);

// boss configurado directo sobre su slot (game.cpp)
Enemy* const boss = enemies.spawn(EnemyType::Boss, 6, 5, 7);
boss->dropKey = true;

// sala con guarda de nullptr (room.cpp)
Room* World::roomById(int id) {
    return (id >= 0 && id < MAX_ROOMS) ? &rooms[id] : nullptr;
}
```



## Explicación de innovaciones

### 1. Puzzles cruzados entre habitaciones

En `room.h` hay dos estructuras separadas: `TriggerPlate`, que guarda `plateRoomId` y `gateRoomId` como campos distintos, y `PuzzleGroup`, que agrupa N placas con un contador `triggered` y un umbral `needed`. Las placas viven en `World::plates[16]`, no dentro de cada sala. Cuando el jugador pisa una placa, `checkPlate()` recorre ese array con aritmética de punteros, actualiza el grupo y llama a `world.roomById(gateRoomId)` para modificar el tile de la puerta aunque el jugador no esté en esa sala. Si la puerta es remota, el mensaje dice "Una puerta LEJANA se abre..." para que el jugador sepa que algo cambió en otro cuarto.



### 2. Jefe con drop de llave

`Enemy` tiene el campo `bool dropKey`. En `game.cpp`, el bump attack chequea `target->hp <= 0` y luego `target->dropKey`; si es true, llama a `items.spawn(ItemType::Key, ...)` en la posición exacta donde murió el boss, creando el ítem en ese momento sobre el array estático. La llave no existe en el mapa hasta que el jefe muere — el combate es obligatorio para progresar.



### 3. Combate por colisión (bump attack)

En `tryMove()`, antes de verificar si la casilla destino es walkable, se llama a `enemies.at(roomId, nx, ny)`. Si devuelve un puntero válido, se calcula el daño (1 base, 2 con espada) y se retorna sin mover al jugador. El movimiento y el ataque son la misma acción, igual al sistema estándar de los roguelikes.



### 4. Minimapa con niebla de guerra

`visited[MAX_ROOMS]` es un array booleano en `Game` que se marca en `markVisited()` cada vez que el jugador entra a una nueva sala. El minimapa usa tres `COLOR_PAIR` distintos: sala actual (cyan inverso), visitada (verde), y no descubierta (negro sobre negro, invisible). Con M se ve la grilla 3×3 completa con `^v<>` indicando conexiones y `+` para la puerta cerrada.



### 5. Habitaciones temáticas

`World::init()` construye cada sala con llamadas a `placePillar()`, `placeTrap()` y `setTile()` sobre el array estático `tiles[ROOM_H][ROOM_W]`. Cada sala tiene su propio bloque de inicialización. Los nombres se copian con `strncpy` a `room.name[40]` y se muestran en el borde superior del frame durante el juego.



### 6. Renderizado con 18 colores y barra de vida

`Renderer::init()` registra los 18 pares con `init_pair()`. La barra de vida de cada enemigo se dibuja en `drawEntities()` solo si `e->hp < e->maxHp`, centrada encima del sprite. El Jefe usa `COLOR_PAIR(Color::Boss)` (texto negro sobre rojo) para diferenciarse del resto. Todas las llamadas a mvaddch/mvprintw pasan por wrappers que verifican límites del terminal antes de escribir, evitando crashes en PDCurses cuando la ventana es pequeña.



### 7. Memoria estática total

`EnemyPool` tiene `Enemy enemies[16]` y un contador. `ItemPool` tiene `Item items[16]`. `World` tiene `Room rooms[9]`, `TriggerPlate plates[16]` y `PuzzleGroup groups[8]`. Todo son miembros de valor dentro de `Game`, que vive en el stack de `main()`. Los métodos `spawn()` devuelven `T*` al slot del array estático para configuración posterior sin copias. No hay un solo `new` en todo el código.
