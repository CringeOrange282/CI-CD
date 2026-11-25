#include "LoadChecker.h"

#include <random>
#include <stdexcept>

namespace orch {

LoadChecker::LoadChecker(std::size_t capacity)
    : _capacity(capacity), _run_id(std::to_string(std::random_device{}()) + "-" +
                                  std::to_string(std::random_device{}())) {
  if (capacity == 0) {
    throw std::invalid_argument("Load checker requires a positive capacity");
  }
}

bool LoadChecker::WaitForCapacity(std::chrono::milliseconds wait) {
  std::unique_lock lock(_mutex);
  return _capacity_available.wait_for(
      lock, wait, [this] { return _active.size() < _capacity; });
}

LoadChecker::Attempt LoadChecker::StartAttempt(const std::string &key) {
  const std::lock_guard lock(_mutex);
  if (_active.size() >= _capacity || _active.contains(key)) {
    throw std::logic_error("No capacity to start calculate task: " + key);
  }

  Attempt attempt{key, _run_id + "-" + std::to_string(++_next_id)};
  const bool first_dispatch = _dispatched_keys.insert(key).second;
  _active.emplace(key, ActiveAttempt{attempt.id, Clock::now(), first_dispatch});
  return attempt;
}

bool LoadChecker::ClaimResult(
    const std::string &key, const std::optional<std::string> &attempt_id) {
  const std::lock_guard lock(_mutex);
  const auto it = _active.find(key);
  if (it == _active.end() || it->second.claimed) {
    return false;
  }
  auto &active = it->second;
  if (attempt_id ? *attempt_id != active.id : !active.accepts_legacy_result) {
    return false;
  }
  active.claimed = true;
  return true;
}

std::optional<LoadChecker::Attempt>
LoadChecker::ClaimExpired(std::chrono::milliseconds timeout) {
  const std::lock_guard lock(_mutex);
  const auto now = Clock::now();
  auto oldest = _active.end();
  for (auto it = _active.begin(); it != _active.end(); ++it) {
    const auto &active = it->second;
    if (!active.claimed &&
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - active.sent_at) >= timeout &&
        (oldest == _active.end() || active.sent_at < oldest->second.sent_at)) {
      oldest = it;
    }
  }
  if (oldest == _active.end()) {
    return std::nullopt;
  }

  Attempt attempt{oldest->first, oldest->second.id};
  oldest->second.claimed = true;
  return attempt;
}

void LoadChecker::Release(const std::string &key) {
  {
    const std::lock_guard lock(_mutex);
    _active.erase(key);
  }
  _capacity_available.notify_one();
}

} // namespace orch
