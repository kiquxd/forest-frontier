#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "adventure.hpp"
#include "nation.hpp"
#include "progress.hpp"
#include "save_slots.hpp"
#include "settlement_system.hpp"

namespace {
void Check(bool value, const char* message) {
  if (!value) {
    throw std::runtime_error(message);
  }
}
int Recipe(const std::string& id) {
  for (int i = 0; i < static_cast<int>(frontier::Recipes().size()); ++i) {
    if (frontier::Recipes()[i].id == id) {
      return i;
    }
  }
  throw std::runtime_error("Missing recipe");
}
void Advance(frontier::Save& save, float seconds) {
  frontier::TickTowns(save, seconds);
  frontier::TickNation(save, seconds);
}
}  // namespace
int main(int argc, char** argv) {
  auto folder =
      std::filesystem::temp_directory_path() /
      ("frontier-nation-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    auto path = frontier::CreateSaveSlot(folder, "Fresh-state", 742019);
    auto save = frontier::LoadSave(path);
    int home = save.home_city;
    int first = save.controlled_npc;
    int second = save.citizens[1].npc == first ? save.citizens[2].npc
                                               : save.citizens[1].npc;
    Check(
        save.nation && frontier::HasControlled(save) &&
            save.inventory.wood == 0 && save.stocks[home][0] > 0,
        "New campaign controls a resident with city-owned starting resources");
    auto fresh_summary = frontier::InventorySummary(save);
    Check(std::none_of(fresh_summary.begin(), fresh_summary.end(),
                       [](const auto& entry) {
                         return entry.icon.starts_with("weapon_");
                       }),
          "Fresh-world summary does not invent unowned weapon copies");
    auto total = frontier::MoneySupply(save);
    Check(total == save.money_supply, "Initial money is fully accounted for");
    save.skills.insert("gathering");
    auto original_position = save.player;
    save.player = save.player + frontier::Vec{10, 15};
    save.inventory.wood = 3;
    save.health = 73;
    Check(frontier::SelectResident(save, second).ok &&
              save.inventory.wood == 0 && save.skills.contains("gathering"),
          "Switching shares state knowledge but not cargo");
    save.inventory.reeds = 2;
    Check(frontier::SelectResident(save, first).ok &&
              save.inventory.wood == 3 && save.inventory.reeds == 0 &&
              save.health == 73 &&
              frontier::Length(save.player - original_position) > 10,
          "Switch back restores exact cargo, health and position");
    save.player = frontier::Settlements()[home].pos + frontier::Vec{110, 110};
    int wood = save.stocks[home][0];
    Check(frontier::TransferGoods(save, home, 0, 3, true).ok &&
              save.stocks[home][0] == wood + 3 && save.inventory.wood == 0,
          "Deposit transfers cargo exactly once");
    Check(!frontier::TransferGoods(save, home, 0, 1, true).ok &&
              frontier::MoneySupply(save) == total,
          "Empty deposit does not create goods or coins");
    for (const auto& skill : frontier::Skills()) {
      save.skills.insert(skill.id);
    }
    save.stocks[home][0] = save.stocks[home][1] = save.stocks[home][2] = 1000;
    Check(frontier::BuildFacility(save, home, "forge").ok &&
              !frontier::BuildFacility(save, home, "forge").ok,
          "Small population limits new forges");
    auto forge = save.facilities.back().id;
    Check(forge > 0 && frontier::MoneySupply(save) == total,
          "Building fees remain in the regional fund");
    Check(frontier::QueueProduction(save, Recipe("sword"), home).ok,
          "Production reserves materials");
    Check(save.inventory.weapons.empty() && save.crafted == 0 &&
              save.production.size() == 1,
          "Queued output is not granted immediately");
    Advance(save, 59);
    Check(save.crafted == 0, "Production cannot finish early");
    Advance(save, 1);
    Check(
        save.crafted == 1 && save.city_products[home]["weapon:wood_sword"] == 1,
        "Completion places output in the city warehouse");
    auto warehouse_summary = frontier::InventorySummary(save);
    Check(std::any_of(warehouse_summary.begin(), warehouse_summary.end(),
                      [](const auto& entry) {
                        return entry.icon == "weapon_wood_sword" &&
                               entry.count == 1;
                      }),
          "Saved-world summary includes stored weapons before withdrawal");
    Check(
        frontier::TransferProduct(save, home, "weapon:wood_sword", false).ok &&
            frontier::EquipWeapon(save, "wood_sword").ok,
        "Resident takes and equips a counted warehouse copy");
    Check(!frontier::TransferProduct(save, home, "weapon:wood_sword", false).ok,
          "Warehouse copy cannot be taken twice");
    Check(frontier::SelectResident(save, second).ok &&
              !frontier::EquipWeapon(save, "wood_sword").ok,
          "Another resident cannot equip someone else's carried weapon");
    Check(frontier::SelectResident(save, first).ok, "Return to first resident");
    auto stock_before = save.stocks[home];
    Check(frontier::QueueProduction(save, Recipe("campfire"), home).ok,
          "Queue a cancellable job");
    auto job = save.production.back().id;
    Check(frontier::CancelProduction(save, job).ok &&
              save.stocks[home] == stock_before &&
              !frontier::CancelProduction(save, job).ok,
          "Cancellation returns reserved inputs once");
    Check(frontier::PlantResource(save, home, 0).ok,
          "Plant a renewable forest plot");
    Advance(save, 600);
    int grown_before = save.stocks[home][0];
    Check(frontier::HarvestPlot(save, 0).ok &&
              save.stocks[home][0] == grown_before + 20,
          "Mature trees yield a renewable wood batch");
    Check(!save.travelers.empty() &&
              frontier::AcceptTraveler(save, save.travelers[0].npc).ok,
          "A wanderer can settle with housing and food");
    int newcomer = save.travelers[0].npc;
    Check(frontier::SelectResident(save, newcomer).ok &&
              frontier::SelectResident(save, first).ok,
          "Newly accepted resident can be directly controlled");
    Check(frontier::BuildFacility(save, home, "infirmary").ok,
          "Build the first medical facility");
    save.health = 40;
    Check(frontier::TreatResident(save, first).ok && save.health == 80 &&
              frontier::MoneySupply(save) == total,
          "Treatment restores health and transfers its fee");
    auto task_it = std::find_if(
        save.city_tasks.begin(), save.city_tasks.end(), [&](const auto& t) {
          return t.city == home && t.reward == "recipe:outfit_forager";
        });
    Check(task_it != save.city_tasks.end(),
          "Fresh town has an introductory city project");
    auto task_id = task_it->id;
    int coins_before = save.coins;
    Check(frontier::AcceptCityTask(save, task_id).ok &&
              frontier::FinishCityTask(save, task_id).ok &&
              save.coins == coins_before &&
              save.known_recipes.contains("outfit_forager"),
          "City project grants state knowledge without coins");
    Check(!frontier::FinishCityTask(save, task_id).ok,
          "City reward cannot be claimed twice");
    int foreign = (home + 1) % static_cast<int>(save.stocks.size());
    save.relations[foreign] = {40, frontier::Treaty::kPeace, true};
    save.stocks[foreign][0] = 0;
    frontier::RefreshCityTasks(save);
    auto trade = std::find_if(
        save.city_tasks.begin(), save.city_tasks.end(),
        [&](const auto& t) { return t.city == foreign && t.kind == "trade"; });
    Check(trade != save.city_tasks.end(),
          "A foreign deficit creates a budgeted trade order");
    auto order_id = trade->id;
    int good = trade->good;
    int quantity = trade->quantity;
    int price = trade->price;
    Check(frontier::AcceptCityTask(save, order_id).ok &&
              frontier::MoneySupply(save) == total,
          "Accepted trade moves payment into escrow");
    Check(!frontier::FinishCityTask(save, order_id).ok,
          "Trade delivery cannot be completed remotely");
    save.player =
        frontier::Settlements()[foreign].pos + frontier::Vec{110, 110};
    frontier::GoodCount(save.inventory, good) = quantity;
    coins_before = save.coins;
    Check(frontier::FinishCityTask(save, order_id).ok &&
              save.coins == coins_before + price &&
              frontier::MoneySupply(save) == total,
          "Delivered goods pay from buyer escrow without minting money");
    save.player = frontier::Settlements()[home].pos + frontier::Vec{110, 110};
    frontier::StoreControlled(save);
    Check(frontier::LoseResident(save, first, true, foreign).ok &&
              !frontier::HasControlled(save),
          "Selected resident can become captive without an avatar respawn");
    Check(frontier::SelectResident(save, second).ok &&
              !frontier::EquipWeapon(save, "wood_sword").ok,
          "Captured gear remains allocated");
    Check(frontier::RansomResident(save, first).ok &&
              frontier::SelectResident(save, first).ok &&
              save.inventory.weapon_id == "wood_sword",
          "Ransom returns the same carried equipment");
    frontier::WriteSave(path, save);
    auto loaded = frontier::LoadSave(path);
    frontier::ConfigureWorld(loaded.seed);
    frontier::InitializeAdventure(loaded);
    Check(loaded.controlled_npc == first &&
              loaded.city_products == save.city_products &&
              loaded.travelers.size() == save.travelers.size() &&
              frontier::MoneySupply(loaded) == total,
          "v9 roundtrip preserves resident cargo, new arrivals, warehouses and "
          "money");
    // Advance simulated deadlines directly; these checks never idle in real
    // time.
    auto advanced = loaded;
    advanced.region_fund -= 1000;
    advanced.coins += 1000;
    for (auto& amount : advanced.stocks[home]) {
      amount = 1000;
    }
    advanced.technologies.insert("steelworking");
    Check(
        frontier::QueueProduction(advanced, Recipe("steel"), home, forge).ok &&
            frontier::QueueProduction(advanced, Recipe("campfire"), home).ok,
        "Different physical workshops accept parallel jobs");
    auto steel_due = advanced.production[0].due;
    Check(
        frontier::QueueProduction(advanced, Recipe("steel"), home, forge).ok &&
            advanced.production.back().due > steel_due,
        "A workshop serializes its own jobs");
    Check(advanced.production[1].due < steel_due,
          "Workbench does not wait for a forge's longer job");
    Advance(advanced, 60);
    Check(advanced.city_products[home]["campfire"] == 1 &&
              advanced.production.size() == 2,
          "The independent workbench finishes while the forge remains busy");
    Check(frontier::BuildFacility(advanced, home, "school").ok,
          "Build a school for hospital research");
    Check(!frontier::ResearchHospital(advanced).ok,
          "Hospital research requires medical knowledge");
    advanced.technologies.insert("medical_care");
    Check(frontier::ResearchHospital(advanced).ok &&
              frontier::BuildFacility(advanced, home, "hospital").ok,
          "Medical knowledge unlocks the hospital upgrade");
    advanced.health = 1;
    Check(frontier::TreatResident(advanced, first).ok && advanced.health == 100,
          "Hospital fully restores a visiting resident");
    auto survey =
        std::find_if(advanced.city_tasks.begin(), advanced.city_tasks.end(),
                     [](const auto& t) { return t.kind == "survey"; });
    auto survey_id = survey->id;
    Check(frontier::AcceptCityTask(advanced, survey_id).ok &&
              !frontier::FinishCityTask(advanced, survey_id).ok &&
              frontier::CancelCityTask(advanced, survey_id).ok,
          "Survey requires new discoveries and can be abandoned");
    frontier::RefreshCityTasks(advanced);
    auto retry = std::find_if(
        advanced.city_tasks.begin(), advanced.city_tasks.end(),
        [](const auto& t) { return t.kind == "survey" && !t.completed; });
    Check(retry != advanced.city_tasks.end(),
          "Abandoning a knowledge project does not permanently block it");
    auto retry_id = retry->id;
    Check(frontier::AcceptCityTask(advanced, retry_id).ok,
          "Accept replacement survey");
    int discovered = 0;
    for (auto& relation : advanced.relations) {
      if (!relation.discovered && discovered < 2) {
        relation.discovered = true;
        ++discovered;
      }
    }
    Check(frontier::FinishCityTask(advanced, retry_id).ok &&
              advanced.technologies.contains("civic_charter") &&
              frontier::MoneySupply(advanced) == total,
          "New discoveries reward shared knowledge without changing money");
    frontier::WriteSave(folder / "advanced.json", advanced);
    auto advanced_loaded = frontier::LoadSave(folder / "advanced.json");
    Check(advanced_loaded.production.size() == 2 &&
              advanced_loaded.technologies.contains("hospitals"),
          "Pending workshop jobs and hospital progression survive reload");
    auto reports = frontier::TownReports(save, frontier::MakeNpcs());
    Check(reports[0].residents == frontier::CityPopulation(save, home),
          "Town summaries include accepted wanderers");
    advanced.owners[foreign] = 0;
    advanced.relations[foreign].treaty = frontier::Treaty::kUnion;
    frontier::InitializeAdventure(advanced);
    advanced.stocks[foreign][2] = 100;
    Check(!frontier::MoveResident(advanced, first, foreign).ok,
          "Changing a home town cannot teleport a resident");
    advanced.player =
        frontier::Settlements()[foreign].pos + frontier::Vec{110, 110};
    Check(frontier::MoveResident(advanced, first, foreign).ok &&
              frontier::GetCitizen(advanced, first)->city == foreign,
          "Visiting an owned town permits migration with housing and food");
    Check(!frontier::ShipGoods(advanced, home, foreign, 1, 5).ok,
          "Warehouse shipment requires an explicitly built road");
    advanced.roads.push_back({home,
                              foreign,
                              {frontier::Settlements()[home].pos,
                               frontier::Settlements()[foreign].pos}});
    int destination_stock = advanced.stocks[foreign][1];
    Check(frontier::ShipGoods(advanced, home, foreign, 1, 5).ok,
          "Road-connected warehouses can reserve a shipment");
    advanced.relations[foreign].treaty = frontier::Treaty::kWar;
    Advance(advanced, 1000);
    Check(advanced.stocks[foreign][1] == destination_stock &&
              !advanced.shipments.empty(),
          "Hostile transit prevents scheduled deliveries");
    advanced.relations[foreign].treaty = frontier::Treaty::kUnion;
    Advance(advanced, 1000);
    Check(advanced.stocks[foreign][1] == destination_stock + 5,
          "Peaceful access resumes delivery without duplicating cargo");
    bool specialist_checked = false;
    for (int city = 0; city < static_cast<int>(advanced.stocks.size());
         ++city) {
      const auto& type =
          frontier::GetContent().towns[frontier::Settlements()[city].type];
      if (frontier::OwnsCity(advanced, city)) {
        continue;
      }
      for (int material : type.goods) {
        if (material < 6) {
          continue;
        }
        advanced.relations[city] = {40, frontier::Treaty::kPeace, true};
        advanced.technologies.insert(type.technology);
        int before = advanced.stocks[home][material];
        Check(frontier::OrderRareGoods(advanced, city, home, material, 1).ok,
              "A knowledgeable peaceful specialist accepts a paid order");
        Advance(advanced, 1000);
        Check(advanced.stocks[home][material] == before + 1 &&
                  frontier::MoneySupply(advanced) == total,
              "Specialist cargo arrives while its payment stays in the system");
        specialist_checked = true;
        break;
      }
      if (specialist_checked) {
        break;
      }
    }
    Check(specialist_checked, "Fresh region contains a rare-goods specialist");
    if (argc == 3 && std::string(argv[1]) == "--fixture") {
      frontier::WriteSave(argv[2], loaded);
    }
    frontier::LoseResident(save, first, false);
    Check(save.weapon_copies["wood_sword"] == 0 &&
              frontier::GetCitizen(save, first)->state == "dead" &&
              frontier::MoneySupply(save) == total,
          "Death destroys carried gear and keeps state coins in the system");
    auto malformed = frontier::WriteNation(loaded);
    malformed["region_fund"] = loaded.region_fund + 1;
    auto invalid = loaded;
    invalid.production.clear();
    invalid.plots.clear();
    invalid.city_tasks.clear();
    invalid.shipments.clear();
    invalid.travelers.clear();
    bool rejected = false;
    try {
      frontier::ReadNation(malformed, invalid);
    } catch (const std::exception&) {
      rejected = true;
    }
    Check(rejected, "Malformed money ledger is rejected");
    frontier::Save legacy;
    legacy.seed = save.seed;
    frontier::ConfigureWorld(legacy.seed);
    frontier::InitializeCampaign(legacy, false);
    frontier::InitializeAdventure(legacy);
    legacy.skills.insert("gathering");
    legacy.inventory.wood = 7;
    legacy.inventory.weapons.insert("wood_sword");
    legacy.inventory.weapon_id = "wood_sword";
    legacy.weapon_copies["wood_sword"] = 1;
    int legacy_wood = legacy.stocks[legacy.home_city][0];
    frontier::InitializeNation(legacy);
    Check(legacy.skills.contains("gathering") && legacy.inventory.wood == 0 &&
              legacy.stocks[legacy.home_city][0] == legacy_wood + 7 &&
              legacy.inventory.weapon_id == "wood_sword",
          "Legacy migration preserves knowledge, loose goods and equipment");
    frontier::WriteSave(folder / "migrated.json", legacy);
    Check(frontier::LoadSave(folder / "migrated.json").nation,
          "Converted legacy allocations remain valid after reload");
    std::filesystem::remove_all(folder);
    std::cout << "Resident control, warehouses, production, growth, boards, "
                 "captivity and closed money ledger passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::filesystem::remove_all(folder);
    return 1;
  }
}
