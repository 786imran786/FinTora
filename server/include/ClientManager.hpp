#pragma once

#include <memory>
#include <unordered_set>
#include <mutex>
#include <string>
#include <functional>

namespace fintora {

class WebSocketSession;

class ClientManager {
public:
    void addClient(std::shared_ptr<WebSocketSession> session);
    void removeClient(std::shared_ptr<WebSocketSession> session);
    void broadcast(const std::string& message);
    size_t clientCount() const;

private:
    mutable std::mutex mutex_;
    std::unordered_set<std::shared_ptr<WebSocketSession>> sessions_;
};

} // namespace fintora
