#pragma once

#include "ExternalResponseCache.h"
#include "LoadChecker.h"
#include "Loggable.h"

#include "Interfaces.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace orch {

// =========================================================================
// МОДУЛЬ РАБОТЫ С ОТВЕТАМИ ОТ ВНЕШНЕГО СЕРВИСА
// =========================================================================

class ExternalResponseHandler : public Loggable {
public:
  ExternalResponseHandler(
      Logger &&logger, std::shared_ptr<ExternalResponseCache> cache,
      std::shared_ptr<IRecipientRegistry> recipient_registry,
      std::shared_ptr<IUserNotifier> user_notifier, LoadChecker &load_checker);

  ~ExternalResponseHandler() = default;

  // 1. Принять успешный ответ от внешнего сервиса -> сохранить в кэш ->
  // разослать
  void HandleExternalSuccess(const std::string &dedup_key,
                             const std::optional<std::string> &attempt_id,
                             const json &result_data);

  // 2. Принять ошибку от внешнего сервиса -> НЕ кэшировать -> уведомить
  // подписавшихся
  void HandleExternalError(const std::string &dedup_key,
                           const std::optional<std::string> &attempt_id,
                           const std::string &error_message);

  // CalculateCanceler уже закрепил завершение просроченной попытки за собой.
  void HandleTimeout(const LoadChecker::Attempt &attempt);

  // 3. Взять готовый ответ из кэша.
  std::optional<json> GetCachedResponse(const std::string &dedup_key) const;

private:
  void CompleteWithError(const std::string &dedup_key,
                         const std::string &error_message);
  void SaveToCache(const std::string &dedup_key, const json &result_data);
  void BroadcastToSubscribers(const std::string &dedup_key,
                              const json &payload);

private:
  std::shared_ptr<ExternalResponseCache> _cache;
  std::shared_ptr<IRecipientRegistry> _recipient_registry;
  std::shared_ptr<IUserNotifier> _user_notifier;
  LoadChecker &_load_checker;
};

} // namespace orch
