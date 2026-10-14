#include <array>
#include <chrono>
#include <cstddef>
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
void MakeFlatWorld(frontier::World& world) {
  world.chunks.clear();
  world.buildings.clear();
  for (int y = -2; y <= 3; y++) {
    for (int x = -2; x <= 3; x++) {
      frontier::Chunk chunk;
      chunk.coord = {x, y};
      chunk.tiles.resize(256,
                         frontier::Tile{0, false, frontier::Terrain::kForest});
      world.chunks.emplace(chunk.coord, std::move(chunk));
    }
  }
}

}  // namespace

int main() {
  try {
    Check(frontier::GetContent().npcs.size() >= 20 &&
              frontier::GetContent().weapons.size() >= 14 &&
              frontier::Recipes().size() >= 28,
          "Content catalogs loaded");
    std::array<int, 4> counts{};
    for (int i = 0; i < 20000; i++) {
      counts[frontier::GetContent()
                 .npcs[frontier::WeightedNpc(frontier::Hash(i))]
                 .rarity]++;
    }
    Check(counts[0] > counts[1] && counts[1] > counts[2] &&
              counts[2] > counts[3] && counts[3] > 0,
          "Rarity weights produce rarer NPCs less frequently");
    frontier::ConfigureWorld(742019);
    auto cities = frontier::Settlements();
    frontier::ConfigureWorld(10);
    auto other = frontier::Settlements();
    frontier::ConfigureWorld(742019);
    Check(cities.size() == frontier::Settlements().size() && cities.size() > 12,
          "Seeded towns generated");
    Check(other.back().pos.x != cities.back().pos.x,
          "Seed changes distant settlements");
    for (size_t i = 0; i < cities.size(); i++) {
      Check(cities[i].pos.x == frontier::Settlements()[i].pos.x &&
                cities[i].type == frontier::Settlements()[i].type,
            "Deterministic town generation");
    }
    frontier::Save save;
    frontier::EnsureCityState(save);
    save.coins = 2000;
    save.inventory.wood = 200;
    save.inventory.stone = 200;
    save.inventory.fish = 100;
    int gold = save.coins + save.treasury[0];
    Check(frontier::LearnTechnology(save, 0).ok &&
              save.technologies.contains("ironworking"),
          "Learn from friendly city's master");
    Check(gold == save.coins + save.treasury[0], "Research conserves money");
    Check(!frontier::Trade(save, 2, 7, true).ok,
          "Rare material locked by technology and reputation");
    Check(frontier::Diplomacy(save, 2, 0).ok &&
              frontier::LearnTechnology(save, 2).ok &&
              frontier::Trade(save, 2, 7, true).ok,
          "Trust unlocks ship technology and copper");
    Check(!frontier::Trade(save, 0, 7, true).ok,
          "Other towns cannot supply exclusive material");
    Check(frontier::Diplomacy(save, 0, 2).ok &&
              save.relations[0].treaty == frontier::Treaty::kWar &&
              !frontier::Trade(save, 0, 0, true).ok,
          "War closes market");
    Check(!frontier::LearnTechnology(save, 0).ok,
          "War blocks technology sharing");
    Check(frontier::Diplomacy(save, 0, 1).ok &&
              save.relations[0].treaty == frontier::Treaty::kTruce &&
              frontier::Trade(save, 0, 0, true).ok,
          "Truce reopens market");
    for (int i = 0; i < 2; i++) {
      Check(frontier::Diplomacy(save, 0, 0).ok, "Gift builds trust");
    }
    Check(frontier::Diplomacy(save, 0, 3).ok, "Alliance requires trust");
    for (int i = 0; i < 3; i++) {
      Check(frontier::Diplomacy(save, 0, 8).ok,
            "City order improves relations");
    }
    Check(frontier::Diplomacy(save, 0, 4).ok &&
              save.relations[0].treaty == frontier::Treaty::kUnion,
          "Alliance can become union");
    int wood = save.inventory.wood, stone = save.inventory.stone;
    Check(frontier::Diplomacy(save, 0, 6).ok && save.walls.size() == 1 &&
              save.inventory.wood == wood - 8 &&
              save.inventory.stone == stone - 3,
          "Walls consume resources");
    for (int i = 0; i < 10; i++) {
      Check(frontier::Diplomacy(save, 0, 6).ok, "Build connected ring");
    }
    Check(!frontier::Diplomacy(save, 0, 6).ok && save.walls.size() == 11,
          "Wall ring leaves a gate");
    Check(frontier::CityWeapon(0, save.seed).rarity >= 1 &&
              frontier::CityWeapon(8, save.seed).rarity == 3,
          "City offers respect rarity");
    for (auto& type : frontier::GetContent().towns) {
      save.technologies.insert(type.technology);
    }
    save.inventory.rare.fill(100);
    save.inventory.reeds = 100;
    for (auto id : {"ship", "submarine", "sonar", "oxygen"}) {
      int recipe = -1;
      for (size_t i = 0; i < frontier::Recipes().size(); i++) {
        if (frontier::Recipes()[i].id == id) {
          recipe = static_cast<int>(i);
        }
      }
      Check(frontier::Craft(save, recipe).ok, "Advanced craft chain");
    }
    save.player = {1100, -1100};
    Check(frontier::Dive(save).ok && save.submerged &&
              frontier::ActiveVehicle(save) == 3,
          "Crafted submarine enables diving");
    Check(frontier::Sonar(save).ok && !save.explored_underwater.empty() &&
              save.explored.empty(),
          "Sonar reveals underwater only");
    Check(frontier::Dive(save).ok && !save.submerged, "Surface independently");
    frontier::World world(save.seed);
    MakeFlatWorld(world);
    frontier::Gameplay game;
    save.submerged = false;
    save.vehicle = 1;
    save.player = {1100, -1100};
    game.walk = 2;
    game.MovePlayer(world, save, {8, 0});
    Check(game.walk == 2, "Water vehicle movement cannot animate walking legs");
    Check(!game.Attack(world, save).ok, "Cannot swing a sword inside a vessel");
    save.player = {120, 140};
    save.walls.clear();
    save.explored.clear();
    save.explored_underwater.clear();
    game.Explore(save, 1000, 600, 1);
    Check(frontier::ExploredAt(save.explored, save.player) &&
              !frontier::ExploredAt(save.explored, {1100, -1100}),
          "Map cannot reveal unseen lake");
    game.Explore(save, 1000, 600, 1);
    auto known = save.explored.size();
    save.player = {1100, -450};
    game.Explore(save, 1000, 600, 1);
    Check(save.explored.size() > known &&
              !frontier::ExploredAt(save.explored, {1100, -1800}),
          "Exploration reveals only part of lake");
    save.player = {120, 140};
    world.BeginConversation(0, save);
    frontier::Vec speaker = world.npcs[0].pos;
    Check(frontier::Length(speaker - save.player) <= 85,
          "Speaker stands beside player");
    for (int i = 0; i < 30; i++) {
      world.Tick(.1f, save, i * .1);
    }
    Check(frontier::Length(speaker - world.npcs[0].pos) < .001,
          "Speaker does not wander while speaking");
    world.talking_npc = -1;
    world.Tick(.1f, save, 4);
    Check(frontier::Length(world.npcs[0].pos - speaker) > 0,
          "Speaker resumes normal behavior");
    save.player = {560, 400};
    save.inventory.weapon_id = "wood_sword";
    save.inventory.weapons.insert("wood_sword");
    auto& enemies = world.chunks.at({0, 0}).enemies;
    enemies.push_back({999, {620, 400}, {620, 400}, {620, 400}});
    game.Attack(world, save);
    Check(enemies[0].hp == 60, "No instant melee damage at windup");
    game.Advance(.1f);
    game.Combat(world, save, .1f);
    Check(enemies[0].hp == 60, "Windup is harmless");
    game.Advance(.2f);
    game.Combat(world, save, .2f);
    Check(enemies[0].hp == 35, "Blade deals damage in active phase");
    game.Combat(world, save, .1f);
    Check(enemies[0].hp == 35, "One hit per swing");
    game.Advance(1);
    save.inventory.weapons.insert("bow");
    save.inventory.weapon_id = "bow";
    save.inventory.ammo = 1;
    game.Attack(world, save);
    game.Advance(.25f);
    game.Combat(world, save, .01f);
    Check(save.inventory.ammo == 0 && !game.projectiles.empty(),
          "Bow fires projectile and consumes crafted ammo");
    for (int i = 0; i < 30; i++) {
      game.Combat(world, save, .02f);
    }
    Check(enemies[0].hp < 35, "Arrow collision deals damage");
    save.player = {1100, 400};
    save.health = 100;
    game.Advance(2);
    enemies[0] = {1000, {1124, 400}, {1124, 400}, {1124, 400}};
    enemies[0].kind = 1;
    enemies[0].city = 0;
    enemies[0].hp = 100;
    for (int i = 0; i < 8; i++) {
      game.Tick(world, save, .1f, i * .1);
    }
    Check(save.health == 100, "Allied guards cannot attack player");
    frontier::Diplomacy(save, 0, 2);
    for (int i = 0; i < 8; i++) {
      game.Tick(world, save, .1f, 1 + i * .1);
    }
    Check(save.health == 82,
          "War makes guards hostile with telegraphed attacks");
    frontier::Diplomacy(save, 0, 1);
    game.Advance(2);
    int health = save.health;
    for (int i = 0; i < 12; i++) {
      game.Tick(world, save, .1f, 2 + i * .1);
    }
    Check(save.health == health, "Truce stops guard combat");
    save.relations[0].treaty = frontier::Treaty::kUnion;
    save.submerged = true;
    save.player = {1100, -1100};
    save.oxygen = 200;
    int coins = save.coins, treasury = save.treasury[0];
    for (int i = 0; i < 200; i++) {
      game.Tick(world, save, .1f, 4 + i * .1);
    }
    Check(save.coins == coins + 1 && save.treasury[0] == treasury - 1,
          "Union income continues underwater and conserves money");
    auto path =
        std::filesystem::temp_directory_path() /
        ("frontier-adventure-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()) +
         ".json");
    save.submerged = false;
    frontier::WriteSave(path, save);
    auto restored = frontier::LoadSave(path);
    Check(restored.technologies == save.technologies &&
              restored.relations[0].treaty == frontier::Treaty::kUnion &&
              restored.explored == save.explored &&
              restored.inventory.submarine &&
              restored.inventory.weapons == save.inventory.weapons,
          "Advanced state persists");
    std::filesystem::remove(path);
    std::cout << "Content rarity, diplomacy, research, walls, vessels, fog, "
                 "conversation, timed combat and persistence passed\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
