#include "ExternalResponseHandler.h"

namespace orch {

ExternalResponseHandler::ExternalResponseHandler(
    Logger &&logger, std::shared_ptr<ExternalResponseCache> cache,
    std::shared_ptr<IRecipientRegistry> recipient_registry,
    std::shared_ptr<IUserNotifier> user_notifier, LoadChecker &load_checker)
    : Loggable(std::move(logger)), _cache(std::move(cache)),
      _recipient_registry(std::move(recipient_registry)),
      _user_notifier(std::move(user_notifier)), _load_checker(load_checker) {}

void ExternalResponseHandler::HandleExternalSuccess(
    const std::string &dedup_key, const std::optional<std::string> &attempt_id,
    const json &result_data) {
  if (!_load_checker.ClaimResult(dedup_key, attempt_id)) {
    LOG_DEBUG("Ignoring result for inactive attempt, key=" << dedup_key);
    return;
  }
  LOG_INFO("Received successful response for key=" << dedup_key);

  // 1. Сначала сохраняем в кэш, чтобы все последующие запросы сразу брали
  // оттуда
  SaveToCache(dedup_key, result_data);

  // 2. Формируем итоговый JSON для пользователей и рассылаем всем подписавшимся
  json response_payload = {{"dedup_key", dedup_key},
                           {"status", "completed"},
                           {"from_cache", false},
                           {"data", result_data}};

  BroadcastToSubscribers(dedup_key, response_payload);
}

void ExternalResponseHandler::HandleExternalError(
    const std::string &dedup_key, const std::optional<std::string> &attempt_id,
    const std::string &error_message) {
  if (!_load_checker.ClaimResult(dedup_key, attempt_id)) {
    LOG_DEBUG("Ignoring error for inactive attempt, key=" << dedup_key);
    return;
  }
  CompleteWithError(dedup_key, error_message);
}

void ExternalResponseHandler::HandleTimeout(const LoadChecker::Attempt &attempt) {
  CompleteWithError(attempt.key, "Calculation timed out");
}

void ExternalResponseHandler::CompleteWithError(
    const std::string &dedup_key, const std::string &error_message) {
  LOG_ERROR("Received error from external service for key="
            << dedup_key << ": " << error_message);

  // Ошибку в кэш НЕ сохраняем, но обязательно уведомляем всех ждущих и очищаем
  // подписку
  json error_payload = {
      {"dedup_key", dedup_key}, {"status", "failed"}, {"error", error_message}};

  BroadcastToSubscribers(dedup_key, error_payload);
}

std::optional<json>
ExternalResponseHandler::GetCachedResponse(const std::string &dedup_key) const {
  return _cache->Get(dedup_key);
}

void ExternalResponseHandler::SaveToCache(const std::string &dedup_key,
                                          const json &result_data) {
  _cache->Put(dedup_key, result_data);
  LOG_DEBUG("Saved response to cache for key=" << dedup_key);
}

void ExternalResponseHandler::BroadcastToSubscribers(
    const std::string &dedup_key, const json &payload) {
  // Освобождаем слот до удаления подписки, после которого возможен повтор.
  _load_checker.Release(dedup_key);
  std::vector<std::string> subscribers =
      _recipient_registry->ExtractSubscribers(dedup_key);

  if (subscribers.empty()) {
    LOG_WARN("No active subscribers found for key=" << dedup_key);
    return;
  }

  LOG_DEBUG("Broadcasting response for key=" << dedup_key << " to "
            << subscribers.size() << " subscribers");
  _user_notifier->NotifyBatch(subscribers, payload);
}

} // namespace orch
