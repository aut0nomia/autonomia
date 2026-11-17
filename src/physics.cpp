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
constexpr float WALL_JUMP_HORIZONTAL = 700.0f * SCALE; // Strong push-off

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
  float min_x = state.player_x - PLAYER_HALF_W + 1.0f;
  float max_x = state.player_x + PLAYER_HALF_W - 1.0f;
  float feet_y = state.player_y + PLAYER_HALF_H;

  return box_collides(min_x, feet_y, max_x, feet_y + 3.0f * SCALE);
}

// Generous wall detection margin (up to 5 pixels)
bool touching_wall_left(game_state const &state) {
  float left_x = state.player_x - PLAYER_HALF_W;
  float min_y = state.player_y - PLAYER_HALF_H + 2.0f;
  float max_y = state.player_y + PLAYER_HALF_H - 2.0f;
  float margin = 5.0f * SCALE;
  return box_collides(left_x - margin, min_y, left_x, max_y);
}

bool touching_wall_right(game_state const &state) {
  float right_x = state.player_x + PLAYER_HALF_W;
  float min_y = state.player_y - PLAYER_HALF_H + 2.0f;
  float max_y = state.player_y + PLAYER_HALF_H - 2.0f;
  float margin = 5.0f * SCALE;
  return box_collides(right_x, min_y, right_x + margin, max_y);
}

static bool prev_w_key = false;

void jump(game_state &state) {
  if (is_on_ground(state)) {
    state.velocity_y = -JUMP_FORCE;
  } else if (touching_wall_left(state)) {
    // Kick UP and strongly RIGHT
    state.velocity_y = -JUMP_FORCE * 1.05f;
    state.velocity_x = WALL_JUMP_HORIZONTAL;
  } else if (touching_wall_right(state)) {
    // Kick UP and strongly LEFT
    state.velocity_y = -JUMP_FORCE * 1.05f;
    state.velocity_x = -WALL_JUMP_HORIZONTAL;
  }
}

void update(game_state &state) {
  const Uint8 *keys = SDL_GetKeyboardState(nullptr);
  bool w_pressed = keys[SDL_SCANCODE_W];

  // Jump triggers on key press edge OR continuous if on ground
  if (w_pressed && (!prev_w_key || is_on_ground(state))) {
    jump(state);
  }
  prev_w_key = w_pressed;

  // Horizontal input
  float input_x = 0.0f;
  if (keys[SDL_SCANCODE_A]) input_x -= speed;
  if (keys[SDL_SCANCODE_D]) input_x += speed;

  // When wall jumping, give priority to the wall kick impulse
  float total_vx = input_x + state.velocity_x;
  state.velocity_x *= 0.90f; // decay kick over a few frames
  if (std::abs(state.velocity_x) < 10.0f) state.velocity_x = 0.0f;

  float move_x = total_vx * DT;

  // --- Move X with sub-stepping ---
  if (move_x != 0.0f) {
    float step = (move_x > 0 ? 1.0f : -1.0f) * std::min(std::abs(move_x), TILE_SIZE_X * 0.4f);
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
        break;
      } else {
        state.player_x = next_x;
        remaining -= current_step;
      }
    }
  }

  // --- Vertical Movement & Wall Slide ---
  bool on_wall = (!is_on_ground(state)) && (touching_wall_left(state) || touching_wall_right(state));
  
  // If sliding down a wall, cap fall speed (wall slide friction)
  if (on_wall && state.velocity_y > 0.0f) {
    state.velocity_y += (GRAVITY * 0.4f) * DT; // lighter gravity against wall
    state.velocity_y = std::min(state.velocity_y, 250.0f * SCALE); // slow slide down
  } else {
    state.velocity_y += GRAVITY * DT;
  }

  float move_y = state.velocity_y * DT;

  if (move_y != 0.0f) {
    float step = (move_y > 0 ? 1.0f : -1.0f) * std::min(std::abs(move_y), TILE_SIZE_Y * 0.4f);
    float remaining = move_y;

    while (std::abs(remaining) > 0.001f) {
      float current_step = (std::abs(remaining) < std::abs(step)) ? remaining : step;
      float next_y = state.player_y + current_step;

      float min_x = state.player_x - PLAYER_HALF_W + 1.0f;
      float max_x = state.player_x + PLAYER_HALF_W - 1.0f;
      float min_y = next_y - PLAYER_HALF_H;
      float max_y = next_y + PLAYER_HALF_H;

      if (box_collides(min_x, min_y, max_x, max_y)) {
        state.velocity_y = 0.0f;
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
