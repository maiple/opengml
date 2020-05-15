#pragma once

#include "Async.hpp"

#include "ogm/common/types.hpp"
#include "ogm/common/error.hpp"
#include "ogm/interpreter/Buffer.hpp"

#include <memory>
#include <map>
#include <cstdlib>

namespace ogm { namespace interpreter {

typedef int32_t socket_id_t;
typedef size_t port_t;

class Buffer;

enum class NetworkProtocol
{
    TCP=0,
    UDP=1,
    BLUETOOTH=2
};

struct Socket;

// event to notify the listener about
// new connection, data received, etc.
struct SocketEvent : public AsyncEvent
{
    // socket the event came from
    socket_id_t m_socket;

    enum Type
    {
        CONNECTION_ACCEPTED,
        CONNECTION_ENDED,
        DATA_RECEIVED,
        NONBLOCKING,
    } m_type;

    SocketEvent(socket_id_t socket_id, async_listener_id_t listener, Type type)
        : AsyncEvent(listener, asset::DynamicSubEvent::OTHER_ASYNC_NETWORK)
        , m_socket(socket_id)
        , m_type(type)
    { }
    
    void produce_info(ds_index_t) override;

    union
    {
        //  only if CONNECTION_ACCEPTED
        socket_id_t m_connected_socket;

        // only if NONBLOCKING
        bool m_success;
    };

    // only if DATA_RECEIVED
    std::shared_ptr<Buffer> m_buffer;
};

// "raw" bein false here means that extra data will be sent automatically to be interpreted
// by OpenGML in order to preserve reliability and/or message-based communication.
class NetworkManager
{
    std::map<socket_id_t, Socket*> m_sockets;
    bool m_init_sockets = false;
    bool m_networking_available = false;
    bool m_config_nonblocking = false;

public:
    socket_id_t create_server_socket(bool raw, NetworkProtocol, port_t, size_t max_client, async_listener_id_t listener);
    socket_id_t create_socket(bool raw, NetworkProtocol, async_listener_id_t);
    socket_id_t create_socket(bool raw, NetworkProtocol, async_listener_id_t, port_t);
    bool connect_socket(socket_id_t, bool raw, const char* url, port_t); // returns true on success or blocking.
    int32_t send(socket_id_t, size_t datac, const char* datav, const char* url=nullptr, port_t port=0);
    void destroy_socket(socket_id_t);

    // receives queued data updates
    void receive(std::vector<std::unique_ptr<AsyncEvent>>& out);

    // flushes all non-raw sockets.
    void flush_send_all();
    
    void set_option(size_t option_index, real_t value);

private:
    void receive_tcp_stream(socket_id_t, Socket* s, size_t datac, const char* datav, std::vector<std::unique_ptr<AsyncEvent>>& out);
    socket_id_t add_receiving_socket(int socket, bool raw, NetworkProtocol np, async_listener_id_t listener);
    void init();

    // attempts to flush send buffer for socket
    // returns number of bytes remaining to be sent.
    // (non-raw only.)
    size_t flush_tcp_send_buffer(Socket* s);
};

}}
