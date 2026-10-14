#ifndef FOREST_FRONTIER_SRC_MENU_HPP_
#define FOREST_FRONTIER_SRC_MENU_HPP_

#include <filesystem>
#include <optional>
#include <string>

#include "render.hpp"
#include "save_slots.hpp"

namespace frontier {
struct MenuOptions {
  std::filesystem::path directory;
  std::filesystem::path preferred;
  std::filesystem::path settings_path;
  std::filesystem::path screenshot;
  Settings settings;
  int frames{};
  std::string preview;
};
std::optional<std::filesystem::path> MainMenu(MenuOptions& options);
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_MENU_HPP_
