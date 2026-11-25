#include "CalculateCanceler.h"

#include <exception>
#include <thread>
#include <utility>

namespace orch {

CalculateCanceler::CalculateCanceler(
    Logger &&logger, LoadChecker &load_checker,
    ExternalResponseHandler &response_handler, const std::string &service_origin,
    std::string endpoint, std::chrono::milliseconds timeout)
    : AsyncComponentBase(std::move(logger)), _load_checker(load_checker),
      _response_handler(response_handler), _client(service_origin),
      _endpoint(std::move(endpoint)), _timeout(timeout) {
  _client.set_connection_timeout(std::chrono::seconds(5));
  _client.set_max_timeout(std::chrono::seconds(5));
}

CalculateCanceler::~CalculateCanceler() {
  StopSync();
}

void CalculateCanceler::StartAsync(RunMode mode) {
  AsyncComponentBase::Start(mode, true);
}

void CalculateCanceler::ThreadJob() {
  const auto attempt = _load_checker.ClaimExpired(_timeout);
  if (!attempt) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return;
  }

  try {
    const json payload = {{"dedup_key", attempt->key},
                          {"attempt_id", attempt->id}};
    const auto response =
        _client.Post(_endpoint, payload.dump(), "application/json");
    if (!response) {
      LOG_ERROR("Cancel POST failed, key=" << attempt->key
                << ": " << httplib::to_string(response.error()));
    } else if (response->status < 200 || response->status >= 300) {
      LOG_ERROR("Cancel endpoint returned HTTP " << response->status
                << ", key=" << attempt->key);
    }
  } catch (const std::exception &ex) {
    LOG_ERROR("Failed to cancel calculate task, key=" << attempt->key
              << ": " << ex.what());
  }
  _response_handler.HandleTimeout(*attempt);
}

} // namespace orch
