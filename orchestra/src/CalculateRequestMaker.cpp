#include "CalculateRequestMaker.h"

#include <exception>

namespace orch {

CalculateRequestMaker::CalculateRequestMaker(
    Logger &&logger,
    std::shared_ptr<Queue<ProcessTask>> process_queue,
    std::shared_ptr<IResponseCache> response_cache,
    std::shared_ptr<IRecipientRegistry> recipient_registry,
    std::shared_ptr<IDataReader> data_reader)
    : Loggable(std::move(logger)),
      _process_queue(std::move(process_queue)),
      _response_cache(std::move(response_cache)),
      _recipient_registry(std::move(recipient_registry)),
      _data_reader(std::move(data_reader)) {}

json CalculateRequestMaker::ProcessRequest(const Request &req) {
  LOG_DEBUG("Received request id=" << req.id << " from author=" << req.author);

  const auto dedup_key =
      json::array({req.name_process, req.document, req.date}).dump();

  bool registered_new_task = false;
  json response;
  try {
    auto result = _response_cache->Get(dedup_key);
    if (!result) {
      if (_recipient_registry->RegisterOrJoin(dedup_key, req.author)) {
        return {{"request_id", req.id}, {"enqueued", false},
                {"status", "in_progress"}};
      }
      registered_new_task = true;

      // Расчёт мог завершиться между первым чтением кеша и регистрацией.
      result = _response_cache->Get(dedup_key);
    }

    if (result) {
      response = {{"request_id", req.id}, {"enqueued", false},
                  {"status", "cached"}, {"result", std::move(*result)}};
      if (registered_new_task) {
        _recipient_registry->ExtractAll(dedup_key);
      }
      return response;
    }

    LOG_DEBUG("Reading data for doc=" << req.document << " date=" << req.date);
    json raw_data = _data_reader->ReadData(req.document, req.date);
    response = {{"request_id", req.id}, {"enqueued", true},
                {"status", "enqueued"}};

    _process_queue->Push(ProcessTask{req, dedup_key, std::move(raw_data)});
  } catch (const std::exception &ex) {
    if (registered_new_task) {
      _recipient_registry->ExtractAll(dedup_key);
    }
    LOG_ERROR("Failed to prepare or enqueue task, key="
              << dedup_key << ": " << ex.what());
    return {{"request_id", req.id}, {"enqueued", false},
            {"status", "failed"}, {"error", ex.what()}};
  }

  LOG_INFO("Task pushed to process queue, key=" << dedup_key);
  return response;
}

} // namespace orch
