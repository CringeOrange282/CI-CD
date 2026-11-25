#pragma once

#include "AsyncComponentBase.h"
#include "Interfaces.h"
#include "Queue.h"

#include <httplib.h>
#include <string>
#include <vector>

namespace orch {

class Notifier : public AsyncComponentBase, public IUserNotifier {
public:
  Notifier(Logger &&logger, const std::string &origin, std::string endpoint);
  ~Notifier() override;

  void StartAsync(RunMode mode = RunMode::RUN_IN_LOOP) override;
  void Notify(const std::string &author, const json &response_payload) override;
  void NotifyBatch(const std::vector<std::string> &authors,
                   const json &response_payload) override;

protected:
  void ThreadJob() override;

private:
  struct Notification {
    std::vector<std::string> authors;
    json payload;
  };

  Queue<Notification> _notifications;
  httplib::Client _client;
  std::string _endpoint;
};

} // namespace orch
