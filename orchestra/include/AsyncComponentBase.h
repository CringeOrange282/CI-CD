#pragma once

#include "AsyncComponent.h"
#include "Loggable.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <utility>
#include <thread>

namespace orch {

class AsyncComponentBase : public AsyncComponent, public Loggable {
public:
    explicit AsyncComponentBase(Logger&& logger) : Loggable(std::move(logger)) {}
    AsyncComponentBase(const AsyncComponentBase& component)             = delete;
    AsyncComponentBase& operator=(const AsyncComponentBase& component)  = delete;
    ~AsyncComponentBase();

    void Init()                                                 override {}
    void StartSync(RunMode mode = RunMode::RUN_IN_LOOP)         override;
    void StartAsync(RunMode mode = RunMode::RUN_IN_LOOP)        override;
    void StopSync()                                             override;
    void StopAsync()                                            override;
    void WaitStop()                                             override;


protected:
    virtual void ThreadJob() = 0;
    virtual void Start(RunMode mode, bool wait);

    bool IsStopped() 
    {
        return _stopped.load(std::memory_order_relaxed);
    }

private:
    void Trampoline(RunMode mode);

    std::unique_ptr<std::thread>    _worker     { nullptr };
    std::mutex                      _mut;
    std::condition_variable         _stopped_cond;
    std::atomic_bool                _stopped    { true };
};
} //namespace orch
