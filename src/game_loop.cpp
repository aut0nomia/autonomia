#include "game_loop.hpp"
#include "config.hpp"
#include "game_state.hpp"
#include "input.hpp"
#include "physics.hpp"
#include "renderer.hpp"

#include <SDL2/SDL.h>
#include <chrono>

using namespace std::chrono_literals;

constexpr std::chrono::nanoseconds
    timestep(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::duration<double>(1.0 / FRAMERATE)));

void run_game() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return;
  }

  SDL_Window *window = SDL_CreateWindow("Autonomia", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
                                        WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == nullptr) {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
  }

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

    render(renderer, interpolated_state);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
