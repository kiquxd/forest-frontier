#pragma once
#include "gameplay.hpp"
#include <optional>

namespace frontier {
struct Settings {
  int width{1600},height{900},fps{60},port{8080};
  float zoom{1.15f};
  bool fullscreen{},debug{true},dialogues{true};
  std::string lastSave;
};
Settings loadSettings(const std::filesystem::path& path);
void writeSettings(const std::filesystem::path& path,const Settings& settings);
struct SaveSlot {
  std::filesystem::path path;
  std::filesystem::file_time_type modified{};
  std::optional<Save> save;
  std::vector<Settlement> towns;
  std::string error;
  int discovered{},owned{};
};
std::vector<SaveSlot> scanSaveSlots(const std::filesystem::path& directory);
std::filesystem::path createSaveSlot(const std::filesystem::path& directory,const std::string& name,uint64_t seed,const std::string& background="traveler");
void deleteSaveSlot(const std::filesystem::path& directory,const std::filesystem::path& path);
}
