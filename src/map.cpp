#include "map.hpp"
#include <random>
#include <algorithm>

static std::mt19937 rng(1337);

bool is_solid_tile(int col, int row) {
  if (row >= MAP_ROWS) return true; // bottom is solid floor
  if (col < 0 || col >= MAP_COLS || row < 0) {
    return false; // Open air on left, right, and top (no walls / ceiling)
  }
  return g_map[row][col] == '#';
}

bool is_solid_at(float x, float y) {
  int col = static_cast<int>(x / TILE_SIZE_X);
  int row = static_cast<int>(y / TILE_SIZE_Y);
  return is_solid_tile(col, row);
}

const char* get_map_buffer() {
  return &g_map[0][0];
}

void generate_random_level(game_state &state) {
  // 1. Clear everything to air '.' (no ceiling, no left/right walls)
  for (int r = 0; r < MAP_ROWS; ++r) {
    for (int c = 0; c < MAP_COLS; ++c) {
      g_map[r][c] = '.';
    }
  }

  // 2. Bottom level of the map: one big solid platform (bottom 16 rows)
  constexpr int FLOOR_ROWS = 16;
  for (int r = MAP_ROWS - FLOOR_ROWS; r < MAP_ROWS; ++r) {
    for (int c = 0; c < MAP_COLS; ++c) {
      g_map[r][c] = '#';
    }
  }

  // 4. Procedural randomized platforms in the middle air space
  // Tier spacing: e.g. every 16 to 22 rows vertically
  std::uniform_int_distribution<int> width_dist(12, 35);
  std::uniform_int_distribution<int> col_dist(4, MAP_COLS - 45);
  std::uniform_int_distribution<int> count_dist(2, 5);

  int highest_platform_row = MAP_ROWS - FLOOR_ROWS;
  int highest_platform_col = MAP_COLS / 2;

  for (int r = MAP_ROWS - FLOOR_ROWS - 18; r >= 15; r -= 18) {
    int num_plats = count_dist(rng);
    for (int i = 0; i < num_plats; ++i) {
      int start_c = col_dist(rng);
      int plat_w = width_dist(rng);
      int end_c = std::min(start_c + plat_w, MAP_COLS - 2);

      // Create a 2-tile thick platform
      for (int c = start_c; c < end_c; ++c) {
        g_map[r][c] = '#';
        if (r + 1 < MAP_ROWS) {
          g_map[r + 1][c] = '#';
        }
      }

      if (r < highest_platform_row) {
        highest_platform_row = r;
        highest_platform_col = (start_c + end_c) / 2;
      }
    }
  }

  // 5. Place Goal on or above the highest platform
  int goal_row = std::max(2, highest_platform_row - 2);
  int goal_col = highest_platform_col;

  state.goal_x = (goal_col + 0.5f) * TILE_SIZE_X;
  state.goal_y = (goal_row + 0.5f) * TILE_SIZE_Y;

  // 6. Spawn player on the bottom floor, safely in the open
  state.player_x = 40.0f * TILE_SIZE_X;
  state.player_y = (MAP_ROWS - FLOOR_ROWS - 2) * TILE_SIZE_Y;
  state.velocity_x = 0.0f;
  state.velocity_y = 0.0f;
}
