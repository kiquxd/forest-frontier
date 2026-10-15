#ifndef FOREST_FRONTIER_SRC_WORLD_HPP_
#define FOREST_FRONTIER_SRC_WORLD_HPP_

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "content.hpp"

namespace frontier {
struct Vec {
  float x{};
  float y{};
};
Vec operator+(Vec a, Vec b);
Vec operator-(Vec a, Vec b);
Vec operator*(Vec a, float b);
float Length(Vec a);
Vec Normalized(Vec a);
Vec Project(Vec a);
Vec Unproject(Vec a);
constexpr float kTileSize = 48.f;
constexpr int kChunkTiles = 16;
constexpr float kChunkSize = kTileSize * kChunkTiles;
struct Coord {
  int x{};
  int y{};
  bool operator==(const Coord&) const = default;
};
struct CoordHash {
  size_t operator()(Coord coord) const;
};
Coord ChunkAt(Vec position);
uint64_t Hash(uint64_t n);
uint64_t ObjectId(int x, int y, uint64_t seed);
enum class ResourceKind {
  kTree,
  kStone,
  kBerry,
  kReed,
  kFish,
  kOre,
  kCrystal,
  kPearl,
  kSalvage
};
enum class Terrain { kForest, kSand, kShallow, kDeep };
Terrain TerrainAt(Vec position, uint64_t seed);
const char* TerrainName(Terrain terrain);
struct Resource {
  uint64_t id;
  Vec pos;
  ResourceKind kind;
  int variant;
};
struct Animal {
  Vec pos;
  Vec home;
  float phase;
  Vec previous;
  float walk{};
};
struct Enemy {
  uint64_t id;
  Vec pos;
  Vec home;
  Vec previous;
  int hp{60};
  float cooldown{};
  float flash{};
  float walk{};
  float windup{};
  Vec attack_target{};
  int kind{};
  int city{-1};
  int target_npc{-1};
};
struct Tile {
  uint8_t variant;
  bool path;
  Terrain terrain{Terrain::kForest};
};
struct Chunk {
  Coord coord;
  std::vector<Tile> tiles;
  std::vector<Resource> resources;
  std::vector<Animal> animals;
  std::vector<Enemy> enemies;
  std::vector<Resource> underwater;
  double generation_ms{};
};
struct Settlement {
  std::string name;
  Vec pos;
  int type{};
  int rarity{};
  int attitude{20};
};
struct Npc {
  int id{};
  int settlement{};
  std::string name;
  std::string role;
  std::string personality;
  Vec pos;
  Vec home;
  float phase{};
  Vec previous;
  float walk{};
  int type{};
  int rarity{};
  bool ruler{};
  int origin_city{-1};
};
struct Building {
  Vec pos;
  int variant;
  bool hall{};
  std::string facility{};
};
const std::vector<Settlement>& Settlements();
void ConfigureWorld(uint64_t seed);
bool PathAt(Vec position);
std::vector<Npc> MakeNpcs();
std::vector<Building> MakeBuildings();
Chunk GenerateChunk(Coord coord, uint64_t seed);
struct Inventory {
  int wood{};
  int stone{};
  int berries{};
  int reeds{};
  int fish{};
  int leather{};
  int potions{};
  int campfires{};
  bool axe{};
  bool pickaxe{};
  bool sword{};
  bool rod{};
  bool boat{};
  bool armor{};
  std::array<int, kGoodTypes - 6> rare{};
  bool ship{};
  bool submarine{};
  bool sonar{};
  bool oxygen{};
  int ammo{};
  int repairs{};
  std::unordered_set<std::string> weapons{};
  std::string weapon_id{};
};
enum class Treaty { kPeace, kWar, kTruce, kAlliance, kUnion };
struct Relations {
  int opinion{20};
  Treaty treaty{Treaty::kPeace};
  bool discovered{};
};
struct Wall {
  int city;
  Vec a;
  Vec b;
  int hp{200};
};
constexpr float kMapCellSize = 96;
Coord MapCell(Vec position);
bool ExploredAt(const std::unordered_set<Coord, CoordHash>& cells,
                Vec position);
struct QuestProgress {
  std::string id;
  bool completed{};
  int baseline{};
};
struct Citizen {
  int npc{};
  int city{};
  std::string job = "idle";
  std::string weapon;
  std::string outfit;
  bool trained{};
  int health{100};
  std::string state = "active";
  int captor{-1};
  int ransom{};
  Vec position{};
  Inventory cargo{};
  std::string background = "traveler";
  std::unordered_map<std::string, int> weapons{};
  std::unordered_map<std::string, int> clothes{};
  std::unordered_set<std::string> vessels{};
  bool underwater{};
  float air{100};
  int hull{100};
  int vehicle{};
};
struct HistoryEntry {
  std::string kind;
  std::string description;
  int64_t timestamp{};
  int city{-1};
};
struct MapNote {
  Vec position;
  bool underwater{};
  std::string title;
  std::string text;
};
struct KnownDeposit {
  Vec position;
  int good;
  bool underwater;
};
struct Facility {
  int city;
  std::string kind;
  Vec position;
  uint64_t id{};
};
struct Road {
  int from;
  int to;
  std::vector<Vec> points;
};
struct Tribute {
  int city;
  int patron;
  int previous_owner;
  int amount{10};
  int buyout{300};
  double next_due{};
};
struct WorkshopJob {
  uint64_t id;
  uint64_t facility;
  int source;
  int destination;
  std::string recipe;
  double due;
  std::array<int, kGoodTypes> materials;
  int fee{};
};
struct GrowingPlot {
  int city;
  int good;
  double due;
  bool ready{};
};
struct CityTask {
  uint64_t id;
  int city;
  std::string kind;
  int good;
  int quantity;
  int price{};
  int escrow{};
  bool accepted{};
  bool completed{};
  std::string reward;
  int baseline{};
  double expires{};
};
struct Shipment {
  uint64_t id;
  int from;
  int to;
  int good;
  int quantity;
  double due;
};
struct Traveler {
  int npc;
  std::string name;
  int city;
  bool accepted{};
  bool declined{};
};
struct Save {
  Vec player{120, 140};
  Inventory inventory;
  uint64_t seed{742019};
  std::unordered_set<uint64_t> harvested;
  std::unordered_set<uint64_t> defeated;
  int health{100};
  int coins{20};
  std::vector<Vec> campfires;
  std::vector<std::array<int, kGoodTypes>> stocks{{40, 30, 25, 20, 12, 8},
                                                  {30, 40, 35, 20, 12, 8}};
  std::vector<int> treasury{150, 150};
  std::vector<Relations> relations;
  std::unordered_set<std::string> technologies;
  std::unordered_set<Coord, CoordHash> explored;
  std::unordered_set<Coord, CoordHash> explored_underwater;
  std::vector<Wall> walls;
  bool submerged{};
  float oxygen{100};
  int hull{100};
  int vehicle{};
  bool campaign{};
  int home_city{-1};
  std::vector<int> owners;
  std::vector<std::vector<int>> city_opinions;
  std::unordered_set<std::string> skills;
  int gathered{};
  int crafted{};
  int caught{};
  int orders{};
  bool adventure{};
  std::string character_id = "traveler";
  std::string outfit_id = "traveler";
  std::unordered_map<std::string, int> weapon_copies;
  std::unordered_map<std::string, int> outfit_copies;
  std::unordered_map<int, QuestProgress> quests;
  std::unordered_set<std::string> known_recipes;
  std::vector<Citizen> citizens;
  float work_clock{};
  int tracked_quest{-1};
  int wolf_kills{};
  std::array<bool, kGoodTypes> known_goods{};
  std::vector<HistoryEntry> history;
  std::vector<MapNote> map_notes;
  std::unordered_map<uint64_t, KnownDeposit> known_deposits;
  std::unordered_set<Coord, CoordHash> visible_surface;
  std::unordered_set<Coord, CoordHash> visible_underwater;
  std::unordered_set<std::string> vessels{};
  bool urban{};
  double game_time{};
  double craft_ready_at{};
  std::vector<float> city_radii;
  std::vector<int> previous_owners;
  std::vector<Facility> facilities;
  std::vector<Road> roads;
  std::vector<Tribute> tributes;
  bool nation{};
  std::string state_trait = "traveler";
  int controlled_npc{-1};
  int region_fund{};
  int64_t money_supply{};
  uint64_t next_entity{1};
  int64_t economy_cycle{-1};
  std::vector<std::unordered_map<std::string, int>> city_products;
  std::vector<int> housing;
  double production_time{};
  std::vector<WorkshopJob> production;
  std::vector<GrowingPlot> plots;
  std::vector<CityTask> city_tasks;
  std::vector<Shipment> shipments;
  std::vector<Traveler> travelers;
};
Save LoadSave(const std::filesystem::path& path);
void WriteSave(const std::filesystem::path& path, const Save& save);

// Only CPU generation runs on this worker. GPU resources stay on the render
// thread.
class World {
 public:
  explicit World(uint64_t seed);
  ~World();
  World(const World&) = delete;
  World& operator=(const World&) = delete;
  void Request(Vec player, int radius);
  int Integrate(int budget = 3);
  size_t Pending() const;
  void Tick(float dt, const Save& save, double time);
  const Resource* Nearest(Vec position,
                          const std::unordered_set<uint64_t>& removed,
                          float reach) const;
  const Resource* Nearest(Vec position, const Save& save, float reach) const;
  void BeginConversation(int id, const Save& save);
  int talking_npc{-1};
  bool Clear(Vec position, const Save& save, float radius = 12,
             bool boat = false) const;
  void ResolvePlayerContacts(Save& save);
  void SyncInfrastructure(const Save& save);
  bool SightBlocked(Vec start, Vec end, const Save& save) const;
  Vec Move(Vec position, Vec displacement, const Save& save, float radius = 12,
           bool boat = false) const;
  std::vector<Vec> Path(Vec start, Vec goal, const Save& save) const;
  std::unordered_map<Coord, Chunk, CoordHash> chunks;
  std::vector<Npc> npcs;
  std::vector<Building> buildings;
  double generated_ms{};
  double max_generation_ms{};
  size_t generated_count{};

 private:
  void Run();
  uint64_t seed_;
  mutable std::mutex mutex_;
  std::condition_variable wake_;
  bool stopping_{};
  std::deque<Coord> jobs_;
  std::deque<Chunk> ready_;
  std::unordered_set<Coord, CoordHash> desired_;
  std::unordered_set<Coord, CoordHash> pending_;
  std::thread worker_;
};
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_WORLD_HPP_
