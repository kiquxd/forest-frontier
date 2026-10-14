#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>

#include "perf.hpp"
#include "world.hpp"

namespace {

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

}  // namespace

int main() {
  try {
    Check(frontier::ChunkAt({-.1f, -768.1f}) == frontier::Coord{-1, -2},
          "Negative chunk coordinates");
    for (frontier::Vec position :
         {frontier::Vec{0, 0}, frontier::Vec{-124, 895},
          frontier::Vec{2300, -2000}}) {
      Check(frontier::Length(frontier::Unproject(frontier::Project(position)) -
                             position) < .001f,
            "Projection round trip");
    }
    auto a = frontier::GenerateChunk({-2, 3}, 742019),
         b = frontier::GenerateChunk({-2, 3}, 742019),
         chunk = frontier::GenerateChunk({-2, 3}, 5);
    Check(a.tiles.size() == 256, "Chunk tiles");
    Check(a.resources.size() == b.resources.size(),
          "Deterministic resource count");
    for (size_t i = 0; i < a.resources.size(); i++) {
      auto& resource = a.resources[i];
      Check(resource.id == b.resources[i].id &&
                resource.pos.x == b.resources[i].pos.x &&
                resource.pos.y == b.resources[i].pos.y,
            "Deterministic resource positions");
      Check(frontier::ChunkAt(resource.pos) == a.coord,
            "Resource stays in chunk");
    }
    Check(!a.resources.empty() && !chunk.resources.empty() &&
              a.resources.front().id != chunk.resources.front().id,
          "Seed changes world");
    auto temp = std::filesystem::temp_directory_path() /
                ("frontier-test-" +
                 std::to_string(frontier::Hash(static_cast<uint64_t>(
                     frontier::Clock::now().time_since_epoch().count()))));
    std::filesystem::create_directories(temp);
    frontier::Save save;
    save.player = {-50, 125};
    save.inventory = {7, 3, 2};
    save.harvested.insert(a.resources.front().id);
    frontier::WriteSave(temp / "world.json", save);
    auto loaded = frontier::LoadSave(temp / "world.json");
    Check(loaded.seed == save.seed && loaded.harvested == save.harvested &&
              loaded.player.x == -50 && loaded.inventory.wood == 7,
          "Save round trip preserves uint64 IDs");
    frontier::World world(save.seed);
    world.Request({-900, 2700}, 1);
    auto deadline = frontier::Clock::now() + std::chrono::seconds(3);
    while (world.chunks.size() < 9 && frontier::Clock::now() < deadline) {
      world.Integrate();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Check(world.chunks.size() == 9 && world.Pending() == 0,
          "Async generation complete");
    frontier::Vec rp = a.resources.front().pos;
    Check(world.Nearest(rp, std::unordered_set<uint64_t>{}, 5) != nullptr,
          "Resource lookup");
    Check(world.Nearest(rp, save.harvested, 5) == nullptr,
          "Harvest stays removed");
    world.Request({10000, -10000}, 1);
    world.Integrate();
    deadline = frontier::Clock::now() + std::chrono::seconds(3);
    while (world.chunks.size() < 9 && frontier::Clock::now() < deadline) {
      world.Integrate();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Check(world.chunks.size() == 9, "Bounded resident chunk cache");
    for (auto& [coord, chunk] : world.chunks) {
      Check(std::abs(coord.x - frontier::ChunkAt({10000, -10000}).x) <= 1,
            "Old chunks evicted");
    }
    {
      frontier::Perf perf(temp);
      perf.Frame(10, 1, 2, 7, false);
      perf.Frame(30, 1, 2, 27, true);
      perf.Flush();
      Check(perf.p95 == 30 && perf.fps == 50, "Frame statistics");
    }
    std::filesystem::remove_all(temp);
    std::cout << "world, persistence, worker, projection and telemetry checks "
                 "passed\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
