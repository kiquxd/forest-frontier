#ifndef FOREST_FRONTIER_SRC_SETTLEMENT_SYSTEM_HPP_
#define FOREST_FRONTIER_SRC_SETTLEMENT_SYSTEM_HPP_

#include <string>
#include <vector>

#include "gameplay.hpp"

namespace frontier {
inline constexpr double kMaxCraftCooldown = 7200;
struct FacilityDef {
  std::string id;
  std::string name;
  int wood;
  int stone;
  int coins;
};
const std::vector<FacilityDef>& FacilityTypes();
void InitializeTowns(Save& save);
float CityRadius(const Save& save, int city);
int CityAt(const Save& save, Vec point);
bool ResidentActive(const Save& save, int npc);
int ActiveResidents(const Save& save);
bool RoadAt(const Save& save, Vec point, float radius = 0);
bool BridgeAt(const Save& save, Vec point, float radius = 0);
bool FacilityAccess(const Save& save, int city, const std::string& kind);
std::string RecipeFacility(const Recipe& recipe);
int RecipeRarity(const Recipe& recipe);
double CraftDelay(const Recipe& recipe);
double CraftWait(const Save& save);
Action BuildFacility(Save& save, int city, const std::string& kind);
Action ExpandCity(Save& save, int city);
struct RoadQuote {
  bool ok{};
  std::string message;
  int wood{};
  int stone{};
  int land_tiles{};
  int bridge_tiles{};
};
RoadQuote QuoteRoad(const Save& save, int from, int to,
                    const std::vector<Vec>& points);
Action BuildRoad(Save& save, int from, int to, const std::vector<Vec>& points);
Action LoseResident(Save& save, int npc, bool captured, int captor = -1);
Action RansomResident(Save& save, int npc);
void DestroyPlayerWeapon(Save& save);
Action SurrenderTown(Save& save, int city, int patron);
Action BuyIndependence(Save& save, int city, bool restore_previous);
void TickTowns(Save& save, float dt);
std::string TownInfrastructure(const Save& save, int city);
}  // namespace frontier
#endif  // FOREST_FRONTIER_SRC_SETTLEMENT_SYSTEM_HPP_
