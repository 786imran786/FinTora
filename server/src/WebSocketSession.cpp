#include "WebSocketSession.hpp"
#include <iostream>

namespace fintora {

WebSocketSession::WebSocketSession(tcp::socket socket, ClientManager& clients, RequestHandler& handler)
    : ws_(std::move(socket)), clients_(clients), handler_(handler) {}

WebSocketSession::~WebSocketSession() = default;

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
    if (closed_) return;

    ws_.async_read(buffer_,
        beast::bind_front_handler(&WebSocketSession::onRead, shared_from_this()));
}

void WebSocketSession::onRead(beast::error_code ec, std::size_t /*bytesTransferred*/) {
    if (ec) {
        if (ec != websocket::error::closed &&
            ec != net::error::operation_aborted &&
            ec != net::error::connection_reset &&
            ec != net::error::connection_aborted) {
            std::cerr << "Read error: " << ec.message() << std::endl;
        }
        disconnect();
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
    if (closed_) return;

    auto msg = std::make_shared<const std::string>(message);

    net::post(
        ws_.get_executor(),
        beast::bind_front_handler(
            &WebSocketSession::onSend,
            shared_from_this(),
            msg));
}

void WebSocketSession::onSend(std::shared_ptr<const std::string> message) {
    if (closed_) return;

    writeQueue_.push_back(message);

    if (writeQueue_.size() > 1) {
        return;
    }

    ws_.text(true);
    ws_.async_write(
        net::buffer(*writeQueue_.front()),
        beast::bind_front_handler(
            &WebSocketSession::onWrite,
            shared_from_this()));
}

void WebSocketSession::onWrite(beast::error_code ec, std::size_t /*bytesTransferred*/) {
    if (ec) {
        if (ec != websocket::error::closed &&
            ec != net::error::operation_aborted &&
            ec != net::error::connection_reset &&
            ec != net::error::connection_aborted) {
            std::cerr << "Write error: " << ec.message() << std::endl;
        }
        disconnect();
        return;
    }

    if (closed_) return;

    writeQueue_.erase(writeQueue_.begin());

    if (!writeQueue_.empty()) {
        ws_.text(true);
        ws_.async_write(
            net::buffer(*writeQueue_.front()),
            beast::bind_front_handler(
                &WebSocketSession::onWrite,
                shared_from_this()));
    }
}

void WebSocketSession::disconnect() {
    bool expected = false;
    if (closed_.compare_exchange_strong(expected, true)) {
        clients_.removeClient(shared_from_this());
        beast::error_code ec;
        ws_.next_layer().socket().close(ec);
        std::cout << "Client disconnected (total: " << clients_.clientCount() << ")" << std::endl;
    }
}

} // namespace fintora
