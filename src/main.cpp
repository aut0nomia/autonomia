<<<<<<< Updated upstream
#include "task.hpp"

#include <algorithm>
=======
#include <algorithm>
#include <format>
>>>>>>> Stashed changes
#include <iostream>
#include <vector>

int main() {
  auto tasks = std::vector<Task>{
      {.name = "Configure CMakePresets", .priority = 1},
      {.name = "Verify clangd inlay hints", .priority = 2},
      {.name = "Test build across editors", .priority = 3},
  };

  std::ranges::sort(tasks, {}, &Task::priority);

  for (const auto &task : tasks) {
    std::cout << format_status(task) << '\n';
  }

  return 0;
}
