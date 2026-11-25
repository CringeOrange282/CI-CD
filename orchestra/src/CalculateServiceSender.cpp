#include "CalculateServiceSender.h"

#include <chrono>
#include <exception>
#include <utility>

namespace orch {

CalculateServiceSender::CalculateServiceSender(
    Logger &&logger, std::shared_ptr<PriorityQueue<ProcessTask>> process_queue,
    LoadChecker &load_checker, ExternalResponseHandler &response_handler,
    const std::string &service_origin, std::string endpoint)
    : AsyncComponentBase(std::move(logger)),
      _process_queue(std::move(process_queue)),
      _load_checker(load_checker),
      _response_handler(response_handler),
      _client(service_origin),
      _endpoint(std::move(endpoint)) {
  _client.set_connection_timeout(std::chrono::seconds(30));
  _client.set_max_timeout(std::chrono::seconds(30));
}

CalculateServiceSender::~CalculateServiceSender() {
  StopSync();
}

void CalculateServiceSender::StartAsync(RunMode mode) {
  LOG_DEBUG("Starting calculate service sender, endpoint=" << _endpoint);
  // Дождаться запуска потока, чтобы немедленная остановка не потерялась.
  AsyncComponentBase::Start(mode, true);
}

void CalculateServiceSender::ThreadJob() {
  // Sender один. Пока он ждёт задачу, другие потоки могут только освободить слот.
  if (!_load_checker.WaitForCapacity(std::chrono::milliseconds(50))) {
    return;
  }
  auto task = _process_queue->WaitAndPop(std::chrono::milliseconds(50));
  if (!task) {
    return;
  }

  try {
    LOG_DEBUG("Dequeued calculate task, key=" << task->dedup_key
              << ", process=" << task->request.name_process);

    json payload = {
        {"dedup_key", task->dedup_key},
        {"input_data", std::move(task->input_data)},
        {"name_process", task->request.name_process},
    };
    const auto attempt = _load_checker.StartAttempt(task->dedup_key);
    payload["attempt_id"] = attempt.id;
    const std::string body = payload.dump();

    LOG_DEBUG("Sending POST " << _endpoint << ", key=" << task->dedup_key
              << ", bytes=" << body.size());
    const auto response = _client.Post(_endpoint, body, "application/json");

    if (!response) {
      // Сервис мог принять POST до разрыва связи. Попытку завершит callback или таймаут.
      LOG_ERROR("Calculate service POST failed, key=" << task->dedup_key
                << ": " << httplib::to_string(response.error()));
    } else if (response->status != httplib::StatusCode::OK_200) {
      LOG_ERROR("Calculate service returned HTTP " << response->status
                << ", key=" << task->dedup_key);
      _response_handler.HandleExternalError(
          attempt.key, attempt.id,
          "Calculate service returned HTTP " + std::to_string(response->status));
    } else {
      LOG_DEBUG("Calculate service accepted task, HTTP " << response->status
                << ", key=" << task->dedup_key);
    }
  } catch (const std::exception &ex) {
    LOG_ERROR("Failed to send calculate task, key=" << task->dedup_key
              << ": " << ex.what());
  }
}

} // namespace orch
