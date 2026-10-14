#include "render.hpp"
#include <algorithm>
#include <cmath>

namespace frontier {
namespace {
constexpr Color ink{228,232,214,255},muted{151,174,150,255},gold{231,189,115,255};
constexpr float rowHeight=64,cardWidth=162,cardHeight=126,columnStep=184,rowStep=152;
Rectangle bodyOf(Rectangle box) {return {box.x+4,box.y+58,box.width-8,box.height-58};}
void scrollBar(Rectangle box,float scroll,float total) {
  if(total<=box.height) return;
  float height=std::max(22.f,box.height*box.height/total);
  DrawRectangleRounded({box.x+box.width-5,box.y,4,box.height},1,6,{43,63,48,255});
  DrawRectangleRounded({box.x+box.width-5,box.y+scroll/(total-box.height)*(box.height-height),4,height},1,6,{116,153,96,255});
}
}
void Renderer::icon(const std::string& id,Rectangle box,Color tint) const {
  auto found=iconIds_.find(id);if(found==iconIds_.end()) found=iconIds_.find("book");
  if(found==iconIds_.end()||!icons_.id) return;
  int n=found->second;DrawTexturePro(icons_,{float(n%16*64),float(n/16*64),64,64},box,{0,0},0,tint);
}
void Renderer::summaryInput(SummaryView& view,Rectangle box,bool keyboard) const {
  if(keyboard&&(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_RIGHT))) {
    view.tab=(view.tab+(IsKeyPressed(KEY_LEFT) ? 4:1))%5;view.scroll=0;view.horizontal=0;
  }
  if(keyboard&&IsKeyPressed(KEY_PAGE_DOWN)) view.scroll+=box.height*.7f;
  if(keyboard&&IsKeyPressed(KEY_PAGE_UP)) view.scroll-=box.height*.7f;
  if(CheckCollisionPointRec(GetMousePosition(),box)) {
    float wheel=GetMouseWheelMove();
    if(view.tab==4&&IsKeyDown(KEY_LEFT_SHIFT)) view.horizontal-=wheel*70;
    else view.scroll-=wheel*70;
    if(view.tab==4&&IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {auto delta=GetMouseDelta();view.horizontal-=delta.x;view.scroll-=delta.y;}
  }
  view.scroll=std::max(0.f,view.scroll);view.horizontal=std::max(0.f,view.horizontal);
}
void Renderer::drawSummary(const Save& s,SummaryView& view,Rectangle box,bool enabled) {
  const char* tabs[]={"Overview","World","Inventory","Skills","Skill tree"};
  float tabWidth=box.width/5;
  for(int i=0;i<5;i++) {
    Rectangle tab{box.x+i*tabWidth,box.y,tabWidth-5,44};
    bool hover=enabled&&CheckCollisionPointRec(GetMousePosition(),tab);
    DrawRectangleRounded(tab,.55f,12,view.tab==i ? Color{81,108,65,255}:hover ? Color{52,78,55,255}:Color{33,51,42,255});
    wrapped(tabs[i],tab.x+10,tab.y+10,tab.width-20,16,ink,1);
    if(hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {view.tab=i;view.scroll=view.horizontal=0;}
  }
  auto body=bodyOf(box);
  if(view.tab==4) {
    auto cards=progressCards(s);
    if(view.selectedNode<0) view.selectedNode=int(std::find_if(cards.begin(),cards.end(),[](const auto& card){return card.id=="gathering";})-cards.begin());
    view.selectedNode=std::clamp(view.selectedNode,0,int(cards.size())-1);
    wrapped("Light: learned · dark: locked · SHIFT+wheel: pan",body.x,body.y,body.width-12,16,muted,1);
    Rectangle canvas{body.x,body.y+30,body.width,body.height-174};
    int columns=0,rows=0;for(const auto& card:cards) {columns=std::max(columns,card.column+1);rows=std::max(rows,card.row+1);}
    float totalWidth=columns*columnStep,totalHeight=rows*rowStep;
    view.scroll=std::clamp(view.scroll,0.f,std::max(0.f,totalHeight-canvas.height));
    view.horizontal=std::clamp(view.horizontal,0.f,std::max(0.f,totalWidth-canvas.width));
    auto position=[&](const ProgressCard& card){return Rectangle{canvas.x+8+card.column*columnStep-view.horizontal,canvas.y+4+card.row*rowStep-view.scroll,cardWidth,cardHeight};};
    BeginScissorMode(int(canvas.x),int(canvas.y),int(canvas.width),int(canvas.height));
    for(const auto& card:cards) for(const auto& dependency:card.dependencies) {
      auto parent=std::find_if(cards.begin(),cards.end(),[&](const auto& item){return item.id==dependency;});
      if(parent==cards.end()) continue;
      auto a=position(*parent),b=position(card);
      Vector2 from{a.x+a.width,a.y+a.height*.5f},to{b.x,b.y+b.height*.5f};
      Color color=parent->learned ? Color{117,173,102,255}:Color{58,90,63,255};
      if(card.column-parent->column>1) {
        // Route long edges through row/column gutters instead of behind unrelated cells.
        float gutter=b.y+b.height+12;
        const Vector2 points[]={from,{from.x+10,from.y},{from.x+10,gutter},{to.x-10,gutter},{to.x-10,to.y},to};
        for(int p=0;p<5;p++) DrawLineEx(points[p],points[p+1],2,color);
      } else {
        float middle=(from.x+to.x)*.5f;
        DrawLineEx(from,{middle,from.y},2,color);DrawLineEx({middle,from.y},{middle,to.y},2,color);DrawLineEx({middle,to.y},to,2,color);
      }
      DrawLineEx({to.x-6,to.y-4},to,2,color);DrawLineEx({to.x-6,to.y+4},to,2,color);
    }
    for(int i=0;i<int(cards.size());i++) {
      const auto& card=cards[i];auto cell=position(card);
      if(cell.y+cell.height<canvas.y||cell.y>canvas.y+canvas.height||cell.x+cell.width<canvas.x||cell.x>canvas.x+canvas.width) continue;
      DrawRectangleRounded(cell,.15f,10,card.learned ? Color{88,128,69,255}:Color{26,57,38,255});
      if(i==view.selectedNode) DrawRectangleRoundedLinesEx(cell,.15f,10,2,gold);
      icon(card.icon,{cell.x+(cell.width-64)*.5f,cell.y+8,64,64},card.learned ? WHITE:Color{158,177,157,255});
      wrapped(card.activity ? card.name+"\n"+std::to_string(card.amount)+" recorded":card.name,cell.x+8,cell.y+78,cell.width-16,16,ink,2);
      if(card.ready&&!card.learned) DrawCircleV({cell.x+cell.width-12,cell.y+12},4,gold);
      auto mouse=GetMousePosition();if(enabled&&CheckCollisionPointRec(mouse,canvas)&&CheckCollisionPointRec(mouse,cell)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) view.selectedNode=i;
    }
    EndScissorMode();scrollBar(canvas,view.scroll,totalHeight);
    if(totalWidth>canvas.width) {
      float length=canvas.width*canvas.width/totalWidth;
      DrawRectangle(int(canvas.x),int(canvas.y+canvas.height-4),int(canvas.width),4,{43,63,48,255});
      DrawRectangle(int(canvas.x+view.horizontal/(totalWidth-canvas.width)*(canvas.width-length)),int(canvas.y+canvas.height-4),int(length),4,gold);
    }
    const auto& selected=cards[view.selectedNode];
    wrapped(selected.name+" · "+(selected.activity ? (selected.amount>0 ? "recorded":"not started"):selected.learned ? "learned":selected.ready ? "ready":"locked"),body.x,body.y+body.height-138,body.width-12,18,gold,1);
    wrapped(selected.detail,body.x,body.y+body.height-106,body.width-12,16,ink,4);
    return;
  }
  std::vector<SummaryEntry> entries;
  if(view.tab==0) {
    int discovered=0,owned=0,done=0;for(size_t i=0;i<s.relations.size();i++) {discovered+=s.relations[i].discovered;owned+=ownsCity(s,int(i));}
    for(const auto& [id,q]:s.quests) done+=q.completed;
    entries={{"town","Known towns: "+std::to_string(discovered)+" / "+std::to_string(settlements().size()),"Owned towns: "+std::to_string(owned)},
      {"town","Home: "+settlements().at(s.homeCity).name,"Position: "+std::to_string(int(s.player.x))+", "+std::to_string(int(s.player.y))},
      {"heart","Health","",s.health},{"coins","Coins","",s.coins},
      {"map","Explored map","Surface cells: "+std::to_string(s.explored.size())+" · seafloor: "+std::to_string(s.exploredUnderwater.size())},
      {"book","Learned skills: "+std::to_string(s.skills.size())+" / "+std::to_string(skills().size()),"Technologies: "+std::to_string(s.technologies.size())},
      {"person","Background: "+playerCharacter(s).name,"Residents: "+std::to_string(s.citizens.size())},
      {"book","Completed quests: "+std::to_string(done),"Accepted quests: "+std::to_string(s.quests.size())},
      {"gather_sites","Gathered sites: "+std::to_string(s.gathered),"Crafted items: "+std::to_string(s.crafted)+" · fish caught: "+std::to_string(s.caught)},
      {"town_orders","Town orders: "+std::to_string(s.orders),"Walls: "+std::to_string(s.walls.size())+" · campfires: "+std::to_string(s.campfires.size())},
      {"map","World seed: "+std::to_string(s.seed),s.submerged ? "Current layer: underwater":"Current layer: surface"}};
    if(!view.caption.empty()) entries.push_back({"book",view.caption,""});
  } else if(view.tab==1) {
    Rectangle layer{body.x,body.y,std::min(300.f,body.width-8),36};
    DrawRectangleRounded(layer,.5f,10,{52,78,55,255});text(view.underwater ? "Seafloor · switch layer":"Surface · switch layer",layer.x+10,layer.y+7,16,ink);
    if(enabled&&CheckCollisionPointRec(GetMousePosition(),layer)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {view.underwater=!view.underwater;view.scroll=0;}
    float size=std::min({270.f,body.width-12,body.height*.45f});drawSavedMap(s,{body.x,body.y+45,size,size},view.underwater ? 1:0);
    body.y+=size+58;body.height-=size+58;
    for(size_t city=0;city<settlements().size();city++) if(city<s.relations.size()&&s.relations[city].discovered) {
      const auto& town=settlements()[city];const auto& relation=s.relations[city];
      entries.push_back({"town",town.name+(ownsCity(s,int(city)) ? " · owned":" · "+std::string(treatyName(relation.treaty))),"Position: "+std::to_string(int(town.pos.x))+", "+std::to_string(int(town.pos.y))+" · trust "+std::to_string(relation.opinion)});
    }
  } else if(view.tab==2) entries=inventorySummary(s);
  else {
    for(int i=0;i<int(skills().size());i++) entries.push_back({"skill_"+skills()[i].id,skills()[i].name,lessonState(s,i).detail});
    for(const auto& card:progressCards(s)) if(card.icon.starts_with("tech_")||card.id=="civic_charter")
      entries.push_back({card.icon,card.name+(card.learned ? " · learned":" · locked"),card.learned ? "Technology learned":card.detail});
  }
  float total=entries.size()*rowHeight;
  view.scroll=std::clamp(view.scroll,0.f,std::max(0.f,total-body.height));
  BeginScissorMode(int(body.x),int(body.y),int(body.width),int(body.height));
  for(int i=0;i<int(entries.size());i++) {
    float y=body.y+i*rowHeight-view.scroll;if(y+rowHeight<body.y||y>body.y+body.height) continue;
    const auto& entry=entries[i];
    DrawRectangleRounded({body.x,y,body.width-12,rowHeight-5},.12f,8,i%2 ? Color{25,43,34,255}:Color{29,47,37,255});
    icon(entry.icon,{body.x+8,y+12,32,32});
    float countWidth=entry.count>=0 ? 100:0;
    wrapped(entry.name,body.x+52,y+7,body.width-72-countWidth,18,ink,1);
    if(entry.count>=0) {auto count=std::to_string(entry.count);text(count,body.x+body.width-25-measure(count,18),y+8,18,gold);}
    if(!entry.detail.empty()) wrapped(entry.detail,body.x+52,y+34,body.width-72,16,muted,1);
  }
  EndScissorMode();scrollBar(body,view.scroll,total);
}
}
