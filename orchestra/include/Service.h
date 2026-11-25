#pragma once

#include <string>

namespace orch {

class CalculateRequestMaker;
class ExternalResponseHandler;
struct ProcessTask;
template <typename T> class PriorityQueue;

bool RunService(const std::string& host, int port,
                CalculateRequestMaker& request_maker,
                ExternalResponseHandler& response_handler,
                const PriorityQueue<ProcessTask>& process_queue);

} //namespace orch
