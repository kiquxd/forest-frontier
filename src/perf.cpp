#include "perf.hpp"
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <numeric>
#include <unistd.h>

namespace frontier {
static double percentile(std::vector<double> values,double q) {
  if(values.empty()) return 0;
  std::sort(values.begin(),values.end());
  return values[std::min(values.size()-1,size_t(std::ceil(q*values.size())-1))];
}
static double round3(double n) {return std::round(n*1000)/1000;}
Perf::Perf(const std::filesystem::path& dir) {
  std::filesystem::create_directories(dir);
  auto now=std::time(nullptr); std::tm tm{}; localtime_r(&now,&tm);
  std::ostringstream s; s<<"perf-"<<std::put_time(&tm,"%Y%m%d-%H%M%S")<<'-'<<getpid()<<".jsonl";
  path_=dir/s.str(); file_.open(path_);
  if(!file_) throw std::runtime_error("Cannot create performance log");
  samples_.reserve(8192);
}
void Perf::event(nlohmann::json j) {
  j["elapsed_s"]=round3(msSince(start_)/1000);
  file_<<j.dump()<<'\n'; file_.flush();
}
void Perf::frame(double f,double u,double s,double p,bool busy) {samples_.push_back({f,u,s,p,busy});}
void Perf::flush(nlohmann::json context) {
  if(samples_.empty()) return;
  std::vector<double> frames,busyFrames,idleFrames; frames.reserve(samples_.size());
  double total=0,update=0,submit=0,present=0; int slow=0;
  for(auto& s:samples_) {
    frames.push_back(s.frame); (s.llm ? busyFrames : idleFrames).push_back(s.frame);
    total+=s.frame; update+=s.update; submit+=s.submit; present+=s.present;
    if(s.frame>20) slow++;
  }
  double count=double(samples_.size());
  fps=1000*count/std::max(total,.001); p95=percentile(frames,.95); p99=percentile(frames,.99); workMs=(update+submit)/count;
  context["event"]="perf"; context["frames"]=samples_.size(); context["fps"]=round3(fps);
  context["frame_ms"]={{"mean",round3(total/count)},{"p95",round3(p95)},{"p99",round3(p99)},
    {"max",round3(*std::max_element(frames.begin(),frames.end()))}};
  context["update_ms_mean"]=round3(update/count); context["render_submit_ms_mean"]=round3(submit/count);
  context["present_wait_ms_mean"]=round3(present/count); context["frames_over_20ms"]=slow;
  context["llm_busy_frames"]=busyFrames.size();
  context["llm_busy_p95_ms"]=busyFrames.empty() ? nlohmann::json(nullptr) : nlohmann::json(round3(percentile(busyFrames,.95)));
  context["llm_idle_p95_ms"]=idleFrames.empty() ? nlohmann::json(nullptr) : nlohmann::json(round3(percentile(idleFrames,.95)));
  std::ifstream mem("/proc/self/statm"); long pages=0,resident=0;
  if(mem>>pages>>resident) context["game_rss_mb"]=round3(double(resident)*sysconf(_SC_PAGESIZE)/(1024*1024));
  event(std::move(context)); samples_.clear();
}
}
