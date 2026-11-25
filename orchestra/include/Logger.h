#pragma once

#include <string>
#include <cinttypes>

namespace orch {

class Logger {
public:
    enum class LogLevel : uint8_t {
        TRACE = 0,
        DEBUG,
        INFO,
        WARN,
        ERR
    };
    
    Logger() {}
    explicit Logger(const std::string& name) : _name(name) {}

    static void         SetLogLevel(LogLevel level);
    static LogLevel     GetLogLevel() { return _level; }
    
    void                PrintMessage(LogLevel level, const std::string& message) const;
    const std::string   GetName() const { return _name; }

private:    
    static std::string  LogLevelToString(LogLevel level);

    static LogLevel     _level;
    const std::string   _name;
};

} //namespace orch
