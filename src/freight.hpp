#ifndef FOREST_FRONTIER_SRC_FREIGHT_HPP_
#define FOREST_FRONTIER_SRC_FREIGHT_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "gameplay.hpp"

namespace frontier {
inline constexpr int kFreightCapacity = 40;
inline constexpr size_t kMaxFreightWaypoints = 4096;
Vec FreightDepot(int city);
bool FreightCityAccess(const Save& save, int city);
std::vector<FreightWaypoint> PlanRoadFreight(const Save& save, int from,
                                             int to);
bool FreightSegmentValid(const Save& save, Vec start,
                         const FreightWaypoint& end,
                         bool geometry_only = false);
Action BuildFreightCart(Save& save, int city);
Action DispatchFreight(Save& save, int from, int to, int good, int quantity);
Action ReturnFreight(Save& save, uint64_t id);
void TickFreightCart(World& world, Save& save, FreightCart& cart, float dt);
void TickFreight(World& world, Save& save, float dt);
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_FREIGHT_HPP_
