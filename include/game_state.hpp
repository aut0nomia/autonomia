#pragma once

struct game_state {
  float player_x{200.0f};
  float player_y{800.0f};
  float velocity_x{0.0f};
  float velocity_y{0.0f};

  // Goal position in window pixel coordinates
  float goal_x{1000.0f};
  float goal_y{200.0f};
  int level{1};
  unsigned int seed{0};
};
