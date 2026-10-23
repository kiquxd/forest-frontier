#include "freight.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <utility>
#include <vector>

#include "geometry.hpp"
#include "nation.hpp"
#include "progress.hpp"
#include "settlement_system.hpp"

namespace frontier {
namespace {
constexpr float kCartRadius = 12;
constexpr float kCartSpeed = 95;
constexpr float kRoadSample = 24;

bool OnRoad(const Road& road, Vec position, float width = 2) {
  for (size_t i = 1; i < road.points.size(); ++i) {
    if (SegmentDistance(position, road.points[i - 1], road.points[i]) <=
        width) {
      return true;
    }
  }
  return false;
}
bool InConnector(const Save& save, int city, Vec position) {
  return FreightCityAccess(save, city) &&
         Length(position - Settlements()[city].pos) <=
             CityRadius(save, city) + 50;
}
bool LocalAccess(const Save& save, Vec point) {
  int city = CityAt(save, point);
  return (city < 0 || FreightCityAccess(save, city)) &&
         ExploredAt(save.explored, point);
}
bool LoadedAt(const World& world, Vec point) {
  for (float x : {-kCartRadius, kCartRadius}) {
    for (float y : {-kCartRadius, kCartRadius}) {
      if (!world.chunks.contains(ChunkAt(point + Vec{x, y}))) {
        return false;
      }
    }
  }
  return true;
}
bool Danger(const World& world, const Save& save, Vec position) {
  for (const auto& [coord, chunk] : world.chunks) {
    for (const auto& enemy : chunk.enemies) {
      if (world.ActiveAt(enemy.pos) && !save.defeated.contains(enemy.id) &&
          (!enemy.kind ||
           save.relations.at(enemy.city).treaty == Treaty::kWar) &&
          Length(enemy.pos - position) < 200) {
        return true;
      }
    }
  }
  return false;
}
std::vector<Vec> RoadSamples(const Road& road, bool reverse) {
  std::vector<Vec> samples;
  if (road.points.size() < 2) {
    return samples;
  }
  samples.push_back(road.points.front());
  for (size_t i = 1; i < road.points.size(); ++i) {
    Vec start = road.points[i - 1];
    Vec delta = road.points[i] - start;
    int steps =
        std::max(1, static_cast<int>(std::ceil(Length(delta) / kRoadSample)));
    for (int step = 1; step <= steps; ++step) {
      samples.push_back(start + delta * (static_cast<float>(step) / steps));
    }
  }
  if (reverse) {
    std::reverse(samples.begin(), samples.end());
  }
  return samples;
}
bool RoadAccess(const Save& save, const Road& road) {
  if (!FreightCityAccess(save, road.from) ||
      !FreightCityAccess(save, road.to)) {
    return false;
  }
  // A plotted road can also cross a third town between its graph endpoints.
  for (auto point : RoadSamples(road, false)) {
    if (!LocalAccess(save, point)) {
      return false;
    }
  }
  return true;
}
void Pause(FreightCart& cart, const std::string& status) {
  cart.status = status;
  cart.local_route.clear();
  cart.retry = 1;
}
bool PlanConnector(World& world, const Save& save, FreightCart& cart,
                   const FreightWaypoint& target) {
  auto route = world.Path(cart.position, target.position, save, true);
  if (route.empty() && Length(cart.position - target.position) > kTileSize) {
    return false;
  }
  for (auto point : route) {
    if (!InConnector(save, target.city, point) || !LocalAccess(save, point)) {
      return false;
    }
  }
  // The bounded local planner may return a partial route for a long connector.
  if (Length(cart.position - target.position) <= kTileSize ||
      (!route.empty() && Length(route.back() - target.position) <= 35)) {
    route.push_back(target.position);
  }
  cart.local_route = std::move(route);
  return !cart.local_route.empty();
}
bool MoveCart(World& world, const Save& save, FreightCart& cart,
              const FreightWaypoint& target, float dt) {
  if (Length(cart.position - target.position) < .1f) {
    return true;
  }
  Vec goal = target.position;
  if (target.city >= 0) {
    if (cart.local_route.empty() && !PlanConnector(world, save, cart, target)) {
      Pause(cart, "Town connector blocked; cargo retained");
      return false;
    }
    while (!cart.local_route.empty() &&
           Length(cart.local_route.front() - cart.position) < .1f) {
      cart.local_route.erase(cart.local_route.begin());
    }
    if (cart.local_route.empty()) {
      return Length(cart.position - target.position) < .1f;
    }
    goal = cart.local_route.front();
  }
  Vec delta = goal - cart.position;
  float distance = std::min(Length(delta), kCartSpeed * dt);
  Vec direction = Normalized(delta);
  int steps = std::max(1, static_cast<int>(std::ceil(distance / 3)));
  Vec point = cart.position;
  for (int i = 0; i < steps; ++i) {
    Vec next = point + direction * (distance / steps);
    if (!LoadedAt(world, next)) {
      Pause(cart, "Waiting for loaded road terrain");
      return false;
    }
    FreightWaypoint step{next, target.road, target.city};
    if (!FreightSegmentValid(save, point, step) || !LocalAccess(save, next)) {
      Pause(cart, "Route access suspended; cargo retained");
      return false;
    }
    if (Danger(world, save, next)) {
      Pause(cart, "Danger ahead; freight paused");
      return false;
    }
    auto terrain = TerrainAt(next, save.seed);
    const bool water =
        terrain == Terrain::kShallow || terrain == Terrain::kDeep;
    if ((water && !BridgeAt(save, next, kCartRadius)) ||
        !world.Clear(next, save, kCartRadius, false, true)) {
      Pause(cart, "Road obstructed; cargo retained");
      return false;
    }
    point = next;
  }
  cart.facing = Length(delta) > .001f ? direction : cart.facing;
  cart.position = point;
  return Length(cart.position - target.position) < .1f;
}
void UnloadCart(Save& save, FreightCart& cart) {
  int moved =
      std::min(cart.quantity, 1000000 - save.stocks[cart.to][cart.good]);
  save.stocks[cart.to][cart.good] += moved;
  cart.quantity -= moved;
  save.known_goods[cart.good] = true;
  if (cart.quantity) {
    Pause(cart, "Destination warehouse full; cargo retained");
    return;
  }
  cart.city = cart.to;
  cart.route.clear();
  cart.local_route.clear();
  cart.waypoint = 0;
  cart.returning = false;
  cart.status = "Ready";
  cart.next_update = -1;
  RecordHistory(save, "freight", "Freight cart unloaded at warehouse",
                cart.city);
}
}  // namespace

Vec FreightDepot(int city) {
  return Settlements()[city].pos + Vec{55, 55};
}
bool FreightCityAccess(const Save& save, int city) {
  return city >= 0 && city < static_cast<int>(save.stocks.size()) &&
         save.relations[city].treaty != Treaty::kWar &&
         (OwnsCity(save, city) || save.relations[city].opinion >= 0);
}
bool FreightSegmentValid(const Save& save, Vec start,
                         const FreightWaypoint& end, bool geometry_only) {
  if (end.city >= 0) {
    return end.road == -1 && end.city < static_cast<int>(save.stocks.size()) &&
           (geometry_only || FreightCityAccess(save, end.city)) &&
           Length(start - Settlements()[end.city].pos) <=
               CityRadius(save, end.city) + 50 &&
           Length(end.position - Settlements()[end.city].pos) <=
               CityRadius(save, end.city) + 50;
  }
  if (end.road < 0 || end.road >= static_cast<int>(save.roads.size()) ||
      Length(end.position - start) > kRoadSample + 8) {
    return false;
  }
  const auto& road = save.roads[end.road];
  if (!geometry_only && (!FreightCityAccess(save, road.from) ||
                         !FreightCityAccess(save, road.to))) {
    return false;
  }
  int steps = std::max(
      1, static_cast<int>(std::ceil(Length(end.position - start) / 6)));
  for (int i = 0; i <= steps; ++i) {
    Vec point =
        start + (end.position - start) * (static_cast<float>(i) / steps);
    if (!OnRoad(road, point) || (!geometry_only && !LocalAccess(save, point))) {
      return false;
    }
  }
  return true;
}
std::vector<FreightWaypoint> PlanRoadFreight(const Save& save, int from,
                                             int to) {
  if (!OwnsCity(save, from) || !OwnsCity(save, to) || from == to ||
      !FreightCityAccess(save, from) || !FreightCityAccess(save, to)) {
    return {};
  }
  const int count = static_cast<int>(save.stocks.size());
  std::vector<int> parent(count, -1), via(count, -1);
  std::queue<int> queue;
  parent[from] = from;
  queue.push(from);
  while (!queue.empty() && parent[to] < 0) {
    int city = queue.front();
    queue.pop();
    for (int i = 0; i < static_cast<int>(save.roads.size()); ++i) {
      const auto& road = save.roads[i];
      int next = road.from == city ? road.to : road.to == city ? road.from : -1;
      if (next >= 0 && parent[next] < 0 && RoadAccess(save, road)) {
        parent[next] = city;
        via[next] = i;
        queue.push(next);
      }
    }
  }
  if (parent[to] < 0) {
    return {};
  }
  std::vector<int> cities;
  for (int city = to; city != from; city = parent[city]) {
    cities.push_back(city);
  }
  std::reverse(cities.begin(), cities.end());
  std::vector<FreightWaypoint> route{{FreightDepot(from), -1, from}};
  for (int city : cities) {
    int previous = parent[city];
    int index = via[city];
    auto points =
        RoadSamples(save.roads[index], save.roads[index].from != previous);
    size_t begin = 0, end = points.size();
    while (begin < end && Length(points[begin] - Settlements()[previous].pos) <=
                              CityRadius(save, previous)) {
      ++begin;
    }
    while (end > begin && Length(points[end - 1] - Settlements()[city].pos) <=
                              CityRadius(save, city)) {
      --end;
    }
    if (begin >= end) {
      return {};
    }
    // Town-centre road endpoints overlap halls. Connect to the first road
    // point outside town using local collision-aware routing instead.
    route.push_back({points[begin], -1, previous});
    for (size_t i = begin + 1; i < end; ++i) {
      route.push_back({points[i], index, -1});
    }
    if (route.size() > kMaxFreightWaypoints - 1) {
      return {};
    }
  }
  route.push_back({FreightDepot(to), -1, to});
  return route;
}
Action BuildFreightCart(Save& save, int city) {
  if (!save.nation || !OwnsCity(save, city) ||
      !HasSkill(save, "construction") ||
      !MissingSkillPrerequisites(save, "construction").empty()) {
    return {false,
            "Build a cart in your town after learning the full Construction "
            "branch"};
  }
  if (save.freight_carts.size() >= 64 || save.stocks[city][0] < 20 ||
      save.stocks[city][1] < 5 || save.stocks[city][5] < 4) {
    return {false,
            "Cart costs city wood 20, stone 5, leather 4; maximum 64 carts"};
  }
  FreightCart cart;
  cart.id = save.next_entity;
  cart.city = cart.from = cart.to = city;
  cart.position = cart.previous = FreightDepot(city);
  save.freight_carts.push_back(cart);
  ++save.next_entity;
  save.stocks[city][0] -= 20;
  save.stocks[city][1] -= 5;
  save.stocks[city][5] -= 4;
  RecordHistory(save, "freight", "Built a road freight cart", city);
  return {true, "Freight cart ready; capacity 40 goods"};
}
Action DispatchFreight(Save& save, int from, int to, int good, int quantity) {
  if (good < 0 || good >= kGoodTypes || quantity < 1 ||
      quantity > kFreightCapacity) {
    return {false, "Choose 1–40 goods for one cart"};
  }
  auto route = PlanRoadFreight(save, from, to);
  if (route.empty()) {
    return {false,
            "Freight needs owned endpoints and an explored peaceful built-road "
            "route"};
  }
  auto cart = std::find_if(
      save.freight_carts.begin(), save.freight_carts.end(), [&](const auto& c) {
        return c.city == from && c.route.empty() && c.quantity == 0;
      });
  if (cart == save.freight_carts.end() || save.stocks[from][good] < quantity) {
    return {false, "Source town needs a ready cart and sufficient goods"};
  }
  route.front().position = cart->position;
  cart->route = std::move(route);
  cart->from = from;
  cart->to = to;
  cart->good = good;
  cart->quantity = quantity;
  cart->returning = false;
  cart->waypoint = 1;
  cart->local_route.clear();
  cart->retry = 0;
  cart->next_update = -1;
  cart->status = "Loaded; departing";
  save.stocks[from][good] -= quantity;
  return {true,
          "Loaded a freight cart; cargo arrives only after physical delivery"};
}
Action ReturnFreight(Save& save, uint64_t id) {
  auto cart = std::find_if(save.freight_carts.begin(), save.freight_carts.end(),
                           [&](const auto& c) { return c.id == id; });
  if (cart == save.freight_carts.end() || cart->route.empty() ||
      cart->returning || !OwnsCity(save, cart->from)) {
    return {false, "Choose an outbound cart with an owned source town"};
  }
  std::vector<FreightWaypoint> route{{cart->position, -1, -1}};
  // Each reversed segment inherits the original segment's access mode.
  size_t next = std::min(cart->waypoint, cart->route.size() - 1);
  for (size_t i = next; i > 0; --i) {
    auto point = cart->route[i - 1];
    point.road = cart->route[i].road;
    point.city = cart->route[i].city;
    route.push_back(point);
  }
  cart->route = std::move(route);
  cart->to = cart->from;
  cart->waypoint = 1;
  cart->returning = true;
  cart->local_route.clear();
  cart->retry = 0;
  cart->next_update = -1;
  cart->status = "Returning with remaining cargo";
  return {true, "Cart returning along its travelled route; no remote refund"};
}
void TickFreightCart(World& world, Save& save, FreightCart& cart, float dt) {
  if (!save.nation || !std::isfinite(dt) || dt <= 0) {
    return;
  }
  dt = std::min(dt, .25f);
  cart.previous = cart.position;
  if (cart.route.empty()) {
    return;
  }
  cart.retry = std::max(0.f, cart.retry - dt);
  if (cart.retry > 0) {
    return;
  }
  if (!OwnsCity(save, cart.to) || !FreightCityAccess(save, cart.from) ||
      !FreightCityAccess(save, cart.to)) {
    Pause(cart, "Endpoint access suspended; cargo retained");
    return;
  }
  bool access = true;
  for (size_t i = 1; i < cart.route.size(); ++i) {
    const auto& point = cart.route[i];
    if (point.city >= 0
            ? !FreightCityAccess(save, point.city)
            : point.road < 0 ||
                  point.road >= static_cast<int>(save.roads.size()) ||
                  !FreightCityAccess(save, save.roads[point.road].from) ||
                  !FreightCityAccess(save, save.roads[point.road].to)) {
      access = false;
      break;
    }
  }
  if (!access) {
    Pause(cart, "Hostile or missing transit route; cargo retained");
    return;
  }
  if (!world.chunks.contains(ChunkAt(cart.position))) {
    Pause(cart, "Waiting for loaded road terrain");
    return;
  }
  if (Danger(world, save, cart.position)) {
    Pause(cart, "Danger nearby; freight paused");
    return;
  }
  if (cart.waypoint >= cart.route.size()) {
    UnloadCart(save, cart);
    return;
  }
  cart.status =
      cart.returning ? "Returning on built roads" : "Travelling on built roads";
  if (MoveCart(world, save, cart, cart.route[cart.waypoint], dt)) {
    ++cart.waypoint;
    cart.local_route.clear();
  }
}
void TickFreight(World& world, Save& save, float dt) {
  for (auto& cart : save.freight_carts) {
    TickFreightCart(world, save, cart, dt);
  }
}
}  // namespace frontier
