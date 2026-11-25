#include "AsyncComponentBase.h"
#include "Utils.h"
#include <stdexcept>

namespace orch {

AsyncComponentBase::~AsyncComponentBase() 
{ 
    StopSync(); 
}

void AsyncComponentBase::Start(RunMode mode, bool wait)
{
    if(_worker)
        THROW_RUNTIME_EXCEPTION("worker thread is already running");
    
    _stopped.store(true, std::memory_order_relaxed);
    _worker = std::make_unique<std::thread>([this, mode] { this->Trampoline(mode); });

    if(wait)
    {
        std::unique_lock<std::mutex> lk(_mut);
        _stopped_cond.wait(lk, [this] { return !this->_stopped.load(std::memory_order_relaxed); });
    }
}

void AsyncComponentBase::StartAsync(RunMode mode) {
    LOG_DEBUG("Starting component asynchronously");
    Start(mode, false);
}

void AsyncComponentBase::StartSync(RunMode mode) {
    LOG_DEBUG("Starting component synchronously");
    Start(mode, true);
    LOG_DEBUG("Component has been started");
}

void AsyncComponentBase::StopSync() {
    StopAsync();
    WaitStop();
}

void AsyncComponentBase::StopAsync() {
    LOG_DEBUG("Stopping component asynchronously");
    if(!_worker) 
        return;
    
    _stopped.store(true, std::memory_order_relaxed);
}

void AsyncComponentBase::WaitStop() {
    LOG_DEBUG("Waiting component stop");
    if(!_worker) 
        return;

    if(_worker->joinable())
        _worker->join();
    else 
        LOG_ERROR("Worker thread unjoinable");
    
    _worker.reset();
    LOG_DEBUG("Component is completely stopped");
}

void AsyncComponentBase::Trampoline(RunMode mode)
{
    {
        std::lock_guard<std::mutex> lk(_mut);
        _stopped.store(false, std::memory_order_relaxed);
    }
    _stopped_cond.notify_one();
    
    try {
        
        do {
            ThreadJob();
        } while(!_stopped.load(std::memory_order_relaxed) && (mode == RunMode::RUN_IN_LOOP));

    } catch(std::runtime_error e) {
        LOG_ERROR("Runtime exception in worker thread: " << e.what());
    } catch(...) {
        LOG_ERROR("Uknown error in worker thread");
    }
}

} // namespace orch
