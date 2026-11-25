#pragma once

#include <memory>
#include <string>

#include <httplib.h>

#include "AsyncComponentBase.h"
#include "CalculateRequestMaker.h"

namespace orch {

class CalculateServiceSender : public AsyncComponentBase {
public:
  CalculateServiceSender(Logger &&logger,
                         std::shared_ptr<PriorityQueue<ProcessTask>> process_queue,
                         const std::string &service_origin, std::string endpoint);
  ~CalculateServiceSender() override;

  void StartAsync(RunMode mode = RunMode::RUN_IN_LOOP) override;

protected:
  void ThreadJob() override;

private:
  std::shared_ptr<PriorityQueue<ProcessTask>> _process_queue;
  httplib::Client _client;
  std::string _endpoint;
};

} // namespace orch
