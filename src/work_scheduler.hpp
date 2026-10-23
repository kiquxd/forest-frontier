#ifndef FOREST_FRONTIER_SRC_WORK_SCHEDULER_HPP_
#define FOREST_FRONTIER_SRC_WORK_SCHEDULER_HPP_

#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <queue>
#include <vector>

#include "world.hpp"

namespace frontier {
// Main-thread event queue. Generation remains on World's existing CPU worker;
// all gameplay state changes stay sequential on the owner/render thread.
class WorkScheduler {
 public:
  static constexpr size_t kMaxEventsPerStep = 256;
  struct Stats {
    size_t events{};
    size_t overdue{};
    size_t terrain_chunks{};
  };
  void Tick(World& world, Save& save, float dt,
            size_t budget = kMaxEventsPerStep);
  const Stats& LastStep() const {
    return stats_;
  }

 private:
  struct Key {
    int kind{};
    uint64_t id{};
    auto operator<=>(const Key&) const = default;
  };
  struct State {
    Citizen* resident{};
    FreightCart* cart{};
    double* due{};
    float* retry{};
    double queued{-1};
    uint64_t ticket{};
    bool seen{};
  };
  struct Event {
    double due;
    Key key;
    uint64_t ticket;
    bool operator<(const Event& other) const;
  };
  void Queue(const Key& key, State& state, double due);
  void Reconcile(Save& save);
  void RequestTerrain(World& world, const Save& save);
  bool TerrainReady(const World& world, const Save& save,
                    const State& state) const;
  std::map<Key, State> states_;
  std::priority_queue<Event> events_;
  uint64_t next_ticket_{};
  Stats stats_;
};
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_WORK_SCHEDULER_HPP_
