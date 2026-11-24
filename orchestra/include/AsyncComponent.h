#pragma once

#include <memory>

namespace orch {

class AsyncComponent {
public:
    using Ptr = std::unique_ptr<AsyncComponent>;

    enum class RunMode {
        RUN_IN_LOOP,
        RUN_ONCE
    };

    virtual ~AsyncComponent() {}

    virtual void Init()          = 0;
    virtual void StartSync(RunMode mode)    = 0;
    virtual void StartAsync(RunMode mode)   = 0;
    virtual void StopSync()                 = 0;
    virtual void StopAsync()                = 0;
    virtual void WaitStop()                 = 0;

};
} //namespace orch
