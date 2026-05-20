# Dungeon Crawler — Parcial Final C++

Videojuego dungeon-crawler por turnos inspirado en Adventure (Atari 2600, 1980),
desarrollado en C++ con ncurses. Optimiza el uso de RAM mediante arreglos estáticos
y punteros; sin `new`/`delete` en el game-loop.

## Integrantes
- *(Tu nombre aquí)*

## Objetivo del juego
Consigue la **llave** (`k`) en la habitación 6 (abajo-izquierda),
abre la **puerta bloqueada** (`+`) hacia la habitación 2 (arriba-derecha)
y pisa el **exit** (`X`) para escapar.

## Mapa de habitaciones

```
[0]──[1]──[2 EXIT 🔒]
 |    |    |
[3]──[4]──[5]
 |    |    |
[6]──[7]──[8]
```

- **Inicio:** habitación 4 (centro)
- **Llave:** habitación 6
- **Salida:** habitación 2 (bloqueada)

## Controles

| Tecla | Acción |
|-------|--------|
| `W` / `↑` | Mover arriba |
| `S` / `↓` | Mover abajo  |
| `A` / `←` | Mover izquierda |
| `D` / `→` | Mover derecha |
| `F` / `Space` | Recoger / soltar objeto |
| `Q` | Salir |

## Entidades

| Símbolo | Descripción |
|---------|-------------|
| `@`     | Jugador     |
| `C`     | Chaser — persigue directamente |
| `P`     | Patrol — patrulla, persigue si estás a ≤4 tiles |
| `k`     | Llave |
| `s`     | Espada *(decorativa — innovación futura)* |
| `p`     | Poción (+2 HP, auto-use) |
| `X`     | Salida (victoria) |
| `+`     | Puerta bloqueada |

## Compilación

### Linux / macOS
```bash
sudo apt install libncurses-dev   # Debian/Ubuntu
# brew install ncurses            # macOS (si es necesario)

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/dungeon
```

### Windows (PDCurses + MinGW / MSVC)
```powershell
# Instala PDCurses: https://pdcurses.org/
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\dungeon.exe
```

## Arquitectura técnica

```
include/   ← interfaces (.h)
src/       ← implementaciones (.cpp)
```

| Archivo | Responsabilidad |
|---------|-----------------|
| `types.h`    | Constantes globales, enums de tile, colores ncurses |
| `item.h/cpp` | Struct Item + pool estático sin heap |
| `player.h/cpp` | Estado del jugador, inventario single-slot |
| `enemy.h/cpp`  | Dos tipos de enemigos (Chaser, Patrol) con IA |
| `room.h/cpp`   | Tile map estático, world 3×3 |
| `renderer.h/cpp` | Rendering completo con ncurses |
| `game.h/cpp`   | Game loop, input, update, transiciones |
| `main.cpp`     | Entry point |

## Restricciones cumplidas
- ✅ Lenguaje: C++17
- ✅ Mínimo 6 habitaciones conectadas (9 en total)
- ✅ Colisiones funcionales
- ✅ 2 tipos de enemigos con persecución
- ✅ Inventario de 1 slot (recoger / soltar)
- ✅ Condición de victoria (Exit) y derrota (HP = 0)
- ✅ Código modular en múltiples `.h` y `.cpp`
- ✅ Punteros verificables en lógica (`Item*`, `Enemy*`, `Room*`)
- ✅ CMake para compilación automática
- ✅ Sin `new`/`delete` en el game-loop
- ✅ Arreglos estáticos de entidades (`ItemPool`, `EnemyPool`)
