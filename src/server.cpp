#include "server.h"

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

server::server(const char * path)
    : server(listen(path))
{
}

server::~server()
{
    if (sock_ >= 0) {
        close(sock_);
    }
}

server & server::operator=(server && other) noexcept
{
    if (this != &other) {
        if (sock_ >= 0) {
            close(sock_);
        }

        sock_ = other.sock_;
        other.sock_ = -1;
    }

    return *this;
}