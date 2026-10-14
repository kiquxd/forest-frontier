#include <chrono>
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
  for (int y = -2; y <= 3; y++) {
    for (int x = -2; x <= 3; x++) {
      frontier::Chunk chunk;
      chunk.coord = {x, y};
      chunk.tiles.resize(256,
                         frontier::Tile{0, false, frontier::Terrain::kForest});
      world.chunks.emplace(chunk.coord, std::move(chunk));
    }
  }
  world.buildings.clear();
}

}  // namespace

int main() {
  try {
    frontier::Save save;
    auto inventory = save.inventory;
    Check(!frontier::Craft(save, 2).ok && save.inventory.wood == inventory.wood,
          "Failed crafting cannot consume resources");
    save.inventory.wood = 60;
    save.inventory.stone = 30;
    save.inventory.reeds = 20;
    save.inventory.berries = 12;
    save.inventory.leather = 8;
    for (int i = 0; i < 8; i++) {
      Check(frontier::Craft(save, i).ok, "All recipes craft");
    }
    int wood = save.inventory.wood;
    Check(frontier::Craft(save, 2).ok && save.inventory.wood == wood - 6 &&
              save.weapon_copies["wood_sword"] == 2,
          "Spare weapons consume materials and create separate copies");
    Check(save.inventory.sword && save.inventory.boat && save.inventory.armor &&
              save.inventory.potions == 1 && save.inventory.campfires == 1,
          "Equipment and consumables");
    int total_coins = save.coins + save.treasury[0],
        total_wood = save.inventory.wood + save.stocks[0][0];
    Check(frontier::Trade(save, 0, 0, true).ok &&
              frontier::Trade(save, 0, 0, false).ok,
          "Buy and sell");
    Check(total_coins == save.coins + save.treasury[0] &&
              total_wood == save.inventory.wood + save.stocks[0][0],
          "Trade conserves money and goods");
    save.coins = 0;
    int stock = save.stocks[0][1];
    Check(!frontier::Trade(save, 0, 1, true).ok && stock == save.stocks[0][1],
          "Insufficient funds is atomic");
    save.treasury[0] = 0;
    Check(!frontier::Trade(save, 0, 0, false).ok, "Treasury limits selling");
    save.coins = 20;
    save.stocks[0][1] = 0;
    Check(!frontier::Trade(save, 0, 1, true).ok, "Finite merchant stock");
    save.health = 100;
    Check(!frontier::Consume(save, true).ok && save.inventory.potions == 1,
          "Full health cannot waste potion");
    save.health = 40;
    Check(frontier::Consume(save, true).ok && save.health == 85 &&
              save.inventory.potions == 0,
          "Potion healing");
    save.inventory.berries = 3;
    Check(frontier::Consume(save, false).ok && save.health == 90 &&
              save.inventory.berries == 2,
          "Eating");
    frontier::Resource fish{12345, {0, 0}, frontier::ResourceKind::kFish, 0};
    save.inventory.rod = false;
    Check(
        !frontier::Harvest(save, fish).ok && !save.harvested.contains(fish.id),
        "Fishing requires rod");
    save.inventory.rod = true;
    Check(frontier::Harvest(save, fish).ok && save.inventory.fish == 1,
          "Fishing adds fish");
    Check(!frontier::Harvest(save, fish).ok && save.inventory.fish == 1,
          "No repeated harvest");
    Check(frontier::TerrainAt({1100, -1100}, save.seed) ==
              frontier::Terrain::kDeep,
          "Lake center");
    Check(frontier::TerrainAt({0, 0}, save.seed) == frontier::Terrain::kForest,
          "Village remains dry");
    bool sand = false, shallows = false;
    for (int y = -2000; y < 0; y += 24) {
      auto t = frontier::TerrainAt({1100, static_cast<float>(y)}, save.seed);
      sand |= t == frontier::Terrain::kSand;
      shallows |= t == frontier::Terrain::kShallow;
    }
    Check(sand && shallows, "Lake has sand and shallows");
    frontier::World world(save.seed);
    MakeFlatWorld(world);
    world.chunks.at({0, 0}).resources.push_back(
        {777, {500, 400}, frontier::ResourceKind::kTree, 0});
    frontier::Vec position = world.Move({450, 400}, {200, 0}, save);
    Check(position.x < 480, "Substeps prevent tunneling through tree");
    auto path = world.Path({450, 400}, {650, 400}, save);
    Check(!path.empty(), "Navigation finds path around tree");
    position = {450, 400};
    for (auto waypoint : path) {
      position = world.Move(position, waypoint - position, save);
    }
    Check(frontier::Length(position - frontier::Vec{650, 400}) < 25,
          "Navigation path is physically traversable");
    save.harvested.insert(777);
    position = world.Move({450, 400}, {200, 0}, save);
    Check(position.x > 649, "Harvest removes collision");
    world.buildings.push_back({{200, 200}, 0});
    Check(!world.Clear({200, 200}, save), "Building collision");
    world.buildings.clear();
    world.chunks.at({0, 0}).tiles[8 * 16 + 8].terrain =
        frontier::Terrain::kDeep;
    Check(!world.Clear({408, 408}, save, 12, false) &&
              world.Clear({408, 408}, save, 12, true),
          "Deep water requires boat");
    position = world.Move({408, 340}, {0, 140}, save);
    Check(position.y <= 373, "Cannot tunnel into deep water");
    position = world.Move({408, 340}, {0, 140}, save, 12, true);
    Check(position.y > 479, "Boat crosses deep water");
    frontier::Gameplay game;
    save.player = {560, 400};
    save.defeated.clear();
    save.inventory.leather = 0;
    save.coins = 20;
    world.chunks.at({0, 0}).enemies.push_back(
        {999, {620, 400}, {620, 400}, {620, 400}});
    for (int i = 0; i < 3; i++) {
      game.Advance(1);
      Check(game.Attack(world, save).ok, "Sword attack");
      game.Advance(.32f);
      game.Combat(world, save, .32f);
    }
    Check(save.defeated.contains(999) && save.inventory.leather == 2 &&
              save.coins == 23,
          "Three sword hits defeat wolf with one loot drop");
    game.Advance(1);
    game.Attack(world, save);
    game.Advance(.32f);
    game.Combat(world, save, .32f);
    Check(save.inventory.leather == 2, "Defeated wolf cannot drop twice");
    auto& enemy = world.chunks.at({0, 0}).enemies[0];
    save.defeated.clear();
    save.player = {1100, 400};
    enemy = {999, {1124, 400}, {1124, 400}, {1124, 400}};
    save.health = 100;
    save.inventory.armor = false;
    game.Advance(2);
    game.Tick(world, save, .1f, 0);
    Check(save.health == 100 && enemy.windup > 0,
          "Wolf telegraphs before biting");
    for (int i = 0; i < 5; i++) {
      game.Tick(world, save, .1f, .1);
    }
    Check(save.health == 88, "Wolf bite");
    game.Tick(world, save, .1f, .1);
    Check(save.health == 88, "Bite cooldown / invulnerability");
    enemy.cooldown = 0;
    enemy.windup = 0;
    save.health = 1;
    save.coins = 100;
    game.Advance(2);
    for (int i = 0; i < 6; i++) {
      game.Tick(world, save, .1f, .2);
    }
    Check(save.health == 100 && save.coins == 80 && game.respawns == 1,
          "Death respawns and preserves inventory");
    save.player = {1100, 400};
    enemy = {999, {1124, 400}, {1124, 400}, {1124, 400}};
    save.health = 100;
    game.Advance(4);
    save.inventory.armor = true;
    for (int i = 0; i < 6; i++) {
      game.Tick(world, save, .1f, .3);
    }
    Check(save.health == 93, "Armor damage reduction");
    save.campfires.push_back(save.player);
    save.health = 50;
    enemy.cooldown = 0;
    game.Advance(4);
    game.Tick(world, save, 1, .4);
    Check(save.health == 58, "Campfire heals and prevents attacks");
    auto temp =
        std::filesystem::temp_directory_path() /
        ("frontier-gameplay-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(temp);
    auto save_path = temp / "world.json";
    save.defeated.insert(0xffffffffffffffffULL);
    frontier::WriteSave(save_path, save);
    auto loaded = frontier::LoadSave(save_path);
    Check(loaded.defeated == save.defeated && loaded.inventory.boat &&
              loaded.health == save.health && loaded.stocks == save.stocks &&
              loaded.campfires.size() == 1,
          "Version 2 save round trip");
    {
      std::ofstream out(save_path);
      out << R"({"version":1,"seed":742019,"player":[120,140],"wood":9,"stone":5,"berries":2,"harvested":[18446744073709551615]})";
    }
    loaded = frontier::LoadSave(save_path);
    Check(loaded.inventory.wood == 9 &&
              loaded.harvested.contains(0xffffffffffffffffULL) &&
              loaded.health == 100,
          "Old save migration");
    frontier::WriteSave(save_path, loaded);
    Check(std::filesystem::exists(save_path.string() + ".v1.bak"),
          "Migration keeps original backup");
    std::filesystem::remove_all(temp);
    std::cout << "Craft, trade, harvest, terrain, collision, navigation, "
                 "combat, healing and migration checks passed\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
