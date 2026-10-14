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
#include "progress.hpp"

namespace frontier {
namespace {
void Validate(const Settings& settings) {
  if (settings.width < 900 || settings.width > 3840 || settings.height < 650 ||
      settings.height > 2160 || settings.fps < 0 || settings.fps > 240 ||
      settings.port < 1 || settings.port > 65535 ||
      !std::isfinite(settings.text_scale) || settings.text_scale < 1 ||
      settings.text_scale > 1.4f || !ValidBindings(settings.bindings) ||
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
    settings.text_scale = json.value("text_scale", 1.f);
    settings.high_contrast = json.value("high_contrast", false);
    if (json.contains("bindings")) {
      for (const auto& action : InputActions()) {
        if (json["bindings"].contains(action.id)) {
          auto& row = json["bindings"][action.id];
          settings.bindings[static_cast<int>(action.action)] = {row.at(0),
                                                                row.at(1)};
        }
      }
    }
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
                         {"text_scale", settings.text_scale},
                         {"high_contrast", settings.high_contrast},
                         {"last_save", settings.last_save}};
  json["bindings"] = nlohmann::json::object();
  for (const auto& action : InputActions()) {
    auto b = settings.bindings[static_cast<int>(action.action)];
    json["bindings"][action.id] = {b.primary, b.alternative};
  }
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
  RecordHistory(save, "created",
                "Founded " + Settlements()[save.home_city].name,
                save.home_city);
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
namespace {
void ValidateSource(const std::filesystem::path& source) {
  if (std::filesystem::is_symlink(source) ||
      !std::filesystem::is_regular_file(source) ||
      std::filesystem::file_size(source) > 128 * 1024 * 1024) {
    throw std::runtime_error("Select a regular save file (up to 128 MiB)");
  }
}
void ValidateLocalSlot(const std::filesystem::path& directory,
                       const std::filesystem::path& source) {
  ValidateSource(source);
  if (source.extension() != ".json" ||
      std::filesystem::weakly_canonical(source.parent_path()) !=
          std::filesystem::weakly_canonical(directory)) {
    throw std::runtime_error("Invalid save slot path");
  }
}
std::filesystem::path UniqueSlot(const std::filesystem::path& directory,
                                 const std::string& name) {
  std::string slug;
  for (unsigned char c : name) {
    if (std::isalnum(c) || c == '-' || c == '_') {
      slug += static_cast<char>(c);
    } else if (c == ' ') {
      slug += '-';
    }
  }
  if (slug.empty()) {
    throw std::runtime_error("Enter a world name with letters or digits");
  }
  slug.resize(std::min(slug.size(), size_t{48}));
  std::filesystem::create_directories(directory);
  auto path = directory / (slug + ".json");
  for (int suffix = 2; std::filesystem::exists(path); ++suffix) {
    path = directory / (slug + "-" + std::to_string(suffix) + ".json");
  }
  return path;
}
Save CheckedSave(const std::filesystem::path& source) {
  ValidateSource(source);
  return LoadSave(source);
}
}  // namespace
std::filesystem::path RenameSaveSlot(const std::filesystem::path& directory,
                                     const std::filesystem::path& source,
                                     const std::string& name) {
  ValidateLocalSlot(directory, source);
  (void)CheckedSave(source);
  if (name == source.stem().string()) {
    return source;
  }
  auto destination = UniqueSlot(directory, name);
  std::filesystem::rename(source, destination);
  return destination;
}
std::filesystem::path CopySaveSlot(const std::filesystem::path& directory,
                                   const std::filesystem::path& source,
                                   const std::string& name) {
  ValidateLocalSlot(directory, source);
  auto save = CheckedSave(source);
  auto destination = UniqueSlot(directory, name);
  WriteSave(destination, save);
  return destination;
}
std::filesystem::path ImportSaveSlot(const std::filesystem::path& directory,
                                     const std::filesystem::path& source,
                                     const std::string& name) {
  if (source.extension() != ".json") {
    throw std::runtime_error("Import expects a .json world file");
  }
  auto save = CheckedSave(source);
  auto destination =
      UniqueSlot(directory, name.empty() ? source.stem().string() : name);
  WriteSave(destination, save);
  return destination;
}
void ExportSaveSlot(const std::filesystem::path& directory,
                    const std::filesystem::path& source,
                    const std::filesystem::path& destination) {
  ValidateLocalSlot(directory, source);
  if (destination.extension() != ".json" ||
      std::filesystem::exists(destination) ||
      std::filesystem::is_symlink(destination)) {
    throw std::runtime_error(
        "Choose a new .json export path; existing files are preserved");
  }
  auto save = CheckedSave(source);
  WriteSave(destination, save);
}
std::vector<SaveSlot> ScanRecoveredSlots(
    const std::filesystem::path& directory) {
  std::vector<SaveSlot> result;
  for (const auto& folder : {directory, directory / ".trash"}) {
    if (!std::filesystem::is_directory(folder)) {
      continue;
    }
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
      if (entry.is_symlink() || !entry.is_regular_file()) {
        continue;
      }
      bool backup = folder == directory && entry.path().extension() == ".bak";
      bool deleted = folder != directory && entry.path().extension() == ".json";
      if (!backup && !deleted) {
        continue;
      }
      SaveSlot slot;
      slot.path = entry.path();
      slot.modified = entry.last_write_time();
      try {
        slot.save = CheckedSave(slot.path);
      } catch (const std::exception& error) {
        slot.error = error.what();
      }
      result.push_back(std::move(slot));
    }
  }
  std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
    return a.modified > b.modified;
  });
  return result;
}
std::filesystem::path RestoreSaveSlot(const std::filesystem::path& directory,
                                      const std::filesystem::path& source,
                                      const std::string& name) {
  auto parent = std::filesystem::weakly_canonical(source.parent_path());
  bool backup = parent == std::filesystem::weakly_canonical(directory) &&
                source.extension() == ".bak";
  bool deleted =
      parent == std::filesystem::weakly_canonical(directory / ".trash") &&
      source.extension() == ".json";
  if (!backup && !deleted) {
    throw std::runtime_error(
        "Select a backup or deleted world from this save directory");
  }
  auto save = CheckedSave(source);
  auto destination =
      UniqueSlot(directory, name.empty() ? "Restored world" : name);
  WriteSave(destination, save);
  return destination;
}
}  // namespace frontier
