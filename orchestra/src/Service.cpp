#include "Service.h"

#include "Queue.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <charconv>
#include <compare>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <utility>

namespace orch {
namespace {

using Json = nlohmann::json;
using OperationId = std::uint64_t;

struct RequestKey {
    std::string operation;
    std::string information_id;
    std::string information_from_date;

    auto operator<=>(const RequestKey&) const = default;
};

struct Operation {
    OperationId id = 0;
    std::set<std::string> users;
    std::condition_variable lookup_finished;
    int lookup_status = 0;
    const char* lookup_error = nullptr;
    std::optional<Json> result;
};

struct Job {
    OperationId id;
    std::string operation;
    Json process_data;
};

struct LookupResult {
    int status;
    const char* error;
    Json data;
};

bool HasString(const Json& body, const char* name)
{
    const auto field = body.find(name);
    return field != body.end() && field->is_string() &&
           !field->get_ref<const std::string&>().empty();
}

void SendJson(httplib::Response& response, int status, const Json& body)
{
    response.status = status;
    response.set_header("Cache-Control", "no-store");
    response.set_content(body.dump(), "application/json");
}

void SendError(httplib::Response& response, int status, const char* message)
{
    SendJson(response, status, {{"error", message}});
}

void SendOperation(httplib::Response& response, const Operation& operation)
{
    if (operation.lookup_error) {
        SendError(response, operation.lookup_status, operation.lookup_error);
    } else if (operation.result) {
        SendJson(response, 200, {
            {"operation_id", operation.id},
            {"status", "completed"},
            {"result", *operation.result},
        });
    } else {
        SendJson(response, 202, {
            {"operation_id", operation.id},
            {"status", "queued"},
            {"message", "OK, your request is in the queue"},
        });
    }
}

std::optional<OperationId> ReadOperationId(const httplib::Request& request)
{
    const auto value = request.matches[1].str();
    OperationId id = 0;
    const auto first = value.data();
    const auto last = first + value.size();
    const auto [end, error] = std::from_chars(first, last, id);
    if (error == std::errc{} && end == last && id != 0) {
        return id;
    }
    return std::nullopt;
}

class Service {
public:
    explicit Service(std::string information_url)
        : information_url_(std::move(information_url))
    {
    }

    void Submit(const httplib::Request& request, httplib::Response& response)
    {
        const auto body = Json::parse(request.body, nullptr, false);
        if (!body.is_object() || body.size() != 4 || !HasString(body, "user") ||
            !HasString(body, "operation") || !HasString(body, "information_id") ||
            !HasString(body, "information_from_date")) {
            SendError(response, 400,
                "Expected non-empty strings for user, operation, information_id, and information_from_date");
            return;
        }

        const RequestKey key{
            body["operation"].get<std::string>(),
            body["information_id"].get<std::string>(),
            body["information_from_date"].get<std::string>(),
        };
        const auto& user = body["user"].get_ref<const std::string&>();

        std::unique_lock lock(mutex_);
        if (const auto existing = requests_.find(key); existing != requests_.end()) {
            const auto operation = existing->second;
            if (!operation->result) {
                operation->users.insert(user);
                operation->lookup_finished.wait(lock, [&] { return operation->lookup_status != 0; });
            }
            SendOperation(response, *operation);
            return;
        }

        auto operation = std::make_shared<Operation>();
        operation->id = next_id_++;
        operation->users.insert(user);
        requests_.emplace(key, operation);
        lock.unlock();

        auto lookup = Lookup(key);

        lock.lock();
        if (lookup.status == 200) {
            try {
                operations_.emplace(operation->id, operation);
                queue_.Push(Job{operation->id, key.operation, std::move(lookup.data)});
                lookup.status = 202;
            } catch (...) {
                operations_.erase(operation->id);
                lookup.status = 500;
                lookup.error = "Could not enqueue the operation";
            }
        }
        if (lookup.error) {
            requests_.erase(key);
        }
        operation->lookup_status = lookup.status;
        operation->lookup_error = lookup.error;
        operation->lookup_finished.notify_all();
        SendOperation(response, *operation);
    }

    void GetOperation(const httplib::Request& request, httplib::Response& response)
    {
        const auto id = ReadOperationId(request);
        std::lock_guard lock(mutex_);
        const auto operation = id ? operations_.find(*id) : operations_.end();
        if (operation == operations_.end()) {
            SendError(response, 404, "Operation not found");
            return;
        }
        SendOperation(response, *operation->second);
    }

    void NextJob(httplib::Response& response)
    {
        std::lock_guard lock(mutex_);
        if (!active_job_) {
            active_job_ = queue_.Pop();
        }
        if (!active_job_) {
            response.status = 204;
            return;
        }
        SendJson(response, 200, {
            {"operation_id", active_job_->id},
            {"operation", active_job_->operation},
            {"process_data", active_job_->process_data},
        });
    }

    void Complete(const httplib::Request& request, httplib::Response& response)
    {
        auto body = Json::parse(request.body, nullptr, false);
        if (!body.is_object() || !body.contains("result")) {
            SendError(response, 400, "Expected a JSON object with a result field");
            return;
        }

        const auto id = ReadOperationId(request);
        std::lock_guard lock(mutex_);
        const auto entry = id ? operations_.find(*id) : operations_.end();
        if (entry == operations_.end()) {
            SendError(response, 404, "Operation not found");
            return;
        }

        auto& operation = *entry->second;
        if (operation.result) {
            if (*operation.result != body["result"]) {
                SendError(response, 409, "Operation already has a different result");
                return;
            }
        } else {
            if (!active_job_ || active_job_->id != operation.id) {
                SendError(response, 409, "Operation is not the active queue item");
                return;
            }
            operation.result = std::move(body["result"]);
            operation.users.clear();
            active_job_.reset();
        }
        SendOperation(response, operation);
    }

private:
    LookupResult Lookup(const RequestKey& key) const
    {
        try {
            httplib::Client client(information_url_);
            if (!client.is_valid()) {
                return {502, "Invalid information service URL", {}};
            }
            client.set_connection_timeout(3);
            client.set_read_timeout(5);
            client.set_write_timeout(5);
            const httplib::Params params{
                {"information_id", key.information_id},
                {"information_from_date", key.information_from_date},
            };
            const auto response = client.Get("/information", params);
            if (!response) {
                if (response.error() == httplib::Error::ConnectionTimeout ||
                    response.error() == httplib::Error::Timeout) {
                    return {504, "Information service timed out", {}};
                }
                return {502, "Could not reach information service", {}};
            }
            if (response->status == 404) {
                return {404, "Information not found", {}};
            }
            if (response->status != 200) {
                return {502, "Information service returned an unexpected status", {}};
            }
            auto data = Json::parse(response->body, nullptr, false);
            if (data.is_discarded()) {
                return {502, "Information service returned invalid JSON", {}};
            }
            return {200, nullptr, std::move(data)};
        } catch (const std::exception&) {
            return {502, "Could not read information service response", {}};
        }
    }

    const std::string information_url_;
    std::mutex mutex_;
    OperationId next_id_ = 1;
    std::map<RequestKey, std::shared_ptr<Operation>> requests_;
    std::map<OperationId, std::shared_ptr<Operation>> operations_;
    Queue<Job> queue_;
    std::optional<Job> active_job_;
};

} // namespace

bool RunService(const std::string& host, int port, const std::string& information_url)
{
    Service service(information_url);
    httplib::Server server;
    server.Post("/requests", [&](const auto& request, auto& response) {
        service.Submit(request, response);
    });
    server.Get(R"(/operations/([0-9]+))", [&](const auto& request, auto& response) {
        service.GetOperation(request, response);
    });
    server.Post("/queue/next", [&](const auto&, auto& response) {
        service.NextJob(response);
    });
    server.Put(R"(/operations/([0-9]+)/result)", [&](const auto& request, auto& response) {
        service.Complete(request, response);
    });

    if (!server.bind_to_port(host, port)) {
        std::cerr << "Could not bind to " << host << ':' << port << '\n';
        return false;
    }
    std::cout << "Orchestra listening on " << host << ':' << port << std::endl;
    return server.listen_after_bind();
}

} // namespace orch
