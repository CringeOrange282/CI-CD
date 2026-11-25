#include "ResponseCache.h"

namespace orch {

std::optional<json> ResponseCache::Get(const std::string& dedup_key) const {
    std::shared_lock<std::shared_mutex> lock(_mutex);
    auto it = _storage.find(dedup_key);
    if (it == _storage.end()) {
        return std::nullopt;
    }
    return it->second;
}

void ResponseCache::Put(const std::string& dedup_key, const json& response) {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _storage[dedup_key] = response;
}

bool ResponseCache::Contains(const std::string& dedup_key) const {
    std::shared_lock<std::shared_mutex> lock(_mutex);
    return _storage.find(dedup_key) != _storage.end();
}

void ResponseCache::Remove(const std::string& dedup_key) {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _storage.erase(dedup_key);
}

void ResponseCache::Clear() {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _storage.clear();
}

} // namespace orch
