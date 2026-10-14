#pragma once
#include "json.hpp"
#include <memory>
#include <string>
#include <vector>

namespace frontier {
struct Message {std::string role, text;};
struct Reply {
  int npcId{-1};
  bool busy{}, done{}, online{};
  std::string text, error;
  double totalMs{}, firstTokenMs{-1}, promptMs{}, generationMs{};
  int promptTokens{}, outputTokens{};
};
class Dialogue {
public:
  explicit Dialogue(int port=8080,bool enabled=true);
  ~Dialogue();
  bool ask(int npcId,const std::string& system,const std::vector<Message>& messages);
  Reply snapshot() const;
  // Consume completion exactly once; it can belong to a closed dialogue panel.
  bool takeCompletion(Reply& out);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
