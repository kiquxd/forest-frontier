#include <algorithm>
#include <cmath>
#include <cstddef>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "input.hpp"
#include "nation.hpp"
#include "progress.hpp"
#include "render.hpp"
#include "settlement_system.hpp"

namespace frontier {
namespace {
constexpr Color kInk{228, 232, 214, 255};
constexpr Color kMuted{151, 174, 150, 255};
constexpr Color kGold{231, 189, 115, 255};
constexpr float kRowHeight = 64;
constexpr float kCardWidth = 162;
constexpr float kColumnStep = 184;
Rectangle BodyOf(Rectangle box) {
  return {box.x + 4, box.y + 58, box.width - 8, box.height - 58};
}
void ScrollBar(Rectangle box, float scroll, float total) {
  if (total <= box.height) {
    return;
  }
  float height = std::max(22.f, box.height * box.height / total);
  DrawRectangleRounded({box.x + box.width - 5, box.y, 4, box.height}, 1, 6,
                       {43, 63, 48, 255});
  DrawRectangleRounded(
      {box.x + box.width - 5,
       box.y + scroll / (total - box.height) * (box.height - height), 4,
       height},
      1, 6, {116, 153, 96, 255});
}
}  // namespace
void Renderer::Icon(const std::string& id, Rectangle box, Color tint) const {
  auto found = icon_ids_.find(id);
  if (found == icon_ids_.end()) {
    found = icon_ids_.find("book");
  }
  if (found == icon_ids_.end() || !icons_.id) {
    return;
  }
  int n = found->second;
  DrawTexturePro(icons_,
                 {static_cast<float>(n % 16 * 64),
                  static_cast<float>(n / 16 * 64), 64, 64},
                 box, {0, 0}, 0, tint);
}
void Renderer::SummaryInput(SummaryView& view, Rectangle box,
                            bool keyboard) const {
  view.keyboard = keyboard;
  if (keyboard && !view.editable && view.tab != 4 &&
      (MenuLeft() || MenuRight())) {
    view.tab =
        (view.tab + (MenuLeft() || IsKeyDown(KEY_LEFT_SHIFT) ? 4 : 1)) % 5;
    view.scroll = 0;
    view.horizontal = 0;
  }
  if (keyboard && IsKeyPressed(KEY_PAGE_DOWN)) {
    view.scroll += box.height * .7f;
  }
  if (keyboard && IsKeyPressed(KEY_PAGE_UP)) {
    view.scroll -= box.height * .7f;
  }
  if (CheckCollisionPointRec(GetMousePosition(), box)) {
    float wheel = GetMouseWheelMove();
    if (view.tab == 4 && IsKeyDown(KEY_LEFT_SHIFT)) {
      view.horizontal -= wheel * 70;
    } else {
      view.scroll -= wheel * 70;
    }
    if (view.tab == 4 && IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
      auto delta = GetMouseDelta();
      view.horizontal -= delta.x;
      view.scroll -= delta.y;
    }
  }
  view.scroll = std::max(0.f, view.scroll);
  view.horizontal = std::max(0.f, view.horizontal);
}
void Renderer::DrawSummary(const Save& save, SummaryView& view, Rectangle box,
                           bool enabled, const std::vector<Npc>* npcs) {
  const bool state_tree = save.nation && view.editable;
  if (state_tree) {
    view.tab = 4;
    Wrapped("State knowledge · " + ActionKey(InputAction::kNation) +
                ": towns · " + ActionKey(InputAction::kInventory) + ": cargo",
            box.x + 8, box.y + 12, box.width - 16, 18, kInk, 1);
  }
  const char* tabs[] = {"Overview", "World", "Resources", "Settlements",
                        "Skill tree"};
  float tab_width = box.width / 5;
  for (int i = 0; !state_tree && i < 5; i++) {
    Rectangle tab{box.x + i * tab_width, box.y, tab_width - 5, 44};
    bool hover = enabled && CheckCollisionPointRec(GetMousePosition(), tab);
    DrawRectangleRounded(tab, .55f, 12,
                         view.tab == i ? Color{81, 108, 65, 255}
                         : hover       ? Color{52, 78, 55, 255}
                                       : Color{33, 51, 42, 255});
    Wrapped(tabs[i], tab.x + 10, tab.y + 10, tab.width - 20, 16, kInk, 1);
    if (UiActivate(tab, enabled)) {
      view.tab = i;
      view.scroll = view.horizontal = 0;
    }
  }
  auto body = BodyOf(box);
  if (view.tab == 3) {
    auto generated = npcs ? std::vector<Npc>{} : MakeNpcs();
    DrawSettlements(save, view, body, enabled, npcs ? *npcs : generated);
    return;
  }
  if (view.tab == 4) {
    auto cards = ProgressCards(save);
    const float card_height = std::max(126.f, 90 + TextSize(16) * 2);
    const float row_step = card_height + 26;
    if (view.selected_node < 0) {
      view.selected_node =
          static_cast<int>(std::find_if(cards.begin(), cards.end(),
                                        [](const auto& card) {
                                          return card.id == "gathering";
                                        }) -
                           cards.begin());
    }
    view.selected_node =
        std::clamp(view.selected_node, 0, static_cast<int>(cards.size()) - 1);
    if (enabled && view.keyboard &&
        (MenuUp() || MenuDown() || (MenuLeft() || MenuRight()))) {
      const int dx = MenuRight() ? 1 : MenuLeft() ? -1 : 0;
      const int dy = MenuDown() ? 1 : MenuUp() ? -1 : 0;
      view.selected_node =
          AdjacentProgressCard(cards, view.selected_node, dx, dy);
      const auto& node = cards[view.selected_node];
      view.scroll = std::max(0.f, node.row * row_step - 40);
      view.horizontal = std::max(0.f, node.column * kColumnStep - 40);
    }
    Wrapped("Arrows: select · Enter: learn · SHIFT+wheel: pan", body.x, body.y,
            body.width - 12, 16, kMuted, 1);
    Rectangle canvas{body.x, body.y + 30, body.width, body.height - 174};
    int columns = 0, rows = 0;
    for (const auto& card : cards) {
      columns = std::max(columns, card.column + 1);
      rows = std::max(rows, card.row + 1);
    }
    float total_width = columns * kColumnStep, total_height = rows * row_step;
    view.scroll = std::clamp(view.scroll, 0.f,
                             std::max(0.f, total_height - canvas.height));
    view.horizontal = std::clamp(view.horizontal, 0.f,
                                 std::max(0.f, total_width - canvas.width));
    auto position = [&](const ProgressCard& card) {
      return Rectangle{
          canvas.x + 8 + card.column * kColumnStep - view.horizontal,
          canvas.y + 4 + card.row * row_step - view.scroll, kCardWidth,
          card_height};
    };
    BeginScissorMode(static_cast<int>(canvas.x), static_cast<int>(canvas.y),
                     static_cast<int>(canvas.width),
                     static_cast<int>(canvas.height));
    for (const auto& card : cards) {
      for (const auto& dependency : card.dependencies) {
        auto parent = std::find_if(
            cards.begin(), cards.end(),
            [&](const auto& item) { return item.id == dependency; });
        if (parent == cards.end()) {
          continue;
        }
        auto a = position(*parent), b = position(card);
        const bool same_column = card.column == parent->column;
        Vector2 from{a.x + a.width, a.y + a.height * .5f},
            to{same_column ? b.x + b.width : b.x, b.y + b.height * .5f};
        Color color = parent->learned ? Color{117, 173, 102, 255}
                                      : Color{58, 90, 63, 255};
        if (same_column) {
          // A vertical dependency must not run through unrelated cells between
          // its endpoints and imply that those cells are prerequisites too.
          const float gutter = from.x + 10;
          DrawLineEx(from, {gutter, from.y}, 2, color);
          DrawLineEx({gutter, from.y}, {gutter, to.y}, 2, color);
          DrawLineEx({gutter, to.y}, to, 2, color);
        } else if (card.column - parent->column > 1) {
          // Route long edges through row/column gutters instead of behind
          // unrelated cells.
          float gutter = b.y + b.height + 12;
          const Vector2 points[] = {from,
                                    {from.x + 10, from.y},
                                    {from.x + 10, gutter},
                                    {to.x - 10, gutter},
                                    {to.x - 10, to.y},
                                    to};
          for (int p = 0; p < 5; p++) {
            DrawLineEx(points[p], points[p + 1], 2, color);
          }
        } else {
          float middle = (from.x + to.x) * .5f;
          DrawLineEx(from, {middle, from.y}, 2, color);
          DrawLineEx({middle, from.y}, {middle, to.y}, 2, color);
          DrawLineEx({middle, to.y}, to, 2, color);
        }
        const float arrow_x = to.x + (same_column ? 6 : -6);
        DrawLineEx({arrow_x, to.y - 4}, to, 2, color);
        DrawLineEx({arrow_x, to.y + 4}, to, 2, color);
      }
    }
    for (int i = 0; i < static_cast<int>(cards.size()); i++) {
      const auto& card = cards[i];
      auto cell = position(card);
      if (cell.y + cell.height < canvas.y ||
          cell.y > canvas.y + canvas.height || cell.x + cell.width < canvas.x ||
          cell.x > canvas.x + canvas.width) {
        continue;
      }
      DrawRectangleRounded(
          cell, .15f, 10,
          card.learned ? Color{88, 128, 69, 255} : Color{26, 57, 38, 255});
      if (i == view.selected_node) {
        DrawRectangleRoundedLinesEx(cell, .15f, 10, 2, kGold);
      }
      Icon(card.icon, {cell.x + (cell.width - 64) * .5f, cell.y + 8, 64, 64},
           card.learned ? WHITE : Color{158, 177, 157, 255});
      Wrapped(card.activity
                  ? card.name + "\n" + std::to_string(card.amount) + " recorded"
                  : card.name,
              cell.x + 8, cell.y + 78, cell.width - 16, 16, kInk, 2);
      Text(card.learned ? "OK"
           : card.ready ? "+"
                        : "-",
           cell.x + 6, cell.y + 6, 16, kGold);
      if (ActiveSettings().high_contrast) {
        DrawRectangleRoundedLinesEx(cell, .15f, 10, 2,
                                    card.learned ? Color{187, 238, 157, 255}
                                                 : Color{154, 163, 150, 255});
      }
      if (card.ready && !card.learned) {
        DrawCircleV({cell.x + cell.width - 12, cell.y + 12}, 4, kGold);
      }
      if (UiActivate(GetCollisionRec(cell, canvas), enabled, card.id)) {
        view.selected_node = i;
      }
    }
    EndScissorMode();
    ScrollBar(canvas, view.scroll, total_height);
    if (total_width > canvas.width) {
      float length = canvas.width * canvas.width / total_width;
      DrawRectangle(static_cast<int>(canvas.x),
                    static_cast<int>(canvas.y + canvas.height - 4),
                    static_cast<int>(canvas.width), 4, {43, 63, 48, 255});
      DrawRectangle(
          static_cast<int>(canvas.x + view.horizontal /
                                          (total_width - canvas.width) *
                                          (canvas.width - length)),
          static_cast<int>(canvas.y + canvas.height - 4),
          static_cast<int>(length), 4, kGold);
    }
    const auto& selected = cards[view.selected_node];
    Wrapped(selected.name + " · " +
                (selected.activity
                     ? (selected.amount > 0 ? "recorded" : "not started")
                 : selected.learned ? "learned"
                 : selected.ready   ? "ready"
                                    : "locked"),
            body.x, body.y + body.height - 138, body.width - 12, 18, kGold, 1);
    Wrapped(selected.detail, body.x, body.y + body.height - 106,
            body.width - 12, 16, kInk, view.editable ? 2 : 4);
    if (view.editable) {
      bool can_unlock =
          !selected.activity && !selected.learned && selected.ready;
      bool direct =
          std::any_of(
              Skills().begin(), Skills().end(),
              [&](const auto& skill) { return skill.id == selected.id; }) ||
          std::any_of(
              ResearchNodes().begin(), ResearchNodes().end(),
              [&](const auto& research) { return research.id == selected.id; });
      Rectangle action{body.x, body.y + body.height - 38,
                       std::min(310.f, body.width), 36};
      DrawRectangleRounded(action, .5f, 10,
                           can_unlock && direct ? Color{81, 108, 65, 255}
                                                : Color{37, 51, 42, 255});
      Wrapped(selected.learned ? "Already learned"
              : direct         ? "Learn / research · Enter"
                               : "Visit teacher / complete quest",
              action.x + 10, action.y + 7, action.width - 20, 16, kInk, 1);
      if (enabled && can_unlock && direct &&
          (MenuConfirm() || UiActivate(action))) {
        view.requested_node = selected.id;
      }
      if (!view.status.empty()) {
        Wrapped(view.status, action.x + action.width + 12, action.y + 6,
                body.width - action.width - 24, 16, kGold, 1);
      }
    }
    return;
  }
  std::vector<SummaryEntry> entries;
  if (view.tab == 0 && view.history) {
    entries.push_back({"book", "Recent campaign history",
                       "Last 512 events; older saves start recording now."});
    for (auto it = save.history.rbegin(); it != save.history.rend(); ++it) {
      std::time_t stamp = static_cast<std::time_t>(it->timestamp);
      std::tm local{};
      localtime_r(&stamp, &local);
      std::ostringstream time;
      time << std::put_time(&local, "%Y-%m-%d %H:%M");
      entries.push_back(
          {"book", it->description, it->kind + " · " + time.str()});
    }
  } else if (view.tab == 0) {
    int discovered = 0, owned = 0, done = 0;
    for (size_t i = 0; i < save.relations.size(); i++) {
      discovered += save.relations[i].discovered;
      owned += OwnsCity(save, static_cast<int>(i));
    }
    for (const auto& [id, q] : save.quests) {
      done += q.completed;
    }
    if (save.nation) {
      done = std::count_if(
          save.city_tasks.begin(), save.city_tasks.end(),
          [](const auto& t) { return t.completed && t.accepted; });
    }
    entries = {
        {"town",
         "Known towns: " + std::to_string(discovered) + " / " +
             std::to_string(Settlements().size()),
         "Owned towns: " + std::to_string(owned)},
        {"town", "Home: " + Settlements().at(save.home_city).name,
         "Position: " + std::to_string(static_cast<int>(save.player.x)) + ", " +
             std::to_string(static_cast<int>(save.player.y))},
        {"heart", "Health", "", save.health},
        {"coins", "Coins", "", save.coins},
        {"map", "Explored map",
         "Surface cells: " + std::to_string(save.explored.size()) +
             " · seafloor: " + std::to_string(save.explored_underwater.size())},
        {"book",
         "Learned skills: " + std::to_string(save.skills.size()) + " / " +
             std::to_string(Skills().size()),
         "Technologies: " + std::to_string(save.technologies.size())},
        {"person",
         (save.nation
              ? "Selected resident: " +
                    ResidentName(save, save.controlled_npc) + " · State trait: "
              : "Background: ") +
             PlayerCharacter(save).name,
         "Residents: " + std::to_string(save.citizens.size())},
        {"book", "Completed quests: " + std::to_string(done),
         save.nation
             ? "Town projects and orders are managed in the state / town hall "
               "menu"
             : "Accepted quests: " + std::to_string(save.quests.size())},
        {"gather_sites", "Gathered sites: " + std::to_string(save.gathered),
         "Crafted items: " + std::to_string(save.crafted) +
             " · fish caught: " + std::to_string(save.caught)},
        {"town_orders", "Town orders: " + std::to_string(save.orders),
         "Walls: " + std::to_string(save.walls.size()) +
             " · campfires: " + std::to_string(save.campfires.size())},
        {"map", "World seed: " + std::to_string(save.seed),
         save.submerged ? "Current layer: underwater"
                        : "Current layer: surface"}};
    entries.push_back(
        {"book", "Recent history", "Click this row to view recorded events"});
    if (!view.caption.empty()) {
      entries.push_back({"book", view.caption, ""});
    }
  } else if (view.tab == 1) {
    Rectangle layer{body.x, body.y, std::min(300.f, body.width - 8), 36};
    DrawRectangleRounded(layer, .5f, 10, {52, 78, 55, 255});
    Text(view.underwater ? "Seafloor · switch layer" : "Surface · switch layer",
         layer.x + 10, layer.y + 7, 16, kInk);
    if (UiActivate(layer, enabled)) {
      view.underwater = !view.underwater;
      view.scroll = 0;
    }
    float size = std::min({270.f, body.width - 12, body.height * .45f});
    DrawSavedMap(save, {body.x, body.y + 45, size, size},
                 view.underwater ? 1 : 0);
    body.y += size + 58;
    body.height -= size + 58;
    for (size_t city = 0; city < Settlements().size(); city++) {
      if (city < save.relations.size() && save.relations[city].discovered) {
        const auto& town = Settlements()[city];
        const auto& relation = save.relations[city];
        entries.push_back(
            {"town",
             town.name +
                 (OwnsCity(save, static_cast<int>(city))
                      ? " · owned"
                      : " · " + std::string(TreatyName(relation.treaty))),
             "Position: " + std::to_string(static_cast<int>(town.pos.x)) +
                 ", " + std::to_string(static_cast<int>(town.pos.y)) +
                 " · trust " + std::to_string(relation.opinion)});
      }
    }
  } else if (view.tab == 2) {
    entries = ResourceCatalog(save);
    for (const auto& item : InventorySummary(save)) {
      if (!item.icon.starts_with("good_")) {
        entries.push_back(item);
      }
    }
  }
  float total = entries.size() * kRowHeight;
  view.scroll =
      std::clamp(view.scroll, 0.f, std::max(0.f, total - body.height));
  BeginScissorMode(static_cast<int>(body.x), static_cast<int>(body.y),
                   static_cast<int>(body.width), static_cast<int>(body.height));
  for (int i = 0; i < static_cast<int>(entries.size()); i++) {
    float y = body.y + i * kRowHeight - view.scroll;
    if (y + kRowHeight < body.y || y > body.y + body.height) {
      continue;
    }
    const auto& entry = entries[i];
    if (view.tab == 0 &&
        (entry.name == "Recent history" ||
         entry.name == "Recent campaign history") &&
        UiActivate({body.x, y, body.width - 12, kRowHeight - 5}, enabled)) {
      view.history = !view.history;
      view.scroll = 0;
    }
    DrawRectangleRounded(
        {body.x, y, body.width - 12, kRowHeight - 5}, .12f, 8,
        i % 2 ? Color{25, 43, 34, 255} : Color{29, 47, 37, 255});
    Icon(entry.icon, {body.x + 8, y + 12, 32, 32});
    float count_width = entry.count >= 0 ? 100 : 0;
    Wrapped(entry.name, body.x + 52, y + 7, body.width - 72 - count_width, 18,
            kInk, 1);
    if (entry.count >= 0) {
      auto count = std::to_string(entry.count);
      Text(count, body.x + body.width - 25 - Measure(count, 18), y + 8, 18,
           kGold);
    }
    if (!entry.detail.empty()) {
      Wrapped(entry.detail, body.x + 52, y + 34, body.width - 72, 16, kMuted,
              1);
    }
  }
  EndScissorMode();
  ScrollBar(body, view.scroll, total);
}
void Renderer::DrawSettlements(const Save& save, SummaryView& view,
                               Rectangle body, bool enabled,
                               const std::vector<Npc>& npcs) {
  auto reports = TownReports(save, npcs);
  if (reports.empty()) {
    Wrapped("Explore to discover settlements.", body.x, body.y, body.width, 20,
            kInk, 2);
    return;
  }
  view.selected_city =
      std::clamp(view.selected_city, 0, static_cast<int>(reports.size()) - 1);
  if (enabled && view.keyboard && (MenuUp() || MenuDown())) {
    view.selected_city =
        (view.selected_city + static_cast<int>(reports.size()) +
         (MenuUp() ? -1 : 1)) %
        static_cast<int>(reports.size());
    view.selected_resident = -1;
    view.resident_page = 0;
  }
  float list_width = std::min(220.f, body.width * .32f);
  int shown = std::max(1, static_cast<int>(body.height / 64));
  int first = view.selected_city / shown * shown;
  for (int i = first;
       i < std::min(first + shown, static_cast<int>(reports.size())); ++i) {
    Rectangle row{body.x, body.y + (i - first) * 64.f, list_width - 8, 58};
    DrawRectangleRounded(row, .18f, 8,
                         i == view.selected_city ? Color{76, 100, 61, 255}
                                                 : Color{28, 48, 37, 255});
    Icon("town", {row.x + 8, row.y + 9, 32, 32});
    Wrapped(reports[i].name, row.x + 46, row.y + 7, row.width - 52, 17, kInk,
            1);
    Wrapped(reports[i].owned
                ? "Your town"
                : TreatyName(save.relations[reports[i].city].treaty),
            row.x + 46, row.y + 32, row.width - 52, 16, kMuted, 1);
    if (UiActivate(row, enabled)) {
      view.selected_city = i;
      view.selected_resident = -1;
      view.resident_page = 0;
    }
  }
  const auto& report = reports[view.selected_city];
  int city = report.city;
  float x = body.x + list_width + 8, width = body.width - list_width - 14;
  const auto& town = Settlements()[city];
  const auto& type = GetContent().towns[town.type];
  Wrapped(
      report.name + " · " + report.specialty + " · " + RarityName(town.rarity),
      x, body.y, width, 20, kGold, 2);
  Wrapped(std::string(report.owned ? "Owned by you" : "Independent ruler") +
              " · residents " + std::to_string(report.residents) +
              " · visitors " + std::to_string(report.visitors),
          x, body.y + 58, width, 16, kInk, 2);
  Wrapped("Trust " + std::to_string(save.relations[city].opinion) + " · " +
              TreatyName(save.relations[city].treaty) + " · walls " +
              std::to_string(report.walls) + " (HP " +
              std::to_string(report.wall_health) + ")",
          x, body.y + 105, width, 16, kInk, 2);
  std::string goods = "Specialty goods: ";
  for (int good : type.goods) {
    goods += std::string(GoodName(good)) + " · ";
  }
  Wrapped(goods + " Knowledge: " + report.technology, x, body.y + 152, width,
          16, kMuted, 3);
  Wrapped(TownInfrastructure(save, city), x, body.y + 219, width, 15, kMuted,
          2);
  if (!report.owned) {
    Wrapped(
        "Visit the ruler for diplomacy and the town master for lessons. Jobs "
        "and equipment belong to this independent town.",
        x, body.y + 266, width, 17, kInk, 4);
    return;
  }
  std::vector<const Citizen*> residents;
  for (const auto& citizen : save.citizens) {
    if (citizen.city == city) {
      residents.push_back(&citizen);
    }
  }
  std::sort(residents.begin(), residents.end(),
            [](const auto* a, const auto* b) { return a->npc < b->npc; });
  if (residents.empty()) {
    return;
  }
  const float resident_step = std::max(46.f, TextSize(16) * 2 + 12);
  int roster_rows =
      std::max(1, static_cast<int>((body.height - 375) / resident_step));
  int pages =
      (static_cast<int>(residents.size()) + roster_rows - 1) / roster_rows;
  view.resident_page = std::clamp(view.resident_page, 0, pages - 1);
  Rectangle prev{x, body.y + 263, 90, TextSize(16) + 10},
      next{x + 100, body.y + 263, 90, TextSize(16) + 10};
  auto button = [&](Rectangle box, const std::string& title) {
    DrawRectangleRounded(box, .4f, 8, {49, 76, 52, 255});
    Wrapped(title, box.x + 6, box.y + 5, box.width - 12, 16, kInk, 1);
    return UiActivate(box, enabled);
  };
  if (button(prev, "Previous")) {
    view.resident_page = std::max(0, view.resident_page - 1);
  }
  if (button(next, "Next")) {
    view.resident_page = std::min(pages - 1, view.resident_page + 1);
  }
  for (int i = view.resident_page * roster_rows;
       i < std::min((view.resident_page + 1) * roster_rows,
                    static_cast<int>(residents.size()));
       ++i) {
    const auto& resident = *residents[i];
    auto npc = std::find_if(npcs.begin(), npcs.end(), [&](const auto& n) {
      return n.id == resident.npc;
    });
    float y =
        body.y + 301 + (i - view.resident_page * roster_rows) * resident_step;
    Rectangle row{x, y, width, resident_step - 4};
    DrawRectangleRounded(row, .12f, 8,
                         view.selected_resident == resident.npc
                             ? Color{71, 96, 57, 255}
                             : Color{29, 48, 37, 255});
    Wrapped((npc == npcs.end() ? "Resident " + std::to_string(resident.npc)
                               : npc->name) +
                " · " + resident.state + " · " + resident.job,
            x + 8, y + 3, width - 16, 16, kInk, 1);
    Wrapped(
        (resident.weapon.empty() ? "No weapon" : Weapon(resident.weapon).name) +
            " · " +
            (resident.outfit.empty() ? "No work clothes"
                                     : Outfit(resident.outfit).name),
        x + 8, y + TextSize(16) + 5, width - 16, 16, kMuted, 1);
    if (UiActivate(row, enabled)) {
      view.selected_resident = resident.npc;
      auto weapons = OwnedWeapons(save), outfits = OwnedOutfits(save);
      outfits.insert(outfits.begin(), "");
      view.resident_job =
          static_cast<int>(std::find(ResidentJobs().begin(),
                                     ResidentJobs().end(), resident.job) -
                           ResidentJobs().begin());
      view.resident_weapon = static_cast<int>(
          std::find(weapons.begin(), weapons.end(), resident.weapon) -
          weapons.begin());
      view.resident_outfit = static_cast<int>(
          std::find(outfits.begin(), outfits.end(), resident.outfit) -
          outfits.begin());
    }
  }
  if (view.editable && view.selected_resident >= 0 &&
      GetCitizen(save, view.selected_resident) &&
      GetCitizen(save, view.selected_resident)->city == city) {
    const auto* selected = GetCitizen(save, view.selected_resident);
    if (selected->state != "active") {
      float y = body.y + body.height - 70;
      if (selected->state == "captive") {
        if (button({x, y, width, 42}, "Ransom · " +
                                          std::to_string(selected->ransom) +
                                          " coins · gear returns")) {
          view.requested_ransom = selected->npc;
        }
      } else {
        Wrapped(
            "This resident died. Assigned gear was destroyed; they cannot be "
            "reassigned.",
            x, y, width, 16, kMuted, 2);
      }
      return;
    }
    auto weapons = OwnedWeapons(save), outfits = OwnedOutfits(save);
    outfits.insert(outfits.begin(), "");
    view.resident_job = std::clamp(view.resident_job, 0,
                                   static_cast<int>(ResidentJobs().size()) - 1);
    view.resident_weapon = std::clamp(view.resident_weapon, 0,
                                      static_cast<int>(weapons.size()) - 1);
    view.resident_outfit = std::clamp(view.resident_outfit, 0,
                                      static_cast<int>(outfits.size()) - 1);
    float y = body.y + body.height - 76;
    if (button({x, y, width * .32f - 4, 32},
               ResidentJobs()[view.resident_job]) ||
        (enabled && ActionPressed(InputAction::kResidentJob))) {
      view.resident_job =
          (view.resident_job + 1) % static_cast<int>(ResidentJobs().size());
    }
    if (button({x + width * .32f, y, width * .34f - 4, 32},
               weapons[view.resident_weapon].empty()
                   ? "No weapon"
                   : Weapon(weapons[view.resident_weapon]).name) ||
        (enabled && ActionPressed(InputAction::kResidentWeapon))) {
      view.resident_weapon =
          (view.resident_weapon + 1) % static_cast<int>(weapons.size());
    }
    if (button({x + width * .66f, y, width * .34f - 4, 32},
               outfits[view.resident_outfit].empty()
                   ? "No clothes"
                   : Outfit(outfits[view.resident_outfit]).name) ||
        (enabled && ActionPressed(InputAction::kResidentOutfit))) {
      view.resident_outfit =
          (view.resident_outfit + 1) % static_cast<int>(outfits.size());
    }
    if (button({x, y + 40, std::min(220.f, width), 32}, "Assign · Enter") ||
        (enabled && MenuConfirm())) {
      view.requested_assignment = true;
    }
    if (!view.status.empty()) {
      Wrapped(view.status, x + 230, y + 42, std::max(0.f, width - 230), 16,
              kGold, 1);
    }
  }
}
}  // namespace frontier
