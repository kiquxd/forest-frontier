#include "dialogue.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <ratio>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "httplib.h"
#include "json.hpp"

namespace frontier {
using Timer = std::chrono::steady_clock;
namespace {
double Elapsed(Timer::time_point t) {
  return std::chrono::duration<double, std::milli>(Timer::now() - t).count();
}
}  // namespace

struct Dialogue::Impl {
  mutable std::mutex mutex;
  std::condition_variable wake;
  httplib::Client client;
  Reply reply;
  nlohmann::json body;
  bool stopping{};
  bool queued{};
  bool completed{};
  bool enabled{};
  std::thread worker;
  explicit Impl(int port, bool active)
      : client("127.0.0.1", port), enabled(active) {
    client.set_connection_timeout(2);
    client.set_read_timeout(20);
    client.set_write_timeout(5);
    client.set_max_timeout(std::chrono::seconds(90));
    client.set_follow_location(false);
    if (enabled) {
      worker = std::thread([this] { Run(); });
    }
  }
  ~Impl() {
    {
      std::lock_guard lock(mutex);
      stopping = true;
    }
    wake.notify_all();
    client.stop();
    if (worker.joinable()) {
      worker.join();
    }
  }
  void Run() {
    // Health check is also off the render thread. A missing server never
    // prevents play.
    auto health = client.Get("/health");
    {
      std::lock_guard lock(mutex);
      reply.online = health && health->status == 200;
    }
    for (;;) {
      nlohmann::json request_body;
      {
        std::unique_lock lock(mutex);
        wake.wait(lock, [&] { return stopping || queued; });
        if (stopping) {
          return;
        }
        request_body = std::move(body);
        queued = false;
      }
      const auto start = Timer::now();
      std::string buffer;
      bool done = false, malformed = false;
      int http_status = 0;
      httplib::Request req;
      req.method = "POST";
      req.path = "/v1/chat/completions";
      req.set_header("Content-Type", "application/json");
      req.body = request_body.dump();
      req.response_handler = [&](const httplib::Response& response) {
        http_status = response.status;
        return true;
      };
      req.content_receiver = [&](const char* data, size_t size, uint64_t,
                                 uint64_t) {
        {
          std::lock_guard lock(mutex);
          if (stopping) {
            return false;
          }
        }
        if (http_status != 200) {
          return true;
        }
        buffer.append(data, size);
        if (buffer.size() > 65536) {
          malformed = true;
          return false;
        }
        size_t end;
        while ((end = buffer.find('\n')) != std::string::npos) {
          std::string line = buffer.substr(0, end);
          buffer.erase(0, end + 1);
          if (!line.empty() && line.back() == '\r') {
            line.pop_back();
          }
          if (!line.starts_with("data:")) {
            continue;
          }
          auto payload = line.substr(5);
          auto first = payload.find_first_not_of(' ');
          if (first == std::string::npos) {
            continue;
          }
          payload.erase(0, first);
          if (payload == "[DONE]") {
            done = true;
            continue;
          }
          try {
            auto event = nlohmann::json::parse(payload);
            if (event.contains("error")) {
              malformed = true;
              return false;
            }
            std::lock_guard lock(mutex);
            if (event.contains("choices") && !event["choices"].empty()) {
              auto& choice = event["choices"][0];
              if (choice.contains("delta") &&
                  choice["delta"].contains("content") &&
                  choice["delta"]["content"].is_string()) {
                auto content = choice["delta"]["content"].get<std::string>();
                if (!content.empty() && reply.first_token_ms < 0) {
                  reply.first_token_ms = Elapsed(start);
                }
                reply.text += content;
                if (reply.text.size() > 16384) {
                  malformed = true;
                  return false;
                }
              }
            }
            if (event.contains("usage") && event["usage"].is_object()) {
              reply.prompt_tokens = event["usage"].value("prompt_tokens", 0);
              reply.output_tokens =
                  event["usage"].value("completion_tokens", 0);
            }
            if (event.contains("timings") && event["timings"].is_object()) {
              reply.prompt_ms = event["timings"].value("prompt_ms", 0.0);
              reply.generation_ms = event["timings"].value("predicted_ms", 0.0);
            }
          } catch (const std::exception&) {
            malformed = true;
            return false;
          }
        }
        return true;
      };
      auto result = client.send(req);
      {
        std::lock_guard lock(mutex);
        reply.total_ms = Elapsed(start);
        reply.busy = false;
        reply.done = true;
        completed = true;
        reply.online = result && http_status == 200;
        if (!result) {
          reply.error =
              malformed ? "invalid_stream" : httplib::to_string(result.error());
        } else if (http_status != 200) {
          reply.error = "http_" + std::to_string(http_status);
        } else if (malformed || !done || reply.text.empty()) {
          reply.error = "incomplete_stream";
        }
      }
    }
  }
};
Dialogue::Dialogue(int port, bool enabled)
    : impl_(std::make_unique<Impl>(port, enabled)) {
}
Dialogue::~Dialogue() = default;
bool Dialogue::Ask(int npc_id, const std::string& system,
                   const std::vector<Message>& messages) {
  std::lock_guard lock(impl_->mutex);
  if (!impl_->enabled || impl_->reply.busy || impl_->completed) {
    return false;
  }
  bool online = impl_->reply.online;
  impl_->reply = {};
  impl_->reply.npc_id = npc_id;
  impl_->reply.busy = true;
  impl_->reply.online = online;
  nlohmann::json list = nlohmann::json::array();
  list.push_back({{"role", "system"}, {"content", system}});
  for (auto& message : messages) {
    list.push_back({{"role", message.role}, {"content", message.text}});
  }
  impl_->body = {{"model", "forest-npc"},
                 {"messages", list},
                 {"stream", true},
                 {"stream_options", {{"include_usage", true}}},
                 {"max_tokens", 128},
                 {"temperature", .7},
                 {"top_p", .8},
                 {"top_k", 20},
                 {"min_p", 0.0},
                 {"presence_penalty", 1.5},
                 {"chat_template_kwargs", {{"enable_thinking", false}}}};
  impl_->queued = true;
  impl_->wake.notify_one();
  return true;
}
Reply Dialogue::Snapshot() const {
  std::lock_guard lock(impl_->mutex);
  return impl_->reply;
}
bool Dialogue::TakeCompletion(Reply& out) {
  std::lock_guard lock(impl_->mutex);
  if (!impl_->completed) {
    return false;
  }
  out = impl_->reply;
  impl_->completed = false;
  return true;
}
}  // namespace frontier
