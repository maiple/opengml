#pragma once

#include "ogm/common/error.hpp"

#include <map>
#include <cstdlib>

namespace ogm { namespace interpreter {

typedef int32_t socket_id_t;
typedef size_t port_t;

enum class NetworkProtocol
{
    UDP=0,
    TCP=1,
    BLUETOOTH=2
};

struct Socket;

typedef int64_t network_listener_id_t;

// event to notify the listener about
// new connection, data received, etc.
struct SocketEvent
{
    // socket the event came from
    socket_id_t m_socket;

    // destination listener.
    network_listener_id_t m_listener;

    enum Type
    {
        CONNECTION_ACCEPTED,
        CONNECTION_ENDED,
        DATA_RECEIVED
    } m_type;

    SocketEvent(socket_id_t socket, network_listener_id_t listener, Type type)
        : m_socket(socket)
        , m_listener(listener)
        , m_type(type)
    { }

    union
    {
        socket_id_t m_connected_socket;
        size_t m_data_len;
    };

    char* m_data = nullptr;

    ~SocketEvent()
    {
        if (m_data) delete[] m_data;
    }
};

class NetworkManager
{
    std::map<socket_id_t, Socket*> m_sockets;
    bool m_init_sockets = false;
    bool m_networking_available = false;

public:
    socket_id_t create_server_socket(bool raw, NetworkProtocol, port_t, size_t max_client, network_listener_id_t listener);
    socket_id_t create_socket(bool raw, NetworkProtocol, network_listener_id_t);
    socket_id_t create_socket(bool raw, NetworkProtocol, network_listener_id_t, port_t);
    bool connect_socket(socket_id_t, const char* url, port_t);
    size_t send_raw(socket_id_t, size_t datac, const char* datav, const char* url=nullptr, port_t port=0);
    void destroy_socket(socket_id_t);

    // receives queued data updates
    void receive(std::vector<SocketEvent>& out);

private:
    socket_id_t add_receiving_socket(int socket, NetworkProtocol np, network_listener_id_t listener);
    void init();
};

}}
