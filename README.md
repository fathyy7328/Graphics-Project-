# Garden Tower Defense

A four-member DMET 502 computer graphics project: a garden-themed 2D tower-defense game built with C++ and modern OpenGL.

Real Madrid defenders protect the trophies from Barcelona-themed enemies. The game architecture, module ownership, and shared interface checklist are documented in [PROJECT_ARCHITECTURE.md](PROJECT_ARCHITECTURE.md).

## Project status

The folders and empty source placeholders define the team architecture. Gameplay code has not been added yet. `Common.h` is intentionally empty for the team to agree on together. The CMake file currently sets up the shared dependencies; the game executable target will be added when the team begins the implementation.

## Team modules

- `core/` - Member 1: engine, renderer, menus, and cutscene
- `world/` - Member 2: garden, lanes, interface, coins, and sound
- `players/` - Member 3: defender models, movement, and projectiles
- `enemies/` - Member 4: enemy models, waves, and combat
- `assets/` - Art and audio grouped by use
- `external/` - GLFW, GLM, and GLAD dependencies
