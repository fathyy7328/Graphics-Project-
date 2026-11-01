# Garden Tower Defense

A four-member DMET 502 computer graphics project: a garden-themed 2D tower-defense game built with C++ and modern OpenGL.

Real Madrid defenders protect the trophies from Barcelona-themed enemies. The game architecture, module ownership, and shared interface checklist are documented in [PROJECT_ARCHITECTURE.md](PROJECT_ARCHITECTURE.md).

## Project status

The project is under active development. It currently builds the `GardenTowerDefense` executable with CMake, GLFW, GLAD, GLM, and FreeType. The game includes a text-rendered main menu, world drawing code, and an enemy triangle/wave prototype.

## Team modules

- `core/` - Member 1: engine, renderer, menus, and cutscene
- `world/` - Member 2: garden, lanes, interface, coins, and sound
- `players/` - Member 3: defender models, movement, and projectiles
- `enemies/` - Member 4: enemy models, waves, and combat
- `assets/` - Art and audio grouped by use
- `external/` - GLFW, GLM, and GLAD dependencies

## Windows setup (FreeType)

The project includes GLFW, GLM, and GLAD. FreeType is an external dependency and must be installed on each teammate's computer. These steps use the course MinGW-w64 toolchain and MSYS2 UCRT64 FreeType package.

Before starting, install the course CMake and x86_64 UCRT MinGW tools using the university's OpenGL template setup instructions. The commands below assume those tools are on `PATH` and MinGW was installed in the default `devtools` folder.

### 1. Get the project

Clone the repository and open PowerShell in the project root, the folder containing `CMakeLists.txt`:

```powershell
git clone https://github.com/fathyy7328/Graphics-Project-.git
cd Graphics-Project-
```

### 2. Install FreeType

Open the **MSYS2 UCRT64** terminal (not PowerShell) and install the UCRT64 FreeType package:

```bash
pacman -Syu
```

If MSYS2 asks you to close the terminal after updating, close it, reopen **MSYS2 UCRT64**, and run the update command again. Then install FreeType:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-freetype
```

### 3. Configure with CMake

In PowerShell at the project root, configure the project. This example assumes the course MinGW tools were installed in the default `%USERPROFILE%\devtools` folder and MSYS2 is installed at `C:\msys64`:

```powershell
cmake -S . -B build_graphics_game -G "MinGW Makefiles" -DCMAKE_C_COMPILER="$env:USERPROFILE\devtools\mingw\mingw64\bin\gcc.exe" -DCMAKE_CXX_COMPILER="$env:USERPROFILE\devtools\mingw\mingw64\bin\g++.exe" -DCMAKE_PREFIX_PATH="C:\msys64\ucrt64"
```

If the tools or MSYS2 are installed in different locations, replace those paths with the actual locations. The FreeType prefix must point to the MSYS2 `ucrt64` folder so CMake can find `ft2build.h` and the FreeType library.

### 4. Build

Run this from the project root in PowerShell:

```powershell
cmake --build build_graphics_game --target GardenTowerDefense
```

### 5. Run

The executable needs FreeType's DLL at runtime. In the same PowerShell window, add the MSYS2 UCRT64 `bin` folder to `PATH`, then run the game:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
.\build_graphics_game\GardenTowerDefense.exe
```

Run the game while the current directory is the project root so relative paths to the font and shader files resolve correctly. The `PATH` command only affects that PowerShell window; repeat it in a new window before launching the game.

### Common FreeType errors

- **`ft2build.h` not found:** confirm the UCRT64 FreeType package is installed and configure with `-DCMAKE_PREFIX_PATH="C:\msys64\ucrt64"`.
- **`libfreetype-6.dll` not found or the game does not start:** add `C:\msys64\ucrt64\bin` to `PATH` in the launch terminal as shown above.
- **CMake keeps using the wrong compiler or FreeType location:** configure a fresh build folder with the correct compiler and prefix paths rather than reusing a build folder configured with different tool locations.

## Third-party notices

Attribution and license information for bundled LearnOpenGL code and the Antonio font is in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The font's SIL Open Font License is included beside the font in `assets/fonts/OFL.txt`.
