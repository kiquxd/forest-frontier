#include "render.hpp"
#include "adventure.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <fstream>
#include "json.hpp"

namespace frontier {
static constexpr Color ink{222,231,213,255}, muted{151,174,150,255}, gold{231,189,115,255};
static constexpr Color panel{20,31,29,238};
static Vector2 v(Vec p) {return {p.x,p.y};}
Renderer::Renderer() {
  // One texture atlas means grass, trees, people and huts share raylib's draw batch.
  const int outfitStart=64+int(content().npcs.size())*5;
  int count=outfitStart+int(content().outfits.size())*5;sprites_.resize(count);
  Image atlas=GenImageColor(1024,((count+7)/8)*160,BLANK);
  for(int index=0;index<count;index++) {
    const int ox=(index%8)*128,oy=(index/8)*160;
    sprites_[index]={float(ox),float(oy),128,160};
    int base=index,frame=0;
    if(index>=28&&index<44) {base=13+(index-28)/4;frame=(index-28)%4+1;}
    if(index>=44&&index<48) {base=17;frame=(index-44)%4+1;}
    if(index>=48&&index<52) {base=25;frame=(index-48)%4+1;}
    int npc=index>=64&&index<outfitStart ? (index-64)/5:-1;
    int clothing=index>=outfitStart ? (index-outfitStart)/5:-1;
    if(clothing>=0) {base=16;frame=(index-outfitStart)%5;}
    if(npc>=0) {base=13;frame=(index-64)%5;}
    int stride=frame ? std::array<int,4>{-3,0,3,0}[(frame-1)%4]:0;
    int bob=frame ? (frame%2):0;
    auto rect=[&](int x,int y,int w,int h,Color c){ImageDrawRectangle(&atlas,ox+x,oy+y,w,h,c);};
    auto circle=[&](int x,int y,int r,Color c){ImageDrawCircle(&atlas,ox+x,oy+y,r,c);};
    auto line=[&](int x,int y,int xx,int yy,Color c){ImageDrawLine(&atlas,ox+x,oy+y,ox+xx,oy+yy,c);};
    auto tri=[&](int x,int y,int xx,int yy,int xxx,int yyy,Color c){
      ImageDrawTriangle(&atlas,{float(ox+x),float(oy+y)},{float(ox+xx),float(oy+yy)},{float(ox+xxx),float(oy+yyy)},c);
    };
    if(base<8||(base>=20&&base<=22)||base==52) {
      Color c=base<6 ? Color{uint8_t(65+base*2),uint8_t(91+base*3),uint8_t(58+base*2),255}
                            : Color{uint8_t(123+(base-6)*5),uint8_t(111+(base-6)*4),80,255};
      if(base==20) c={179,162,112,255};
      if(base==21) c={69,137,143,255};
      if(base==22) c={43,97,126,255};
      if(base==52) c={63,105,113,255};
      tri(22,120,64,96,106,120,c); tri(22,120,106,120,64,144,c);
      for(int i=0;i<36;i++) {
        uint64_t h=hash(index*200+i);
        int x=32+h%64,y=102+(h>>16)%35;
        if(std::abs(x-64)/42.f+std::abs(y-120)/24.f>.8f) continue;
        if(base<6) {line(x,y,x+2,y-3,Color{91,119,73,150});line(x+2,y-3,x+4,y-1,Color{91,119,73,150});}
        else if(base<=20) rect(x,y,2,1,Color{150,137,102,170});
        else if(i%5==0) line(x,y,x+9,y,{116,179,183,125});
      }
    } else if(base<=10) {
      tri(29,143,91,143,69,155,{18,32,25,75});
      rect(59,111,10,34,{88,65,45,255}); rect(59,111,3,34,{116,83,52,255});
      int shift=(index-8)*5;
      tri(22,125,64,41-shift,106,125,{37,67,46,255});
      tri(30,99,64,24-shift,98,99,{44,81,51,255});
      tri(40,72,64,9,88,72,{54,94,58,255});
      tri(40,72,64,9,61,72,{68,110,68,255});
      line(45,103,78,103,{73,109,63,255}); line(51,79,69,79,{79,118,72,255});
    } else if(base==11) {
      tri(41,146,85,146,73,154,{18,32,25,65});
      tri(42,144,45,125,71,117,{127,138,131,255});
      tri(42,144,71,117,89,140,{108,120,115,255});
      tri(42,144,89,140,70,148,{81,96,91,255});
      line(47,125,69,119,{165,174,157,255});
    } else if(base==12) {
      circle(53,135,13,{44,76,48,255});circle(70,133,16,{49,90,53,255});circle(62,123,12,{59,103,57,255});
      for(int i=0;i<7;i++) circle(48+(i*13)%34,121+(i*7)%21,2,{187,75,87,255});
    } else if(base<=16) {
      Color coat=base==16 ? Color{216,176,98,255} : base==13 ? Color{115,151,137,255} : base==14 ? Color{171,110,89,255} : Color{143,135,171,255};
      if(npc>=0) {auto color=content().npcs[npc].color;coat={uint8_t(color[0]),uint8_t(color[1]),uint8_t(color[2]),255};}
      if(clothing>=0) {auto color=content().outfits[clothing].color;coat={uint8_t(color[0]),uint8_t(color[1]),uint8_t(color[2]),255};}
      rect(53,141,24,4,{13,26,25,100});
      rect(57-stride,127,5,16-stride,{52,56,54,255});rect(68+stride,127,5,16+stride,{52,56,54,255});
      rect(55,103-bob,21,27,coat);rect(51,106-bob+stride,4,21,coat);rect(76,106-bob-stride,4,21,coat);
      rect(58,91-bob,15,15,{206,168,128,255});rect(57,87-bob,17,6,{65,55,45,255});
      rect(65,97-bob,2,2,{45,44,34,255});rect(58,116-bob,16,3,{74,66,47,255});
      if(base==16) {rect(53,105-bob,6,19,{107,80,48,255});rect(59,83-bob,16,5,{173,128,68,255});}
      if(clothing>=0&&content().outfits[clothing].id=="guard") {rect(52,101-bob,26,5,{184,192,190,255});rect(58,81-bob,17,10,{127,148,159,255});}
      if(clothing>=0&&content().outfits[clothing].id=="scholar") rect(54,126,22,12,coat);
      if(npc>=0&&content().npcs[npc].rarity>=1) {rect(54,86-bob,23,3,coat);rect(60,80-bob,11,7,coat);}
    } else if(base==17) {
      rect(51,143,29,4,{14,27,22,70}); circle(62,136-bob*2,10,{188,176,146,255});circle(74,128-bob*2,6,{198,184,150,255});
      rect(73,112-bob*2,3,14,{187,171,140,255});rect(78,114-bob*2,3,13,{187,171,140,255});circle(76,127-bob*2,1,{39,43,32,255});circle(51,135-bob*2,4,{224,216,194,255});
      if(frame) {rect(59-stride,139,4,5,{170,160,132,255});rect(69+stride,139,4,5,{170,160,132,255});}
    } else if(base==18||base==19||base==58) {
      auto quad=[&](int a,int b,int c,int d,int e,int f,int g,int h,Color color) {tri(a,b,c,d,e,f,color);tri(a,b,e,f,g,h,color);};
      // Two wall planes, a hip roof, stone plinth and layered cast shadow.
      quad(8,129,63,154,124,124,68,104,{15,27,23,85});
      quad(18,78,64,101,64,146,18,123,{174,133,83,255});
      quad(64,101,110,78,110,123,64,146,{110,85,60,255});
      quad(18,118,64,141,64,146,18,123,{111,124,115,255});
      quad(64,141,110,118,110,123,64,146,{73,88,83,255});
      for(int i=0;i<5;i++) {line(20,84+i*8,62,105+i*8,{119,87,55,255});line(66,105+i*8,108,84+i*8,{78,64,47,255});}
      line(20,79,20,118,{84,63,44,255});line(63,101,63,141,{86,63,42,255});line(107,80,107,119,{72,60,43,255});
      Color roof=base==19 ? Color{85,124,105,255}:base==58 ? Color{172,92,60,255}:Color{153,99,61,255};
      tri(9,79,64,34,64,108,roof);tri(64,34,120,79,64,108,{uint8_t(roof.r*.7f),uint8_t(roof.g*.7f),uint8_t(roof.b*.7f),255});
      line(9,79,64,108,{204,151,87,255});line(64,108,120,79,{96,69,46,255});line(64,35,64,107,{194,138,79,255});
      for(int i=0;i<5;i++) {line(24+i*7,70-i*5,61,100-i*9,{115,75,47,255});line(67,99-i*9,105-i*6,78-i*5,{86,67,47,255});}
      // Windows sit on their respective isometric wall planes.
      quad(28,89,45,98,45,112,28,103,{62,111,125,255});quad(31,92,43,98,43,108,31,102,{148,198,188,255});
      line(36,94,36,106,{203,181,125,255});line(29,98,44,105,{203,181,125,255});
      quad(87,95,101,88,101,101,87,108,{48,87,96,255});line(94,91,94,104,{161,156,107,255});
      quad(68,115,81,108,81,133,68,140,{49,45,33,255});quad(70,117,79,112,79,131,70,136,{112,77,43,255});circle(77,124,1,{227,193,115,255});
      quad(67,141,82,133,91,137,75,146,{147,151,129,255});line(70,143,85,136,{202,196,157,255});
      quad(88,57,96,53,96,71,88,75,{112,105,88,255});quad(96,53,102,57,102,74,96,71,{79,82,73,255});rect(88,51,14,5,{159,151,124,255});
      if(base==58) {
        quad(54,19,68,25,68,68,54,61,{190,170,128,255});quad(68,25,80,19,80,61,68,68,{116,118,98,255});
        tri(50,21,67,4,85,21,{87,65,47,255});circle(62,37,5,{231,213,154,255});line(62,37,62,33,{65,71,60,255});line(62,37,65,39,{65,71,60,255});
        line(83,24,83,48,{199,177,120,255});tri(84,25,102,31,84,38,{186,78,51,255});
      }
    } else if(base==23) {
      for(int i=0;i<7;i++) {int x=46+i*6,y=108+(i*7)%22;line(x,144,x-3,y,{120,144,66,255});rect(x-5,y-7,4,12,{144,113,66,255});}
    } else if(base==24) {
      line(44,141,83,141,{165,207,199,190});line(49,146,77,146,{129,189,190,150});
      tri(49,133,77,133,66,139,{160,193,193,190});tri(73,136,82,130,82,141,{172,206,202,190});
    } else if(base==25) {
      rect(40,144,54,4,{14,26,27,90});
      rect(46-stride,127,6,17-stride,{88,100,100,255});rect(69+stride,127,6,17+stride,{88,100,100,255});
      rect(42,119-bob,40,17,{106,121,121,255});circle(83,117-bob,10,{126,137,130,255});
      tri(76,110-bob,77,101-bob,83,110-bob,{105,119,113,255});tri(85,110-bob,90,101-bob,92,113-bob,{105,119,113,255});
      rect(88,117-bob,10,6,{127,135,130,255});rect(89,115-bob,2,2,{228,164,85,255});
      tri(42,122,28,113,36,128,{97,113,112,255});
    } else if(base==26) {
      tri(26,130,103,130,89,153,{125,82,48,255});tri(26,130,89,153,40,153,{155,102,55,255});
      rect(35,128,60,4,{205,156,82,255});line(40,138,89,138,{84,66,41,255});
    } else if(base==27) {
      rect(44,137,42,7,{97,70,42,255});line(48,147,81,133,{139,91,42,255});
      tri(48,138,65,105,84,138,{228,114,48,255});tri(58,138,65,119,73,138,{244,195,80,255});
    } else if(base==53) {
      tri(43,144,50,120,75,117,{120,128,137,255});tri(43,144,75,117,91,142,{90,100,111,255});
      rect(59,125,5,5,{192,139,92,255});rect(75,136,6,4,{192,139,92,255});
    } else if(base==54) {
      tri(51,145,61,105,73,143,{131,182,213,255});tri(73,143,80,116,89,144,{151,113,220,255});
      line(60,116,62,137,{212,225,239,255});
    } else if(base==55) {
      circle(64,138,14,{105,117,148,255});circle(64,131,7,{240,229,198,255});line(50,143,78,143,{163,177,180,255});
    } else if(base==56) {
      rect(44,122,40,23,{104,104,91,255});rect(49,127,30,4,{68,83,84,255});rect(62,132,5,7,{195,154,85,255});
    } else if(base==57) {
      for(int i=0;i<6;i++) {int x=46+i*7;line(x,144,x-4,110+(i*9)%20,{154,107,131,255});circle(x-4,110+(i*9)%20,4,{174,131,141,255});}
    }
  }
  atlas_=LoadTextureFromImage(atlas); UnloadImage(atlas); SetTextureFilter(atlas_,TEXTURE_FILTER_POINT);
  const std::string candidates[]={std::string(FRONTIER_ROOT)+"/assets/NotoSans.ttf",
    "/usr/share/fonts/google-noto-vf/NotoSans[wght].ttf","/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"};
  std::vector<int> points; for(int c=32;c<127;c++) points.push_back(c); for(int c=0x400;c<=0x45f;c++) points.push_back(c);
  points.push_back(0x2014);points.push_back(0x2026);points.push_back(0x00b7);
  std::string fontPath;for(auto& path:candidates) if(std::filesystem::exists(path)) {fontPath=path;break;}
  // Bake every used physical size; never rescale a nearby glyph atlas.
  for(int size=20;size<=45;size++) {
    Font loaded=fontPath.empty() ? GetFontDefault():LoadFontEx(fontPath.c_str(),size,points.data(),int(points.size()));
    bool owned=!fontPath.empty()&&loaded.texture.id!=0;if(!owned) loaded=GetFontDefault();
    SetTextureFilter(loaded.texture,TEXTURE_FILTER_POINT);fonts_.push_back({size,loaded,owned});
  }
  const std::string iconRoot=std::string(FRONTIER_ROOT)+"/assets/ui/";
  icons_=LoadTexture((iconRoot+"icons.png").c_str());SetTextureFilter(icons_,TEXTURE_FILTER_POINT);
  std::ifstream manifest(iconRoot+"icons.json");auto ids=nlohmann::json::parse(manifest).get<std::vector<std::string>>();
  for(int i=0;i<int(ids.size());i++) iconIds_[ids[i]]=i;
  items_.reserve(4096);
}
Renderer::~Renderer() {for(auto& f:fonts_) if(f.owned) UnloadFont(f.font);for(auto& map:maps_) if(map.texture.id) UnloadTexture(map.texture);UnloadTexture(icons_);UnloadTexture(atlas_);}
const Font& Renderer::font(float size) const {
  return std::min_element(fonts_.begin(),fonts_.end(),[&](auto& a,auto& b){return std::abs(a.size-size)<std::abs(b.size-size);})->font;
}
float Renderer::textSize(float size) {return std::round(std::max(16.f,size)*1.25f);}
float Renderer::measure(const std::string& s,float size) const {size=textSize(size);return MeasureTextEx(font(size),s.c_str(),size,1).x;}
void Renderer::text(const std::string& s,float x,float y,float size,Color color) const {
  size=textSize(size);DrawTextEx(font(size),s.c_str(),{std::round(x),std::round(y)},size,1,color);
}
void Renderer::wrapped(const std::string& s,float x,float y,float width,float size,Color color,int maxLines) const {
  const float lineHeight=textSize(size)+6;
  std::string line; int lines=0;
  auto emit=[&]{text(line,x,y+lines*lineHeight,size,color);line.clear();lines++;};
  for(size_t i=0;i<s.size();) {
    int bytes=0; GetCodepointNext(s.c_str()+i,&bytes); if(bytes<=0) break;
    auto ch=s.substr(i,bytes); i+=bytes;
    if(ch=="\n") {emit(); if(lines>=maxLines) return; continue;}
    if(measure(line+ch,size)>width&&!line.empty()) {
      auto space=line.find_last_of(' ');
      if(space!=std::string::npos) {auto tail=line.substr(space+1);line.resize(space);emit();line=tail;}
      else emit();
      if(lines>=maxLines) {text("…",x+width-20,y+(lines-1)*lineHeight,size,color);return;}
    }
    line+=ch;
  }
  if(!line.empty()&&lines<maxLines) emit();
}
void Renderer::sprite(int index,Vec p,Color tint,bool flip) const {
  auto src=sprites_[index];
  if(index<8||(index>=20&&index<=22)||index==52) {
    src.x+=22;src.y+=96;src.width=84;src.height=48;
    DrawTexturePro(atlas_,src,{p.x,p.y,tileSize*1.7320508f,tileSize},{tileSize*.8660254f,tileSize*.5f},0,tint);
  } else {if(flip) src.width=-src.width;DrawTextureRec(atlas_,src,{p.x-64,p.y-144},tint);}
}
bool Renderer::visible(Vec p,const Camera2D& c,float margin) const {
  auto screen=GetWorldToScreen2D(v(p),c);
  return screen.x>-margin&&screen.y>-margin&&screen.x<GetScreenWidth()+margin&&screen.y<GetScreenHeight()+margin;
}
size_t Renderer::drawWorld(const World& world,const Save& save,const Gameplay& game,const Camera2D& camera,double time,float alpha,const Resource* highlighted) {
  const int outfitStart=64+int(content().npcs.size())*5;
  items_.clear(); size_t count=0;
  for(auto& [coord,chunk]:world.chunks) {
    // Chunk bounds first, then individual tile bounds, all in projected coordinates.
    Vec mid=project({(coord.x+.5f)*chunkSize,(coord.y+.5f)*chunkSize});
    if(!visible(mid,camera,chunkSize*1.5f*camera.zoom)) continue;
    for(int y=0;y<chunkTiles;y++) for(int x=0;x<chunkTiles;x++) {
      Vec p=project({(coord.x*chunkTiles+x+.5f)*tileSize,(coord.y*chunkTiles+y+.5f)*tileSize});
      if(!visible(p,camera,55)) continue;
      auto t=chunk.tiles[y*chunkTiles+x];
      if(save.submerged&&t.terrain!=Terrain::Deep&&t.terrain!=Terrain::Shallow) continue;
      int image=t.path ? 6+t.variant%2 : t.variant%6;
      if(t.terrain!=Terrain::Forest) image=20+int(t.terrain)-1;
      if(save.submerged) image=52;
      Color tint=WHITE;
      if(!save.submerged&&(t.terrain==Terrain::Deep||t.terrain==Terrain::Shallow)) {
        uint8_t light=uint8_t(243+std::sin(time*1.5+(coord.x*chunkTiles+x)*.8+(coord.y*chunkTiles+y)*.6)*12);
        tint={light,light,255,255};
      }
      if(save.submerged) {
        Vec cellWorld{(coord.x*chunkTiles+x+.5f)*tileSize,(coord.y*chunkTiles+y+.5f)*tileSize};auto cell=mapCell(cellWorld);
        if(objectId(cell.x,cell.y,save.seed)%31==0) tint={209,137,160,255};
        if(length(cellWorld-save.player)>650) tint={115,139,147,255};
      }
      sprite(image,p,tint); count++;
    }
    for(auto& r:save.submerged ? chunk.underwater:chunk.resources) {
      if(save.harvested.contains(r.id)) continue;
      auto p=project(r.pos); if(!visible(p,camera)) continue;
      int image=r.kind==ResourceKind::Tree ? 8+r.variant : r.kind==ResourceKind::Stone ? 11 : r.kind==ResourceKind::Berry ? 12 :
        r.kind==ResourceKind::Reed ? 23:r.kind==ResourceKind::Fish ? 24:r.kind==ResourceKind::Ore ? 53:r.kind==ResourceKind::Crystal ? 54:r.kind==ResourceKind::Pearl ? 55:56;
      items_.push_back({p,image,WHITE});
    }
    for(auto& a:chunk.animals) {
      if(save.submerged) continue;
      auto p=project(a.previous+(a.pos-a.previous)*alpha);
      bool moving=length(a.pos-a.previous)>.01f;
      if(visible(p,camera)) items_.push_back({p,moving ? 44+int(a.walk)%4:17,WHITE,project(a.pos-a.previous).x<0});
    }
    for(auto& e:chunk.enemies) {
      if(save.submerged) continue;
      if(save.defeated.contains(e.id)) continue;
      auto p=project(e.previous+(e.pos-e.previous)*alpha);
      int image=e.kind ? (length(e.pos-e.previous)>.01f ? 32+int(e.walk)%4:14):(length(e.pos-e.previous)>.01f ? 48+int(e.walk)%4:25);
      if(visible(p,camera)) items_.push_back({p,image,
        e.flash>0 ? Color{255,160,135,255}:WHITE,project(e.pos-e.previous).x<0});
    }
  }
  if(!save.submerged) for(auto& b:world.buildings) {auto p=project(b.pos);if(visible(p,camera)) items_.push_back({p,b.hall ? 58:18+b.variant%2,WHITE});}
  for(auto& n:world.npcs) {
    if(save.submerged||n.ruler) continue;
    auto p=project(n.previous+(n.pos-n.previous)*alpha);
    int image=64+n.type*5+(length(n.pos-n.previous)>.01f ? 1+int(n.walk)%4:0);
    if(auto resident=citizen(save,n.id);resident&&!resident->outfit.empty()) {for(int i=0;i<int(content().outfits.size());i++) if(content().outfits[i].id==resident->outfit) image=outfitStart+i*5+(length(n.pos-n.previous)>.01f ? 1+int(n.walk)%4:0);}
    bool flip=n.id==world.talkingNpc ? project(save.player-n.pos).x<0:project(n.pos-n.previous).x<0;
    if(visible(p,camera)) items_.push_back({p,image,WHITE,flip});
  }
  if(!save.submerged) for(auto fire:save.campfires) {auto p=project(fire);if(visible(p,camera)) items_.push_back({p,27,{255,uint8_t(232+std::sin(time*9)*20),210,255}});}
  Vec player=project(save.player);
  auto terrain=terrainAt(save.player,save.seed);
  bool afloat=watercraft(save)&&(terrain==Terrain::Deep||terrain==Terrain::Shallow);
  int playerImage=game.moving&&game.swing<=0 ? 40+int(game.walk)%4:16;
  if(save.adventure) for(int i=0;i<int(content().outfits.size());i++) if(content().outfits[i].id==save.outfitId) playerImage=outfitStart+i*5+(game.moving&&game.swing<=0 ? 1+int(game.walk)%4:0);
  if(!afloat) items_.push_back({player,playerImage,
    game.invulnerable>0&&int(time*12)%2 ? Color{255,158,145,210}:WHITE,project(game.facing).x<0});
  if(highlighted) {
    auto p=project(highlighted->pos);
    DrawEllipse(int(p.x),int(p.y),20,10,{228,195,116,160});
  }
  DrawEllipse(int(player.x),int(player.y),17,8,{237,202,123,60});
  std::sort(items_.begin(),items_.end(),[](auto& a,auto& b){return a.pos.y<b.pos.y;});
  for(auto& item:items_) sprite(item.sprite,item.pos,item.tint,item.flip);
  if(!save.submerged) for(auto& resident:save.citizens) if(!resident.weapon.empty()&&resident.npc<int(world.npcs.size())) {
    auto& npc=world.npcs[resident.npc];auto p=project(npc.previous+(npc.pos-npc.previous)*alpha);if(!visible(p,camera)) continue;
    DrawLineEx({p.x+12,p.y-26},{p.x+23,p.y-42},3,{186,194,183,255});DrawLineEx({p.x+9,p.y-29},{p.x+17,p.y-24},3,{189,153,91,255});
  }
  if(afloat) vehicle(save,game,player,time);
  if(!save.submerged) for(auto& wall:save.walls) if(wall.hp>0) {
    auto a=project(wall.a),b=project(wall.b);if(!visible(a,camera)&&!visible(b,camera)) continue;
    Color wallColor=save.technologies.contains("fortification") ? Color{142,155,147,255}:Color{153,111,66,255};
    DrawLineEx(v(a),v(b),12,{85,69,47,255});
    int posts=int(length(wall.b-wall.a)/13);
    for(int i=0;i<=posts;i++) {auto p=a+(b-a)*(float(i)/posts);DrawLineEx({p.x,p.y},{p.x,p.y-27},8,wallColor);}
    DrawLineEx({a.x,a.y-14},{b.x,b.y-14},4,{93,67,43,255});
  }
  if(game.swing>0) {
    auto& weapon=game.swingingWeapon();float progress=game.attackElapsed/std::max(game.attackDuration,.001f);
    float angle=progress<.3f ? -1.3f-progress : progress<.58f ? -1.6f+(progress-.3f)/.28f*2.9f : 1.3f*(1-(progress-.58f)/.42f);
    float aim=std::atan2(game.attackFacing.y,game.attackFacing.x);
    if(weapon.kind=="spear"||weapon.kind=="bow"||weapon.kind=="fists") angle=0;
    Vec direction{std::cos(aim+angle),std::sin(aim+angle)};
    float reach=std::min(weapon.reach,150.f)*.75f;
    Vec hand=player+Vec{0,-25}+project(direction*17),tip=player+Vec{0,-25}+project(direction*reach);
    DrawLineEx({player.x,player.y-24},v(hand),5,{205,165,121,255});
    if(weapon.kind=="fists") DrawCircleV(v(hand+project(direction*12)),5,{208,166,123,255});
    else if(weapon.kind=="bow") {
      auto normal=normalized(project(direction));Vec cross{-normal.y,normal.x};
      float degrees=std::atan2(normal.y,normal.x)*180/3.14159265f;
      DrawCircleSectorLines(v(hand),20,degrees-90,degrees+90,12,{192,154,85,255});
      DrawLineEx(v(hand+cross*20),v(hand-normal*(progress<.3f ? 10.f:0)),1,ink);
      DrawLineEx(v(hand-cross*20),v(hand-normal*(progress<.3f ? 10.f:0)),1,ink);
    }
    else {
      Color blade=weapon.rarity>=2 ? Color{166,207,237,255}:Color{222,224,208,255};
      auto unit=normalized(tip-hand);Vec cross{-unit.y,unit.x};
      DrawLineEx(v(hand),v(tip),weapon.kind=="hammer" ? 6.f:4.f,blade);
      DrawLineEx(v(hand+cross*8),v(hand-cross*8),3,gold);
      if(weapon.kind=="axe") DrawLineEx(v(tip+cross*13),v(tip-cross*3),9,blade);
      if(weapon.kind=="hammer") DrawLineEx(v(tip+cross*10),v(tip-cross*10),12,blade);
      if(progress>=.3f&&progress<.58f) for(int i=1;i<=3;i++) {
        Vec d{std::cos(aim+angle-i*.15f),std::sin(aim+angle-i*.15f)};auto trail=player+Vec{0,-25}+project(d*reach);
        DrawLineEx(v(player+Vec{0,-25}+project(d*reach*.45f)),v(trail),2,{184,207,209,uint8_t(100/i)});
      }
    }
  }
  for(auto& arrow:game.projectiles) {auto p=project(arrow.pos),d=project(normalized(arrow.velocity)*23);DrawLineEx({p.x,p.y-24},{p.x-d.x,p.y-d.y-24},2,gold);}
  for(auto& [coord,chunk]:world.chunks) for(auto& e:chunk.enemies) {
    if(save.submerged||save.defeated.contains(e.id)) continue;
    auto p=project(e.previous+(e.pos-e.previous)*alpha);if(!visible(p,camera,30)) continue;
    if(e.windup>0) DrawEllipseLines(int(p.x),int(p.y),26,13,{241,143,93,255});
    int maxHp=e.kind ? 100:60;if(e.hp==maxHp) continue;
    DrawRectangle(int(p.x-20),int(p.y-52),40,5,{30,38,33,240});
    DrawRectangle(int(p.x-20),int(p.y-52),std::max(0,e.hp)*40/maxHp,5,{207,98,79,255});
  }
  // Player marker remains visible behind trees.
  DrawCircleV({player.x,player.y-64},3,{246,213,143,255});
  return count+items_.size();
}
void Renderer::drawLabels(const World& world,const Save& save,const Ui& ui,const Camera2D& camera) const {
  if(save.submerged) return;
  for(size_t city=0;city<settlements().size();city++) {
    auto& s=settlements()[city];
    auto iso=project(s.pos)+Vec{0,-330};if(!visible(iso,camera)) continue;
    auto screen=GetWorldToScreen2D(v(iso),camera);Vec p{screen.x,screen.y};
    std::string cityLabel=s.name+(ownsCity(save,int(city)) ? " · your town":"");
    float width=measure(cityLabel,22);
    DrawRectangleRounded({p.x-width*.5f-15,p.y-6,width+30,35},.25f,4,{22,35,29,200});
    Color color=save.relations[city].treaty==Treaty::War ? Color{228,131,110,255}:ink;
    text(cityLabel,p.x-width*.5f,p.y,22,color);
    if(length(save.player-(s.pos+Vec{55,55}))<90) {auto door=GetWorldToScreen2D(v(project(s.pos)+Vec{0,30}),camera);text("P: enter town hall",door.x-80,door.y,16,gold);}
  }
  for(auto& npc:world.npcs) {
    if(npc.ruler||npc.id==ui.npc) continue;
    auto point=project(npc.pos)+Vec{0,-83};if(!visible(point,camera)) continue;
    auto q=npcQuest(save,npc);if(!q) continue;auto found=save.quests.find(npc.id);if(found!=save.quests.end()&&found->second.completed) continue;
    auto screen=GetWorldToScreen2D(v(point),camera);DrawCircleV({screen.x,screen.y},10,{38,52,39,230});text(found==save.quests.end() ? "?":"!",screen.x-4,screen.y-9,18,gold);
  }
  if(ui.npc>=0&&ui.npc<int(world.npcs.size())&&!world.npcs[ui.npc].ruler) {
    auto& npc=world.npcs[ui.npc];auto iso=project(npc.pos)+Vec{0,-90};auto screen=GetWorldToScreen2D(v(iso),camera);
    DrawEllipse(int(screen.x),int(screen.y),28,18,{238,235,212,255});
    DrawTriangle({screen.x-10,screen.y+12},{screen.x+3,screen.y+29},{screen.x+10,screen.y+12},{238,235,212,255});
    for(int i=-1;i<=1;i++) DrawCircleV({screen.x+i*9.f,screen.y},ui.busy ? 2.5f+float(std::sin(ui.time*4+i))*.6f:2.5f,{54,69,56,255});
  }
}
void Renderer::vehicle(const Save& save,const Gameplay& game,Vec p,double time) const {
  int type=activeVehicle(save);Vec direction=normalized(project(game.facing)),side{-direction.y,direction.x};
  float size=type==2 ? 88.f:type==3 ? 64.f:48.f,width=type==2 ? 30.f:type==3 ? 23.f:21.f;
  auto point=[&](float x,float y){return v(p+direction*x+side*y);};
  auto triangle=[](Vector2 a,Vector2 b,Vector2 c,Color color) {
    if((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x)>0) std::swap(b,c);
    DrawTriangle(a,b,c,color);
  };
  if(game.moving) for(int i=0;i<3;i++) {
    auto trail=p-direction*(size+i*13.f);DrawLineEx(v(trail-side*(width+i*5)),v(trail+side*(width+i*5)),1,{159,215,211,uint8_t(95-i*20)});
  }
  Color hull=type==3 ? Color{133,154,155,255}:type==2 ? Color{122,82,50,255}:Color{162,108,59,255};
  std::vector<Vector2> outline;
  if(type==3) for(int i=0;i<12;i++) {float a=i*6.2831853f/12;outline.push_back(point(std::cos(a)*size,std::sin(a)*width));}
  else outline={point(size,0),point(size*.4f,width),point(-size*.65f,width*.85f),point(-size,width*.35f),point(-size,-width*.35f),point(-size*.65f,-width*.85f),point(size*.4f,-width)};
  Color rim=type==3 ? Color{181,195,191,255}:Color{213,171,103,255};
  for(size_t i=0;i<outline.size();i++) {triangle(v(p),outline[i],outline[(i+1)%outline.size()],hull);DrawLineEx(outline[i],outline[(i+1)%outline.size()],3,rim);}
  for(int i=-3;i<=3;i++) {
    float x=i*size*.21f;
    DrawLineEx(point(x,-width*.66f),point(x,width*.66f),type==3 ? 2:1,type==3 ? Color{86,108,117,255}:Color{81,54,35,255});
    for(int sign:{-1,1}) DrawCircleV(point(x,sign*width*.7f),1.5f,{205,190,151,255});
  }
  if(type==3) {
    for(int sign:{-1,1}) {triangle(point(-size*.55f,sign*width*.6f),point(-size*.9f,sign*width*1.4f),point(-size*.2f,sign*width*.8f),{93,125,135,255});}
    DrawLineEx(point(-size,0),point(-size-13,0),4,{109,118,114,255});
    float spin=game.moving ? std::sin(time*22)*12:9;
    DrawLineEx(point(-size-10,-spin),point(-size-10,spin),3,{192,159,91,255});
    DrawCircleV(point(size*.65f,0),5,{247,222,137,255});
    for(int i:{-1,1}) {DrawCircleV(point(i*size*.4f,-3),10,{65,81,87,255});DrawCircleV(point(i*size*.4f,-3),7,{104,208,221,255});DrawCircleV(point(i*size*.4f-2,-5),2,{215,250,245,255});}
    DrawEllipse(int(p.x),int(p.y-5),22,15,{107,126,131,255});
    DrawCircleV(v(p+direction*24+Vec{0,-9}),7,{116,211,218,255});DrawCircleV(v(p-direction*19+Vec{0,-9}),7,{116,211,218,255});
    if(!save.submerged) DrawLineEx({p.x,p.y-5},{p.x,p.y-30},4,{179,183,171,255});
    else {
      DrawCircleLines(int(p.x),int(p.y),75+std::sin(time*1.4)*5,{110,205,213,100});
      if(game.sonarCooldown>6) DrawCircleLines(int(p.x),int(p.y),(10-game.sonarCooldown)*180,{113,219,224,140});
    }
    return;
  }
  if(type==2) {
    for(int sign:{-1,1}) {
      DrawLineEx(point(-size*.8f,sign*width*.7f),point(size*.55f,sign*width*.7f),2,{224,199,145,255});
      for(int i=-3;i<=2;i++) DrawLineEx(point(i*size*.22f,sign*width*.7f),point(i*size*.22f,sign*width*.7f-9),2,{188,152,94,255});
    }
    triangle(point(-size*.45f,-width*.45f),point(-size*.8f,0),point(-size*.4f,width*.45f),{100,54,34,255});
    DrawCircleV(point(-size*.6f,0),6,{202,176,112,255});
    DrawLineEx({p.x,p.y-114},point(-size*.7f,-width*.6f),1,{197,184,150,255});
    DrawLineEx({p.x,p.y-114},point(size*.7f,0),1,{197,184,150,255});
  }
  // Seated pilot is part of the vehicle; no walking legs are drawn on water.
  auto coat=playerOutfit(save).color;DrawRectangle(int(p.x-9),int(p.y-23),18,18,{uint8_t(coat[0]),uint8_t(coat[1]),uint8_t(coat[2]),255});DrawCircleV({p.x,p.y-32},7,{208,169,131,255});
  if(type==1) {
    float stroke=game.moving ? std::sin(time*5)*16:0;
    for(int sign:{-1,1}) {
      auto pivot=p+side*(float(sign)*13)-direction*5,blade=p+side*(float(sign)*43)+direction*stroke;
      DrawLineEx(v(pivot),v(blade),3,{213,177,113,255});DrawCircleV(v(blade),4,{213,177,113,255});
    }
  } else {
    DrawLineEx({p.x,p.y},{p.x,p.y-116},5,{152,120,75,255});
    triangle({p.x+2,p.y-118},{p.x+23,p.y-113},{p.x+2,p.y-106},{175,76,51,255});
    float wind=std::sin(time*1.2)*4;
    triangle({p.x+5,p.y-112},{p.x+54+wind,p.y-40},{p.x+5,p.y-40},{227,225,198,255});
    for(int i=0;i<3;i++) DrawLineEx({p.x+6,p.y-93+i*17.f},{p.x+18+i*12.f,p.y-75+i*17.f},1,{166,165,145,255});
    triangle({p.x-4,p.y-99},{p.x-40+wind,p.y-46},{p.x-4,p.y-46},{201,210,190,255});
  }
}
int Renderer::menuPageRows() {return std::clamp(int((GetScreenHeight()-40-menuListTop-menuFooterHeight)/menuRowHeight),1,8);}
Rectangle Renderer::pauseBounds() {
  float width=std::min(1240.f,GetScreenWidth()-48.f),height=GetScreenHeight()-48.f;
  return {(GetScreenWidth()-width)*.5f,24,width,height};
}
Rectangle Renderer::pauseSummaryBounds() {
  auto box=pauseBounds();return {box.x+20,box.y+60,box.width-40,box.height-192};
}
Rectangle Renderer::pauseButton(int choice) {
  auto box=pauseBounds();float width=std::min(330.f,(box.width-60)*.5f);
  return {choice==0 ? box.x+20:box.x+box.width-width-20,box.y+box.height-74,width,50};
}
Rectangle Renderer::menuBounds(int rows) {
  float width=std::min(960.f,GetScreenWidth()-40.f),height=std::min(rows,8)*menuRowHeight+menuListTop+menuFooterHeight;
  return {(GetScreenWidth()-width)*.5f,(GetScreenHeight()-height)*.5f,width,height};
}
void Renderer::drawUi(const World& world,const Save& save,Ui& ui,const Camera2D& camera) {
  float w=float(GetScreenWidth()),h=float(GetScreenHeight());
  DrawRectangleRounded({20,20,390,52},.12f,6,panel);
  text("FOREST FRONTIER",38,29,28,gold);
  DrawRectangleRounded({20,84,390,188},.1f,6,panel);
  text("Health "+std::to_string(save.health)+" / 100",38,96,16,ink);
  DrawRectangle(38,122,224,6,{45,59,45,255});DrawRectangle(38,122,224*save.health/100,6,save.health<30 ? Color{220,104,83,255}:Color{137,185,121,255});
  text("Coins "+std::to_string(save.coins),38,138,16,gold);
  wrapped("Wood "+std::to_string(ui.wood)+"   Stone "+std::to_string(ui.stone)+"   Berries "+std::to_string(ui.berries),38,163,354,14,ink,1);
  wrapped("Reeds "+std::to_string(save.inventory.reeds)+"   Fish "+std::to_string(save.inventory.fish)+"   Leather "+std::to_string(save.inventory.leather),38,185,354,14,ink,1);
  text("Potions "+std::to_string(save.inventory.potions)+"   Campfires "+std::to_string(save.inventory.campfires),38,207,14,muted);
  text(save.submerged ? "Underwater world":terrainName(terrainAt(save.player,save.seed)),38,242,15,gold);
  DrawRectangleRounded({20,284,390,86},.1f,6,panel);
  std::string equipment=currentWeapon(save).name;
  if(save.adventure) equipment+=" · "+playerOutfit(save).name;
  else if(save.inventory.armor) equipment+=" · Armor";
  if(watercraft(save)) equipment+=activeVehicle(save)==3 ? " · Submarine":activeVehicle(save)==2 ? " · Ship":" · Boat";
  wrapped(equipment,38,291,354,16,ink,2);
  text("L: quests · E: NPC lore",38,346,14,muted);
  DrawRectangleRounded({20,372,390,102},.1f,6,panel);
  text("Iron "+std::to_string(goodCount(save.inventory,6))+" · Copper "+std::to_string(goodCount(save.inventory,7))+" · Steel "+std::to_string(goodCount(save.inventory,10)),38,384,16,ink);
  text("Crystals "+std::to_string(goodCount(save.inventory,8))+" · Pearls "+std::to_string(goodCount(save.inventory,9)),38,410,16,ink);
  text(save.submerged ? "Air "+std::to_string(int(save.oxygen))+" · Hull "+std::to_string(save.hull):"Arrows "+std::to_string(save.inventory.ammo)+" · I: equipment",38,443,16,gold);

  if(save.campaign) {
    DrawRectangleRounded({20,486,390,74},.1f,6,panel);
    text("Skills "+std::to_string(save.skills.size())+" / "+std::to_string(skills().size()),38,496,16,gold);
    wrapped(save.skills.empty() ? "K: skills · O: clothing · L: quests":"K: skills and resident training",38,524,354,14,ink,2);
  }
  float mx=w-280,my=20;
  DrawRectangleRounded({mx,my,260,270},.08f,6,panel);text(save.submerged ? "Seafloor map":"Explored world",mx+16,my+12,16,ink);
  drawMap(save,ui,camera,{mx+15,my+42,230,196},false);
  text("M: map · bright: visible area",mx+16,my+246,13,muted);

  if(ui.debug) {
    DrawRectangleRounded({w-300,302,280,138},.1f,6,panel);
    std::ostringstream perf;perf<<std::fixed<<std::setprecision(1)<<"FPS "<<(ui.fps>0 ? ui.fps : GetFPS())<<"   p95 "<<ui.p95<<" ms";
    text(perf.str(),w-282,315,17,ui.p95>20 ? gold : ink);
    std::ostringstream work;work<<std::fixed<<std::setprecision(2)<<"CPU "<<ui.workMs<<" ms · p99 "<<ui.p99;
    text(work.str(),w-282,342,13,muted);
    text("Chunks "+std::to_string(ui.chunks)+" / pending "+std::to_string(ui.pending),w-282,364,14,muted);
    text("Visible "+std::to_string(ui.visible)+" / animals "+std::to_string(ui.animals),w-282,385,14,muted);
    text(ui.benchmark ? "Auto-route enabled" : "F3: hide performance stats",w-282,409,13,ui.benchmark ? gold : muted);
  }
  DrawRectangleRounded({20,h-96,std::min(w-40,1000.f),76},.12f,6,panel);
  text(w<1100 ? "WASD / arrows · SHIFT · E/T: residents · C: craft · P: town · M: map":"WASD / arrows · SHIFT · E: talk · T: trade · C: craft · P: town · M: map · SPACE: attack",37,h-83,14,ink);
  text(w<1100 ? "SPACE: attack · I: equipment · R/V/G: vessels · F: gather":"F: gather · Q: eat · H: potion · I: gear · V: vessel · R: dive · G: sonar · J: repair",37,h-62,13,muted);
  wrapped(ui.hint.empty() ? "Find towns: their technologies unlock new resources." : ui.hint,37,h-41,std::min(w-74,966.f),16,gold,1);
  if(!ui.notice.empty()) {
    float tw=measure(ui.notice,17),width=std::min(w-40,tw+36);
    DrawRectangleRounded({(w-width)*.5f,28,width,tw>width-36 ? 62.f:38.f},.2f,4,panel);
    wrapped(ui.notice,(w-width)*.5f+18,37,width-36,17,gold,2);
  }
  if(ui.harvestProgress>0) {
    DrawRectangle(int(w*.5f-75),int(h*.5f+50),150,6,{20,32,24,230});
    DrawRectangle(int(w*.5f-75),int(h*.5f+50),int(150*ui.harvestProgress),6,gold);
  }
  if(save.trackedQuest>=0&&save.trackedQuest<int(world.npcs.size())&&save.quests.contains(save.trackedQuest)) {
    auto& progress=save.quests.at(save.trackedQuest);auto& task=quest(progress.id);auto& giver=world.npcs[save.trackedQuest];float y=ui.debug ? 452.f:302.f;
    DrawRectangleRounded({w-300,y,280,92},.08f,6,panel);text(task.title,w-282,y+10,17,gold);
    text(std::to_string(std::min(task.target,questProgress(save,progress)))+" / "+std::to_string(task.target)+" · L: journal",w-282,y+36,16,ink);
    text(giver.name+" · "+std::to_string(int(length(giver.pos-save.player)/tileSize))+" tiles",w-282,y+61,16,muted);
  }
  if(ui.npc>=0) {
    float pw=std::min(720.f,w-50),ph=500,px=(w-pw)*.5f,py=h-ph-96;
    auto& n=world.npcs.at(ui.npc);
    if(n.ruler) {
      float roomY=std::max(20.f,py-125);
      DrawRectangle(int(px-15),int(roomY),int(pw+30),int(py+ph-roomY+15),{73,58,43,255});
      for(float y=roomY;y<py+ph;y+=45) DrawLineEx({px-15,y},{px+pw+15,y},2,{105,82,54,255});
      DrawRectangle(int(px+35),int(roomY+35),70,55,{123,77,43,255});sprite(64+n.type*5,{px+70,roomY+100});
      DrawRectangleRounded({px+44,roomY+7,52,23},.4f,8,{236,227,190,255});for(int i=0;i<3;i++) DrawCircleV({px+57+i*12.f,roomY+18},2,{80,99,75,255});
      text("Audience in the town hall",px+125,roomY+43,20,gold);
    }
    DrawRectangleRounded({px,py,pw,ph},.04f,8,{18,29,28,250});DrawRectangle(int(px),int(py),int(pw),3,gold);
    text(n.name+" · "+n.role,px+24,py+20,24,gold);text("ESC: close",px+pw-125,py+25,16,muted);
    text(settlements()[n.settlement].name,px+24,py+53,16,muted);
    wrapped(npcIntroduction(save,n),px+24,py+79,pw-48,17,ink,3);
    wrapped(questStatus(save,n),px+24,py+155,pw-48,17,gold,5);
    auto q=npcQuest(save,n);auto found=save.quests.find(n.id);
    DrawRectangleRounded({px+20,py+269,pw-40,32},.1f,4,q ? Color{64,84,53,255}:Color{40,51,43,255});
    text(!q ? "No quest offered":found==save.quests.end() ? "1: accept quest":found->second.completed ? "Quest completed":"1: turn in quest",px+32,py+275,17,ink);
    text("F4: learn the town technology from its master",px+24,py+315,16,muted);
    if(auto resident=citizen(save,n.id)) {
      auto weapons=ownedWeapons(save);auto clothes=ownedOutfits(save);clothes.insert(clothes.begin(),"");
      int wi=std::clamp(ui.residentWeapon,0,int(weapons.size())-1),oi=std::clamp(ui.residentOutfit,0,int(clothes.size())-1);
      text("Resident · current job: "+resident->job,px+24,py+349,17,gold);
      text("3: job · "+residentJobs()[ui.residentJob],px+24,py+377,16,ink);
      text("4: weapon · "+(weapons[wi].empty() ? std::string("None"):weapon(weapons[wi]).name),px+24,py+402,16,ink);
      text("5: clothes · "+(clothes[oi].empty() ? std::string("None"):outfit(clothes[oi]).name),px+24,py+427,16,ink);
      text("6: train and assign · K: training requirements",px+24,py+463,16,gold);
    } else wrapped("Not your resident. Trade, quests and knowledge can still build trust.",px+24,py+363,pw-48,18,muted,3);
  }
  if(ui.crafting||ui.trading||ui.civic||ui.equipment||ui.progression||ui.wardrobe||ui.journal) {
    auto weapons=ownedWeapons(save);auto clothes=ownedOutfits(save);std::vector<int> quests;for(auto& [id,q]:save.quests) quests.push_back(id);std::sort(quests.begin(),quests.end());if(quests.empty()) quests.push_back(-1);
    int total=ui.crafting ? int(recipes().size()):ui.trading ? goodTypes:ui.civic ? int(cityActions().size()):ui.progression ? int(skills().size()+researchNodes().size()):ui.wardrobe ? std::max(1,int(clothes.size())):ui.journal ? int(quests.size()):int(weapons.size());
    int pageSize=menuPageRows(),rows=std::min(total,pageSize),page=ui.selection/pageSize*pageSize;auto box=menuBounds(rows);float px=box.x,py=box.y;
    DrawRectangleRounded(box,.035f,6,{18,29,28,250});DrawRectangle(int(px),int(py),int(box.width),3,gold);
    if(ui.civic) {
      float roomX=box.x-110,roomY=box.y-80;
      DrawRectangle(int(roomX),int(roomY),int(box.width+220),int(box.height+130),{73,58,43,255});
      for(int i=0;i<12;i++) DrawLineEx({roomX,roomY+i*55},{roomX+box.width+220,roomY+i*55},2,{105,82,54,255});
      DrawRectangle(int(roomX+30),int(roomY+20),74,48,{123,77,43,255});
      if(!ownsCity(save,ui.merchant)) {for(auto& n:world.npcs) if(n.ruler&&n.settlement==ui.merchant) sprite(64+n.type*5,{roomX+65,roomY+85});}
      text(ownsCity(save,ui.merchant) ? "Your town hall · the ruler's seat is vacant":"Town hall · the ruler welcomes visitors",box.x,roomY+22,18,gold);
      DrawRectangleRounded(box,.035f,6,{18,29,28,250});
    }
    std::string title=ui.crafting ? "Crafting":ui.trading ? "Market":ui.civic ? "Town hall · lessons and diplomacy":ui.progression ? "Skills and resident training":ui.wardrobe ? "Wardrobe":ui.journal ? "Quest journal":"Weapons";
    wrapped(title,px+24,py+18,box.width-205,24,gold,1);text("ESC: close",px+box.width-157,py+26,16,muted);
    if(ui.civic) {
      auto& city=settlements()[ui.merchant];auto relation=save.relations[ui.merchant];
      text(city.name+" · "+rarityName(city.rarity)+" · "+treatyName(relation.treaty)+" · Trust "+std::to_string(relation.opinion),px+24,py+59,16,relation.treaty==Treaty::War ? Color{226,134,110,255}:muted);
    } else if(ui.progression||ui.wardrobe||ui.journal) text("Select a row · ENTER: learn / equip / track",px+24,py+61,16,muted);
    else if(ui.crafting) text("Recipe                              Materials / knowledge",px+24,py+61,16,muted);
    else if(ui.equipment) text("Weapon                              Damage / reach / rarity",px+24,py+61,16,muted);
    else {
      text("Goods",px+24,py+61,16,muted);text("Owned",px+box.width*.31f,py+61,16,muted);
      text("Stock",px+box.width*.48f,py+61,16,muted);text("Buy / sell",px+box.width-250,py+61,16,muted);
    }
    for(int row=0;row<rows&&page+row<total;row++) {
      int i=page+row;float y=py+menuListTop+row*menuRowHeight;
      if(ui.selection==i) DrawRectangleRounded({px+16,y-5,box.width-32,47},.12f,4,{56,70,52,255});
      if(ui.crafting) {
        auto& r=recipes()[i];bool ready=true;std::string costs;
        for(int g=0;g<goodTypes;g++) if(r.cost[g]) {
          if(!costs.empty()) costs+=" · ";
          costs+=std::string(goodName(g))+" "+std::to_string(r.cost[g]);
          if(goodCount(save.inventory,g)<r.cost[g]) ready=false;
        }
        bool skillLocked=!hasSkill(save,"crafting")||!hasSkill(save,recipeSkill(r));
        bool locked=!recipeKnown(save,r)||skillLocked||(!r.technology.empty()&&!save.technologies.contains(r.technology));
        text(r.name,px+24,y,17,equipped(save.inventory,i)||locked ? muted:ink);
        wrapped(equipped(save.inventory,i) ? "Crafted":locked ? "Requires: "+(!recipeKnown(save,r) ? std::string("NPC quest reward"):skillLocked ? skillName(!hasSkill(save,"crafting") ? "crafting":recipeSkill(r)):std::string(technologyName(r.technology))):costs,px+box.width*.38f,y+2,box.width*.62f-24,16,ready&&!locked ? gold:muted,1);
      } else if(ui.progression) {
        if(i<int(skills().size())) wrapped(skills()[i].name+" · "+lessonState(save,i).detail,px+24,y,box.width-48,16,lessonState(save,i).learned ? gold:ink,1);
        else {auto& node=researchNodes()[i-int(skills().size())];text(node.name+" · "+(save.technologies.contains(node.id) ? "researched":"locked"),px+24,y,17,ink);wrapped(node.description,px+24,y+26,box.width-48,16,muted,1);}
      } else if(ui.wardrobe) {
        if(clothes.empty()) text("No clothes yet",px+24,y,17,muted);
        else {auto& item=outfit(clothes[i]);text(item.name+(save.outfitId==item.id ? " · worn":"")+" · free copies "+std::to_string(availableOutfit(save,item.id)),px+24,y,17,ink);}
      } else if(ui.journal) {
        int id=quests[i];text(id<0 ? "Meet residents to find quests":quest(save.quests.at(id).id).title+" · "+(save.quests.at(id).completed ? "completed":"active"),px+24,y,17,ink);
      } else if(ui.trading) {
        text(goodName(i),px+24,y,17,ink);text(std::to_string(goodCount(save.inventory,i)),px+box.width*.34f,y,17,ink);
        text(std::to_string(save.stocks[ui.merchant][i]),px+box.width*.5f,y,17,muted);
        DrawRectangleRounded({px+box.width-254,y-2,102,29},.16f,4,{70,88,60,255});
        DrawRectangleRounded({px+box.width-130,y-2,102,29},.16f,4,{90,76,52,255});
        text("Buy "+std::to_string(buyPrice(ui.merchant,i)),px+box.width-246,y+3,16,ink);
        text("Sell "+std::to_string(sellPrice(ui.merchant,i)),px+box.width-125,y+3,16,ink);
      } else if(ui.civic) {
        bool lesson=i>=10&&i<10+int(skills().size());
        std::string label=lesson ? skills()[i-10].name+" · "+lessonState(save,i-10).detail:cityActions()[i];
        wrapped(label,px+24,y,box.width-48,16,ink,1);
      }
      else {
        Save display;display.inventory.weaponId=weapons[i];auto& item=currentWeapon(display);
        text(item.name,px+24,y,17,save.inventory.weaponId==item.id ? gold:ink);
        text(std::to_string(item.damage)+" / "+std::to_string(int(item.reach))+" / "+rarityName(item.rarity),px+300,y+2,16,muted);
      }
    }
    float bottom=py+menuListTop+rows*menuRowHeight;
    std::string detail=ui.crafting ? recipes()[ui.selection].effect:ui.trading ? "Coins "+std::to_string(save.coins)+" · Treasury "+std::to_string(save.treasury[ui.merchant]):
      ui.civic ? "Town technology: "+std::string(technologyName(content().towns[settlements()[ui.merchant].type].technology)):"Choose a weapon; bows and crossbows consume arrows";
    if(ui.civic&&ui.selection>=10&&ui.selection<10+int(skills().size())) detail=lessonState(save,ui.selection-10).detail;
    if(ui.civic&&ui.selection==10+int(skills().size())&&save.campaign) {
      float y=35;DrawRectangle(360,20,GetScreenWidth()-690,210,{18,29,28,250});
      text("Attitude to visitors: culture, distance and character",375,y,14,gold);y+=25;
      for(int city=0;city<int(settlements().size())&&y<220;city++) if(city!=ui.merchant&&save.relations[city].discovered) {
        text(settlements()[city].name+" · "+std::to_string(save.cityOpinions[ui.merchant][city]),375,y,16,ink);y+=23;
      }
    }
    if(ui.civic&&ui.selection==7) detail="Offer: "+cityWeapon(ui.merchant,save.seed).name+" · "+rarityName(cityWeapon(ui.merchant,save.seed).rarity);
    if(ui.progression) {
      if(ui.selection<int(skills().size())) detail=lessonState(save,ui.selection).detail;
      else {auto& node=researchNodes()[ui.selection-int(skills().size())];detail=researchRequirements(save,node);}
    }
    if(ui.wardrobe&&!clothes.empty()) detail=outfit(clothes[ui.selection]).description;
    if(ui.journal&&quests[ui.selection]>=0) {int id=quests[ui.selection];detail="Giver: "+world.npcs.at(id).name+" · "+settlements()[world.npcs.at(id).settlement].name+" · progress "+std::to_string(questProgress(save,save.quests.at(id)))+" / "+std::to_string(quest(save.quests.at(id).id).target);}
    int selectedLesson=ui.progression&&ui.selection<int(skills().size()) ? ui.selection:
      ui.civic&&ui.selection>=10&&ui.selection<10+int(skills().size()) ? ui.selection-10:-1;
    bool learned=selectedLesson>=0&&lessonState(save,selectedLesson).learned;
    wrapped(detail,px+24,bottom+8,box.width-48,16,muted,2);
    if(selectedLesson>=0) {
      auto state=lessonState(save,selectedLesson);
      float barWidth=std::min(250.f,box.width*.32f);
      DrawRectangleRounded({px+24,bottom+65,barWidth,7},1,12,{45,59,45,255});
      float fill=float(state.progress)/state.target;
      if(fill>0) DrawRectangleRounded({px+24,bottom+65,barWidth*fill,7},1,12,gold);
      text("Skills learned: "+std::to_string(save.skills.size())+" / "+std::to_string(skills().size()),px+barWidth+40,bottom+56,16,gold);
    }
    if(!ui.notice.empty()) wrapped(ui.notice,px+24,bottom+83,box.width-48,16,gold,1);
    text("Page "+std::to_string(page/pageSize+1)+" / "+std::to_string((total+pageSize-1)/pageSize)+" · PGUP / PGDN",px+24,bottom+119,16,muted);
    if(!ui.trading) {
      DrawRectangleRounded({px+box.width-274,bottom+108,250,40},.65f,12,learned ? Color{43,57,47,255}:Color{75,91,60,255});
      text(learned ? "Learned":ui.crafting ? "ENTER: craft":ui.civic ? (selectedLesson>=0 ? "ENTER: learn":"ENTER: select"):ui.progression ? "ENTER: learn":ui.journal ? "ENTER: track":"ENTER: equip",px+box.width-259,bottom+117,16,learned ? muted:ink);
    } else text("ENTER / SHIFT+ENTER",px+box.width-285,bottom+119,16,ink);
  }
  if(ui.cartography) {
    float side=std::min(w-120,h-160);Rectangle map{(w-side)*.5f,(h-side)*.5f,side,side};
    DrawRectangleRounded({map.x-28,map.y-54,map.width+56,map.height+106},.035f,6,{18,29,28,252});
    text(save.submerged ? "Seafloor map":"Map of the explored world",map.x,map.y-38,24,gold);
    drawMap(save,ui,camera,map,true);
    text("Wheel: zoom · Drag: pan · M / ESC: close",map.x,map.y+map.height+16,16,muted);
  }
  if(ui.paused) {
    DrawRectangle(0,0,int(w),int(h),{0,0,0,185});
    auto box=pauseBounds();DrawRectangleRounded(box,.06f,12,{22,35,31,255});
    text("World summary · paused",box.x+20,box.y+16,24,gold);
    drawSummary(save,ui.summary,pauseSummaryBounds());
    wrapped(ui.pauseError.empty() ? (ui.temporarySession ? "Temporary world · no save file":"Quit saves this world · ESC resumes · UP / DOWN chooses a button"):ui.pauseError,box.x+20,box.y+box.height-124,box.width-40,16,ui.pauseError.empty() ? muted:gold,2);
    for(int i=0;i<2;i++) {
      auto button=pauseButton(i);bool hover=CheckCollisionPointRec(GetMousePosition(),button);
      DrawRectangleRounded(button,.65f,12,ui.pauseSelection==i||hover ? Color{81,99,64,255}:Color{36,51,44,255});
      text(i==0 ? "Resume · ESC":"Quit to main menu",button.x+18,button.y+13,18,ink);
    }
  }
}
}
