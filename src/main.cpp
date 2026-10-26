#include <iostream>
#include <string_view>
#include <vector>

using namespace std;

struct Task {
  std::string_view name;
  int priority{};
};

auto format_status(const Task &task) -> std::string {
  return format("[Priority {}] {}", task.priority, task.name);
}

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
