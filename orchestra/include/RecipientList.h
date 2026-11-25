#pragma once
#include "IRecipientList.h"
#include <unordered_map>
#include <mutex>

class RecipientList : public IRecipientList {
public:
    static RecipientList& GetInstance();

    RecipientList(const RecipientList&) = delete;
    RecipientList& operator=(const RecipientList&) = delete;

    bool AddSubscriber(const std::string& dedup_key, const std::string& author) override;
    bool FindRequest(const std::string& dedup_key) const override;
    std::vector<std::string> GetSubscribers(const std::string& dedup_key) const override;
    void RemoveRequest(const std::string& dedup_key) override;

private:
    RecipientList() = default;
    ~RecipientList() override = default;

    std::unordered_map<std::string, std::vector<std::string>> active_requests_;
    mutable std::mutex mtx_;
};

