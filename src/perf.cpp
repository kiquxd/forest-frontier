#include "perf.hpp"

#include <unistd.h>

#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "json.hpp"

namespace frontier {
namespace {
double Percentile(std::vector<double> values, double q) {
  if (values.empty()) {
    return 0;
  }
  std::sort(values.begin(), values.end());
  return values[std::min(
      values.size() - 1,
      static_cast<size_t>(std::ceil(q * values.size()) - 1))];
}
double Round3(double n) {
  return std::round(n * 1000) / 1000;
}
}  // namespace

Perf::Perf(const std::filesystem::path& dir) {
  std::filesystem::create_directories(dir);
  auto now = std::time(nullptr);
  std::tm tm{};
  localtime_r(&now, &tm);
  std::ostringstream s;
  s << "perf-" << std::put_time(&tm, "%Y%m%d-%H%M%S") << '-' << getpid()
    << ".jsonl";
  path_ = dir / s.str();
  file_.open(path_);
  if (!file_) {
    throw std::runtime_error("Cannot create performance log");
  }
  samples_.reserve(8192);
}
void Perf::Event(nlohmann::json json) {
  json["elapsed_s"] = Round3(ElapsedMilliseconds(start_) / 1000);
  file_ << json.dump() << '\n';
  file_.flush();
}
void Perf::Frame(double f, double u, double s, double p, bool busy) {
  samples_.push_back({f, u, s, p, busy});
}
void Perf::Flush(nlohmann::json context) {
  if (samples_.empty()) {
    return;
  }
  std::vector<double> frames, busy_frames, idle_frames;
  frames.reserve(samples_.size());
  double total = 0, update = 0, submit = 0, present = 0;
  int slow = 0;
  for (auto& s : samples_) {
    frames.push_back(s.frame);
    (s.llm ? busy_frames : idle_frames).push_back(s.frame);
    total += s.frame;
    update += s.update;
    submit += s.submit;
    present += s.present;
    if (s.frame > 20) {
      slow++;
    }
  }
  double count = static_cast<double>(samples_.size());
  fps = 1000 * count / std::max(total, .001);
  p95 = Percentile(frames, .95);
  p99 = Percentile(frames, .99);
  work_ms = (update + submit) / count;
  context["event"] = "perf";
  context["frames"] = samples_.size();
  context["fps"] = Round3(fps);
  context["frame_ms"] = {
      {"mean", Round3(total / count)},
      {"p95", Round3(p95)},
      {"p99", Round3(p99)},
      {"max", Round3(*std::max_element(frames.begin(), frames.end()))}};
  context["update_ms_mean"] = Round3(update / count);
  context["render_submit_ms_mean"] = Round3(submit / count);
  context["present_wait_ms_mean"] = Round3(present / count);
  context["frames_over_20ms"] = slow;
  context["llm_busy_frames"] = busy_frames.size();
  context["llm_busy_p95_ms"] =
      busy_frames.empty()
          ? nlohmann::json(nullptr)
          : nlohmann::json(Round3(Percentile(busy_frames, .95)));
  context["llm_idle_p95_ms"] =
      idle_frames.empty()
          ? nlohmann::json(nullptr)
          : nlohmann::json(Round3(Percentile(idle_frames, .95)));
#ifdef __APPLE__
  mach_task_basic_info_data_t info{};
  mach_msg_type_number_t count_info = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                reinterpret_cast<task_info_t>(&info),
                &count_info) == KERN_SUCCESS) {
    context["game_rss_mb"] =
        Round3(static_cast<double>(info.resident_size) / (1024 * 1024));
  }
#else
  std::ifstream mem("/proc/self/statm");
  long pages = 0, resident = 0;
  if (mem >> pages >> resident) {
    context["game_rss_mb"] = Round3(static_cast<double>(resident) *
                                    sysconf(_SC_PAGESIZE) / (1024 * 1024));
  }
#endif
  Event(std::move(context));
  samples_.clear();
}
}  // namespace frontier
