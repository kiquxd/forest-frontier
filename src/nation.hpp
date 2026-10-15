#ifndef FOREST_FRONTIER_SRC_NATION_HPP_
#define FOREST_FRONTIER_SRC_NATION_HPP_

#include <string>
#include <vector>

#include "gameplay.hpp"
#include "json.hpp"

namespace frontier {
void InitializeNation(Save& save);
void StoreControlled(Save& save);
void SyncNationActors(World& world, const Save& save);
Action SelectResident(Save& save, int npc);
std::string ResidentName(const Save& save, int npc);
bool HasControlled(const Save& save);
int64_t MoneySupply(const Save& save);
int CityPopulation(const Save& save, int city);
int FacilityLimit(const Save& save, int city, const std::string& kind);
bool ConnectedCities(const Save& save, int from, int to);
Action TransferGoods(Save& save, int city, int good, int amount, bool deposit);
Action TransferProduct(Save& save, int city, const std::string& product,
                       bool deposit);
struct ProductionRequirement {
  std::string icon;
  std::string label;
  std::string detail;
  bool met{};
};
struct ProductionStatus {
  uint64_t facility{};
  double due{};
  int fee{};
  std::vector<std::string> missing;
  std::vector<ProductionRequirement> requirements;
};
// Read-only quote shared by the catalogue and authoritative queue action.
ProductionStatus CheckProduction(const Save& save, int recipe, int source,
                                 uint64_t facility = 0);
Action QueueProduction(Save& save, int recipe, int source = -1,
                       uint64_t facility = 0);
Action CancelProduction(Save& save, uint64_t id);
struct ProductionProgress {
  double fraction{};
  double remaining{};
  double starts_in{};
  int ahead{};
  std::string state;
};
ProductionProgress GetProductionProgress(const Save& save,
                                         const WorkshopJob& job);
// Active-world runtime only, supplied from steady_clock by the application.
// This is separate from paused simulation updates and never uses offline time.
int TickProduction(Save& save, double elapsed_seconds);
Action PlantResource(Save& save, int city, int good);
Action HarvestPlot(Save& save, int index);
Action OrderRareGoods(Save& save, int from, int to, int good, int quantity);
Action ShipGoods(Save& save, int from, int to, int good, int quantity);
Action AcceptCityTask(Save& save, uint64_t id);
Action FinishCityTask(Save& save, uint64_t id);
Action CancelCityTask(Save& save, uint64_t id);
void RefreshCityTasks(Save& save);
Action AcceptTraveler(Save& save, int npc);
Action MoveResident(Save& save, int npc, int city);
Action BuildHousing(Save& save, int city);
Action TreatResident(Save& save, int npc);
Action ResearchHospital(Save& save);
Action AssignNationCitizen(Save& save, int npc, const std::string& job,
                           const std::string& weapon,
                           const std::string& outfit);
void TickNation(Save& save, float dt);
void ReadNation(const nlohmann::json& json, Save& save);
nlohmann::json WriteNation(const Save& save);
}  // namespace frontier
#endif  // FOREST_FRONTIER_SRC_NATION_HPP_
