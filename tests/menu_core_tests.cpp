#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "controls.hpp"
#include "json.hpp"
#include "progress.hpp"
#include "save_slots.hpp"
namespace {
void Check(bool value, const char* why) {
  if (!value) {
    throw std::runtime_error(why);
  }
}
}  // namespace
int main() {
  auto root =
      std::filesystem::temp_directory_path() /
      ("frontier-menu-core-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    auto bindings = frontier::DefaultBindings();
    Check(frontier::ValidBindings(bindings), "Defaults have no collisions");
    Check(!frontier::SetBinding(bindings, frontier::InputAction::kTalk, 0, 80,
                                false),
          "Conflict requires explicit swap");
    Check(frontier::SetBinding(bindings, frontier::InputAction::kTalk, 0, 80,
                               true),
          "Swap accepted");
    Check(
        bindings[static_cast<int>(frontier::InputAction::kHall)].primary == 69,
        "Swap preserves other action");
    Check(!frontier::SetBinding(bindings, frontier::InputAction::kTalk, 0, 256,
                                true),
          "Escape stays reserved");
    frontier::Settings settings;
    settings.bindings = bindings;
    settings.text_scale = 1.2f;
    settings.high_contrast = true;
    frontier::WriteSettings(root / "settings.json", settings);
    auto loaded_settings = frontier::LoadSettings(root / "settings.json");
    Check(loaded_settings.high_contrast && loaded_settings.text_scale == 1.2f &&
              loaded_settings.bindings[5].primary == 80,
          "Settings and control roundtrip");
    nlohmann::json settings_json;
    {
      std::ifstream in(root / "settings.json");
      in >> settings_json;
    }
    Check(!settings_json.contains("dialogues"),
          "Deprecated dialogue switch is removed");
    auto old_bindings = settings_json;
    for (const auto* id : {"nation", "residents", "inventory"}) {
      old_bindings["bindings"].erase(id);
    }
    old_bindings["bindings"]["talk"] = {78, 0};
    {
      std::ofstream out(root / "old-bindings.json");
      out << old_bindings;
    }
    auto upgraded = frontier::LoadSettings(root / "old-bindings.json");
    Check(upgraded.high_contrast && upgraded.text_scale == 1.2f &&
              upgraded.bindings[static_cast<int>(frontier::InputAction::kTalk)]
                      .primary == 78 &&
              frontier::ValidBindings(upgraded.bindings),
          "New actions keep pre-existing custom bindings without collisions");
    // Older settings keep their useful options and discard the retired switch.
    settings_json.erase("bindings");
    settings_json.erase("text_scale");
    settings_json.erase("high_contrast");
    settings_json["dialogues"] = true;
    {
      std::ofstream out(root / "legacy-settings.json");
      out << settings_json;
    }
    auto legacy_settings =
        frontier::LoadSettings(root / "legacy-settings.json");
    Check(legacy_settings.port == settings.port &&
              frontier::ValidBindings(legacy_settings.bindings) &&
              legacy_settings.text_scale == 1,
          "Legacy settings receive new control/accessibility defaults");
    auto slots = root / "saves";
    auto original = frontier::CreateSaveSlot(slots, "Original", 742019);
    auto save = frontier::LoadSave(original);
    Check(!save.history.empty(), "New world history is initialized");
    auto npc_count = frontier::MakeNpcs().size();
    auto reports = frontier::TownReports(save, frontier::MakeNpcs());
    Check(reports.size() == 1 && reports[0].owned && reports[0].residents > 0 &&
              reports[0].residents < static_cast<int>(npc_count),
          "Only discovered towns are reported, residents exclude visitors and "
          "rulers");
    auto learned = frontier::UnlockProgress(save, "gathering");
    Check(learned.ok, "Tree unlock uses authoritative lesson rules");
    auto history = save.history.size();
    Check(!frontier::UnlockProgress(save, "gathering").ok &&
              save.history.size() == history,
          "Duplicate unlock has no reward/history mutation");
    int coins = save.coins;
    Check(!frontier::UnlockProgress(save, "ironworking").ok &&
              save.coins == coins,
          "Tree cannot bypass a town teacher");
    save.inventory.wood = 8;
    frontier::RememberResources(save);
    save.inventory.wood = 0;
    Check(save.known_goods[0] &&
              frontier::ResourceCatalog(save)[0].name == "Wood",
          "Discovery survives spending stock");
    frontier::WriteSave(original, save);
    auto copy = frontier::CopySaveSlot(slots, original, "Copy");
    Check(frontier::LoadSave(copy).skills.contains("gathering") &&
              frontier::LoadSave(copy).history.size() == history,
          "Copy preserves progression and history");
    auto renamed = frontier::RenameSaveSlot(slots, copy, "Renamed");
    Check(!std::filesystem::exists(copy) && std::filesystem::exists(renamed),
          "Rename moves only the selected world");
    auto exported = root / "export.json";
    frontier::ExportSaveSlot(slots, original, exported);
    bool blocked = false;
    try {
      frontier::ExportSaveSlot(slots, original, exported);
    } catch (const std::exception&) {
      blocked = true;
    }
    Check(blocked, "Export never overwrites an existing file");
    auto imported = frontier::ImportSaveSlot(slots, exported, "Imported");
    Check(frontier::LoadSave(imported).known_goods[0],
          "Import retains discovered resources");
    {
      std::ofstream out(root / "bad.json");
      out << "{}";
    }
    auto before = frontier::ScanSaveSlots(slots).size();
    blocked = false;
    try {
      frontier::ImportSaveSlot(slots, root / "bad.json", "Bad");
    } catch (const std::exception&) {
      blocked = true;
    }
    Check(blocked && frontier::ScanSaveSlots(slots).size() == before,
          "Invalid import cannot create a slot");
    frontier::DeleteSaveSlot(slots, renamed);
    auto recovered = frontier::ScanRecoveredSlots(slots);
    Check(recovered.size() == 1 && recovered[0].save.has_value(),
          "Deleted world appears in recovery");
    auto restored =
        frontier::RestoreSaveSlot(slots, recovered[0].path, "Restored");
    Check(frontier::LoadSave(restored).skills.contains("gathering") &&
              std::filesystem::exists(original) &&
              std::filesystem::exists(recovered[0].path),
          "Recovery preserves existing worlds and recovery source");
    nlohmann::json legacy;
    {
      std::ifstream in(original);
      in >> legacy;
    }
    // A real v5 avatar was not simultaneously an assigned resident.
    int active_npc = legacy["nation"].value("controlled", -1);
    for (auto& c : legacy["citizens"]) {
      if (c["npc"] == active_npc) {
        c["weapon"] = "";
        c["outfit"] = "";
      }
    }
    legacy["version"] = 5;
    legacy.erase("known_goods");
    legacy.erase("history");
    auto old = slots / "legacy.json";
    {
      std::ofstream out(old);
      out << legacy;
    }
    auto migrated = frontier::LoadSave(old);
    frontier::WriteSave(old, migrated);
    Check(std::filesystem::exists(old.string() + ".v5.bak") &&
              frontier::LoadSave(old).skills.contains("gathering"),
          "v5 migration preserves backup and skills");
    for (int i = 0; i < 520; ++i) {
      frontier::RecordHistory(save, "test", std::to_string(i), save.home_city);
    }
    Check(save.history.size() == 512 &&
              save.history.front().description == "8" &&
              save.history.back().description == "519",
          "Bounded history keeps the most recent events");
    frontier::WriteSave(original, save);
    Check(frontier::LoadSave(original).history.size() == 512,
          "Maximum retained history roundtrips");
    nlohmann::json malformed;
    {
      std::ifstream in(original);
      in >> malformed;
    }
    malformed["history"][0]["city"] = 1000000;
    {
      std::ofstream out(root / "bad-history.json");
      out << malformed;
    }
    before = frontier::ScanSaveSlots(slots).size();
    blocked = false;
    try {
      frontier::ImportSaveSlot(slots, root / "bad-history.json", "Bad history");
    } catch (const std::exception&) {
      blocked = true;
    }
    Check(blocked && frontier::ScanSaveSlots(slots).size() == before,
          "Malformed history cannot create an imported slot");
    std::filesystem::remove_all(root);
    std::cout << "Controls, progression, history, migration and save "
                 "operations passed\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    std::filesystem::remove_all(root);
    return 1;
  }
}
