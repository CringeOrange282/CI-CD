#include "Service.h"
#include "CalculateRequestMaker.h"

#include <httplib.h>

namespace orch
{

    bool RunService(const std::string& host, int port,
                    CalculateRequestMaker& request_maker)
    {
        httplib::Server server;
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
            
        });

        if (!server.bind_to_port(host, port)) {
            std::cerr << "Could not bind to " << host << ':' << port << '\n';
            return false;
        }

        std::cout << "Orchestra listening on " << host << ':' << port << std::endl;
        return server.listen_after_bind();
    }
    
} // namespace orch
