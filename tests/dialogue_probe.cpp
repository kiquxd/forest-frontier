#include "dialogue.hpp"
#include <chrono>
#include <thread>
#include <iostream>
using namespace frontier;
int main(int argc,char** argv) {
  if(argc!=2) return 2;
  Dialogue d(std::stoi(argv[1]));
  if(!d.ask(3,"Ты лесничий.",{{"user","Привет!"}})) return 3;
  if(d.ask(4,"another",{{"user","second"}})) return 4;
  auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(8);
  Reply r;
  while(std::chrono::steady_clock::now()<deadline) {
    if(d.takeCompletion(r)) {
      nlohmann::json output={{"text",r.text},{"error",r.error},{"npc",r.npcId},{"first_token_ms",r.firstTokenMs},
        {"total_ms",r.totalMs},{"tokens",r.outputTokens},{"online",r.online}};
      std::cout<<output.dump()<<'\n';return 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  } return 5;
}
