#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "dialogue.hpp"
#include "json.hpp"
int main(int argc, char** argv) {
  if (argc != 2) {
    return 2;
  }
  frontier::Dialogue dialogue(std::stoi(argv[1]));
  if (!dialogue.Ask(3, "Ты лесничий.", {{"user", "Привет!"}})) {
    return 3;
  }
  if (dialogue.Ask(4, "another", {{"user", "second"}})) {
    return 4;
  }
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
  frontier::Reply reply;
  while (std::chrono::steady_clock::now() < deadline) {
    if (dialogue.TakeCompletion(reply)) {
      nlohmann::json output = {{"text", reply.text},
                               {"error", reply.error},
                               {"npc", reply.npc_id},
                               {"first_token_ms", reply.first_token_ms},
                               {"total_ms", reply.total_ms},
                               {"tokens", reply.output_tokens},
                               {"online", reply.online}};
      std::cout << output.dump() << '\n';
      return 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return 5;
}
