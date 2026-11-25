#pragma once
#include <string>
#include <vector>

class IRecipientList {
public:
    virtual ~IRecipientList() = default;

    virtual bool AddSubscriber(const std::string& dedup_key, const std::string& author) = 0;
    virtual bool FindRequest(const std::string& dedup_key) const = 0;
    virtual std::vector<std::string> GetSubscribers(const std::string& dedup_key) const = 0;
    virtual void RemoveRequest(const std::string& dedup_key) = 0;
};

