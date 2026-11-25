#include "ExternalResponseCache.h"

#include <mutex>

namespace orch {

std::optional<json>
ExternalResponseCache::Get(const std::string &dedup_key) const {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  auto it = _storage.find(dedup_key);
  if (it == _storage.end()) {
    return std::nullopt;
  }
  return it->second;
}

void ExternalResponseCache::Put(const std::string &dedup_key,
                                const json &response) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _storage[dedup_key] = response;
}

bool ExternalResponseCache::Contains(const std::string &dedup_key) const {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return _storage.find(dedup_key) != _storage.end();
}

void ExternalResponseCache::Remove(const std::string &dedup_key) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _storage.erase(dedup_key);
}

void ExternalResponseCache::Clear() {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _storage.clear();
}

} // namespace orch
