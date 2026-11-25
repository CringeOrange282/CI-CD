#pragma once

#include "request.h"

#include <chrono>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

namespace orch {

template <typename T>
class PriorityQueue {
public:
    bool Empty() const {
        const std::lock_guard lock(_mutex);
        return _high_queue.empty() && _low_queue.empty();
    }

    void Push(T value, RequestPriority priority) {
        {
            const std::lock_guard lock(_mutex);
            auto &queue = priority == RequestPriority::High ? _high_queue : _low_queue;
            queue.push(std::move(value));
        }
        _has_tasks.notify_one();
    }

    void Push(const Request &request) requires std::same_as<T, Request> {
        Push(request, request.priority);
    }

    std::optional<T> Pop() {
        const std::lock_guard lock(_mutex);
        return PopUnlocked();
    }

    std::optional<T> WaitAndPop(std::chrono::milliseconds timeout) {
        std::unique_lock lock(_mutex);
        if (!_has_tasks.wait_for(lock, timeout, [this] {
                return !_high_queue.empty() || !_low_queue.empty();
            })) {
            return std::nullopt;
        }
        return PopUnlocked();
    }

    std::size_t sizeTotal() const {
        const std::lock_guard lock(_mutex);
        return _high_queue.size() + _low_queue.size();
    }

    std::size_t sizeHigh() const {
        const std::lock_guard lock(_mutex);
        return _high_queue.size();
    }

    std::size_t sizeLow() const {
        const std::lock_guard lock(_mutex);
        return _low_queue.size();
    }

private:
    std::optional<T> PopUnlocked() {
        auto &queue = _high_queue.empty() ? _low_queue : _high_queue;
        if (queue.empty()) {
            return std::nullopt;
        }
        std::optional<T> value(std::in_place, std::move(queue.front()));
        queue.pop();
        return value;
    }

    mutable std::mutex _mutex;
    std::condition_variable _has_tasks;
    std::queue<T> _high_queue;
    std::queue<T> _low_queue;
};

} // namespace orch
