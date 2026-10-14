#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "adventure.hpp"
#include "json.hpp"
#include "save_slots.hpp"
#include "settlement_system.hpp"

namespace {
void Check(bool ok, const char* reason) {
  if (!ok) {
    throw std::runtime_error(reason);
  }
}
int FindRecipeIndex(const std::string& id) {
  for (int i = 0; i < static_cast<int>(frontier::Recipes().size()); i++) {
    if (frontier::Recipes()[i].id == id) {
      return i;
    }
  }
  throw std::runtime_error("Missing recipe");
}

frontier::Action CraftAfterWait(frontier::Save& save, int recipe) {
  save.game_time = std::max(save.game_time, save.craft_ready_at);
  return frontier::Craft(save, recipe);
}

}  // namespace

int main() {
  auto folder =
      std::filesystem::temp_directory_path() /
      ("frontier-quests-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    frontier::ConfigureWorld(742019);
    frontier::Save save;
    frontier::InitializeCampaign(save);
    frontier::InitializeAdventure(save);
    save.facilities.push_back({save.home_city, "tailor", save.player});
    save.facilities.push_back({save.home_city, "school", save.player});
    auto npcs = frontier::MakeNpcs();
    Check(save.citizens.size() >= 4 && save.outfit_copies.at("traveler") == 1,
          "New campaign has an owned resident roster and one basic outfit");
    int offers = 0, giver = -1, hunter = -1;
    for (auto& npc : npcs) {
      auto quest = frontier::NpcQuest(save, npc);
      if (quest) {
        offers++;
        if (quest->id == "supplies" && giver < 0) {
          giver = npc.id;
        }
        if (quest->id == "wolfwatch") {
          hunter = npc.id;
        }
      }
      Check(!frontier::NpcIntroduction(save, npc).empty(),
            "Every NPC has authored lore");
    }
    Check(offers > 0 && offers < static_cast<int>(npcs.size()) && giver >= 0,
          "Only some NPCs offer quests");
    auto& npc = npcs[giver];
    save.relations[npc.settlement] = {30, frontier::Treaty::kPeace, true};
    Check(frontier::AcceptQuest(save, npc).ok &&
              !frontier::AcceptQuest(save, npc).ok,
          "Accept quest exactly once");
    int coins = save.coins;
    Check(!frontier::FinishQuest(save, npc).ok && save.coins == coins &&
              !save.known_recipes.contains("outfit_forager"),
          "Incomplete quest cannot grant rewards");
    Check(frontier::TrainSkill(save, 0).ok,
          "First skill available without funds");
    for (int i = 0; i < 2; i++) {
      Check(frontier::Harvest(save, {static_cast<uint64_t>(100 + i),
                                     {},
                                     frontier::ResourceKind::kTree,
                                     0})
                .ok,
            "Gather quest supplies");
    }
    Check(frontier::FinishQuest(save, npc).ok && save.inventory.wood == 0 &&
              save.coins == 10 && save.known_recipes.contains("outfit_forager"),
          "Turn-in consumes requested goods and grants coins and recipe");
    Check(!frontier::FinishQuest(save, npc).ok && save.coins == 10,
          "Quest rewards cannot be duplicated");
    for (int i = 0; i < 4; i++) {
      frontier::Harvest(save, {static_cast<uint64_t>(200 + i),
                               {},
                               frontier::ResourceKind::kTree,
                               0});
    }
    for (int i = 0; i < 3; i++) {
      frontier::Harvest(save, {static_cast<uint64_t>(300 + i),
                               {},
                               frontier::ResourceKind::kReed,
                               0});
    }
    Check(frontier::TrainSkill(save, 2).ok &&
              CraftAfterWait(save, FindRecipeIndex("outfit_forager")).ok,
          "Quest recipe makes usable clothing");
    save.player = {1100, -1100};
    save.submerged = true;
    Check(frontier::EquipOutfit(save, "forager").ok &&
              frontier::GatherTimeMultiplier(save) < 1,
          "Clothes swap underwater and apply gathering perk");
    Check(!CraftAfterWait(save, FindRecipeIndex("outfit_artisan")).ok,
          "Unknown quest recipes remain locked");
    save.submerged = false;
    save.player = frontier::Settlements()[save.home_city].pos;
    save.inventory.wood = 60;
    save.inventory.reeds = 60;
    save.inventory.stone = 60;
    Check(CraftAfterWait(save, FindRecipeIndex("outfit_forager")).ok &&
              save.outfit_copies.at("forager") == 2,
          "Craft spare clothes");
    Check(frontier::TrainSkill(save, 1).ok && frontier::TrainSkill(save, 4).ok,
          "Practice supports initial training skills");
    Check(CraftAfterWait(save, 2).ok && CraftAfterWait(save, 2).ok &&
              save.weapon_copies.at("wood_sword") == 2,
          "Spare weapons use counted copies");
    Check(frontier::EquipWeapon(save, "wood_sword").ok,
          "Weapon swaps have no location restriction");
    save.coins = 100;
    Check(frontier::Research(save, 0).ok,
          "Research combines funds, residents, skills, clothes and completed "
          "quests");
    int first = save.citizens[0].npc, second = save.citizens[1].npc;
    Check(frontier::AssignCitizen(save, first, "gatherer", "", "forager").ok,
          "Train and equip an owned resident");
    Check(!frontier::AssignCitizen(save, second, "gatherer", "", "forager").ok,
          "Player and resident cannot share the same clothing copy");
    int stock = save.stocks[save.citizens[0].city][0];
    frontier::TickResidents(save, 20);
    Check(save.stocks[save.citizens[0].city][0] == stock + 1,
          "Trained gatherer contributes to actual town stock");
    Check(!frontier::EquipOutfit(save, "artisan").ok,
          "Cannot equip clothing that is not owned");
    Check(frontier::ResearchRequirements(save, frontier::ResearchNodes()[5])
                      .find("Avg. foreign trust") != std::string::npos &&
              !frontier::Research(save, 5).ok,
          "Multi-town leadership enforces trust and settlement requirements");
    save.outfit_copies["hunter"] = 1;
    Check(frontier::Research(save, 1).ok &&
              frontier::AssignCitizen(save, second, "guard", "wood_sword",
                                      "hunter")
                  .ok,
          "Militia research trains an equipped defender");
    save.submerged = false;
    save.player = {120, 140};
    {
      frontier::World world(save.seed);
      world.buildings.clear();
      frontier::Chunk flat;
      flat.coord = {0, 0};
      flat.tiles.resize(256,
                        frontier::Tile{0, false, frontier::Terrain::kForest});
      world.chunks.emplace(flat.coord, std::move(flat));
      frontier::Gameplay game;
      int copies = save.weapon_copies["wood_sword"];
      for (int i = 0; i < 12; i++) {
        game.Advance(1);
        Check(game.Attack(world, save).ok, "Repeated sword attacks");
        game.Advance(.2f);
        game.Combat(world, save, .2f);
      }
      Check(save.weapon_copies["wood_sword"] == copies,
            "Attacks never consume weapons");
      save.character_id = "warden";
      auto& chunk = world.chunks.at({0, 0});
      frontier::Enemy target{99999, save.player + frontier::Vec{80, 0},
                             save.player + frontier::Vec{80, 0},
                             save.player + frontier::Vec{80, 0}};
      chunk.enemies.push_back(target);
      game.Advance(1);
      game.Attack(world, save);
      game.Advance(.32f);
      game.Combat(world, save, .32f);
      Check(chunk.enemies[0].hp == 30,
            "Warden attack bonus applies to real hits");
      save.character_id = "traveler";
      world.chunks.clear();
      auto center = frontier::ChunkAt(world.npcs[second].pos);
      for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
          frontier::Chunk c;
          c.coord = {center.x + x, center.y + y};
          c.tiles.resize(256,
                         frontier::Tile{0, false, frontier::Terrain::kForest});
          world.chunks.emplace(c.coord, std::move(c));
        }
      }
      save.player =
          frontier::Settlements()[save.home_city].pos + frontier::Vec{110, 110};
      auto enemy_pos = world.npcs[second].pos + frontier::Vec{50, 0};
      auto& defense = world.chunks.at(frontier::ChunkAt(enemy_pos));
      defense.enemies.push_back({99998, enemy_pos, enemy_pos, enemy_pos});
      game.Tick(world, save, 1.6f, 0);
      Check(
          defense.enemies[0].hp < 60 &&
              save.weapon_copies["wood_sword"] == copies,
          "Equipped residents protect against nearby wolves without consuming "
          "weapons");
    }
    if (hunter >= 0) {
      auto& h = npcs[hunter];
      save.relations[h.settlement] = {30, frontier::Treaty::kPeace, true};
      save.wolf_kills = 3;
      Check(frontier::AcceptQuest(save, h).ok &&
                frontier::GetQuestProgress(save, save.quests.at(h.id)) == 0,
            "Kill quests do not count earlier kills");
      save.wolf_kills++;
      Check(frontier::GetQuestProgress(save, save.quests.at(h.id)) == 1,
            "New wolf defeat counts");
    }
    std::filesystem::create_directories(folder);
    auto path = folder / "world.json";
    frontier::WriteSave(path, save);
    auto loaded = frontier::LoadSave(path);
    Check(loaded.known_recipes == save.known_recipes &&
              loaded.weapon_copies == save.weapon_copies &&
              loaded.outfit_id == "forager" &&
              loaded.citizens[0].job == "gatherer" &&
              loaded.quests.at(giver).completed,
          "New progression and equipment survive save roundtrip");
    auto legacy = folder / "legacy.json";
    frontier::Save old;
    old.inventory.sword = true;
    old.inventory.weapons.insert("wood_sword");
    old.inventory.weapon_id = "wood_sword";
    old.inventory.armor = true;
    frontier::InitializeCampaign(old, true);
    frontier::WriteSave(legacy, old);
    nlohmann::json json;
    {
      std::ifstream in(legacy);
      in >> json;
    }
    json["version"] = 4;
    {
      std::ofstream out(legacy);
      out << json;
    }
    auto migrated = frontier::LoadSave(legacy);
    frontier::InitializeAdventure(migrated);
    frontier::WriteSave(legacy, migrated);
    Check(migrated.weapon_copies.at("wood_sword") == 1 &&
              migrated.outfit_copies.at("leather_armor") == 1 &&
              std::filesystem::exists(legacy.string() + ".v4.bak"),
          "v4 equipment migrates without loss and retains backup");
    for (auto& hero : frontier::GetContent().characters) {
      auto character_path = frontier::CreateSaveSlot(folder / "characters",
                                                     hero.id, 123, hero.id);
      auto character_save = frontier::LoadSave(character_path);
      Check(character_save.character_id == hero.id &&
                character_save.coins == hero.coins,
            "Selected background and start bonus persist");
    }
    Check(frontier::Character("scout").speed > 1 &&
              frontier::Character("scout").taken > 1 &&
              frontier::Character("artisan").gather < 1 &&
              frontier::Character("artisan").damage < 1,
          "Backgrounds have distinct positive and negative modifiers");
    std::filesystem::remove_all(folder);
    std::cout << "quests, recipe rewards, backgrounds, perks, resident jobs, "
                 "gear conservation and migration passed\n";
  } catch (const std::exception& e) {
    std::filesystem::remove_all(folder);
    std::cerr << e.what() << '\n';
    return 1;
  }
}
