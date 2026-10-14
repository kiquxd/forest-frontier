#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "gameplay.hpp"
#include "json.hpp"
#include "save_slots.hpp"

namespace {
void Check(bool ok, const char* message) {
  if (!ok) {
    throw std::runtime_error(message);
  }
}

}  // namespace

int main() {
  auto root =
      std::filesystem::temp_directory_path() /
      ("frontier-slots-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    auto directory = root / "saves";
    Check(frontier::ScanSaveSlots(directory).empty(),
          "Missing save directory is empty");
    auto first = frontier::CreateSaveSlot(directory, "My world", 123),
         second = frontier::CreateSaveSlot(directory, "My world", 456);
    Check(first != second && std::filesystem::exists(first) &&
              std::filesystem::exists(second),
          "Named worlds never overwrite existing slots");
    auto save = frontier::LoadSave(first);
    frontier::ConfigureWorld(save.seed);
    save.skills.insert("gathering");
    save.inventory.wood = 17;
    save.explored.insert(frontier::MapCell(save.player));
    int foreign =
        (save.home_city + 1) % static_cast<int>(save.relations.size());
    save.relations[foreign].discovered = true;
    frontier::WriteSave(first, save);
    {
      std::ofstream out(directory / "broken.json");
      out << "not json";
    }
    {
      std::ofstream out(directory / "ignored.json.tmp");
      out << "temporary";
    }
    std::filesystem::create_symlink(first, directory / "linked.json");
    auto slots = frontier::ScanSaveSlots(directory);
    Check(slots.size() == 3,
          "Only direct JSON slots are listed; temporary files and symlinks "
          "ignored");
    bool summary = false, invalid = false;
    for (auto& slot : slots) {
      if (slot.path == first) {
        summary = slot.save && slot.discovered == 2 && slot.owned == 1 &&
                  slot.save->inventory.wood == 17 &&
                  slot.save->skills.contains("gathering") &&
                  !slot.save->explored.empty() &&
                  slot.towns.size() == slot.save->relations.size();
      }
      if (slot.path.filename() == "broken.json") {
        invalid = !slot.save && !slot.error.empty();
      }
    }
    Check(summary && invalid,
          "Summary preserves explored towns, inventory, skills and map; "
          "corrupt saves do not break browser");
    frontier::Settings settings;
    settings.fps = 144;
    settings.zoom = 1.35f;
    settings.high_contrast = true;
    settings.port = 8090;
    settings.last_save = first.string();
    frontier::WriteSettings(root / "settings.json", settings);
    auto loaded = frontier::LoadSettings(root / "settings.json");
    Check(loaded.fps == 144 && loaded.zoom == 1.35f && loaded.high_contrast &&
              loaded.port == 8090 && loaded.last_save == first.string(),
          "Settings and last played slot persist");
    {
      std::ofstream out(root / "settings.json");
      out << "{\"port\":0}";
    }
    Check(frontier::LoadSettings(root / "settings.json").port == 8080,
          "Invalid settings recover to safe defaults");
    auto outside = frontier::CreateSaveSlot(root / "outside", "Keep me", 789);
    bool rejected = false;
    try {
      frontier::DeleteSaveSlot(directory, outside);
    } catch (const std::exception&) {
      rejected = true;
    }
    Check(rejected && std::filesystem::exists(outside),
          "Deletion cannot target a file outside the slot directory");
    frontier::DeleteSaveSlot(directory, first);
    Check(!std::filesystem::exists(first) &&
              frontier::ScanSaveSlots(directory).size() == 2,
          "Deleted world disappears from browser");
    auto trash = std::filesystem::directory_iterator(directory / ".trash");
    Check(trash != std::filesystem::directory_iterator{} &&
              frontier::LoadSave(trash->path()).inventory.wood == 17,
          "Deleted world remains recoverable with progress intact");
    // Content shown to the model and player is now English, while IDs stay
    // unchanged.
    auto ascii = [](const std::string& s) {
      for (unsigned char c : s) {
        if (c >= 128) {
          return false;
        }
      }
      return true;
    };
    for (auto& npc : frontier::GetContent().npcs) {
      Check(ascii(npc.role) && ascii(npc.personality), "English NPC context");
      for (auto& name : npc.names) {
        Check(ascii(name), "English NPC names");
      }
    }
    for (auto& recipe : frontier::Recipes()) {
      Check(ascii(recipe.name) && ascii(recipe.effect), "English recipe text");
    }
    for (auto& town : frontier::GetContent().towns) {
      Check(ascii(town.name), "English settlement types");
    }
    std::filesystem::remove_all(root);
    std::cout << "slot browser, summary, settings, deletion recovery and "
                 "English content passed\n";
  } catch (const std::exception& e) {
    std::filesystem::remove_all(root);
    std::cerr << e.what() << '\n';
    return 1;
  }
}
