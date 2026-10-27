#pragma once

#include "game_state.hpp"

#include <SDL2/SDL.h>

void render(SDL_Renderer *renderer, game_state const &state);

game_state interpolate(game_state const &current, game_state const &previous,
                       float alpha);
