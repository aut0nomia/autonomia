#include "task.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

int main() {
  Task task{.name = "Test task", .priority = 1};
  assert(format_status(task) == "[Priority 1] Test task");

  std::vector<Task> tasks{
      {.name = "Task B", .priority = 2},
      {.name = "Task A", .priority = 1},
  };

  std::ranges::sort(tasks, {}, &Task::priority);
  assert(tasks[0].name == "Task A");
  assert(tasks[1].name == "Task B");

  std::cout << "All tests passed successfully.\n";
  return 0;
}
