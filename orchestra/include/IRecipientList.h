#pragma once
#include <string>
#include <vector>

class IRecipientList {
public:
    virtual ~IRecipientList() = default;

    virtual bool addSubscriber(const std::string& dedupKey, const std::string& author) = 0;
    virtual std::vector<std::string> getSubscribers(const std::string& dedupKey) const = 0;
    virtual void removeSubscriber(const std::string& dedupKey, const std::string& author) = 0;
    virtual void removeRequest(const std::string& dedupKey) = 0;
};