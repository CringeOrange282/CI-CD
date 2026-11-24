#pragma once
#include <string>
#include "json.hpp"

namespace orch {
    enum class RequestPriority {
        High, //ручной
        Low //автоматический
    };
    struct Request {
        using json = nlohmann::json;
        std::string id;
        std::string author;
        std::string date;
        std::string document;
        std::string name_process;
        RequestPriority priority;
        std::string create_at; //время создания
        Request();
        Request(std::string, std::string, std::string, std::string, 
            std::string, RequestPriority, std::string);

        static Request FromJson(const json&);
        json ToJson();
    };
}
