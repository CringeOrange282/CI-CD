#include "RecipientList.h"
#include <algorithm>

RecipientList& RecipientList::getInstance() {
    static RecipientList instance;
    return instance;
}

bool RecipientList::addSubscriber(const std::string& dedupKey, const std::string& author) {
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = activeRequests_.find(dedupKey);
    if (it == activeRequests_.end()) {
        activeRequests_[dedupKey] = { author };
        return true;
    }

    auto& subscribers = it->second;
    if (std::find(subscribers.begin(), subscribers.end(), author) == subscribers.end()) {
        subscribers.push_back(author);
    }
    return false;
}

std::vector<std::string> RecipientList::getSubscribers(const std::string& dedupKey) const {
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = activeRequests_.find(dedupKey);
    if (it != activeRequests_.end()) {
        return it->second;
    }
    return {};
}

void RecipientList::removeSubscriber(const std::string& dedupKey, const std::string& author) {
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = activeRequests_.find(dedupKey);
    if (it != activeRequests_.end()) {
        auto& subscribers = it->second;
        subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), author), subscribers.end());

        if (subscribers.empty()) {
            activeRequests_.erase(it);
        }
    }
}

void RecipientList::removeRequest(const std::string& dedupKey) {
    std::lock_guard<std::mutex> lock(mtx_);
    activeRequests_.erase(dedupKey);
}