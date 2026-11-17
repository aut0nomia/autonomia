#include "game_loop.hpp"
#include "config.hpp"
#include "game_state.hpp"
#include "input.hpp"
#include "map.hpp"
#include "physics.hpp"
#include "renderer.hpp"

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <chrono>
#include <cstdio>

using namespace std::chrono_literals;

constexpr std::chrono::nanoseconds timestep(std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::duration<double>(1.0 / FRAMERATE)));

void run_game() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return;
  }

  // Request OpenGL 3.3 Core Profile
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  // Create window with OpenGL flag (instead of SDL_WINDOW_SHOWN)
  SDL_Window *window = SDL_CreateWindow(
      "Autonomia", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return;
  }

  // Create the OpenGL context (replaces SDL_CreateRenderer)
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (gl_context == nullptr) {
    SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
  }

  // Initialize GLAD — loads all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::fprintf(stderr, "Failed to initialize GLAD\n");
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
  }

  // Enable VSync
  SDL_GL_SetSwapInterval(1);

  // Set up OpenGL viewport and shaders
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  renderer_init();

  using clock = std::chrono::high_resolution_clock;

  std::chrono::nanoseconds lag(0ns);
  auto time_start = clock::now();
  bool quit_game = false;

  game_state current_state;
  generate_random_level(current_state);
  renderer_update_map();

  game_state previous_state = current_state;

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

    render(window, interpolated_state);
  }

  renderer_cleanup();
  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
