#pragma once

#include <memory>
#include <string>

#include <httplib.h>

#include "AsyncComponentBase.h"
#include "CalculateRequestMaker.h"
#include "ExternalResponseHandler.h"

namespace orch {

class CalculateServiceSender : public AsyncComponentBase {
public:
  CalculateServiceSender(Logger &&logger,
                         std::shared_ptr<PriorityQueue<ProcessTask>> process_queue,
                         LoadChecker &load_checker,
                         ExternalResponseHandler &response_handler,
                         const std::string &service_origin, std::string endpoint);
  ~CalculateServiceSender() override;

  void StartAsync(RunMode mode = RunMode::RUN_IN_LOOP) override;

protected:
  void ThreadJob() override;

private:
  std::shared_ptr<PriorityQueue<ProcessTask>> _process_queue;
  LoadChecker &_load_checker;
  ExternalResponseHandler &_response_handler;
  httplib::Client _client;
  std::string _endpoint;
};

} // namespace orch
