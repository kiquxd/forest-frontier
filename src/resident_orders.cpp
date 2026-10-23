#include "resident_orders.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <vector>

#include "adventure.hpp"
#include "nation.hpp"
#include "progress.hpp"
#include "settlement_system.hpp"

namespace frontier {
namespace {
constexpr float kWorkReach = 58;
constexpr int kCarryBatch = 10;

Action WorkRequirements(const Save& save, const Citizen& c, int good) {
  std::string skill = good == 4 ? "fishing" : good == 6 ? "tools" : "gathering";
  if (!HasSkill(save, skill) ||
      !MissingSkillPrerequisites(save, skill).empty()) {
    return {false, "Learn the full " + SkillName(skill) + " branch first"};
  }
  if (good == 4 && !c.cargo.rod) {
    return {false, "This resident needs their own fishing rod"};
  }
  if (good == 6 &&
      (!c.cargo.pickaxe || !save.technologies.contains("ironworking"))) {
    return {false, "Ore mining needs this resident's pickaxe and Ironworking"};
  }
  return {true, "Ready"};
}
bool Renewable(int good) {
  return good == 0 || good == 2 || good == 3;
}
Vec WarehousePosition(int city) {
  // Match the hall entrance used by direct-control interactions.
  return Settlements()[city].pos + Vec{55, 55};
}
bool InTown(const Save& save, int city, Vec position) {
  return Length(position - Settlements()[city].pos) <= CityRadius(save, city);
}
bool Safe(const World& world, const Save& save, Vec position) {
  for (const auto& [coord, chunk] : world.chunks) {
    for (const auto& enemy : chunk.enemies) {
      if (world.ActiveAt(enemy.pos) && !save.defeated.contains(enemy.id) &&
          (!enemy.kind ||
           save.relations.at(enemy.city).treaty == Treaty::kWar) &&
          Length(enemy.pos - position) < 240) {
        return false;
      }
    }
  }
  return true;
}
bool Land(const Save& save, Vec point) {
  const auto terrain = TerrainAt(point, save.seed);
  return terrain == Terrain::kForest || terrain == Terrain::kSand;
}
// Use the existing local planner, explicitly independent of the selected
// resident's vessel and underwater layer. Never route across unknown territory
// or outside this worker's town, even when the planner finds a shortcut.
bool Plan(World& world, const Save& save, Citizen& c, Vec destination) {
  auto route = world.Path(c.position, destination, save, true);
  if (Length(c.position - destination) > 16 &&
      (route.empty() || Length(route.back() - destination) > 35)) {
    return false;
  }
  for (auto point : route) {
    if (!InTown(save, c.city, point) || !Land(save, point) ||
        !ExploredAt(save.explored, point) || !Safe(world, save, point)) {
      return false;
    }
  }
  // Finish at the actual destination rather than a nearby tile centre.
  route.push_back(destination);
  c.order.destination = destination;
  c.order.route = std::move(route);
  return true;
}
bool Approach(World& world, const Save& save, Citizen& c, Vec target) {
  static const std::array<Vec, 8> kOffsets = {
      Vec{36, 0},  Vec{-36, 0},  Vec{0, 36},   Vec{0, -36},
      Vec{26, 26}, Vec{-26, 26}, Vec{26, -26}, Vec{-26, -26}};
  for (auto offset : kOffsets) {
    Vec point = target + offset;
    if (InTown(save, c.city, point) && Land(save, point) &&
        ExploredAt(save.explored, point) && Safe(world, save, point) &&
        world.Clear(point, save, 12, false, true) &&
        Plan(world, save, c, point)) {
      return true;
    }
  }
  return false;
}
void Wait(Citizen& c, const std::string& reason) {
  c.order.status = reason;
  c.order.route.clear();
  c.order.retry = 2;
}
bool Walk(World& world, const Save& save, Citizen& c, float dt) {
  if (Length(c.position - c.order.destination) < 8) {
    c.order.route.clear();
    return true;
  }
  if (c.order.route.empty() && !Plan(world, save, c, c.order.destination)) {
    Wait(c, "Blocked route; waiting");
    return false;
  }
  while (!c.order.route.empty() &&
         Length(c.order.route.front() - c.position) < 8) {
    c.order.route.erase(c.order.route.begin());
  }
  if (c.order.route.empty()) {
    return false;
  }
  Vec goal = c.order.route.front();
  if (!Safe(world, save, goal) || !InTown(save, c.city, goal) ||
      !Land(save, goal) || !ExploredAt(save.explored, goal)) {
    Wait(c, "Unsafe route; waiting");
    return false;
  }
  Vec old = c.position;
  float speed = 72 * PlayerCharacter(save).speed *
                (c.outfit.empty() ? 1 : Outfit(c.outfit).speed);
  c.position = world.Move(
      old, Normalized(goal - old) * std::min(Length(goal - old), dt * speed),
      save, 12, false, true);
  if (Length(c.position - old) < .01f) {
    Wait(c, "Blocked route; waiting");
  }
  return false;
}
bool Reserved(const Save& save, const Citizen& c, uint64_t target,
              bool planted) {
  return std::any_of(
      save.citizens.begin(), save.citizens.end(), [&](const auto& other) {
        return other.npc != c.npc && other.state == "active" &&
               other.order.kind == "gather" && other.order.target == target &&
               other.order.planted == planted && other.order.phase != "seek";
      });
}
const Resource* Target(const World& world, const Save& save, const Citizen& c) {
  if (c.order.planted) {
    for (const auto& plot : save.plots) {
      if (plot.resource.id == c.order.target && plot.ready &&
          plot.city == c.city &&
          world.chunks.contains(ChunkAt(plot.resource.pos))) {
        return &plot.resource;
      }
    }
  } else {
    for (const auto& [coord, chunk] : world.chunks) {
      for (const auto& resource : chunk.resources) {
        if (resource.id == c.order.target &&
            !save.harvested.contains(resource.id)) {
          return &resource;
        }
      }
    }
  }
  return nullptr;
}
bool FindResource(World& world, const Save& save, Citizen& c) {
  struct Candidate {
    const Resource* resource;
    bool planted;
  };
  std::vector<Candidate> candidates;
  auto add = [&](const Resource& r, bool planted) {
    if (ResourceGood(r.kind) == c.order.good && InTown(save, c.city, r.pos) &&
        ExploredAt(save.explored, r.pos) && Safe(world, save, r.pos) &&
        world.chunks.contains(ChunkAt(r.pos)) &&
        !Reserved(save, c, r.id, planted)) {
      candidates.push_back({&r, planted});
    }
  };
  for (const auto& [coord, chunk] : world.chunks) {
    for (const auto& resource : chunk.resources) {
      if (!save.harvested.contains(resource.id)) {
        add(resource, false);
      }
    }
  }
  for (const auto& plot : save.plots) {
    if (plot.city == c.city && plot.resource.id && plot.ready) {
      add(plot.resource, true);
    }
  }
  std::sort(candidates.begin(), candidates.end(), [&](auto a, auto b) {
    float da = Length(a.resource->pos - c.position);
    float db = Length(b.resource->pos - c.position);
    return da == db ? a.resource->id < b.resource->id : da < db;
  });
  // Bound expensive route attempts; inaccessible sites never yield resources.
  int attempts = 0;
  for (auto candidate : candidates) {
    if (++attempts > 8) {
      break;
    }
    if (Approach(world, save, c, candidate.resource->pos)) {
      c.order.target = candidate.resource->id;
      c.order.planted = candidate.planted;
      c.order.phase = "travel";
      c.order.progress = 0;
      return true;
    }
  }
  return false;
}
bool PlantSite(const World& world, const Save& save, int city, Vec point) {
  if (!InTown(save, city, point) || !Land(save, point) ||
      !ExploredAt(save.explored, point) || RoadAt(save, point, 36) ||
      PathAt(point) || !world.Clear(point, save, 30, false, true)) {
    return false;
  }
  for (const auto& p : save.plots) {
    if (p.resource.id && Length(p.resource.pos - point) < 72) {
      return false;
    }
  }
  return true;
}
bool FindPlantSite(World& world, const Save& save, Citizen& c) {
  Vec centre = Settlements()[c.city].pos;
  // A stable lattice keeps placement deterministic, without a random search.
  for (int ring = 2; ring <= 14; ++ring) {
    for (int y = -ring; y <= ring; ++y) {
      for (int x = -ring; x <= ring; ++x) {
        if (std::abs(x) != ring && std::abs(y) != ring) {
          continue;
        }
        Vec point = centre + Vec{x * 72.f, y * 72.f};
        if (PlantSite(world, save, c.city, point) && Safe(world, save, point) &&
            Plan(world, save, c, point)) {
          // A planting target is a location, not a generated world resource.
          c.order.target = 0;
          c.order.phase = "travel";
          c.order.progress = 0;
          return true;
        }
      }
    }
  }
  return false;
}
void DeliverCargo(World& world, Save& save, Citizen& c, float dt) {
  auto& order = c.order;
  if (order.phase != "deliver") {
    order.route.clear();
    order.phase = "deliver";
    if (!Approach(world, save, c, WarehousePosition(c.city))) {
      Wait(c, "Warehouse route blocked");
      return;
    }
  }
  if (Length(order.destination - WarehousePosition(c.city)) > kWorkReach &&
      !Approach(world, save, c, WarehousePosition(c.city))) {
    Wait(c, "Warehouse route blocked");
    return;
  }
  order.status = "Carrying goods to warehouse";
  if (!Walk(world, save, c, dt)) {
    return;
  }
  int& amount = GoodCount(c.cargo, order.good);
  int moved = std::min(amount, 1000000 - save.stocks[c.city][order.good]);
  save.stocks[c.city][order.good] += moved;
  amount -= moved;
  if (amount > 0) {
    Wait(c, "Warehouse full; cargo retained");
  } else {
    order.phase = "seek";
    order.target = 0;
    order.progress = 0;
  }
}
void GatherResource(World& world, Save& save, Citizen& c, float dt) {
  auto& order = c.order;
  const auto* target = order.phase == "seek" ? nullptr : Target(world, save, c);
  if (!target || ResourceGood(target->kind) != order.good ||
      !InTown(save, c.city, target->pos)) {
    order.phase = "seek";
    order.target = 0;
    order.route.clear();
    if (!FindResource(world, save, c)) {
      if (GoodCount(c.cargo, order.good) > 0) {
        order.phase = "deliver";
        if (!Approach(world, save, c, WarehousePosition(c.city))) {
          Wait(c, "Warehouse route blocked");
        }
      } else {
        Wait(c, "No reachable explored resources; waiting");
      }
    }
    return;
  }
  if (!Safe(world, save, target->pos)) {
    Wait(c, "Danger near work site; paused");
    return;
  }
  order.status = "Walking to resource";
  if (order.phase == "travel" && Walk(world, save, c, dt)) {
    order.phase = "work";
  }
  if (order.phase == "work") {
    if (Length(c.position - target->pos) > kWorkReach) {
      order.phase = "seek";
      return;
    }
    if (world.SightBlocked(c.position, target->pos, save, true)) {
      order.phase = "seek";
      Wait(c, "Work site blocked; waiting");
      return;
    }
    order.status = "Gathering";
    order.progress += dt;
    float duration = HarvestDuration(*target, c.cargo) *
                     PlayerCharacter(save).gather *
                     (c.outfit.empty() ? 1 : Outfit(c.outfit).gather);
    if (order.progress >= duration) {
      auto result = HarvestInto(save, c.cargo, false, *target);
      order.phase = "seek";
      order.target = 0;
      order.progress = 0;
      if (!result.ok) {
        Wait(c, result.message);
      }
    }
  }
}
void PlantResources(World& world, Save& save, Citizen& c, float dt) {
  auto& order = c.order;
  auto quote = CheckPlantResource(save, c.city, order.good);
  if (!quote.ok) {
    Wait(c, quote.message);
    return;
  }
  if (c.cargo.wood < 2 || c.cargo.berries < 2) {
    if (order.phase != "supply") {
      order.phase = "supply";
      order.route.clear();
      if (!Approach(world, save, c, WarehousePosition(c.city))) {
        Wait(c, "Supply route blocked");
        return;
      }
    }
    if (Length(order.destination - WarehousePosition(c.city)) > kWorkReach &&
        !Approach(world, save, c, WarehousePosition(c.city))) {
      Wait(c, "Supply route blocked");
      return;
    }
    order.status = "Fetching planting supplies";
    if (!Walk(world, save, c, dt)) {
      return;
    }
    const int wood = std::max(0, 2 - c.cargo.wood);
    const int berries = std::max(0, 2 - c.cargo.berries);
    if (save.stocks[c.city][0] < wood || save.stocks[c.city][2] < berries) {
      Wait(c, "Warehouse needs planting wood and berries");
      return;
    }
    save.stocks[c.city][0] -= wood;
    save.stocks[c.city][2] -= berries;
    c.cargo.wood += wood;
    c.cargo.berries += berries;
    order.phase = "seek";
    order.route.clear();
    order.progress = 0;
    return;
  }
  if (order.phase == "supply") {
    order.phase = "seek";
    order.route.clear();
  }
  if (order.phase == "seek") {
    if (!FindPlantSite(world, save, c)) {
      Wait(c, "No clear explored planting site; waiting");
    }
    return;
  }
  order.status = "Walking to planting site";
  if (order.phase == "travel" && Walk(world, save, c, dt)) {
    order.phase = "work";
  }
  if (order.phase == "work") {
    if (Length(c.position - order.destination) > 8) {
      order.phase = "travel";
      order.progress = 0;
      order.route.clear();
      return;
    }
    order.status = "Planting";
    order.progress += dt;
    if (order.progress >= 3) {
      // Plant exactly where the resident stood; revalidate and charge once.
      auto result = PlantResourceAt(world, save, c.city, order.good,
                                    order.destination, c.cargo);
      order.phase = "seek";
      order.progress = 0;
      order.route.clear();
      if (!result.ok) {
        Wait(c, result.message);
      }
    }
  }
}
}  // namespace

const std::vector<WorkOrderDef>& WorkOrders() {
  static const std::vector<WorkOrderDef> kOrders = {
      {"Gather wood", "gather", 0},    {"Gather stone", "gather", 1},
      {"Gather berries", "gather", 2}, {"Gather reeds", "gather", 3},
      {"Plant trees", "plant", 0},     {"Plant berries", "plant", 2},
      {"Plant reeds", "plant", 3},     {"Fish", "gather", 4},
      {"Mine iron ore", "gather", 6}};
  return kOrders;
}
Action OrderResident(Save& save, int npc, const std::string& kind, int good) {
  StoreControlled(save);
  auto* c = GetCitizen(save, npc);
  if (!save.nation || !c || c->state != "active" || !OwnsCity(save, c->city) ||
      c->underwater || !InTown(save, c->city, c->position)) {
    return {false, "Choose a free surface resident on home-town territory"};
  }
  const auto& orders = WorkOrders();
  if (std::none_of(orders.begin(), orders.end(), [&](const auto& order) {
        return order.kind == kind && order.good == good;
      })) {
    return {false, "Choose a supported gathering or planting order"};
  }
  auto requirements = WorkRequirements(save, *c, kind == "plant" ? 0 : good);
  if (!requirements.ok) {
    return requirements;
  }
  c->order = {};
  c->order.kind = kind;
  c->order.good = good;
  c->order.status = npc == save.controlled_npc
                        ? "Manual control; switch resident to start"
                        : "Looking for a work site";
  RecordHistory(save, "resident_order", kind + " " + GoodName(good), c->city);
  return {true, c->order.status};
}
Action CancelResidentOrder(Save& save, int npc) {
  auto* c = GetCitizen(save, npc);
  if (!save.nation || !c || !OwnsCity(save, c->city)) {
    return {false, "Resident is unavailable"};
  }
  c->order = {};
  return {true, "Order cancelled; carried goods stay with the resident"};
}
std::string ResidentOrderLabel(const Citizen& c) {
  return c.order.kind == "idle"
             ? "Idle"
             : (c.order.kind == "gather" ? "Gather " : "Plant ") +
                   std::string(GoodName(c.order.good)) + " · " + c.order.status;
}
Action PlantResourceAt(World& world, Save& save, int city, int good,
                       Vec position, Inventory& supplies) {
  if (!save.nation || !OwnsCity(save, city) || !HasSkill(save, "gathering") ||
      !Renewable(good) || !PlantSite(world, save, city, position) ||
      !Safe(world, save, position)) {
    return {false,
            "Planting needs a safe, clear, explored land site in your town"};
  }
  auto quote = CheckPlantResource(save, city, good);
  if (!quote.ok) {
    return quote;
  }
  if (supplies.wood < 2 || supplies.berries < 2) {
    return {false, "Carry 2 wood and 2 berries to the planting site"};
  }
  GrowingPlot plot{city, good, save.game_time + (good == 0 ? 600 : 180), false};
  plot.resource = {save.next_entity, position,
                   good == 0   ? ResourceKind::kTree
                   : good == 2 ? ResourceKind::kBerry
                               : ResourceKind::kReed,
                   0};
  save.plots.push_back(plot);
  ++save.next_entity;
  supplies.wood -= 2;
  supplies.berries -= 2;
  RecordHistory(save, "planting",
                "Resident planted " + std::string(GoodName(good)), city);
  return {true, "Planted from carried supplies"};
}
void TickResidentOrder(World& world, Save& save, Citizen& c, float dt) {
  if (!save.nation || !std::isfinite(dt) || dt <= 0) {
    return;
  }
  dt = std::min(dt, .25f);
  auto& order = c.order;
  if (order.kind == "idle" || c.npc == save.controlled_npc) {
    return;
  }
  if (c.state != "active" || !OwnsCity(save, c.city)) {
    order = {};
    return;
  }
  if (c.underwater || !InTown(save, c.city, c.position) ||
      !world.chunks.contains(ChunkAt(c.position))) {
    order.status = "Waiting for local surface terrain";
    order.route.clear();
    return;
  }
  if (world.talking_npc == c.npc) {
    Wait(c, "In conversation; work paused");
    return;
  }
  if (!Safe(world, save, c.position) ||
      save.relations[c.city].treaty == Treaty::kWar) {
    Wait(c, "Danger nearby; work paused");
    return;
  }
  order.retry = std::max(0.f, order.retry - dt);
  if (order.retry > 0) {
    return;
  }
  auto requirements =
      WorkRequirements(save, c, order.kind == "plant" ? 0 : order.good);
  if (!requirements.ok) {
    Wait(c, requirements.message);
    return;
  }
  if (order.kind == "gather" &&
      (order.phase == "deliver" ||
       GoodCount(c.cargo, order.good) >= kCarryBatch)) {
    DeliverCargo(world, save, c, dt);
  } else if (order.kind == "gather") {
    GatherResource(world, save, c, dt);
  } else {
    PlantResources(world, save, c, dt);
  }
}
void TickResidentOrders(World& world, Save& save, float dt) {
  if (!save.nation || !std::isfinite(dt) || dt <= 0) {
    return;
  }
  dt = std::min(dt, .25f);  // Work never catches up while a world is closed.
  std::vector<Citizen*> residents;
  for (auto& c : save.citizens) {
    residents.push_back(&c);
  }
  std::sort(residents.begin(), residents.end(),
            [](auto a, auto b) { return a->npc < b->npc; });
  for (auto* resident : residents) {
    TickResidentOrder(world, save, *resident, dt);
  }
}
}  // namespace frontier
