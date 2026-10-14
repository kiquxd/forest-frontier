#include "save_slots.hpp"
#include "adventure.hpp"
#include "json.hpp"
#include <algorithm>
#include <fstream>
#include <chrono>
#include <cctype>
#include <cmath>

namespace frontier {
static void validate(const Settings& s) {
  if(s.width<900||s.width>3840||s.height<650||s.height>2160||s.fps<0||s.fps>240||s.port<1||s.port>65535||!std::isfinite(s.zoom)||s.zoom<.65f||s.zoom>1.7f)
    throw std::runtime_error("Invalid settings");
}
Settings loadSettings(const std::filesystem::path& path) {
  Settings s;if(!std::filesystem::exists(path)) return s;
  try {
    std::ifstream in(path);nlohmann::json j;in>>j;
    s.width=j.value("width",s.width);s.height=j.value("height",s.height);s.fps=j.value("fps",s.fps);
    s.port=j.value("port",s.port);s.zoom=j.value("zoom",s.zoom);s.fullscreen=j.value("fullscreen",s.fullscreen);
    s.debug=j.value("debug",s.debug);s.dialogues=j.value("dialogues",s.dialogues);s.lastSave=j.value("last_save",std::string{});validate(s);
  } catch(const std::exception&) {return Settings{};}
  return s;
}
void writeSettings(const std::filesystem::path& path,const Settings& s) {
  validate(s);if(!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
  nlohmann::json j={{"width",s.width},{"height",s.height},{"fps",s.fps},{"port",s.port},{"zoom",s.zoom},{"fullscreen",s.fullscreen},{"debug",s.debug},{"dialogues",s.dialogues},{"last_save",s.lastSave}};
  auto tmp=path;tmp+=".tmp";{std::ofstream out(tmp);if(!out) throw std::runtime_error("Cannot write settings");out<<j.dump(2)<<'\n';out.flush();if(!out) throw std::runtime_error("Cannot write settings");}
  std::filesystem::rename(tmp,path);
}
std::vector<SaveSlot> scanSaveSlots(const std::filesystem::path& directory) {
  std::vector<SaveSlot> slots;if(!std::filesystem::exists(directory)) return slots;
  for(auto& entry:std::filesystem::directory_iterator(directory)) {
    if(entry.is_symlink()||!entry.is_regular_file()||entry.path().extension()!=".json") continue;
    SaveSlot slot;slot.path=entry.path();slot.modified=entry.last_write_time();
    try {
      auto save=loadSave(slot.path);configureWorld(save.seed);initializeCampaign(save,true);initializeAdventure(save);
      slot.towns=settlements();for(auto& r:save.relations) if(r.discovered) slot.discovered++;
      slot.owned=int(std::count(save.owners.begin(),save.owners.end(),0));slot.save=std::move(save);
    } catch(const std::exception& e) {slot.error=e.what();}
    slots.push_back(std::move(slot));
  }
  std::sort(slots.begin(),slots.end(),[](const auto& a,const auto& b){return a.modified==b.modified ? a.path.filename()<b.path.filename():a.modified>b.modified;});
  return slots;
}
std::filesystem::path createSaveSlot(const std::filesystem::path& directory,const std::string& name,uint64_t seed,const std::string& background) {
  std::string slug;
  for(unsigned char c:name) if(std::isalnum(c)||c=='-'||c=='_') slug+=char(c);else if(c==' ') slug+='-';
  if(slug.empty()) slug="new-world";
  if(slug.size()>48) slug.resize(48);
  std::filesystem::create_directories(directory);auto path=directory/(slug+".json");int suffix=2;
  while(std::filesystem::exists(path)) path=directory/(slug+"-"+std::to_string(suffix++)+".json");
  Save save;save.seed=seed;configureWorld(seed);initializeCampaign(save);save.characterId=character(background).id;save.coins+=character(background).coins;initializeAdventure(save);writeSave(path,save);return path;
}
void deleteSaveSlot(const std::filesystem::path& directory,const std::filesystem::path& path) {
  if(std::filesystem::is_symlink(path)||path.extension()!=".json"||std::filesystem::weakly_canonical(path.parent_path())!=std::filesystem::weakly_canonical(directory))
    throw std::runtime_error("Invalid save slot path");
  auto trash=directory/".trash";std::filesystem::create_directories(trash);
  auto destination=trash/(path.stem().string()+"-"+std::to_string(std::chrono::system_clock::now().time_since_epoch().count())+".json");
  std::filesystem::rename(path,destination);
}
}
