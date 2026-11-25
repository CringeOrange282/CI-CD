#pragma once
#include "IRecipientList.h"
#include <unordered_map>
#include <mutex>

class RecipientList : public IRecipientList {
public:
    static RecipientList& getInstance();

    RecipientList(const RecipientList&) = delete;
    RecipientList& operator=(const RecipientList&) = delete;

    bool addSubscriber(const std::string& dedupKey, const std::string& author) override;
    std::vector<std::string> getSubscribers(const std::string& dedupKey) const override;
    void removeSubscriber(const std::string& dedupKey, const std::string& author) override;
    void removeRequest(const std::string& dedupKey) override;

private:
    RecipientList() = default;
    ~RecipientList() override = default;

    std::unordered_map<std::string, std::vector<std::string>> activeRequests_;
    mutable std::mutex mtx_;
};