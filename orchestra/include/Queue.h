#pragma once 

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <optional>

namespace orch {

template<class T>
class Queue {
public:
    using Ptr = std::unique_ptr<Queue<T>>;

    void                Push(const T& value);
    void                Push(T&& value);
    bool                WaitAndPop(T& value, std::chrono::milliseconds timeout);
    std::optional<T>    WaitAndPop(std::chrono::milliseconds timeout);
    bool                Pop(T& value);
    std::optional<T>    Pop();
    bool                Empty() const;
    size_t              Size()  const;
    void                Clear();
private:
 mutable std::mutex         _mut;
 std::queue<T>              _data_queue;
 std::condition_variable    _data_cond;
};

template <class T>
inline void Queue<T>::Push(T&& value)
{
    bool was_empty = false;
    {
        std::lock_guard<std::mutex> lk(_mut);
        was_empty = _data_queue.empty();
        _data_queue.push(std::move(value));
    }
    if (was_empty)
        _data_cond.notify_one();
}


template <class T>
inline void Queue<T>::Push(const T &value)
{
    bool was_empty = false;
    {
        std::lock_guard<std::mutex> lk(_mut);
        was_empty = _data_queue.empty();
        _data_queue.push(value);
    }
    if(was_empty)
        _data_cond.notify_one();
}

template <class T>
inline bool Queue<T>::WaitAndPop(T &value, std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lk(_mut);
    if(!_data_cond.wait_for(lk, timeout, [this]() { return !(this->_data_queue.empty()); })) {
        return false;
    }  
    value = std::move(_data_queue.front());
    _data_queue.pop();
    return true;
}

template <class T>
inline std::optional<T> Queue<T>::WaitAndPop(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lk(_mut);
    if(!_data_cond.wait_for(lk, timeout, [this]() { return !(this->_data_queue.empty()); })) {
        return std::nullopt;
    }  
    std::optional<T> res{ std::move(_data_queue.front()) };
    _data_queue.pop();

    return res;
}

template <class T>
inline bool Queue<T>::Pop(T &value)
{
    std::lock_guard<std::mutex> lk(_mut);
    if(_data_queue.empty()) {
        return false;
    }
    value = std::move(_data_queue.front());
    _data_queue.pop();
    return true;
}

template <class T>
inline std::optional<T> Queue<T>::Pop()
{
    std::lock_guard<std::mutex> lk(_mut);
    if(_data_queue.empty()) {
        return std::nullopt;
    }

    std::optional<T> res{ std::move(_data_queue.front()) };
    _data_queue.pop();
    return res;
}

template <class T>
inline bool Queue<T>::Empty() const
{
    std::lock_guard<std::mutex> lk(_mut);
    return _data_queue.empty();
}

template <class T>
inline size_t Queue<T>::Size() const
{
    std::lock_guard<std::mutex> lk(_mut);
    return _data_queue.size();
}

template <class T>
inline void Queue<T>::Clear()
{
    std::lock_guard<std::mutex> lk(_mut);
    for(size_t i = 0; i < _data_queue.size(); i++)
        _data_queue.pop();
}

} // namespace orch
