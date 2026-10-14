#ifndef FOREST_FRONTIER_SRC_RENDER_HPP_
#define FOREST_FRONTIER_SRC_RENDER_HPP_

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "gameplay.hpp"
#include "raylib.h"
#include "summary.hpp"
#include "world.hpp"

namespace frontier {
struct SummaryView {
  int tab{};
  int selected_node{-1};
  float scroll{};
  float horizontal{};
  bool underwater{};
  std::string caption;
  bool editable{};
  bool keyboard{true};
  bool history{};
  int selected_city{};
  int selected_resident{-1};
  int resident_job{};
  int resident_weapon{};
  int resident_outfit{};
  int resident_page{};
  std::string requested_node;
  bool requested_assignment{};
  std::string status;
};
struct Ui {
  int wood{};
  int stone{};
  int berries{};
  int npc{-1};
  std::string hint;
  std::string notice;
  std::string input;
  std::string reply;
  bool busy{};
  bool online{};
  bool debug{true};
  bool benchmark{};
  float harvest_progress{};
  float action_bar_top{};
  double fps{};
  double p95{};
  double p99{};
  double work_ms{};
  double time{};
  size_t chunks{};
  size_t pending{};
  size_t visible{};
  size_t animals{};
  bool crafting{};
  bool trading{};
  int selection{};
  int merchant{-1};
  bool civic{};
  bool equipment{};
  bool cartography{};
  bool progression{};
  bool wardrobe{};
  bool journal{};
  bool paused{};
  int pause_selection{};
  std::string pause_error;
  bool temporary_session{};
  bool settings_open{};
  std::vector<std::pair<std::string, std::string>> actions;
  SummaryView summary;
  int resident_job{};
  int resident_weapon{};
  int resident_outfit{};
  bool Blocked() const {
    return paused || npc >= 0 || crafting || trading || civic || equipment ||
           cartography || progression || wardrobe || journal;
  }
  Vec map_center;
  float map_range{4000};
  int map_layer_override{-1};
};
class Renderer {
 public:
  Renderer();
  ~Renderer();
  size_t DrawWorld(const World& world, const Save& save, const Gameplay& game,
                   const Camera2D& camera, double time, float alpha,
                   const Resource* highlighted);
  void DrawLabels(const World& world, const Save& save, const Ui& ui,
                  const Camera2D& camera) const;
  static Rectangle MenuBounds(int rows);
  static Rectangle PauseBounds();
  static Rectangle PauseButton(int choice);
  static Rectangle PauseSummaryBounds();
  static Rectangle ProgressSummaryBounds();
  static Rectangle ProgressCloseButton();
  static int MenuPageRows();
  static constexpr float kMenuRowHeight = 50;
  static constexpr float kMenuListTop = 102;
  static constexpr float kMenuFooterHeight = 155;
  static float TextSize(float size);
  void DrawUi(const World& world, const Save& save, Ui& ui,
              const Camera2D& camera);
  void DrawSavedMap(const Save& save, Rectangle box, int layer = -1);
  void SummaryInput(SummaryView& view, Rectangle box,
                    bool keyboard = true) const;
  void DrawSummary(const Save& save, SummaryView& view, Rectangle box,
                   bool enabled = true, const std::vector<Npc>* npcs = nullptr);
  void Text(const std::string& s, float x, float y, float size,
            Color color) const;
  void Wrapped(const std::string& s, float x, float y, float width, float size,
               Color color, int max_lines = 8) const;
  float Measure(const std::string& s, float size) const;

 private:
  Texture2D atlas_{};
  Texture2D icons_{};
  std::unordered_map<std::string, int> icon_ids_;
  void Icon(const std::string& id, Rectangle box, Color tint = WHITE) const;
  struct SizedFont {
    int size;
    Font font;
    bool owned;
  };
  std::vector<SizedFont> fonts_;
  const Font& GetFont(float size) const;
  std::vector<Rectangle> sprites_;
  struct Item {
    Vec pos;
    int sprite;
    Color tint;
    bool flip{};
  };
  std::vector<Item> items_;
  void Sprite(int index, Vec position, Color tint = WHITE,
              bool flip = false) const;
  bool Visible(Vec position, const Camera2D& c, float margin = 180) const;
  void Vehicle(const Save& save, const Gameplay& game, Vec position,
               double time) const;
  struct MapCache {
    Texture2D texture{};
    Vec center;
    float range{};
    double refresh{-1};
    bool submerged{};
    uint64_t seed{};
  };
  std::array<MapCache, 2> maps_{};
  void DrawSettlements(const Save& save, SummaryView& view, Rectangle body,
                       bool enabled, const std::vector<Npc>& npcs);
  void DrawMap(const Save& save, const Ui& ui, const Camera2D& camera,
               Rectangle box, bool large, bool viewport = true);
};
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_RENDER_HPP_
