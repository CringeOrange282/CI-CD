#include "JsonDatabase.h"
#include <fstream>
#include <stdexcept>

namespace orch {

// Конструктор только сохраняет путь к файлу, никакой записи на диск
JsonDatabase::JsonDatabase(std::string file_path)
    : _file_path(std::move(file_path)) {}

json JsonDatabase::LoadArrayFromFile() const {
  std::ifstream file(_file_path);
  // Если файла ещё не существует или он пуст — возвращаем пустой массив []
  if (!file.is_open() || file.peek() == std::ifstream::traits_type::eof()) {
    return json::array();
  }

  json data;
  file >> data;
  return data.is_array() ? data : json::array();
}

void JsonDatabase::SaveArrayToFile(const json &records) const {
  std::ofstream file(_file_path, std::ios::trunc);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open file for writing: " + _file_path);
  }
  file << records.dump(4);
}

std::optional<json> JsonDatabase::Read(const std::string &document,
                                       const std::string &date) const {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  json records = LoadArrayFromFile();

  for (const auto &item : records) {
    if (item.value("document", "") == document &&
        item.value("date", "") == date) {
      return item.value("content", json::object());
    }
  }
  return std::nullopt;
}

void JsonDatabase::Write(const std::string &document, const std::string &date,
                         const json &content) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  json records = LoadArrayFromFile();

  for (auto &item : records) {
    if (item.value("document", "") == document &&
        item.value("date", "") == date) {
      item["content"] = content;
      SaveArrayToFile(records);
      return;
    }
  }

  records.push_back(
      {{"document", document}, {"date", date}, {"content", content}});
  SaveArrayToFile(records);
}

} // namespace orch
