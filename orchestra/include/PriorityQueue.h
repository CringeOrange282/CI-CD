#pragma once

#include "Queue.h"
#include "request.h"

namespace orch{
template <typename T>
class PriorityQueue {
private:
    Queue<T> low_queue;
    Queue<T> high_queue;
public:
    bool Empty()const;
    void Push(const Request&);
    std::optional<Request> Pop();
    std::optional<Request> WaitAndPop(std::chrono::milliseconds timeout);
    
    size_t sizeTotal()const;
    size_t sizeHigh()const;
    size_t sizeLow()const;
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

template<typename T>
size_t PriorityQueue<T>::sizeTotal()const {
    return high_queue.Size() + low_queue.Size();
}

template<typename T>
size_t PriorityQueue<T>::sizeHigh()const {
    return high_queue.Size();
}

template<typename T>
size_t PriorityQueue<T>::sizeLow()const {
    return low_queue.Size();
}
}
