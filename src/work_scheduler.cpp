#include "work_scheduler.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "freight.hpp"
#include "gameplay.hpp"
#include "resident_orders.hpp"
#include "settlement_system.hpp"

namespace frontier {
namespace {
constexpr double kWorkStep = .1;
using ChunkSet = std::unordered_set<Coord, CoordHash>;

std::vector<Coord> CityChunks(const Save& save, int city) {
  float extent = CityRadius(save, city) + 96;
  Vec centre = Settlements()[city].pos;
  Coord low = ChunkAt(centre - Vec{extent, extent});
  Coord high = ChunkAt(centre + Vec{extent, extent});
  std::vector<Coord> out;
  for (int y = low.y; y <= high.y; ++y) {
    for (int x = low.x; x <= high.x; ++x) {
      out.push_back({x, y});
    }
  }
  return out;
}
int ConnectorCity(const FreightCart& cart) {
  if (cart.route.empty()) {
    return cart.city;
  }
  return cart.route[std::min(cart.waypoint, cart.route.size() - 1)].city;
}
std::vector<Coord> CartChunks(const FreightCart& cart) {
  ChunkSet needed;
  auto add = [&](Vec position) {
    for (float x : {-32.f, 32.f}) {
      for (float y : {-32.f, 32.f}) {
        needed.insert(ChunkAt(position + Vec{x, y}));
      }
    }
  };
  add(cart.position);
  if (cart.waypoint < cart.route.size()) {
    add(cart.route[cart.waypoint].position);
  }
  std::vector<Coord> out(needed.begin(), needed.end());
  std::sort(out.begin(), out.end(), [](Coord a, Coord b) {
    return a.x != b.x ? a.x < b.x : a.y < b.y;
  });
  return out;
}
bool Ready(const World& world, const std::vector<Coord>& needed) {
  return std::all_of(needed.begin(), needed.end(),
                     [&](Coord coord) { return world.chunks.contains(coord); });
}
}  // namespace

bool WorkScheduler::Event::operator<(const Event& other) const {
  if (due != other.due) {
    return due > other.due;
  }
  return key != other.key ? key > other.key : ticket > other.ticket;
}
void WorkScheduler::Queue(const Key& key, State& state, double due) {
  state.queued = *state.due = due;
  state.ticket = ++next_ticket_;
  events_.push({due, key, state.ticket});
}
void WorkScheduler::Reconcile(Save& save) {
  for (auto& [key, state] : states_) {
    state.seen = false;
  }
  auto add = [&](Key key, Citizen* resident, FreightCart* cart, double& due,
                 float& retry) {
    auto& state = states_[key];
    state.resident = resident;
    state.cart = cart;
    state.due = &due;
    state.retry = &retry;
    state.seen = true;
    if (due < 0 || state.queued != due) {
      Queue(key, state, due < 0 ? save.game_time : due);
    }
  };
  for (auto& resident : save.citizens) {
    if (resident.order.kind == "idle") {
      continue;
    }
    if (resident.state != "active" || !OwnsCity(save, resident.city)) {
      resident.order = {};
      continue;
    }
    if (resident.npc == save.controlled_npc) {
      resident.order.next_update = -1;
      continue;
    }
    add({0, static_cast<uint64_t>(resident.npc)}, &resident, nullptr,
        resident.order.next_update, resident.order.retry);
  }
  for (auto& cart : save.freight_carts) {
    if (cart.route.empty()) {
      cart.next_update = -1;
      continue;
    }
    add({1, cart.id}, nullptr, &cart, cart.next_update, cart.retry);
  }
  std::erase_if(states_, [](const auto& row) { return !row.second.seen; });
  // Bound stale events after cancellations/reassignments during UI activity.
  if (events_.size() > states_.size() * 4 + 32) {
    events_ = {};
    for (const auto& [key, state] : states_) {
      events_.push({state.queued, key, state.ticket});
    }
  }
}
void WorkScheduler::RequestTerrain(World& world, const Save& save) {
  std::map<Key, std::vector<Coord>> groups;
  auto add_city = [&](int city) {
    Key key{0, static_cast<uint64_t>(city)};
    if (!groups.contains(key)) {
      groups.emplace(key, CityChunks(save, city));
    }
  };
  for (const auto& [key, state] : states_) {
    if (state.resident) {
      add_city(state.resident->city);
    } else {
      int city = ConnectorCity(*state.cart);
      if (city >= 0) {
        add_city(city);
      } else {
        groups.emplace(Key{1, key.id}, CartChunks(*state.cart));
      }
    }
  }
  std::vector<const std::vector<Coord>*> requests;
  for (const auto& [key, needed] : groups) {
    requests.push_back(&needed);
  }
  ChunkSet admitted;
  // Whole groups prevent partially loaded towns changing target selection.
  // Rotate once per simulation second if total demand exceeds cache capacity.
  size_t offset =
      requests.empty()
          ? 0
          : static_cast<size_t>(std::floor(save.game_time)) % requests.size();
  for (size_t i = 0; i < requests.size(); ++i) {
    const auto& group = *requests[(i + offset) % requests.size()];
    size_t extra = std::count_if(group.begin(), group.end(), [&](Coord coord) {
      return !admitted.contains(coord);
    });
    if (admitted.size() + extra <= World::kMaxWorkChunks) {
      admitted.insert(group.begin(), group.end());
    }
  }
  stats_.terrain_chunks = admitted.size();
  world.RequestWork(admitted);
}
bool WorkScheduler::TerrainReady(const World& world, const Save& save,
                                 const State& state) const {
  if (state.resident) {
    return Ready(world, CityChunks(save, state.resident->city));
  }
  int city = ConnectorCity(*state.cart);
  return city >= 0 ? Ready(world, CityChunks(save, city))
                   : Ready(world, CartChunks(*state.cart));
}
void WorkScheduler::Tick(World& world, Save& save, float dt, size_t budget) {
  if (!std::isfinite(dt) || dt <= 0) {
    return;
  }
  stats_ = {};
  if (!save.nation) {
    states_.clear();
    events_ = {};
    world.RequestWork({});
    return;
  }
  Reconcile(save);
  RequestTerrain(world, save);
  while (!events_.empty() && events_.top().due <= save.game_time + 1e-8 &&
         stats_.events < budget) {
    const auto event = events_.top();
    events_.pop();
    auto it = states_.find(event.key);
    if (it == states_.end() || it->second.ticket != event.ticket) {
      continue;
    }
    auto& state = it->second;
    ++stats_.events;
    if (!TerrainReady(world, save, state)) {
      if (state.resident) {
        state.resident->order.status = "Preparing work site";
        state.resident->order.route.clear();
      } else {
        state.cart->status = "Preparing next road section";
        state.cart->local_route.clear();
      }
      // Terrain preparation is real waiting, not an accrual of free work.
      Queue(event.key, state, save.game_time + kWorkStep);
      continue;
    }
    *state.retry = 0;
    if (state.resident) {
      TickResidentOrder(world, save, *state.resident, kWorkStep);
    } else {
      TickFreightCart(world, save, *state.cart, kWorkStep);
    }
    bool active = state.resident ? state.resident->order.kind != "idle"
                                 : !state.cart->route.empty();
    if (!active) {
      *state.due = -1;
      states_.erase(it);
      continue;
    }
    double delay = std::max(kWorkStep, static_cast<double>(*state.retry));
    Queue(event.key, state, event.due + delay);
  }
  for (const auto& [key, state] : states_) {
    stats_.overdue += state.queued <= save.game_time + 1e-8;
  }
}
}  // namespace frontier
