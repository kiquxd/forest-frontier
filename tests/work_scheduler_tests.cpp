#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "adventure.hpp"
#include "freight.hpp"
#include "nation.hpp"
#include "resident_orders.hpp"
#include "settlement_system.hpp"
#include "work_scheduler.hpp"

namespace {
using Clock = std::chrono::steady_clock;
void Check(bool ok, const char* message) {
  if (!ok) {
    throw std::runtime_error(message);
  }
}
frontier::Save InitialState() {
  frontier::Save save;
  frontier::ConfigureWorld(save.seed);
  frontier::InitializeCampaign(save, true);
  frontier::InitializeAdventure(save);
  frontier::InitializeNation(save);
  save.player = {1900, 0};
  frontier::StoreControlled(save);
  for (auto& c : save.citizens) {
    if (c.npc != save.controlled_npc) {
      c.position = {24, 24};
    }
  }
  for (int y = -8; y <= 8; ++y) {
    for (int x = -8; x <= 8; ++x) {
      save.explored.insert({x, y});
    }
  }
  return save;
}
int Worker(const frontier::Save& save) {
  for (const auto& c : save.citizens) {
    if (c.npc != save.controlled_npc) {
      return c.npc;
    }
  }
  throw std::runtime_error("Missing worker fixture");
}
void Generated(frontier::World& world) {
  auto deadline = Clock::now() + std::chrono::seconds(3);
  do {
    world.Integrate(64);
    if (world.Pending() == 0) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  } while (Clock::now() < deadline);
  throw std::runtime_error("Background generation timed out");
}
void Step(frontier::World& world, frontier::Save& save,
          frontier::WorkScheduler& scheduler,
          size_t budget = frontier::WorkScheduler::kMaxEventsPerStep) {
  save.game_time += .1;
  world.Integrate(64);
  scheduler.Tick(world, save, .1f, budget);
}
void Prepare(frontier::World& world, frontier::Save& save,
             frontier::WorkScheduler& scheduler) {
  world.Request(save.player, 1);
  Step(world, save, scheduler);
  Generated(world);
  world.buildings.clear();
  for (auto& [coord, chunk] : world.chunks) {
    if (!world.ActiveAt({(coord.x + .5f) * frontier::kChunkSize,
                         (coord.y + .5f) * frontier::kChunkSize})) {
      chunk.resources.clear();
      chunk.underwater.clear();
      chunk.enemies.clear();
      chunk.animals.clear();
    }
  }
}
void WorkOutsideView(const frontier::Save& initial) {
  auto save = initial;
  const int worker = Worker(save);
  Check(frontier::OrderResident(save, worker, "gather", 2).ok,
        "Assign remote worker");
  frontier::World world(save.seed);
  frontier::WorkScheduler scheduler;
  Prepare(world, save, scheduler);
  Check(world.ActiveAt(save.player) && !world.ActiveAt({168, 24}) &&
            world.chunks.contains({0, 0}),
        "Work terrain is generated separately from the player's active area");
  auto& chunk = world.chunks.at({0, 0});
  chunk.resources.push_back(
      {900001, {168, 24}, frontier::ResourceKind::kBerry, 0});
  chunk.enemies.push_back({900008, {150, 24}, {150, 24}, {150, 24}});
  chunk.enemies[0].cooldown = 3;
  chunk.enemies[0].flash = 2;
  chunk.enemies[0].windup = 1;
  chunk.animals.push_back({{140, 110}, {140, 110}, 1, {140, 110}, 0});
  frontier::Npc foreign;
  foreign.id = 90000;
  foreign.pos = foreign.home = foreign.previous = {100, 100};
  foreign.phase = 1;
  world.npcs.push_back(foreign);
  const auto known = save.explored;
  const int stock = save.stocks[0][2];
  frontier::Gameplay game;
  // Exercise the production integration, including ordinary NPC and combat
  // updates, rather than only calling the worker helper directly.
  for (int i = 0; i < 100; ++i) {
    world.Integrate(64);
    world.Tick(.1f, save, i * .1);
    game.Tick(world, save, .1f, i * .1);
  }
  const auto& remote = world.chunks.at({0, 0});
  Check(save.harvested.contains(900001) && save.stocks[0][2] == stock + 2 &&
            frontier::GetCitizen(save, worker)->cargo.berries == 0,
        "Off-camera resident physically gathers and delivers without following "
        "the player");
  Check(remote.enemies[0].cooldown == 3 && remote.enemies[0].flash == 2 &&
            remote.enemies[0].windup == 1 && remote.animals[0].pos.x == 140 &&
            world.npcs.back().pos.x == 100 && world.npcs.back().pos.y == 100,
        "Service terrain does not activate enemy timers, animals or foreign "
        "NPC movement");
  Check(
      save.explored == known && !save.defeated.contains(900008),
      "Background work reveals no map and awards no off-camera combat rewards");
  save.player = {60, 60};
  frontier::StoreControlled(save);
  world.Request(save.player, 1);
  Generated(world);
  world.Tick(.1f, save, 12);
  game.Tick(world, save, .1f, 12);
  Check(world.chunks.at({0, 0}).enemies[0].cooldown < 3 &&
            frontier::Length(world.npcs.back().pos - foreign.pos) > 0,
        "Returning the player activates normal local NPC and combat updates");
  save.player = {1900, 0};
  frontier::StoreControlled(save);
  world.Request(save.player, 1);
  frontier::CancelResidentOrder(save, worker);
  Step(world, save, scheduler);
  Generated(world);
  Check(!world.chunks.contains({0, 0}),
        "Cancelling the last job releases remote terrain");
}
void ScheduleAndPersistence(const frontier::Save& initial,
                            const std::filesystem::path& directory) {
  auto save = initial;
  const int worker = Worker(save);
  frontier::OrderResident(save, worker, "gather", 2);
  frontier::World world(save.seed);
  frontier::WorkScheduler scheduler;
  Prepare(world, save, scheduler);
  world.chunks.at({0, 0}).resources = {
      {900001, {168, 24}, frontier::ResourceKind::kBerry, 0}};
  auto* c = frontier::GetCitizen(save, worker);
  c->position = {132, 24};
  c->order.phase = "work";
  c->order.target = 900001;
  c->order.progress = .3f;
  c->order.next_update = save.game_time + 1;
  Step(world, save, scheduler);
  Check(c->order.progress == .3f && scheduler.LastStep().events == 0,
        "A future completion deadline does not execute early");
  const auto path = directory / "work.json";
  frontier::WriteSave(path, save);
  auto loaded = frontier::LoadSave(path);
  auto* restored = frontier::GetCitizen(loaded, worker);
  Check(restored->order.next_update == c->order.next_update &&
            restored->order.progress == .3f,
        "Work progress and scheduled simulation deadline roundtrip exactly");
  frontier::WorkScheduler resumed;
  resumed.Tick(world, loaded, 0);
  Check(restored->order.progress == .3f,
        "Paused menus do not service background events");
  resumed.Tick(world, loaded, .1f);
  Check(restored->order.progress == .3f,
        "Reopening a world adds no wall-clock/offline progress");
  loaded.game_time = restored->order.next_update;
  resumed.Tick(world, loaded, .1f);
  Check(restored->order.progress > .39f && restored->order.progress < .41f,
        "A restored deadline executes exactly one work step");
  frontier::CancelResidentOrder(loaded, worker);
  for (int i = 0; i < 20; ++i) {
    Step(world, loaded, resumed);
  }
  Check(!loaded.harvested.contains(900001),
        "Cancelled queue entries cannot harvest later");

  auto crowded = initial;
  for (auto& resident : crowded.citizens) {
    if (resident.npc != crowded.controlled_npc) {
      frontier::OrderResident(crowded, resident.npc, "gather", 2);
    }
  }
  frontier::WorkScheduler limited;
  Step(world, crowded, limited, 1);
  Generated(world);
  Step(world, crowded, limited, 1);
  Check(limited.LastStep().events == 1 && limited.LastStep().overdue > 0,
        "A bounded step leaves deferred work queued rather than processing an "
        "unbounded batch");
  const auto budget_path = directory / "budget.json";
  frontier::WriteSave(budget_path, crowded);
  auto budget_loaded = frontier::LoadSave(budget_path);
  Check(frontier::GetCitizen(budget_loaded, worker)->order.next_update ==
            frontier::GetCitizen(crowded, worker)->order.next_update,
        "Deferred deadlines survive save/load without losing the queue's "
        "pending work");

  auto legacy = nlohmann::json::parse(std::ifstream(path));
  legacy["version"] = 13;
  for (auto& row : legacy["nation"]["residents"]) {
    row["order"].erase("next_update");
  }
  {
    std::ofstream output(path);
    output << legacy;
  }
  auto migrated = frontier::LoadSave(path);
  Check(frontier::GetCitizen(migrated, worker)->order.next_update == -1 &&
            frontier::GetCitizen(migrated, worker)->order.progress == .3f,
        "v13 loads with no invented offline deadline and preserves earned work "
        "progress");
  frontier::WriteSave(path, migrated);
  Check(std::filesystem::exists(path.string() + ".v13.bak"),
        "v14 migration preserves the original v13 save");
  std::unordered_set<frontier::Coord, frontier::CoordHash> excessive;
  for (size_t i = 0; i <= frontier::World::kMaxWorkChunks; ++i) {
    excessive.insert({static_cast<int>(i), 0});
  }
  bool rejected = false;
  try {
    world.RequestWork(excessive);
  } catch (const std::exception&) {
    rejected = true;
  }
  Check(rejected, "Service terrain requests cannot exceed the bounded cache");
}
frontier::Save FreightState(const frontier::Save& initial) {
  auto save = initial;
  save.player = {10000, -10000};
  frontier::StoreControlled(save);
  save.owners[1] = 0;
  save.relations[0] = save.relations[1] = {50, frontier::Treaty::kPeace, true};
  for (auto& stock : save.stocks) {
    stock.fill(100000);
  }
  for (int y = -10; y <= 20; ++y) {
    for (int x = -10; x <= 35; ++x) {
      save.explored.insert({x, y});
    }
  }
  save.roads.push_back({0, 1, {{0, 0}, {1000, 350}, {2000, 700}}});
  return save;
}
void OffCameraFreight(const frontier::Save& initial,
                      const std::filesystem::path& directory) {
  auto save = FreightState(initial);
  Check(frontier::BuildFreightCart(save, 0).ok &&
            frontier::ShipGoods(save, 0, 1, 0, 10).ok,
        "Dispatch off-camera cart");
  const int stock = save.stocks[1][0];
  frontier::World world(save.seed);
  frontier::WorkScheduler scheduler;
  world.Request(save.player, 1);
  const auto deadline = Clock::now() + std::chrono::seconds(5);
  bool persisted = false;
  for (int i = 0; i < 2000 && !save.freight_carts[0].route.empty(); ++i) {
    world.Integrate(64);
    // This streaming scenario supplies a cleared route. Obstruction and cargo
    // retention are covered separately by the freight tests.
    for (const auto& [coord, chunk] : world.chunks) {
      for (const auto& resource : chunk.resources) {
        save.harvested.insert(resource.id);
      }
    }
    Step(world, save, scheduler);
    if (world.Pending()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!persisted && save.freight_carts[0].waypoint > 10) {
      frontier::WriteSave(directory / "cart.json", save);
      auto loaded = frontier::LoadSave(directory / "cart.json");
      Check(loaded.freight_carts[0].next_update ==
                    save.freight_carts[0].next_update &&
                loaded.freight_carts[0].quantity == 10,
            "Freight deadlines and exact cargo persist mid-trip");
      persisted = true;
    }
    Check(!world.ActiveAt(save.freight_carts[0].position),
          "Advancing a service cart never activates its distant terrain");
    Check(world.chunks.size() <= 9 + frontier::World::kMaxWorkChunks,
          "View and service cache stay bounded during streaming");
    Check(Clock::now() < deadline,
          "Off-camera freight scenario exceeded its CPU deadline");
  }
  if (!save.freight_carts[0].route.empty()) {
    const auto& cart = save.freight_carts[0];
    std::cerr << "cart: " << cart.status << " waypoint " << cart.waypoint << '/'
              << cart.route.size() << " position " << cart.position.x << ','
              << cart.position.y << " time " << save.game_time << '\n';
  }
  Check(persisted && save.freight_carts[0].route.empty() &&
            save.stocks[1][0] == stock + 10,
        "Freight loads its own road sections and delivers without a following "
        "player");
}
void Benchmark(const frontier::Save& initial) {
  auto save = FreightState(initial);
  for (int i = 0; i < 64; ++i) {
    Check(frontier::BuildFreightCart(save, 0).ok &&
              frontier::DispatchFreight(save, 0, 1, 0, 1).ok,
          "Scheduler benchmark dispatch");
  }
  frontier::World world(save.seed);
  frontier::WorkScheduler scheduler;
  world.Request(save.player, 1);
  const auto deadline = Clock::now() + std::chrono::seconds(5);
  std::vector<double> times;
  double total = 0;
  size_t max_overdue = 0;
  size_t max_chunks = 0;
  while (Clock::now() < deadline) {
    for (const auto& [coord, chunk] : world.chunks) {
      for (const auto& resource : chunk.resources) {
        save.harvested.insert(resource.id);
      }
    }
    for (const auto& cart : save.freight_carts) {
      if (cart.route.empty()) {
        Check(
            frontier::DispatchFreight(save, cart.city, 1 - cart.city, 0, 1).ok,
            "Scheduler benchmark return dispatch");
      }
    }
    auto begin = Clock::now();
    Step(world, save, scheduler);
    double elapsed =
        std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
    times.push_back(elapsed);
    total += elapsed;
    max_overdue = std::max(max_overdue, scheduler.LastStep().overdue);
    max_chunks = std::max(max_chunks, world.chunks.size());
  }
  std::sort(times.begin(), times.end());
  std::cout << "CPU scheduler/stream integration sample: 64 carts, 5 seconds, "
            << times.size() << " steps; mean_ms=" << total / times.size()
            << ", p95_ms=" << times[times.size() * 95 / 100]
            << ", max_ms=" << times.back() << ", max_overdue=" << max_overdue
            << ", max_chunks=" << max_chunks << '\n';
}
}  // namespace
int main(int argc, char** argv) {
  const auto directory =
      std::filesystem::temp_directory_path() /
      ("frontier-scheduler-" +
       std::to_string(Clock::now().time_since_epoch().count()));
  try {
    auto initial = InitialState();
    if (argc == 2 && std::string(argv[1]) == "--benchmark") {
      Benchmark(initial);
      return 0;
    }
    WorkOutsideView(initial);
    ScheduleAndPersistence(initial, directory);
    OffCameraFreight(initial, directory);
    std::filesystem::remove_all(directory);
    std::cout << "service terrain, bounded event scheduling, off-camera "
                 "work/freight and persistence passed\n";
  } catch (const std::exception& error) {
    std::filesystem::remove_all(directory);
    std::cerr << error.what() << '\n';
    return 1;
  }
}
