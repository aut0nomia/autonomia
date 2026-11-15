#include "input.hpp"

bool handle_events(SDL_Event &event) {
  while (SDL_PollEvent(&event) != 0) {
    if (event.type == SDL_QUIT) {
      return true;
    }
  }
  return false;
}
