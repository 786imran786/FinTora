#include "ClientManager.hpp"
#include "WebSocketSession.hpp"
#include <iostream>

namespace fintora {

void ClientManager::addClient(std::shared_ptr<WebSocketSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.insert(std::move(session));
}

void ClientManager::removeClient(std::shared_ptr<WebSocketSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session);
}

void ClientManager::broadcast(const std::string& message) {
    std::vector<std::shared_ptr<WebSocketSession>> targets;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        targets.assign(sessions_.begin(), sessions_.end());
    }

    for (const auto& session : targets) {
        try {
            session->send(message);
        } catch (const std::exception& e) {
            std::cerr << "Broadcast error: " << e.what() << std::endl;
        }
    }
}

size_t ClientManager::clientCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

} // namespace fintora
