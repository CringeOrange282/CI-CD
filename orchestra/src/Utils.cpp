#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <time.h>
#include <unordered_map>
#include <fstream>
#include <thread>

namespace orch::utils {

std::string ToLowerCase(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return str;
}

std::string GetFileContent(const std::string& file_path)
{
    std::ifstream file(file_path);
    std::stringstream ss;
    ss << file.rdbuf();
    
    return ss.str();
}

bool WaitFor(std::function<bool(void)> terminator, std::chrono::milliseconds timeout)
{
    using namespace std::chrono;
    static constexpr milliseconds WAIT_STEP(SYNC_TIMEOUT_MS);
   
    while (timeout.count() > 0)
    {
        if (terminator())
            return true;

        std::this_thread::sleep_for(WAIT_STEP);
        timeout -= WAIT_STEP;
    }

    return false;
}

std::string FormatDate(const std::chrono::time_point<std::chrono::system_clock>& time_point)
{
    std::time_t ctime = std::chrono::system_clock::to_time_t(time_point);
    std::tm time_info;
    localtime_s(&time_info, &ctime);
    
    std::stringstream ss;
    ss << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string StringVectorToString(const std::vector<std::string>& vec, const std::string& separator)
{
    std::string res;
    for(auto str : vec)
        res += str + separator;

    return res;
}

std::vector<std::string> SplitString(const std::string &str, char separator)
{
    std::vector<std::string> lines;
    std::stringstream ss(str);
    std::string line;

    while (std::getline(ss, line, '\n'))
        lines.push_back(line);

    return lines;
}

} //namespace orch::utils
