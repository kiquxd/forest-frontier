#include "world.hpp"
#include "perf.hpp"
#include "dialogue.hpp"
#include "render.hpp"
#include "gameplay.hpp"
#include "menu.hpp"
#include "adventure.hpp"
#include "raylib.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <random>

using namespace frontier;
static std::vector<std::string> graphicsInfo;
static void toggleFullscreen() {
  static Vector2 windowed{1600,900};
  if(!IsWindowFullscreen()) {
    windowed={float(GetScreenWidth()),float(GetScreenHeight())};
    int monitor=GetCurrentMonitor();
    // raylib keeps the requested logical size during fullscreen; match the native mode.
    SetWindowSize(GetMonitorWidth(monitor),GetMonitorHeight(monitor));
    ToggleFullscreen();
  } else {
    ToggleFullscreen();SetWindowSize(int(windowed.x),int(windowed.y));
  }
}
static void startupTrace(int level,const char* format,va_list args) {
  char line[2048];std::vsnprintf(line,sizeof(line),format,args);
  std::string message=line;
  if(message.find("Vendor:")!=std::string::npos||message.find("Renderer:")!=std::string::npos||message.find("Version:")!=std::string::npos)
    graphicsInfo.push_back(message);
  if(level>=LOG_WARNING) std::cerr<<message<<'\n';
}
struct Options {
  int width=1600,height=900,port=8080,frames=0;
  bool uncapped=false,benchmarkLlm=false,benchmarkGameplay=false,noSave=false;
  bool benchmarkExpedition=false,benchmarkCommunity=false;
  bool saveExplicit=false,portExplicit=false,fullscreen=false,direct=false,debug=true,dialogues=true;
  int fps=60;float zoom=1.15f;
  std::optional<Vec> spawn;
  std::string preview;
  double benchmarkSeconds=0;
  uint64_t seed=742019;bool seedExplicit{};
  std::filesystem::path settingsPath=std::string(FRONTIER_ROOT)+"/settings.json";
  std::filesystem::path logDir=std::string(FRONTIER_ROOT)+"/logs";
  std::filesystem::path savePath=std::string(FRONTIER_ROOT)+"/saves/world.json", screenshot;
};
static Options parse(int argc,char** argv) {
  Options o;
  for(int i=1;i<argc;i++) {
    std::string arg=argv[i];
    auto value=[&]()->std::string {if(i+1>=argc) throw std::runtime_error("Missing value for "+arg);return argv[++i];};
    if(arg=="--play") o.direct=true;
    else if(arg=="--uncapped") o.uncapped=true;
    else if(arg=="--fullscreen") o.fullscreen=true;
    else if(arg=="--benchmark") {o.benchmarkSeconds=std::stod(value());o.noSave=true;}
    else if(arg=="--benchmark-llm") o.benchmarkLlm=true;
    else if(arg=="--benchmark-gameplay") o.benchmarkGameplay=true;
    else if(arg=="--benchmark-community") {o.benchmarkCommunity=true;o.benchmarkExpedition=true;o.benchmarkGameplay=true;}
    else if(arg=="--benchmark-expedition") {o.benchmarkExpedition=true;o.benchmarkGameplay=true;}
    else if(arg=="--spawn") {float x=std::stof(value()),y=std::stof(value());o.spawn=Vec{x,y};o.noSave=true;}
    else if(arg=="--preview") {o.preview=value();o.noSave=true;}
    else if(arg=="--no-save") o.noSave=true;
    else if(arg=="--width") o.width=std::stoi(value());
    else if(arg=="--height") o.height=std::stoi(value());
    else if(arg=="--llm-port") {o.port=std::stoi(value());o.portExplicit=true;}
    else if(arg=="--frames") o.frames=std::stoi(value());
    else if(arg=="--seed") {o.seed=std::stoull(value());o.seedExplicit=true;}
    else if(arg=="--settings") o.settingsPath=value();
    else if(arg=="--log-dir") o.logDir=value();
    else if(arg=="--save") {o.savePath=value();o.saveExplicit=true;}
    else if(arg=="--screenshot") o.screenshot=value();
    else if(arg=="--help") {
      std::cout<<"forest_frontier [--uncapped] [--benchmark SECONDS] [--benchmark-llm]\n"
        "  [--play] [--width N --height N] [--llm-port N] [--seed N] [--no-save]\n"
        "  [--settings FILE] [--frames N] [--log-dir DIR] [--save FILE] [--screenshot FILE]\n"
        "  [--fullscreen] [--benchmark-gameplay] [--benchmark-expedition] [--benchmark-community] [--spawn X Y]\n"
        "  [--preview menu|newgame|saves|settings|quests|skills|clothes|residents|houses|craft|trade|city|map|talk|ruler|ship|submarine|weapons|combat]\n";
      std::exit(0);
    } else throw std::runtime_error("Unknown option: "+arg);
  }
  if(o.width<800||o.width>7680||o.height<600||o.height>4320||o.port<1||o.port>65535||o.frames<0||!std::isfinite(o.benchmarkSeconds)||o.benchmarkSeconds<0)
    throw std::runtime_error("Invalid window size, port, frames or benchmark duration");
  if(o.benchmarkLlm && o.benchmarkSeconds<=0) throw std::runtime_error("--benchmark-llm requires --benchmark SECONDS");
  if(o.benchmarkGameplay&&o.benchmarkSeconds<=0) throw std::runtime_error("--benchmark-gameplay requires --benchmark SECONDS");
  if(o.spawn&&(!std::isfinite(o.spawn->x)||!std::isfinite(o.spawn->y)||length(*o.spawn)>1e6)) throw std::runtime_error("Invalid spawn");
  const std::vector<std::string> previews={"menu","newgame","saves","settings","quests","skills","clothes","residents","houses","craft","trade","city","map","talk","ruler","ship","submarine","weapons","combat"};
  if(!o.preview.empty()&&std::find(previews.begin(),previews.end(),o.preview)==previews.end()) throw std::runtime_error("Unknown preview");
  return o;
}
static std::string npcPrompt(const Npc& n,const Save& s) {
  return "You are a character in Forest Frontier, a quiet exploration and survival game. Reply only in English, in first person, in 1-3 short sentences. "
    "Do not output reasoning, tags or lists. Never mention AI or language models. "
    "Your name is "+n.name+", your role is "+n.role+", and you live in "+settlements()[n.settlement].name+". "+n.personality+
    (n.originCity>=0 ? " You are visiting from "+settlements()[n.originCity].name+". The host's attitude to your town is "+std::to_string(s.campaign ? s.cityOpinions[n.settlement][n.originCity]:0)+". Unwelcome visitors stay outside the central square.":" ")+
    (n.ruler ? " You are the ruler and receive visitors inside the town hall.":" ")+
    (ownsCity(s,n.settlement) ? " The player owns this town.":" This town is independent of the player.")+
    " Town halls offer free skill lessons after practical milestones. Gathering is the first lesson and has no prerequisite. Learned skills: "+std::to_string(s.skills.size())+
    ". The world has pine forests, lakes, a coast, wolves, and towns with different technologies. "
    "Player resources: wood "+std::to_string(s.inventory.wood)+", stone "+std::to_string(s.inventory.stone)+", berries "+std::to_string(s.inventory.berries)+", coins "+std::to_string(s.coins)+". "
    "WASD or arrow keys: movement. E near residents: talk; P at a town hall: lessons and audience; T: trade; C: craft; M: map; I: equipment; V: vessel; R: dive; G: sonar. "
    "A fishing rod catches fish; a boat crosses deep water; campfires heal and deter wolves. Skills and technologies unlock crafting. "
    "This town's technology: "+technologyName(content().towns[settlements()[n.settlement].type].technology)+". Trust: "+std::to_string(s.relations[n.settlement].opinion)+", treaty: "+treatyName(s.relations[n.settlement].treaty)+
    ". Technology research requires peace, trust 20, craftsmanship and 20 coins. Skill lessons are free. Alliance requires trust 30; union requires trust 70 and an alliance. Guards attack during war. "
    "Pineford teaches ironworking; Saltwater Wharf teaches shipbuilding; Copper Beacon teaches pressure hulls; Moon Academy teaches optics; Pearl Haven teaches deep-sea harvesting. "
    "You only converse. Never claim you changed resources, treaties or skills; those changes happen through gameplay buttons.";
}
static std::string cpuName() {
  std::ifstream f("/proc/cpuinfo");std::string line;
  while(std::getline(f,line)) if(line.starts_with("model name")) return line.substr(line.find(':')+2);
  return "unknown";
}
static int play(const Options& options) {
  Perf perf(options.logDir);
  Save save;
  if(!options.noSave) save=loadSave(options.savePath);
  if(options.noSave||!std::filesystem::exists(options.savePath)) save.seed=options.seed;
  if(options.spawn) save.player=*options.spawn;
  if(options.benchmarkGameplay||!options.preview.empty()) {
    save.inventory.wood=60;save.inventory.stone=30;save.inventory.berries=20;
    save.inventory.reeds=12;save.inventory.leather=8;save.coins=60;
  }
  const bool fixture=options.benchmarkSeconds>0||!options.preview.empty();
  const bool legacy=!options.noSave&&std::filesystem::exists(options.savePath);
  if(!legacy&&!fixture&&!options.spawn&&!options.seedExplicit) save.seed=(uint64_t(std::random_device{}())<<32)^std::random_device{}();
  configureWorld(save.seed);ensureCityState(save);
  if(!fixture) initializeCampaign(save,legacy);
  if(!fixture) initializeAdventure(save);
  if(options.spawn) save.player=*options.spawn;
  if(options.benchmarkExpedition) {
    save.coins=10000;save.inventory.wood=200;save.inventory.stone=100;save.inventory.reeds=100;save.inventory.fish=100;
    for(int city=0;city<9;city++) {
      if(save.relations[city].treaty==Treaty::War) diplomacy(save,city,1);
      while(save.relations[city].opinion<20) diplomacy(save,city,0);
      if(!save.technologies.contains(content().towns[settlements()[city].type].technology)) learnTechnology(save,city);
      for(int good:content().towns[settlements()[city].type].goods) {
        while(save.stocks[city][good]>0) if(!trade(save,city,good,true).ok) break;
      }
    }
    diplomacy(save,0,0);diplomacy(save,0,3);
    while(save.relations[0].opinion<70) if(!diplomacy(save,0,8).ok) break;
    diplomacy(save,0,4);diplomacy(save,0,6);diplomacy(save,0,6);
  }
  if(!options.preview.empty()) {
    for(auto& type:content().towns) save.technologies.insert(type.technology);
    save.inventory.rare.fill(50);save.coins=500;save.inventory.sword=true;save.inventory.weapons.insert("wood_sword");save.inventory.weaponId="wood_sword";
    if(options.preview=="ship"||options.preview=="submarine") {
      save.inventory.boat=true;save.inventory.ship=true;save.inventory.submarine=true;save.inventory.sonar=true;
      save.vehicle=options.preview=="ship" ? 2:3;save.submerged=options.preview=="submarine";
      if(!options.spawn) save.player={1100,-1100};
    }
    if(options.preview=="weapons") for(auto& item:content().weapons) save.inventory.weapons.insert(item.id);
    if(options.preview=="quests"||options.preview=="skills"||options.preview=="clothes"||options.preview=="residents") {
      initializeCampaign(save,true);initializeAdventure(save);for(auto& item:content().outfits) save.outfitCopies[item.id]=1;
      if(options.preview=="clothes") equipOutfit(save,"forager");
    }
  }
  if(!IsWindowReady()) {
    SetTraceLogLevel(LOG_INFO);SetTraceLogCallback(startupTrace);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(options.width,options.height,"Forest Frontier");
    SetTraceLogLevel(LOG_WARNING);SetTraceLogCallback(nullptr);
  }
  if(!IsWindowReady()) throw std::runtime_error("Cannot create window / OpenGL context");
  SetWindowMinSize(900,650);SetExitKey(KEY_NULL);SetTargetFPS(options.uncapped ? 0 : options.fps);
  if(options.fullscreen!=IsWindowFullscreen()) toggleFullscreen();
  {
    Renderer renderer;
    World world(save.seed);
    Gameplay game;
    Dialogue dialogue(options.port,options.benchmarkLlm&&options.dialogues);
    std::vector<std::vector<Message>> history(world.npcs.size());
    std::vector<std::string> replies(world.npcs.size());
    Ui ui;ui.debug=options.debug;ui.temporarySession=options.noSave; ui.benchmark=options.benchmarkSeconds>0;
    ui.summary.caption="Live world · includes current unsaved progress";
    if(options.preview=="skills") {ui.progression=true;ui.selection=int(skills().size());}
    if(options.preview=="clothes") ui.wardrobe=true;
    if(options.preview=="quests"||options.preview=="residents") {ui.npc=0;world.beginConversation(0,save);acceptQuest(save,world.npcs[0]);}
    if(options.preview=="craft") ui.crafting=true;
    if(options.preview=="trade") {ui.trading=true;ui.merchant=0;}
    if(options.preview=="city") {ui.civic=true;ui.merchant=0;}
    if(options.preview=="weapons") ui.equipment=true;
    if(options.preview=="map") {ui.cartography=true;ui.mapCenter=save.player;ui.mapRange=2500;}
    if(options.preview=="talk") {ui.npc=0;world.beginConversation(0,save);}
    if(options.preview=="ruler") {ui.npc=1;world.beginConversation(1,save);}
    bool communityAssigned=false;
    if(options.benchmarkCommunity) {
      initializeCampaign(save,true);initializeAdventure(save);
      auto& giver=world.npcs[0];auto accepted=acceptQuest(save,giver);auto completed=finishQuest(save,giver);
      int clothing=-1;for(int i=0;i<int(recipes().size());i++) if(recipes()[i].id=="outfit_forager") clothing=i;
      bool prepared=accepted.ok&&completed.ok&&clothing>=0;
      if(clothing>=0) {prepared=craft(save,clothing).ok&&prepared;prepared=craft(save,clothing).ok&&prepared;}
      prepared=craft(save,2).ok&&prepared;prepared=craft(save,2).ok&&prepared;
      for(int i=0;i<6;i++) prepared=trade(save,save.homeCity,5,true).ok&&prepared;
      prepared=research(save,0).ok&&prepared;prepared=assignCitizen(save,save.citizens[0].npc,"gatherer","","forager").ok&&prepared;
      perf.event({{"event","benchmark_community"},{"ok",prepared},{"quests",save.quests.size()},{"residents",save.citizens.size()}});
    }
    Camera2D camera{};camera.zoom=options.zoom;
    float desiredZoom=save.submerged ? .85f:options.zoom;bool lastLayer=save.submerged;double nextExplore=0;
    Coord previous{INT32_MAX,INT32_MAX}; int previousRadius=-1;
    double simAccumulator=0,simulationTime=0,lastFlush=0,lastSave=0,noticeUntil=0,lastLlm=-20,nextPlan=0;
    std::vector<Vec> routePath;size_t pathIndex=0;double pathMsTotal=0,pathMsMax=0;int plans=0;
    bool spawnReady=false;int scenarioStep=0;
    float gathering=0;uint64_t gatheringId=0;
    int frameCount=0,routeIndex=0;double pauseUntil=5;
    Clock::time_point sessionStart=Clock::now(),lastPresentation=sessionStart;
    std::vector<Vec> route={Vec{120,140},Vec{2120,840},Vec{2750,1000},Vec{-1000,-900},Vec{120,140}};
    if(options.benchmarkGameplay) route={Vec{120,140},Vec{2120,840},Vec{2200,-450},Vec{1100,-1100},Vec{1100,-1900},Vec{120,140}};
    auto context=[&] {
      size_t animals=0,resources=0,enemies=0,activeEnemies=0;
      for(auto& [coord,c]:world.chunks) {
        animals+=c.animals.size();resources+=c.resources.size();
        for(auto& e:c.enemies) if(!save.defeated.contains(e.id)) {enemies++;if(length(e.home-save.player)<=1300) activeEnemies++;}
      }
      return nlohmann::json{{"chunks",world.chunks.size()},{"pending_chunks",world.pending()},
        {"visible_objects",ui.visible},{"resident_animals",animals},{"resident_resources",resources},
        {"resident_enemies",enemies},{"active_enemies",activeEnemies},
        {"player",{save.player.x,save.player.y}},{"zoom",camera.zoom},{"width",GetScreenWidth()},{"height",GetScreenHeight()},
        {"framebuffer_width",GetRenderWidth()},{"framebuffer_height",GetRenderHeight()},
        {"chunk_generation_count",world.generatedCount},{"chunk_generation_ms_total",world.generatedMs},
        {"chunk_generation_ms_max",world.maxGenerationMs},{"harvested",save.harvested.size()},
        {"route",ui.benchmark},{"llm_busy",dialogue.snapshot().busy},
        {"health",save.health},{"coins",save.coins},{"attacks",game.attacks},{"kills",game.kills},
        {"damage_taken",game.damageTaken},{"respawns",game.respawns},{"collision_frames",game.collisions},
        {"path_plans",plans},{"path_ms_total",pathMsTotal},{"path_ms_max",pathMsMax},
        {"biome",terrainName(terrainAt(save.player,save.seed))},{"submerged",save.submerged},
        {"known_surface_cells",save.explored.size()},{"known_underwater_cells",save.exploredUnderwater.size()},
        {"home_city",save.homeCity},{"skills",save.skills.size()},{"gathered",save.gathered},{"crafted",save.crafted},
        {"paused",ui.paused},{"summary_tab",ui.paused ? ui.summary.tab:-1},
        {"owned_cities",std::count(save.owners.begin(),save.owners.end(),0)},
        {"quests",save.quests.size()},{"resident_jobs",save.citizens.size()},{"outfit",save.outfitId},
        {"walls",save.walls.size()},{"technologies",save.technologies.size()},{"weapons",save.inventory.weapons.size()},
        {"oxygen",save.oxygen},{"hull",save.hull}};
    };
    perf.event({{"event","session_start"},{"cpu",cpuName()},{"seed",save.seed},{"width",options.width},
      {"height",options.height},{"graphics",graphicsInfo},{"fps_limit",options.uncapped ? 0 : options.fps},{"benchmark_s",options.benchmarkSeconds},
      {"benchmark_llm",options.benchmarkLlm},{"llm_port",options.port},{"raylib",RAYLIB_VERSION},
      {"benchmark_expedition",options.benchmarkExpedition},{"npc_types",content().npcs.size()},{"weapon_types",content().weapons.size()},
      {"settlements",settlements().size()},{"recipes",recipes().size()},
      {"build", "Release recommended"},{"timing_note","render_submit is CPU only; present_wait includes GPU/driver and FPS limiter"}});
    std::cout<<"Performance log: "<<perf.path()<<'\n';
    auto notify=[&](std::string s,double now) {ui.notice=std::move(s);noticeUntil=now+3;};
    auto ask=[&](int id,const std::string& input,double now) {
      if(!options.dialogues) {notify("NPC dialogue is disabled in settings",now);return;}
      if(id<0||input.empty()||dialogue.snapshot().busy) return;
      auto messages=history[id];
      while(messages.size()>4) messages.erase(messages.begin(),messages.begin()+2);
      messages.push_back({"user",input});
      if(dialogue.ask(id,npcPrompt(world.npcs[id],save),messages)) {
        history[id]=std::move(messages);replies[id].clear();ui.input.clear();lastLlm=now;
        perf.event({{"event","llm_request"},{"npc_id",id},{"history_messages",history[id].size()},{"max_tokens",128}});
      }
    };
    bool quit=false,savedForExit=false;
    while(!quit&&!WindowShouldClose()) {
      const double now=msSince(sessionStart)/1000;
      float dt=std::clamp(GetFrameTime(),.00001f,.05f);
      const auto updateStart=Clock::now();
      game.moving=false;
      if(now>noticeUntil) ui.notice.clear();
      Reply completed;
      if(dialogue.takeCompletion(completed)) {
        if(completed.error.empty()) {
          history[completed.npcId].push_back({"assistant",completed.text});
          while(history[completed.npcId].size()>8) history[completed.npcId].erase(history[completed.npcId].begin(),history[completed.npcId].begin()+2);
          replies[completed.npcId]=completed.text;
        } else {
          if(!history[completed.npcId].empty()&&history[completed.npcId].back().role=="user") history[completed.npcId].pop_back();
          replies[completed.npcId]="No reply received. Check the local model server and try again.";
        }
        perf.event({{"event","llm_result"},{"npc_id",completed.npcId},{"ok",completed.error.empty()},
          {"error",completed.error},{"total_ms",completed.totalMs},{"first_token_ms",completed.firstTokenMs},
          {"prompt_tokens",completed.promptTokens},{"output_tokens",completed.outputTokens},
          {"prompt_ms",completed.promptMs},{"generation_ms",completed.generationMs},
          {"output_tokens_per_s",completed.generationMs>0 ? completed.outputTokens*1000/completed.generationMs : 0}});
      }
      if(IsKeyPressed(KEY_F3)) ui.debug=!ui.debug;
      if(IsKeyPressed(KEY_F11)) toggleFullscreen();
      const bool pauseWasActive=ui.paused;
      if(IsKeyPressed(KEY_TAB)&&!ui.paused&&ui.npc<0&&!ui.crafting&&!ui.trading&&!ui.civic&&!ui.equipment&&!ui.cartography&&!ui.progression&&!ui.wardrobe&&!ui.journal) {
        ui.benchmark=!ui.benchmark;pauseUntil=now;routeIndex=0;routePath.clear();pathIndex=0;nextPlan=0;
      }
      if(IsKeyPressed(KEY_ESCAPE)) {
        if(ui.paused) ui.paused=false;
        else if(ui.blocked()) {
          ui.npc=-1;ui.input.clear();ui.crafting=false;ui.trading=false;ui.civic=false;ui.equipment=false;ui.cartography=false;ui.progression=ui.wardrobe=ui.journal=false;world.talkingNpc=-1;
        }
        else {ui.paused=true;ui.pauseSelection=0;ui.pauseError.clear();}
      }
      if(ui.paused) {
        renderer.summaryInput(ui.summary,Renderer::pauseSummaryBounds());
        if(IsKeyPressed(KEY_UP)||IsKeyPressed(KEY_DOWN)) ui.pauseSelection=1-ui.pauseSelection;
        bool activate=IsKeyPressed(KEY_ENTER);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) for(int i=0;i<2;i++) {
          if(CheckCollisionPointRec(GetMousePosition(),Renderer::pauseButton(i))) {ui.pauseSelection=i;activate=true;}
        }
        if(activate) {
          if(ui.pauseSelection==0) ui.paused=false;
          else {
            try {
              auto started=Clock::now();
              if(!options.noSave) writeSave(options.savePath,save);
              savedForExit=true;quit=true;
              perf.event({{"event","quit_to_menu"},{"saved",!options.noSave},{"duration_ms",msSince(started)}});
            } catch(const std::exception& error) {
              ui.pauseError="Could not save: "+std::string(error.what())+". Your session is still open. Retry Quit after fixing the save path.";
              perf.event({{"event","save_failed"},{"reason","quit_to_menu"},{"error",error.what()}});
            }
          }
        }
      }
      const Resource* resource=nullptr;
      if(!ui.paused&&!quit&&!pauseWasActive) {
        game.advance(dt);
        if(ui.cartography) {
          ui.mapRange=std::clamp(ui.mapRange*(1-GetMouseWheelMove()*.12f),900.f,18000.f);
          if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {auto delta=GetMouseDelta();ui.mapCenter=ui.mapCenter-Vec{delta.x,delta.y}*(ui.mapRange*2/std::min(GetScreenWidth()-120,GetScreenHeight()-160));}
        }
        if((ui.npc<0&&!ui.crafting&&!ui.trading&&!ui.civic&&!ui.equipment&&!ui.cartography&&!ui.progression&&!ui.wardrobe&&!ui.journal)||ui.benchmark) {
          if(!ui.cartography) desiredZoom=std::clamp(desiredZoom+GetMouseWheelMove()*.1f,.65f,1.7f);
          Vec move{};
          if(ui.benchmark) {
            if(now>=pauseUntil) {
              auto d=route[routeIndex]-save.player;
              if(length(d)<55) {routeIndex=(routeIndex+1)%int(route.size());pauseUntil=now+5;routePath.clear();pathIndex=0;nextPlan=0;}
              else {
                if((now>=nextPlan||(pathIndex>=routePath.size()&&!routePath.empty()))&&spawnReady) {
                  auto t=Clock::now();routePath=world.path(save.player,route[routeIndex],save);pathIndex=0;
                  double cost=msSince(t);pathMsTotal+=cost;pathMsMax=std::max(pathMsMax,cost);plans++;nextPlan=now+2;
                }
                while(pathIndex<routePath.size()&&length(routePath[pathIndex]-save.player)<10) pathIndex++;
                Vec direction=pathIndex<routePath.size() ? routePath[pathIndex]-save.player : Vec{};
                move=normalized(direction)*std::min(length(direction),dt*480);
              }
            }
          } else {
            Vec screen{float((IsKeyDown(KEY_RIGHT)||IsKeyDown(KEY_D))-(IsKeyDown(KEY_LEFT)||IsKeyDown(KEY_A))),float((IsKeyDown(KEY_DOWN)||IsKeyDown(KEY_S))-(IsKeyDown(KEY_UP)||IsKeyDown(KEY_W)))};
            move=normalized(unproject(screen))*(dt*(IsKeyDown(KEY_LEFT_SHIFT) ? 340.f : 185.f));
          }
          game.movePlayer(world,save,move);
          if(length(save.player)>1e6f) save.player=normalized(save.player)*1e6f;
        }
        int nearestNpc=-1;float npcDistance=150;
        for(auto& n:world.npcs) {if(n.ruler) continue;float d=length(n.pos-save.player);if(d<npcDistance) {npcDistance=d;nearestNpc=n.id;}}
        int nearbyHall=-1;for(size_t city=0;city<settlements().size();city++) if(length(save.player-(settlements()[city].pos+Vec{55,55}))<90) nearbyHall=int(city);
        if(ui.npc<0&&!ui.crafting&&!ui.trading&&!ui.civic&&!ui.equipment&&!ui.cartography&&!ui.progression&&!ui.wardrobe&&!ui.journal&&IsKeyPressed(KEY_E)&&nearestNpc>=0) {
          ui.npc=nearestNpc;world.beginConversation(ui.npc,save);ui.input.clear();ui.residentJob=ui.residentWeapon=ui.residentOutfit=0;
          perf.event({{"event","conversation_started"},{"npc",ui.npc},{"city",world.npcs[ui.npc].settlement}});
          if(auto resident=citizen(save,ui.npc)) {
            auto weapons=ownedWeapons(save);auto clothes=ownedOutfits(save);clothes.insert(clothes.begin(),"");
            auto job=std::find(residentJobs().begin(),residentJobs().end(),resident->job);ui.residentJob=int(job-residentJobs().begin());
            auto wi=std::find(weapons.begin(),weapons.end(),resident->weapon);if(wi!=weapons.end()) ui.residentWeapon=int(wi-weapons.begin());
            auto oi=std::find(clothes.begin(),clothes.end(),resident->outfit);if(oi!=clothes.end()) ui.residentOutfit=int(oi-clothes.begin());
          }
          while(GetCharPressed()) {}
        } else if(ui.npc>=0) {
          auto& npc=world.npcs[ui.npc];
          float pw=std::min(720.f,GetScreenWidth()-50.f),py=GetScreenHeight()-500.f-96,px=(GetScreenWidth()-pw)*.5f;
          if(IsKeyPressed(KEY_ONE)||(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(GetMousePosition(),{px+20,py+269,pw-40,32}))) {
            auto found=save.quests.find(npc.id);auto result=found==save.quests.end() ? acceptQuest(save,npc):finishQuest(save,npc);notify(result.message,now);
            perf.event({{"event","quest"},{"npc",npc.id},{"ok",result.ok}});
          }
          if(IsKeyPressed(KEY_F4)) {
            int city=npc.settlement;auto& type=content().towns[settlements()[city].type];
            if(content().npcs[npc.type].id!=type.teacher) notify("Find the town master or open the town hall with P",now);
            else notify(learnTechnology(save,city).message,now);
          }
          if(citizen(save,npc.id)) {
            auto weapons=ownedWeapons(save);auto clothes=ownedOutfits(save);clothes.insert(clothes.begin(),"");
            if(IsKeyPressed(KEY_THREE)) ui.residentJob=(ui.residentJob+1)%int(residentJobs().size());
            if(IsKeyPressed(KEY_FOUR)) ui.residentWeapon=(ui.residentWeapon+1)%int(weapons.size());
            if(IsKeyPressed(KEY_FIVE)) ui.residentOutfit=(ui.residentOutfit+1)%int(clothes.size());
            ui.residentWeapon=std::clamp(ui.residentWeapon,0,int(weapons.size())-1);ui.residentOutfit=std::clamp(ui.residentOutfit,0,int(clothes.size())-1);
            if(IsKeyPressed(KEY_SIX)) notify(assignCitizen(save,npc.id,residentJobs()[ui.residentJob],weapons[ui.residentWeapon],clothes[ui.residentOutfit]).message,now);
          }
        }
        if(ui.npc<0) {
          if(IsKeyPressed(KEY_K)||IsKeyPressed(KEY_O)||IsKeyPressed(KEY_L)) {
            bool progress=IsKeyPressed(KEY_K),clothes=IsKeyPressed(KEY_O);
            ui.progression=progress&&!ui.progression;ui.wardrobe=clothes&&!ui.wardrobe;ui.journal=!progress&&!clothes&&!ui.journal;
            ui.crafting=ui.trading=ui.civic=ui.equipment=ui.cartography=false;ui.selection=0;
          }
          if(IsKeyPressed(KEY_M)) {ui.cartography=!ui.cartography;ui.mapCenter=save.player;ui.crafting=ui.trading=ui.civic=ui.equipment=ui.progression=ui.wardrobe=ui.journal=false;}
          if(IsKeyPressed(KEY_I)) {ui.equipment=!ui.equipment;ui.crafting=ui.trading=ui.civic=ui.cartography=ui.progression=ui.wardrobe=ui.journal=false;ui.selection=0;}
          if(IsKeyPressed(KEY_C)) {ui.crafting=!ui.crafting;ui.trading=ui.civic=ui.equipment=ui.cartography=ui.progression=ui.wardrobe=ui.journal=false;ui.selection=0;}
          if(IsKeyPressed(KEY_P)) {
            if(nearbyHall<0) notify("Approach the town hall entrance — P: enter",now);
            else {ui.civic=!ui.civic;ui.merchant=nearbyHall;ui.crafting=ui.trading=ui.equipment=ui.cartography=ui.progression=ui.wardrobe=ui.journal=false;ui.selection=save.skills.empty()&&save.campaign ? 10:0;}
          }
          if(IsKeyPressed(KEY_T)) {
            if(ui.trading) ui.trading=false;
            else if(nearestNpc>=0) {ui.trading=true;ui.crafting=ui.civic=ui.equipment=ui.cartography=ui.progression=ui.wardrobe=ui.journal=false;ui.merchant=world.npcs[nearestNpc].settlement;ui.selection=0;}
            else notify("Approach a town resident to trade",now);
          }
        }
        if(ui.crafting||ui.trading||ui.civic||ui.equipment||ui.progression||ui.wardrobe||ui.journal) {
          auto weapons=ownedWeapons(save);auto clothes=ownedOutfits(save);std::vector<int> quests;
          for(auto& [id,q]:save.quests) quests.push_back(id);
          std::sort(quests.begin(),quests.end());
          if(quests.empty()) quests.push_back(-1);
          int rows=ui.crafting ? int(recipes().size()):ui.trading ? goodTypes:ui.civic ? int(cityActions().size()):ui.progression ? int(skills().size()+researchNodes().size()):ui.wardrobe ? std::max(1,int(clothes.size())):ui.journal ? int(quests.size()):int(weapons.size());
          int pageSize=Renderer::menuPageRows(),shown=std::min(rows,pageSize),page=(ui.selection/pageSize)*pageSize;
          if(IsKeyPressed(KEY_UP)||IsKeyPressedRepeat(KEY_UP)) ui.selection=(ui.selection+rows-1)%rows;
          if(IsKeyPressed(KEY_DOWN)||IsKeyPressedRepeat(KEY_DOWN)) ui.selection=(ui.selection+1)%rows;
          if(IsKeyPressed(KEY_PAGE_DOWN)) ui.selection=std::min(rows-1,ui.selection+pageSize);
          if(IsKeyPressed(KEY_PAGE_UP)) ui.selection=std::max(0,ui.selection-pageSize);
          ui.selection=std::clamp(ui.selection-int(GetMouseWheelMove()),0,rows-1);page=ui.selection/pageSize*pageSize;
          bool perform=IsKeyPressed(KEY_ENTER),buy=!IsKeyDown(KEY_LEFT_SHIFT);
          if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            auto box=Renderer::menuBounds(shown);auto mouse=GetMousePosition();
            int row=int(std::floor((mouse.y-box.y-Renderer::menuListTop)/Renderer::menuRowHeight));
            if(CheckCollisionPointRec(mouse,box)&&row>=0&&row<shown&&page+row<rows) {
              ui.selection=page+row;
              if(ui.trading) {
                float y=box.y+Renderer::menuListTop+row*Renderer::menuRowHeight;
                bool buyHit=CheckCollisionPointRec(mouse,{box.x+box.width-254,y-2,102,29});
                bool sellHit=CheckCollisionPointRec(mouse,{box.x+box.width-130,y-2,102,29});
                if(buyHit||sellHit) {perform=true;buy=buyHit;}
              }
            }
            if(!ui.trading&&CheckCollisionPointRec(mouse,{box.x+box.width-274,box.y+Renderer::menuListTop+shown*Renderer::menuRowHeight+108,250,40})) perform=true;
          }
          int lesson=ui.progression&&ui.selection<int(skills().size()) ? ui.selection:
            ui.civic&&ui.selection>=10&&ui.selection<10+int(skills().size()) ? ui.selection-10:-1;
          if(perform&&lesson>=0&&lessonState(save,lesson).learned) perform=false;
          if(perform&&ui.civic&&ui.selection==9&&!ownsCity(save,ui.merchant)) {
            for(auto& n:world.npcs) if(n.ruler&&n.settlement==ui.merchant) {ui.npc=n.id;world.beginConversation(n.id,save);ui.civic=false;ui.input.clear();break;}
          } else if(perform) {
            Action result=ui.crafting ? craft(save,ui.selection):ui.trading ? trade(save,ui.merchant,ui.selection,buy):
              ui.civic ? diplomacy(save,ui.merchant,ui.selection):ui.progression ? (ui.selection<int(skills().size()) ? trainSkill(save,ui.selection):research(save,ui.selection-int(skills().size()))):
              ui.wardrobe ? (clothes.empty() ? Action{false,"No clothes yet"}:equipOutfit(save,clothes[ui.selection])):
              ui.journal ? Action{quests[ui.selection]>=0,quests[ui.selection]>=0 ? "Quest tracked":"No quests yet"}:equipWeapon(save,weapons[ui.selection]);
            if(ui.journal&&result.ok) save.trackedQuest=quests[ui.selection];
            notify(result.message,now);
            perf.event({{"event",ui.crafting ? "craft":"trade"},{"item",ui.selection},{"ok",result.ok},{"coins",save.coins}});
          }
        } else if(!ui.blocked()) {
          if(IsKeyPressed(KEY_SPACE)||IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            auto result=game.attack(world,save);if(!result.message.empty()) notify(result.message,now);
          }
          if(IsKeyPressed(KEY_H)) notify(consume(save,true).message,now);
          if(IsKeyPressed(KEY_Q)) notify(consume(save,false).message,now);
          if(IsKeyPressed(KEY_B)) notify(game.placeCampfire(world,save).message,now);
          if(IsKeyPressed(KEY_V)) notify(cycleVehicle(save).message,now);
          if(IsKeyPressed(KEY_R)) notify(dive(save).message,now);
          if(IsKeyPressed(KEY_J)) notify(repairVessel(save).message,now);
          if(IsKeyPressed(KEY_G)) {
            if(game.sonarCooldown>0) notify("Sonar recharging",now);
            else {auto result=sonar(save);notify(result.message,now);if(result.ok) game.sonarCooldown=10;}
          }
        }
        if(options.benchmarkLlm&&!ui.crafting&&!ui.trading&&!ui.civic&&!ui.equipment&&!ui.cartography&&!ui.progression&&!ui.wardrobe&&!ui.journal) {
          if(nearestNpc<0) {ui.npc=-1;world.talkingNpc=-1;}
          if(nearestNpc>=0&&now>2&&now-lastLlm>15&&!dialogue.snapshot().busy) {
            ui.npc=nearestNpc;world.beginConversation(ui.npc,save);
            ask(nearestNpc,"Give a two-sentence greeting based only on your stated role and lore. Do not invent a task or reward.",now);
          }
        }
        if(ui.npc>=0&&length(world.npcs[ui.npc].pos-save.player)>190) {ui.npc=-1;world.talkingNpc=-1;}
        if(save.submerged!=lastLayer) {desiredZoom=save.submerged ? .85f:options.zoom;lastLayer=save.submerged;}
        camera.zoom+=(desiredZoom-camera.zoom)*std::min(1.f,dt*6);
        auto iso=project(save.player);
        camera.target={iso.x,iso.y};camera.offset={GetScreenWidth()*.5f,GetScreenHeight()*.5f};
        // Project viewport corners back onto the ground; keep a margin for tall trees.
        int radius=1;
        for(Vector2 corner:std::array<Vector2,4>{{{-180,-180},{float(GetScreenWidth()+180),-180},
          {-180,float(GetScreenHeight()+180)},{float(GetScreenWidth()+180),float(GetScreenHeight()+180)}}}) {
          auto p=GetScreenToWorld2D(corner,camera);auto d=unproject({p.x,p.y})-save.player;
          radius=std::max(radius,int(std::ceil(std::max(std::abs(d.x),std::abs(d.y))/chunkSize))+1);
        }
        radius=std::min(radius,5);
        Coord current=chunkAt(save.player);
        if(current!=previous||radius!=previousRadius) {world.request(save.player,radius);previous=current;previousRadius=radius;}
        world.integrate();
        if(options.preview=="combat"&&scenarioStep==0&&world.chunks.contains(chunkAt(save.player))) {
          Vec home=save.player+Vec{90,0};world.chunks.at(chunkAt(save.player)).enemies.push_back({hash(0x123abc),home,home,home});
          scenarioStep=1;
        }
        if(!spawnReady) {spawnReady=game.repairSpawn(world,save);if(spawnReady&&ui.npc>=0) world.beginConversation(ui.npc,save);}
        simAccumulator+=dt;
        int ticks=0;
        while(simAccumulator>=.1&&ticks++<3) {
          simulationTime+=.1;world.tick(.1f,save,simulationTime);game.tick(world,save,.1f,simulationTime);simAccumulator-=.1;
        }
        game.combat(world,save,dt);
        if(options.preview=="combat"&&now>.7&&game.attackCooldown<=0) {game.facing={1,0};game.attack(world,save);}
        if(now>=nextExplore) {game.explore(save,float(GetScreenWidth()),float(GetScreenHeight()),camera.zoom);nextExplore=now+.2;}
        if(!game.notice.empty()) {notify(game.notice,now);game.notice.clear();}
        if(options.benchmarkGameplay) {
          if(scenarioStep<8&&now>1+scenarioStep*.35) {
            auto result=craft(save,scenarioStep);perf.event({{"event","benchmark_craft"},{"item",scenarioStep},{"ok",result.ok}});scenarioStep++;
          }
          if(options.benchmarkCommunity&&scenarioStep>=8&&!communityAssigned) {
            bool trained=craft(save,5).ok;trained=research(save,1).ok&&trained;
            trained=assignCitizen(save,save.citizens[1].npc,"guard","wood_sword","leather_armor").ok&&trained;
            trained=equipOutfit(save,"forager").ok&&trained;communityAssigned=true;
            perf.event({{"event","benchmark_resident_training"},{"ok",trained}});
          }
          if(scenarioStep==8&&now>4) {
            auto buy=trade(save,0,0,true),sell=trade(save,0,0,false);
            perf.event({{"event","benchmark_trade"},{"ok",buy.ok&&sell.ok}});scenarioStep++;
          }
          if(scenarioStep==9&&now>4.4) {
            auto result=game.placeCampfire(world,save);
            perf.event({{"event","benchmark_campfire"},{"ok",result.ok}});scenarioStep++;
          }
          if(options.benchmarkExpedition&&scenarioStep==10&&now>4.7) {
            for(int i=0;i<4;i++) craft(save,10);
            bool success=true;
            for(int recipe:{9,11,12,13,14}) {auto result=craft(save,recipe);success=success&&result.ok;}
            save.vehicle=2;scenarioStep++;
            perf.event({{"event","benchmark_vessels"},{"ok",success}});
          }
          if(options.benchmarkExpedition&&scenarioStep==11&&terrainAt(save.player,save.seed)==Terrain::Deep&&now>25) {
            save.vehicle=3;auto result=dive(save);if(result.ok) {scenarioStep++;perf.event({{"event","benchmark_dive"},{"ok",true}});}
          }
          if(options.benchmarkExpedition&&scenarioStep==12&&now>33) {
            auto result=sonar(save);game.sonarCooldown=10;scenarioStep++;
            perf.event({{"event","benchmark_sonar"},{"ok",result.ok}});
          }
          if(options.benchmarkExpedition&&scenarioStep==13&&now>45) {
            if(save.submerged) dive(save);
            save.vehicle=2;scenarioStep++;
            perf.event({{"event","benchmark_surface"},{"ok",!save.submerged}});
          }
          if(now>6&&game.attackCooldown<=0) {
            bool nearby=false;
            for(auto& [coord,c]:world.chunks) for(auto& e:c.enemies) if(!save.defeated.contains(e.id)&&
              (!e.kind||save.relations[e.city].treaty==Treaty::War)&&length(e.pos-save.player)<110) {
              game.facing=normalized(e.pos-save.player);nearby=true;
            }
            if(nearby) {auto result=game.attack(world,save);if(result.ok) pauseUntil=std::max(pauseUntil,now+game.attackDuration+.15);}
          }
          if(save.health<60) consume(save,true);
        }
        resource=world.nearest(save.player,save,90);
        ui.hint.clear();
        if(nearestNpc>=0) ui.hint="E: talk · T: trade with "+world.npcs[nearestNpc].name;
        else if(resource) ui.hint=resource->kind==ResourceKind::Fish ? (save.inventory.rod ? "Hold F: catch fish":"Fishing rod required — C: crafting"):
          "Hold F: gather "+std::string(goodName(resourceGood(resource->kind)));
        if(ui.npc>=0) ui.hint="Talking with "+world.npcs[ui.npc].name+" · ESC: end conversation";
        else if(nearbyHall>=0) ui.hint+=ui.hint.empty() ? "P: enter town hall":" · P: town hall";
        if(ui.npc<0&&!ui.crafting&&!ui.trading&&!ui.civic&&!ui.equipment&&!ui.cartography&&!ui.progression&&!ui.wardrobe&&!ui.journal&&resource&&IsKeyDown(KEY_F)) {
          if(gatheringId!=resource->id) {gathering=0;gatheringId=resource->id;}
          gathering+=dt;
          if(gathering>=harvestDuration(*resource,save.inventory)*gatherTimeMultiplier(save)) {
            notify(harvest(save,*resource).message,now);
            gathering=0;resource=nullptr;
          }
        } else gathering=0;
        ui.harvestProgress=resource ? gathering/(harvestDuration(*resource,save.inventory)*gatherTimeMultiplier(save)):0;
      }
      if(!options.noSave&&!ui.paused&&!quit&&now-lastSave>30) {
        auto t=Clock::now();writeSave(options.savePath,save);lastSave=now;
        perf.event({{"event","save"},{"duration_ms",msSince(t)},{"harvested",save.harvested.size()}});
      }
      auto state=dialogue.snapshot();ui.busy=state.busy;ui.online=state.online;
      ui.reply=ui.npc>=0 ? (state.busy&&state.npcId==ui.npc ? state.text : replies[ui.npc]) : "";
      // Busy label applies to the current speaker only; another request can keep running after closing.
      ui.busy=state.busy&&state.npcId==ui.npc;
      ui.wood=save.inventory.wood;ui.stone=save.inventory.stone;ui.berries=save.inventory.berries;
      ui.fps=perf.fps;ui.p95=perf.p95;ui.p99=perf.p99;ui.workMs=perf.workMs;
      ui.chunks=world.chunks.size();ui.pending=world.pending();ui.time=now;
      ui.animals=0;for(auto& [coord,c]:world.chunks) for(auto& a:c.animals) if(length(a.pos-save.player)<1100) ui.animals++;
      double updateMs=msSince(updateStart);
      auto renderStart=Clock::now();
      BeginDrawing();ClearBackground(save.submerged ? Color{12,28,39,255}:Color{41,58,42,255});BeginMode2D(camera);
      ui.visible=renderer.drawWorld(world,save,game,camera,now,float(simAccumulator/.1),resource);
      EndMode2D();renderer.drawLabels(world,save,ui,camera);renderer.drawUi(world,save,ui,camera);
      double submitMs=msSince(renderStart);
      auto presentStart=Clock::now();EndDrawing();double presentMs=msSince(presentStart);
      auto presented=Clock::now();
      perf.frame(std::chrono::duration<double,std::milli>(presented-lastPresentation).count(),updateMs,submitMs,presentMs,state.busy);
      lastPresentation=presented;
      if(now-lastFlush>=5) {perf.flush(context());lastFlush=now;}
      frameCount++;
      if((options.frames>0&&frameCount>=options.frames)||(options.benchmarkSeconds>0&&now>=options.benchmarkSeconds)) quit=true;
    }
    if(!options.screenshot.empty()) {
      if(!options.screenshot.parent_path().empty()) std::filesystem::create_directories(options.screenshot.parent_path());
      Image screenshot{rlReadScreenPixels(GetRenderWidth(),GetRenderHeight()),GetRenderWidth(),GetRenderHeight(),1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
      if(!screenshot.data) throw std::runtime_error("Cannot read screenshot pixels");
      bool exported=ExportImage(screenshot,options.screenshot.c_str());UnloadImage(screenshot);
      if(!exported) throw std::runtime_error("Cannot export screenshot");
    }
    if(!options.noSave&&!savedForExit) writeSave(options.savePath,save);
    perf.flush(context());
    auto finalReply=dialogue.snapshot();
    if(finalReply.busy) perf.event({{"event","llm_cancelled"},{"npc_id",finalReply.npcId},{"reason","session_exit"}});
    perf.event({{"event","session_end"},{"total_frames",frameCount}});
    std::cout<<"Frames: "<<frameCount<<". Last window FPS: "<<perf.fps<<", p95: "<<perf.p95<<" ms\n";
  }
  return 0;
}
int main(int argc,char** argv) {
  try {
    auto options=parse(argc,argv);
    const bool menuPreview=options.preview=="menu"||options.preview=="newgame"||options.preview=="saves"||options.preview=="settings";
    const bool useMenu=menuPreview||(options.frames==0&&options.benchmarkSeconds==0&&options.preview.empty());
    if(!useMenu) {int result=play(options);CloseWindow();return result;}
    const auto settingsPath=options.settingsPath;
    auto settings=loadSettings(settingsPath);
    settings.fullscreen=options.fullscreen;
    if(options.width!=1600||options.height!=900) {settings.width=options.width;settings.height=options.height;}
    if(options.portExplicit) settings.port=options.port;
    if(options.uncapped) settings.fps=0;
    SetTraceLogLevel(LOG_INFO);SetTraceLogCallback(startupTrace);SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(settings.width,settings.height,"Forest Frontier");SetTraceLogLevel(LOG_WARNING);SetTraceLogCallback(nullptr);
    if(!IsWindowReady()) throw std::runtime_error("Cannot create window");
    SetWindowMinSize(900,650);SetExitKey(KEY_NULL);if(settings.fullscreen) toggleFullscreen();
    MenuOptions menu;menu.directory=options.savePath.parent_path();if(menu.directory.empty()) menu.directory=".";
    menu.preferred=options.saveExplicit ? options.savePath:std::filesystem::path(settings.lastSave);menu.settingsPath=settingsPath;menu.settings=settings;
    menu.preview=menuPreview ? options.preview:"";menu.frames=menuPreview ? options.frames:0;menu.screenshot=options.screenshot;
    bool directStart=(options.direct||options.noSave||options.spawn.has_value())&&!menuPreview;
    while(!WindowShouldClose()) {
      bool firstDirectSession=directStart;directStart=false;
      auto slot=firstDirectSession ? std::optional<std::filesystem::path>(options.savePath):mainMenu(menu);if(!slot) break;
      menu.settings.lastSave=std::filesystem::absolute(*slot).string();writeSettings(settingsPath,menu.settings);
      auto gameOptions=options;gameOptions.savePath=*slot;gameOptions.preview.clear();gameOptions.frames=0;
      if(!firstDirectSession) gameOptions.screenshot.clear();
      if(!firstDirectSession) {
        gameOptions.noSave=false;gameOptions.spawn.reset();
        gameOptions.width=menu.settings.width;gameOptions.height=menu.settings.height;gameOptions.fullscreen=menu.settings.fullscreen;
        gameOptions.fps=menu.settings.fps;gameOptions.uncapped=false;gameOptions.port=menu.settings.port;
        gameOptions.zoom=menu.settings.zoom;gameOptions.debug=menu.settings.debug;gameOptions.dialogues=menu.settings.dialogues;
      }
      play(gameOptions);menu.preferred=*slot;
      menu.settings.fullscreen=IsWindowFullscreen();
    }
    CloseWindow();return 0;
  } catch(const std::exception& e) {std::cerr<<"Forest Frontier: "<<e.what()<<'\n';if(IsWindowReady()) CloseWindow();return 1;}
}
