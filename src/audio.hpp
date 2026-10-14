#ifndef FOREST_FRONTIER_SRC_AUDIO_HPP_
#define FOREST_FRONTIER_SRC_AUDIO_HPP_
#include <map>
#include <string>
#include <unordered_set>

#include "gameplay.hpp"
#include "raylib.h"
namespace frontier {
class Soundscape {
 public:
  Soundscape();
  ~Soundscape();
  Soundscape(const Soundscape&) = delete;
  Soundscape& operator=(const Soundscape&) = delete;
  bool Ready() const {
    return ready_;
  }
  void Update(const Save* save, const Gameplay* game, bool paused,
              const World* world = nullptr);

 private:
  void Effect(const std::string& id, float gain = 1, float pan = .5f);
  bool ready_{};
  bool owns_device_{};
  Music music_{};
  int track_{-1};
  std::map<std::string, Sound> sounds_;
  float step_clock_{};
  double ambient_at_{};
  int damage_{}, gathered_{}, health_{}, coins_{}, hits_{}, played_attack_{};
  std::unordered_set<uint64_t> alerts_;
  bool initialized_{};
};
}  // namespace frontier
#endif  // FOREST_FRONTIER_SRC_AUDIO_HPP_
