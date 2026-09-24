#pragma once

#include "ClientManager.hpp"
#include "RequestHandler.hpp"
#include "EngineAdapter.hpp"
#include "Metrics.hpp"

#include <boost/asio.hpp>
#include <cstdint>
#include <memory>

namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace fintora {

class Server {
public:
    Server(uint16_t port);

    void run();
    void stop();

private:
    void doAccept();

    net::io_context ioc_;
    tcp::acceptor acceptor_;
    EngineAdapter engine_;
    Metrics metrics_;
    ClientManager clients_;
    RequestHandler handler_;
};

} // namespace fintora
