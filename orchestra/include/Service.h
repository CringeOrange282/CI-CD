#pragma once

#include <string>

namespace orch {

class CalculateRequestMaker;

bool RunService(const std::string& host, int port,
                CalculateRequestMaker& request_maker);

} //namespace orch
