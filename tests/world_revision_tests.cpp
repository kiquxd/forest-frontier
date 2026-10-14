#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "adventure.hpp"
#include "atlas_notes.hpp"
#include "gameplay.hpp"
#include "geometry.hpp"
#include "json.hpp"
#include "save_slots.hpp"
#include "vessels.hpp"
namespace {
void Check(bool ok, const char* text) {
  if (!ok) {
    throw std::runtime_error(text);
  }
}
}  // namespace
int main() {
  auto dir = std::filesystem::temp_directory_path() /
             ("frontier-revision-" +
              std::to_string(
                  std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    Check(frontier::CircleBox({11, 11}, 2, {{0, 0}, {10, 10}}),
          "Rounded circle/box corner intersects");
    Check(!frontier::CircleBox({12, 12}, 2, {{0, 0}, {10, 10}}),
          "Box corner has no inflated square collision");
    Check(frontier::CircleCapsule({22, 0}, 3, {{0, 0}, {20, 0}, 1}),
          "Capsule includes end caps");
    Check(frontier::SegmentsDistance({0, 0}, {1000, 1000}, {490, 510},
                                     {510, 490}) == 0,
          "Sight rays detect thin crossing walls without samples");
    std::array<frontier::Vec, 3> triangle = {
        frontier::Vec{0, 0}, {20, 0}, {0, 20}};
    Check(frontier::CirclePolygon({5, 5}, 1, triangle) &&
              !frontier::CirclePolygon({20, 20}, 2, triangle),
          "Polygon interior and exterior");
    frontier::Save save;
    frontier::ConfigureWorld(save.seed);
    frontier::InitializeCampaign(save);
    frontier::InitializeAdventure(save);
    frontier::World world(save.seed);
    world.buildings.clear();
    world.npcs.clear();
    frontier::Chunk chunk;
    chunk.coord = {0, 0};
    chunk.tiles.resize(frontier::kChunkTiles * frontier::kChunkTiles);
    chunk.resources.push_back(
        {101, {160, 160}, frontier::ResourceKind::kOre, 0});
    world.chunks.emplace(chunk.coord, std::move(chunk));
    auto outline = frontier::BuildingOutline({{240, 240}, 0, false});
    auto left = frontier::Project(outline[0] - frontier::Vec{240, 240});
    Check(std::abs(left.x + 46) < .001f && std::abs(left.y + 21) < .001f,
          "Building collision matches atlas plinth pixels");
    world.buildings.push_back({{240, 240}, 0, false});
    Check(!world.Clear({220, 220}, save) && world.Clear({265, 265}, save),
          "Building collision follows shifted isometric plinth");
    auto slid = world.Move({260, 190}, {-30, 40}, save);
    Check(slid.y > 225 && slid.x >= 254,
          "Movement slides down a rectangular facade");
    save.player = {120, 120};
    save.explored.clear();
    save.relations[0].discovered = false;
    frontier::Gameplay game;
    game.Explore(save, 900, 650, 1, &world);
    auto cells = save.explored;
    Check(save.known_deposits.contains(101), "Visible deposits are remembered");
    auto other = save;
    other.explored.clear();
    game.Explore(other, 3840, 2160, .65f, &world);
    Check(cells == other.explored,
          "Discovery is independent of window and zoom");
    Check(world.SightBlocked({150, 220}, {300, 220}, save),
          "Buildings block sight");
    auto events = save.history.size();
    game.Explore(save, 900, 650, 1, &world);
    Check(save.history.size() == events,
          "Repeated discovery does not flood history");
    int note = frontier::StoreMapNote(save, -1, save.player, false, "Home",
                                      "Remember the forge.");
    Check(note == 0 && save.map_notes.size() == 1,
          "Explored locations accept notes");
    bool rejected = false;
    try {
      frontier::StoreMapNote(save, -1, {100000, 100000}, false, "Hidden", "");
    } catch (const std::exception&) {
      rejected = true;
    }
    Check(rejected, "Unknown locations reject notes");
    save.skills.insert("crafting");
    save.inventory.wood = 1000;
    save.inventory.reeds = 1000;
    save.inventory.leather = 1000;
    save.inventory.rare.fill(1000);
    save.coins = 1000;
    auto recipe = [](const std::string& id) {
      for (size_t i = 0; i < frontier::Recipes().size(); i++) {
        if (frontier::Recipes()[i].id == id) {
          return static_cast<int>(i);
        }
      }
      throw std::runtime_error("Missing vessel recipe");
    };
    Check(frontier::Craft(save, recipe("vessel_raft")).ok &&
              frontier::ActiveVehicle(save) == 5,
          "Raft is accessible through basic crafting");
    Check(!frontier::Craft(save, recipe("vessel_flagship")).ok,
          "Prestige ship requires knowledge");
    save.skills.insert("sailing");
    save.technologies.insert("guild_charter");
    save.technologies.insert("shipbuilding");
    save.technologies.insert("navigation_school");
    Check(frontier::Craft(save, recipe("vessel_flagship")).ok &&
              save.coins == 700 && frontier::ActiveVehicle(save) == 9,
          "Flagship checks knowledge, materials and coins");
    auto coins = save.coins;
    Check(!frontier::Craft(save, recipe("vessel_flagship")).ok &&
              save.coins == coins,
          "Duplicate vessel does not consume resources");
    Check(frontier::Vessel(9).speed > frontier::Vessel(2).speed &&
              frontier::Vessel(2).speed > frontier::Vessel(5).speed,
          "Ship progression improves travel");
    auto path = dir / "world.json";
    frontier::WriteSave(path, save);
    auto loaded = frontier::LoadSave(path);
    Check(loaded.known_deposits.contains(101),
          "Deposits survive reload independent of loaded chunks");
    Check(loaded.vessels == save.vessels &&
              loaded.map_notes[0].text == save.map_notes[0].text,
          "v7 keeps ships and notes");
    nlohmann::json old;
    {
      std::ifstream in(path);
      in >> old;
    }
    old["version"] = 6;
    old.erase("vessels");
    old.erase("map_notes");
    old["vehicle"] = 0;
    {
      std::ofstream out(path);
      out << old;
    }
    auto migrated = frontier::LoadSave(path);
    frontier::WriteSave(path, migrated);
    Check(std::filesystem::exists(path.string() + ".v6.bak") &&
              migrated.skills == save.skills,
          "v6 migration preserves progress and original");
    frontier::Settings settings;
    settings.music_mode = 4;
    settings.music_volume = .2f;
    settings.effects_volume = 0;
    frontier::WriteSettings(dir / "settings.json", settings);
    auto read = frontier::LoadSettings(dir / "settings.json");
    Check(read.music_mode == 4 && read.effects_volume == 0 &&
              read.music_volume == .2f,
          "Audio settings persist");
    Check(frontier::DeleteMapNote(save, 0) && save.map_notes.empty(),
          "Journal notes can be removed");
    std::filesystem::remove_all(dir);
    std::cout << "Geometry, sight, notes, vessels and v7 migration passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::filesystem::remove_all(dir);
    return 1;
  }
}
