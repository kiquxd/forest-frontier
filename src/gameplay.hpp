#pragma once
#include "world.hpp"
#include <array>

namespace frontier {
struct Action {bool ok;std::string message;};
const std::vector<Recipe>& recipes();
const char* goodName(int good);
int& goodCount(Inventory& inv,int good);
int goodCount(const Inventory& inv,int good);
bool equipped(const Inventory& inv,int recipe);
void ensureCityState(Save& save);
void initializeCampaign(Save& save,bool legacy=false);
bool ownsCity(const Save& save,int city);
struct Skill {std::string id,name,requirement;};
const std::vector<Skill>& skills();
bool hasSkill(const Save& save,const std::string& id);
std::string skillName(const std::string& id);
std::string recipeSkill(const Recipe& recipe);
Action trainSkill(Save& save,int skill);
struct LessonState {bool learned,ready;int progress,target;std::string detail;};
LessonState lessonState(const Save& save,int skill);
const WeaponDef& currentWeapon(const Save& save);
const char* treatyName(Treaty treaty);
const char* technologyName(const std::string& id);
std::vector<std::string> ownedWeapons(const Save& save);
Action equipWeapon(Save& save,const std::string& id);
const std::vector<std::string>& cityActions();
Action diplomacy(Save& save,int city,int action);
Action learnTechnology(Save& save,int city);
const WeaponDef& cityWeapon(int city,uint64_t seed);
bool watercraft(const Save& save);
int activeVehicle(const Save& save);
Action cycleVehicle(Save& save);
Action dive(Save& save);
Action repairVessel(Save& save);
Action sonar(Save& save);
int resourceGood(ResourceKind kind);
Action craft(Save& save,int recipe);
int buyPrice(int settlement,int good);
int sellPrice(int settlement,int good);
Action trade(Save& save,int settlement,int good,bool buy);
Action consume(Save& save,bool potion);
float harvestDuration(const Resource& resource,const Inventory& inv);
Action harvest(Save& save,const Resource& resource);

class Gameplay {
public:
  void advance(float dt);
  Vec movePlayer(World& world,Save& save,Vec displacement);
  void tick(World& world,Save& save,float dt,double time);
  void combat(World& world,Save& save,float dt);
  void explore(Save& save,float width,float height,float zoom);
  Action attack(World& world,Save& save);
  Action placeCampfire(World& world,Save& save);
  bool repairSpawn(const World& world,Save& save) const;
  Vec facing{1,0};
  float walk{},swing{},invulnerable{},attackCooldown{};
  float attackElapsed{},attackDuration{},sonarCooldown{};
  Vec attackFacing{1,0};
  struct Projectile {Vec pos,velocity;float life;int damage;};
  std::vector<Projectile> projectiles;
  const WeaponDef& swingingWeapon() const {return attackWeapon_;}
  bool moving{};
  int attacks{},kills{},damageTaken{},collisions{},respawns{};
  std::string notice;
private:
  bool hitResolved_{true};
  WeaponDef attackWeapon_;
  void damage(World& world,Save& save,Enemy& enemy,int amount,Vec direction);
  float regeneration_{};
  float civicClock_{};
  float pressureClock_{};float defenderClock_{};
};
}
