#include "client/net/include/websocket_client.hpp"

#include <string>
#include <utility>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

namespace kfc::net {
namespace {

using WsClient = websocketpp::client<websocketpp::config::asio_client>;
using ConnectionHdl = websocketpp::connection_hdl;

}  // namespace

struct WebSocketClient::Impl {
    WsClient endpoint;
    ConnectionHdl hdl;
    bool open = false;
    std::function<void(const std::string&)> onMessage;
    std::function<void()> onOpen;
};

WebSocketClient::WebSocketClient(const std::string& host, std::uint16_t port)
    : impl_(std::make_unique<Impl>()) {
    WsClient& endpoint = impl_->endpoint;
    endpoint.clear_access_channels(websocketpp::log::alevel::all);
    endpoint.clear_error_channels(websocketpp::log::elevel::all);
    endpoint.init_asio();

    endpoint.set_open_handler([this](ConnectionHdl hdl) {
        impl_->hdl = hdl;
        impl_->open = true;
        if (impl_->onOpen) impl_->onOpen();
    });
    endpoint.set_close_handler([this](ConnectionHdl) { impl_->open = false; });
    endpoint.set_fail_handler([this](ConnectionHdl) { impl_->open = false; });
    endpoint.set_message_handler(
        [this](ConnectionHdl, WsClient::message_ptr message) {
            if (impl_->onMessage) impl_->onMessage(message->get_payload());
        });

    const std::string uri = "ws://" + host + ":" + std::to_string(port);
    websocketpp::lib::error_code ec;
    WsClient::connection_ptr connection = endpoint.get_connection(uri, ec);
    if (!ec) endpoint.connect(connection);
}

WebSocketClient::~WebSocketClient() = default;

void WebSocketClient::onMessage(std::function<void(const std::string&)> handler) {
    impl_->onMessage = std::move(handler);
}

void WebSocketClient::onOpen(std::function<void()> handler) {
    impl_->onOpen = std::move(handler);
}

void WebSocketClient::poll() { impl_->endpoint.poll(); }

void WebSocketClient::send(const std::string& message) {
    if (!impl_->open) return;  // not connected yet: drop, the next state corrects
    websocketpp::lib::error_code ec;
    impl_->endpoint.send(impl_->hdl, message, websocketpp::frame::opcode::text,
                         ec);
}

}  // namespace kfc::net
