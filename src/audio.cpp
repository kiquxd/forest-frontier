#include "audio.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include "input.hpp"
namespace frontier {
Soundscape::Soundscape() {
  owns_device_ = !IsAudioDeviceReady();
  if (owns_device_) {
    InitAudioDevice();
  }
  ready_ = IsAudioDeviceReady();
  if (!ready_) {
    return;
  }
  for (auto name :
       {"step_grass", "step_stone", "step_water", "swing", "hit", "gather",
        "ui", "bubbles", "forest_air", "water_air", "fire_air", "wolf"}) {
    auto path = std::string(FRONTIER_ROOT) + "/assets/audio/" + name + ".wav";
    auto sound = LoadSound(path.c_str());
    if (IsSoundValid(sound)) {
      sounds_.emplace(name, sound);
    }
  }
}
Soundscape::~Soundscape() {
  if (!ready_) {
    return;
  }
  if (IsMusicValid(music_)) {
    UnloadMusicStream(music_);
  }
  for (auto& [id, sound] : sounds_) {
    UnloadSound(sound);
  }
  if (owns_device_) {
    CloseAudioDevice();
  }
}
void Soundscape::Effect(const std::string& id, float gain, float pan) {
  if (ActiveSettings().effects_volume <= 0) {
    return;
  }
  auto found = sounds_.find(id);
  if (found == sounds_.end()) {
    return;
  }
  SetSoundVolume(found->second, ActiveSettings().effects_volume * gain);
  SetSoundPan(found->second, pan);
  PlaySound(found->second);
}
void Soundscape::Update(const Save* save, const Gameplay* game, bool paused,
                        const World* world) {
  if (!ready_) {
    return;
  }
  const auto& settings = ActiveSettings();
  int desired = settings.music_mode;
  bool water =
      save && (save->submerged ||
               TerrainAt(save->player, save->seed) == Terrain::kDeep ||
               TerrainAt(save->player, save->seed) == Terrain::kShallow);
  if (desired == 0) {
    desired = save && save->submerged ? 3 : water ? 2 : 1;
  }
  if (desired != track_) {
    if (IsMusicValid(music_)) {
      UnloadMusicStream(music_);
      music_ = {};
    }
    track_ = desired;
    if (desired >= 1 && desired <= 3) {
      const std::array<std::string, 3> tracks = {"forest", "harbor", "depths"};
      auto path = std::string(FRONTIER_ROOT) + "/assets/audio/" +
                  tracks[desired - 1] + ".wav";
      music_ = LoadMusicStream(path.c_str());
      if (IsMusicValid(music_)) {
        music_.looping = true;
        PlayMusicStream(music_);
      }
    }
  }
  if (IsMusicValid(music_)) {
    SetMusicVolume(music_, settings.music_volume);
    UpdateMusicStream(music_);
  }
  if (!save || !game) {
    initialized_ = false;
    return;
  }
  if (initialized_ && !paused) {
    if (game->swing > 0 &&
        game->attack_elapsed >= game->attack_duration * .3f &&
        game->attacks != played_attack_) {
      Effect("swing");
      played_attack_ = game->attacks;
    }
    auto spatial = [&](const std::string& id, Vec source, float gain) {
      float distance = Length(source - save->player);
      float attenuation = std::clamp(1 - distance / 700.f, 0.f, 1.f);
      float pan =
          std::clamp(.5f + Project(source - save->player).x / 1000.f, 0.f, 1.f);
      if (attenuation > 0) {
        Effect(id, gain * attenuation, pan);
      }
    };
    if (game->hits > hits_ && !game->hit_effects.empty()) {
      spatial("hit", game->hit_effects.back().position, 1);
    }
    if (world) {
      std::unordered_set<uint64_t> active;
      for (const auto& [coord, chunk] : world->chunks) {
        for (const auto& enemy : chunk.enemies) {
          if (!enemy.kind && enemy.windup > 0 &&
              !save->defeated.contains(enemy.id)) {
            active.insert(enemy.id);
            if (!alerts_.contains(enemy.id)) {
              spatial("wolf", enemy.pos, .8f);
            }
          }
        }
      }
      alerts_ = std::move(active);
    }
    if (game->damage_taken > damage_ || save->health < health_) {
      Effect("hit");
    }
    if (save->gathered > gathered_) {
      Effect("gather");
    }
    if (save->coins != coins_) {
      Effect("ui", .5f);
    }
    if (game->moving) {
      step_clock_ += GetFrameTime();
      if (step_clock_ > .32f) {
        Effect(water                  ? "step_water"
               : PathAt(save->player) ? "step_stone"
                                      : "step_grass",
               .6f);
        step_clock_ = 0;
      }
    }
    if (GetTime() > ambient_at_) {
      Effect(save->submerged ? "bubbles"
             : water         ? "water_air"
                             : "forest_air",
             .25f);
      for (const auto& fire : save->campfires) {
        if (Length(fire - save->player) < 500) {
          spatial("fire_air", fire, .7f);
        }
      }
      ambient_at_ = GetTime() + 4;
    }
  }
  if (!initialized_) {
    played_attack_ = game->attacks;
  }
  initialized_ = true;
  hits_ = game->hits;
  damage_ = game->damage_taken;
  gathered_ = save->gathered;
  health_ = save->health;
  coins_ = save->coins;
}
}  // namespace frontier
