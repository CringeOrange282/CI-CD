#pragma once

#include <memory>

#include "Interfaces.h"
#include "Loggable.h"
#include "PriorityQueue.h"
#include "request.h"


namespace orch {

// Задача, которую модуль передаёт дальше в очередь процессов
struct ProcessTask {
  Request request;
  std::string dedup_key;
  json input_data;
};

// =========================================================================
// МОДУЛЬ ПОДГОТОВКИ ЗАПРОСОВ НА РАСЧЁТ
// =========================================================================

class CalculateRequestMaker : public Loggable {
public:
  CalculateRequestMaker(Logger &&logger,
                        std::shared_ptr<PriorityQueue<ProcessTask>> process_queue,
                        std::shared_ptr<IResponseCache> response_cache,
                        std::shared_ptr<IRecipientRegistry> recipient_registry,
                        std::shared_ptr<IDataReader> data_reader);

  json ProcessRequest(const Request &req);

private:
  std::shared_ptr<PriorityQueue<ProcessTask>> _process_queue;
  std::shared_ptr<IResponseCache> _response_cache;
  std::shared_ptr<IRecipientRegistry> _recipient_registry;
  std::shared_ptr<IDataReader> _data_reader;
};

} // namespace orch
