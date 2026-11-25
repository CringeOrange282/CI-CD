#pragma once

#include "Loggable.h"
#include "ResponseCache.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace orch {

// Контракты соседних модулей (список подписавшихся и отправка на фронт)
class IRecipientRegistry {
public:
    virtual ~IRecipientRegistry() = default;
    // Извлекает и удаляет всех пользователей, подписавшихся на данный dedup_key
    virtual std::vector<std::string> ExtractSubscribers(const std::string& dedup_key) = 0;
};

class IUserNotifier {
public:
    virtual ~IUserNotifier() = default;
    virtual void Notify(const std::string& author, const json& payload) = 0;
};

// =========================================================================
// МОДУЛЬ РАБОТЫ С ОТВЕТАМИ ОТ ВНЕШНЕГО СЕРВИСА
// =========================================================================

class ResponseHandler : public Loggable {
public:
    ResponseHandler(
        Logger&& logger,
        std::shared_ptr<ResponseCache> cache,
        std::shared_ptr<IRecipientRegistry> recipient_registry,
        std::shared_ptr<IUserNotifier> user_notifier
    );

    ~ResponseHandler() = default;

    // 1. Принять успешный ответ от внешнего сервиса -> сохранить в кэш -> разослать
    void HandleExternalSuccess(const std::string& dedup_key, const json& result_data);

    // 2. Принять ошибку от внешнего сервиса -> НЕ кэшировать -> уведомить подписавшихся
    void HandleExternalError(const std::string& dedup_key, const std::string& error_message);

    // 3. Взять готовый ответ из кэша (для модуля проверки запросов RequestChecker)
    std::optional<json> GetCachedResponse(const std::string& dedup_key) const;

private:
    void SaveToCache(const std::string& dedup_key, const json& result_data);
    void BroadcastToSubscribers(const std::string& dedup_key, const json& payload);

private:
    std::shared_ptr<ResponseCache> _cache;
    std::shared_ptr<IRecipientRegistry> _recipient_registry;
    std::shared_ptr<IUserNotifier> _user_notifier;
};

} // namespace orch
