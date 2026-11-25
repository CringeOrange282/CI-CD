#pragma once

#include "AsyncComponentBase.h"
#include "ExternalResponseHandler.h"

#include <chrono>
#include <httplib.h>
#include <string>

namespace orch {

class CalculateCanceler : public AsyncComponentBase {
public:
  CalculateCanceler(Logger &&logger, LoadChecker &load_checker,
                    ExternalResponseHandler &response_handler,
                    const std::string &service_origin, std::string endpoint,
                    std::chrono::milliseconds timeout);
  ~CalculateCanceler() override;

  void StartAsync(RunMode mode = RunMode::RUN_IN_LOOP) override;

protected:
  void ThreadJob() override;

private:
  LoadChecker &_load_checker;
  ExternalResponseHandler &_response_handler;
  httplib::Client _client;
  std::string _endpoint;
  std::chrono::milliseconds _timeout;
};

} // namespace orch
