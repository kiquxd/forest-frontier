#include "save_slots.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "adventure.hpp"
#include "json.hpp"

namespace frontier {
namespace {
void Validate(const Settings& settings) {
  if (settings.width < 900 || settings.width > 3840 || settings.height < 650 ||
      settings.height > 2160 || settings.fps < 0 || settings.fps > 240 ||
      settings.port < 1 || settings.port > 65535 ||
      !std::isfinite(settings.zoom) || settings.zoom < .65f ||
      settings.zoom > 1.7f) {
    throw std::runtime_error("Invalid settings");
  }
}
}  // namespace

Settings LoadSettings(const std::filesystem::path& path) {
  Settings settings;
  if (!std::filesystem::exists(path)) {
    return settings;
  }
  try {
    std::ifstream in(path);
    nlohmann::json json;
    in >> json;
    settings.width = json.value("width", settings.width);
    settings.height = json.value("height", settings.height);
    settings.fps = json.value("fps", settings.fps);
    settings.port = json.value("port", settings.port);
    settings.zoom = json.value("zoom", settings.zoom);
    settings.fullscreen = json.value("fullscreen", settings.fullscreen);
    settings.debug = json.value("debug", settings.debug);
    settings.dialogues = json.value("dialogues", settings.dialogues);
    settings.last_save = json.value("last_save", std::string{});
    Validate(settings);
  } catch (const std::exception&) {
    return Settings{};
  }
  return settings;
}
void WriteSettings(const std::filesystem::path& path,
                   const Settings& settings) {
  Validate(settings);
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  nlohmann::json json = {{"width", settings.width},
                         {"height", settings.height},
                         {"fps", settings.fps},
                         {"port", settings.port},
                         {"zoom", settings.zoom},
                         {"fullscreen", settings.fullscreen},
                         {"debug", settings.debug},
                         {"dialogues", settings.dialogues},
                         {"last_save", settings.last_save}};
  auto tmp = path;
  tmp += ".tmp";
  {
    std::ofstream out(tmp);
    if (!out) {
      throw std::runtime_error("Cannot write settings");
    }
    out << json.dump(2) << '\n';
    out.flush();
    if (!out) {
      throw std::runtime_error("Cannot write settings");
    }
  }
  std::filesystem::rename(tmp, path);
}
std::vector<SaveSlot> ScanSaveSlots(const std::filesystem::path& directory) {
  std::vector<SaveSlot> slots;
  if (!std::filesystem::exists(directory)) {
    return slots;
  }
  for (auto& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_symlink() || !entry.is_regular_file() ||
        entry.path().extension() != ".json") {
      continue;
    }
    SaveSlot slot;
    slot.path = entry.path();
    slot.modified = entry.last_write_time();
    try {
      auto save = LoadSave(slot.path);
      ConfigureWorld(save.seed);
      InitializeCampaign(save, true);
      InitializeAdventure(save);
      slot.towns = Settlements();
      for (auto& r : save.relations) {
        if (r.discovered) {
          slot.discovered++;
        }
      }
      slot.owned = static_cast<int>(
          std::count(save.owners.begin(), save.owners.end(), 0));
      slot.save = std::move(save);
    } catch (const std::exception& e) {
      slot.error = e.what();
    }
    slots.push_back(std::move(slot));
  }
  std::sort(slots.begin(), slots.end(), [](const auto& a, const auto& b) {
    return a.modified == b.modified ? a.path.filename() < b.path.filename()
                                    : a.modified > b.modified;
  });
  return slots;
}
std::filesystem::path CreateSaveSlot(const std::filesystem::path& directory,
                                     const std::string& name, uint64_t seed,
                                     const std::string& background) {
  std::string slug;
  for (unsigned char c : name) {
    if (std::isalnum(c) || c == '-' || c == '_') {
      slug += static_cast<char>(c);
    } else if (c == ' ') {
      slug += '-';
    }
  }
  if (slug.empty()) {
    slug = "new-world";
  }
  if (slug.size() > 48) {
    slug.resize(48);
  }
  std::filesystem::create_directories(directory);
  auto path = directory / (slug + ".json");
  int suffix = 2;
  while (std::filesystem::exists(path)) {
    path = directory / (slug + "-" + std::to_string(suffix++) + ".json");
  }
  Save save;
  save.seed = seed;
  ConfigureWorld(seed);
  InitializeCampaign(save);
  save.character_id = Character(background).id;
  save.coins += Character(background).coins;
  InitializeAdventure(save);
  WriteSave(path, save);
  return path;
}
void DeleteSaveSlot(const std::filesystem::path& directory,
                    const std::filesystem::path& path) {
  if (std::filesystem::is_symlink(path) || path.extension() != ".json" ||
      std::filesystem::weakly_canonical(path.parent_path()) !=
          std::filesystem::weakly_canonical(directory)) {
    throw std::runtime_error("Invalid save slot path");
  }
  auto trash = directory / ".trash";
  std::filesystem::create_directories(trash);
  auto destination =
      trash /
      (path.stem().string() + "-" +
       std::to_string(
           std::chrono::system_clock::now().time_since_epoch().count()) +
       ".json");
  std::filesystem::rename(path, destination);
}
}  // namespace frontier
