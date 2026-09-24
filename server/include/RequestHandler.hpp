#pragma once

#include "Protocol.hpp"
#include "EngineAdapter.hpp"
#include "Metrics.hpp"
#include "ClientManager.hpp"

#include <memory>
#include <string>
#include <mutex>

namespace fintora {

class WebSocketSession;

struct HandleResult {
    json directResponse;
    std::vector<json> broadcasts;
};

class RequestHandler {
public:
    RequestHandler(EngineAdapter& engine, Metrics& metrics, ClientManager& clients);

    HandleResult handle(const std::string& rawMessage);

private:
    HandleResult handlePlaceOrder(const json& msg);
    HandleResult handleCancelOrder(const json& msg);
    HandleResult handleGetMetrics();

public:
    json getOrderBookMessage();

    EngineAdapter& engine_;
    Metrics& metrics_;
    ClientManager& clients_;
    std::mutex engineMutex_;
};

} // namespace fintora
