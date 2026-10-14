#pragma once
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <chrono>

namespace frontier {
using Clock=std::chrono::steady_clock;
inline double msSince(Clock::time_point t) {return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
class Perf {
public:
  explicit Perf(const std::filesystem::path& dir);
  void event(nlohmann::json j);
  void frame(double frameMs,double updateMs,double submitMs,double presentMs,bool llmBusy);
  void flush(nlohmann::json context = {});
  double p95{}, p99{}, fps{}, workMs{};
  const std::filesystem::path& path() const { return path_; }
private:
  struct Sample {double frame,update,submit,present; bool llm;};
  std::vector<Sample> samples_;
  std::filesystem::path path_;
  std::ofstream file_;
  Clock::time_point start_=Clock::now();
};
}
