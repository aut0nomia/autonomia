#include "map.hpp"
#include <random>
#include <algorithm>

static std::mt19937 rng(1337);

bool is_solid_tile(int col, int row) {
  if (row >= MAP_ROWS) return true; // bottom is solid floor
  if (col < 0 || col >= MAP_COLS || row < 0) {
    return false; // Open air on left, right, and top
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
  // 1. Clear everything to air '.'
  for (int r = 0; r < MAP_ROWS; ++r) {
    for (int c = 0; c < MAP_COLS; ++c) {
      g_map[r][c] = '.';
    }
  }

  // 2. Bottom level of the map: one big solid platform (bottom 16 rows)
  constexpr int FLOOR_ROWS = 16;
  int floor_top = MAP_ROWS - FLOOR_ROWS;
  for (int r = floor_top; r < MAP_ROWS; ++r) {
    for (int c = 0; c < MAP_COLS; ++c) {
      g_map[r][c] = '#';
    }
  }

  // 3. Generate Vertical Walls / Wall Jump Pillars & Shafts
  // Several tall vertical walls extending upwards
  std::uniform_int_distribution<int> wall_col_dist(30, MAP_COLS - 60);
  std::uniform_int_distribution<int> wall_height_dist(45, 95);
  std::uniform_int_distribution<int> wall_thick_dist(3, 5);

  // A) Generate 3-4 tall vertical pillars from the floor up
  for (int i = 0; i < 4; ++i) {
    int start_c = wall_col_dist(rng);
    int wall_h = wall_height_dist(rng);
    int thick = wall_thick_dist(rng);

    for (int r = floor_top - wall_h; r < floor_top; ++r) {
      if (r >= 0) {
        for (int c = start_c; c < start_c + thick && c < MAP_COLS; ++c) {
          g_map[r][c] = '#';
        }
      }
    }

    // B) With 50% chance, create a parallel wall next to it (a Wall-Jump Chute/Shaft!)
    // Spaced 10-14 tiles away — perfect for ping-pong wall jumping
    if (rng() % 2 == 0 && start_c + thick + 14 < MAP_COLS) {
      int shaft_c = start_c + thick + 11;
      for (int r = floor_top - wall_h; r < floor_top; ++r) {
        if (r >= 0) {
          for (int c = shaft_c; c < shaft_c + thick && c < MAP_COLS; ++c) {
            g_map[r][c] = '#';
          }
        }
      }
    }
  }

  // 4. Procedural randomized horizontal platforms at different heights
  std::uniform_int_distribution<int> width_dist(18, 45);
  std::uniform_int_distribution<int> col_dist(10, MAP_COLS - 55);
  std::uniform_int_distribution<int> count_dist(2, 4);

  int highest_platform_row = floor_top;
  int highest_platform_col = MAP_COLS / 2;

  for (int r = floor_top - 20; r >= 15; r -= 18) {
    int num_plats = count_dist(rng);
    for (int i = 0; i < num_plats; ++i) {
      int start_c = col_dist(rng);
      int plat_w = width_dist(rng);
      int end_c = std::min(start_c + plat_w, MAP_COLS - 2);

      // Create a 3-tile thick platform with solid vertical side edges
      for (int row_offset = 0; row_offset < 3; ++row_offset) {
        int pr = r + row_offset;
        if (pr < MAP_ROWS) {
          for (int c = start_c; c < end_c; ++c) {
            g_map[pr][c] = '#';
          }
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

  // 6. Spawn player on the bottom floor in open air
  state.player_x = 25.0f * TILE_SIZE_X;
  state.player_y = (floor_top - 2) * TILE_SIZE_Y;
  state.velocity_x = 0.0f;
  state.velocity_y = 0.0f;
}
