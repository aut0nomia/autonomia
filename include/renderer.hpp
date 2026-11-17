#pragma once

#include "game_state.hpp"
#include <SDL2/SDL.h>

void renderer_init();
void renderer_cleanup();
void renderer_update_map();

void render(SDL_Window *window, game_state const &state);

game_state interpolate(game_state const &current, game_state const &previous,
                       float alpha);
