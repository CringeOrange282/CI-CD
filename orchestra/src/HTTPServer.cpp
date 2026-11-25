#include "Service.h"
#include "CalculateRequestMaker.h"
#include "ExternalResponseHandler.h"

#include <optional>
#include <httplib.h>

namespace orch
{

    bool RunService(const std::string& host, int port,
                    CalculateRequestMaker& request_maker,
                    ExternalResponseHandler& response_handler,
                    const PriorityQueue<ProcessTask>& process_queue)
    {
        httplib::Server server;
        // Разрешаем запросы из браузера (CORS)
        server.set_default_headers({
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
            {"Access-Control-Allow-Headers", "Content-Type"}
        });

        // Обработка предварительных OPTIONS-запросов браузера
        server.Options(".*", [](const auto&, auto& res) {
            res.status = 204;
        });
        server.Get("/queue", [&](const auto&, auto& res) {
            const auto keys = process_queue.Snapshot(&ProcessTask::dedup_key);
            const json response = {{"keys", keys}};
            res.set_header("Cache-Control", "no-store");
            res.set_content(response.dump(), "application/json");
        });

        server.Post("/add_task", [&](const auto& req, auto& res) {
            Request task_req;
            try {
                task_req = Request::FromJson(json::parse(req.body));
            } catch (const json::exception&) {
                res.status = 400;
                const json response = {
                    {"enqueued", false}, {"status", "invalid_request"}};
                res.set_content(response.dump(), "application/json");
                return;
            }

            res.set_content(request_maker.ProcessRequest(task_req).dump(),
                            "application/json");
        });

        server.Post("/result", [&](const auto& req, auto& res) {
            json result;
            std::string key;
            std::optional<std::string> attempt_id;
            std::optional<std::string> error;
            try {
                result = json::parse(req.body);
                key = result.at("key").get<std::string>();
                if (result.contains("attempt_id")) {
                    attempt_id = result.at("attempt_id").get<std::string>();
                }
                const auto status = result.at("status").get<std::string>();
                if (key.empty() || status.empty() ||
                    (attempt_id && attempt_id->empty())) {
                    res.status = httplib::StatusCode::BadRequest_400;
                    return;
                }
                if (status == "failed") {
                    error = result.at("error").get<std::string>();
                }
            } catch (const json::exception&) {
                res.status = httplib::StatusCode::BadRequest_400;
                return;
            }

            if (error) {
                response_handler.HandleExternalError(key, attempt_id, *error);
            } else {
                response_handler.HandleExternalSuccess(key, attempt_id, result);
            }
            res.status = httplib::StatusCode::OK_200;
        });

        if (!server.bind_to_port(host, port)) {
            std::cerr << "Could not bind to " << host << ':' << port << '\n';
            return false;
        }

        std::cout << "Orchestra listening on " << host << ':' << port << std::endl;
        return server.listen_after_bind();
    }
    
} // namespace orch
