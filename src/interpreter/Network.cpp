#include "ogm/common/util.hpp"
#include "ogm/interpreter/Network.hpp"
#include "ogm/interpreter/Buffer.hpp"

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
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#endif

#include <iostream>
#include <deque>
#include <queue>

// reference: http://www.wangafu.net/~nickm/libevent-book/01_intro.html

namespace ogm { namespace interpreter
{
    #ifdef NETWORKING_ENABLED

    const size_t K_BUFF_SIZE = 4096;
    char g_buffer[K_BUFF_SIZE];

    // setsocketopt uses pointers to ints to set flags..
    int g_opt_enabled[2]{ 0, 1 };

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
        bool m_raw; // if not raw: do message magic for TCP.
        bool m_server; // is this the server?

        // the opengml instance that receives these updates.
        network_listener_id_t m_listener;

        // We use pointers so that in the async event a safe reference to this
        // buffer can be attained.
        std::shared_ptr<Buffer> m_recv_buffer;
        std::deque<char> m_send_buffer;

        // magic (non-raw) information
        uint64_t m_magic_send_message_id = 0;
        uint64_t m_magic_recv_message_id = 0;
        size_t m_magic_recv_buffer_offset = 0;
        size_t m_magic_recv_buffer_length = 0;
    };

    static bool errno_eagain()
    {
        return errno == EAGAIN || errno == EWOULDBLOCK;
    }

    // this is the non-raw header.

    #pragma pack(4)
    struct MagicHeader
    {
        // message ID.
        uint64_t m_message_n;

        // message size.
        uint32_t m_message_length;
    };
    static_assert(sizeof(MagicHeader) == 12, "MagicHeader must be 12 bytes.");

    static void interpret_header(const MagicHeader* h, Socket* s)
    {
        if (h->m_message_n != s->m_magic_recv_message_id)
        {
            throw MiscError("received message ID is unexpected.");
        }
        s->m_magic_recv_buffer_length = h->m_message_length + sizeof(MagicHeader);
    }

    // tries to set the "nodelay" option on the given tcp socket,
    // disabling Nagle's algorithm.
    inline static void socket_set_nodelay(Socket* s, bool enable=true)
    {
        if (s->m_protocol == NetworkProtocol::TCP)
        {
            setsockopt(s->m_socket_fd, SOL_TCP, TCP_NODELAY, &g_opt_enabled[enable], sizeof(int));
        }
    }

    template<bool enable=true>
    inline static void socket_cork(Socket* s)
    {
        ogm_assert(s->m_protocol == NetworkProtocol::TCP);
        #if defined(_WIN32) || defined(__APPLE__)
        setsockopt(s->m_socket_fd, SOL_TCP, TCP_NODELAY, &g_opt_enabled[!enable], sizeof(int));
        if (!enable)
        {
            // send zero bytes to clear buffer.
            ::send(s->m_socket_fd, nullptr, 0, 0);
        }
        #else
        setsockopt(s->m_socket_fd, SOL_TCP, TCP_CORK, &g_opt_enabled[enable], sizeof(int));
        #endif
    }

    inline static void socket_uncork(Socket* s)
    {
        socket_cork<false>(s);
    }

    #endif // NETWORKING_ENABLED

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
        s->m_raw = raw;

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
            perror("Failed to create socket");
            delete s;
            return -3;
        }

        fcntl(s->m_socket_fd, F_SETFL, O_NONBLOCK);

        // disable Nagle's algorithm.
        socket_set_nodelay(s);

        #ifndef WIN32
        // not sure why this exists...
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
            perror("Failed to bind socket");
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
        s->m_raw = raw;

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
    bool NetworkManager::connect_socket(socket_id_t id, bool raw, const char* url, port_t port)
    {
        #ifdef NETWORKING_ENABLED
        if (m_sockets.find(id) == m_sockets.end())
        {
            throw MiscError("unknown socket ID");
        }

        Socket* s = m_sockets.at(id);
        s->m_raw = raw;

        struct sockaddr_in sin;

        struct hostent* h = gethostbyname(url);
        if (!h)
        {
            throw MiscError("Failed to find host for " + std::string(url) + ": " + hstrerror(h_errno));
        }
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

    int32_t NetworkManager::send(socket_id_t id, size_t datac, const char* datav, const char* url, port_t port)
    {
        // FIXME: ::send can return -1. Handle errors correctly.
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
                return ::sendto(s->m_socket_fd, datav, datac, 0, (sockaddr*)&addr, sizeof(sockaddr_storage));
            }
        case NetworkProtocol::TCP:
            {
                if (s->m_raw)
                {
                    int32_t r = ::send(s->m_socket_fd, datav, datac, 0);
                    if (r == 0 || (r == -1 && errno_eagain()))
                    {
                        return 0;
                    }
                    else if (r == -1)
                    {
                        perror("TCP send error");
                        throw MiscError("TCP send error.");
                    }
                    return r;
                }
                else
                {
                    // OPTIMIZE: consider extracting the corking logic in flush_tcp_send_buffer to
                    // include both this flush and the send later on?
                    bool send_blocked = !!flush_tcp_send_buffer(s);

                    size_t sent = 0;
                    const size_t k_max_copy_size = 60;
                    const size_t k_magic_header_size = sizeof(MagicHeader);
                    ogm_assert(k_max_copy_size + k_magic_header_size < K_BUFF_SIZE);

                    // create magic header
                    MagicHeader* mh = reinterpret_cast<MagicHeader*>(g_buffer);
                    mh->m_message_n = s->m_magic_send_message_id++;
                    mh->m_message_length = datac;

                    if (datac != mh->m_message_length)
                    {
                        throw MiscError("32-bit overflow; TCP message too long to send.");
                    }

                    // for small sends, copy data into the buffer.
                    size_t buffer_c = k_magic_header_size;
                    if (datac < k_max_copy_size)
                    {
                        memcpy(g_buffer + buffer_c, datav, datac);
                        buffer_c += datac;
                        datac = 0;
                    }

                    socket_cork(s);
                    const size_t k_max_send = 65000;
                    const char* buffer = g_buffer;
                    while (buffer_c > 0 || datac > 0)
                    // send g_buffer contents, then send data.
                    {
                        bool send_data = buffer_c == 0;
                        const char*& b = (send_data)
                            ? datav
                            : buffer;
                        size_t& c = (send_data)
                            ? datac
                            : buffer_c;
                        int32_t r = 0;
                        if (!send_blocked)
                        {
                            // send data now (unless there is a queued message ahead of us.)
                            r = ::send(s->m_socket_fd, b, c, 0);
                        }
                        if (r == 0 || (r == -1 && errno_eagain()))
                        // no data could be written.
                        {
                            send_blocked = true;

                            // put data in the send buffer, then use it later.
                            while (c > 0)
                            {
                                s->m_send_buffer.push_back(*(b++));
                                c--;
                            }
                        }
                        else if (r == -1)
                        {
                            perror("TCP send error");
                            throw MiscError("TCP send error.");
                        }
                        else
                        // some data written successfully
                        {
                            ogm_assert(r >= 0);
                            ogm_assert(r <= c);
                            c -= r;
                            b += r;
                            sent += r;
                        }
                    }

                    socket_uncork(s);
                    return sent;
                }
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
                        event.m_connected_socket = add_receiving_socket(new_socket, s->m_raw, s->m_protocol, s->m_listener);
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
                        s->m_socket_fd, g_buffer, K_BUFF_SIZE, 0,
                        (sockaddr*) &in, &insize
                    );
                    if (result < 0)
                    {
                        if (errno_eagain() || errno == ENOTCONN)
                        {
                            // we'll check again next loop.
                            continue;
                        }
                        else switch (errno)
                        {
                            default:
                                // FIXME: is there a constant better suited to this?
                                // This is especially misleading since
                                perror("UDP recvfrom error");
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
                        s->m_recv_buffer->clear();
                        s->m_recv_buffer->write(g_buffer, result);
                        SocketEvent& event = out.emplace_back(id, s->m_listener, SocketEvent::DATA_RECEIVED);
                        event.m_buffer = s->m_recv_buffer;
                    }
                }
                break;
            case NetworkProtocol::TCP:
                {
                    int result = recv(s->m_socket_fd, g_buffer, K_BUFF_SIZE, 0);
                    // TODO: check errno = EAGAIN
                    if (result < 0)
                    {
                        if (errno_eagain() || errno == ENOTCONN)
                        {
                            // we'll check again next loop.
                            continue;
                        }
                        else switch (errno)
                        {
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
                        receive_tcp_stream(id, s, result, g_buffer, out);
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

    void NetworkManager::flush_send_all()
    {
        #ifdef NETWORKING_ENABLED
        for (auto& [id, s] : m_sockets)
        {
            if (s)
            {
                flush_tcp_send_buffer(s);
            }
        }
        #endif
    }

    inline size_t NetworkManager::flush_tcp_send_buffer(Socket* s)
    {
        #ifdef NETWORKING_ENABLED
        const size_t k_send_amount = 512;
        ogm_assert(k_send_amount < K_BUFF_SIZE)

        socket_cork(s);

        // loop until buffer empty *or* failed to send some data.
        while (!s->m_send_buffer.empty())
        {
            size_t max_send = std::min(k_send_amount, s->m_send_buffer.size());
            size_t send_count = 0;

            // put up to k_send_amount bytes of s->m_send_buffer onto g_buffer.
            while (send_count < max_send)
            {
                ogm_assert(!s->m_send_buffer.empty());
                g_buffer[send_count++] = s->m_send_buffer.front();
                s->m_send_buffer.pop_front();
            }

            int32_t sent_c = ::send(s->m_socket_fd, g_buffer, send_count, 0);
            if (sent_c < max_send)
            {
                // ignore errors
                if (sent_c < 0) sent_c = 0;

                // put from g_buffer back into deque.
                for (size_t i = max_send; i --> sent_c;)
                {
                    s->m_send_buffer.push_front(g_buffer[i]);
                }

                // failed to finish sending, so we exit loop.
                // (another iteration is unlikely to be fruitful.)
                break;
            }
        }

        socket_uncork(s);

        return s->m_send_buffer.size();
        #else
        return 0;
        #endif
    }

    inline void NetworkManager::receive_tcp_stream(socket_id_t id, Socket* s, size_t datac, const char* datav, std::vector<SocketEvent>& out)
    {
        #ifdef NETWORKING_ENABLED
        if (s->m_raw)
        {
            s->m_recv_buffer->seek(0);
            s->m_recv_buffer->clear(); // give the user consistent results.
            s->m_recv_buffer->write(datav, datac);
            auto& event = out.emplace_back(id, s->m_listener, SocketEvent::DATA_RECEIVED);
            event.m_buffer = s->m_recv_buffer;
        }
        else while (datac > 0)
        {
            bool new_message = s->m_magic_recv_buffer_offset == s->m_magic_recv_buffer_length;
            if (new_message)
            {
                // create a new buffer to handle this message.
                s->m_recv_buffer = std::move(std::shared_ptr<Buffer>(new Buffer(64, Buffer::GROW)));
                s->m_recv_buffer->clear();
                s->m_recv_buffer->seek(0);
                if (datac >= sizeof(MagicHeader))
                {
                    // interpret header right away.
                    interpret_header(reinterpret_cast<const MagicHeader*>(datav), s);
                    s->m_magic_recv_buffer_offset = sizeof(MagicHeader);
                    datac -= sizeof(MagicHeader);
                    datav += sizeof(MagicHeader);
                }
                else
                {
                    // wait until the full header is received.
                    s->m_recv_buffer->write(datav, datac);
                    s->m_magic_recv_buffer_length = sizeof(MagicHeader);
                    s->m_magic_recv_buffer_offset = datac;
                    return;
                }
            }
            else if (s->m_magic_recv_buffer_offset < sizeof(MagicHeader))
            // still receiving header.
            {
                if (datac + s->m_magic_recv_buffer_offset >= sizeof(MagicHeader))
                // received full header (finally.)
                {
                    size_t header_remaning = sizeof(MagicHeader) - s->m_magic_recv_buffer_offset;

                    // write remaining header to buffer, then read the whole header.
                    s->m_recv_buffer->write(datav, header_remaning);
                    interpret_header(
                        reinterpret_cast<MagicHeader*>(s->m_recv_buffer->get_address()),
                        s
                    );
                    ogm_assert(datac >= header_remaning);
                    datac -= header_remaning;
                    datav += header_remaning;
                    s->m_magic_recv_buffer_offset = sizeof(MagicHeader);
                }
                else
                {
                    // add to the buffer.
                    s->m_recv_buffer->write(datav, datac);
                    s->m_magic_recv_buffer_offset += datac;
                    return;
                }
            }

            ogm_assert(s->m_magic_recv_buffer_offset <= s->m_magic_recv_buffer_length);

            // receive additional body data
            size_t body_datac = std::min(datac, s->m_magic_recv_buffer_length - s->m_magic_recv_buffer_offset);
            s->m_recv_buffer->write(datav, body_datac);
            datav += body_datac;
            datac -= body_datac;
            s->m_magic_recv_buffer_offset += body_datac;

            ogm_assert(s->m_magic_recv_buffer_offset <= s->m_magic_recv_buffer_length);

            // check if message fully received.
            if (s->m_magic_recv_buffer_offset == s->m_magic_recv_buffer_length)
            // message fully received
            {
                auto& event = out.emplace_back(id, s->m_listener, SocketEvent::DATA_RECEIVED);
                event.m_buffer = s->m_recv_buffer;
                s->m_magic_recv_message_id++;
            }
            else if (s->m_magic_recv_buffer_offset < s->m_magic_recv_buffer_length)
            {
                return;
            }
        }
        #endif
    }

    socket_id_t NetworkManager::add_receiving_socket(int socket, bool raw, NetworkProtocol np, network_listener_id_t listener)
    {
        #ifdef NETWORKING_ENABLED
        socket_id_t out_id = m_sockets.size();
        Socket* s = new Socket();
        s->m_server = false; // delegated by a server, but not a server.
        s->m_socket_fd = socket;
        s->m_listener = listener;
        s->m_raw = raw;

        fcntl(s->m_socket_fd, F_SETFL, O_NONBLOCK);

        s->m_protocol = np;

        m_sockets[out_id] = s;
        return out_id;
        #else
        return 0;
        #endif
    }
}}
