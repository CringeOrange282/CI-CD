#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace orch {

class LoadChecker {
public:
  struct Attempt {
    std::string key;
    std::string id;
  };

  explicit LoadChecker(std::size_t capacity);

  bool WaitForCapacity(std::chrono::milliseconds wait);
  Attempt StartAttempt(const std::string &key);
  bool ClaimResult(const std::string &key,
                   const std::optional<std::string> &attempt_id);
  std::optional<Attempt> ClaimExpired(std::chrono::milliseconds timeout);
  void Release(const std::string &key);

private:
  using Clock = std::chrono::steady_clock;

  struct ActiveAttempt {
    std::string id;
    Clock::time_point sent_at;
    bool accepts_legacy_result;
    bool claimed = false;
  };

  const std::size_t _capacity;
  const std::string _run_id;
  std::mutex _mutex;
  std::condition_variable _capacity_available;
  std::unordered_map<std::string, ActiveAttempt> _active;
  std::unordered_set<std::string> _dispatched_keys;
  std::size_t _next_id = 0;
};

} // namespace orch
