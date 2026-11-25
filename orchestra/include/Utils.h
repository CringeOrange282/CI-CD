#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <sstream>
#include <chrono>

#define THROW_RUNTIME_EXCEPTION(...) \
        orch::utils::ThrowRuntimeException(__func__, __VA_ARGS__)

namespace orch::utils {

static constexpr uint32_t                  SYNC_TIMEOUT_MS = 200;
static constexpr std::chrono::milliseconds SYNC_TIMEOUT { SYNC_TIMEOUT_MS };

template<typename... Args>
[[noreturn]] void ThrowRuntimeException(const std::string& prefix, Args&&... args) 
{
    std::stringstream ss;
    ss << prefix << " : ";
    (ss << ... << std::forward<Args>(args));
    throw std::runtime_error(ss.str());
}

std::string GetFileContent(const std::string& file_path);

bool WaitFor(std::function<bool(void)> terminator, std::chrono::milliseconds timeout);

template<typename... Args>
static std::string StringFromParts(Args&&... args) {
    std::stringstream ss;
    (ss << ... << std::forward<Args>(args));
    return ss.str();
}

std::string              ToLowerCase(std::string str);
std::string              FormatDate(const std::chrono::time_point<std::chrono::system_clock>& time_point);
std::string              StringVectorToString(const std::vector<std::string>& vec, const std::string& separator);
std::vector<std::string> SplitString(const std::string& str, char separator);

} //namespace orch
