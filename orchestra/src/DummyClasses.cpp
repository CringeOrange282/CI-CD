#include "DummyClasses.h"

#include <unordered_map>

namespace orch {

// Минимальные заглушки для запуска
class DummyCache : public IResponseCache {
public:
  std::unordered_map<std::string, json> storage;
  std::optional<json> Get(const std::string &key) const override {
    auto it = storage.find(key);
    return (it != storage.end()) ? std::optional<json>(it->second)
                                 : std::nullopt;
  }
};

class DummyRegistry : public IRecipientRegistry {
public:
  std::unordered_map<std::string, std::vector<std::string>> active;
  bool RegisterOrJoin(const std::string &key,
                      const std::string &author) override {
    bool exists = active.count(key) > 0;
    active[key].push_back(author);
    return exists;
  }
  std::vector<std::string> ExtractAll(const std::string &key) override {
    auto res = std::move(active[key]);
    active.erase(key);
    return res;
  }
};

class DummyReader : public IDataReader {
public:
  json ReadData(const std::string &doc, const std::string &date) override {
    return {{"doc", doc}, {"date", date}, {"portfolio_sum", 1500000}};
  }
};

class DummyNotifier : public IUserNotifier {
public:
  void Notify(const std::string &author, const json &payload) override {
    std::cout << "[NOTIFY -> " << author << "]: " << payload.dump()
              << std::endl;
  }
};

std::shared_ptr<IResponseCache> MakeDummyCache() {
  return std::make_shared<DummyCache>();
}

std::shared_ptr<IRecipientRegistry> MakeDummyRegistry() {
  return std::make_shared<DummyRegistry>();
}

std::shared_ptr<IDataReader> MakeDummyReader() {
  return std::make_shared<DummyReader>();
}

} // namespace orch
