#include "Notifier.h"

#include <chrono>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace orch {

Notifier::Notifier(Logger &&logger, const std::string &origin,
                   std::string endpoint)
    : AsyncComponentBase(std::move(logger)), _client(origin),
      _endpoint(std::move(endpoint)) {
  _client.set_connection_timeout(std::chrono::seconds(30));
  _client.set_max_timeout(std::chrono::seconds(30));
}

Notifier::~Notifier() {
  // Завершаем поток, пока очередь и HTTP-клиент ещё доступны.
  StopSync();
}

void Notifier::StartAsync(RunMode mode) {
  // Дожидаемся старта потока, чтобы немедленный StopSync не потерялся.
  AsyncComponentBase::Start(mode, true);
}

void Notifier::Notify(const std::string &author, const json &response_payload) {
  NotifyBatch({author}, response_payload);
}

void Notifier::NotifyBatch(const std::vector<std::string> &authors,
                           const json &response_payload) {
  const auto key = response_payload.find("dedup_key");
  const auto *dedup_key = key == response_payload.end()
                              ? nullptr
                              : key->get_ptr<const std::string *>();
  const std::string_view log_key = dedup_key ? *dedup_key : std::string_view{};
  LOG_DEBUG("Notification received, key=" << log_key
            << ", recipients=" << authors.size());

  // Вся группа подписчиков попадает в очередь как одно уведомление.
  if (!authors.empty()) {
    _notifications.Push(Notification{authors, response_payload});
    LOG_DEBUG("Notification enqueued, key=" << log_key
              << ", recipients=" << authors.size());
  }
}

void Notifier::ThreadJob() {
  // Периодически возвращаем управление базовому классу для проверки остановки.
  auto notification = _notifications.WaitAndPop(std::chrono::milliseconds(50));
  if (!notification) {
    return;
  }

  std::string dedup_key;
  try {
    const auto &payload = notification->payload;
    dedup_key = payload.at("dedup_key").get<std::string>();
    // CalculateRequestMaker записывает в ключ операцию, документ и дату в этом порядке.
    const auto metadata = json::parse(dedup_key);
    if (!metadata.is_array() || metadata.size() != 3) {
      throw std::invalid_argument("Expected operation, document and date in dedup_key");
    }

    const auto status = payload.at("status").get<std::string>();
    // В поле result передаём данные расчёта или объект со статусом и текстом ошибки.
    json result = status == "failed"
                      ? json{{"status", status}, {"error", payload.at("error")}}
                      : payload.at("data");
    const json body = {
        {"список получателей", notification->authors},
        {"document", metadata.at(1).get<std::string>()},
        {"date", metadata.at(2).get<std::string>()},
        {"operation", metadata.at(0).get<std::string>()},
        {"result", std::move(result)},
    };
    // Для каждого уведомления выполняется одна попытка отправки.
    const auto response = _client.Post(_endpoint, body.dump(), "application/json");
    if (!response) {
      LOG_ERROR("Notification POST failed, key=" << dedup_key
                << ": " << httplib::to_string(response.error()));
    } else if (response->status < 200 || response->status >= 300) {
      LOG_ERROR("Notification endpoint returned HTTP " << response->status
                << ", key=" << dedup_key);
    } else {
      LOG_DEBUG("Notification sent, key=" << dedup_key
                << ", recipients=" << notification->authors.size());
    }
  } catch (const std::exception &ex) {
    // Ошибка текущего уведомления не прерывает обработку очереди.
    LOG_ERROR("Failed to send notification, key=" << dedup_key << ": " << ex.what());
  }
}

} // namespace orch
