#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "adventure.hpp"
#include "nation.hpp"
#include "resident_orders.hpp"
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
  save.player = {60, 60};
  frontier::StoreControlled(save);
  save.skills.insert("gathering");
  for (auto& c : save.citizens) {
    if (c.npc != save.controlled_npc) {
      c.position = {24, 24};
    }
  }
  for (int y = -6; y <= 6; ++y) {
    for (int x = -6; x <= 6; ++x) {
      save.explored.insert({x, y});
    }
  }
  return save;
}
void Terrain(frontier::World& world) {
  world.chunks.clear();
  world.buildings.clear();
  for (int y = -1; y <= 0; ++y) {
    for (int x = -1; x <= 0; ++x) {
      frontier::Chunk chunk;
      chunk.coord = {x, y};
      chunk.tiles.resize(frontier::kChunkTiles * frontier::kChunkTiles);
      world.chunks.emplace(chunk.coord, std::move(chunk));
    }
  }
}
void Run(frontier::World& world, frontier::Save& save, int ticks) {
  for (int i = 0; i < ticks; ++i) {
    frontier::TickResidentOrders(world, save, .1f);
  }
}
int Worker(const frontier::Save& save, int skip = -1) {
  for (const auto& c : save.citizens) {
    if (c.npc != save.controlled_npc && c.npc != skip) {
      return c.npc;
    }
  }
  throw std::runtime_error("Need an uncontrolled resident");
}
}  // namespace

int main() {
  auto directory =
      std::filesystem::temp_directory_path() /
      ("frontier-orders-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    auto initial = NewState();
    int worker = Worker(initial);
    int second = Worker(initial, worker);
    const auto path = directory / "state.json";
    // Only one World is alive. Fixtures never reconfigure its global metadata.
    frontier::World world(initial.seed);
    Terrain(world);
    const frontier::Resource berry{
        900001, {168, 24}, frontier::ResourceKind::kBerry, 0};
    world.chunks.at({0, 0}).resources = {berry};
    auto save = initial;
    auto* c = frontier::GetCitizen(save, worker);
    save.skills.clear();
    Check(!frontier::OrderResident(save, worker, "gather", 2).ok,
          "Orders enforce Gathering authoritatively");
    save.skills.insert("gathering");
    Check(!frontier::OrderResident(save, worker, "plant", 1).ok &&
              !frontier::OrderResident(save, worker, "gather", 7).ok,
          "Orders reject nonrenewable planting and unsupported resources");
    Check(frontier::OrderResident(save, worker, "gather", 2).ok,
          "A free resident can receive a gathering order");
    auto before = save.stocks[0][2];
    Run(world, save, 1);
    Check(c->position.x == 24 && save.harvested.empty(),
          "Finding a target neither teleports nor harvests it");
    frontier::TickResidentOrders(world, save, 0);
    Check(save.harvested.empty(), "Paused simulation does no work");
    Run(world, save, 60);
    Check(save.harvested.contains(berry.id) &&
              save.stocks[0][2] == before + 2 && c->cargo.berries == 0 &&
              save.inventory.berries == 0 &&
              save.gathered == initial.gathered + 1 &&
              frontier::MoneySupply(save) == initial.money_supply,
          "Worker reaches site, gathers once into their own cargo and walks it "
          "to the warehouse without minting coins");
    Check(!frontier::Harvest(save, berry).ok,
          "Direct control cannot gather a site already consumed by a worker");

    save = initial;
    frontier::OrderResident(save, worker, "gather", 2);
    world.talking_npc = worker;
    Run(world, save, 50);
    Check(save.harvested.empty(), "Conversation pauses autonomous work");
    world.talking_npc = -1;
    Run(world, save, 100);
    Check(save.harvested.contains(berry.id),
          "Worker resumes after conversation");
    save = initial;
    auto& worker_order = frontier::GetCitizen(save, worker)->order;
    frontier::OrderResident(save, worker, "gather", 0);
    worker_order.target = berry.id;
    worker_order.phase = "work";
    frontier::GetCitizen(save, worker)->position = {132, 24};
    save.inventory.axe = true;
    world.chunks.at({0, 0}).resources[0].kind = frontier::ResourceKind::kTree;
    Run(world, save, 4);
    Check(save.harvested.empty(),
          "Selected resident's axe does not speed up another resident");
    Run(world, save, 4);
    Check(save.harvested.contains(berry.id),
          "Unarmed tree gathering follows the normal duration");
    world.chunks.at({0, 0}).resources[0].kind = frontier::ResourceKind::kBerry;
    save = initial;
    save.inventory.rod = true;
    Check(!frontier::OrderResident(save, worker, "gather", 4).ok,
          "Selected resident's rod cannot enable another worker's fishing");
    auto* specialist = frontier::GetCitizen(save, worker);
    specialist->cargo.rod = true;
    save.skills.insert("fishing");
    world.chunks.at({0, 0}).resources[0].kind = frontier::ResourceKind::kFish;
    Check(frontier::OrderResident(save, worker, "gather", 4).ok,
          "A trained fisher with a personal rod accepts work");
    Run(world, save, 150);
    Check(save.caught == initial.caught + 1 &&
              save.stocks[0][4] == initial.stocks[0][4] + 1,
          "Fishing consumes the real site and delivers fish with shared "
          "practice");
    save = initial;
    specialist = frontier::GetCitizen(save, worker);
    specialist->cargo.pickaxe = true;
    save.skills.insert("tools");
    save.skills.erase("crafting");
    save.technologies.insert("ironworking");
    Check(!frontier::OrderResident(save, worker, "gather", 6).ok,
          "Mining checks the full Tools ancestor branch");
    save.skills.insert("crafting");
    world.chunks.at({0, 0}).resources[0].kind = frontier::ResourceKind::kOre;
    Check(frontier::OrderResident(save, worker, "gather", 6).ok,
          "A properly equipped miner with shared knowledge accepts work");
    Run(world, save, 150);
    Check(save.stocks[0][6] == initial.stocks[0][6] + 1 &&
              save.harvested.contains(berry.id),
          "Mining carries the actual ore to the warehouse");
    world.chunks.at({0, 0}).resources[0].kind = frontier::ResourceKind::kBerry;
    save = initial;
    world.buildings = frontier::MakeBuildings();
    frontier::GetCitizen(save, worker)->position = {70, 70};
    frontier::OrderResident(save, worker, "gather", 2);
    Run(world, save, 150);
    Check(save.harvested.contains(berry.id) && save.stocks[0][2] == before + 2,
          "A real town hall footprint does not block warehouse delivery");
    world.buildings.clear();
    save = initial;
    save.submerged = true;
    frontier::OrderResident(save, worker, "gather", 2);
    Run(world, save, 150);
    Check(save.harvested.contains(berry.id),
          "Surface worker routing does not inherit the controlled resident's "
          "underwater layer");
    save = initial;
    save.walls = {{0, {90, -55}, {245, -55}, 200},
                  {0, {245, -55}, {245, 100}, 200},
                  {0, {245, 100}, {90, 100}, 200},
                  {0, {90, 100}, {90, -55}, 200}};
    frontier::OrderResident(save, worker, "gather", 2);
    Run(world, save, 150);
    Check(save.harvested.empty() && save.stocks[0][2] == before,
          "A worker never gathers through a closed collision boundary");
    save = initial;
    frontier::OrderResident(save, worker, "gather", 2);
    frontier::Gameplay game;
    for (int i = 0; i < 150; ++i) {
      world.Tick(.1f, save, i * .1);
      game.Tick(world, save, .1f, i * .1);
    }
    Check(save.harvested.contains(berry.id) && save.stocks[0][2] == before + 2,
          "The simulation tick integrates authoritative orders and actor "
          "synchronization");
    save = initial;
    save.explored.clear();
    frontier::OrderResident(save, worker, "gather", 2);
    Run(world, save, 100);
    Check(save.harvested.empty(), "Workers cannot gather unexplored resources");
    save = initial;
    auto& chunk = world.chunks.at({0, 0});
    chunk.enemies.push_back({900009, {70, 24}, {70, 24}, {70, 24}});
    frontier::OrderResident(save, worker, "gather", 2);
    Run(world, save, 100);
    Check(save.harvested.empty() &&
              frontier::GetCitizen(save, worker)->order.status.find("Danger") !=
                  std::string::npos,
          "Nearby living wolves pause work");
    save.defeated.insert(900009);
    Run(world, save, 100);
    Check(save.harvested.contains(berry.id),
          "Work resumes after danger is gone");
    chunk.enemies.clear();

    save = initial;
    frontier::OrderResident(save, worker, "gather", 2);
    frontier::OrderResident(save, second, "gather", 2);
    Run(world, save, 100);
    Check(save.stocks[0][2] == before + 2 &&
              save.gathered == initial.gathered + 1,
          "Competing residents never duplicate a resource");

    save = initial;
    save.stocks[0][2] = 1000000;
    frontier::OrderResident(save, worker, "gather", 2);
    Run(world, save, 100);
    c = frontier::GetCitizen(save, worker);
    Check(c->cargo.berries == 2 && save.stocks[0][2] == 1000000,
          "A full warehouse retains the real cargo with its worker");
    auto resumed = save;
    resumed.stocks[0][2] -= 2;
    Run(world, resumed, 50);
    Check(
        frontier::GetCitizen(resumed, worker)->cargo.berries == 0 &&
            resumed.stocks[0][2] == 1000000,
        "A waiting delivery resumes exactly once when warehouse space appears");
    auto held_position = c->position;
    Check(
        frontier::CancelResidentOrder(save, worker).ok && c->cargo.berries == 2,
        "Cancellation preserves cargo");
    Run(world, save, 50);
    Check(c->position.x == held_position.x && c->position.y == held_position.y,
          "A cancelled worker remains in place");
    frontier::OrderResident(save, worker, "gather", 2);
    Check(frontier::SelectResident(save, worker).ok &&
              save.inventory.berries == 2 && c->order.kind == "idle",
          "Taking control uses the same cargo, not a second inventory");

    save = initial;
    frontier::OrderResident(save, worker, "gather", 2);
    c = frontier::GetCitizen(save, worker);
    c->position = {132, 24};
    c->order.phase = "work";
    c->order.target = berry.id;
    c->order.progress = .3f;
    frontier::WriteSave(path, save);
    auto partial = frontier::LoadSave(path);
    Check(frontier::GetCitizen(partial, worker)->order.progress == .3f,
          "In-progress work survives loading at its exact progress");
    Run(world, partial, 5);
    Check(partial.harvested.contains(berry.id) &&
              frontier::GetCitizen(partial, worker)->cargo.berries == 2,
          "Loaded in-progress work produces a single actor-owned batch");
    save = initial;
    save.stocks[0][0] = 0;
    frontier::OrderResident(save, worker, "plant", 2);
    Run(world, save, 100);
    Check(save.plots.empty() && save.stocks[0][2] == initial.stocks[0][2],
          "Unavailable planting inputs never consume a partial cost");
    save = initial;
    frontier::OrderResident(save, worker, "plant", 0);
    c = frontier::GetCitizen(save, worker);
    Run(world, save, 1);
    Check(c->order.phase == "supply" && c->cargo.wood == 0,
          "A planter first plans a physical warehouse visit");
    for (int i = 0; i < 100 && c->cargo.wood == 0; ++i) {
      Run(world, save, 1);
    }
    Check(c->cargo.wood == 2 && c->cargo.berries == 2 &&
              save.stocks[0][0] == initial.stocks[0][0] - 2 &&
              save.plots.empty(),
          "Supplies move once from warehouse to worker before planting");
    frontier::WriteSave(path, save);
    auto supplied = frontier::LoadSave(path);
    Check(frontier::GetCitizen(supplied, worker)->cargo.wood == 2,
          "Carried planting inputs survive save/load without a second "
          "reservation");
    frontier::CancelResidentOrder(save, worker);
    Check(c->cargo.wood == 2 && c->cargo.berries == 2,
          "Cancelling a planter retains supplies in their physical cargo");
    save = initial;
    world.chunks.at({0, 0}).resources.clear();
    frontier::OrderResident(save, worker, "plant", 2);
    int wood = save.stocks[0][0];
    int berries = save.stocks[0][2];
    Run(world, save, 500);
    Check(save.plots.size() == 3 && save.stocks[0][0] == wood - 6 &&
              save.stocks[0][2] == berries - 6 &&
              save.plots[0].resource.id != 0,
          "Planter walks to distinct sites, pays once per plot and stops at "
          "the territorial limit");
    for (const auto& p : save.plots) {
      Check(!p.ready && p.due == save.game_time + 180,
            "New physical berry plants grow on simulation time");
    }
    frontier::WriteSave(path, save);
    auto loaded = frontier::LoadSave(path);
    Check(loaded.plots.size() == 3 &&
              loaded.plots[0].resource.pos.x == save.plots[0].resource.pos.x &&
              frontier::GetCitizen(loaded, worker)->order.kind == "plant" &&
              frontier::GetCitizen(loaded, worker)->order.route.empty(),
          "Spatial planting and resident orders survive a save without a stale "
          "route");
    auto corrupt = nlohmann::json::parse(std::ifstream(path));
    corrupt["nation"]["residents"][0]["order"]["kind"] = "mint_coins";
    {
      std::ofstream output(directory / "corrupt.json");
      output << corrupt;
    }
    bool rejected = false;
    try {
      (void)frontier::LoadSave(directory / "corrupt.json");
    } catch (const std::exception&) {
      rejected = true;
    }
    Check(rejected, "Unknown persisted commands are rejected");
    auto legacy = nlohmann::json::parse(std::ifstream(path));
    legacy["version"] = 11;
    legacy["nation"]["plots"] = nlohmann::json::array();
    for (auto& row : legacy["nation"]["residents"]) {
      row.erase("order");
    }
    {
      std::ofstream output(path);
      output << legacy;
    }
    auto migrated = frontier::LoadSave(path);
    Check(frontier::GetCitizen(migrated, worker)->order.kind == "idle",
          "Older resident saves default to idle without resetting progression");
    frontier::WriteSave(path, migrated);
    Check(std::filesystem::exists(path.string() + ".v11.bak"),
          "Writing a migrated v11 save preserves the original backup");

    save.game_time += 180;
    frontier::TickNation(save, .1f);
    frontier::CancelResidentOrder(save, worker);
    auto plot = save.plots[0].resource;
    save.player = plot.pos;
    frontier::StoreControlled(save);
    Check(world.Nearest(save.player, save, 5) != nullptr &&
              !frontier::HarvestPlot(save, 0).ok,
          "A mature planted resource is gathered on site rather than remotely "
          "teleported to stock");
    int carried = save.inventory.berries;
    Check(frontier::Harvest(save, plot).ok &&
              save.inventory.berries == carried + 12 &&
              save.plots.size() == 2 && !frontier::Harvest(save, plot).ok,
          "Manual harvest of planted berries yields one real cargo batch");
    frontier::OrderResident(save, worker, "gather", 2);
    int stock = save.stocks[0][2];
    Run(world, save, 500);
    Check(save.plots.empty() && save.stocks[0][2] == stock + 24,
          "Workers harvest mature plants and carry their exact yield home");

    save = initial;
    frontier::OrderResident(save, worker, "gather", 2);
    frontier::GetCitizen(save, worker)->cargo.berries = 2;
    Check(frontier::LoseResident(save, worker, true, 1).ok &&
              frontier::GetCitizen(save, worker)->cargo.berries == 2 &&
              frontier::GetCitizen(save, worker)->order.kind == "idle",
          "Capture cancels an order while preserving the resident's gear");
    Check(!frontier::OrderResident(save, worker, "plant", 0).ok,
          "Captives cannot be assigned work");
    save = initial;
    frontier::OrderResident(save, worker, "gather", 2);
    frontier::GetCitizen(save, worker)->cargo.berries = 2;
    Check(frontier::LoseResident(save, worker, false).ok &&
              frontier::GetCitizen(save, worker)->cargo.berries == 0 &&
              frontier::GetCitizen(save, worker)->order.kind == "idle",
          "Resident death cancels work permanently");
    save = initial;
    frontier::OrderResident(save, worker, "gather", 2);
    world.chunks.clear();
    Run(world, save, 100);
    Check(save.harvested.empty() &&
              frontier::GetCitizen(save, worker)->position.x == 24,
          "Unloaded terrain pauses orders without generating resources or "
          "teleporting");
    std::filesystem::remove_all(directory);
    std::cout << "resident orders, physical planting, real cargo, safety and "
                 "save migration passed\n";
  } catch (const std::exception& error) {
    std::filesystem::remove_all(directory);
    std::cerr << error.what() << '\n';
    return 1;
  }
}
