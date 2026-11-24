#include "Service.h"

#include <charconv>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

const char* EnvOr(const char* name, const char* fallback)
{
    const char* value = std::getenv(name);
    return value && *value ? value : fallback;
}

} // namespace

int main()
{
    const auto* host = EnvOr("ORCHESTRA_HOST", "127.0.0.1");
    const auto* information_url = EnvOr("ORCHESTRA_DATA_URL", "http://127.0.0.1:8081");
    const std::string_view port_text = EnvOr("ORCHESTRA_PORT", "8080");

    int port = 0;
    const auto* end = port_text.data() + port_text.size();
    const auto [last, error] = std::from_chars(port_text.data(), end, port);
    if (last != end || error != std::errc{} || port < 1 || port > 65535) {
        std::cerr << "ORCHESTRA_PORT must be an integer between 1 and 65535.\n";
        return 1;
    }

    return orch::RunService(host, port, information_url) ? 0 : 1;
}
