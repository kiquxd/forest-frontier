#include "render.hpp"
#include <algorithm>
#include <cmath>

namespace frontier {
static Color landColor(Vec p,const Save& s,bool submerged) {
  auto terrain=terrainAt(p,s.seed);
  if(submerged) {
    if(terrain!=Terrain::Deep&&terrain!=Terrain::Shallow) return {16,24,27,255};
    return terrain==Terrain::Deep ? Color{48,93,106,255}:Color{83,132,130,255};
  }
  if(terrain==Terrain::Deep) return {43,100,134,255};
  if(terrain==Terrain::Shallow) return {72,146,160,255};
  if(terrain==Terrain::Sand) return {189,168,113,255};
  if(pathAt(p)) return {154,127,81,255};
  return {65,105,65,255};
}
void Renderer::drawMap(const Save& s,const Ui& ui,const Camera2D& camera,Rectangle box,bool large,bool showViewport) {
  const bool submerged=ui.mapLayerOverride<0 ? s.submerged:bool(ui.mapLayerOverride);
  auto& cache=maps_[large ? 1:0];
  Vec desired=large ? ui.mapCenter:Vec{std::floor(s.player.x/384)*384,std::floor(s.player.y/384)*384};
  float range=large ? ui.mapRange:2100;
  constexpr int pixels=256;
  if(cache.seed!=s.seed||cache.refresh<0||ui.time-cache.refresh>.3||cache.submerged!=submerged||cache.range!=range||length(cache.center-desired)>100) {
    cache.seed=s.seed;cache.center=desired;cache.range=range;cache.refresh=ui.time;cache.submerged=submerged;
    Image image=GenImageColor(pixels,pixels,{11,19,22,255});
    auto& explored=submerged ? s.exploredUnderwater:s.explored;
    auto point=[&](Vec p){return Vector2{(p.x-cache.center.x+range)/(2*range)*pixels,(p.y-cache.center.y+range)/(2*range)*pixels};};
    auto low=mapCell(cache.center-Vec{range,range}),high=mapCell(cache.center+Vec{range,range});
    for(int y=low.y;y<=high.y;y++) for(int x=low.x;x<=high.x;x++) {
      Coord c{x,y};Vec world{(x+.5f)*mapCellSize,(y+.5f)*mapCellSize};
      if(!explored.contains(c)) continue;
      auto a=point({x*mapCellSize,y*mapCellSize}),b=point({(x+1)*mapCellSize,(y+1)*mapCellSize});
      Color color=landColor(world,s,submerged);
      int left=int(std::floor(a.x)),top=int(std::floor(a.y)),right=int(std::ceil(b.x)),bottom=int(std::ceil(b.y));
      ImageDrawRectangle(&image,left,top,right-left,bottom-top,color);
      auto terrain=terrainAt(world,s.seed);
      if(!submerged&&terrain==Terrain::Forest&&!pathAt(world)&&objectId(x,y,s.seed)%5==0&&right-left>=4) {
        int cx=(left+right)/2,cy=(top+bottom)/2;
        ImageDrawTriangle(&image,{float(cx),float(cy-2)},{float(cx-2),float(cy+2)},{float(cx+2),float(cy+2)},{41,74,48,255});
      }
      // Neighboring cells share edges; shore boundaries follow the actual terrain.
      if(terrain==Terrain::Deep||terrain==Terrain::Shallow) {
        auto above=terrainAt(world-Vec{0,mapCellSize},s.seed),side=terrainAt(world-Vec{mapCellSize,0},s.seed);
        if(above==Terrain::Forest||above==Terrain::Sand) ImageDrawLine(&image,left,top,right,top,{220,203,154,255});
        if(side==Terrain::Forest||side==Terrain::Sand) ImageDrawLine(&image,left,top,left,bottom,{220,203,154,255});
      }
    }
    if(!submerged) ImageDrawLineEx(&image,point(settlements()[0].pos),point(settlements()[1].pos),std::max(1,int(64/(range*2)*pixels)),{154,127,81,255});
    if(!submerged) for(size_t city=0;city<settlements().size();city++) {
      auto& town=settlements()[city];auto center=point(town.pos);
      if(center.x<-25||center.y<-25||center.x>pixels+25||center.y>pixels+25) continue;
      auto& type=content().towns[town.type];Color roof{uint8_t(type.color[0]),uint8_t(type.color[1]),uint8_t(type.color[2]),255};
      float unit=std::max(2.f,190.f/(range*2)*pixels);
      ImageDrawRectangle(&image,int(center.x-unit*2.5f),int(center.y-1),int(unit*5),2,{181,151,101,255});
      for(int i=0;i<5+town.rarity;i++) {
        float angle=i*6.2831853f/(5+town.rarity);int x=int(center.x+std::cos(angle)*unit*2),y=int(center.y+std::sin(angle)*unit*2);
        ImageDrawRectangle(&image,x-2,y,5,4,{202,179,137,255});
        ImageDrawTriangle(&image,{float(x-3),float(y)},{float(x+2),float(y-3)},{float(x+5),float(y)},roof);
      }
      ImageDrawRectangle(&image,int(center.x)-unit,int(center.y)-unit,unit*2,unit*2,ownsCity(s,int(city)) ? Color{224,204,131,255}:roof);
      for(auto& wall:s.walls) if(wall.city==int(city)&&wall.hp>0) ImageDrawLineV(&image,point(wall.a),point(wall.b),{207,193,153,255});
    }
    // Apply fog last, so icons, walls and lakes cannot disclose unexplored pieces.
    for(int y=0;y<pixels;y++) for(int x=0;x<pixels;x++) {
      Vec world{cache.center.x-range+(x+.5f)/pixels*range*2,cache.center.y-range+(y+.5f)/pixels*range*2};
      if(!exploredAt(explored,world)) ImageDrawPixel(&image,x,y,{11,19,22,255});
      else {
        auto relative=project(world-s.player);
        bool current=std::abs(relative.x)<=GetScreenWidth()/(2*camera.zoom)&&std::abs(relative.y)<=GetScreenHeight()/(2*camera.zoom);
        if(submerged&&length(world-s.player)>650) current=false;
        if(!current) {
          auto* colors=static_cast<Color*>(image.data);auto& color=colors[y*pixels+x];
          color.r=uint8_t(color.r*.64f);color.g=uint8_t(color.g*.64f);color.b=uint8_t(color.b*.64f);
        }
      }
    }
    if(cache.texture.id) UpdateTexture(cache.texture,image.data);else {cache.texture=LoadTextureFromImage(image);SetTextureFilter(cache.texture,TEXTURE_FILTER_POINT);}
    UnloadImage(image);
  }
  auto mapPos=[&](Vec p){return Vector2{box.x+(p.x-cache.center.x+cache.range)/(2*cache.range)*box.width,box.y+(p.y-cache.center.y+cache.range)/(2*cache.range)*box.height};};
  DrawTexturePro(cache.texture,{0,0,256,256},box,{0,0},0,WHITE);
  BeginScissorMode(int(box.x),int(box.y),int(box.width),int(box.height));
  std::array<Vector2,4> corners={Vector2{0,0},Vector2{float(GetScreenWidth()),0},Vector2{float(GetScreenWidth()),float(GetScreenHeight())},Vector2{0,float(GetScreenHeight())}};
  std::array<Vector2,4> viewport;
  for(int i=0;i<4;i++) {auto p=GetScreenToWorld2D(corners[i],camera);viewport[i]=mapPos(unproject({p.x,p.y}));}
  if(showViewport) for(int i=0;i<4;i++) DrawLineEx(viewport[i],viewport[(i+1)%4],1,{204,217,184,180});
  auto player=mapPos(s.player);DrawCircleV(player,4,{248,221,141,255});DrawCircleLines(int(player.x),int(player.y),6,{30,39,31,255});
  if(submerged) DrawCircleLines(int(player.x),int(player.y),650/(2*cache.range)*box.width,{123,208,216,180});
  if(large&&!submerged) for(size_t i=0;i<settlements().size();i++) if(exploredAt(s.explored,settlements()[i].pos)) {
    auto point=mapPos(settlements()[i].pos);
    text(settlements()[i].name,point.x+10,point.y+12,16,{235,227,195,255});
  }
  EndScissorMode();
  DrawRectangleLinesEx(box,1,{146,169,145,255});
}
void Renderer::drawSavedMap(const Save& save,Rectangle box,int layer) {
  const bool submerged=layer<0 ? save.submerged:bool(layer);
  const auto& cells=submerged ? save.exploredUnderwater:save.explored;
  Vec low=save.player,high=save.player;
  for(auto cell:cells) {Vec p{cell.x*mapCellSize,cell.y*mapCellSize};low.x=std::min(low.x,p.x);low.y=std::min(low.y,p.y);high.x=std::max(high.x,p.x+mapCellSize);high.y=std::max(high.y,p.y+mapCellSize);}
  Ui ui;ui.mapLayerOverride=layer;ui.mapCenter=(low+high)*.5f;ui.mapRange=std::max(700.f,std::max(high.x-low.x,high.y-low.y)*.55f);ui.time=GetTime();
  Camera2D camera{};camera.zoom=1;drawMap(save,ui,camera,box,true,false);
}

}
