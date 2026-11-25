#include "JsonDatabase.h"

#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace orch {

// Конструктор только сохраняет путь к файлу, никакой записи на диск
JsonDatabase::JsonDatabase(std::string file_path)
    : _file_path(std::move(file_path)) {}

json JsonDatabase::LoadArrayFromFile() const {
  std::ifstream file(_file_path);
  if (!file.is_open()) {
    if (std::filesystem::exists(_file_path)) {
      throw std::runtime_error("Cannot open data file: " + _file_path);
    }
    return json::array();
  }
  if (file.peek() == std::ifstream::traits_type::eof()) {
    return json::array();
  }

  json data;
  file >> data;
  if (!data.is_array()) {
    throw std::runtime_error("Expected an array in data file: " + _file_path);
  }
  return data;
}

void JsonDatabase::SaveArrayToFile(const json &records) const {
  std::ofstream file(_file_path, std::ios::trunc);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open file for writing: " + _file_path);
  }
  file << records.dump(4);
}

std::optional<json> JsonDatabase::ReadData(const std::string &document,
                                          const std::string &date) {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  json records = LoadArrayFromFile();

  for (auto &item : records) {
    if (item.value("document", "") == document &&
        item.value("date", "") == date) {
      auto content = std::move(item.at("content"));
      if (!content.is_object()) {
        throw std::runtime_error("Expected an object in content for document: " +
                                 document);
      }
      content["doc"] = document;
      content["date"] = date;
      return content;
    }
  }
  return std::nullopt;
}

void JsonDatabase::WriteData(const std::string &document,
                             const std::string &date, const json &content) {
  if (!content.is_object()) {
    throw std::invalid_argument("Expected an object in content");
  }
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
