#include "game_loop.hpp"
#include <string>
#include <optional>
#include <iostream>

int main(int argc, char **argv) {
  std::optional<unsigned int> seed;
  if (argc > 1) {
    try {
      seed = static_cast<unsigned int>(std::stoul(argv[1]));
      std::cout << "Starting game with custom seed: " << *seed << std::endl;
    } catch (...) {
      std::cerr << "Invalid seed provided: " << argv[1] << ", using random seed." << std::endl;
    }
  }

  run_game(seed);
  return 0;
}
