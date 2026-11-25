#pragma once

#include "Interfaces.h"
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace orch {

using json = nlohmann::json;

class ExternalResponseCache : public IResponseCache {
public:
  ExternalResponseCache() = default;
  ~ExternalResponseCache() override = default;

  // Взять ответ из кэша по ключу (потокобезопасно, разрешает параллельное
  // чтение)
  std::optional<json> Get(const std::string &dedup_key) const override;

  // Сохранить или обновить ответ в кэше (эксклюзивная блокировка)
  void Put(const std::string &dedup_key, const json &response);

  // Проверить наличие ключа
  bool Contains(const std::string &dedup_key) const;

  // Удалить устаревшую запись (пригодится для инвалидации кэша)
  void Remove(const std::string &dedup_key);

  // Полная очистка кэша
  void Clear();

private:
  mutable std::shared_mutex _mutex;
  std::unordered_map<std::string, json> _storage;
};

} // namespace orch
