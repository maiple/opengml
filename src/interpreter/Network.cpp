#include "ogm/common/util.hpp"
#include "ogm/interpreter/Network.hpp"

#ifdef NETWORKING_ENABLED
#ifdef _WIN32
#include <WinSock2.h>
#include <ws2def.h>
#include <WS2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#endif

#include <iostream>

namespace ogm { namespace interpreter
{
    #ifdef NETWORKING_ENABLED
    addrinfo hints_tcp;
    addrinfo hints_udp;

    #ifdef _WIN32
    typedef SOCKET socket_t;
    #else
    typedef int socket_t;
    #endif

    struct Socket
    {
        struct addrinfo* m_info = nullptr;
        socket_t m_socket = -1;
        port_t m_port = -1;
        NetworkProtocol m_protocol;
        bool m_server;
        network_listener_id_t m_listener;
    };
    #endif

    void NetworkManager::init()
    {
        #ifdef NETWORKING_ENABLED
        if (m_init_sockets) return;

        m_networking_available = true;

        // set up hints
        memset(&hints_tcp, 0, sizeof(addrinfo));
        hints_tcp.ai_family = AF_UNSPEC; // IPv4, IPv6, dou xihuan.
        hints_tcp.ai_socktype = SOCK_STREAM; // tcp
        hints_tcp.ai_flags = AI_PASSIVE;

        memset(&hints_udp, 0, sizeof(addrinfo));
        hints_udp.ai_family = AF_UNSPEC; // IPv4, IPv6, dou xihuan.
        hints_udp.ai_socktype = SOCK_DGRAM; // udp
        hints_udp.ai_flags = AI_PASSIVE;

        #ifdef _WIN32
        {
            WSADATA wsaData;

            if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
            {
                std::cerr << "Error in WSAStartup (for networking)." << std::endl;
                m_networking_available = false;
            }

            // TODO: WSACleanup() on end.

        }
        #endif

        m_init_sockets = true;
        #endif
    }

    socket_id_t NetworkManager::create_server_socket(bool raw, NetworkProtocol m, port_t port, size_t max_client, network_listener_id_t listener)
    {
        #ifdef NETWORKING_ENABLED
        init();

        socket_id_t tr = m_sockets.size();
        Socket* s = new Socket();
        s->m_protocol = m;
        s->m_server = true;

        int status;
        struct addrinfo* hint = (m == NetworkProtocol::UDP)
            ? &hints_udp
            : &hints_tcp;

        if ((status = getaddrinfo(
            nullptr,
            std::to_string(port).c_str(),
            hint,
            &s->m_info)) != 0) {
            delete s;
            return -1;
        }

        s->m_port = port;
        s->m_socket = socket(
            AF_UNSPEC,
            (m == NetworkProtocol::UDP)
            ? SOCK_DGRAM
            : SOCK_STREAM,
            0
        );
        s->m_listener = listener;

        fcntl(s->m_socket, F_SETFL, O_NONBLOCK);

        if (bind(
            s->m_socket,
            s->m_info->ai_addr,
            s->m_info->ai_addrlen
        ) == -1)
        {
            delete s;
            return -2;
        }

        listen(s->m_socket, 32);

        m_sockets[tr] = s;
        return tr;
        #else
        return 0;
        #endif
    }

    socket_id_t NetworkManager::create_socket(bool raw, NetworkProtocol nm, network_listener_id_t listener)
    {
        #ifdef NETWORKING_ENABLED
        init();

        // TODO: find unused port.
        return create_socket(raw, nm, listener, -1);
        #else
        return 0;
        #endif
    }

    socket_id_t NetworkManager::create_socket(bool raw, NetworkProtocol nm, network_listener_id_t listener, port_t)
    {
        #ifdef NETWORKING_ENABLED
        // TODO: set port to what is requested.

        init();

        socket_id_t tr = m_sockets.size();
        Socket* s = new Socket();
        s->m_server = false;
        s->m_socket = socket(
            AF_UNSPEC,
            (nm == NetworkProtocol::UDP)
            ? SOCK_DGRAM
            : SOCK_STREAM,
            0
        );

        fcntl(s->m_socket, F_SETFL, O_NONBLOCK);

        s->m_protocol = nm;
        s->m_listener = listener;

        m_sockets[tr] = s;
        return tr;
        #else
        return 0;
        #endif
    }

    socket_id_t NetworkManager::add_receiving_socket(int socket, NetworkProtocol np, network_listener_id_t listener)
    {
        #ifdef NETWORKING_ENABLED
        socket_id_t tr = m_sockets.size();
        Socket* s = new Socket();
        s->m_server = false; // delegated by a server, but not a server.
        s->m_socket = socket;
        fcntl(s->m_socket, F_SETFL, O_NONBLOCK);

        s->m_protocol = np;
        s->m_listener = listener;

        m_sockets[tr] = s;
        return tr;
        #else
        return 0;
        #endif
    }

    bool NetworkManager::connect_socket(socket_id_t id, const char* url, port_t port)
    {
        #ifdef NETWORKING_ENABLED
        if (m_sockets.find(id) == m_sockets.end())
        {
            throw MiscError("unknown socket ID");
        }

        Socket* s = m_sockets.at(id);

        getaddrinfo(url, std::to_string(port).c_str(),
            (s->m_protocol == NetworkProtocol::UDP)
            ? &hints_udp
            : &hints_tcp,
            &s->m_info
        );

        if (connect(s->m_socket, s->m_info->ai_addr, s->m_info->ai_addrlen))
        {
            return false;
        }
        return true;
        #else
        return false;
        #endif
    }

    #ifdef NETWORKING_ENABLED
    namespace
    {
        // https://stackoverflow.com/a/24560310
        int resolvehelper(const char* hostname, int family, const char* service, sockaddr_storage* pAddr)
        {
            int result;
            addrinfo* result_list = NULL;
            addrinfo hints = {};
            hints.ai_family = family;
            hints.ai_socktype = SOCK_DGRAM;
            result = getaddrinfo(hostname, service, &hints, &result_list);
            if (result == 0)
            {
                //ASSERT(result_list->ai_addrlen <= sizeof(sockaddr_in));
                memcpy(pAddr, result_list->ai_addr, result_list->ai_addrlen);
                freeaddrinfo(result_list);
            }

            return result;
        }
    }
    #endif

    size_t NetworkManager::send_raw(socket_id_t id, size_t datac, const char* datav, const char* url, port_t port)
    {
        #ifdef NETWORKING_ENABLED
        if (m_sockets.find(id) == m_sockets.end())
        {
            throw MiscError("unknown socket ID");
        }

        Socket* s = m_sockets.at(id);

        switch (s->m_protocol)
        {
        case NetworkProtocol::UDP:
            {
                sockaddr_storage addr = {};
                resolvehelper(url, AF_UNSPEC, std::to_string(port).c_str(), &addr);
                return sendto(s->m_socket, datav, datac, 0, (sockaddr*)&addr, sizeof(sockaddr_storage));
            }
        case NetworkProtocol::TCP:
            {
                return send(s->m_socket, datav, datac, 0);
            }
        case NetworkProtocol::BLUETOOTH:
            // TODO
            return 0;
        }
        #endif
        return 0;
    }

    void NetworkManager::destroy_socket(socket_id_t id)
    {
        #ifdef NETWORKING_ENABLED
        Socket* s = m_sockets.at(id);
        if (!s) return;

        #ifdef _WIN32
        closesocket(s->m_socket);
        #else
        close(s->m_socket);
        #endif
        freeaddrinfo(s->m_info);

        delete m_sockets.at(id);
        m_sockets.at(id) = nullptr;
        #endif
    }

    const size_t K_RECV_SIZE = 4096;
    char g_recv_buffer[K_RECV_SIZE];

    void NetworkManager::receive(std::vector<SocketEvent>& out)
    {
        #ifdef NETWORKING_ENABLED
        for (const std::pair<const socket_id_t, Socket*>& pair : m_sockets)
        {
            socket_id_t id = pair.first;
            Socket* s = pair.second;
            if (!s) continue;

            if (s->m_server)
            {
                // accept until no more connections to accept.
                while(true)
                {
                    sockaddr_storage in;
                    socklen_t addrsize = sizeof(in);
                    int new_socket = accept(s->m_socket, (sockaddr *) &in, &addrsize);
                    if (new_socket != -1)
                    {
                        // TODO: notify listener of new connection
                        out.emplace_back(id, s->m_listener, SocketEvent::CONNECTION_ACCEPTED);
                        out.back().m_connected_socket = add_receiving_socket(new_socket, s->m_protocol, s->m_listener);
                    }
                    else
                    {
                        // no new connections
                        break;
                    }
                }
            }

            switch(s->m_protocol)
            {
            case NetworkProtocol::UDP:
                {
                    sockaddr_storage in;
                    socklen_t insize = sizeof(in);
                    int result = recvfrom(
                        s->m_socket, g_recv_buffer, K_RECV_SIZE, 0,
                        (sockaddr*) &in, &insize
                    );
                    if (result < 0) continue; // no data or errror.
                    if (result == 0)
                    // connection ended.
                    {
                        // this actually shouldn't be possible on UDP.
                        out.emplace_back(id, s->m_listener, SocketEvent::CONNECTION_ENDED);
                    }
                    else
                    {
                        char* data = new char[result];
                        memcpy(data, g_recv_buffer, result);
                        out.emplace_back(id, s->m_listener, SocketEvent::DATA_RECEIVED);
                        out.back().m_data_len = result;
                        out.back().m_data = data;
                        // TODO: set "from" address
                    }
                }
                break;
            case NetworkProtocol::TCP:
                {
                    int result = recv(s->m_socket, g_recv_buffer, K_RECV_SIZE, 0);
                    if (result < 0) continue; // no data or errror.
                    if (result == 0)
                    // connection ended.
                    {
                        out.emplace_back(id, s->m_listener, SocketEvent::CONNECTION_ENDED);
                    }
                    else
                    {
                        char* data = new char[result];
                        memcpy(data, g_recv_buffer, result);
                        out.emplace_back(id, s->m_listener, SocketEvent::DATA_RECEIVED);
                        out.back().m_data_len = result;
                        out.back().m_data = data;
                    }
                }
                break;

            }
        }
        #endif
    }
}}
