#pragma once

#include "Interfaces.h"
#include "json.hpp"
#include <optional>
#include <shared_mutex>
#include <string>

namespace orch {

using json = nlohmann::json;

class JsonDatabase : public IDataReader {
public:
  explicit JsonDatabase(std::string file_path);
  ~JsonDatabase() override = default;

  // Реализация метода IDataReader
  std::optional<json> ReadData(const std::string &document,
                               const std::string &date) override;

  // Запись или обновление документа за дату (для наполнения базы/тестов)
  void WriteData(const std::string &document, const std::string &date,
                 const json &content);

private:
  json LoadArrayFromFile() const;
  void SaveArrayToFile(const json &records) const;

private:
  std::string _file_path;
  mutable std::shared_mutex _mutex;
};

} // namespace orch
