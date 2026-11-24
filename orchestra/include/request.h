#pragma once
#include <string>
#include "json.hpp"

using json = nlohmann::json;

enum class RequestPriority {
    High, //ручной
    Low //автоматический
};
struct Request {
    std::string id;
    std::string author;
    std::string date;
    std::string document;
    std::string name_process;
    RequestPriority priority = RequestPriority::High;
    std::string create_at; //время создания
    Request() = default;
    std::string getDedupKey() const {
      return name_process + "-" + document + "-" + date;
    }
    
    static Request FromJson(const json& json_) {
        if (!json_.is_null() && !json_.empty()) {
            Request request;
            request.id = json_.value("id","");
            request.author = json_.at("author").get<std::string>();
            request.date = json_.at("date").get<std::string>();
            request.document = json_.at("document").get<std::string>();
            request.name_process = json_.at("process").get<std::string>();
            std::string priority = json_.value("priority", "manual");
            request.priority = (priority == "manual") ? RequestPriority::High : RequestPriority::Low;
            request.create_at = json_.value("create_at", "");
            return request;
        }
    }

    json ToJson() {
        return {
        {"id",id},
        {"author", author},
        {"date", date},
        {"document", document},
        {"process", name_process},
        {"priority", (priority == RequestPriority::High ? "manual" : "auto")},
        {"create_at",create_at}
    };
    }
};


