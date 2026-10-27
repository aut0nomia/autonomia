#include "game_loop.hpp"
#include "game_state.hpp"
#include "input.hpp"
#include "physics.hpp"
#include "renderer.hpp"

#include <SDL2/SDL.h>
#include <chrono>

using namespace std::chrono_literals;

constexpr std::chrono::nanoseconds timestep(16ms);

void run_game() {
  // TODO: SDL_Init, SDL_CreateWindow, SDL_CreateRenderer

  using clock = std::chrono::high_resolution_clock;

  std::chrono::nanoseconds lag(0ns);
  auto time_start = clock::now();
  bool quit_game = false;

  game_state current_state;
  game_state previous_state;

  SDL_Event event;

  while (!quit_game) {
    auto now = clock::now();
    auto delta_time = now - time_start;
    time_start = now;
    lag += std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time);

    quit_game = handle_events(event);

    while (lag >= timestep) {
      lag -= timestep;

      previous_state = current_state;
      update(current_state);
    }

    auto alpha = (float)lag.count() / timestep.count();
    auto interpolated_state = interpolate(current_state, previous_state, alpha);

    // TODO: pass your SDL_Renderer* to render()
    // render(renderer, interpolated_state);
  }

  // TODO: SDL_DestroyRenderer, SDL_DestroyWindow, SDL_Quit
}
