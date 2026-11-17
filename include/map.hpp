#pragma once

#include "config.hpp"
#include "game_state.hpp"
#include <vector>

// Map dimension: 480 columns x 270 rows
inline char g_map[MAP_ROWS][MAP_COLS];

// Generates a randomized level and sets player spawn & goal
void generate_random_level(game_state &state);

bool is_solid_tile(int col, int row);
bool is_solid_at(float x, float y);
const char* get_map_buffer();
