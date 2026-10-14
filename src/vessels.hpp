#ifndef FOREST_FRONTIER_SRC_VESSELS_HPP_
#define FOREST_FRONTIER_SRC_VESSELS_HPP_
#include <string>
#include <vector>

#include "world.hpp"
namespace frontier {
struct VesselDef {
  int number;
  std::string id;
  std::string name;
  float speed;
  float view;
  float length;
  float width;
  int prestige;
  int coins;
};
const std::vector<VesselDef>& Vessels();
const VesselDef& Vessel(int number);
const VesselDef& Vessel(const std::string& id);
bool OwnsVessel(const Save& save, int number);
}  // namespace frontier
#endif  // FOREST_FRONTIER_SRC_VESSELS_HPP_
