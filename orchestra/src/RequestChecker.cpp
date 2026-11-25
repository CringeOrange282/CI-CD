#include "RequestChecker.h"

#include <chrono>

namespace orch {

RequestChecker::RequestChecker(
    Logger &&logger, std::shared_ptr<PriorityQueue<Request>> incoming_queue,
    std::shared_ptr<Queue<ProcessTask>> process_queue,
    std::shared_ptr<IResponseCache> response_cache,
    std::shared_ptr<IRecipientRegistry> recipient_registry,
    std::shared_ptr<IDataReader> data_reader,
    std::shared_ptr<ICancellationManager> cancellation_manager,
    std::shared_ptr<IUserNotifier> user_notifier)
    : AsyncComponentBase(std::move(logger)),
      _incoming_queue(std::move(incoming_queue)),
      _process_queue(std::move(process_queue)),
      _response_cache(std::move(response_cache)),
      _recipient_registry(std::move(recipient_registry)),
      _data_reader(std::move(data_reader)),
      _cancellation_manager(std::move(cancellation_manager)),
      _user_notifier(std::move(user_notifier)) {}

void RequestChecker::ThreadJob() {
  auto req_opt = _incoming_queue->WaitAndPop(std::chrono::milliseconds(50));
  if (!req_opt.has_value()) {
    return;
  }

  HandleRequest(req_opt.value());
}

void RequestChecker::HandleRequest(Request &req) {
  LOG_DEBUG("Received request id=", req.id, " from author=", req.author);

  if (_cancellation_manager->IsCancelled(req.id)) {
    LOG_INFO("Request ", req.id, " is cancelled before processing");
    _user_notifier->Notify(req.author,
                           {{"request_id", req.id}, {"status", "cancelled"}});
    return;
  }

  std::string dedup_key = req.getDedupKey();

  // Иначе читаем данные и отправляем задачу в очередь процессов
  PrepareAndEnqueueTask(req, dedup_key);
}

void RequestChecker::PrepareAndEnqueueTask(Request &req,
                                           const std::string &dedup_key) {
  try {
    LOG_DEBUG("Reading data for doc=", req.document, " date=", req.date);
    json raw_data = _data_reader->ReadData(req.document, req.date);

    if (_cancellation_manager->IsCancelled(req.id)) {
      LOG_INFO("Request ", req.id, " cancelled after data read");
      NotifyAllWaiting(dedup_key,
                       {{"request_id", req.id}, {"status", "cancelled"}});
      return;
    }

    ProcessTask task{std::move(req), dedup_key, std::move(raw_data)};
    _process_queue->Push(std::move(task));
    LOG_INFO("Task pushed to process queue, key=", dedup_key);

  } catch (const std::exception &ex) {
    LOG_ERROR("Data read failed for key=", dedup_key, ": ", ex.what());
    NotifyAllWaiting(
        dedup_key,
        {{"request_id", req.id}, {"status", "failed"}, {"error", ex.what()}});
  }
}

void RequestChecker::NotifyAllWaiting(const std::string &dedup_key,
                                      const json &payload) {
  auto waiting_authors = _recipient_registry->ExtractSubscribers(dedup_key);
  for (const auto &author : waiting_authors) {
    _user_notifier->Notify(author, payload);
  }
}

} // namespace orch
