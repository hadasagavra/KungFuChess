#include "server/net/include/websocket_server.hpp"

#include <map>
#include <utility>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace kfc::server {
namespace {

using WsServer = websocketpp::server<websocketpp::config::asio>;
using ConnectionHdl = websocketpp::connection_hdl;

}  // namespace

// A connection_hdl is a weak_ptr, so it is kept in maps ordered by owner_less.
// Two maps give both directions: hdl -> id to name an incoming message's sender,
// id -> hdl to address an outgoing one.
struct WebSocketServer::Impl {
    WsServer endpoint;
    std::map<ConnectionHdl, ClientId, std::owner_less<ConnectionHdl>> idByHdl;
    std::map<ClientId, ConnectionHdl> hdlById;
    ClientId nextId = 1;
    std::function<void(ClientId)> onConnected;
    std::function<void(ClientId, const std::string&)> onMessage;
    std::function<void(ClientId)> onDisconnected;
};

WebSocketServer::WebSocketServer(std::uint16_t port)
    : impl_(std::make_unique<Impl>()) {
    WsServer& endpoint = impl_->endpoint;
    // We do our own reporting; silence websocketpp's own logging.
    endpoint.clear_access_channels(websocketpp::log::alevel::all);
    endpoint.clear_error_channels(websocketpp::log::elevel::all);
    endpoint.init_asio();
    endpoint.set_reuse_addr(true);

    endpoint.set_open_handler([this](ConnectionHdl hdl) {
        const ClientId id = impl_->nextId++;
        impl_->idByHdl[hdl] = id;
        impl_->hdlById[id] = hdl;
        if (impl_->onConnected) impl_->onConnected(id);
    });
    endpoint.set_close_handler([this](ConnectionHdl hdl) {
        const auto found = impl_->idByHdl.find(hdl);
        if (found == impl_->idByHdl.end()) return;
        const ClientId id = found->second;
        impl_->hdlById.erase(id);
        impl_->idByHdl.erase(found);
        // Report after the maps are cleared, so a handler that sends sees the
        // client as already gone rather than sending to a dead connection.
        if (impl_->onDisconnected) impl_->onDisconnected(id);
    });
    endpoint.set_message_handler(
        [this](ConnectionHdl hdl, WsServer::message_ptr message) {
            const auto found = impl_->idByHdl.find(hdl);
            if (found != impl_->idByHdl.end() && impl_->onMessage) {
                impl_->onMessage(found->second, message->get_payload());
            }
        });

    endpoint.listen(port);
    endpoint.start_accept();
}

WebSocketServer::~WebSocketServer() = default;

void WebSocketServer::onClientConnected(std::function<void(ClientId)> handler) {
    impl_->onConnected = std::move(handler);
}

void WebSocketServer::onMessage(
    std::function<void(ClientId, const std::string&)> handler) {
    impl_->onMessage = std::move(handler);
}

void WebSocketServer::onClientDisconnected(std::function<void(ClientId)> handler) {
    impl_->onDisconnected = std::move(handler);
}

void WebSocketServer::poll() { impl_->endpoint.poll(); }

void WebSocketServer::send(ClientId client, const std::string& message) {
    const auto found = impl_->hdlById.find(client);
    if (found == impl_->hdlById.end()) return;
    websocketpp::lib::error_code ec;
    impl_->endpoint.send(found->second, message,
                         websocketpp::frame::opcode::text, ec);
}

void WebSocketServer::broadcast(const std::string& message) {
    for (const auto& [id, hdl] : impl_->hdlById) {
        websocketpp::lib::error_code ec;
        impl_->endpoint.send(hdl, message, websocketpp::frame::opcode::text, ec);
    }
}

}  // namespace kfc::server
