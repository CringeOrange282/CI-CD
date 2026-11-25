#include "RecipientList.h"
#include <algorithm>

RecipientList& RecipientList::GetInstance() {
    static RecipientList instance;
    return instance;
}

bool RecipientList::AddSubscriber(const std::string& dedup_key, const std::string& author) {
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = active_requests_.find(dedup_key);
    if (it == active_requests_.end()) {
        active_requests_[dedup_key] = { author };
        return true;
    }

    auto& subscribers = it->second;
    if (std::find(subscribers.begin(), subscribers.end(), author) == subscribers.end()) {
        subscribers.push_back(author);
    }
    return false;
}

bool RecipientList::FindRequest(const std::string& dedup_key) const {
    std::lock_guard<std::mutex> lock(mtx_);
    return active_requests_.find(dedup_key) != active_requests_.end();
}

std::vector<std::string> RecipientList::GetSubscribers(const std::string& dedup_key) const {
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = active_requests_.find(dedup_key);
    if (it != active_requests_.end()) {
        return it->second;
    }
    return {};
}

void RecipientList::RemoveRequest(const std::string& dedup_key) {
    std::lock_guard<std::mutex> lock(mtx_);
    active_requests_.erase(dedup_key);
}

