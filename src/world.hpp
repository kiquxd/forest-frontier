#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <array>
#include "content.hpp"

namespace frontier {
struct Vec { float x{}, y{}; };
Vec operator+(Vec a, Vec b);
Vec operator-(Vec a, Vec b);
Vec operator*(Vec a, float b);
float length(Vec a);
Vec normalized(Vec a);
Vec project(Vec a);
Vec unproject(Vec a);
constexpr float tileSize = 48.f;
constexpr int chunkTiles = 16;
constexpr float chunkSize = tileSize * chunkTiles;
struct Coord {
  int x{}, y{};
  bool operator==(const Coord&) const = default;
};
struct CoordHash { size_t operator()(Coord c) const; };
Coord chunkAt(Vec p);
uint64_t hash(uint64_t n);
uint64_t objectId(int x, int y, uint64_t seed);
enum class ResourceKind { Tree, Stone, Berry, Reed, Fish, Ore, Crystal, Pearl, Salvage };
enum class Terrain { Forest, Sand, Shallow, Deep };
Terrain terrainAt(Vec p,uint64_t seed);
const char* terrainName(Terrain terrain);
struct Resource { uint64_t id; Vec pos; ResourceKind kind; int variant; };
struct Animal { Vec pos, home; float phase; Vec previous; float walk{}; };
struct Enemy {
  uint64_t id; Vec pos,home,previous; int hp{60}; float cooldown{},flash{},walk{};
  float windup{};Vec attackTarget{};int kind{},city{-1};
};
struct Tile { uint8_t variant; bool path; Terrain terrain{Terrain::Forest}; };
struct Chunk {
  Coord coord;
  std::vector<Tile> tiles;
  std::vector<Resource> resources;
  std::vector<Animal> animals;
  std::vector<Enemy> enemies;
  std::vector<Resource> underwater;
  double generationMs{};
};
struct Settlement {std::string name;Vec pos;int type{},rarity{},attitude{20};};
struct Npc {
  int id{}, settlement{};
  std::string name, role, personality;
  Vec pos, home;
  float phase{};
  Vec previous;
  float walk{};
  int type{},rarity{};
  bool ruler{};int originCity{-1};
};
struct Building { Vec pos; int variant; bool hall{}; };
const std::vector<Settlement>& settlements();
void configureWorld(uint64_t seed);
bool pathAt(Vec p);
std::vector<Npc> makeNpcs();
std::vector<Building> makeBuildings();
Chunk generateChunk(Coord c, uint64_t seed);
struct Inventory {
  int wood{},stone{},berries{},reeds{},fish{},leather{},potions{},campfires{};
  bool axe{},pickaxe{},sword{},rod{},boat{},armor{};
  std::array<int,goodTypes-6> rare{};
  bool ship{},submarine{},sonar{},oxygen{};
  int ammo{},repairs{};
  std::unordered_set<std::string> weapons{};
  std::string weaponId{};
};
enum class Treaty {Peace,War,Truce,Alliance,Union};
struct Relations {int opinion{20};Treaty treaty{Treaty::Peace};bool discovered{};};
struct Wall {int city;Vec a,b;int hp{200};};
constexpr float mapCellSize=96;
Coord mapCell(Vec p);
bool exploredAt(const std::unordered_set<Coord,CoordHash>& cells,Vec p);
struct QuestProgress {std::string id;bool completed{};int baseline{};};
struct Citizen {int npc{},city{};std::string job="idle",weapon,outfit;bool trained{};};
struct Save {
  Vec player{120, 140};
  Inventory inventory;
  uint64_t seed{742019};
  std::unordered_set<uint64_t> harvested;
  std::unordered_set<uint64_t> defeated;
  int health{100},coins{20};
  std::vector<Vec> campfires;
  std::vector<std::array<int,goodTypes>> stocks{{40,30,25,20,12,8},{30,40,35,20,12,8}};
  std::vector<int> treasury{150,150};
  std::vector<Relations> relations;
  std::unordered_set<std::string> technologies;
  std::unordered_set<Coord,CoordHash> explored,exploredUnderwater;
  std::vector<Wall> walls;
  bool submerged{};float oxygen{100};int hull{100};
  int vehicle{};
  bool campaign{};int homeCity{-1};
  std::vector<int> owners;
  std::vector<std::vector<int>> cityOpinions;
  std::unordered_set<std::string> skills;
  int gathered{},crafted{},caught{},orders{};
  bool adventure{};std::string characterId="traveler",outfitId="traveler";
  std::unordered_map<std::string,int> weaponCopies,outfitCopies;
  std::unordered_map<int,QuestProgress> quests;
  std::unordered_set<std::string> knownRecipes;
  std::vector<Citizen> citizens;
  float workClock{};int trackedQuest{-1};int wolfKills{};
};
Save loadSave(const std::filesystem::path& path);
void writeSave(const std::filesystem::path& path, const Save& save);

// Only CPU generation runs on this worker. GPU resources stay on the render thread.
class World {
public:
  explicit World(uint64_t seed);
  ~World();
  World(const World&) = delete;
  World& operator=(const World&) = delete;
  void request(Vec player, int radius);
  int integrate(int budget = 3);
  size_t pending() const;
  void tick(float dt, const Save& save, double time);
  const Resource* nearest(Vec p, const std::unordered_set<uint64_t>& removed, float reach) const;
  const Resource* nearest(Vec p,const Save& save,float reach) const;
  void beginConversation(int id,const Save& save);
  int talkingNpc{-1};
  bool clear(Vec p,const Save& save,float radius=12,bool boat=false) const;
  Vec move(Vec p,Vec displacement,const Save& save,float radius=12,bool boat=false) const;
  std::vector<Vec> path(Vec start,Vec goal,const Save& save) const;
  std::unordered_map<Coord, Chunk, CoordHash> chunks;
  std::vector<Npc> npcs;
  std::vector<Building> buildings;
  double generatedMs{}, maxGenerationMs{};
  size_t generatedCount{};
private:
  void run();
  uint64_t seed_;
  mutable std::mutex mutex_;
  std::condition_variable wake_;
  bool stopping_{};
  std::deque<Coord> jobs_;
  std::deque<Chunk> ready_;
  std::unordered_set<Coord, CoordHash> desired_, pending_;
  std::thread worker_;
};
}
