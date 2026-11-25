#include <charconv>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>

#include "CalculateRequestMaker.h"
#include "CalculateServiceSender.h"
#include "DummyClasses.h"
#include "ExternalResponseCache.h"
#include "ExternalResponseHandler.h"
#include "JsonDatabase.h"
#include "Notifier.h"
#include "RecipientList.h"
#include "Service.h"

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
        recipient_registry, user_notifier);
    
    orch::CalculateServiceSender sender(
        orch::Logger("CalculateServiceSender"), queue, argv[3], argv[4]);

    sender.StartAsync();
    
    return orch::RunService(argv[1], port, request_maker, response_handler) ? 0 : 1;

  } catch (const std::exception &ex) {
    std::cerr << "Orchestra failed: " << ex.what() << '\n';
    return 1;
  }
}
