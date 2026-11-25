#include <charconv>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "CalculateCanceler.h"
#include "CalculateRequestMaker.h"
#include "CalculateServiceSender.h"
#include "DummyClasses.h"
#include "ExternalResponseCache.h"
#include "ExternalResponseHandler.h"
#include "JsonDatabase.h"
#include "LoadChecker.h"
#include "Notifier.h"
#include "RecipientList.h"
#include "Service.h"

namespace {

struct LoadConfig {
  std::size_t max_concurrent_requests = 4;
  std::chrono::milliseconds request_timeout{300'000};
  std::string cancel_path = "/cancel";
};

template <typename T>
T ReadPositiveInteger(const json &config, const char *field, T fallback) {
  const auto value = config.find(field);
  if (value == config.end()) {
    return fallback;
  }
  if (!value->is_number_integer() ||
      (!value->is_number_unsigned() && value->get<json::number_integer_t>() <= 0)) {
    throw std::invalid_argument(std::string(field) + " must be a positive integer");
  }
  const auto number = value->get<json::number_unsigned_t>();
  if (number == 0 || number > std::numeric_limits<T>::max()) {
    throw std::invalid_argument(std::string(field) + " is out of range");
  }
  return static_cast<T>(number);
}

LoadConfig ReadLoadConfig() {
  LoadConfig config;
  const char *path = std::getenv("ORCHESTRA_CONFIG_FILE");
  if (!path || !*path) {
    return config;
  }
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error(std::string("Cannot open Orchestra config: ") + path);
  }
  const auto values = json::parse(file);
  if (!values.is_object()) {
    throw std::invalid_argument("Orchestra config must be a JSON object");
  }
  config.max_concurrent_requests = ReadPositiveInteger(
      values, "max_concurrent_requests", config.max_concurrent_requests);
  config.request_timeout = std::chrono::milliseconds(ReadPositiveInteger(
      values, "request_timeout_ms", config.request_timeout.count()));
  config.cancel_path = values.value("cancel_path", config.cancel_path);
  if (config.cancel_path.empty() || config.cancel_path.front() != '/') {
    throw std::invalid_argument("cancel_path must start with '/'");
  }
  return config;
}

} // namespace

int main(int argc, char *argv[]) {
  if (argc != 5 && argc != 7) {
    std::cerr << "Usage: " << argv[0]
              << " <listen-host> <listen-port> <service-origin> <service-path>"
              << " [<notifier-origin> <notifier-path>]\n";
    return 1;
  }

  orch::Logger::SetLogLevel(orch::Logger::LogLevel::DEBUG);

  const std::string_view port_arg(argv[2]);
  const char *port_end = port_arg.data() + port_arg.size();
  int port = 0;
  const auto [end, error] = std::from_chars(port_arg.data(), port_end, port);
  if (error != std::errc{} || end != port_end || port < 1 || port > 65535) {
    std::cerr << "Invalid listen port: " << port_arg << '\n';
    return 1;
  }

  try {
    const auto config = ReadLoadConfig();
    orch::LoadChecker load_checker(config.max_concurrent_requests);
    auto queue = std::make_shared<orch::PriorityQueue<orch::ProcessTask>>();
    auto response_cache = std::make_shared<orch::ExternalResponseCache>();
    const char *data_file = std::getenv("ORCHESTRA_DATA_FILE");
    auto data_reader = std::make_shared<orch::JsonDatabase>(
        data_file && *data_file ? data_file : "data.json");

    // Время жизни списка определяется самим синглтоном.
    auto recipient_registry = std::shared_ptr<IRecipientRegistry>(
        &RecipientList::GetInstance(), [](IRecipientRegistry *) {});

    orch::CalculateRequestMaker request_maker(
        orch::Logger("CalculateRequestMaker"), queue, response_cache,
        recipient_registry, data_reader);

    std::shared_ptr<IUserNotifier> user_notifier;
    if (argc == 7) {
      auto notifier = std::make_shared<orch::Notifier>(
          orch::Logger("Notifier"), argv[5], argv[6]);
      notifier->StartAsync();
      user_notifier = std::move(notifier);
    } 
    else {
      user_notifier = orch::MakeDummyNotifier();
    }
    
    orch::ExternalResponseHandler response_handler(
        orch::Logger("ExternalResponseHandler"), response_cache,
        recipient_registry, user_notifier, load_checker);
    
    orch::CalculateCanceler canceler(
        orch::Logger("CalculateCanceler"), load_checker, response_handler,
        argv[3], config.cancel_path, config.request_timeout);
    orch::CalculateServiceSender sender(
        orch::Logger("CalculateServiceSender"), queue, load_checker,
        response_handler, argv[3], argv[4]);

    canceler.StartAsync();
    sender.StartAsync();
    
    return orch::RunService(argv[1], port, request_maker, response_handler) ? 0 : 1;

  } catch (const std::exception &ex) {
    std::cerr << "Orchestra failed: " << ex.what() << '\n';
    return 1;
  }
}
