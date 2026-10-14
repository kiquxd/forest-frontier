#ifndef FOREST_FRONTIER_SRC_DIALOGUE_HPP_
#define FOREST_FRONTIER_SRC_DIALOGUE_HPP_

#include <memory>
#include <string>
#include <vector>

namespace frontier {
struct Message {
  std::string role;
  std::string text;
};
struct Reply {
  int npc_id{-1};
  bool busy{};
  bool done{};
  bool online{};
  std::string text;
  std::string error;
  double total_ms{};
  double first_token_ms{-1};
  double prompt_ms{};
  double generation_ms{};
  int prompt_tokens{};
  int output_tokens{};
};
class Dialogue {
 public:
  explicit Dialogue(int port = 8080, bool enabled = true);
  ~Dialogue();
  bool Ask(int npc_id, const std::string& system,
           const std::vector<Message>& messages);
  Reply Snapshot() const;
  // Consume completion exactly once; it can belong to a closed dialogue panel.
  bool TakeCompletion(Reply& out);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_DIALOGUE_HPP_
