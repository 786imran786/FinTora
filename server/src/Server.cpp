#include "Server.hpp"
#include "WebSocketSession.hpp"
#include <iostream>
#include <thread>

namespace fintora {

Server::Server(uint16_t port)
    : acceptor_(ioc_, tcp::endpoint(tcp::v4(), port))
    , handler_(engine_, metrics_, clients_) {
    std::cout << "Server started on port " << port << std::endl;
}

void Server::run() {
    doAccept();

    auto threadCount = std::max<unsigned>(1, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;
    for (unsigned i = 1; i < threadCount; ++i) {
        threads.emplace_back([this] { ioc_.run(); });
    }

    ioc_.run();

    for (auto& t : threads) {
        t.join();
    }
}

void Server::stop() {
    ioc_.stop();
}

void Server::doAccept() {
    acceptor_.async_accept(
        [this](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto session = std::make_shared<WebSocketSession>(
                    std::move(socket), clients_, handler_);
                session->run();
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }
            doAccept();
        });
}

} // namespace fintora
