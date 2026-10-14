#include "save_slots.hpp"
#include "json.hpp"
#include <fstream>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace frontier;
static void check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
int main() {
  auto root=std::filesystem::temp_directory_path()/("frontier-slots-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    auto directory=root/"saves";check(scanSaveSlots(directory).empty(),"Missing save directory is empty");
    auto first=createSaveSlot(directory,"My world",123),second=createSaveSlot(directory,"My world",456);
    check(first!=second&&std::filesystem::exists(first)&&std::filesystem::exists(second),"Named worlds never overwrite existing slots");
    auto save=loadSave(first);configureWorld(save.seed);save.skills.insert("gathering");save.inventory.wood=17;save.explored.insert(mapCell(save.player));
    int foreign=(save.homeCity+1)%int(save.relations.size());save.relations[foreign].discovered=true;writeSave(first,save);
    {std::ofstream out(directory/"broken.json");out<<"not json";}
    {std::ofstream out(directory/"ignored.json.tmp");out<<"temporary";}
    std::filesystem::create_symlink(first,directory/"linked.json");
    auto slots=scanSaveSlots(directory);check(slots.size()==3,"Only direct JSON slots are listed; temporary files and symlinks ignored");
    bool summary=false,invalid=false;for(auto& slot:slots) {
      if(slot.path==first) summary=slot.save&&slot.discovered==2&&slot.owned==1&&slot.save->inventory.wood==17&&slot.save->skills.contains("gathering")&&!slot.save->explored.empty()&&slot.towns.size()==slot.save->relations.size();
      if(slot.path.filename()=="broken.json") invalid=!slot.save&&!slot.error.empty();
    }
    check(summary&&invalid,"Summary preserves explored towns, inventory, skills and map; corrupt saves do not break browser");
    Settings settings;settings.fps=144;settings.zoom=1.35f;settings.dialogues=false;settings.port=8090;settings.lastSave=first.string();
    writeSettings(root/"settings.json",settings);auto loaded=loadSettings(root/"settings.json");
    check(loaded.fps==144&&loaded.zoom==1.35f&&!loaded.dialogues&&loaded.port==8090&&loaded.lastSave==first.string(),"Settings and last played slot persist");
    {std::ofstream out(root/"settings.json");out<<"{\"port\":0}";}
    check(loadSettings(root/"settings.json").port==8080,"Invalid settings recover to safe defaults");
    auto outside=createSaveSlot(root/"outside","Keep me",789);bool rejected=false;
    try {deleteSaveSlot(directory,outside);} catch(const std::exception&) {rejected=true;}
    check(rejected&&std::filesystem::exists(outside),"Deletion cannot target a file outside the slot directory");
    deleteSaveSlot(directory,first);check(!std::filesystem::exists(first)&&scanSaveSlots(directory).size()==2,"Deleted world disappears from browser");
    auto trash=std::filesystem::directory_iterator(directory/".trash");check(trash!=std::filesystem::directory_iterator{}&&loadSave(trash->path()).inventory.wood==17,"Deleted world remains recoverable with progress intact");
    // Content shown to the model and player is now English, while IDs stay unchanged.
    auto ascii=[](const std::string& s){for(unsigned char c:s) if(c>=128) return false;return true;};
    for(auto& npc:content().npcs) {check(ascii(npc.role)&&ascii(npc.personality),"English NPC context");for(auto& name:npc.names) check(ascii(name),"English NPC names");}
    for(auto& recipe:recipes()) check(ascii(recipe.name)&&ascii(recipe.effect),"English recipe text");
    for(auto& town:content().towns) check(ascii(town.name),"English settlement types");
    std::filesystem::remove_all(root);std::cout<<"slot browser, summary, settings, deletion recovery and English content passed\n";
  } catch(const std::exception& e) {std::filesystem::remove_all(root);std::cerr<<e.what()<<'\n';return 1;}
}
