#include "CalculateServiceSender.h"

#include <chrono>
#include <exception>
#include <utility>

namespace orch {

CalculateServiceSender::CalculateServiceSender(
    Logger &&logger, std::shared_ptr<Queue<ProcessTask>> process_queue,
    const std::string &service_origin, std::string endpoint)
    : AsyncComponentBase(std::move(logger)),
      _process_queue(std::move(process_queue)),
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
  auto task = _process_queue->WaitAndPop(std::chrono::milliseconds(50));
  if (!task) {
    return;
  }

  try {
    LOG_DEBUG("Dequeued calculate task, key=" << task->dedup_key
              << ", process=" << task->request.name_process);

    const json payload = {
        {"dedup_key", task->dedup_key},
        {"input_data", std::move(task->input_data)},
        {"name_process", task->request.name_process},
    };
    const std::string body = payload.dump();

    LOG_DEBUG("Sending POST " << _endpoint << ", key=" << task->dedup_key
              << ", bytes=" << body.size());
    const auto response = _client.Post(_endpoint, body, "application/json");

    if (!response) {
      LOG_ERROR("Calculate service POST failed, key=" << task->dedup_key
                << ": " << httplib::to_string(response.error()));
    } else if (response->status != httplib::StatusCode::OK_200) {
      LOG_ERROR("Calculate service returned HTTP " << response->status
                << ", key=" << task->dedup_key);
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
