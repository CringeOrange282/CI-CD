#pragma once

#include "json.hpp"
#include <optional>
#include <shared_mutex>
#include <string>

namespace orch {

using json = nlohmann::json;

class JsonDatabase {
public:
  explicit JsonDatabase(std::string file_path);
  ~JsonDatabase() = default;

  std::optional<json> Read(const std::string &document,
                           const std::string &date) const;
  void Write(const std::string &document, const std::string &date,
             const json &content);

private:
  json LoadArrayFromFile() const;
  void SaveArrayToFile(const json &records) const;

private:
  std::string _file_path;
  mutable std::shared_mutex _mutex;
};

} // namespace orch
