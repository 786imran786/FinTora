#pragma once

#include "ClientManager.hpp"
#include "RequestHandler.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <string>
#include <vector>
#include <atomic>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace fintora {

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    WebSocketSession(tcp::socket socket, ClientManager& clients, RequestHandler& handler);
    ~WebSocketSession();

    void run();
    void send(const std::string& message);
    void disconnect();

private:
    void onAccept(beast::error_code ec);
    void doRead();
    void onRead(beast::error_code ec, std::size_t bytesTransferred);
    void onSend(std::shared_ptr<const std::string> message);
    void onWrite(beast::error_code ec, std::size_t bytesTransferred);
    void sendInitialOrderBook();

    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    ClientManager& clients_;
    RequestHandler& handler_;
    std::vector<std::shared_ptr<const std::string>> writeQueue_;
    std::atomic<bool> closed_{false};
};

} // namespace fintora
