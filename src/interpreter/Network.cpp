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

// reference: http://www.wangafu.net/~nickm/libevent-book/01_intro.html

namespace ogm { namespace interpreter
{
    #ifdef NETWORKING_ENABLED

    #ifdef _WIN32
    typedef SOCKET socket_t;
    #else
    typedef int socket_t;
    #endif

    struct Socket
    {
        socket_t m_socket_fd = -1;
        port_t m_port = -1;
        NetworkProtocol m_protocol;
        bool m_server; // is this the server?

        // the opengml instance that receives these updates.
        network_listener_id_t m_listener;
    };
    #endif

    void NetworkManager::init()
    {
        // removed.
    }

    socket_id_t NetworkManager::create_server_socket(bool raw, NetworkProtocol m, port_t port, size_t max_client, network_listener_id_t listener)
    {
        #ifdef NETWORKING_ENABLED
        init();

        socket_id_t out_id = m_sockets.size();
        Socket* s = new Socket();
        s->m_protocol = m;
        s->m_server = true;
        s->m_listener = listener;

        int status;

        s->m_port = port;
        s->m_socket_fd = socket(
            AF_INET, // NOTE: this restricts us to IPV4!
            (m == NetworkProtocol::UDP)
            ? SOCK_DGRAM
            : SOCK_STREAM,
            0
        );

        if (static_cast<int32_t>(s->m_socket_fd) < 0)
        {
            delete s;
            return -3;
        }

        fcntl(s->m_socket_fd, F_SETFL, O_NONBLOCK);

        #ifndef WIN32
        // not sure why this exists.
        {
            int one = 1;
            setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        }
        #endif

        // bind
        struct sockaddr_in sin;
        sin.sin_family = AF_INET; // This limits us to IPV4
        sin.sin_port = htons(port);
        sin.sin_addr.s_addr = 0;

        if (
            bind(
                s->m_socket_fd,
                (struct sockaddr*)&sin,
                sizeof(sin)
            )
            == -1
        )
        {
            delete s;
            return -2;
        }

        if (m == NetworkProtocol::TCP)
        {
            if (listen(s->m_socket_fd, 32) < 0)
            {
                delete s;
                return -4;
            }
        }

        m_sockets[out_id] = s;
        return out_id;
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

        socket_id_t out_id = m_sockets.size();
        Socket* s = new Socket();
        s->m_listener = listener;
        s->m_server = false;

        // create socket file descriptor.
        s->m_socket_fd = socket(
            AF_INET, // this limits us to ipv4
            (nm == NetworkProtocol::UDP)
            ? SOCK_DGRAM
            : SOCK_STREAM,
            0
        );

        fcntl(s->m_socket_fd, F_SETFL, O_NONBLOCK);

        s->m_protocol = nm;

        m_sockets[out_id] = s;
        return out_id;
        #else
        return 0;
        #endif
    }

    // connect a client socket to a remote host
    bool NetworkManager::connect_socket(socket_id_t id, const char* url, port_t port)
    {
        #ifdef NETWORKING_ENABLED
        if (m_sockets.find(id) == m_sockets.end())
        {
            throw MiscError("unknown socket ID");
        }

        Socket* s = m_sockets.at(id);

        struct sockaddr_in sin;

        struct hostent* h = gethostbyname(url);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        sin.sin_addr = *(struct in_addr*)h->h_addr;
        if (connect(s->m_socket_fd, (struct sockaddr*) &sin, sizeof(sin)))
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
                return sendto(s->m_socket_fd, datav, datac, 0, (sockaddr*)&addr, sizeof(sockaddr_storage));
            }
        case NetworkProtocol::TCP:
            {
                return send(s->m_socket_fd, datav, datac, 0);
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
        closesocket(s->m_socket_fd);
        #else
        close(s->m_socket_fd);
        #endif
        //freeaddrinfo(s->m_info);

        delete m_sockets.at(id);
        m_sockets.at(id) = nullptr;
        #endif
    }

    const size_t K_RECV_SIZE = 4096;
    char g_recv_buffer[K_RECV_SIZE];

    void NetworkManager::receive(std::vector<SocketEvent>& out)
    {
        #ifdef NETWORKING_ENABLED
        for (std::pair<const socket_id_t, Socket*>& pair : m_sockets)
        {
            socket_id_t id = pair.first;
            Socket *& s = pair.second;
            if (!s) continue;

            bool close_socket = false;

            if (s->m_server && s->m_protocol == NetworkProtocol::TCP)
            {
                // accept until no more connections to accept.
                while(true)
                {
                    sockaddr_storage in;
                    socklen_t addrsize = sizeof(in);
                    int new_socket = accept(s->m_socket_fd, (sockaddr *) &in, &addrsize);
                    if (new_socket >= 0)
                    {
                        std::cout << "Connection received." << std::endl;
                        SocketEvent& event = out.emplace_back(id, s->m_listener, SocketEvent::CONNECTION_ACCEPTED);

                        // create new socket for this
                        event.m_connected_socket = add_receiving_socket(new_socket, s->m_protocol, s->m_listener);
                    }
                    else
                    {
                        // no new connections
                        break;
                    }
                }
            }

            // receive data
            switch(s->m_protocol)
            {
            case NetworkProtocol::UDP:
                {
                    sockaddr_storage in;
                    socklen_t insize = sizeof(in);
                    int result = recvfrom(
                        s->m_socket_fd, g_recv_buffer, K_RECV_SIZE, 0,
                        (sockaddr*) &in, &insize
                    );
                    if (result < 0)
                    {
                        // portability
                        if (EWOULDBLOCK != EAGAIN && errno == EWOULDBLOCK)
                        {
                            errno = EAGAIN;
                        }
                        switch (errno)
                        {
                            case EAGAIN:
                            case ENOTCONN:
                                // we'll check again next loop.
                                continue;
                            default:
                                // FIXME: is there a constant better suited to this?
                                // This is especially misleading since
                                perror("UDP recv error");
                                out.emplace_back(id, s->m_listener, SocketEvent::CONNECTION_ENDED);
                                close_socket = true;
                                break;
                        }
                    }
                    else if (result == 0)
                    // connection ended.
                    {
                        // this actually shouldn't be possible on UDP.
                        out.emplace_back(id, s->m_listener, SocketEvent::CONNECTION_ENDED);
                        close_socket = true;

                        // maybe we should assert false here...
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
                    int result = recv(s->m_socket_fd, g_recv_buffer, K_RECV_SIZE, 0);
                    // TODO: check errno = EAGAIN
                    if (result < 0)
                    {
                        // portability
                        if (EWOULDBLOCK != EAGAIN && errno == EWOULDBLOCK)
                        {
                            errno = EAGAIN;
                        }
                        switch (errno)
                        {
                            case EAGAIN:
                            case ENOTCONN:
                                // we'll check again next loop.
                                continue;
                            default:
                                perror("TCP recv error");
                                // FIXME: is there a constant better suited to this?
                                out.emplace_back(id, s->m_listener, SocketEvent::CONNECTION_ENDED);
                                close_socket = true;
                                break;
                        }
                    }
                    else if (result == 0)
                    // connection ended.
                    {
                        out.emplace_back(id, s->m_listener, SocketEvent::CONNECTION_ENDED);
                        close_socket = true;
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
            case NetworkProtocol::BLUETOOTH:
                // TODO.
                break;
            }

            if (close_socket)
            {
                delete s;
                s = nullptr;
            }
        }
        #endif
    }

    socket_id_t NetworkManager::add_receiving_socket(int socket, NetworkProtocol np, network_listener_id_t listener)
    {
        #ifdef NETWORKING_ENABLED
        socket_id_t out_id = m_sockets.size();
        Socket* s = new Socket();
        s->m_server = false; // delegated by a server, but not a server.
        s->m_socket_fd = socket;
        s->m_listener = listener;

        fcntl(s->m_socket_fd, F_SETFL, O_NONBLOCK);

        s->m_protocol = np;

        m_sockets[out_id] = s;
        return out_id;
        #else
        return 0;
        #endif
    }
}}
