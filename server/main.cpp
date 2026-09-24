#include "Server.hpp"
#include <cstdlib>
#include <iostream>
#include <csignal>

static std::unique_ptr<fintora::Server> gServer;

void signalHandler(int) {
    if (gServer) {
        gServer->stop();
    }
}

int main(int argc, char* argv[]) {
    uint16_t port = 8080;

    if (argc >= 2) {
        try {
            int p = std::stoi(argv[1]);
            if (p > 0 && p <= 65535) {
                port = static_cast<uint16_t>(p);
            } else {
                std::cerr << "Invalid port: " << argv[1] << std::endl;
                return 1;
            }
        } catch (...) {
            std::cerr << "Invalid port: " << argv[1] << std::endl;
            return 1;
        }
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        gServer = std::make_unique<fintora::Server>(port);
        gServer->run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
