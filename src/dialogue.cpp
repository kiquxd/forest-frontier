#include "dialogue.hpp"
#include "httplib.h"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace frontier {
using Timer=std::chrono::steady_clock;
static double elapsed(Timer::time_point t) {return std::chrono::duration<double,std::milli>(Timer::now()-t).count();}
struct Dialogue::Impl {
  mutable std::mutex mutex;
  std::condition_variable wake;
  httplib::Client client;
  Reply reply;
  nlohmann::json body;
  bool stopping{}, queued{}, completed{},enabled{};
  std::thread worker;
  explicit Impl(int port,bool active):client("127.0.0.1",port),enabled(active) {
    client.set_connection_timeout(2);
    client.set_read_timeout(20);
    client.set_write_timeout(5);
    client.set_max_timeout(std::chrono::seconds(90));
    client.set_follow_location(false);
    if(enabled) worker=std::thread([this]{run();});
  }
  ~Impl() {
    {std::lock_guard lock(mutex); stopping=true;}
    wake.notify_all(); client.stop(); if(worker.joinable()) worker.join();
  }
  void run() {
    // Health check is also off the render thread. A missing server never prevents play.
    auto health=client.Get("/health");
    {std::lock_guard lock(mutex); reply.online=health && health->status==200;}
    for(;;) {
      nlohmann::json requestBody;
      {std::unique_lock lock(mutex); wake.wait(lock,[&]{return stopping||queued;});
        if(stopping) return;
        requestBody=std::move(body); queued=false;}
      const auto start=Timer::now();
      std::string buffer; bool done=false,malformed=false; int httpStatus=0;
      httplib::Request req; req.method="POST"; req.path="/v1/chat/completions";
      req.set_header("Content-Type","application/json"); req.body=requestBody.dump();
      req.response_handler=[&](const httplib::Response& response) {httpStatus=response.status; return true;};
      req.content_receiver=[&](const char* data,size_t size,uint64_t,uint64_t) {
        {std::lock_guard lock(mutex); if(stopping) return false;}
        if(httpStatus!=200) return true;
        buffer.append(data,size);
        if(buffer.size()>65536) {malformed=true; return false;}
        size_t end;
        while((end=buffer.find('\n'))!=std::string::npos) {
          std::string line=buffer.substr(0,end); buffer.erase(0,end+1);
          if(!line.empty()&&line.back()=='\r') line.pop_back();
          if(!line.starts_with("data:")) continue;
          auto payload=line.substr(5); auto first=payload.find_first_not_of(' ');
          if(first==std::string::npos) continue;
          payload.erase(0,first);
          if(payload=="[DONE]") {done=true; continue;}
          try {
            auto event=nlohmann::json::parse(payload);
            if(event.contains("error")) {malformed=true; return false;}
            std::lock_guard lock(mutex);
            if(event.contains("choices")&&!event["choices"].empty()) {
              auto& choice=event["choices"][0];
              if(choice.contains("delta")&&choice["delta"].contains("content")&&choice["delta"]["content"].is_string()) {
                auto content=choice["delta"]["content"].get<std::string>();
                if(!content.empty()&&reply.firstTokenMs<0) reply.firstTokenMs=elapsed(start);
                reply.text+=content;
                if(reply.text.size()>16384) {malformed=true; return false;}
              }
            }
            if(event.contains("usage")&&event["usage"].is_object()) {
              reply.promptTokens=event["usage"].value("prompt_tokens",0);
              reply.outputTokens=event["usage"].value("completion_tokens",0);
            }
            if(event.contains("timings")&&event["timings"].is_object()) {
              reply.promptMs=event["timings"].value("prompt_ms",0.0);
              reply.generationMs=event["timings"].value("predicted_ms",0.0);
            }
          } catch(const std::exception&) {malformed=true; return false;}
        }
        return true;
      };
      auto result=client.send(req);
      {std::lock_guard lock(mutex);
        reply.totalMs=elapsed(start); reply.busy=false; reply.done=true; completed=true;
        reply.online=result && httpStatus==200;
        if(!result) reply.error=malformed ? "invalid_stream" : httplib::to_string(result.error());
        else if(httpStatus!=200) reply.error="http_"+std::to_string(httpStatus);
        else if(malformed||!done||reply.text.empty()) reply.error="incomplete_stream";
      }
    }
  }
};
Dialogue::Dialogue(int port,bool enabled):impl_(std::make_unique<Impl>(port,enabled)) {}
Dialogue::~Dialogue()=default;
bool Dialogue::ask(int npcId,const std::string& system,const std::vector<Message>& messages) {
  std::lock_guard lock(impl_->mutex);
  if(!impl_->enabled||impl_->reply.busy||impl_->completed) return false;
  bool online=impl_->reply.online;
  impl_->reply={}; impl_->reply.npcId=npcId; impl_->reply.busy=true; impl_->reply.online=online;
  nlohmann::json list=nlohmann::json::array(); list.push_back({{"role","system"},{"content",system}});
  for(auto& m:messages) list.push_back({{"role",m.role},{"content",m.text}});
  impl_->body={{"model","forest-npc"},{"messages",list},{"stream",true},{"stream_options",{{"include_usage",true}}},
    {"max_tokens",128},{"temperature",.7},{"top_p",.8},{"top_k",20},{"min_p",0.0},
    {"presence_penalty",1.5},{"chat_template_kwargs",{{"enable_thinking",false}}}};
  impl_->queued=true; impl_->wake.notify_one(); return true;
}
Reply Dialogue::snapshot() const {std::lock_guard lock(impl_->mutex); return impl_->reply;}
bool Dialogue::takeCompletion(Reply& out) {
  std::lock_guard lock(impl_->mutex);
  if(!impl_->completed) return false;
  out=impl_->reply; impl_->completed=false; return true;
}
}
