#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <ratio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "adventure.hpp"
#include "atlas_notes.hpp"
#include "dialogue.hpp"
#include "gameplay.hpp"
#include "input.hpp"
#include "json.hpp"
#include "menu.hpp"
#include "nation.hpp"
#include "perf.hpp"
#include "progress.hpp"
#include "raylib.h"
#include "render.hpp"
#include "rlgl.h"
#include "settings_ui.hpp"
#include "settlement_system.hpp"
#include "vessels.hpp"
#include "world.hpp"

namespace {

static std::vector<std::string> graphics_info;
void ToggleGameFullscreen() {
  static Vector2 windowed{1600, 900};
  if (!IsWindowFullscreen()) {
    windowed = {static_cast<float>(GetScreenWidth()),
                static_cast<float>(GetScreenHeight())};
    int monitor = GetCurrentMonitor();
    // raylib keeps the requested logical size during fullscreen; match the
    // native mode.
    SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
    ToggleFullscreen();
  } else {
    ToggleFullscreen();
    SetWindowSize(static_cast<int>(windowed.x), static_cast<int>(windowed.y));
  }
}
void StartupTrace(int level, const char* format, va_list args) {
  char line[2048];
  std::vsnprintf(line, sizeof(line), format, args);
  std::string message = line;
  if (message.find("Vendor:") != std::string::npos ||
      message.find("Renderer:") != std::string::npos ||
      message.find("Version:") != std::string::npos) {
    graphics_info.push_back(message);
  }
  if (level >= LOG_WARNING) {
    std::cerr << message << '\n';
  }
}
struct Options {
  int width = 1600;
  int height = 900;
  int port = 8080;
  int frames = 0;
  bool uncapped = false;
  bool benchmark_llm = false;
  bool benchmark_gameplay = false;
  bool no_save = false;
  bool benchmark_expedition = false;
  bool benchmark_community = false;
  bool save_explicit = false;
  bool port_explicit = false;
  bool fullscreen = false;
  bool direct = false;
  bool debug = true;
  bool dialogues = true;
  int fps = 60;
  float zoom = 1.15f;
  std::optional<frontier::Vec> spawn;
  std::string preview;
  double benchmark_seconds = 0;
  uint64_t seed = 742019;
  bool seed_explicit{};
  std::filesystem::path settings_path =
      std::string(FRONTIER_ROOT) + "/settings.json";
  std::filesystem::path log_dir = std::string(FRONTIER_ROOT) + "/logs";
  std::filesystem::path save_path =
      std::string(FRONTIER_ROOT) + "/saves/world.json";
  std::filesystem::path screenshot;
};
Options Parse(int argc, char** argv) {
  Options options;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    auto value = [&]() -> std::string {
      if (i + 1 >= argc) {
        throw std::runtime_error("Missing value for " + arg);
      }
      return argv[++i];
    };
    if (arg == "--play") {
      options.direct = true;
    } else if (arg == "--uncapped") {
      options.uncapped = true;
    } else if (arg == "--fullscreen") {
      options.fullscreen = true;
    } else if (arg == "--benchmark") {
      options.benchmark_seconds = std::stod(value());
      options.no_save = true;
    } else if (arg == "--benchmark-llm") {
      options.benchmark_llm = true;
    } else if (arg == "--benchmark-gameplay") {
      options.benchmark_gameplay = true;
    } else if (arg == "--benchmark-community") {
      options.benchmark_community = true;
      options.benchmark_expedition = true;
      options.benchmark_gameplay = true;
    } else if (arg == "--benchmark-expedition") {
      options.benchmark_expedition = true;
      options.benchmark_gameplay = true;
    } else if (arg == "--spawn") {
      float x = std::stof(value()), y = std::stof(value());
      options.spawn = frontier::Vec{x, y};
      options.no_save = true;
    } else if (arg == "--preview") {
      options.preview = value();
      options.no_save = true;
    } else if (arg == "--no-save") {
      options.no_save = true;
    } else if (arg == "--width") {
      options.width = std::stoi(value());
    } else if (arg == "--height") {
      options.height = std::stoi(value());
    } else if (arg == "--llm-port") {
      options.port = std::stoi(value());
      options.port_explicit = true;
    } else if (arg == "--frames") {
      options.frames = std::stoi(value());
    } else if (arg == "--seed") {
      options.seed = std::stoull(value());
      options.seed_explicit = true;
    } else if (arg == "--settings") {
      options.settings_path = value();
    } else if (arg == "--log-dir") {
      options.log_dir = value();
    } else if (arg == "--save") {
      options.save_path = value();
      options.save_explicit = true;
    } else if (arg == "--screenshot") {
      options.screenshot = value();
    } else if (arg == "--help") {
      std::cout
          << "forest_frontier [--uncapped] [--benchmark SECONDS] "
             "[--benchmark-llm]\n"
             "  [--play] [--width N --height N] [--llm-port N] [--seed N] "
             "[--no-save]\n"
             "  [--settings FILE] [--frames N] [--log-dir DIR] [--save FILE] "
             "[--screenshot FILE]\n"
             "  [--fullscreen] [--benchmark-gameplay] [--benchmark-expedition] "
             "[--benchmark-community] [--spawn X Y]\n"
             "  [--preview "
             "menu|newgame|saves|settings|quests|skills|clothes|residents|"
             "houses|craft|trade|city|map|talk|ruler|ship|submarine|weapons|"
             "combat]\n";
      std::exit(0);
    } else {
      throw std::runtime_error("Unknown option: " + arg);
    }
  }
  if (options.width < 800 || options.width > 7680 || options.height < 600 ||
      options.height > 4320 || options.port < 1 || options.port > 65535 ||
      options.frames < 0 || !std::isfinite(options.benchmark_seconds) ||
      options.benchmark_seconds < 0) {
    throw std::runtime_error(
        "Invalid window size, port, frames or benchmark duration");
  }
  if (options.benchmark_llm && options.benchmark_seconds <= 0) {
    throw std::runtime_error("--benchmark-llm requires --benchmark SECONDS");
  }
  if (options.benchmark_gameplay && options.benchmark_seconds <= 0) {
    throw std::runtime_error(
        "--benchmark-gameplay requires --benchmark SECONDS");
  }
  if (options.spawn &&
      (!std::isfinite(options.spawn->x) || !std::isfinite(options.spawn->y) ||
       frontier::Length(*options.spawn) > 1e6)) {
    throw std::runtime_error("Invalid spawn");
  }
  const std::vector<std::string> previews = {
      "menu",   "newgame",   "saves",     "settings",   "quests",
      "skills", "clothes",   "residents", "houses",     "craft",
      "trade",  "city",      "map",       "talk",       "ruler",
      "ship",   "submarine", "weapons",   "combat",     "raft",
      "canoe",  "skiff",     "cutter",    "brigantine", "flagship"};
  if (!options.preview.empty() &&
      std::find(previews.begin(), previews.end(), options.preview) ==
          previews.end()) {
    throw std::runtime_error("Unknown preview");
  }
  return options;
}
std::string NpcPrompt(const frontier::Npc& npc, const frontier::Save& save) {
  return "You are a character in Forest Frontier, a quiet exploration and "
         "survival game. Reply only in English, in first person, in 1-3 short "
         "sentences. "
         "Do not output reasoning, tags or lists. Never mention AI or language "
         "models. "
         "Your name is " +
         npc.name + ", your role is " + npc.role + ", and you live in " +
         frontier::Settlements()[npc.settlement].name + ". " + npc.personality +
         (npc.origin_city >= 0
              ? " You are visiting from " +
                    frontier::Settlements()[npc.origin_city].name +
                    ". The host's attitude to your town is " +
                    std::to_string(save.campaign
                                       ? save.city_opinions[npc.settlement]
                                                           [npc.origin_city]
                                       : 0) +
                    ". Unwelcome visitors stay outside the central square."
              : " ") +
         (npc.ruler
              ? " You are the ruler and receive visitors inside the town hall."
              : " ") +
         (frontier::OwnsCity(save, npc.settlement)
              ? " The player owns this town."
              : " This town is independent of the player.") +
         " Town halls offer free skill lessons after practical milestones. "
         "Gathering is the first lesson and has no prerequisite. Learned "
         "skills: " +
         std::to_string(save.skills.size()) +
         ". The world has pine forests, lakes, a coast, wolves, and towns with "
         "different technologies. "
         "Player resources: wood " +
         std::to_string(save.inventory.wood) + ", stone " +
         std::to_string(save.inventory.stone) + ", berries " +
         std::to_string(save.inventory.berries) + ", coins " +
         std::to_string(save.coins) +
         ". "
         "WASD or arrow keys: movement. E near residents: talk; P at a town "
         "hall: lessons and audience; T: trade; C: craft; M: map; I: "
         "equipment; V: vessel; R: dive; G: sonar. "
         "A fishing rod catches fish; a boat crosses deep water; campfires "
         "heal and deter wolves. Skills and technologies unlock crafting. "
         "This town's technology: " +
         frontier::TechnologyName(
             frontier::GetContent()
                 .towns[frontier::Settlements()[npc.settlement].type]
                 .technology) +
         ". Trust: " + std::to_string(save.relations[npc.settlement].opinion) +
         ", treaty: " +
         frontier::TreatyName(save.relations[npc.settlement].treaty) +
         ". Technology research requires peace, trust 20, craftsmanship and 20 "
         "coins. Skill lessons are free. Alliance requires trust 30; union "
         "requires trust 70 and an alliance. Guards attack during war. "
         "Pineford teaches ironworking; Saltwater Wharf teaches shipbuilding; "
         "Copper Beacon teaches pressure hulls; Moon Academy teaches optics; "
         "Pearl Haven teaches deep-sea harvesting. "
         "You only converse. Never claim you changed resources, treaties or "
         "skills; those changes happen through gameplay buttons.";
}
std::string CpuName() {
  std::ifstream f("/proc/cpuinfo");
  std::string line;
  while (std::getline(f, line)) {
    if (line.starts_with("model name")) {
      return line.substr(line.find(':') + 2);
    }
  }
  return "unknown";
}
frontier::Action FixtureCraft(frontier::Save& save, int recipe) {
  // Explicit benchmark fixtures advance simulated craft time and use the home
  // workshop. This helper is never called by ordinary gameplay input.
  save.game_time = std::max(save.game_time, save.craft_ready_at);
  auto position = save.player;
  bool submerged = save.submerged;
  save.submerged = false;
  save.player = frontier::Settlements()[save.home_city].pos;
  auto result = frontier::Craft(save, recipe);
  save.player = position;
  save.submerged = submerged;
  return result;
}

int Play(const Options& options) {
  frontier::Perf perf(options.log_dir);
  frontier::Save save;
  if (!options.no_save) {
    save = frontier::LoadSave(options.save_path);
  }
  if (options.no_save || !std::filesystem::exists(options.save_path)) {
    save.seed = options.seed;
  }
  if (options.spawn) {
    save.player = *options.spawn;
  }
  if (options.benchmark_gameplay || !options.preview.empty()) {
    save.inventory.wood = 60;
    save.inventory.stone = 30;
    save.inventory.berries = 20;
    save.inventory.reeds = 12;
    save.inventory.leather = 8;
    save.coins = 60;
  }
  const bool fixture =
      options.benchmark_seconds > 0 || !options.preview.empty();
  const bool legacy =
      !options.no_save && std::filesystem::exists(options.save_path);
  if (!legacy && !fixture && !options.spawn && !options.seed_explicit) {
    save.seed = (static_cast<uint64_t>(std::random_device{}()) << 32) ^
                std::random_device{}();
  }
  frontier::ConfigureWorld(save.seed);
  frontier::EnsureCityState(save);
  if (!fixture) {
    frontier::InitializeCampaign(save, legacy);
  }
  if (!fixture) {
    frontier::InitializeAdventure(save);
    frontier::InitializeNation(save);
  }
  if (options.spawn) {
    save.player = *options.spawn;
  }
  if (options.benchmark_expedition) {
    save.coins = 10000;
    save.inventory.wood = 200;
    save.inventory.stone = 100;
    save.inventory.reeds = 100;
    save.inventory.fish = 100;
    for (int city = 0; city < 9; city++) {
      if (save.relations[city].treaty == frontier::Treaty::kWar) {
        frontier::Diplomacy(save, city, 1);
      }
      while (save.relations[city].opinion < 20) {
        frontier::Diplomacy(save, city, 0);
      }
      if (!save.technologies.contains(
              frontier::GetContent()
                  .towns[frontier::Settlements()[city].type]
                  .technology)) {
        frontier::LearnTechnology(save, city);
      }
      for (int good : frontier::GetContent()
                          .towns[frontier::Settlements()[city].type]
                          .goods) {
        while (save.stocks[city][good] > 0) {
          if (!frontier::Trade(save, city, good, true).ok) {
            break;
          }
        }
      }
    }
    frontier::Diplomacy(save, 0, 0);
    frontier::Diplomacy(save, 0, 3);
    while (save.relations[0].opinion < 70) {
      if (!frontier::Diplomacy(save, 0, 8).ok) {
        break;
      }
    }
    frontier::Diplomacy(save, 0, 4);
    frontier::Diplomacy(save, 0, 6);
    frontier::Diplomacy(save, 0, 6);
  }
  if (!options.preview.empty()) {
    for (auto& type : frontier::GetContent().towns) {
      save.technologies.insert(type.technology);
    }
    save.inventory.rare.fill(50);
    save.coins = 500;
    save.inventory.sword = true;
    save.inventory.weapons.insert("wood_sword");
    save.inventory.weapon_id = "wood_sword";
    if (options.preview == "ship" || options.preview == "submarine" ||
        options.preview == "raft" || options.preview == "canoe" ||
        options.preview == "skiff" || options.preview == "cutter" ||
        options.preview == "brigantine" || options.preview == "flagship") {
      save.inventory.boat = true;
      save.inventory.ship = true;
      save.inventory.submarine = true;
      save.inventory.sonar = true;
      save.vehicle = options.preview == "ship" ? 2
                     : options.preview == "submarine"
                         ? 3
                         : frontier::Vessel(options.preview).number;
      if (save.vehicle > 3) {
        save.vessels.insert(options.preview);
      }
      save.submerged = options.preview == "submarine";
      if (!options.spawn) {
        save.player = {1100, -1100};
      }
    }
    if (options.preview == "weapons") {
      for (auto& item : frontier::GetContent().weapons) {
        save.inventory.weapons.insert(item.id);
      }
    }
    if (options.preview == "quests" || options.preview == "skills" ||
        options.preview == "clothes" || options.preview == "residents") {
      frontier::InitializeCampaign(save, true);
      frontier::InitializeAdventure(save);
      for (auto& item : frontier::GetContent().outfits) {
        save.outfit_copies[item.id] = 1;
      }
      if (options.preview == "clothes") {
        frontier::EquipOutfit(save, "forager");
      }
    }
  }
  if (!IsWindowReady()) {
    SetTraceLogLevel(LOG_INFO);
    SetTraceLogCallback(StartupTrace);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(options.width, options.height, "Forest Frontier");
    SetTraceLogLevel(LOG_WARNING);
    SetTraceLogCallback(nullptr);
  }
  if (!IsWindowReady()) {
    throw std::runtime_error("Cannot create window / OpenGL context");
  }
  SetWindowMinSize(900, 650);
  SetExitKey(KEY_NULL);
  SetTargetFPS(options.uncapped ? 0 : options.fps);
  if (options.fullscreen != IsWindowFullscreen()) {
    ToggleGameFullscreen();
  }
  {
    frontier::Renderer renderer;
    frontier::World world(save.seed);
    frontier::Gameplay game;
    frontier::Dialogue dialogue(options.port,
                                options.benchmark_llm && options.dialogues);
    std::vector<std::vector<frontier::Message>> history(world.npcs.size());
    std::vector<std::string> replies(world.npcs.size());
    frontier::Ui ui;
    ui.debug = options.debug;
    ui.temporary_session = options.no_save;
    ui.benchmark = options.benchmark_seconds > 0;
    ui.summary.caption = "Live world · includes current unsaved progress";
    if (options.preview == "skills") {
      ui.progression = true;
      ui.selection = static_cast<int>(frontier::Skills().size());
    }
    if (options.preview == "clothes") {
      ui.wardrobe = true;
    }
    if (options.preview == "quests" || options.preview == "residents") {
      ui.npc = 0;
      world.BeginConversation(0, save);
      frontier::AcceptQuest(save, world.npcs[0]);
    }
    if (options.preview == "craft") {
      ui.crafting = true;
    }
    if (options.preview == "trade") {
      ui.trading = true;
      ui.merchant = 0;
    }
    if (options.preview == "city") {
      ui.civic = true;
      ui.merchant = 0;
    }
    if (options.preview == "weapons") {
      ui.equipment = true;
    }
    if (options.preview == "map") {
      ui.cartography = true;
      ui.map_center = save.player;
      ui.map_range = 2500;
    }
    if (options.preview == "talk") {
      ui.npc = 0;
      world.BeginConversation(0, save);
    }
    if (options.preview == "ruler") {
      ui.npc = 1;
      world.BeginConversation(1, save);
    }
    bool community_assigned = false;
    if (options.benchmark_gameplay) {
      frontier::InitializeCampaign(save, true);
      frontier::InitializeAdventure(save);
      frontier::InitializeTowns(save);
      for (const auto& f : frontier::FacilityTypes()) {
        bool present = std::any_of(
            save.facilities.begin(), save.facilities.end(),
            [&](const auto& site) {
              return site.city == save.home_city && site.kind == f.id;
            });
        if (!present) {
          save.facilities.push_back(
              {save.home_city, f.id,
               frontier::Settlements()[save.home_city].pos +
                   frontier::Vec{310, 0}});
        }
      }
    }
    if (options.benchmark_community) {
      frontier::InitializeCampaign(save, true);
      frontier::InitializeAdventure(save);
      auto& giver = world.npcs[0];
      auto accepted = frontier::AcceptQuest(save, giver);
      auto completed = frontier::FinishQuest(save, giver);
      int clothing = -1;
      for (int i = 0; i < static_cast<int>(frontier::Recipes().size()); i++) {
        if (frontier::Recipes()[i].id == "outfit_forager") {
          clothing = i;
        }
      }
      bool prepared = accepted.ok && completed.ok && clothing >= 0;
      if (clothing >= 0) {
        prepared = FixtureCraft(save, clothing).ok && prepared;
        prepared = FixtureCraft(save, clothing).ok && prepared;
      }
      prepared = FixtureCraft(save, 2).ok && prepared;
      prepared = FixtureCraft(save, 2).ok && prepared;
      for (int i = 0; i < 6; i++) {
        prepared =
            frontier::Trade(save, save.home_city, 5, true).ok && prepared;
      }
      auto research_position = save.player;
      save.player = frontier::Settlements()[save.home_city].pos;
      prepared = frontier::Research(save, 0).ok && prepared;
      save.player = research_position;
      prepared = frontier::AssignCitizen(save, save.citizens[0].npc, "gatherer",
                                         "", "forager")
                     .ok &&
                 prepared;
      perf.Event({{"event", "benchmark_community"},
                  {"ok", prepared},
                  {"quests", save.quests.size()},
                  {"residents", save.citizens.size()}});
    }
    Camera2D camera{};
    camera.zoom = options.zoom;
    float desired_zoom = save.submerged ? .85f : options.zoom;
    bool last_layer = save.submerged;
    double next_explore = 0;
    frontier::Coord previous{INT32_MAX, INT32_MAX};
    int previous_radius = -1;
    double sim_accumulator = 0, simulation_time = 0, last_flush = 0,
           last_save = 0, notice_until = 0, last_llm = -20, next_plan = 0;
    std::vector<frontier::Vec> route_path;
    size_t path_index = 0;
    double path_ms_total = 0, path_ms_max = 0;
    int plans = 0;
    bool spawn_ready = false;
    int scenario_step = 0;
    float gathering = 0;
    uint64_t gathering_id = 0;
    int frame_count = 0, route_index = 0;
    double pause_until = 5;
    frontier::Clock::time_point session_start = frontier::Clock::now(),
                                last_presentation = session_start;
    std::vector<frontier::Vec> route = {
        frontier::Vec{120, 140}, frontier::Vec{2120, 840},
        frontier::Vec{2750, 1000}, frontier::Vec{-1000, -900},
        frontier::Vec{120, 140}};
    if (options.benchmark_gameplay) {
      route = {frontier::Vec{120, 140},    frontier::Vec{2120, 840},
               frontier::Vec{2200, -450},  frontier::Vec{1100, -1100},
               frontier::Vec{1100, -1900}, frontier::Vec{120, 140}};
    }
    auto context = [&] {
      size_t animals = 0, resources = 0, enemies = 0, active_enemies = 0;
      for (auto& [coord, c] : world.chunks) {
        animals += c.animals.size();
        resources += c.resources.size();
        for (auto& enemy : c.enemies) {
          if (!save.defeated.contains(enemy.id)) {
            enemies++;
            if (frontier::Length(enemy.home - save.player) <= 1300) {
              active_enemies++;
            }
          }
        }
      }
      return nlohmann::json{
          {"chunks", world.chunks.size()},
          {"pending_chunks", world.Pending()},
          {"visible_objects", ui.visible},
          {"resident_animals", animals},
          {"resident_resources", resources},
          {"resident_enemies", enemies},
          {"active_enemies", active_enemies},
          {"player", {save.player.x, save.player.y}},
          {"zoom", camera.zoom},
          {"width", GetScreenWidth()},
          {"height", GetScreenHeight()},
          {"framebuffer_width", GetRenderWidth()},
          {"framebuffer_height", GetRenderHeight()},
          {"chunk_generation_count", world.generated_count},
          {"chunk_generation_ms_total", world.generated_ms},
          {"chunk_generation_ms_max", world.max_generation_ms},
          {"harvested", save.harvested.size()},
          {"route", ui.benchmark},
          {"llm_busy", dialogue.Snapshot().busy},
          {"health", save.health},
          {"coins", save.coins},
          {"attacks", game.attacks},
          {"kills", game.kills},
          {"damage_taken", game.damage_taken},
          {"respawns", game.respawns},
          {"collision_frames", game.collisions},
          {"path_plans", plans},
          {"path_ms_total", path_ms_total},
          {"path_ms_max", path_ms_max},
          {"biome",
           frontier::TerrainName(frontier::TerrainAt(save.player, save.seed))},
          {"submerged", save.submerged},
          {"audio_ready", renderer.AudioReady()},
          {"music_mode", frontier::ActiveSettings().music_mode},
          {"music_volume", frontier::ActiveSettings().music_volume},
          {"effects_volume", frontier::ActiveSettings().effects_volume},
          {"map_notes", save.map_notes.size()},
          {"map_open", ui.cartography},
          {"map_overlay", ui.map_overlay},
          {"new_vessels", save.vessels.size()},
          {"visible_surface_cells", save.visible_surface.size()},
          {"known_surface_cells", save.explored.size()},
          {"known_underwater_cells", save.explored_underwater.size()},
          {"home_city", save.home_city},
          {"skills", save.skills.size()},
          {"gathered", save.gathered},
          {"crafted", save.crafted},
          {"game_time", save.game_time},
          {"craft_wait", frontier::CraftWait(save)},
          {"activity_buildings", save.facilities.size()},
          {"built_roads", save.roads.size()},
          {"tribute_towns", save.tributes.size()},
          {"active_residents", frontier::ActiveResidents(save)},
          {"paused", ui.paused || ui.progression},
          {"summary_tab", ui.progression ? ui.summary.tab : -1},
          {"owned_cities",
           std::count(save.owners.begin(), save.owners.end(), 0)},
          {"quests", save.quests.size()},
          {"resident_jobs", save.citizens.size()},
          {"controlled_resident", save.controlled_npc},
          {"production_jobs", save.production.size()},
          {"production_time", save.production_time},
          {"money_total", frontier::MoneySupply(save)},
          {"outfit", save.outfit_id},
          {"walls", save.walls.size()},
          {"technologies", save.technologies.size()},
          {"weapons", save.inventory.weapons.size()},
          {"oxygen", save.oxygen},
          {"hull", save.hull}};
    };
    perf.Event({{"event", "session_start"},
                {"cpu", CpuName()},
                {"seed", save.seed},
                {"width", options.width},
                {"height", options.height},
                {"graphics", graphics_info},
                {"fps_limit", options.uncapped ? 0 : options.fps},
                {"benchmark_s", options.benchmark_seconds},
                {"benchmark_llm", options.benchmark_llm},
                {"llm_port", options.port},
                {"raylib", RAYLIB_VERSION},
                {"benchmark_expedition", options.benchmark_expedition},
                {"npc_types", frontier::GetContent().npcs.size()},
                {"weapon_types", frontier::GetContent().weapons.size()},
                {"settlements", frontier::Settlements().size()},
                {"recipes", frontier::Recipes().size()},
                {"build", "Release recommended"},
                {"timing_note",
                 "render_submit is CPU only; present_wait includes GPU/driver "
                 "and FPS limiter"}});
    std::cout << "Performance log: " << perf.Path() << '\n';
    auto notify = [&](std::string s, double now) {
      ui.notice = std::move(s);
      notice_until = now + 3;
    };
    auto ask = [&](int id, const std::string& input, double now) {
      if (!options.dialogues) {
        notify("NPC dialogue is disabled in settings", now);
        return;
      }
      if (id < 0 || input.empty() || dialogue.Snapshot().busy) {
        return;
      }
      auto messages = history[id];
      while (messages.size() > 4) {
        messages.erase(messages.begin(), messages.begin() + 2);
      }
      messages.push_back({"user", input});
      if (dialogue.Ask(id, NpcPrompt(world.npcs[id], save), messages)) {
        history[id] = std::move(messages);
        replies[id].clear();
        ui.input.clear();
        last_llm = now;
        perf.Event({{"event", "llm_request"},
                    {"npc_id", id},
                    {"history_messages", history[id].size()},
                    {"max_tokens", 128}});
      }
    };
    frontier::Settings live_settings = frontier::ActiveSettings();
    frontier::SettingsPanel settings_panel;
    bool quit = false, saved_for_exit = false;
    auto production_updated = frontier::Clock::now();
    while (!quit && !WindowShouldClose()) {
      const double now = frontier::ElapsedMilliseconds(session_start) / 1000;
      float dt = std::clamp(GetFrameTime(), .00001f, .05f);
      const auto update_start = frontier::Clock::now();
      const double production_elapsed =
          std::chrono::duration<double>(update_start - production_updated)
              .count();
      production_updated = update_start;
      const int completed_items =
          frontier::TickProduction(save, production_elapsed);
      if (completed_items > 0) {
        ui.gov_status =
            "Production finished: " + std::to_string(completed_items) +
            " item(s) added to city resources";
        perf.Event(
            {{"event", "production_finished"}, {"items", completed_items}});
      }
      game.moving = false;
      if (now > notice_until) {
        ui.notice.clear();
      }
      if (!ui.pending_notice.empty()) {
        notify(ui.pending_notice, now);
        ui.pending_notice.clear();
      }
      frontier::Reply completed;
      if (dialogue.TakeCompletion(completed)) {
        if (completed.error.empty()) {
          history[completed.npc_id].push_back({"assistant", completed.text});
          while (history[completed.npc_id].size() > 8) {
            history[completed.npc_id].erase(
                history[completed.npc_id].begin(),
                history[completed.npc_id].begin() + 2);
          }
          replies[completed.npc_id] = completed.text;
        } else {
          if (!history[completed.npc_id].empty() &&
              history[completed.npc_id].back().role == "user") {
            history[completed.npc_id].pop_back();
          }
          replies[completed.npc_id] =
              "No reply received. Check the local model server and try again.";
        }
        perf.Event(
            {{"event", "llm_result"},
             {"npc_id", completed.npc_id},
             {"ok", completed.error.empty()},
             {"error", completed.error},
             {"total_ms", completed.total_ms},
             {"first_token_ms", completed.first_token_ms},
             {"prompt_tokens", completed.prompt_tokens},
             {"output_tokens", completed.output_tokens},
             {"prompt_ms", completed.prompt_ms},
             {"generation_ms", completed.generation_ms},
             {"output_tokens_per_s",
              completed.generation_ms > 0
                  ? completed.output_tokens * 1000 / completed.generation_ms
                  : 0}});
      }
      const bool discovery_was_open = !ui.discovered_towns.empty();
      const bool production_typing =
          ui.government && ui.gov_tab == 2 && ui.production_search_focus;
      if (!discovery_was_open && !production_typing && !ui.settings_open &&
          !ui.map_journal && !ui.road_edit &&
          frontier::ActionPressed(frontier::InputAction::kDebug)) {
        ui.debug = !ui.debug;
      }
      if (!discovery_was_open && !production_typing && !ui.settings_open &&
          frontier::ActionPressed(frontier::InputAction::kFullscreen)) {
        ToggleGameFullscreen();
      }
      const bool settings_was_open = ui.settings_open;
      const bool pause_was_active =
          discovery_was_open || ui.paused || ui.progression;
      if (frontier::ActionPressed(frontier::InputAction::kAutoroute) &&
          !ui.Blocked()) {
        ui.benchmark = !ui.benchmark;
        pause_until = now;
        route_index = 0;
        route_path.clear();
        path_index = 0;
        next_plan = 0;
      }
      if (save.nation && !discovery_was_open && ui.request_control >= 0) {
        auto result = frontier::SelectResident(save, ui.request_control);
        notify(result.message, now);
        ui.request_control = -1;
        if (result.ok) {
          game = frontier::Gameplay{};
          sim_accumulator = 0;
          previous = {INT32_MAX, INT32_MAX};
          spawn_ready = false;
          auto target = frontier::Project(save.player);
          camera.target = {target.x, target.y};
          ui.government = false;
          frontier::SyncNationActors(world, save);
        }
      }
      if (save.nation && !discovery_was_open && !production_typing &&
          !ui.settings_open && !ui.map_journal && !ui.road_edit) {
        bool state_key =
            frontier::ActionPressed(frontier::InputAction::kNation);
        bool resident_key =
            frontier::ActionPressed(frontier::InputAction::kResidents);
        bool inventory_key =
            frontier::ActionPressed(frontier::InputAction::kInventory);
        bool projects_key =
            frontier::ActionPressed(frontier::InputAction::kQuests) ||
            (ui.npc < 0 &&
             frontier::ActionPressed(frontier::InputAction::kQuestAction));
        bool production_key =
            frontier::ActionPressed(frontier::InputAction::kCraft);
        bool hall_key = frontier::ActionPressed(frontier::InputAction::kHall);
        if (state_key || resident_key || inventory_key || projects_key ||
            production_key || hall_key) {
          int local = frontier::CityAt(save, save.player);
          bool near_hall =
              local >= 0 &&
              frontier::Length(save.player -
                               (frontier::Settlements()[local].pos +
                                frontier::Vec{55, 55})) < 90;
          if (hall_key && !near_hall) {
            notify("Approach a town hall entrance", now);
          } else {
            frontier::StoreControlled(save);
            ui.government = !ui.government || resident_key || inventory_key ||
                            projects_key || production_key || hall_key;
            ui.town_hall = hall_key;
            ui.production_search_focus = false;
            ui.gov_city = local >= 0 ? local : save.home_city;
            ui.gov_tab = resident_key               ? 0
                         : inventory_key            ? 1
                         : production_key           ? 2
                         : projects_key || hall_key ? 3
                                                    : 5;
            ui.gov_page = 0;
            ui.gov_status.clear();
            ui.paused = ui.progression = ui.crafting = ui.trading = ui.civic =
                ui.equipment = ui.cartography = ui.wardrobe = ui.journal =
                    false;
            ui.npc = -1;
            world.talking_npc = -1;
          }
        }
      }
      if (frontier::MenuBack() && !ui.settings_open) {
        if (discovery_was_open) {
          perf.Event({{"event", "discovery_dismissed"},
                      {"city", ui.discovered_towns.front()}});
          ui.discovered_towns.erase(ui.discovered_towns.begin());
        } else if (ui.government) {
          ui.government = false;
          if (save.nation && !frontier::HasControlled(save)) {
            ui.paused = true;
            ui.pause_selection = 0;
          }
        } else if (ui.road_edit) {
          ui.road_edit = false;
          ui.road_points.clear();
        } else if (ui.map_journal) {
          ui.map_journal = false;
          ui.note_edit = false;
        } else if (ui.progression) {
          ui.summary.requested_node.clear();
          ui.summary.requested_assignment = false;
          ui.summary.requested_ransom = -1;
          ui.progression = false;
        } else if (ui.paused) {
          ui.paused = false;
        } else if (ui.Blocked()) {
          ui.npc = -1;
          ui.input.clear();
          ui.crafting = false;
          ui.trading = false;
          ui.civic = false;
          ui.equipment = false;
          ui.cartography = false;
          ui.progression = ui.wardrobe = ui.journal = false;
          world.talking_npc = -1;
        } else {
          ui.paused = true;
          ui.pause_selection = 0;
          ui.pause_error.clear();
        }
      }
      if (!discovery_was_open && !production_typing && !ui.settings_open &&
          !ui.map_journal && !ui.road_edit &&
          frontier::ActionPressed(frontier::InputAction::kProgress)) {
        ui.government = false;
        bool opened = !ui.progression;
        ui.npc = -1;
        world.talking_npc = -1;
        ui.crafting = ui.trading = ui.civic = ui.equipment = ui.cartography =
            ui.wardrobe = ui.journal = false;
        ui.paused = false;
        ui.progression = opened;
        ui.summary.requested_node.clear();
        ui.summary.requested_assignment = false;
        ui.summary.requested_ransom = -1;
        ui.summary.editable = true;
        if (opened) {
          ui.summary.tab = 4;
          ui.summary.status.clear();
        }
      }
      if (ui.progression) {
        renderer.SummaryInput(ui.summary,
                              frontier::Renderer::ProgressSummaryBounds());
        if (!ui.summary.requested_node.empty()) {
          auto result =
              frontier::UnlockProgress(save, ui.summary.requested_node);
          ui.summary.status = result.message;
          perf.Event({{"event", "progress_unlock"},
                      {"id", ui.summary.requested_node},
                      {"ok", result.ok}});
          ui.summary.requested_node.clear();
        }
        if (ui.summary.requested_ransom >= 0) {
          ui.summary.status =
              frontier::RansomResident(save, ui.summary.requested_ransom)
                  .message;
          ui.summary.requested_ransom = -1;
        }
        if (ui.summary.requested_assignment) {
          auto weapons = frontier::OwnedWeapons(save),
               outfits = frontier::OwnedOutfits(save);
          outfits.insert(outfits.begin(), "");
          auto result = frontier::AssignCitizen(
              save, ui.summary.selected_resident,
              frontier::ResidentJobs().at(ui.summary.resident_job),
              weapons.at(ui.summary.resident_weapon),
              outfits.at(ui.summary.resident_outfit));
          ui.summary.status = result.message;
          ui.summary.requested_assignment = false;
          ui.summary.requested_ransom = -1;
          perf.Event({{"event", "resident_assignment"},
                      {"npc", ui.summary.selected_resident},
                      {"ok", result.ok}});
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(),
                                   frontier::Renderer::ProgressCloseButton())) {
          ui.progression = false;
          ui.summary.requested_node.clear();
          ui.summary.requested_assignment = false;
          ui.summary.requested_ransom = -1;
        }
      }
      if (ui.paused && !ui.settings_open) {
        if (frontier::MenuUp()) {
          ui.pause_selection = (ui.pause_selection + 3) % 4;
        }
        if (frontier::MenuDown()) {
          ui.pause_selection = (ui.pause_selection + 1) % 4;
        }
        bool activate = frontier::MenuConfirm();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          for (int i = 0; i < 4; ++i) {
            if (CheckCollisionPointRec(GetMousePosition(),
                                       frontier::Renderer::PauseButton(i))) {
              ui.pause_selection = i;
              activate = true;
            }
          }
        }
        if (activate) {
          if (ui.pause_selection == 0) {
            ui.paused = false;
          } else if (ui.pause_selection == 3) {
            settings_panel = {};
            settings_panel.draft = live_settings;
            ui.settings_open = true;
          } else {
            try {
              auto started = frontier::Clock::now();
              if (!options.no_save) {
                frontier::WriteSave(options.save_path, save);
                last_save = now;
              }
              ui.pause_error = options.no_save
                                   ? "Temporary world: no save file."
                                   : "World saved successfully.";
              if (ui.pause_selection == 2) {
                saved_for_exit = true;
                quit = true;
              }
              perf.Event(
                  {{"event",
                    ui.pause_selection == 2 ? "quit_to_menu" : "manual_save"},
                   {"saved", !options.no_save},
                   {"duration_ms", frontier::ElapsedMilliseconds(started)}});
            } catch (const std::exception& error) {
              ui.pause_error = "Could not save: " + std::string(error.what()) +
                               ". Your session is still open. Retry after "
                               "fixing the save path.";
              perf.Event({{"event", "save_failed"},
                          {"reason", "pause"},
                          {"error", error.what()}});
            }
          }
        }
      }
      const frontier::Resource* resource = nullptr;
      if (ui.discovered_towns.empty() && !ui.government && !ui.paused &&
          !ui.progression && !ui.map_journal && !ui.road_edit && !quit &&
          !pause_was_active) {
        game.Advance(dt);
        if (ui.cartography) {
          ui.map_range = std::clamp(
              ui.map_range * (1 - GetMouseWheelMove() * .12f), 900.f, 18000.f);
          if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            auto delta = GetMouseDelta();
            ui.map_center =
                ui.map_center -
                frontier::Vec{delta.x, delta.y} *
                    (ui.map_range * 2 /
                     std::min(GetScreenWidth() - 120, GetScreenHeight() - 160));
          }
        }
        if ((ui.npc < 0 && !ui.crafting && !ui.trading && !ui.civic &&
             !ui.equipment && !ui.cartography && !ui.progression &&
             !ui.wardrobe && !ui.journal) ||
            ui.benchmark) {
          if (!ui.cartography) {
            desired_zoom = std::clamp(desired_zoom + GetMouseWheelMove() * .1f,
                                      .65f, 1.7f);
          }
          frontier::Vec move{};
          if (ui.benchmark) {
            if (now >= pause_until) {
              auto d = route[route_index] - save.player;
              if (frontier::Length(d) < 55) {
                route_index =
                    (route_index + 1) % static_cast<int>(route.size());
                pause_until = now + 5;
                route_path.clear();
                path_index = 0;
                next_plan = 0;
              } else {
                if ((now >= next_plan || (path_index >= route_path.size() &&
                                          !route_path.empty())) &&
                    spawn_ready) {
                  auto t = frontier::Clock::now();
                  route_path =
                      world.Path(save.player, route[route_index], save);
                  path_index = 0;
                  double cost = frontier::ElapsedMilliseconds(t);
                  path_ms_total += cost;
                  path_ms_max = std::max(path_ms_max, cost);
                  plans++;
                  next_plan = now + 2;
                }
                while (path_index < route_path.size() &&
                       frontier::Length(route_path[path_index] - save.player) <
                           10) {
                  path_index++;
                }
                frontier::Vec direction =
                    path_index < route_path.size()
                        ? route_path[path_index] - save.player
                        : frontier::Vec{};
                move = frontier::Normalized(direction) *
                       std::min(frontier::Length(direction), dt * 480);
              }
            }
          } else {
            frontier::Vec screen{
                static_cast<float>(
                    frontier::ActionDown(frontier::InputAction::kRight) -
                    frontier::ActionDown(frontier::InputAction::kLeft)),
                static_cast<float>(
                    frontier::ActionDown(frontier::InputAction::kDown) -
                    frontier::ActionDown(frontier::InputAction::kUp))};
            if (frontier::Length(screen) == 0 && IsGamepadAvailable(0)) {
              screen = {GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X),
                        GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y)};
              if (frontier::Length(screen) < .2f) {
                screen = {};
              }
            }
            move = frontier::Normalized(frontier::Unproject(screen)) *
                   (dt * (frontier::ActionDown(frontier::InputAction::kSprint)
                              ? 340.f
                              : 185.f));
          }
          if (!save.nation || frontier::HasControlled(save)) {
            game.MovePlayer(world, save, move);
          }
          if (frontier::Length(save.player) > 1e6f) {
            save.player = frontier::Normalized(save.player) * 1e6f;
          }
        }
        int nearest_npc = -1;
        float npc_distance = 150;
        for (auto& n : world.npcs) {
          if (n.ruler || (save.nation && n.id == save.controlled_npc) ||
              !frontier::ResidentActive(save, n.id)) {
            continue;
          }
          float d = frontier::Length(n.pos - save.player);
          if (d < npc_distance) {
            npc_distance = d;
            nearest_npc = n.id;
          }
        }
        int nearby_hall = -1;
        for (size_t city = 0; city < frontier::Settlements().size(); city++) {
          if (frontier::Length(save.player -
                               (frontier::Settlements()[city].pos +
                                frontier::Vec{55, 55})) < 90) {
            nearby_hall = static_cast<int>(city);
          }
        }
        if (ui.npc < 0 && !ui.crafting && !ui.trading && !ui.civic &&
            !ui.equipment && !ui.cartography && !ui.progression &&
            !ui.wardrobe && !ui.journal &&
            frontier::ActionPressed(frontier::InputAction::kTalk) &&
            nearest_npc >= 0) {
          ui.npc = nearest_npc;
          world.BeginConversation(ui.npc, save);
          ui.input.clear();
          ui.resident_job = ui.resident_weapon = ui.resident_outfit = 0;
          perf.Event({{"event", "conversation_started"},
                      {"npc", ui.npc},
                      {"city", world.npcs[ui.npc].settlement}});
          if (auto resident = frontier::GetCitizen(save, ui.npc)) {
            auto weapons = frontier::OwnedWeapons(save);
            auto clothes = frontier::OwnedOutfits(save);
            clothes.insert(clothes.begin(), "");
            auto job = std::find(frontier::ResidentJobs().begin(),
                                 frontier::ResidentJobs().end(), resident->job);
            ui.resident_job =
                static_cast<int>(job - frontier::ResidentJobs().begin());
            auto wi =
                std::find(weapons.begin(), weapons.end(), resident->weapon);
            if (wi != weapons.end()) {
              ui.resident_weapon = static_cast<int>(wi - weapons.begin());
            }
            auto oi =
                std::find(clothes.begin(), clothes.end(), resident->outfit);
            if (oi != clothes.end()) {
              ui.resident_outfit = static_cast<int>(oi - clothes.begin());
            }
          }
          while (GetCharPressed()) {
          }
        } else if (ui.npc >= 0) {
          auto& npc = world.npcs[ui.npc];
          float pw = std::min(720.f, GetScreenWidth() - 50.f),
                py = GetScreenHeight() - 500.f - 96,
                px = (GetScreenWidth() - pw) * .5f;
          if (!save.nation &&
              (frontier::ActionPressed(frontier::InputAction::kQuestAction) ||
               (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                CheckCollisionPointRec(GetMousePosition(),
                                       {px + 20, py + 269, pw - 40, 32})))) {
            auto found = save.quests.find(npc.id);
            auto result = found == save.quests.end()
                              ? frontier::AcceptQuest(save, npc)
                              : frontier::FinishQuest(save, npc);
            notify(result.message, now);
            perf.Event(
                {{"event", "quest"}, {"npc", npc.id}, {"ok", result.ok}});
          }
          if (frontier::ActionPressed(frontier::InputAction::kTeacher)) {
            int city = npc.settlement;
            auto& type = frontier::GetContent()
                             .towns[frontier::Settlements()[city].type];
            if (frontier::GetContent().npcs[npc.type].id != type.teacher) {
              notify("Find the town master or visit a town hall", now);
            } else {
              notify(frontier::LearnTechnology(save, city).message, now);
            }
          }
          if (frontier::GetCitizen(save, npc.id)) {
            auto weapons = frontier::OwnedWeapons(save);
            auto clothes = frontier::OwnedOutfits(save);
            clothes.insert(clothes.begin(), "");
            if (frontier::ActionPressed(frontier::InputAction::kResidentJob)) {
              ui.resident_job =
                  (ui.resident_job + 1) %
                  static_cast<int>(frontier::ResidentJobs().size());
            }
            if (frontier::ActionPressed(
                    frontier::InputAction::kResidentWeapon)) {
              ui.resident_weapon =
                  (ui.resident_weapon + 1) % static_cast<int>(weapons.size());
            }
            if (frontier::ActionPressed(
                    frontier::InputAction::kResidentOutfit)) {
              ui.resident_outfit =
                  (ui.resident_outfit + 1) % static_cast<int>(clothes.size());
            }
            ui.resident_weapon = std::clamp(
                ui.resident_weapon, 0, static_cast<int>(weapons.size()) - 1);
            ui.resident_outfit = std::clamp(
                ui.resident_outfit, 0, static_cast<int>(clothes.size()) - 1);
            if (frontier::ActionPressed(
                    frontier::InputAction::kResidentAssign)) {
              notify(
                  frontier::AssignCitizen(
                      save, npc.id, frontier::ResidentJobs()[ui.resident_job],
                      weapons[ui.resident_weapon], clothes[ui.resident_outfit])
                      .message,
                  now);
            }
          }
        }
        if (ui.npc < 0) {
          if (frontier::ActionPressed(frontier::InputAction::kOutfits) ||
              (!save.nation &&
               frontier::ActionPressed(frontier::InputAction::kQuests))) {
            bool progress = false,
                 clothes =
                     frontier::ActionPressed(frontier::InputAction::kOutfits);
            ui.progression = progress && !ui.progression;
            ui.wardrobe = clothes && !ui.wardrobe;
            ui.journal = !progress && !clothes && !ui.journal;
            ui.crafting = ui.trading = ui.civic = ui.equipment =
                ui.cartography = false;
            ui.selection = 0;
          }
          if (frontier::ActionPressed(frontier::InputAction::kMap)) {
            ui.cartography = !ui.cartography;
            ui.map_center = save.player;
            ui.map_range = std::max(900.f, frontier::SightRange(save) * 1.6f);
            ui.crafting = ui.trading = ui.civic = ui.equipment =
                ui.progression = ui.wardrobe = ui.journal = false;
          }
          if (frontier::ActionPressed(frontier::InputAction::kWeapons)) {
            ui.equipment = !ui.equipment;
            ui.crafting = ui.trading = ui.civic = ui.cartography =
                ui.progression = ui.wardrobe = ui.journal = false;
            ui.selection = 0;
          }
          if (!save.nation &&
              frontier::ActionPressed(frontier::InputAction::kCraft)) {
            ui.crafting = !ui.crafting;
            ui.trading = ui.civic = ui.equipment = ui.cartography =
                ui.progression = ui.wardrobe = ui.journal = false;
            ui.selection = 0;
          }
          if (!save.nation &&
              frontier::ActionPressed(frontier::InputAction::kHall)) {
            if (nearby_hall < 0) {
              notify("Approach a town hall entrance", now);
            } else {
              ui.civic = !ui.civic;
              ui.merchant = nearby_hall;
              ui.crafting = ui.trading = ui.equipment = ui.cartography =
                  ui.progression = ui.wardrobe = ui.journal = false;
              ui.selection = save.skills.empty() && save.campaign ? 10 : 0;
            }
          }
          if (frontier::ActionPressed(frontier::InputAction::kTrade)) {
            if (ui.trading) {
              ui.trading = false;
            } else if (nearest_npc >= 0) {
              ui.trading = true;
              ui.crafting = ui.civic = ui.equipment = ui.cartography =
                  ui.progression = ui.wardrobe = ui.journal = false;
              ui.merchant = world.npcs[nearest_npc].settlement;
              ui.selection = 0;
            } else {
              notify("Approach a town resident to trade", now);
            }
          }
        }
        if (ui.crafting || ui.trading || ui.civic || ui.equipment ||
            ui.wardrobe || ui.journal) {
          auto weapons = frontier::OwnedWeapons(save);
          auto clothes = frontier::OwnedOutfits(save);
          std::vector<int> quests;
          for (auto& [id, q] : save.quests) {
            quests.push_back(id);
          }
          std::sort(quests.begin(), quests.end());
          if (quests.empty()) {
            quests.push_back(-1);
          }
          int rows =
              ui.crafting  ? static_cast<int>(frontier::Recipes().size())
              : ui.trading ? frontier::kGoodTypes
              : ui.civic   ? static_cast<int>(frontier::CityActions().size())
              : ui.progression
                  ? static_cast<int>(frontier::Skills().size() +
                                     frontier::ResearchNodes().size())
              : ui.wardrobe ? std::max(1, static_cast<int>(clothes.size()))
              : ui.journal  ? static_cast<int>(quests.size())
                            : static_cast<int>(weapons.size());
          int page_size = frontier::Renderer::MenuPageRows(),
              shown = std::min(rows, page_size),
              page = (ui.selection / page_size) * page_size;
          if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
            ui.selection = (ui.selection + rows - 1) % rows;
          }
          if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
            ui.selection = (ui.selection + 1) % rows;
          }
          if (IsKeyPressed(KEY_PAGE_DOWN)) {
            ui.selection = std::min(rows - 1, ui.selection + page_size);
          }
          if (IsKeyPressed(KEY_PAGE_UP)) {
            ui.selection = std::max(0, ui.selection - page_size);
          }
          ui.selection =
              std::clamp(ui.selection - static_cast<int>(GetMouseWheelMove()),
                         0, rows - 1);
          page = ui.selection / page_size * page_size;
          bool perform = IsKeyPressed(KEY_ENTER),
               buy = !IsKeyDown(KEY_LEFT_SHIFT);
          if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            auto box = frontier::Renderer::MenuBounds(shown);
            auto mouse = GetMousePosition();
            int row = static_cast<int>(std::floor(
                (mouse.y - box.y - frontier::Renderer::kMenuListTop) /
                frontier::Renderer::kMenuRowHeight));
            if (CheckCollisionPointRec(mouse, box) && row >= 0 && row < shown &&
                page + row < rows) {
              ui.selection = page + row;
              if (ui.trading) {
                float y = box.y + frontier::Renderer::kMenuListTop +
                          row * frontier::Renderer::kMenuRowHeight;
                bool buy_hit = CheckCollisionPointRec(
                    mouse, {box.x + box.width - 254, y - 2, 102, 29});
                bool sell_hit = CheckCollisionPointRec(
                    mouse, {box.x + box.width - 130, y - 2, 102, 29});
                if (buy_hit || sell_hit) {
                  perform = true;
                  buy = buy_hit;
                }
              }
            }
            if (!ui.trading &&
                CheckCollisionPointRec(
                    mouse,
                    {box.x + box.width - 274,
                     box.y + frontier::Renderer::kMenuListTop +
                         shown * frontier::Renderer::kMenuRowHeight + 108,
                     250, 40})) {
              perform = true;
            }
          }
          int lesson =
              ui.progression &&
                      ui.selection < static_cast<int>(frontier::Skills().size())
                  ? ui.selection
              : ui.civic && ui.selection >= 10 &&
                      ui.selection <
                          10 + static_cast<int>(frontier::Skills().size())
                  ? ui.selection - 10
                  : -1;
          if (perform && lesson >= 0 &&
              frontier::GetLessonState(save, lesson).learned) {
            perform = false;
          }
          if (perform && ui.civic && ui.selection == 9 &&
              !frontier::OwnsCity(save, ui.merchant)) {
            for (auto& n : world.npcs) {
              if (n.ruler && n.settlement == ui.merchant) {
                ui.npc = n.id;
                world.BeginConversation(n.id, save);
                ui.civic = false;
                ui.input.clear();
                break;
              }
            }
          } else if (perform) {
            frontier::Action result =
                ui.crafting ? frontier::Craft(save, ui.selection)
                : ui.trading
                    ? frontier::Trade(save, ui.merchant, ui.selection, buy)
                : ui.civic
                    ? frontier::Diplomacy(save, ui.merchant, ui.selection)
                : ui.progression
                    ? (ui.selection <
                               static_cast<int>(frontier::Skills().size())
                           ? frontier::TrainSkill(save, ui.selection)
                           : frontier::Research(
                                 save,
                                 ui.selection - static_cast<int>(
                                                    frontier::Skills().size())))
                : ui.wardrobe
                    ? (clothes.empty()
                           ? frontier::Action{false, "No clothes yet"}
                           : frontier::EquipOutfit(save, clothes[ui.selection]))
                : ui.journal
                    ? frontier::Action{quests[ui.selection] >= 0,
                                       quests[ui.selection] >= 0
                                           ? "Quest tracked"
                                           : "No quests yet"}
                    : frontier::EquipWeapon(save, weapons[ui.selection]);
            if (ui.journal && result.ok) {
              save.tracked_quest = quests[ui.selection];
            }
            notify(result.message, now);
            perf.Event({{"event", ui.crafting ? "craft" : "trade"},
                        {"item", ui.selection},
                        {"ok", result.ok},
                        {"coins", save.coins}});
          }
        } else if (!ui.Blocked()) {
          if (frontier::ActionPressed(frontier::InputAction::kAttack) ||
              (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
               GetMousePosition().y < ui.action_bar_top)) {
            auto result = game.Attack(world, save);
            if (!result.message.empty()) {
              notify(result.message, now);
            }
          }
          if (frontier::ActionPressed(frontier::InputAction::kHeal)) {
            notify(frontier::Consume(save, true).message, now);
          }
          if (frontier::ActionPressed(frontier::InputAction::kEat)) {
            notify(frontier::Consume(save, false).message, now);
          }
          if (frontier::ActionPressed(frontier::InputAction::kCampfire)) {
            notify(game.PlaceCampfire(world, save).message, now);
          }
          if (frontier::ActionPressed(frontier::InputAction::kVehicle)) {
            notify(frontier::CycleVehicle(save).message, now);
          }
          if (frontier::ActionPressed(frontier::InputAction::kDive)) {
            notify(frontier::Dive(save).message, now);
          }
          if (frontier::ActionPressed(frontier::InputAction::kRepair)) {
            notify(frontier::RepairVessel(save).message, now);
          }
          if (frontier::ActionPressed(frontier::InputAction::kSonar)) {
            if (game.sonar_cooldown > 0) {
              notify("Sonar recharging", now);
            } else {
              auto result = frontier::Sonar(save);
              notify(result.message, now);
              if (result.ok) {
                game.sonar_cooldown = 10;
              }
            }
          }
        }
        if (options.benchmark_llm && !ui.crafting && !ui.trading && !ui.civic &&
            !ui.equipment && !ui.cartography && !ui.progression &&
            !ui.wardrobe && !ui.journal) {
          if (nearest_npc < 0) {
            ui.npc = -1;
            world.talking_npc = -1;
          }
          if (nearest_npc >= 0 && now > 2 && now - last_llm > 15 &&
              !dialogue.Snapshot().busy) {
            ui.npc = nearest_npc;
            world.BeginConversation(ui.npc, save);
            ask(nearest_npc,
                "Give a two-sentence greeting based only on your stated role "
                "and lore. Do not invent a task or reward.",
                now);
          }
        }
        if (ui.npc >= 0 &&
            (!frontier::ResidentActive(save, world.npcs[ui.npc].id) ||
             frontier::Length(world.npcs[ui.npc].pos - save.player) > 190)) {
          ui.npc = -1;
          world.talking_npc = -1;
        }
        if (save.submerged != last_layer) {
          desired_zoom = save.submerged ? .85f : options.zoom;
          last_layer = save.submerged;
        }
        camera.zoom += (desired_zoom - camera.zoom) * std::min(1.f, dt * 6);
        auto iso = frontier::Project(save.player);
        camera.target = {iso.x, iso.y};
        camera.offset = {GetScreenWidth() * .5f, GetScreenHeight() * .5f};
        // Project viewport corners back onto the ground; keep a margin for tall
        // trees.
        int radius = 1;
        for (Vector2 corner : std::array<Vector2, 4>{
                 {{-180, -180},
                  {static_cast<float>(GetScreenWidth() + 180), -180},
                  {-180, static_cast<float>(GetScreenHeight() + 180)},
                  {static_cast<float>(GetScreenWidth() + 180),
                   static_cast<float>(GetScreenHeight() + 180)}}}) {
          auto p = GetScreenToWorld2D(corner, camera);
          auto d = frontier::Unproject({p.x, p.y}) - save.player;
          radius = std::max(radius, static_cast<int>(std::ceil(
                                        std::max(std::abs(d.x), std::abs(d.y)) /
                                        frontier::kChunkSize)) +
                                        1);
        }
        radius = std::min(radius, 5);
        frontier::Coord current = frontier::ChunkAt(save.player);
        if (current != previous || radius != previous_radius) {
          world.Request(save.player, radius);
          previous = current;
          previous_radius = radius;
        }
        world.Integrate();
        if (options.preview == "combat" && scenario_step == 0 &&
            world.chunks.contains(frontier::ChunkAt(save.player))) {
          frontier::Vec home = save.player + frontier::Vec{90, 0};
          world.chunks.at(frontier::ChunkAt(save.player))
              .enemies.push_back({frontier::Hash(0x123abc), home, home, home});
          scenario_step = 1;
        }
        if (!spawn_ready) {
          spawn_ready = game.RepairSpawn(world, save);
          if (spawn_ready && ui.npc >= 0) {
            world.BeginConversation(ui.npc, save);
          }
        }
        sim_accumulator += dt;
        int ticks = 0;
        while (sim_accumulator >= .1 && ticks++ < 3) {
          simulation_time += .1;
          world.Tick(.1f, save, simulation_time);
          game.Tick(world, save, .1f, simulation_time);
          sim_accumulator -= .1;
        }
        game.Combat(world, save, dt);
        if (options.preview == "combat" && now > .7 &&
            game.attack_cooldown <= 0) {
          game.facing = {1, 0};
          game.Attack(world, save);
        }
        if (now >= next_explore) {
          auto discovered = game.Explore(
              save, static_cast<float>(GetScreenWidth()),
              static_cast<float>(GetScreenHeight()), camera.zoom, &world);
          if (options.preview.empty() && !options.benchmark_gameplay &&
              options.benchmark_seconds <= 0) {
            for (int city : discovered) {
              perf.Event({{"event", "town_discovered"}, {"city", city}});
            }
            ui.discovered_towns.insert(ui.discovered_towns.end(),
                                       discovered.begin(), discovered.end());
          }
          next_explore = now + .2;
        }
        if (!game.notice.empty()) {
          notify(game.notice, now);
          game.notice.clear();
        }
        if (options.benchmark_gameplay) {
          if (scenario_step < 8 && now > 1 + scenario_step * .35) {
            auto result = FixtureCraft(save, scenario_step);
            perf.Event({{"event", "benchmark_craft"},
                        {"item", scenario_step},
                        {"ok", result.ok}});
            scenario_step++;
          }
          if (options.benchmark_community && scenario_step >= 8 &&
              !community_assigned) {
            bool trained = FixtureCraft(save, 5).ok;
            auto research_position = save.player;
            save.player = frontier::Settlements()[save.home_city].pos;
            trained = frontier::Research(save, 1).ok && trained;
            save.player = research_position;
            trained =
                frontier::AssignCitizen(save, save.citizens[1].npc, "guard",
                                        "wood_sword", "leather_armor")
                    .ok &&
                trained;
            trained = frontier::EquipOutfit(save, "forager").ok && trained;
            community_assigned = true;
            perf.Event(
                {{"event", "benchmark_resident_training"}, {"ok", trained}});
          }
          if (scenario_step == 8 && now > 4) {
            auto buy = frontier::Trade(save, 0, 0, true),
                 sell = frontier::Trade(save, 0, 0, false);
            perf.Event(
                {{"event", "benchmark_trade"}, {"ok", buy.ok && sell.ok}});
            scenario_step++;
          }
          if (scenario_step == 9 && now > 4.4) {
            auto result = game.PlaceCampfire(world, save);
            perf.Event({{"event", "benchmark_campfire"}, {"ok", result.ok}});
            scenario_step++;
          }
          if (options.benchmark_expedition && scenario_step == 10 &&
              now > 4.7) {
            for (int i = 0; i < 4; i++) {
              FixtureCraft(save, 10);
            }
            bool success = true;
            for (int recipe : {9, 11, 12, 13, 14}) {
              auto result = FixtureCraft(save, recipe);
              success = success && result.ok;
            }
            save.vehicle = 2;
            scenario_step++;
            perf.Event({{"event", "benchmark_vessels"}, {"ok", success}});
          }
          if (options.benchmark_expedition && scenario_step == 11 &&
              frontier::TerrainAt(save.player, save.seed) ==
                  frontier::Terrain::kDeep &&
              now > 25) {
            save.vehicle = 3;
            auto result = frontier::Dive(save);
            if (result.ok) {
              scenario_step++;
              perf.Event({{"event", "benchmark_dive"}, {"ok", true}});
            }
          }
          if (options.benchmark_expedition && scenario_step == 12 && now > 33) {
            auto result = frontier::Sonar(save);
            game.sonar_cooldown = 10;
            scenario_step++;
            perf.Event({{"event", "benchmark_sonar"}, {"ok", result.ok}});
          }
          if (options.benchmark_expedition && scenario_step == 13 && now > 45) {
            if (save.submerged) {
              frontier::Dive(save);
            }
            save.vehicle = 2;
            scenario_step++;
            perf.Event(
                {{"event", "benchmark_surface"}, {"ok", !save.submerged}});
          }
          if (now > 6 && game.attack_cooldown <= 0) {
            bool nearby = false;
            for (auto& [coord, c] : world.chunks) {
              for (auto& enemy : c.enemies) {
                if (!save.defeated.contains(enemy.id) &&
                    (!enemy.kind || save.relations[enemy.city].treaty ==
                                        frontier::Treaty::kWar) &&
                    frontier::Length(enemy.pos - save.player) < 110) {
                  game.facing = frontier::Normalized(enemy.pos - save.player);
                  nearby = true;
                }
              }
            }
            if (nearby) {
              auto result = game.Attack(world, save);
              if (result.ok) {
                pause_until =
                    std::max(pause_until, now + game.attack_duration + .15);
              }
            }
          }
          if (save.health < 60) {
            frontier::Consume(save, true);
          }
        }
        resource = world.Nearest(save.player, save, 90);
        ui.hint.clear();
        if (nearest_npc >= 0) {
          ui.hint = "Nearby: " + world.npcs[nearest_npc].name;
        } else if (resource) {
          ui.hint =
              resource->kind == frontier::ResourceKind::kFish
                  ? (save.inventory.rod ? "Catch fish with a rod"
                                        : "Fishing rod required")
                  : "Resource: " + std::string(frontier::GoodName(
                                       frontier::ResourceGood(resource->kind)));
        }
        if (ui.npc >= 0) {
          ui.hint = "Talking with " + world.npcs[ui.npc].name +
                    " · ESC: end conversation";
        } else if (nearby_hall >= 0) {
          ui.hint +=
              ui.hint.empty() ? "Town hall nearby" : " · town hall nearby";
        }
        if (ui.npc < 0 && !ui.crafting && !ui.trading && !ui.civic &&
            !ui.equipment && !ui.cartography && !ui.progression &&
            !ui.wardrobe && !ui.journal && resource &&
            frontier::ActionDown(frontier::InputAction::kGather)) {
          if (gathering_id != resource->id) {
            gathering = 0;
            gathering_id = resource->id;
          }
          gathering += dt;
          if (gathering >=
              frontier::HarvestDuration(*resource, save.inventory) *
                  frontier::GatherTimeMultiplier(save)) {
            notify(frontier::Harvest(save, *resource).message, now);
            gathering = 0;
            resource = nullptr;
          }
        } else {
          gathering = 0;
        }
        ui.actions.clear();
        auto hint_action = [&](frontier::InputAction action,
                               const std::string& description) {
          ui.actions.push_back({frontier::ActionKey(action), description});
        };
        if (ui.npc >= 0) {
          if (frontier::NpcQuest(save, world.npcs[ui.npc])) {
            hint_action(frontier::InputAction::kQuestAction, "Quest");
          }
          const auto& speaker = world.npcs[ui.npc];
          const auto& town_type =
              frontier::GetContent()
                  .towns[frontier::Settlements()[speaker.settlement].type];
          if (frontier::GetContent().npcs[speaker.type].id ==
              town_type.teacher) {
            hint_action(frontier::InputAction::kTeacher, "Specialist lesson");
          }
          if (frontier::GetCitizen(save, speaker.id)) {
            hint_action(frontier::InputAction::kResidentJob, "Job");
            hint_action(frontier::InputAction::kResidentWeapon, "Work weapon");
            hint_action(frontier::InputAction::kResidentOutfit, "Work clothes");
            hint_action(frontier::InputAction::kResidentAssign, "Assign");
          }
          ui.actions.push_back({"Esc", "Close conversation"});
        } else if (ui.cartography) {
          hint_action(frontier::InputAction::kMap, "Close map");
          ui.actions.push_back({"Click", "Write / open note"});
          ui.actions.push_back({"Tab", "Journal"});
          ui.actions.push_back({"Middle mouse", "Pan"});
          ui.actions.push_back({"Wheel", "Zoom"});
          ui.actions.push_back({"1 / 2 / 3", "Map layers"});
        } else if (ui.Blocked()) {
          ui.actions.push_back({"Enter / Esc", "Confirm / close"});
        } else {
          const auto& bindings = frontier::ActiveSettings().bindings;
          std::string movement;
          for (auto action :
               {frontier::InputAction::kUp, frontier::InputAction::kLeft,
                frontier::InputAction::kDown, frontier::InputAction::kRight}) {
            if (!movement.empty()) {
              movement += "/";
            }
            movement +=
                frontier::KeyName(bindings[static_cast<int>(action)].primary);
          }
          ui.actions.push_back({movement, "Move"});
          hint_action(frontier::InputAction::kSprint, "Sprint");
          if (save.nation) {
            hint_action(frontier::InputAction::kNation, "State");
            hint_action(frontier::InputAction::kResidents, "Residents");
            hint_action(frontier::InputAction::kInventory, "Cargo / resources");
          }
          hint_action(frontier::InputAction::kProgress, "State knowledge");
          hint_action(frontier::InputAction::kMap, "Map");
          if (nearest_npc >= 0) {
            hint_action(frontier::InputAction::kTalk,
                        "Talk: " + world.npcs[nearest_npc].name);
            hint_action(frontier::InputAction::kTrade, "Trade");
          }
          if (nearby_hall >= 0) {
            hint_action(frontier::InputAction::kHall, "Enter town hall");
          }
          if (resource) {
            hint_action(frontier::InputAction::kGather, "Gather (hold)");
          }
          hint_action(frontier::InputAction::kCraft, "Craft");
          hint_action(frontier::InputAction::kWeapons, "Weapons");
          hint_action(frontier::InputAction::kOutfits, "Outfits");
          if (!save.submerged && !frontier::Watercraft(save)) {
            hint_action(frontier::InputAction::kAttack, "Attack");
          }
          if (frontier::Watercraft(save)) {
            hint_action(frontier::InputAction::kVehicle, "Vessel");
          }
          if (save.inventory.submarine && frontier::ActiveVehicle(save) == 3) {
            hint_action(frontier::InputAction::kDive, "Dive / surface");
          }
          if (save.submerged && save.inventory.sonar) {
            hint_action(frontier::InputAction::kSonar, "Sonar");
          }
          if (save.hull < 100 && save.inventory.repairs) {
            hint_action(frontier::InputAction::kRepair, "Repair");
          }
          if (save.health < 100 &&
              (save.inventory.berries || save.inventory.fish)) {
            hint_action(frontier::InputAction::kEat, "Eat");
          }
          if (save.health < 100 && save.inventory.potions) {
            hint_action(frontier::InputAction::kHeal, "Heal");
          }
          if (save.inventory.campfires && !save.submerged &&
              !frontier::Watercraft(save)) {
            hint_action(frontier::InputAction::kCampfire, "Place campfire");
          }
          hint_action(frontier::InputAction::kQuests, "Quests");
          ui.actions.push_back({"Esc", "Pause"});
        }
        ui.harvest_progress =
            resource
                ? gathering /
                      (frontier::HarvestDuration(*resource, save.inventory) *
                       frontier::GatherTimeMultiplier(save))
                : 0;
      }
      const bool autosave_allowed =
          save.nation ||
          (!ui.paused && !ui.progression && !ui.map_journal && !ui.road_edit);
      if (!options.no_save && autosave_allowed && !quit &&
          now - last_save > 30) {
        auto started = frontier::Clock::now();
        // Bound retries even if the disk/path stays unavailable. A failed
        // background save must not throw away the still-running world.
        last_save = now;
        try {
          frontier::WriteSave(options.save_path, save);
          perf.Event({{"event", "save"},
                      {"duration_ms", frontier::ElapsedMilliseconds(started)},
                      {"harvested", save.harvested.size()}});
        } catch (const std::exception& error) {
          notify(
              "Autosave failed. Your world is still open; retry Save from "
              "the pause menu. " +
                  std::string(error.what()),
              now);
          perf.Event({{"event", "save_failed"},
                      {"reason", "autosave"},
                      {"error", error.what()}});
        }
      }
      auto state = dialogue.Snapshot();
      ui.busy = state.busy;
      ui.online = state.online;
      ui.reply = ui.npc >= 0
                     ? (state.busy && state.npc_id == ui.npc ? state.text
                                                             : replies[ui.npc])
                     : "";
      // Busy label applies to the current speaker only; another request can
      // keep running after closing.
      ui.busy = state.busy && state.npc_id == ui.npc;
      ui.wood = save.inventory.wood;
      ui.stone = save.inventory.stone;
      ui.berries = save.inventory.berries;
      ui.fps = perf.fps;
      ui.p95 = perf.p95;
      ui.p99 = perf.p99;
      ui.work_ms = perf.work_ms;
      ui.chunks = world.chunks.size();
      ui.pending = world.Pending();
      ui.time = now;
      ui.animals = 0;
      for (auto& [coord, c] : world.chunks) {
        for (auto& a : c.animals) {
          if (frontier::Length(a.pos - save.player) < 1100) {
            ui.animals++;
          }
        }
      }
      double update_ms = frontier::ElapsedMilliseconds(update_start);
      auto render_start = frontier::Clock::now();
      if (ui.road_edit) {
        ui.actions = {{"Click", "Town / waypoint"},   {"Right click", "Undo"},
                      {"Enter", "Build entire road"}, {"Esc", "Cancel plan"},
                      {"Middle mouse", "Pan"},        {"Wheel", "Zoom"}};
        ui.hint =
            "Construction requires an explored route and the full material "
            "cost.";
      }
      if (save.nation) {
        frontier::StoreControlled(save);
        frontier::SyncNationActors(world, save);
        if (!frontier::HasControlled(save) && !ui.government && !ui.paused) {
          ui.government = true;
          ui.gov_tab = 0;
          ui.gov_city = save.home_city;
        }
        if (ui.government) {
          ui.actions = {{"Esc", "Close management"},
                        {"N", "State"},
                        {"U", "Residents"},
                        {"Y", "Cargo / resources"}};
        }
      }
      renderer.UpdateAudio(&save, &game, ui.Blocked(), &world);
      BeginDrawing();
      ClearBackground(save.submerged ? Color{12, 28, 39, 255}
                                     : Color{41, 58, 42, 255});
      BeginMode2D(camera);
      ui.visible = renderer.DrawWorld(world, save, game, camera, now,
                                      static_cast<float>(sim_accumulator / .1),
                                      resource, ui.harvest_progress);
      EndMode2D();
      renderer.DrawLabels(world, save, ui, camera);
      renderer.DrawUi(world, save, ui, camera);
      renderer.DrawNationPanel(save, ui);
      renderer.DrawDiscovery(world, save, ui);
      if (ui.settings_open && settings_was_open) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                      Color{0, 0, 0, 185});
        if (frontier::DrawSettingsPanel(renderer, settings_panel, live_settings,
                                        options.settings_path)) {
          ui.settings_open = false;
        }
        ui.debug = live_settings.debug;
        desired_zoom = live_settings.zoom;
      }
      double submit_ms = frontier::ElapsedMilliseconds(render_start);
      auto present_start = frontier::Clock::now();
      EndDrawing();
      double present_ms = frontier::ElapsedMilliseconds(present_start);
      auto presented = frontier::Clock::now();
      perf.Frame(std::chrono::duration<double, std::milli>(presented -
                                                           last_presentation)
                     .count(),
                 update_ms, submit_ms, present_ms, state.busy);
      last_presentation = presented;
      if (now - last_flush >= 5) {
        perf.Flush(context());
        last_flush = now;
      }
      frame_count++;
      if ((options.frames > 0 && frame_count >= options.frames) ||
          (options.benchmark_seconds > 0 && now >= options.benchmark_seconds)) {
        quit = true;
      }
    }
    if (!options.screenshot.empty()) {
      if (!options.screenshot.parent_path().empty()) {
        std::filesystem::create_directories(options.screenshot.parent_path());
      }
      Image screenshot{rlReadScreenPixels(GetRenderWidth(), GetRenderHeight()),
                       GetRenderWidth(), GetRenderHeight(), 1,
                       PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
      if (!screenshot.data) {
        throw std::runtime_error("Cannot read screenshot pixels");
      }
      bool exported = ExportImage(screenshot, options.screenshot.c_str());
      UnloadImage(screenshot);
      if (!exported) {
        throw std::runtime_error("Cannot export screenshot");
      }
    }
    if (!options.no_save && !saved_for_exit) {
      frontier::WriteSave(options.save_path, save);
    }
    perf.Flush(context());
    auto final_reply = dialogue.Snapshot();
    if (final_reply.busy) {
      perf.Event({{"event", "llm_cancelled"},
                  {"npc_id", final_reply.npc_id},
                  {"reason", "session_exit"}});
    }
    perf.Event({{"event", "session_end"}, {"total_frames", frame_count}});
    std::cout << "Frames: " << frame_count << ". Last window FPS: " << perf.fps
              << ", p95: " << perf.p95 << " ms\n";
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    auto options = Parse(argc, argv);
    const bool menu_preview =
        options.preview == "menu" || options.preview == "newgame" ||
        options.preview == "saves" || options.preview == "settings";
    const bool use_menu = menu_preview || (options.frames == 0 &&
                                           options.benchmark_seconds == 0 &&
                                           options.preview.empty());
    if (!use_menu) {
      int result = Play(options);
      CloseWindow();
      return result;
    }
    const auto settings_path = options.settings_path;
    auto settings = frontier::LoadSettings(settings_path);
    settings.fullscreen = options.fullscreen;
    if (options.width != 1600 || options.height != 900) {
      settings.width = options.width;
      settings.height = options.height;
    }
    if (options.port_explicit) {
      settings.port = options.port;
    }
    if (options.uncapped) {
      settings.fps = 0;
    }
    SetTraceLogLevel(LOG_INFO);
    SetTraceLogCallback(StartupTrace);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(settings.width, settings.height, "Forest Frontier");
    SetTraceLogLevel(LOG_WARNING);
    SetTraceLogCallback(nullptr);
    if (!IsWindowReady()) {
      throw std::runtime_error("Cannot create window");
    }
    SetWindowMinSize(900, 650);
    SetExitKey(KEY_NULL);
    if (settings.fullscreen) {
      ToggleGameFullscreen();
    }
    frontier::SetActiveSettings(settings);
    frontier::MenuOptions menu;
    menu.directory = options.save_path.parent_path();
    if (menu.directory.empty()) {
      menu.directory = ".";
    }
    menu.preferred = options.save_explicit
                         ? options.save_path
                         : std::filesystem::path(settings.last_save);
    menu.settings_path = settings_path;
    menu.settings = settings;
    menu.preview = menu_preview ? options.preview : "";
    menu.frames = menu_preview ? options.frames : 0;
    menu.screenshot = options.screenshot;
    frontier::SetActiveSettings(menu.settings);
    bool direct_start =
        (options.direct || options.no_save || options.spawn.has_value()) &&
        !menu_preview;
    while (!WindowShouldClose()) {
      bool first_direct_session = direct_start;
      direct_start = false;
      auto slot = first_direct_session
                      ? std::optional<std::filesystem::path>(options.save_path)
                      : frontier::MainMenu(menu);
      if (!slot) {
        break;
      }
      menu.settings.last_save = std::filesystem::absolute(*slot).string();
      frontier::WriteSettings(settings_path, menu.settings);
      auto game_options = options;
      game_options.save_path = *slot;
      game_options.preview.clear();
      game_options.frames = 0;
      if (!first_direct_session) {
        game_options.screenshot.clear();
      }
      if (!first_direct_session) {
        game_options.no_save = false;
        game_options.spawn.reset();
        game_options.width = menu.settings.width;
        game_options.height = menu.settings.height;
        game_options.fullscreen = menu.settings.fullscreen;
        game_options.fps = menu.settings.fps;
        game_options.uncapped = false;
        game_options.port = menu.settings.port;
        game_options.zoom = menu.settings.zoom;
        game_options.debug = menu.settings.debug;
      }
      frontier::SetActiveSettings(menu.settings);
      Play(game_options);
      menu.settings = frontier::ActiveSettings();
      menu.preferred = *slot;
      menu.settings.fullscreen = IsWindowFullscreen();
    }
    CloseWindow();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Forest Frontier: " << e.what() << '\n';
    if (IsWindowReady()) {
      CloseWindow();
    }
    return 1;
  }
}
