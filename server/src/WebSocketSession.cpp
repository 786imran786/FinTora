#include "WebSocketSession.hpp"
#include <iostream>

namespace fintora {

WebSocketSession::WebSocketSession(tcp::socket socket, ClientManager& clients, RequestHandler& handler)
    : ws_(std::move(socket)), clients_(clients), handler_(handler) {}

WebSocketSession::~WebSocketSession() {
    clients_.removeClient(shared_from_this());
    std::cout << "Client disconnected (total: " << clients_.clientCount() << ")" << std::endl;
}

void WebSocketSession::run() {
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws_.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
        res.set(beast::http::field::server, "fintora-order-server");
    }));

    ws_.async_accept(
        beast::bind_front_handler(&WebSocketSession::onAccept, shared_from_this()));
}

void WebSocketSession::onAccept(beast::error_code ec) {
    if (ec) {
        std::cerr << "Accept error: " << ec.message() << std::endl;
        return;
    }

    clients_.addClient(shared_from_this());
    std::cout << "Client connected (total: " << clients_.clientCount() << ")" << std::endl;

    sendInitialOrderBook();
    doRead();
}

void WebSocketSession::sendInitialOrderBook() {
    try {
        json bookMsg = handler_.getOrderBookMessage();
        send(bookMsg.dump());
    } catch (const std::exception& e) {
        std::cerr << "Failed to send initial order book: " << e.what() << std::endl;
    }
}

void WebSocketSession::doRead() {
    ws_.async_read(buffer_,
        beast::bind_front_handler(&WebSocketSession::onRead, shared_from_this()));
}

void WebSocketSession::onRead(beast::error_code ec, std::size_t /*bytesTransferred*/) {
    if (ec) {
        if (ec != websocket::error::closed) {
            std::cerr << "Read error: " << ec.message() << std::endl;
        }
        return;
    }

    std::string message = beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());

    try {
        auto result = handler_.handle(message);

        if (!result.directResponse.is_null()) {
            send(result.directResponse.dump());
        }

        for (const auto& broadcast : result.broadcasts) {
            clients_.broadcast(broadcast.dump());
        }
    } catch (const std::exception& e) {
        json errorMsg = Protocol::error(std::string("Server error: ") + e.what());
        send(errorMsg.dump());
    }

    doRead();
}

void WebSocketSession::send(const std::string& message) {
    auto msg = std::make_shared<std::string>(message);

    std::lock_guard<std::mutex> lock(writeMutex_);
    writeQueue_.push_back(msg);

    if (!writing_) {
        writing_ = true;
        doWrite();
    }
}

void WebSocketSession::doWrite() {
    if (writeQueue_.empty()) {
        writing_ = false;
        return;
    }

    auto msg = writeQueue_.front();
    writeQueue_.erase(writeQueue_.begin());

    ws_.text(true);
    ws_.async_write(net::buffer(*msg),
        [self = shared_from_this(), msg](beast::error_code ec, std::size_t bytesTransferred) {
            self->onWrite(ec, bytesTransferred);
        });
}

void WebSocketSession::onWrite(beast::error_code ec, std::size_t /*bytesTransferred*/) {
    if (ec) {
        std::cerr << "Write error: " << ec.message() << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(writeMutex_);
    doWrite();
}

} // namespace fintora
