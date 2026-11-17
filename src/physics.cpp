#include "physics.hpp"
#include "SDL2/SDL.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "config.hpp"

constexpr float speed = 200.0f; // pixels/second

void jump(game_state &state) {
  if (state.player_y >= WINDOW_HEIGHT - 30.0f) {
    state.velocity_y = -JUMP_FORCE;
  }
}

void update(game_state &state) {
  const Uint8 *keys = SDL_GetKeyboardState(nullptr);
  if (keys[SDL_SCANCODE_W]) {
    jump(state);
  }
  if (keys[SDL_SCANCODE_S]) {
    state.player_y += speed * DT;
  }
  if (keys[SDL_SCANCODE_A]) {
    state.player_x -= speed * DT;
  }
  if (keys[SDL_SCANCODE_D]) {
    state.player_x += speed * DT;
  }

  // apply gravity
  state.velocity_y += GRAVITY * DT;
  state.player_y += state.velocity_y * DT;

  // apply window borders
  if (state.player_y >= WINDOW_HEIGHT - 30.0f) {
    state.player_y = WINDOW_HEIGHT - 30.0f;
    state.velocity_y = 0.0f; // stop falling on landing
  }
  if (state.player_y <= 10.0f)
    state.player_y = 10.0f;
  if (state.player_x >= WINDOW_WIDTH - 10.0f)
    state.player_x = WINDOW_WIDTH - 10.0f;
  if (state.player_x <= 10.0f)
    state.player_x = 10.0f;
}
