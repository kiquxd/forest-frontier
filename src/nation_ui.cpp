#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "input.hpp"
#include "nation.hpp"
#include "progress.hpp"
#include "render.hpp"
#include "settlement_system.hpp"
#include "summary.hpp"

namespace frontier {
namespace {
struct NationRow {
  std::string title{};
  std::string detail{};
  std::string icon = "book";
  std::string first{};
  std::function<void()> primary{};
  std::string second{};
  std::function<void()> secondary{};
  bool enabled{true};
  std::string tooltip{};
  std::vector<ProductionRequirement> requirements{};
  double progress{-1};
  std::string id{};
};
std::string Duration(double seconds) {
  int total = static_cast<int>(std::ceil(std::max(0.0, seconds)));
  if (total >= 3600) {
    return std::to_string(total / 3600) + "h " +
           std::to_string(total % 3600 / 60) + "m";
  }
  return total >= 60 ? std::to_string(total / 60) + "m " +
                           std::to_string(total % 60) + "s"
                     : std::to_string(total) + "s";
}
std::string ProductName(const std::string& product) {
  if (product.starts_with("weapon:")) {
    return Weapon(product.substr(7)).name;
  }
  if (product.starts_with("outfit:")) {
    return Outfit(product.substr(7)).name;
  }
  for (const auto& r : Recipes()) {
    if (r.result == product) {
      return r.name;
    }
  }
  return product;
}
}  // namespace
void Renderer::DrawNationPanel(Save& save, Ui& ui) {
  if (!save.nation || !ui.government || !ui.discovered_towns.empty()) {
    return;
  }
  const float w = GetScreenWidth(), h = GetScreenHeight();
  DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 190});
  Rectangle box{30, 66, w - 60, h - 126};
  DrawRectangleRounded(box, .035f, 10, {19, 34, 27, 255});
  auto button = [&](Rectangle r, const std::string& title, bool enabled = true,
                    bool selected = false, const std::string& id = "") {
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRounded(r, .3f, 8,
                         !enabled            ? Color{28, 40, 33, 255}
                         : selected || hover ? Color{82, 105, 62, 255}
                                             : Color{39, 62, 44, 255});
    Wrapped(title, r.x + 9, r.y + 7, r.width - 18, 15,
            enabled ? Color{231, 231, 205, 255} : Color{128, 146, 127, 255}, 1);
    return UiActivate(r, enabled, id.empty() ? title : id);
  };
  Text(ui.town_hall ? "Town hall · projects, orders and management"
                    : "State · residents and city economy",
       50, 82, 23, {235, 197, 123, 255});
  if (button({w - 190, 80, 140, 36}, "Close · Esc")) {
    ui.government = false;
    return;
  }
  std::vector<int> cities;
  for (int city = 0; city < static_cast<int>(save.stocks.size()); ++city) {
    if (save.relations[city].discovered || OwnsCity(save, city)) {
      cities.push_back(city);
    }
  }
  if (std::find(cities.begin(), cities.end(), ui.gov_city) == cities.end()) {
    ui.gov_city = save.home_city;
  }
  int city = ui.gov_city;
  int selected = static_cast<int>(
      std::find(cities.begin(), cities.end(), city) - cities.begin());
  if (!ui.town_hall && button({50, 122, 42, 34}, "<")) {
    ui.gov_city = cities[(selected + static_cast<int>(cities.size()) - 1) %
                         cities.size()];
    ui.gov_page = 0;
  }
  Wrapped(Settlements()[city].name +
              (OwnsCity(save, city) ? " · your town" : " · foreign town"),
          102, 129, std::min(400.f, w - 460), 17, {229, 224, 190, 255}, 1);
  if (!ui.town_hall && button({std::min(w - 350, 520.f), 122, 42, 34}, ">")) {
    ui.gov_city = cities[(selected + 1) % cities.size()];
    ui.gov_page = 0;
  }
  if (button({w - 270, 122, 220, 34},
             "Transfer batch: " + std::to_string(ui.gov_amount))) {
    ui.gov_amount = ui.gov_amount == 1 ? 5 : ui.gov_amount == 5 ? 10 : 1;
  }
  Wrapped("State treasury " + std::to_string(save.coins) + " · Town treasury " +
              std::to_string(save.treasury[city]) + " · Residents " +
              std::to_string(CityPopulation(save, city)) + "/" +
              std::to_string(save.housing[city]) +
              " · World paused; production runs · Selected: " +
              (HasControlled(save) ? ResidentName(save, save.controlled_npc)
                                   : "none"),
          50, 164, w - 100, 15, {171, 191, 155, 255}, 1);
  const std::vector<std::string> tabs = {"Residents", "Resources", "Production",
                                         "Projects",  "Orders",    "Towns",
                                         "Growth"};
  if (!ui.production_search_focus && (MenuLeft() || MenuRight())) {
    const int direction = MenuLeft() || IsKeyDown(KEY_LEFT_SHIFT) ? -1 : 1;
    ui.gov_tab =
        (ui.gov_tab + direction + static_cast<int>(tabs.size())) % tabs.size();
    ui.gov_page = 0;
    ui.gov_status.clear();
    ui.production_search_focus = false;
  }
  float minimum_tab_width = 0;
  for (const auto& title : tabs) {
    minimum_tab_width = std::max(minimum_tab_width, Measure(title, 15) + 30);
  }
  const int columns =
      std::clamp(static_cast<int>((w - 100) / minimum_tab_width), 1,
                 static_cast<int>(tabs.size()));
  const float tab_width = (w - 100) / columns;
  const float tab_height = TextSize(15) + 18;
  const int tab_rows = (static_cast<int>(tabs.size()) + columns - 1) / columns;
  for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
    if (button(
            {50 + (i % columns) * tab_width,
             195 + (i / columns) * (tab_height + 5), tab_width - 5, tab_height},
            tabs[i], true, ui.gov_tab == i)) {
      ui.gov_tab = i;
      ui.gov_page = 0;
      ui.gov_status.clear();
      ui.production_search_focus = false;
    }
  }
  const float body_top = 195 + tab_rows * (tab_height + 5) + 10;
  Rectangle body{50, body_top, w - 100, h - body_top - 136};
  auto lowercase = [](std::string value) {
    for (char& ch : value) {
      ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
  };
  if (ui.gov_tab == 2) {
    const Rectangle search{195, body.y, w - 365, 42};
    const bool inside = CheckCollisionPointRec(GetMousePosition(), search);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      ui.production_search_focus = inside;
    }
    const bool search_activated = UiActivate(search);
    if (search_activated) {
      ui.production_search_focus = true;
      ClearUiFocus();
    }
    const auto before = ui.production_search;
    if (ui.production_search_focus) {
      if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
          IsKeyPressed(KEY_A)) {
        ui.production_search.clear();
      }
      for (int ch = GetCharPressed(); ch; ch = GetCharPressed()) {
        if (ch >= 32 && ch <= 126 && ui.production_search.size() < 64) {
          ui.production_search.push_back(static_cast<char>(ch));
        }
      }
      if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) &&
          !ui.production_search.empty()) {
        ui.production_search.pop_back();
      }
      if (!search_activated && IsKeyPressed(KEY_ENTER)) {
        ui.production_search_focus = false;
      }
    }
    if (button({w - 150, body.y, 100, 42}, "Clear")) {
      ui.production_search.clear();
    }
    if (before != ui.production_search) {
      ui.gov_page = 0;
    }
    DrawRectangleRounded(search, .25f, 8,
                         ui.production_search_focus ? Color{58, 79, 54, 255}
                                                    : Color{33, 52, 42, 255});
    Text("Search items", 50, body.y + 9, 16, {210, 218, 190, 255});
    BeginScissorMode(
        static_cast<int>(search.x + 10), static_cast<int>(search.y),
        static_cast<int>(search.width - 20), static_cast<int>(search.height));
    Text(ui.production_search.empty() && !ui.production_search_focus
             ? "Item name..."
             : ui.production_search +
                   (ui.production_search_focus &&
                            static_cast<int>(GetTime() * 2) % 2 == 0
                        ? "|"
                        : ""),
         search.x + 10, search.y + 9, 16, {219, 226, 205, 255});
    EndScissorMode();
    body.y += 56;
    body.height -= 56;
  }
  std::vector<NationRow> rows;
  auto act = [&](const Action& result) { ui.gov_status = result.message; };
  if (ui.gov_tab == 0) {
    for (const auto& c : save.citizens) {
      if (!OwnsCity(save, c.city)) {
        continue;
      }
      int npc = c.npc;
      rows.push_back(
          {ResidentName(save, npc) +
               (npc == save.controlled_npc ? " · selected" : ""),
           Settlements()[c.city].name + " · " + c.state + " · HP " +
               std::to_string(npc == save.controlled_npc ? save.health
                                                         : c.health) +
               " · " + c.job + " · " +
               (c.weapon.empty() ? "unarmed" : Weapon(c.weapon).name),
           "person", c.state == "active" ? "Control" : "",
           c.state == "active"
               ? std::function<void()>([&, npc] { ui.request_control = npc; })
               : std::function<void()>{},
           c.state == "captive" ? "Ransom " + std::to_string(c.ransom)
                                : "Treat",
           c.state == "captive"  ? std::function<void()>([&, npc] {
             act(RansomResident(save, npc));
           })
           : c.state == "active" ? std::function<void()>([&, npc] {
               act(TreatResident(save, npc));
             })
                                 : std::function<void()>{}});
    }
  } else if (ui.gov_tab == 1) {
    bool owned = OwnsCity(save, city);
    rows.push_back({"Resident cargo and city resources",
                    "Transfers are free and require the selected resident on "
                    "this town's territory.",
                    "chest", "Deposit all", [&] {
                      if (!OwnsCity(save, city) ||
                          CityAt(save, save.player) != city) {
                        act({false, "Visit your town before unloading cargo"});
                        return;
                      }
                      for (int g = 0; g < kGoodTypes; ++g) {
                        if (save.stocks[city][g] >
                            1000000 - GoodCount(save.inventory, g)) {
                          act({false, "Warehouse full"});
                          return;
                        }
                      }
                      for (int g = 0; g < kGoodTypes; ++g) {
                        int amount = GoodCount(save.inventory, g);
                        if (amount > 0) {
                          TransferGoods(save, city, g, amount, true);
                        }
                      }
                      act({true, "All resource cargo deposited"});
                    }});
    for (int g = 0; g < kGoodTypes; ++g) {
      rows.push_back({GoodName(g),
                      "City resources " + std::to_string(save.stocks[city][g]) +
                          " · Carried " +
                          std::to_string(GoodCount(save.inventory, g)),
                      "good_" + std::to_string(g), "Deposit",
                      owned ? std::function<void()>([&, g] {
                        act(TransferGoods(save, city, g, ui.gov_amount, true));
                      })
                            : std::function<void()>{},
                      "Withdraw",
                      owned ? std::function<void()>([&, g] {
                        act(TransferGoods(save, city, g, ui.gov_amount, false));
                      })
                            : std::function<void()>{}});
    }
    std::set<std::string> products;
    for (const auto& [id, count] : save.city_products[city]) {
      if (count > 0) {
        products.insert(id);
      }
    }
    if (const auto* c = GetCitizen(save, save.controlled_npc)) {
      for (const auto& [id, count] : c->weapons) {
        if (count > 0) {
          products.insert("weapon:" + id);
        }
      }
      for (const auto& [id, count] : c->clothes) {
        if (count > 0) {
          products.insert("outfit:" + id);
        }
      }
      for (const auto& r : Recipes()) {
        if (r.result.starts_with("weapon:") ||
            r.result.starts_with("outfit:") || r.result == "steel") {
          continue;
        }
        int index = static_cast<int>(&r - Recipes().data());
        if (Equipped(save, index)) {
          products.insert(r.result);
        }
      }
      if (save.inventory.potions) {
        products.insert("potion");
      }
      if (save.inventory.campfires) {
        products.insert("campfire");
      }
      if (save.inventory.ammo) {
        products.insert("ammo");
      }
      if (save.inventory.repairs) {
        products.insert("repair");
      }
    }
    for (const auto& id : products) {
      int stock = save.city_products[city].contains(id)
                      ? save.city_products[city].at(id)
                      : 0;
      rows.push_back(
          {ProductName(id),
           "Warehouse copies " + std::to_string(stock) +
               " · equipped copies must be unequipped before depositing",
           "book", "Deposit",
           [&, id] { act(TransferProduct(save, city, id, true)); }, "Withdraw",
           [&, id] { act(TransferProduct(save, city, id, false)); }});
    }
  } else if (ui.gov_tab == 2) {
    std::vector<const Facility*> workshops;
    for (const auto& f : save.facilities) {
      if (ConnectedCities(save, city, f.city) &&
          (f.kind == "workbench" || f.kind == "forge" || f.kind == "tailor" ||
           f.kind == "shipyard" || f.kind == "alchemy")) {
        workshops.push_back(&f);
      }
    }
    auto selected_workshop =
        std::find_if(workshops.begin(), workshops.end(),
                     [&](const auto* f) { return f->id == ui.gov_facility; });
    std::string label =
        selected_workshop == workshops.end()
            ? "Automatic workshop"
            : (*selected_workshop)->kind + " #" +
                  std::to_string((*selected_workshop)->id) + " · " +
                  Settlements()[(*selected_workshop)->city].name;
    rows.push_back(
        {label,
         "Materials come from " + Settlements()[city].name +
             "; output returns to these city resources. Each workshop has "
             "eight queue slots.",
         "book", "Change", [&, workshops] {
           auto it = std::find_if(
               workshops.begin(), workshops.end(),
               [&](const auto* f) { return f->id == ui.gov_facility; });
           ui.gov_facility =
               it == workshops.end()
                   ? (workshops.empty() ? 0 : workshops.front()->id)
               : ++it == workshops.end() ? 0
                                         : (*it)->id;
           ui.gov_page = 0;
         }});
    for (const auto& job : save.production) {
      if (job.source != city) {
        continue;
      }
      uint64_t id = job.id;
      const Recipe* recipe = nullptr;
      for (const auto& r : Recipes()) {
        if (r.id == job.recipe) {
          recipe = &r;
        }
      }
      const auto progress = GetProductionProgress(save, job);
      std::string detail = progress.state + " · ";
      if (progress.ahead > 0) {
        detail += "starts in " + Duration(progress.starts_in) + " · ";
      }
      detail += Duration(progress.remaining) + " left · " +
                std::to_string(static_cast<int>(progress.fraction * 100)) +
                "% · workshop #" + std::to_string(job.facility);
      rows.push_back(
          {recipe ? recipe->name : job.recipe,
           detail,
           recipe ? RecipeIcon(*recipe) : "book",
           "Cancel",
           [&, id] { act(CancelProduction(save, id)); },
           "",
           {},
           true,
           "Production advances while this world is open, including menus "
           "and an inactive/minimized window. Inputs are reserved; output "
           "returns to city resources.",
           {},
           progress.fraction});
      rows.back().id = "job:" + std::to_string(job.id);
    }
    int matches = 0;
    for (int i = 0; i < static_cast<int>(Recipes().size()); ++i) {
      const auto& recipe = Recipes()[i];
      if (lowercase(recipe.name).find(lowercase(ui.production_search)) ==
          std::string::npos) {
        continue;
      }
      ++matches;
      const auto status = CheckProduction(save, i, city, ui.gov_facility);
      const bool ready = status.missing.empty();
      std::string tooltip = recipe.name + "\n" + recipe.effect + "\n";
      if (ready) {
        tooltip += "Ready to queue. Resources are reserved from this city.";
      } else {
        tooltip += "Missing requirements:";
        for (const auto& missing : status.missing) {
          tooltip += "\n- " + missing;
        }
      }
      rows.push_back(
          {recipe.name,
           Duration(CraftDelay(recipe)) + " · resources: available / required",
           RecipeIcon(recipe),
           "Queue",
           [&, i] { act(QueueProduction(save, i, city, ui.gov_facility)); },
           "",
           {},
           ready,
           tooltip,
           status.requirements});
      rows.back().id = "recipe:" + recipe.id;
    }
    if (!matches) {
      rows.push_back({"No matching items", "Try another item name or Clear."});
    }
  } else if (ui.gov_tab == 3 || ui.gov_tab == 4) {
    bool trade = ui.gov_tab == 4;
    auto reward_label = [](const std::string& reward) {
      if (reward.starts_with("recipe:")) {
        for (const auto& r : Recipes()) {
          if (r.id == reward.substr(7)) {
            return "Recipe: " + r.name;
          }
        }
      }
      if (reward.starts_with("technology:")) {
        return std::string(TechnologyName(reward.substr(11)));
      }
      return reward == "housing" ? std::string("One housing place")
             : reward == "care"  ? std::string("Town medical supplies")
                                 : std::string("Town trust");
    };
    for (const auto& task : save.city_tasks) {
      if (task.completed || (task.kind == "trade") != trade ||
          (ui.town_hall && task.city != city) ||
          (!trade && task.city != city)) {
        continue;
      }
      uint64_t id = task.id;
      std::string detail = std::to_string(task.quantity) + " " +
                           GoodName(task.good) + " · " +
                           (trade ? std::to_string(task.price) + " coins"
                                  : "Reward: " + reward_label(task.reward));
      if (task.kind == "survey") {
        detail =
            "Discover two new towns after accepting · Reward: Civic charter";
      }
      if (task.kind.starts_with("legacy:")) {
        detail += " · migrated objective";
      }
      std::string objective = task.kind == "survey" ? "Town survey"
                              : trade               ? "Supply order"
                                                    : "Town supply project";
      rows.push_back({Settlements()[task.city].name + " · " + objective +
                          " · " + (task.accepted ? "Accepted" : "Available"),
                      detail, "book", task.accepted ? "Deliver" : "Accept",
                      [&, id, accepted = task.accepted] {
                        act(accepted ? FinishCityTask(save, id)
                                     : AcceptCityTask(save, id));
                      },
                      task.accepted ? "Abandon" : "",
                      task.accepted ? std::function<void()>([&, id] {
                        act(CancelCityTask(save, id));
                      })
                                    : std::function<void()>{}});
    }
    if (rows.empty()) {
      rows.push_back({trade ? "No open trade orders" : "No open town projects",
                      "Boards refresh from actual stock deficits. Accepted "
                      "tasks and incoming deliveries reserve the same need."});
    }
  } else if (ui.gov_tab == 5) {
    rows.push_back(
        {"State overview",
         "Owned towns " +
             std::to_string(
                 std::count(save.owners.begin(), save.owners.end(), 0)) +
             " · Active residents " + std::to_string(ActiveResidents(save)) +
             " · Regional market fund " + std::to_string(save.region_fund) +
             " · Pending shipments " + std::to_string(save.shipments.size()),
         "town"});
    auto reports = TownReports(save, MakeNpcs());
    for (int known : cities) {
      const auto& town = Settlements()[known];
      const auto& type = GetContent().towns[town.type];
      auto report =
          std::find_if(reports.begin(), reports.end(),
                       [&](const auto& r) { return r.city == known; });
      int population = report == reports.end() ? 0 : report->residents;
      rows.push_back({town.name + " · " + type.name,
                      "Residents " + std::to_string(population) + " · rarity " +
                          std::to_string(type.rarity) + " · " +
                          TreatyName(save.relations[known].treaty) + " · " +
                          TechnologyName(type.technology),
                      "town"});
      rows.push_back({town.name + " · infrastructure",
                      TownInfrastructure(save, known) + " · trust " +
                          std::to_string(save.relations[known].opinion),
                      "town", "View", [&, known] {
                        ui.gov_city = known;
                        ui.gov_tab = 1;
                        ui.gov_page = 0;
                      }});
    }
    if (OwnsCity(save, city)) {
      rows.push_back(
          {"City budget / state treasury",
           "Keep 40 coins in the town reserve; transfers stay inside the "
           "state.",
           "coins", "To state 10",
           [&] {
             if (save.treasury[city] < 50 || save.coins > 9999990) {
               act({false, "Town reserve or state limit prevents transfer"});
               return;
             }
             save.treasury[city] -= 10;
             save.coins += 10;
             act({true, "Transferred 10 coins to state treasury"});
           },
           "To town 10",
           [&] {
             if (save.coins < 10 || save.treasury[city] > 9999990) {
               act({false, "Insufficient state funds"});
               return;
             }
             save.coins -= 10;
             save.treasury[city] += 10;
             act({true, "Transferred 10 coins to town treasury"});
           }});
    }
    for (int from : cities) {
      if (from == city || !OwnsCity(save, city)) {
        continue;
      }
      if (OwnsCity(save, from)) {
        rows.push_back(
            {"Transfer from " + Settlements()[from].name,
             "Selected material: " + std::string(GoodName(ui.gov_good)) +
                 " · batch " + std::to_string(ui.gov_amount) +
                 " · requires a built road",
             "chest", "Ship",
             [&, from] {
               act(ShipGoods(save, from, city, ui.gov_good, ui.gov_amount));
             },
             "Material",
             [&] { ui.gov_good = (ui.gov_good + 1) % kGoodTypes; }});
      } else {
        const auto& type = GetContent().towns[Settlements()[from].type];
        for (int good : type.goods) {
          if (good < 6) {
            continue;
          }
          rows.push_back(
              {"Order " + std::string(GoodName(good)) + " from " +
                   Settlements()[from].name,
               "Price " + std::to_string(BuyPrice(from, good)) +
                   " each · batch " + std::to_string(ui.gov_amount) +
                   " · specialist knowledge and trust 20",
               "good_" + std::to_string(good), "Order", [&, from, good] {
                 act(OrderRareGoods(save, from, city, good, ui.gov_amount));
               }});
        }
      }
    }
    for (const auto& s : save.shipments) {
      rows.push_back({"Incoming: " + std::string(GoodName(s.good)) + " ×" +
                          std::to_string(s.quantity),
                      Settlements()[s.from].name + " → " +
                          Settlements()[s.to].name + " · " +
                          Duration(s.due - save.game_time),
                      "good_" + std::to_string(s.good)});
    }
    for (int i = 0; i < static_cast<int>(CityActions().size()); ++i) {
      if ((OwnsCity(save, city) && i < 5) || i == 8 ||
          (i >= 10 && i < 10 + static_cast<int>(Skills().size()))) {
        continue;
      }
      rows.push_back({CityActions()[i],
                      "Town action for " + Settlements()[city].name, "town",
                      "Perform", [&, i] { act(Diplomacy(save, city, i)); }});
    }
  } else if (ui.gov_tab == 6) {
    if (!OwnsCity(save, city)) {
      rows.push_back({"Choose an owned town",
                      "Growth, treatment and immigration are managed by the "
                      "town's ruler."});
    } else {
      rows.push_back(
          {"Town housing",
           std::to_string(CityPopulation(save, city)) + " residents / " +
               std::to_string(save.housing[city]) +
               " places · add four places: wood 20, stone 10, coins 10",
           "town", "Build", [&] { act(BuildHousing(save, city)); }});
      rows.push_back({"Change selected resident's home",
                      "Walk the resident to " + Settlements()[city].name +
                          "; requires a housing place and 5 berries",
                      "person", "Set home", [&] {
                        act(MoveResident(save, save.controlled_npc, city));
                      }});
      rows.push_back({"Hospital technology",
                      "Requires Medical care, school access and 80 state coins",
                      "book", "Research",
                      [&] { act(ResearchHospital(save)); }});
      for (int good : {0, 2, 3}) {
        rows.push_back({"Plant " + std::string(GoodName(good)),
                        "Cost: city wood 2 and berries 2 · trees 10m; "
                        "food/reeds 3m · territory limits plot count",
                        "good_" + std::to_string(good), "Plant",
                        [&, good] { act(PlantResource(save, city, good)); }});
      }
      for (int i = 0; i < static_cast<int>(save.plots.size()); ++i) {
        const auto& p = save.plots[i];
        if (p.city != city) {
          continue;
        }
        rows.push_back(
            {std::string(GoodName(p.good)) + " plot",
             p.ready ? "Ready to harvest; goods go to city resources"
                     : "Ready in " + Duration(p.due - save.game_time),
             "good_" + std::to_string(p.good), "Harvest",
             p.ready
                 ? std::function<void()>([&, i] { act(HarvestPlot(save, i)); })
                 : std::function<void()>{}});
      }
      for (const auto& t : save.travelers) {
        if (t.city != city || t.accepted || t.declined) {
          continue;
        }
        rows.push_back(
            {t.name,
             "Wants to settle here · needs one housing place and 5 berries",
             "person", "Accept",
             [&, npc = t.npc] { act(AcceptTraveler(save, npc)); }});
      }
      for (const auto& f : FacilityTypes()) {
        int count =
            std::count_if(save.facilities.begin(), save.facilities.end(),
                          [&](const auto& site) {
                            return site.city == city && site.kind == f.id;
                          });
        rows.push_back({f.name + " · " + std::to_string(count) + "/" +
                            std::to_string(FacilityLimit(save, city, f.id)),
                        "Build: city wood " + std::to_string(f.wood) +
                            ", stone " + std::to_string(f.stone) +
                            ", state coins " + std::to_string(f.coins),
                        "town", "Build", [&, id = f.id] {
                          act(BuildFacility(save, city, id));
                        }});
      }
      rows.push_back({"Expand town territory",
                      "Adds 150 radius; costs increase with each expansion",
                      "town", "Expand", [&] { act(ExpandCity(save, city)); }});
    }
  }
  const float base_height = std::max(72.f, TextSize(16) * 2 + 30);
  const float chip_height = TextSize(14) + 12;
  auto chip_width = [&](const ProductionRequirement& requirement) {
    return Measure(requirement.label, 14) + 42;
  };
  std::vector<float> heights;
  std::vector<std::vector<int>> page_rows(1);
  float occupied = 0;
  for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
    int lines = rows[i].requirements.empty() ? 0 : 1;
    float used = 0;
    const float available = body.width - 66 - (rows[i].primary ? 132 : 0);
    for (const auto& requirement : rows[i].requirements) {
      const float width = chip_width(requirement);
      if (used > 0 && used + width > available) {
        ++lines;
        used = 0;
      }
      used += width + 6;
    }
    const float height = std::min(
        body.height, std::max(base_height, lines ? TextSize(17) + TextSize(14) +
                                                       30 + lines * chip_height
                                                 : base_height));
    if (!page_rows.back().empty() && occupied + height > body.height) {
      page_rows.emplace_back();
      occupied = 0;
    }
    page_rows.back().push_back(i);
    occupied += height;
    heights.push_back(height);
  }
  const int pages = static_cast<int>(page_rows.size());
  if (IsKeyPressed(KEY_PAGE_DOWN)) {
    ++ui.gov_page;
  }
  if (IsKeyPressed(KEY_PAGE_UP)) {
    --ui.gov_page;
  }
  ui.gov_page = std::clamp(ui.gov_page - static_cast<int>(GetMouseWheelMove()),
                           0, pages - 1);
  std::string tooltip;
  float y = body.y;
  for (int i : page_rows[ui.gov_page]) {
    const auto& row = rows[i];
    const float step = heights[i];
    DrawRectangleRounded({body.x, y, body.width, step - 6}, .12f, 8,
                         {29, 48, 36, 255});
    if (!row.tooltip.empty() &&
        (CheckCollisionPointRec(GetMousePosition(),
                                {body.x, y, body.width, step - 6}) ||
         UiFocused({body.x + body.width - (row.secondary ? 260.f : 132.f),
                    y + 14, 120, 36}))) {
      tooltip = row.tooltip;
    }
    const float icon_size = row.requirements.empty() ? 32.f : 40.f;
    Icon(row.icon, {body.x + 6, y + 8, icon_size, icon_size},
         row.enabled ? WHITE : Color{127, 139, 132, 255});
    float actions = row.secondary ? 260 : row.primary ? 132 : 0;
    Wrapped(row.title, body.x + 50, y + 5, body.width - 66 - actions, 17,
            row.enabled ? Color{232, 213, 171, 255} : Color{137, 145, 140, 255},
            1);
    Wrapped(row.detail, body.x + 50, y + TextSize(17) + 11,
            body.width - 66 - actions, 14,
            row.enabled ? Color{173, 192, 160, 255} : Color{127, 136, 129, 255},
            1);
    float chip_x = body.x + 50;
    float chip_y = y + TextSize(17) + TextSize(14) + 23;
    BeginScissorMode(static_cast<int>(body.x), static_cast<int>(y),
                     static_cast<int>(body.width), static_cast<int>(step - 6));
    for (const auto& requirement : row.requirements) {
      const float width = chip_width(requirement);
      if (chip_x > body.x + 50 &&
          chip_x + width > body.x + body.width - 16 - actions) {
        chip_x = body.x + 50;
        chip_y += chip_height;
      }
      const Color tint = requirement.met ? Color{193, 215, 168, 255}
                                         : Color{117, 124, 119, 255};
      Icon(requirement.icon, {chip_x, chip_y + 1, 24, 24}, tint);
      Text(requirement.label, chip_x + 29, chip_y + 2, 14, tint);
      if (CheckCollisionPointRec(GetMousePosition(),
                                 {chip_x, chip_y, width, chip_height}) &&
          GetMousePosition().y < y + step - 6) {
        tooltip =
            requirement.detail +
            (requirement.met ? "\nRequirement met" : "\nMissing requirement");
      }
      chip_x += width + 6;
    }
    EndScissorMode();
    if (row.progress >= 0) {
      const float width = body.width - 66 - actions;
      const Rectangle track{body.x + 50, y + step - 15, width, 7};
      DrawRectangleRounded(track, 1, 6, {45, 62, 49, 255});
      if (row.progress > 0) {
        DrawRectangleRounded(
            {track.x, track.y, width * static_cast<float>(row.progress), 7}, 1,
            6, {163, 192, 121, 255});
      }
    }
    if (row.primary && button({body.x + body.width - actions, y + 14, 120, 36},
                              row.first, row.enabled, false, row.id)) {
      row.primary();
    }
    if (row.secondary &&
        button({body.x + body.width - 126, y + 14, 120, 36}, row.second)) {
      row.secondary();
    }
    y += step;
  }
  float bottom = h - 120;
  if (button({50, bottom, 120, 34}, "Previous")) {
    ui.gov_page = std::max(0, ui.gov_page - 1);
  }
  if (button({180, bottom, 120, 34}, "Next")) {
    ui.gov_page = std::min(pages - 1, ui.gov_page + 1);
  }
  Text(std::to_string(ui.gov_page + 1) + " / " + std::to_string(pages), 320,
       bottom + 6, 16, {177, 194, 161, 255});
  Wrapped(ui.gov_status.empty()
              ? "Arrows: tabs · Tab / Shift+Tab: focus · Enter: activate · "
                "PgUp/PgDn: pages · Esc: close"
              : ui.gov_status,
          50, h - 77, w - 100, 16, {238, 202, 133, 255}, 1);
  if (!tooltip.empty()) {
    const float width = std::min(760.f, w - 60);
    const float line_height = TextSize(16) + 6;
    std::vector<std::string> lines;
    std::istringstream paragraphs(tooltip);
    for (std::string paragraph; std::getline(paragraphs, paragraph);) {
      std::istringstream words(paragraph);
      std::string line;
      for (std::string word; words >> word;) {
        if (!line.empty() && Measure(line + " " + word, 16) > width - 28) {
          lines.push_back(line);
          line.clear();
        }
        line += (line.empty() ? "" : " ") + word;
      }
      lines.push_back(line);
    }
    const int limit = std::max(1, static_cast<int>((h - 64) / line_height));
    if (static_cast<int>(lines.size()) > limit) {
      lines.resize(limit);
      lines.back() = "...";
    }
    const float height = lines.size() * line_height + 24;
    const auto mouse = UiFocusActive() ? Vector2{body.x, body.y + body.height}
                                       : GetMousePosition();
    const float x = std::clamp(mouse.x + 18, 20.f, w - width - 20);
    const float y = std::clamp(mouse.y + 18, 20.f, h - height - 20);
    DrawRectangleRounded({x, y, width, height}, .05f, 8, {12, 23, 19, 255});
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
      Text(lines[i], x + 14, y + 12 + i * line_height, 16,
           i == 0 ? Color{233, 204, 144, 255} : Color{202, 211, 200, 255});
    }
  }
}
}  // namespace frontier
