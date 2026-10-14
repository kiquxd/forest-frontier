#ifndef FOREST_FRONTIER_SRC_PERF_HPP_
#define FOREST_FRONTIER_SRC_PERF_HPP_

#include <chrono>
#include <filesystem>
#include <fstream>
#include <ratio>
#include <vector>

#include "json.hpp"

namespace frontier {
using Clock = std::chrono::steady_clock;
inline double ElapsedMilliseconds(Clock::time_point t) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}
class Perf {
 public:
  explicit Perf(const std::filesystem::path& dir);
  void Event(nlohmann::json json);
  void Frame(double frame_ms, double update_ms, double submit_ms,
             double present_ms, bool llm_busy);
  void Flush(nlohmann::json context = {});
  double p95{};
  double p99{};
  double fps{};
  double work_ms{};
  const std::filesystem::path& Path() const {
    return path_;
  }

 private:
  struct Sample {
    double frame;
    double update;
    double submit;
    double present;
    bool llm;
  };
  std::vector<Sample> samples_;
  std::filesystem::path path_;
  std::ofstream file_;
  Clock::time_point start_ = Clock::now();
};
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_PERF_HPP_
