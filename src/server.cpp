#include "server.h"

#include <netinet/in.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>

#include <system_error>

/**
 * @brief listen on \b path
 * @param path the unix socket path
 * @return socket file descriptor
 */
inline int listen(const char * path)
{
    // unlink exist unix socket
    unlink(path);

    // create socket
    auto const sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::system_error{ errno, std::system_category(), "cannot create socket" };
    }

    // set socket non-blocking
    if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK) < 0) {
        close(sock);
        throw std::system_error{ errno, std::system_category(), "cannot set socket non-blocking" };
    }

    // bind socket
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    if (bind(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        close(sock);
        throw std::system_error{ errno, std::system_category(), "cannot bind socket" };
    }

    // listen on socket
    if (listen(sock, 128) < 0) {
        close(sock);
        throw std::system_error{ errno, std::system_category(), "cannot listen on socket" };
    }

    return sock;
}

inline int listen(unsigned short port)
{
    // create socket
    auto const sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::system_error{ errno, std::system_category(), "cannot create socket" };
    }

    // set socket non-blocking
    if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK) < 0) {
        close(sock);
        throw std::system_error{ errno, std::system_category(), "cannot set socket non-blocking" };
    }

    // bind socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        close(sock);
        throw std::system_error{ errno, std::system_category(), "cannot bind socket" };
    }

    // listen on socket
    if (listen(sock, 128) < 0) {
        close(sock);
        throw std::system_error{ errno, std::system_category(), "cannot listen on socket" };
    }

    return sock;
}

server::server(event_dispatcher &dispatcher, const char * path)
    : io_listener{ listen(path) }
    , dispatcher_{ dispatcher }
{
}

server::server(event_dispatcher &dispatcher, unsigned short port)
    : io_listener{ listen(port) }
    , dispatcher_{ dispatcher }
{
}

void server::on_read()
{
    while (true) {
        // accept client
        auto const client_sock = accept4(sock(), nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (client_sock < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            throw std::system_error{ errno, std::system_category(), "cannot accept client" };
        }

        // create client object
        auto const conn = connection::allocate(*this, client_sock, 256 * 1024);
        active_list_.push_back(*conn);

        // subscribe client
        if (!dispatcher_.subscribe(*conn, event_dispatcher::readable | event_dispatcher::writable)) {
            connection::deallocate(conn);
            throw std::system_error{ EINVAL, std::system_category(), "cannot subscribe client" };
        }

        // resume client
        conn->resume();
    }
}

void server::on_write()
{
}

void server::on_loop()
{
    // destroy closing connections
    while (!closing_list_.empty()) {
        auto & conn = closing_list_.front();
        dispatcher_.unsubscribe(conn);
        closing_list_.pop_front_and_dispose([](connection * conn) {
            connection::deallocate(conn);
        });
    }
}
