#ifndef FOREST_FRONTIER_SRC_RESIDENT_ORDERS_HPP_
#define FOREST_FRONTIER_SRC_RESIDENT_ORDERS_HPP_

#include <string>
#include <vector>

#include "gameplay.hpp"

namespace frontier {
struct WorkOrderDef {
  std::string name;
  std::string kind;
  int good;
};
const std::vector<WorkOrderDef>& WorkOrders();
Action OrderResident(Save& save, int npc, const std::string& kind, int good);
Action CancelResidentOrder(Save& save, int npc);
void TickResidentOrder(World& world, Save& save, Citizen& resident, float dt);
void TickResidentOrders(World& world, Save& save, float dt);
std::string ResidentOrderLabel(const Citizen& resident);
Action PlantResourceAt(World& world, Save& save, int city, int good,
                       Vec position, Inventory& supplies);
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_RESIDENT_ORDERS_HPP_
