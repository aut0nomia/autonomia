#pragma once

#include <format>
#include <string>
#include <string_view>

struct Task {
  std::string_view name;
  int priority{};
};

inline auto format_status(const Task &task) -> std::string {
  return std::format("[Priority {}] {}", task.priority, task.name);
}
