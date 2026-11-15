#include "renderer.hpp"

void render(SDL_Renderer *renderer, game_state const &state) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_Rect player_rect{
      static_cast<int>(state.player_x) - 10,
      static_cast<int>(state.player_y) - 10,
      20,
      20,
  };
  SDL_RenderFillRect(renderer, &player_rect);

  SDL_RenderPresent(renderer);
}

game_state interpolate(game_state const &current, game_state const &previous,
                       float alpha) {
  game_state result;
  result.player_x =
      current.player_x * alpha + previous.player_x * (1.0f - alpha);
  result.player_y =
      current.player_y * alpha + previous.player_y * (1.0f - alpha);
  result.velocity_x = current.velocity_x;
  result.velocity_y = current.velocity_y;
  return result;
}
