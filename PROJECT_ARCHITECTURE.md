# Garden Tower Defense - Project Architecture

This document records the team folder layout and ownership. The C++ placeholders are intentionally empty; the team writes and explains all implementation code together with the course's no-AI rule in mind.

## Theme and game outline

- Setting: a garden.
- Defenders: Real Madrid players protecting the trophies.
- Enemies: Barcelona players trying to reach the trophy garden.
- Game flow: main menu, short cutscene, three gameplay waves, then win or lose screen; Escape opens pause.
- Playfield: at least three horizontal lanes, home base on the left, enemies entering from the right.

## Folder and ownership map

| Location | Contents | Owner |
| --- | --- | --- |
| `core/` | `main.cpp`, OpenGL renderer, game loop/state, menus, cutscene | Member 1 (you) |
| `world/` | Garden background, lanes, home base, cards, coins, health-bar widget, sound | Member 2 |
| `players/` | Three defender models, animation, movement, jump/duck, projectiles, pickups | Member 3 |
| `enemies/` | Enemy models, attacks, wave spawning, combat, drops, win/lose triggers | Member 4 |
| `Common.h` | Shared constants, `Entity`, enums, and module/cross-module declarations | All four members together; freeze after agreement |
| `assets/` | Art and audio files grouped by use | Each asset's module owner |
| `study/` | Separate Lab 1, Lab 2, and Lab 3 practice exercises | Study only |
| `external/`, `src/Glad.cpp` | Existing GLFW, GLM, GLAD dependencies and loader source | Shared setup; keep intact |

## Module connection contract

Each module will provide `init()`, `update(dt)`, `draw()`, and `reset()`.

Shared calls to agree on in `Common.h` include `spawnPlayer(type, lane)`, `jumpLane(lane)`, `duckLane(lane)`, `getPlayers()`, `getEnemies()`, `addCoins(n)`, `drawHealthBar(x, y, hp, maxHp)`, `playSound(name)`, `onGameWon()`, and `onGameLost()`.

Only Member 1 edits `core/main.cpp`. Any proposed change to `Common.h` is discussed with the group before it is made. Modules should use shared constants and types rather than inventing their own duplicate versions.

## `Common.h` contents to decide later

`Common.h` is intentionally left empty for now. When the team is ready to create it, use this checklist to agree on its shared contents together. Put shared declarations and data here; keep each module's implementation in that module's own files.

### Shared constants

- Virtual screen width and height (for example, 1280 by 720).
- Number of lanes and the lane height or spacing.
- Home-base horizontal position and enemy spawn position.
- Unit costs and the starting coin amount.
- Any other values that multiple modules must use consistently, rather than separate hardcoded copies.

### Shared types

- An `Entity` record with: type, team, lane, x-position, y-offset, current health, maximum health, state, and animation timer.
- Player-type enum for Melee, Ranged, and Defense.
- Enemy-type enum for Easy, Medium, and Hard.
- Entity-state enum for the agreed movement and action states.
- Game-state enum for Main Menu, Cutscene, Playing, Paused, Win, and Lose.
- Attack-height enum for high, low, and melee attacks.

### Shared module and cross-module declarations

- The common `init()`, `update(dt)`, `draw()`, and `reset()` entry points for each module, with names that do not conflict across modules.
- `spawnPlayer(type, lane)`.
- `jumpLane(lane)` and `duckLane(lane)`.
- `getPlayers()` and `getEnemies()`.
- `addCoins(n)` and `drawHealthBar(x, y, hp, maxHp)`.
- `playSound(name)`.
- `onGameWon()` and `onGameLost()`.

### Decisions to settle as a team before writing it

- Whether positions are measured in pixels or normalized OpenGL coordinates, and which direction positive y points.
- Whether lane numbering starts at zero or one, and how a lane number maps to its vertical position.
- The exact C++ types and names for shared fields, constants, enum values, and function parameters/return values.
- Which module owns each shared function's implementation and which module is allowed to call it.
- How `getPlayers()` and `getEnemies()` expose entity data safely to the other modules.
- What each entity state means and which attack heights jump or duck can avoid.

Once agreed, record the final choices here or in the team's notes, write `Common.h` together, and announce any later proposed changes to the group before editing it.

## Suggested implementation order

1. Agree on `Common.h` together: constants, entity data, enums, module functions, and cross-module calls.
2. Member 1 creates the compiling CMake/window/renderer skeleton; each member confirms it builds.
3. Build each module with simple placeholder shapes and agreed stubs, then integrate into one build.
4. Replace placeholders with detailed themed models, animations, interface, waves, and combat.
5. Integrate the cutscene and menus, test both a full win and a full loss, and confirm a clean build on every machine.

These folders and empty placeholders describe ownership only. The existing CMake file still builds the three study targets; the team should update the game build together when implementation begins.
