#pragma once

#include <array>

// Shared top-level screen states used by the game modules.
enum class GAME_STATE {
    Main_Menu,
    Cutscene,
    Playing,
    Paused,
    Win,
    Lose
};

// The current screen. Member 1 creates it in core/main.cpp; other modules can read it.
extern GAME_STATE G_STATE;

// Shared first-pass layout values for the Garden Tower Defense game.
// Positions use OpenGL normalized device coordinates: left/bottom = -1,
// right/top = +1. Positive Y points upward.
namespace GameConfig {

inline constexpr int WINDOW_WIDTH = 800;
inline constexpr int WINDOW_HEIGHT = 600;

inline constexpr int LANE_COUNT = 3;
inline constexpr std::array<float, LANE_COUNT> LANE_CENTER_Y = {
    0.56f,  // Top lane
    0.00f,  // Middle lane
    -0.56f  // Bottom lane
};
inline constexpr float LANE_HALF_HEIGHT = 0.15f;

inline constexpr float WORLD_LEFT = -1.0f;
inline constexpr float WORLD_RIGHT = 1.0f;
inline constexpr float HOME_BASE_LEFT = -1.0f;
inline constexpr float HOME_BASE_RIGHT = -0.56f;
inline constexpr float PLAYFIELD_LEFT = -0.52f;
inline constexpr float PLAYFIELD_RIGHT = 1.0f;

inline constexpr float TROPHY_CENTER_X = -0.78f;
inline constexpr float TROPHY_CENTER_Y = 0.0f;
inline constexpr float PLAYER_START_X = -0.36f;
inline constexpr float ENEMY_SPAWN_X = 0.90f;

} // namespace GameConfig

// Shared functions used by more than one member.

// Member 2 (world/World.cpp): draws a health bar centered at (x, y).
// Call it after drawing the character so the bar shows on top.
void drawHealthBar(float x, float y, int hp, int maxHp);
