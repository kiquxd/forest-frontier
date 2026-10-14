#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "gameplay.hpp"
#include "json.hpp"

namespace {
void Check(bool condition, const char* reason) {
  if (!condition) {
    throw std::runtime_error(reason);
  }
}
int FindRecipeIndex(const std::string& id) {
  for (int i = 0; i < static_cast<int>(frontier::Recipes().size()); i++) {
    if (frontier::Recipes()[i].id == id) {
      return i;
    }
  }
  throw std::runtime_error("Missing recipe");
}

}  // namespace

int main() {
  try {
    frontier::ConfigureWorld(742019);
    frontier::Save save;
    frontier::InitializeCampaign(save);
    Check(std::count(save.owners.begin(), save.owners.end(), 0) == 1 &&
              frontier::OwnsCity(save, save.home_city),
          "Exactly one initial player city");
    Check(frontier::Length(save.player -
                           frontier::Settlements()[save.home_city].pos) < 180 &&
              save.coins == 0 && save.skills.empty(),
          "Start near own hall without skills or money");
    auto halls = frontier::MakeBuildings();
    Check(std::count_if(halls.begin(), halls.end(),
                        [](auto& b) { return b.hall; }) ==
              static_cast<int>(frontier::Settlements().size()),
          "Every city has a hall");
    auto people = frontier::MakeNpcs();
    Check(std::count_if(people.begin(), people.end(),
                        [](auto& n) { return n.ruler; }) ==
              static_cast<int>(frontier::Settlements().size()),
          "Every foreign city can supply a resident ruler");
    for (int i = 0; i < static_cast<int>(save.owners.size()); i++) {
      for (int j = 0; j < static_cast<int>(save.owners.size()); j++) {
        Check(save.city_opinions[i][j] == save.city_opinions[j][i],
              "Reciprocal city relationships");
      }
    }
    frontier::Resource tree{1, {}, frontier::ResourceKind::kTree, 0};
    Check(!frontier::Harvest(save, tree).ok &&
              !frontier::Craft(save, FindRecipeIndex("campfire")).ok &&
              !frontier::Trade(save, save.home_city, 0, false).ok,
          "Starting actions locked");
    Check(
        !frontier::TrainSkill(save, 6).ok && !frontier::TrainSkill(save, 10).ok,
        "Advanced lessons need practice");
    Check(frontier::GetLessonState(save, 0).ready &&
              !frontier::GetLessonState(save, 0).learned,
          "Introductory lesson is available in a fresh campaign");
    Check(frontier::Diplomacy(save, save.home_city, 10).ok,
          "Hall teaches gathering");
    Check(frontier::GetLessonState(save, 0).learned &&
              frontier::GetLessonState(save, 0).progress ==
                  frontier::GetLessonState(save, 0).target &&
              frontier::GetLessonState(save, 1).ready,
          "Hall lesson immediately updates learned state and dependent lesson");
    Check(!frontier::Diplomacy(save, save.home_city, 10).ok &&
              save.skills.size() == 1,
          "Repeated hall lesson cannot change progression");
    save.gathered = 4;
    Check(!frontier::GetLessonState(save, 2).ready &&
              frontier::GetLessonState(save, 2).progress == 4 &&
              frontier::GetLessonState(save, 2).target == 5,
          "Craftsmanship shows its exact practice progress");
    save.gathered = 0;
    Check(frontier::TrainSkill(save, 1).ok,
          "Free hall lessons bootstrap trade");
    for (int i = 0; i < 60; i++) {
      frontier::Resource resource{
          static_cast<uint64_t>(100 + i), {}, frontier::ResourceKind(i % 4), 0};
      Check(frontier::Harvest(save, resource).ok, "Gather without tools");
    }
    Check(frontier::TrainSkill(save, 2).ok, "Gathering unlocks craft lesson");
    Check(frontier::Craft(save, FindRecipeIndex("campfire")).ok &&
              frontier::Craft(save, FindRecipeIndex("campfire")).ok,
          "Two basic crafts without tool prerequisite");
    Check(frontier::TrainSkill(save, 3).ok &&
              frontier::TrainSkill(save, 4).ok &&
              frontier::TrainSkill(save, 5).ok,
          "Practice unlocks tools, combat, fishing");
    Check(frontier::Craft(save, FindRecipeIndex("rod")).ok,
          "Craft fishing rod");
    for (int i = 0; i < 3; i++) {
      Check(frontier::Harvest(save, {static_cast<uint64_t>(1000 + i),
                                     {},
                                     frontier::ResourceKind::kFish,
                                     0})
                .ok,
            "Catch fish with rod");
    }
    Check(frontier::TrainSkill(save, 6).ok &&
              frontier::Craft(save, FindRecipeIndex("boat")).ok,
          "Fishing unlocks first watercraft without coins or advanced "
          "technologies");
    Check(frontier::TrainSkill(save, 8).ok && frontier::TrainSkill(save, 9).ok,
          "Construction and diplomacy lessons");
    Check(!frontier::Diplomacy(save, save.home_city, 2).ok,
          "Cannot declare war on own city");
    Check(frontier::Diplomacy(save, save.home_city, 8).ok &&
              frontier::TrainSkill(save, 10).ok,
          "City activity unlocks engineering");
    int foreign = (save.home_city + 1) % static_cast<int>(save.owners.size());
    save.coins = 500;
    save.relations[foreign] = {40, frontier::Treaty::kPeace, true};
    Check(frontier::Diplomacy(save, foreign, 2).ok &&
              save.city_opinions[save.home_city][foreign] == -80,
          "War changes intercity relationship");
    Check(frontier::Diplomacy(save, foreign, 1).ok &&
              save.city_opinions[foreign][save.home_city] == 0,
          "Truce changes mutual relationship");
    save.relations[foreign].opinion = 75;
    Check(frontier::Diplomacy(save, foreign, 3).ok &&
              frontier::Diplomacy(save, foreign, 4).ok &&
              frontier::OwnsCity(save, foreign),
          "Union transfers ownership");
    auto path =
        std::filesystem::temp_directory_path() /
        ("campaign-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()) +
         ".json");
    frontier::WriteSave(path, save);
    auto loaded = frontier::LoadSave(path);
    std::filesystem::remove(path);
    Check(loaded.owners == save.owners && loaded.skills == save.skills &&
              loaded.city_opinions == save.city_opinions && loaded.caught == 3,
          "Campaign and activities survive save roundtrip");
    frontier::Save legacy;
    legacy.inventory.ship = true;
    legacy.technologies.insert("submarine");
    auto old_position = legacy.player;
    frontier::InitializeCampaign(legacy, true);
    Check(legacy.skills.size() == frontier::Skills().size() &&
              legacy.inventory.ship &&
              legacy.technologies.contains("submarine") &&
              frontier::Length(legacy.player - old_position) == 0,
          "Legacy migration preserves progress and position");
    // A migrated v3 slot keeps capabilities and gets an untouched backup.
    auto legacy_path =
        std::filesystem::temp_directory_path() /
        ("campaign-v3-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()) +
         ".json");
    frontier::WriteSave(legacy_path, save);
    nlohmann::json json;
    {
      std::ifstream in(legacy_path);
      in >> json;
    }
    json["version"] = 3;
    {
      std::ofstream out(legacy_path);
      out << json;
    }
    auto migrated = frontier::LoadSave(legacy_path);
    frontier::InitializeCampaign(migrated, true);
    frontier::WriteSave(legacy_path, migrated);
    Check(std::filesystem::exists(legacy_path.string() + ".v3.bak") &&
              migrated.inventory.boat &&
              migrated.skills.size() == frontier::Skills().size(),
          "Old slot backed up without relocking equipment");
    std::filesystem::remove(legacy_path);
    std::filesystem::remove(legacy_path.string() + ".v3.bak");
    // Reception policy changes an actual visitor's position, rather than only
    // text.
    frontier::World world(save.seed);
    world.buildings.clear();
    int host = 0;
    auto center = frontier::ChunkAt(frontier::Settlements()[host].pos);
    for (int y = -2; y <= 2; y++) {
      for (int x = -2; x <= 2; x++) {
        frontier::Chunk chunk;
        chunk.coord = {center.x + x, center.y + y};
        chunk.tiles.resize(
            256, frontier::Tile{0, false, frontier::Terrain::kForest});
        world.chunks.emplace(chunk.coord, std::move(chunk));
      }
    }
    frontier::Save reception = save;
    reception.player =
        frontier::Settlements()[host].pos + frontier::Vec{100, 100};
    int visitor = -1, ruler = -1;
    for (auto& npc : world.npcs) {
      if (npc.settlement == host) {
        if (npc.origin_city >= 0) {
          visitor = npc.id;
        }
        if (npc.ruler) {
          ruler = npc.id;
        }
      }
    }
    Check(visitor >= 0 && ruler >= 0,
          "Resident ruler and foreign visitor exist");
    auto ruler_position = world.npcs[ruler].pos;
    reception.city_opinions[host][world.npcs[visitor].origin_city] = -80;
    world.Tick(20, reception, 0);
    Check(frontier::Length(world.npcs[visitor].pos -
                           frontier::Settlements()[host].pos) > 200,
          "Hostile foreign visitor stays outside central square");
    world.BeginConversation(ruler, reception);
    world.Tick(1, reception, 1);
    Check(frontier::Length(world.npcs[ruler].pos - ruler_position) == 0,
          "Ruler stays inside the hall during conversation");
    std::cout << "campaign ownership, halls, relations, unlock path and "
                 "persistence passed\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
