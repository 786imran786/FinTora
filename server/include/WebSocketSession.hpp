#pragma once

#include "ClientManager.hpp"
#include "RequestHandler.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <string>

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

private:
    void onAccept(beast::error_code ec);
    void doRead();
    void onRead(beast::error_code ec, std::size_t bytesTransferred);
    void onWrite(beast::error_code ec, std::size_t bytesTransferred);
    void sendInitialOrderBook();

    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    ClientManager& clients_;
    RequestHandler& handler_;
    std::vector<std::shared_ptr<std::string>> writeQueue_;
    std::mutex writeMutex_;
    bool writing_ = false;
    void doWrite();
};

} // namespace fintora
