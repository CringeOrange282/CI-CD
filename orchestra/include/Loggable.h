#pragma once

#include <iostream>
#include <sstream>
#include <utility>

#include "Logger.h"

#define LOG_HELPER(level_, ...) \
    do { \
        if (Logger::GetLogLevel() <= level_) \
        { \
            std::stringstream ss; \
            ss << __VA_ARGS__; \
            _logger.PrintMessage(level_, ss.str()); \
        } \
    } while (false)

#define LOG_TRACE(...) LOG_HELPER(Logger::LogLevel::TRACE, __VA_ARGS__)
#define LOG_DEBUG(...) LOG_HELPER(Logger::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  LOG_HELPER(Logger::LogLevel::INFO,  __VA_ARGS__)
#define LOG_WARN(...)  LOG_HELPER(Logger::LogLevel::WARN,  __VA_ARGS__)
#define LOG_ERROR(...) LOG_HELPER(Logger::LogLevel::ERR,   __VA_ARGS__)

#define FALLBACK_LOGGER(...) \
        do { \
            Logger _logger; \
            LOG_ERROR(__VA_ARGS__); \
        } while(false)

namespace orch {

class Loggable {
public:
    explicit Loggable(const Logger& logger) : _logger(logger) {}
    explicit Loggable(Logger&& logger) : _logger(std::move(logger)) {}

    static Logger CreateChildLogger(const Logger& parent, const std::string& child_name)
    {
        return Logger(parent.GetName() + '.' + child_name); 
    }

    Logger CreateChildLogger(const std::string& child_name) 
    {
        return CreateChildLogger(_logger, child_name);  
    }
    
protected:
    Logger _logger;
};

} //namespace orch
