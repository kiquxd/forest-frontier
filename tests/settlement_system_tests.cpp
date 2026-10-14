#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "adventure.hpp"
#include "json.hpp"
#include "settlement_system.hpp"

namespace {
void Check(bool ok, const char* reason) {
  if (!ok) {
    throw std::runtime_error(reason);
  }
}
int RecipeIndex(const std::string& id) {
  for (size_t i = 0; i < frontier::Recipes().size(); ++i) {
    if (frontier::Recipes()[i].id == id) {
      return static_cast<int>(i);
    }
  }
  throw std::runtime_error("Missing recipe");
}
void ExploreRoute(frontier::Save& save,
                  const std::vector<frontier::Vec>& points) {
  for (size_t i = 1; i < points.size(); ++i) {
    int steps = static_cast<int>(
        std::ceil(frontier::Length(points[i] - points[i - 1]) / 12));
    for (int j = 0; j <= steps; ++j) {
      auto cell = frontier::MapCell(points[i - 1] +
                                    (points[i] - points[i - 1]) *
                                        (static_cast<float>(j) / steps));
      for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
          save.explored.insert({cell.x + x, cell.y + y});
        }
      }
    }
  }
}
}  // namespace
int main() {
  auto folder =
      std::filesystem::temp_directory_path() /
      ("frontier-settlement-tests-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    frontier::Save save;
    frontier::ConfigureWorld(save.seed);
    frontier::InitializeCampaign(save);
    frontier::InitializeAdventure(save);
    int home = save.home_city;
    {
      frontier::World world(save.seed);
      frontier::Gameplay game;
      game.Explore(save, 800, 600, 1, &world);
      Check(frontier::ExploredAt(save.explored,
                                 frontier::Settlements()[home].pos),
            "Approaching the hall reveals the road endpoint despite foundation "
            "occlusion");
    }
    Check(frontier::FacilityAccess(save, home, "workbench") &&
              !frontier::FacilityAccess(save, home, "school"),
          "Start has a workbench without a school");
    save.skills.insert("crafting");
    save.skills.insert("construction");
    save.inventory.wood = save.inventory.stone = 100000;
    save.coins = 10000;
    int fire = RecipeIndex("campfire");
    Check(frontier::Craft(save, fire).ok, "Craft at a local workbench");
    auto wood = save.inventory.wood;
    auto ready = save.craft_ready_at;
    Check(!frontier::Craft(save, fire).ok && save.inventory.wood == wood &&
              save.craft_ready_at == ready,
          "Cooldown blocks spam without resource loss or extending the timer");
    frontier::TickTowns(save, 59);
    Check(!frontier::Craft(save, fire).ok,
          "Common crafting remains blocked until one full game minute");
    frontier::TickTowns(save, 1);
    Check(frontier::Craft(save, fire).ok,
          "Game time releases cooldown without sleeping");
    Check(frontier::CraftDelay(
              frontier::Recipes()[RecipeIndex("vessel_flagship")]) >
              frontier::CraftDelay(frontier::Recipes()[fire]),
          "Rare crafts take longer");
    Check(frontier::CraftDelay(frontier::Recipes()[fire]) == 60 &&
              frontier::CraftDelay(
                  frontier::Recipes()[RecipeIndex("vessel_flagship")]) == 7200,
          "Craft tiers range from one minute to two hours");
    save.game_time = save.craft_ready_at;
    auto before = save.player;
    save.player = {100000, 100000};
    Check(!frontier::Craft(save, fire).ok,
          "Craft outside town territory is blocked");
    save.player = before;
    Check(frontier::ExpandCity(save, home).ok &&
              frontier::CityRadius(save, home) == 500,
          "Expansion increases territory");
    Check(frontier::BuildFacility(save, home, "school").ok &&
              !frontier::BuildFacility(save, home, "school").ok,
          "Build school once");
    int partner = (home + 1) % static_cast<int>(save.owners.size());
    save.relations[partner] = {40, frontier::Treaty::kPeace, true};
    save.facilities.push_back(
        {partner, "alchemy", frontier::Settlements()[partner].pos});
    Check(!frontier::FacilityAccess(save, home, "alchemy"),
          "Unconnected foreign building is unavailable");
    std::vector<frontier::Vec> route{frontier::Settlements()[home].pos,
                                     {1100, -1100},
                                     frontier::Settlements()[partner].pos};
    Check(!frontier::BuildRoad(save, home, partner, route).ok &&
              save.roads.empty(),
          "Unknown road cannot be built partially");
    ExploreRoute(save, route);
    auto quote = frontier::QuoteRoad(save, home, partner, route);
    if (!quote.ok || quote.bridge_tiles == 0) {
      std::cerr << quote.message << " bridges " << quote.bridge_tiles << '\n';
    }
    Check(quote.ok && quote.bridge_tiles > 0 &&
              quote.wood == quote.land_tiles + quote.bridge_tiles * 4,
          "Explored route includes more expensive bridges");
    save.inventory.wood = 0;
    auto stone = save.inventory.stone;
    Check(!frontier::BuildRoad(save, home, partner, route).ok &&
              save.roads.empty() && save.inventory.stone == stone,
          "Unaffordable complete route is atomic");
    save.inventory.wood = 100000;
    Check(frontier::BuildRoad(save, home, partner, route).ok &&
              save.roads.size() == 1 &&
              frontier::FacilityAccess(save, home, "alchemy"),
          "Complete road shares facilities");
    {
      frontier::World world(save.seed);
      world.buildings.clear();
      auto water = route[1];
      frontier::Chunk chunk;
      chunk.coord = frontier::ChunkAt(water);
      chunk.tiles.resize(256, {0, false, frontier::Terrain::kDeep});
      world.chunks.emplace(chunk.coord, std::move(chunk));
      Check(world.Clear(water, save, 12, false),
            "Bridge permits walking over deep water");
      auto no_road = save;
      no_road.roads.clear();
      Check(!world.Clear(water, no_road, 12, false),
            "Deep water remains blocked without the bridge");
    }
    save.relations[partner].treaty = frontier::Treaty::kWar;
    Check(!frontier::FacilityAccess(save, home, "alchemy"),
          "War blocks remote facilities");
    Check(frontier::SurrenderTown(save, home, partner).ok &&
              frontier::OwnsCity(save, home),
          "Surrender retains local ownership");
    Check(save.city_opinions[home][partner] == save.relations[partner].opinion,
          "Surrender updates inter-town relations");
    auto coins = save.coins;
    frontier::TickTowns(save, 120);
    Check(save.coins == coins - 10 && save.tributes.size() == 1,
          "Tribute paid in game time");
    Check(frontier::BuyIndependence(save, home, false).ok &&
              save.tributes.empty(),
          "Buy independence");
    save.relations[partner].treaty = frontier::Treaty::kWar;
    Check(frontier::SurrenderTown(save, home, partner).ok,
          "Surrender again after new hostilities");
    save.coins = 0;
    frontier::TickTowns(save, 120);
    Check(save.tributes.empty() &&
              save.relations[partner].treaty == frontier::Treaty::kWar,
          "Missing tribute resumes hostilities once");
    int patron = (partner + 1) % static_cast<int>(save.owners.size());
    if (patron == home) {
      patron = (patron + 1) % static_cast<int>(save.owners.size());
    }
    save.coins = 10000;
    save.owners[partner] = 0;
    save.relations[patron].treaty = frontier::Treaty::kWar;
    Check(frontier::SurrenderTown(save, partner, patron).ok &&
              frontier::BuyIndependence(save, partner, true).ok &&
              save.owners[partner] == partner + 1,
          "A formerly joined town can return to its original ruler");
    save.coins = 10000;
    int npc = save.citizens.front().npc;
    auto* resident = frontier::GetCitizen(save, npc);
    save.inventory.weapons.insert("wood_sword");
    save.weapon_copies["wood_sword"] = 2;
    resident->weapon = "wood_sword";
    Check(frontier::LoseResident(save, npc, true, partner).ok &&
              frontier::AvailableWeapon(save, "wood_sword") == 1,
          "Captive gear remains allocated");
    Check(!frontier::AssignCitizen(save, npc, "idle", "", "").ok,
          "Captive cannot be reassigned");
    Check(frontier::RansomResident(save, npc).ok &&
              resident->weapon == "wood_sword" && resident->state == "active",
          "Ransom returns the same equipment");
    Check(!frontier::RansomResident(save, npc).ok, "Cannot pay ransom twice");
    Check(frontier::LoseResident(save, npc, false).ok &&
              save.weapon_copies["wood_sword"] == 1 && resident->weapon.empty(),
          "Death destroys assigned weapon only once");
    auto count = save.citizens.size();
    frontier::SyncCitizens(save);
    Check(save.citizens.size() == count && !frontier::ResidentActive(save, npc),
          "Dead resident does not respawn through synchronization");
    save.inventory.weapon_id = "wood_sword";
    frontier::DestroyPlayerWeapon(save);
    Check(save.weapon_copies["wood_sword"] == 0 &&
              save.inventory.weapon_id.empty() &&
              !save.inventory.weapons.contains("wood_sword"),
          "Player death removes equipped weapon without a drop");
    int captured_npc = save.citizens[1].npc;
    auto* defender = frontier::GetCitizen(save, captured_npc);
    defender->job = "guard";
    defender->trained = true;
    defender->health = 1;
    defender->weapon = "wood_sword";
    save.inventory.weapons.insert("wood_sword");
    save.weapon_copies["wood_sword"] = 1;
    {
      frontier::World world(save.seed);
      world.buildings.clear();
      world.npcs.clear();
      frontier::Npc n;
      n.id = captured_npc;
      n.pos = n.previous = {100, 100};
      world.npcs.push_back(n);
      frontier::Chunk chunk;
      chunk.coord = {0, 0};
      chunk.tiles.resize(256, {0, false, frontier::Terrain::kForest});
      frontier::Enemy guard;
      guard.id = 1;
      while (frontier::Hash(save.seed ^ guard.id ^
                            static_cast<uint64_t>(captured_npc)) %
                 100 >=
             40) {
        ++guard.id;
      }
      guard.kind = 1;
      guard.city = partner;
      guard.pos = guard.home = guard.previous = {101, 100};
      guard.attack_target = n.pos;
      guard.target_npc = captured_npc;
      guard.windup = .01f;
      chunk.enemies.push_back(guard);
      world.chunks.emplace(chunk.coord, std::move(chunk));
      save.player = {600, 600};
      frontier::Gameplay game;
      game.Tick(world, save, .1f, 0);
      Check(defender->state == "captive" && defender->weapon == "wood_sword",
            "Local hostile guard can capture a defeated defender");
    }
    save.player = frontier::Settlements()[home].pos;
    std::filesystem::create_directories(folder);
    auto path = folder / "world.json";
    save.craft_ready_at = save.game_time + frontier::kMaxCraftCooldown;
    frontier::WriteSave(path, save);
    auto loaded = frontier::LoadSave(path);
    Check(frontier::CraftWait(loaded) == frontier::kMaxCraftCooldown,
          "Cooldown cannot be bypassed by reload");
    auto previous_timer = save;
    previous_timer.craft_ready_at = previous_timer.game_time + 15;
    auto previous_path = folder / "short-timer.json";
    frontier::WriteSave(previous_path, previous_timer);
    Check(frontier::CraftWait(frontier::LoadSave(previous_path)) == 15,
          "Existing short cooldowns remain readable without resetting "
          "progression");
    Check(loaded.game_time == save.game_time && loaded.roads.size() == 1 &&
              loaded.facilities.size() == save.facilities.size() &&
              frontier::GetCitizen(loaded, npc)->state == "dead",
          "v8 persists time, infrastructure and losses");
    Check(frontier::GetCitizen(loaded, captured_npc)->state == "captive" &&
              frontier::AvailableWeapon(loaded, "wood_sword") == 0,
          "Captive equipment remains reserved after reload");
    nlohmann::json legacy;
    {
      std::ifstream in(path);
      in >> legacy;
    }
    legacy["version"] = 7;
    {
      std::ofstream out(path);
      out << legacy;
    }
    auto old = frontier::LoadSave(path);
    auto skills = old.skills;
    frontier::InitializeAdventure(old);
    frontier::WriteSave(path, old);
    Check(std::filesystem::exists(path.string() + ".v7.bak") &&
              old.skills == skills && old.urban,
          "v7 migration preserves progress and backup");
    std::filesystem::remove_all(folder);
    std::cout << "Town construction, roads, cooldowns, captivity, tribute and "
                 "v8 migration passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::filesystem::remove_all(folder);
    return 1;
  }
}
