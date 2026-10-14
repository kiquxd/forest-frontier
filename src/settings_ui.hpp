#ifndef FOREST_FRONTIER_SRC_SETTINGS_UI_HPP_
#define FOREST_FRONTIER_SRC_SETTINGS_UI_HPP_
#include <filesystem>
#include <string>

#include "render.hpp"
#include "save_slots.hpp"
namespace frontier {
struct SettingsPanel {
  Settings draft;
  int selection{};
  int binding_row{};
  int binding_slot{};
  bool controls{};
  bool capturing{};
  int pending_key{-1};
  int conflict{-1};
  std::string notice;
};
// Nonblocking frame UI, shared by the lobby and pause screen.
// Return true only when the panel is closed. Apply writes settings atomically.
bool DrawSettingsPanel(Renderer& renderer, SettingsPanel& panel,
                       Settings& settings, const std::filesystem::path& path);
}  // namespace frontier
#endif  // FOREST_FRONTIER_SRC_SETTINGS_UI_HPP_
