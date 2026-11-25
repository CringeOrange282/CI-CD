#pragma once

#include "json.hpp"
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace orch {

using json = nlohmann::json;

class ResponseCache {
public:
    ResponseCache() = default;
    ~ResponseCache() = default;

    // Взять ответ из кэша по ключу (потокобезопасно, разрешает параллельное чтение)
    std::optional<json> Get(const std::string& dedup_key) const;

    // Сохранить или обновить ответ в кэше (эксклюзивная блокировка)
    void Put(const std::string& dedup_key, const json& response);

    // Проверить наличие ключа
    bool Contains(const std::string& dedup_key) const;

    // Удалить устаревшую запись (пригодится для инвалидации кэша)
    void Remove(const std::string& dedup_key);

    // Полная очистка кэша
    void Clear();

private:
    mutable std::shared_mutex _mutex;
    std::unordered_map<std::string, json> _storage;
};

} // namespace orch
