#include "physics.hpp"
#include "renderer.hpp"
#include "SDL2/SDL.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "config.hpp"
#include "map.hpp"
#include <cmath>
#include <algorithm>

constexpr float PLAYER_HALF_W = TILE_SIZE_X * 1.0f;
constexpr float PLAYER_HALF_H = TILE_SIZE_Y * 1.0f;

constexpr float speed = 500.0f * SCALE; // pixels/second
constexpr float WALL_JUMP_HORIZONTAL = 450.0f * SCALE; // push-off speed

// Tests if the player's bounding box [min_x, max_x] x [min_y, max_y] intersects any solid tile
bool box_collides(float min_x, float min_y, float max_x, float max_y) {
  int start_col = static_cast<int>(std::floor(min_x / TILE_SIZE_X));
  int end_col   = static_cast<int>(std::floor(max_x / TILE_SIZE_X));
  int start_row = static_cast<int>(std::floor(min_y / TILE_SIZE_Y));
  int end_row   = static_cast<int>(std::floor(max_y / TILE_SIZE_Y));

  for (int r = start_row; r <= end_row; ++r) {
    for (int c = start_col; c <= end_col; ++c) {
      if (is_solid_tile(c, r)) {
        return true;
      }
    }
  }
  return false;
}

bool is_on_ground(game_state const &state) {
  // Test a 1-pixel strip directly beneath the player's feet
  float min_x = state.player_x - PLAYER_HALF_W + 1.0f;
  float max_x = state.player_x + PLAYER_HALF_W - 1.0f;
  float feet_y = state.player_y + PLAYER_HALF_H;

  return box_collides(min_x, feet_y, max_x, feet_y + 1.5f);
}

bool touching_wall_left(game_state const &state) {
  float left_x = state.player_x - PLAYER_HALF_W;
  float min_y = state.player_y - PLAYER_HALF_H + 1.0f;
  float max_y = state.player_y + PLAYER_HALF_H - 1.0f;
  return box_collides(left_x - 1.5f, min_y, left_x, max_y);
}

bool touching_wall_right(game_state const &state) {
  float right_x = state.player_x + PLAYER_HALF_W;
  float min_y = state.player_y - PLAYER_HALF_H + 1.0f;
  float max_y = state.player_y + PLAYER_HALF_H - 1.0f;
  return box_collides(right_x, min_y, right_x + 1.5f, max_y);
}

void jump(game_state &state) {
  if (is_on_ground(state)) {
    state.velocity_y = -JUMP_FORCE;
  } else if (touching_wall_left(state)) {
    state.velocity_y = -JUMP_FORCE;
    state.velocity_x = WALL_JUMP_HORIZONTAL;
  } else if (touching_wall_right(state)) {
    state.velocity_y = -JUMP_FORCE;
    state.velocity_x = -WALL_JUMP_HORIZONTAL;
  }
}

void update(game_state &state) {
  const Uint8 *keys = SDL_GetKeyboardState(nullptr);
  if (keys[SDL_SCANCODE_W]) {
    jump(state);
  }

  // Horizontal input
  float input_x = 0.0f;
  if (keys[SDL_SCANCODE_A]) input_x -= speed;
  if (keys[SDL_SCANCODE_D]) input_x += speed;

  float total_vx = input_x + state.velocity_x;
  state.velocity_x *= 0.88f;
  if (std::abs(state.velocity_x) < 5.0f) state.velocity_x = 0.0f;

  float move_x = total_vx * DT;

  // --- Move X with sub-stepping to prevent tunneling ---
  if (move_x != 0.0f) {
    float step = (move_x > 0 ? 1.0f : -1.0f) * std::min(std::abs(move_x), TILE_SIZE_X * 0.5f);
    float remaining = move_x;

    while (std::abs(remaining) > 0.001f) {
      float current_step = (std::abs(remaining) < std::abs(step)) ? remaining : step;
      float next_x = state.player_x + current_step;

      float min_x = next_x - PLAYER_HALF_W;
      float max_x = next_x + PLAYER_HALF_W;
      float min_y = state.player_y - PLAYER_HALF_H + 1.0f;
      float max_y = state.player_y + PLAYER_HALF_H - 1.0f;

      if (box_collides(min_x, min_y, max_x, max_y)) {
        state.velocity_x = 0.0f;
        break; // Stop at obstacle
      } else {
        state.player_x = next_x;
        remaining -= current_step;
      }
    }
  }

  // --- Vertical Movement with sub-stepping to prevent tunneling ---
  state.velocity_y += GRAVITY * DT;
  float move_y = state.velocity_y * DT;

  if (move_y != 0.0f) {
    float step = (move_y > 0 ? 1.0f : -1.0f) * std::min(std::abs(move_y), TILE_SIZE_Y * 0.5f);
    float remaining = move_y;

    while (std::abs(remaining) > 0.001f) {
      float current_step = (std::abs(remaining) < std::abs(step)) ? remaining : step;
      float next_y = state.player_y + current_step;

      float min_x = state.player_x - PLAYER_HALF_W + 1.0f;
      float max_x = state.player_x + PLAYER_HALF_W - 1.0f;
      float min_y = next_y - PLAYER_HALF_H;
      float max_y = next_y + PLAYER_HALF_H;

      if (box_collides(min_x, min_y, max_x, max_y)) {
        state.velocity_y = 0.0f; // Landed or hit head
        break;
      } else {
        state.player_y = next_y;
        remaining -= current_step;
      }
    }
  }

  // Clamp within map horizontal bounds
  if (state.player_x < PLAYER_HALF_W) state.player_x = PLAYER_HALF_W;
  if (state.player_x > WINDOW_WIDTH - PLAYER_HALF_W) state.player_x = WINDOW_WIDTH - PLAYER_HALF_W;

  // --- Goal collision check ---
  float dx = std::abs(state.player_x - state.goal_x);
  float dy = std::abs(state.player_y - state.goal_y);
  if (dx <= TILE_SIZE_X * 2.0f && dy <= TILE_SIZE_Y * 2.0f) {
    state.level++;
    generate_random_level(state);
    renderer_update_map();
  }
}
