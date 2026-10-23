#ifndef FOREST_FRONTIER_SRC_GAMEPLAY_HPP_
#define FOREST_FRONTIER_SRC_GAMEPLAY_HPP_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "work_scheduler.hpp"
#include "world.hpp"

namespace frontier {
struct Action {
  bool ok;
  std::string message;
};
const std::vector<Recipe>& Recipes();
const char* GoodName(int good);
int& GoodCount(Inventory& inventory, int good);
int GoodCount(const Inventory& inventory, int good);
bool Equipped(const Inventory& inventory, int recipe);
bool Equipped(const Save& save, int recipe);
void EnsureCityState(Save& save);
void InitializeCampaign(Save& save, bool legacy = false);
bool OwnsCity(const Save& save, int city);
struct Skill {
  std::string id;
  std::string name;
  std::string requirement;
  std::vector<std::string> prerequisites;
};
const std::vector<Skill>& Skills();
bool HasSkill(const Save& save, const std::string& id);
std::string SkillName(const std::string& id);
std::vector<std::string> MissingSkillPrerequisites(const Save& save,
                                                   const std::string& id);
std::string RecipeSkill(const Recipe& recipe);
Action TrainSkill(Save& save, int skill);
struct LessonState {
  bool learned;
  bool ready;
  int progress;
  int target;
  std::string detail;
};
LessonState GetLessonState(const Save& save, int skill);
const WeaponDef& CurrentWeapon(const Save& save);
const char* TreatyName(Treaty treaty);
const char* TechnologyName(const std::string& id);
std::vector<std::string> OwnedWeapons(const Save& save);
Action EquipWeapon(Save& save, const std::string& id);
const std::vector<std::string>& CityActions();
Action Diplomacy(Save& save, int city, int action);
Action LearnTechnology(Save& save, int city);
const WeaponDef& CityWeapon(int city, uint64_t seed);
bool Watercraft(const Save& save);
int ActiveVehicle(const Save& save);
Action CycleVehicle(Save& save);
Action Dive(Save& save);
Action RepairVessel(Save& save);
Action Sonar(Save& save);
int ResourceGood(ResourceKind kind);
Action Craft(Save& save, int recipe);
int BuyPrice(int settlement, int good);
int SellPrice(int settlement, int good);
Action Trade(Save& save, int settlement, int good, bool buy);
Action Consume(Save& save, bool potion);
float HarvestDuration(const Resource& resource, const Inventory& inventory);
Action Harvest(Save& save, const Resource& resource);
// Same authoritative requirements and progression, with explicit actor cargo.
Action HarvestInto(Save& save, Inventory& cargo, bool submerged,
                   const Resource& resource);

class Gameplay {
 public:
  void Advance(float dt);
  Vec MovePlayer(World& world, Save& save, Vec displacement);
  void Tick(World& world, Save& save, float dt, double time);
  void Combat(World& world, Save& save, float dt);
  // Returns only towns identified by this exploration pass, in stable ID order.
  std::vector<int> Explore(Save& save, float width, float height, float zoom,
                           const World* world = nullptr);
  Action Attack(World& world, Save& save);
  Action PlaceCampfire(World& world, Save& save);
  bool RepairSpawn(const World& world, Save& save) const;
  Vec facing{1, 0};
  float walk{};
  float swing{};
  float invulnerable{};
  float attack_cooldown{};
  float attack_elapsed{};
  float attack_duration{};
  float sonar_cooldown{};
  Vec attack_facing{1, 0};
  struct Projectile {
    Vec pos;
    Vec velocity;
    float life;
    int damage;
  };
  std::vector<Projectile> projectiles;
  struct HitEffect {
    Vec position;
    float age{};
    bool defeated{};
    int kind{};
  };
  std::vector<HitEffect> hit_effects;
  const WeaponDef& SwingingWeapon() const {
    return attack_weapon_;
  }
  const WorkScheduler::Stats& WorkStats() const {
    return work_scheduler_.LastStep();
  }
  bool moving{};
  int attacks{};
  int kills{};
  int hits{};
  int damage_taken{};
  int collisions{};
  int respawns{};
  std::string notice;

 private:
  WorkScheduler work_scheduler_;
  bool hit_resolved_{true};
  WeaponDef attack_weapon_;
  void Damage(World& world, Save& save, Enemy& enemy, int amount,
              Vec direction);
  float regeneration_{};
  float civic_clock_{};
  float pressure_clock_{};
  float defender_clock_{};
};
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_GAMEPLAY_HPP_
