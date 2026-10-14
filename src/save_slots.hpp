#ifndef FOREST_FRONTIER_SRC_SAVE_SLOTS_HPP_
#define FOREST_FRONTIER_SRC_SAVE_SLOTS_HPP_

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "world.hpp"

namespace frontier {
struct Settings {
  int width{1600};
  int height{900};
  int fps{60};
  int port{8080};
  float zoom{1.15f};
  bool fullscreen{};
  bool debug{true};
  bool dialogues{true};
  std::string last_save;
};
Settings LoadSettings(const std::filesystem::path& path);
void WriteSettings(const std::filesystem::path& path, const Settings& settings);
struct SaveSlot {
  std::filesystem::path path;
  std::filesystem::file_time_type modified{};
  std::optional<Save> save;
  std::vector<Settlement> towns;
  std::string error;
  int discovered{};
  int owned{};
};
std::vector<SaveSlot> ScanSaveSlots(const std::filesystem::path& directory);
std::filesystem::path CreateSaveSlot(
    const std::filesystem::path& directory, const std::string& name,
    uint64_t seed, const std::string& background = "traveler");
void DeleteSaveSlot(const std::filesystem::path& directory,
                    const std::filesystem::path& path);
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_SAVE_SLOTS_HPP_
