#pragma once

#include <string>

namespace orch {

class CalculateRequestMaker;
class ExternalResponseHandler;

bool RunService(const std::string& host, int port,
                CalculateRequestMaker& request_maker,
                ExternalResponseHandler& response_handler);

} //namespace orch
