#include "vessels.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>
#include <utility>

#include "json.hpp"
namespace frontier {
const std::vector<VesselDef>& Vessels() {
  static const std::vector<VesselDef> definitions = [] {
    std::vector<VesselDef> result;
    std::set<int> numbers;
    std::set<std::string> ids;
    for (const auto& entry : std::filesystem::directory_iterator(
             std::string(FRONTIER_ROOT) + "/content/vessels")) {
      if (entry.path().extension() != ".json") {
        continue;
      }
      std::ifstream in(entry.path());
      nlohmann::json j;
      in >> j;
      VesselDef v{j.at("number"), j.at("id"),       j.at("name"),
                  j.at("speed"),  j.at("view"),     j.at("length"),
                  j.at("width"),  j.at("prestige"), j.at("coins")};
      if (v.number < 1 || v.number > 9 || !numbers.insert(v.number).second ||
          !ids.insert(v.id).second || !std::isfinite(v.speed) ||
          !std::isfinite(v.view) || !std::isfinite(v.length) ||
          !std::isfinite(v.width) || v.speed <= 0 || v.view < 400 ||
          v.length < 20 || v.width < 10 || v.prestige < 0 || v.coins < 0) {
        throw std::runtime_error("Invalid vessel definition");
      }
      result.push_back(std::move(v));
    }
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) { return a.number < b.number; });
    if (result.size() != 9) {
      throw std::runtime_error("Incomplete vessel catalog");
    }
    return result;
  }();
  return definitions;
}
const VesselDef& Vessel(int number) {
  for (const auto& v : Vessels()) {
    if (v.number == number) {
      return v;
    }
  }
  throw std::runtime_error("Unknown vessel");
}
const VesselDef& Vessel(const std::string& id) {
  for (const auto& v : Vessels()) {
    if (v.id == id) {
      return v;
    }
  }
  throw std::runtime_error("Unknown vessel ID");
}
bool OwnsVessel(const Save& save, int number) {
  if (number == 1) {
    return save.inventory.boat;
  }
  if (number == 2) {
    return save.inventory.ship;
  }
  if (number == 3) {
    return save.inventory.submarine;
  }
  return save.vessels.contains(Vessel(number).id);
}
}  // namespace frontier
