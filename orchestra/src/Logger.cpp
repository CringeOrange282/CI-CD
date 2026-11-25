#include "Logger.h"

#include "Utils.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unistd.h>

namespace orch {
    
    Logger::LogLevel Logger::_level = Logger::LogLevel::INFO;

    inline std::string Logger::LogLevelToString(LogLevel level)
    {
        static const std::unordered_map<LogLevel, const char*> map
        {
            { LogLevel::TRACE, "[T]" },
            { LogLevel::DEBUG, "[D]" },
            { LogLevel::INFO,  "[I]" },
            { LogLevel::WARN,  "[W]" },
            { LogLevel::ERR,   "[E]" }
        };
        return map.find(level)->second;
    }

    void Logger::SetLogLevel(LogLevel level) 
    { 
        _level = level; 
    }

    void Logger::PrintMessage(LogLevel level, const std::string& message) const
    {
        auto now = std::chrono::system_clock::now();
        pid_t pid = getpid();

        std::stringstream ss;
        ss << utils::StringFromParts(
            '[', utils::FormatDate(now), "] ", 
            pid, ' ', 
            LogLevelToString(level), ' ', 
            _name, ": ", message
        );
        ss << std::endl;
            
        std::string output = ss.str();
        std::cout.write(output.c_str(), output.size());
    }

} // namespace orch
