#pragma once
#include "render.hpp"
#include "save_slots.hpp"
namespace frontier {
struct MenuOptions {
  std::filesystem::path directory,preferred,settingsPath,screenshot;
  Settings settings;
  int frames{};
  std::string preview;
};
std::optional<std::filesystem::path> mainMenu(MenuOptions& options);
}
