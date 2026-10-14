#pragma once
#include "world.hpp"
#include "dialogue.hpp"
#include "gameplay.hpp"
#include "raylib.h"
#include <array>
#include "summary.hpp"

namespace frontier {
struct SummaryView {
  int tab{},selectedNode{-1};
  float scroll{},horizontal{};
  bool underwater{};
  std::string caption;
};
struct Ui {
  int wood{},stone{},berries{},npc{-1};
  std::string hint,notice,input,reply;
  bool busy{},online{},debug{true},benchmark{};
  float harvestProgress{};
  double fps{},p95{},p99{},workMs{},time{};
  size_t chunks{},pending{},visible{},animals{};
  bool crafting{},trading{};
  int selection{},merchant{-1};
  bool civic{},equipment{},cartography{},progression{},wardrobe{},journal{},paused{};
  int pauseSelection{};
  std::string pauseError;
  bool temporarySession{};
  SummaryView summary;
  int residentJob{},residentWeapon{},residentOutfit{};
  bool blocked() const {return paused||npc>=0||crafting||trading||civic||equipment||cartography||progression||wardrobe||journal;}
  Vec mapCenter;float mapRange{4000};int mapLayerOverride{-1};
};
class Renderer {
public:
  Renderer();
  ~Renderer();
  size_t drawWorld(const World& world,const Save& save,const Gameplay& game,const Camera2D& camera,double time,float alpha,const Resource* highlighted);
  void drawLabels(const World& world,const Save& save,const Ui& ui,const Camera2D& camera) const;
  static Rectangle menuBounds(int rows);
  static Rectangle pauseBounds();
  static Rectangle pauseButton(int choice);
  static Rectangle pauseSummaryBounds();
  static int menuPageRows();
  static constexpr float menuRowHeight=50,menuListTop=102,menuFooterHeight=155;
  static float textSize(float size);
  void drawUi(const World& world,const Save& save,Ui& ui,const Camera2D& camera);
  void drawSavedMap(const Save& save,Rectangle box,int layer=-1);
  void summaryInput(SummaryView& view,Rectangle box,bool keyboard=true) const;
  void drawSummary(const Save& save,SummaryView& view,Rectangle box,bool enabled=true);
  void text(const std::string& s,float x,float y,float size,Color color) const;
  void wrapped(const std::string& s,float x,float y,float width,float size,Color color,int maxLines=8) const;
  float measure(const std::string& s,float size) const;
private:
  Texture2D atlas_{};
  Texture2D icons_{};
  std::unordered_map<std::string,int> iconIds_;
  void icon(const std::string& id,Rectangle box,Color tint=WHITE) const;
  struct SizedFont {int size;Font font;bool owned;};
  std::vector<SizedFont> fonts_;
  const Font& font(float size) const;
  std::vector<Rectangle> sprites_;
  struct Item {Vec pos;int sprite;Color tint;bool flip{};};
  std::vector<Item> items_;
  void sprite(int index,Vec p,Color tint=WHITE,bool flip=false) const;
  bool visible(Vec p,const Camera2D& c,float margin=180) const;
  void vehicle(const Save& save,const Gameplay& game,Vec p,double time) const;
  struct MapCache {Texture2D texture{};Vec center;float range{};double refresh{-1};bool submerged{};uint64_t seed{};};
  std::array<MapCache,2> maps_{};
  void drawMap(const Save& save,const Ui& ui,const Camera2D& camera,Rectangle box,bool large,bool viewport=true);
};
}
