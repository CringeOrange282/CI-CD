#include "request.h"

namespace orch {
    Request::Request():priority(RequestPriority::High){}

    Request::Request(std::string id_, std::string author_,
        std::string date_, std::string document_, std::string name_process_,
        RequestPriority priority_, std::string create_at_):
        id(id_),author(author_),date(date_),document(document_),name_process(name_process_),
        priority(priority_),create_at(create_at_){}


    Request Request::FromJson(const json& json_) {
        Request request;
        request.id = json_.value("id", "");
        request.author = json_.at("author").get<std::string>();
        request.date = json_.at("date").get<std::string>();
        request.document = json_.at("document").get<std::string>();
        request.name_process = json_.at("process").get<std::string>();
        std::string priority = json_.value("priority", "manual");
        request.priority = (priority == "manual") ? RequestPriority::High : RequestPriority::Low;
        request.create_at = json_.value("create_at", "");
        return request;
    }

    Request::json Request::ToJson() {
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
}
