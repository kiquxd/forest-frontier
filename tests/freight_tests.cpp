#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "adventure.hpp"
#include "freight.hpp"
#include "nation.hpp"
#include "settlement_system.hpp"

namespace {
void Check(bool ok, const char* message) {
  if (!ok) {
    throw std::runtime_error(message);
  }
}
frontier::Save NewState() {
  frontier::Save save;
  frontier::ConfigureWorld(save.seed);
  frontier::InitializeCampaign(save, true);
  frontier::InitializeAdventure(save);
  frontier::InitializeNation(save);
  save.owners[1] = 0;
  save.relations[0] = save.relations[1] = {50, frontier::Treaty::kPeace, true};
  for (const auto& skill : frontier::Skills()) {
    save.skills.insert(skill.id);
  }
  for (auto& stock : save.stocks) {
    stock.fill(100);
  }
  for (int y = -10; y <= 35; ++y) {
    for (int x = -10; x <= 55; ++x) {
      save.explored.insert({x, y});
    }
  }
  save.roads.push_back({0, 1, {{0, 0}, {1000, 350}, {2000, 700}}});
  return save;
}
void Terrain(frontier::World& world) {
  world.chunks.clear();
  world.buildings.clear();
  for (int y = -1; y <= 2; ++y) {
    for (int x = -1; x <= 4; ++x) {
      frontier::Chunk chunk;
      chunk.coord = {x, y};
      chunk.tiles.resize(frontier::kChunkTiles * frontier::kChunkTiles);
      world.chunks.emplace(chunk.coord, std::move(chunk));
    }
  }
}
void Run(frontier::World& world, frontier::Save& save, int ticks) {
  for (int i = 0; i < ticks; ++i) {
    frontier::TickFreight(world, save, .1f);
  }
}
void Benchmark(frontier::World& world, frontier::Save save) {
  for (auto& stock : save.stocks) {
    stock.fill(100000);
  }
  for (int i = 0; i < 64; ++i) {
    Check(frontier::BuildFreightCart(save, 0).ok,
          "Benchmark cart setup failed");
    Check(frontier::DispatchFreight(save, 0, 1, 0, 1).ok,
          "Benchmark dispatch failed");
  }
  using Clock = std::chrono::steady_clock;
  const auto deadline = Clock::now() + std::chrono::seconds(5);
  std::vector<double> times;
  double total = 0;
  while (Clock::now() < deadline) {
    for (const auto& cart : save.freight_carts) {
      if (cart.route.empty()) {
        Check(
            frontier::DispatchFreight(save, cart.city, 1 - cart.city, 0, 1).ok,
            "Benchmark return dispatch failed");
      }
    }
    auto begin = Clock::now();
    frontier::TickFreight(world, save, .1f);
    double elapsed =
        std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
    times.push_back(elapsed);
    total += elapsed;
  }
  std::sort(times.begin(), times.end());
  std::cout << "Synthetic CPU freight sample: 64 carts, 5 seconds, "
            << times.size() << " ticks; mean_ms=" << total / times.size()
            << ", p95_ms=" << times[times.size() * 95 / 100]
            << ", max_ms=" << times.back() << '\n';
}
}  // namespace
int main(int argc, char** argv) {
  const auto directory =
      std::filesystem::temp_directory_path() /
      ("frontier-freight-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    auto initial = NewState();
    frontier::World world(initial.seed);
    Terrain(world);
    if (argc == 2 && std::string(argv[1]) == "--benchmark") {
      Benchmark(world, initial);
      return 0;
    }
    auto save = initial;
    const auto money = frontier::MoneySupply(save);
    Check(!frontier::ShipGoods(save, 0, 1, 0, 5).ok && save.stocks[0][0] == 100,
          "No free or timer-based freight without a real cart");
    save.skills.erase("tools");
    Check(!frontier::BuildFreightCart(save, 0).ok,
          "Cart construction enforces the full ancestor branch");
    save.skills.insert("tools");
    Check(frontier::BuildFreightCart(save, 0).ok &&
              save.freight_carts.size() == 1 && save.stocks[0][0] == 80 &&
              save.stocks[0][1] == 95 && save.stocks[0][5] == 96 &&
              frontier::MoneySupply(save) == money,
          "Building one counted cart consumes materials without minting coins");
    auto ready = save;
    Check(!frontier::ShipGoods(save, 0, 1, 0, 41).ok && save.stocks[0][0] == 80,
          "Capacity rejection is atomic");
    Check(frontier::ShipGoods(save, 0, 1, 0, 10).ok &&
              save.stocks[0][0] == 70 && save.freight_carts[0].quantity == 10 &&
              save.stocks[1][0] == 100 && save.shipments.empty(),
          "Dispatch moves goods into one cart, never a timer shipment");
    Check(!frontier::ShipGoods(save, 0, 1, 0, 5).ok,
          "An allocated cart cannot carry a second delivery");
    auto& cart = save.freight_carts[0];
    auto start = cart.position;
    frontier::TickFreight(world, save, 0);
    Check(cart.position.x == start.x && cart.position.y == start.y,
          "Paused simulation does not advance freight");
    Run(world, save, 1);
    Check(frontier::Length(cart.position - start) <= 10 &&
              save.stocks[1][0] == 100,
          "A cart moves gradually and does not teleport its cargo");
    Run(world, save, 100);
    Check(cart.quantity == 10 && cart.waypoint > 1 &&
              frontier::RoadAt(save, cart.position),
          "Visible freight follows the built road");
    auto midway = save;
    const auto path = directory / "freight.json";
    frontier::WriteSave(path, save);
    auto loaded = frontier::LoadSave(path);
    Check(loaded.freight_carts[0].quantity == 10 &&
              loaded.freight_carts[0].position.x == cart.position.x &&
              loaded.freight_carts[0].waypoint == cart.waypoint &&
              loaded.freight_carts[0].local_route.empty(),
          "An in-transit cart roundtrips with exact cargo and route progress");
    Run(world, loaded, 500);
    Check(loaded.freight_carts[0].route.empty() &&
              loaded.freight_carts[0].city == 1 &&
              loaded.freight_carts[0].quantity == 0 &&
              loaded.stocks[1][0] == 110 &&
              frontier::MoneySupply(loaded) == money,
          "Only arrival unloads cargo and leaves the reusable cart at the "
          "destination");
    Run(world, loaded, 100);
    Check(loaded.stocks[1][0] == 110, "Delivery cannot execute twice");
    Check(frontier::ShipGoods(loaded, 1, 0, 1, 8).ok,
          "The same cart can carry a return load in reverse road direction");
    Run(world, loaded, 600);
    Check(loaded.stocks[0][1] == 103 && loaded.freight_carts[0].city == 0,
          "Reverse dispatch follows the same road and delivers once");

    save = ready;
    save.submerged = true;
    frontier::ShipGoods(save, 0, 1, 0, 10);
    Run(world, save, 600);
    Check(save.stocks[1][0] == 110,
          "Road freight stays on the surface when the selected resident is "
          "underwater");
    save = midway;
    const auto id = save.freight_carts[0].id;
    Check(frontier::ReturnFreight(save, id).ok && save.stocks[0][0] == 70 &&
              !frontier::ReturnFreight(save, id).ok,
          "Cancellation starts one physical return, without an instant refund");
    frontier::WriteSave(directory / "return.json", save);
    auto returned = frontier::LoadSave(directory / "return.json");
    Run(world, returned, 600);
    Check(returned.freight_carts[0].route.empty() &&
              returned.stocks[0][0] == 80 && returned.stocks[1][0] == 100,
          "Saved return travel restores cargo only after reaching the source");

    save = ready;
    save.stocks[1][0] = 999995;
    frontier::ShipGoods(save, 0, 1, 0, 10);
    Run(world, save, 600);
    Check(save.stocks[1][0] == 1000000 && save.freight_carts[0].quantity == 5,
          "Full storage leaves the unaccepted remainder in the real cart");
    Check(frontier::ReturnFreight(save, save.freight_carts[0].id).ok,
          "A partially unloaded cart can return only its remaining cargo");
    Run(world, save, 600);
    Check(save.stocks[0][0] == 75 && save.stocks[1][0] == 1000000,
          "Partial delivery and return conserve the exact total quantity");

    save = midway;
    save.relations[1].treaty = frontier::Treaty::kWar;
    auto stopped = save.freight_carts[0].position;
    Run(world, save, 50);
    Check(save.freight_carts[0].position.x == stopped.x &&
              save.freight_carts[0].quantity == 10,
          "Hostile endpoint suspends freight without losing cargo");
    frontier::WriteSave(directory / "hostile.json", save);
    auto hostile = frontier::LoadSave(directory / "hostile.json");
    hostile.relations[1].treaty = frontier::Treaty::kPeace;
    Run(world, hostile, 600);
    Check(hostile.stocks[1][0] == 110,
          "Peace resumes a saved suspended delivery");

    save = ready;
    save.roads[0].points = {{0, 0}, {1000, 0}, {1000, 700}, {2000, 700}};
    frontier::ShipGoods(save, 0, 1, 0, 10);
    Run(world, save, 800);
    Check(save.stocks[1][0] == 110,
          "A cart follows bent polylines through exact corner waypoints");
    save = midway;
    world.chunks.erase({2, 0});
    Run(world, save, 600);
    Check(save.freight_carts[0].quantity == 10 && save.stocks[1][0] == 100 &&
              save.freight_carts[0].position.x < 1536,
          "The leading footprint stops before an unloaded road chunk");
    Terrain(world);
    Run(world, save, 600);
    Check(save.stocks[1][0] == 110,
          "Loading the next road chunk resumes the same delivery");
    save = midway;
    world.chunks.clear();
    Run(world, save, 50);
    Check(save.freight_carts[0].position.x ==
                  midway.freight_carts[0].position.x &&
              save.freight_carts[0].quantity == 10,
          "Unloaded terrain pauses physical freight rather than running "
          "background simulation");
    Terrain(world);
    world.chunks.at(frontier::ChunkAt(save.freight_carts[0].position))
        .enemies.push_back({900008, save.freight_carts[0].position,
                            save.freight_carts[0].position,
                            save.freight_carts[0].position});
    Run(world, save, 50);
    Check(
        save.freight_carts[0].position.x == midway.freight_carts[0].position.x,
        "A nearby threat pauses the transport");
    Terrain(world);
    save = ready;
    world.buildings = frontier::MakeBuildings();
    frontier::ShipGoods(save, 0, 1, 0, 10);
    Run(world, save, 700);
    Check(save.stocks[1][0] == 110,
          "Town connectors go around actual hall and house collision geometry");
    world.buildings.clear();

    save = midway;
    for (auto& tile :
         world.chunks.at(frontier::ChunkAt(save.freight_carts[0].position))
             .tiles) {
      tile.terrain = frontier::Terrain::kDeep;
    }
    Run(world, save, 30);
    Check(
        frontier::Length(save.freight_carts[0].position -
                         midway.freight_carts[0].position) > 100,
        "The cart traverses water only on the existing road bridge footprint");
    Terrain(world);
    save = midway;
    save.roads.clear();
    Run(world, save, 30);
    Check(
        save.freight_carts[0].position.x ==
                midway.freight_carts[0].position.x &&
            save.freight_carts[0].quantity == 10,
        "Missing roads suspend freight instead of inventing an overland route");
    save = ready;
    for (int y = -35; y <= 35; ++y) {
      for (int x = -20; x <= 55; ++x) {
        save.explored.insert({x, y});
      }
    }
    save.relations[2] = {50, frontier::Treaty::kPeace, true};
    save.roads = {
        {0,
         2,
         {frontier::Settlements()[0].pos, frontier::Settlements()[2].pos}},
        {2,
         1,
         {frontier::Settlements()[2].pos, frontier::Settlements()[1].pos}}};
    Check(frontier::ShipGoods(save, 0, 1, 0, 10).ok,
          "Existing roads can connect owned endpoints through a peaceful "
          "foreign transit town");
    save.relations[2].treaty = frontier::Treaty::kWar;
    Run(world, save, 30);
    Check(frontier::Length(save.freight_carts[0].position -
                           frontier::FreightDepot(0)) == 0 &&
              save.freight_carts[0].quantity == 10,
          "A hostile transit town suspends the entire selected route");
    save = ready;
    save.explored.clear();
    Check(!frontier::ShipGoods(save, 0, 1, 0, 5).ok && save.stocks[0][0] == 80,
          "Dispatch rejects an unexplored road route before reserving cargo");
    save = midway;
    auto json = frontier::WriteNation(save);
    json["freight_carts"][0]["route"][3]["position"] = {90000, 90000};
    auto bad = save;
    bad.freight_carts.clear();
    bad.production.clear();
    bad.plots.clear();
    bad.city_tasks.clear();
    bad.shipments.clear();
    bad.travelers.clear();
    bool rejected = false;
    try {
      frontier::ReadNation(json, bad);
    } catch (const std::exception&) {
      rejected = true;
    }
    Check(rejected, "Corrupt persisted off-road freight is rejected");
    frontier::WriteSave(path, ready);
    auto legacy = nlohmann::json::parse(std::ifstream(path));
    legacy["version"] = 12;
    legacy["nation"].erase("freight_carts");
    uint64_t shipment_id = legacy["nation"]["next_entity"].get<uint64_t>();
    legacy["nation"]["next_entity"] = shipment_id + 1;
    legacy["stocks"][0][0] = 75;
    legacy["nation"]["shipments"].push_back({{"id", shipment_id},
                                             {"from", 0},
                                             {"to", 1},
                                             {"good", 0},
                                             {"quantity", 5},
                                             {"due", ready.game_time + 37}});
    {
      std::ofstream output(path);
      output << legacy;
    }
    auto migrated = frontier::LoadSave(path);
    Check(migrated.freight_carts.empty(),
          "v12 migration does not grant or duplicate carts");
    Check(migrated.shipments.size() == 1 &&
              migrated.shipments[0].due == ready.game_time + 37 &&
              migrated.shipments[0].quantity == 5 &&
              migrated.stocks[0][0] == 75,
          "Migration preserves existing timer escrow and its exact remaining "
          "travel time");
    frontier::WriteSave(path, migrated);
    Check(std::filesystem::exists(path.string() + ".v12.bak"),
          "v13 rewrite preserves a v12 backup");
    std::filesystem::remove_all(directory);
    std::cout << "physical road freight, cargo conservation, route access and "
                 "migration passed\n";
  } catch (const std::exception& error) {
    std::filesystem::remove_all(directory);
    std::cerr << error.what() << '\n';
    return 1;
  }
}
