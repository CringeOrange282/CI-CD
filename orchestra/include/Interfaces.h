#pragma once

#include "request.h"
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using json = orch::Request::json;

// =========================================================================
// ИНТЕРФЕЙСЫ СОСЕДНИХ МОДУЛЕЙ (АБСТРАКТНЫЕ КЛАССЫ)
// =========================================================================

class IResponseCache {
public:
  virtual ~IResponseCache() = default;
  virtual std::optional<json> Get(const std::string &dedup_key) const = 0;
};

class IRecipientRegistry {
public:
  virtual ~IRecipientRegistry() = default;
  // true -> задача уже выполняется, author добавлен в список ожидания
  // false -> задачи ещё не было, зарегистрирована новая с первым получателем
  // author
  virtual bool RegisterOrJoin(const std::string &dedup_key,
                              const std::string &author) = 0;
  virtual std::vector<std::string>
  ExtractSubscribers(const std::string &dedup_key) = 0;
};

class IDataReader {
public:
  virtual ~IDataReader() = default;
  virtual std::optional<json> ReadData(const std::string &document,
                                       const std::string &date) = 0;
};

class ICancellationManager {
public:
  virtual ~ICancellationManager() = default;
  virtual bool IsCancelled(const std::string &request_id) const = 0;
};

class IUserNotifier {
public:
  virtual ~IUserNotifier() = default;
  virtual void Notify(const std::string &author,
                      const json &response_payload) = 0;
  virtual void NotifyBatch(const std::vector<std::string> &authors,
                           const json &response_payload) {
    for (const auto &author : authors) {
      Notify(author, response_payload);
    }
  }
};
