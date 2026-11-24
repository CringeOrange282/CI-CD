#pragma once

#include "FmsQueue.h"
#include "request.h"


template <typename T>
class PriorityQueue {
private:
    fms::FmsQueue<T> low_queue;
    fms::FmsQueue<T> high_queue;
public:
    bool Empty()const;
    void Push(const Request&);
    std::optional<Request> Pop();
    std::optional<Request> WaitAndPop(std::chrono::milliseconds timeout);
};

template<typename T>
bool PriorityQueue<T>::Empty()const {
    return low_queue.Empty() && high_queue.Empty();
}

template<typename T>
void PriorityQueue<T>::Push(const Request& request) {
    if (request.priority == RequestPriority::High) {
        high_queue.Push(request);
    }
    else {
        low_queue.Push(request);
    }
}

template<typename T>
std::optional<Request> PriorityQueue<T>::Pop() {
    if (auto request = high_queue.Pop()) {
        return request;
    }
    return low_queue.Pop();
}

template<typename T>
std::optional<Request> PriorityQueue<T>::WaitAndPop(std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < timeout) {
        if (auto request = high_queue.Pop()) {
            return request;
        }
        if (auto request = low_queue.Pop()) {
            return request;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return std::nullopt;
}
