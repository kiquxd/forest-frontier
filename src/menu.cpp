#include "menu.hpp"
#include "adventure.hpp"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>

namespace frontier {
namespace {
constexpr Color background{17,26,25,255},panel{27,39,35,255},ink{228,232,214,255},muted{143,164,146,255},gold{231,191,114,255};
bool button(Renderer& r,Rectangle box,const std::string& label,bool active=true,bool chosen=false) {
  bool hover=CheckCollisionPointRec(GetMousePosition(),box);
  DrawRectangleRounded({box.x,box.y+3,box.width,box.height},.65f,16,{7,15,14,100});
  DrawRectangleRounded(box,.65f,16,chosen ? Color{81,99,64,255}:hover&&active ? Color{58,76,54,255}:Color{36,51,44,255});
  r.wrapped(label,box.x+16,box.y+std::round((box.height-Renderer::textSize(18))*.5f),box.width-32,18,active ? ink:muted,1);
  return active&&hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
void capture(const std::filesystem::path& path) {
  if(path.empty()) return;
  if(!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
  Image screenshot{rlReadScreenPixels(GetRenderWidth(),GetRenderHeight()),GetRenderWidth(),GetRenderHeight(),1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  if(!screenshot.data) throw std::runtime_error("Cannot capture menu");
  bool ok=ExportImage(screenshot,path.c_str());UnloadImage(screenshot);if(!ok) throw std::runtime_error("Cannot export menu screenshot");
}
std::string updatedAt(std::filesystem::file_time_type value) {
  auto time=std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(value));
  std::tm local{};localtime_r(&time,&local);std::ostringstream out;out<<std::put_time(&local,"%Y-%m-%d %H:%M");return out.str();
}
int menuScene() {
  const char* names[]={"forest","coast","town","underwater"};
  if(const char* requested=std::getenv("FRONTIER_MENU_SCENE")) for(int i=0;i<4;i++) if(std::string(requested)==names[i]) return i;
  static int previous=-1;
  unsigned roll=std::random_device{}();
  int chosen=previous<0 ? int(roll%4):(previous+1+int(roll%3))%4;
  previous=chosen;return chosen;
}
void landscapeTriangle(Vector2 a,Vector2 b,Vector2 c,Color color) {
  // Screen-space Y is downward; keep all procedural faces front-facing.
  if((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x)>0) std::swap(b,c);
  DrawTriangle(a,b,c,color);
}
void landscape(int scene,float w,float h) {
  const Color skies[]={Color{36,70,67,255},Color{48,82,111,255},Color{83,65,87,255},Color{15,64,78,255}};
  DrawRectangleGradientV(0,0,int(w),int(h),skies[scene],{12,25,28,255});
  float horizon=h*.52f;
  if(scene!=3) {
    DrawCircleV({w*.79f,h*.2f},h*.07f,scene==2 ? Color{238,184,125,255}:Color{213,226,203,255});
    for(int i=0;i<8;i++) {
      float x=i*w/6.f,peak=h*(.26f+.07f*std::sin(i*2.f));
      landscapeTriangle({x,horizon},{x+w*.32f,horizon},{x+w*.16f,peak},{39,64,69,255});
    }
    DrawRectangleGradientV(0,int(horizon),int(w),int(h-horizon),{30,72,82,255},{14,35,40,255});
    for(int i=0;i<30;i++) {
      float x=w*.45f+std::sin(i*4.f)*w*.45f,y=horizon+float(i%12)*h*.036f;
      DrawLineEx({x,y},{x+30+i%5*15.f,y},1,{86,132,135,60});
    }
  }
  if(scene==0) {
    for(int i=0;i<26;i++) {
      float x=i*w/24.f,base=h*(.59f+.045f*std::sin(i*2.3f)),height=h*(.1f+.06f*(i%3));
      DrawRectangle(int(x-3),int(base-height*.4f),6,int(height*.5f),{48,54,35,255});
      landscapeTriangle({x,base-height},{x+height*.37f,base},{x-height*.37f,base},{21,48,38,255});
      landscapeTriangle({x,base-height*1.15f},{x+height*.29f,base-height*.24f},{x-height*.29f,base-height*.24f},{31,65,45,255});
    }
    DrawEllipse(int(w*.75f),int(h*.91f),w*.46f,h*.13f,{17,39,31,255});
  } else if(scene==1) {
    landscapeTriangle({w*.88f,h*.54f},{w,h},{w*.65f,h},{42,58,52,255});
    float x=w*.72f,y=h*.7f;
    landscapeTriangle({x-90,y},{x+95,y},{x+40,y+30},{103,71,44,255});
    DrawLineEx({x,y},{x,y-155},4,{148,116,70,255});
    landscapeTriangle({x+5,y-145},{x+85,y-12},{x+5,y-12},{210,208,173,255});
    landscapeTriangle({x-5,y-122},{x-5,y-12},{x-70,y-12},{174,190,160,255});
    float lx=w*.93f,ly=h*.49f;
    DrawRectangle(int(lx-15),int(ly-105),30,110,{172,166,132,255});
    DrawRectangle(int(lx-19),int(ly-110),38,20,{220,183,97,255});
    landscapeTriangle({lx,ly-135},{lx+25,ly-111},{lx-25,ly-111},{86,69,52,255});
  } else if(scene==2) {
    for(int i=0;i<11;i++) {
      float x=w*.48f+i*w*.05f,y=horizon-10+(i%3)*18;
      float bh=55+(i%4)*17.f;
      DrawRectangle(int(x),int(y-bh),int(w*.052f),int(bh),{88,94,73,255});
      landscapeTriangle({x-5,y-bh},{x+w*.056f,y-bh},{x+w*.026f,y-bh-30},{121,82,57,255});
      DrawRectangle(int(x+12),int(y-bh+20),8,13,{224,180,104,255});
      DrawRectangle(int(x+30),int(y-bh+20),8,13,{224,180,104,255});
    }
    DrawRectangle(int(w*.46f),int(horizon+8),int(w*.54f),28,{63,78,69,255});
    for(int i=0;i<18;i++) DrawRectangle(int(w*.46f+i*w*.033f),int(horizon-2),18,14,{79,90,73,255});
    DrawEllipse(int(w*.8f),int(h*.84f),w*.28f,h*.12f,{25,43,39,255});
  } else {
    for(int i=0;i<7;i++) landscapeTriangle({w*(.46f+i*.085f),0},{w*(.5f+i*.085f),0},{w*(.22f+i*.12f),h},{53,124,133,24});
    DrawEllipse(int(w*.74f),int(h*1.02f),w*.6f,h*.28f,{19,44,48,255});
    for(int i=0;i<16;i++) {
      float x=w*(.4f+i*.04f),y=h*(.87f+.06f*std::sin(i));
      DrawLineEx({x,y},{x+10,y-55-(i%3)*22.f},5,{39,106,79,255});
      DrawCircleV({x+10,y-55-(i%3)*22.f},6,{54,130,96,255});
      DrawCircleLines(int(x),int(y-140-(i%4)*55),3+i%4,{94,168,169,120});
    }
    float x=w*.77f,y=h*.48f;
    DrawEllipse(int(x),int(y),96,31,{165,151,83,255});
    DrawRectangleRounded({x-31,y-51,61,28},.4f,8,{124,126,78,255});
    DrawLineEx({x+4,y-49},{x+4,y-78},6,{173,171,108,255});
    for(int i=0;i<4;i++) {DrawCircleV({x-58+i*34.f,y},11,{50,73,76,255});DrawCircleV({x-58+i*34.f,y},7,{93,161,170,255});}
  }
  // Keep controls readable while retaining the landscape on the right.
  DrawRectangleGradientH(0,0,int(w),int(h),{12,22,21,245},{12,22,21,35});
}
}
std::optional<std::filesystem::path> mainMenu(MenuOptions& options) {
  Renderer r;const int scene=menuScene();auto slots=scanSaveSlots(options.directory);int selected=0;
  for(int i=0;i<int(slots.size());i++) if(slots[i].path==options.preferred) selected=i;
  enum class Page {Home,Saves,New,Settings};Page page=options.preview=="newgame" ? Page::New:options.preview=="saves" ? Page::Saves:options.preview=="settings" ? Page::Settings:Page::Home;
  int homeChoice=0,settingChoice=0,frames=0;bool confirmDelete=false,editSeed=false;SummaryView summary;
  int backgroundChoice=0;for(int i=0;i<int(content().characters.size());i++) if(content().characters[i].id=="traveler") backgroundChoice=i;
  Settings draft=options.settings;std::string worldName="New world",seedText,notice;double noticeUntil=0;
  auto notify=[&](const std::string& s){notice=s;noticeUntil=GetTime()+5;};
  auto refresh=[&] {slots=scanSaveSlots(options.directory);selected=std::clamp(selected,0,std::max(0,int(slots.size())-1));summary.scroll=summary.horizontal=0;};
  auto valid=[&] {return !slots.empty()&&slots[selected].save.has_value();};
  SetTargetFPS(options.settings.fps);
  while(!WindowShouldClose()) {
    float w=float(GetScreenWidth()),h=float(GetScreenHeight());
    if(IsKeyPressed(KEY_ESCAPE)) {if(confirmDelete) confirmDelete=false;else if(page!=Page::Home) page=Page::Home;}
    const auto framePage=page;
    BeginDrawing();ClearBackground(background);
    landscape(scene,w,h);
    const float top=120,left=40,listWidth=std::clamp(w*.25f,230.f,330.f),right=left+listWidth+24,detailWidth=w-right-40;
    if(framePage==Page::Home) {
      const std::vector<std::string> actions={"Continue","Saved worlds","New game","Settings","Quit"};
      if(IsKeyPressed(KEY_UP)) homeChoice=(homeChoice+4)%5;
      if(IsKeyPressed(KEY_DOWN)) homeChoice=(homeChoice+1)%5;
      for(int i=0;i<5;i++) {
        bool active=i!=0||valid();
        if(button(r,{left,top+i*62,280,50},actions[i],active,homeChoice==i)||(active&&homeChoice==i&&IsKeyPressed(KEY_ENTER))) {
          if(i==0) {EndDrawing();return slots[selected].path;}
          if(i==1) page=Page::Saves;
          if(i==2) page=Page::New;
          if(i==3) {draft=options.settings;page=Page::Settings;}
          if(i==4) {EndDrawing();return std::nullopt;}
        }
      }
      if(valid()) {
        auto& slot=slots[selected];auto& save=*slot.save;configureWorld(save.seed);
        r.text("Last selected world",right,top,20,muted);r.text(slot.path.stem().string(),right,top+34,28,ink);
        r.text(std::to_string(slot.discovered)+" known towns · "+std::to_string(slot.owned)+" owned · "+std::to_string(save.skills.size())+" skills",right,top+77,18,gold);
        float size=std::min(detailWidth,h-top-185);r.drawSavedMap(save,{right,top+115,size,size});
      } else {r.text("Your journey starts here.",right,top+30,28,ink);r.wrapped("Create a new world. Start with one town, no equipment and everything to discover.",right,top+80,detailWidth,20,muted,4);}
    }
    if(framePage==Page::Saves) {
      r.text("Saved worlds",left,top,24,ink);
      int visible=std::max(1,int((h-top-115)/58));
      if(!confirmDelete) {
        if(IsKeyPressed(KEY_UP)&&!slots.empty()) {selected=(selected+int(slots.size())-1)%int(slots.size());summary.scroll=summary.horizontal=0;}
        if(IsKeyPressed(KEY_DOWN)&&!slots.empty()) {selected=(selected+1)%int(slots.size());summary.scroll=summary.horizontal=0;}
        auto mouse=GetMousePosition();if(mouse.x<right&&!slots.empty()) selected=std::clamp(selected-int(GetMouseWheelMove()),0,int(slots.size())-1);

      }

      int first=selected/visible*visible;
      for(int i=first;i<std::min(first+visible,int(slots.size()));i++) {
        if(button(r,{left,top+43+(i-first)*58,listWidth,48},slots[i].path.stem().string()+(slots[i].save ? "":" [invalid]"),!confirmDelete,selected==i)) {selected=i;summary.scroll=summary.horizontal=0;}
      }
      if(slots.empty()) r.wrapped("No saved worlds. Create a new game to get started.",left,top+50,listWidth,18,muted,4);
      if(button(r,{left,h-76,listWidth,44},"New game",!confirmDelete)) page=Page::New;
      if(!slots.empty()) {
        auto& slot=slots[selected];DrawRectangleRounded({right,top,detailWidth,h-top-90},.03f,6,panel);
        r.text(slot.path.stem().string(),right+20,top+17,26,gold);
        if(!slot.save) r.wrapped("Cannot load this world: "+slot.error,right+20,top+67,detailWidth-40,18,ink,5);
        else {
          auto& save=*slot.save;configureWorld(save.seed);
          summary.caption="Updated: "+updatedAt(slot.modified);
          Rectangle summaryBox{right+20,top+57,detailWidth-40,h-top-165};
          if(!confirmDelete) r.summaryInput(summary,summaryBox);
          r.drawSummary(save,summary,summaryBox,!confirmDelete);
        }
        if(button(r,{right,h-76,170,44},"Load world",valid()&&!confirmDelete)||(!confirmDelete&&valid()&&IsKeyPressed(KEY_ENTER))) {EndDrawing();return slot.path;}
        if(button(r,{right+184,h-76,180,44},"Delete · DEL",!confirmDelete)||(!confirmDelete&&IsKeyPressed(KEY_DELETE))) confirmDelete=true;
      }
    }
    if(framePage==Page::New) {
      float available=std::min(w-80,1150.f);DrawRectangleRounded({left,top,available,h-top-85},.03f,6,panel);
      r.text("Choose your background",left+20,top+12,26,gold);
      r.text("LEFT / RIGHT · "+std::to_string(backgroundChoice+1)+" / "+std::to_string(content().characters.size()),left+available-285,top+19,16,muted);
      float fieldWidth=(available-60)*.5f;
      r.text("World name",left+20,top+54,16,muted);r.text("Seed · empty = random",left+40+fieldWidth,top+54,16,muted);
      Rectangle nameBox{left+20,top+78,fieldWidth,40},seedBox{left+40+fieldWidth,top+78,fieldWidth,40};
      if(CheckCollisionPointRec(GetMousePosition(),nameBox)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) editSeed=false;
      if(CheckCollisionPointRec(GetMousePosition(),seedBox)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) editSeed=true;
      DrawRectangleRounded(nameBox,.1f,4,{40,55,45,255});DrawRectangleRounded(seedBox,.1f,4,{40,55,45,255});
      r.text(worldName+(!editSeed ? "|":""),nameBox.x+10,nameBox.y+7,18,ink);r.text(seedText+(editSeed ? "|":""),seedBox.x+10,seedBox.y+7,18,ink);
      if(IsKeyPressed(KEY_TAB)) editSeed=!editSeed;
      auto& input=editSeed ? seedText:worldName;
      for(int code=GetCharPressed();code>0;code=GetCharPressed()) if(code>=32&&code<127&&input.size()<(editSeed ? 20u:48u)&&(!editSeed||std::isdigit(code))) input+=char(code);
      if((IsKeyPressed(KEY_BACKSPACE)||IsKeyPressedRepeat(KEY_BACKSPACE))&&!input.empty()) input.pop_back();
      int count=int(content().characters.size());if(IsKeyPressed(KEY_LEFT)) backgroundChoice=(backgroundChoice+count-1)%count;if(IsKeyPressed(KEY_RIGHT)) backgroundChoice=(backgroundChoice+1)%count;
      int visibleCards=std::min(count,std::max(1,int((available-40)/215)));
      int firstCard=backgroundChoice/visibleCards*visibleCards;
      float cardWidth=(available-40)/visibleCards;
      for(int i=firstCard;i<std::min(firstCard+visibleCards,count);i++) {
        auto& hero=content().characters[i];Rectangle card{left+20+(i-firstCard)*cardWidth,top+134,cardWidth-8,260};
        if(button(r,card,"",true,backgroundChoice==i)) backgroundChoice=i;
        float cx=card.x+card.width*.5f;Color coat=i%2 ? Color{107,154,124,255}:Color{179,141,95,255};
        DrawRectangle(int(cx-10),int(card.y+34),20,25,coat);DrawCircleV({cx,card.y+23},8,{207,170,129,255});
        DrawRectangle(int(cx-9),int(card.y+59),6,12,{74,82,71,255});DrawRectangle(int(cx+3),int(card.y+59),6,12,{74,82,71,255});
        r.wrapped(hero.name,card.x+10,card.y+78,card.width-20,19,gold,1);
        r.wrapped("+ "+hero.pros,card.x+10,card.y+108,card.width-20,16,ink,3);
        r.wrapped("- "+hero.cons,card.x+10,card.y+198,card.width-20,16,muted,2);
      }
      r.wrapped(content().characters[backgroundChoice].lore,left+20,top+405,available-40,17,ink,h<760 ? 1:2);
      if(button(r,{left+20,h-76,240,44},"Create and play")||IsKeyPressed(KEY_ENTER)) {
        try {uint64_t seed=seedText.empty() ? (uint64_t(std::random_device{}())<<32)^std::random_device{}():std::stoull(seedText);
          auto path=createSaveSlot(options.directory,worldName,seed,content().characters[backgroundChoice].id);EndDrawing();return path;
        } catch(const std::exception& e) {notify(e.what());}
      }
    }
    if(framePage==Page::Settings) {
      r.text("Settings",left,top,28,gold);
      std::vector<std::string> labels={std::string("Fullscreen: ")+(draft.fullscreen ? "on":"off"),"Window resolution: "+std::to_string(draft.width)+" x "+std::to_string(draft.height),"Frame limit: "+(draft.fps ? std::to_string(draft.fps):"uncapped"),"Camera zoom: "+std::to_string(int(draft.zoom*100))+"%",std::string("Performance HUD: ")+(draft.debug ? "on":"off")};
      if(IsKeyPressed(KEY_UP)) settingChoice=(settingChoice+4)%5;
      if(IsKeyPressed(KEY_DOWN)) settingChoice=(settingChoice+1)%5;
      for(int i=0;i<5;i++) if(button(r,{left,top+49+i*52,std::min(w-80,650.f),43},labels[i],true,settingChoice==i)||(settingChoice==i&&(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_LEFT)))) {
        settingChoice=i;int delta=IsKeyPressed(KEY_LEFT) ? -1:1;
        if(i==0) draft.fullscreen=!draft.fullscreen;
        if(i==1) {const std::vector<std::pair<int,int>> sizes={{1280,720},{1600,900},{1920,1080},{2560,1440}};int index=0;for(int j=0;j<4;j++) if(sizes[j].first==draft.width) index=j;auto size=sizes[(index+delta+4)%4];draft.width=size.first;draft.height=size.second;}
        if(i==2) {const std::vector<int> caps={30,60,120,144,0};auto found=std::find(caps.begin(),caps.end(),draft.fps);int index=found==caps.end() ? 1:int(found-caps.begin());draft.fps=caps[(index+delta+5)%5];}
        if(i==3) draft.zoom=std::clamp(draft.zoom+delta*.1f,.65f,1.7f);
        if(i==4) draft.debug=!draft.debug;
      }
      r.wrapped("Arrow keys choose settings; LEFT / RIGHT change values. F5 applies settings. Windowed launch is the default; fullscreen is an explicit session option.",left,top+335,std::min(w-80,800.f),18,muted,3);
      if(button(r,{left,h-76,280,44},"Apply settings · F5")||IsKeyPressed(KEY_F5)) {
        try {writeSettings(options.settingsPath,draft);options.settings=draft;
          if(IsWindowFullscreen()!=draft.fullscreen) {if(draft.fullscreen) SetWindowSize(GetMonitorWidth(GetCurrentMonitor()),GetMonitorHeight(GetCurrentMonitor()));ToggleFullscreen();}
          if(!draft.fullscreen) SetWindowSize(draft.width,draft.height);
          SetTargetFPS(draft.fps);notify("Settings saved");
        } catch(const std::exception& e) {notify(e.what());}
      }
    }
    r.text("FOREST FRONTIER",40,30,36,gold);
    if(page!=Page::Home&&button(r,{w-155,32,115,44},"Back",!confirmDelete)) page=Page::Home;
    if(confirmDelete&&!slots.empty()) {
      DrawRectangle(0,0,int(w),int(h),{0,0,0,185});Rectangle box{(w-560)*.5f,(h-230)*.5f,560,230};DrawRectangleRounded(box,.06f,8,panel);
      r.text("Delete this saved world?",box.x+24,box.y+24,24,gold);
      r.wrapped(slots[selected].path.stem().string()+" will be removed from your saved worlds. You can recover its file from saves/.trash.",box.x+24,box.y+71,512,18,ink,3);
      if(button(r,{box.x+24,box.y+166,230,44},"Confirm · ENTER")||IsKeyPressed(KEY_ENTER)) {try {deleteSaveSlot(options.directory,slots[selected].path);confirmDelete=false;refresh();notify("World deleted");} catch(const std::exception& e) {confirmDelete=false;notify(e.what());}}
      if(button(r,{box.x+278,box.y+166,230,44},"Cancel")) confirmDelete=false;
    }
    if(!notice.empty()&&GetTime()<noticeUntil) {DrawRectangle(30,int(h-32),int(w-60),27,panel);r.wrapped(notice,40,h-29,w-80,16,gold,1);}
    EndDrawing();frames++;
    if(options.frames>0&&frames>=options.frames) {capture(options.screenshot);return std::nullopt;}
  }
  return std::nullopt;
}
}
