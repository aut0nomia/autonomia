#include "physics.hpp"
#include "SDL2/SDL.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "config.hpp"

constexpr float speed = 5.0f;

void update(game_state &state) {
  const Uint8 *keys = SDL_GetKeyboardState(nullptr);
  if (keys[SDL_SCANCODE_W]) {
    state.player_y -= speed;
  }
  if (keys[SDL_SCANCODE_S]) {
    state.player_y += speed;
  }

  if (keys[SDL_SCANCODE_A]) {
    state.player_x -= speed;
  }
  if (keys[SDL_SCANCODE_D]) {
    state.player_x += speed;
  }
  if (state.player_y >= WINDOW_HEIGHT - 10.0f)
    state.player_y = WINDOW_HEIGHT - 10.0f;
  if (state.player_y <= 10.0f)
    state.player_y = 10.0f;
  if (state.player_x >= WINDOW_WIDTH - 10.0f)
    state.player_x = WINDOW_WIDTH - 10.0f;
  if (state.player_x <= 10.0f)
    state.player_x = 10.0f;
}
