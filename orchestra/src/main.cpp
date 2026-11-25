#include <charconv>
#include <exception>
#include <iostream>
#include <memory>
#include <string_view>

#include "CalculateRequestMaker.h"
#include "CalculateServiceSender.h"
#include "DummyClasses.h"
#include "Service.h"

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0]
              << " <listen-host> <listen-port> <service-origin> <service-path>\n";
    return 1;
  }

  const std::string_view port_arg(argv[2]);
  const char *port_end = port_arg.data() + port_arg.size();
  int port = 0;
  const auto [end, error] = std::from_chars(port_arg.data(), port_end, port);
  if (error != std::errc{} || end != port_end || port < 1 || port > 65535) {
    std::cerr << "Invalid listen port: " << port_arg << '\n';
    return 1;
  }

  try {
    auto queue = std::make_shared<orch::Queue<orch::ProcessTask>>();
    orch::CalculateRequestMaker request_maker(
        orch::Logger("CalculateRequestMaker"), queue, orch::MakeDummyCache(),
        orch::MakeDummyRegistry(), orch::MakeDummyReader());
    orch::CalculateServiceSender sender(
        orch::Logger("CalculateServiceSender"), queue, argv[3], argv[4]);

    sender.StartAsync();
    return orch::RunService(argv[1], port, request_maker) ? 0 : 1;
  } catch (const std::exception &ex) {
    std::cerr << "Orchestra failed: " << ex.what() << '\n';
    return 1;
  }
}
