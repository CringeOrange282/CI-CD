#pragma once

#include <memory>
#include <mutex>
#include <optional>

#include "AsyncComponentBase.h"
#include "Interfaces.h"
#include "PriorityQueue.h"
#include "Queue.h"
#include "request.h"


namespace orch {

// Задача, которую модуль передаёт дальше в очередь процессов
struct ProcessTask {
  Request request;
  std::string dedup_key;
  json input_data;
};

// =========================================================================
// МОДУЛЬ ПРОВЕРКИ ПОЛЬЗОВАТЕЛЬСКИХ ЗАПРОСОВ
// =========================================================================

class RequestChecker : public AsyncComponentBase {
public:
  RequestChecker(Logger &&logger,
                 std::shared_ptr<PriorityQueue<Request>> incoming_queue,
                 std::shared_ptr<Queue<ProcessTask>> process_queue,
                 std::shared_ptr<IResponseCache> response_cache,
                 std::shared_ptr<IRecipientRegistry> recipient_registry,
                 std::shared_ptr<IDataReader> data_reader,
                 std::shared_ptr<ICancellationManager> cancellation_manager,
                 std::shared_ptr<IUserNotifier> user_notifier);

  ~RequestChecker() override = default;

protected:
  void ThreadJob() override;

private:
  void HandleRequest(Request &req);
  void PrepareAndEnqueueTask(Request &req, const std::string &dedup_key);
  void NotifyAllWaiting(const std::string &dedup_key, const json &payload);

private:
  std::shared_ptr<PriorityQueue<Request>> _incoming_queue;
  std::shared_ptr<Queue<ProcessTask>> _process_queue;

  std::shared_ptr<IResponseCache> _response_cache;
  std::shared_ptr<IRecipientRegistry> _recipient_registry;
  std::shared_ptr<IDataReader> _data_reader;
  std::shared_ptr<ICancellationManager> _cancellation_manager;
  std::shared_ptr<IUserNotifier> _user_notifier;

  std::mutex _state_mutex;
};

} // namespace orch
