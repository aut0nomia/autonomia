#pragma once

constexpr int WINDOW_WIDTH = 1920;
constexpr int WINDOW_HEIGHT = 1080;
constexpr int FRAMERATE = 60;
constexpr float DT = 1.0f / FRAMERATE;

// Grid: 1920 / 4 = 480 cols, 1080 / 4 = 270 rows
constexpr int MAP_COLS = 1920 / 4;
constexpr int MAP_ROWS = 1080 / 4;

constexpr float TILE_SIZE_X = static_cast<float>(WINDOW_WIDTH) / MAP_COLS;
constexpr float TILE_SIZE_Y = static_cast<float>(WINDOW_HEIGHT) / MAP_ROWS;

// Scaling helpers
constexpr float SCALE = static_cast<float>(WINDOW_HEIGHT) / 1080.0f;
constexpr float GRAVITY = 1500.0f * SCALE;
constexpr float JUMP_FORCE = 750.0f * SCALE;
