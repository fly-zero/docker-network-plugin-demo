#include "server.h"
#include "event_dispatcher.h"
#include "llhttp.h"

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>

#include <system_error>

#include <llhttp.h>

struct connection::http_request {
    bool is_completed{ false };
    std::string url;
};

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

/**
 * @brief URL callback for HTTP parser
 *
 * @param parser HTTP parser
 * @param data URL data
 * @param size URL size
 * @return int return HPE_OK if no error
 */
static int on_http_url(llhttp_t *parser, const char *data, size_t size)
{
    return HPE_OK;
}

/**
 * @brief HTTP message complete callback for HTTP parser
 *
 * @param parser HTTP parser
 * @return int return HPE_OK if no error
 */
static int on_http_message_complete(llhttp_t *parser)
{
    return HPE_OK;
}

auto inline parse_http(llhttp_t &parser, const void * buf, size_t len) -> int
{
    auto const err = llhttp_execute(&parser, static_cast<const char *>(buf), len);
    if (err != HPE_OK) {
        return -1;
    }

    return 0;
}

connection::connection(server & server, int fd, size_t stack_size)
    : io_listener{ fd }
    , stack_size_{ stack_size }
    , source_{ [this](pull_type & sink) {
        this->sink_ = &sink;
        this->run();
    } }
    , server_{ server }
{
}

void connection::on_read()
{
    resume();
}

void connection::on_write()
{
    resume();
}

void connection::run()
{
    // receive http request
    http_request req;
    recv_request(req);
}

ssize_t connection::recv(void * buf, size_t len)
{
    auto const n = ::recv(fd(), buf, len, 0);
    if (n >= 0) {
        return n;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        yield(status::waiting_on_read);
        return 0;
    } else {
        throw std::system_error{ errno, std::system_category(), "cannot receive data" };
    }
}

inline void connection::recv_request(http_request &req)
{
    // create http parser settings
    constexpr llhttp_settings_t settings = {
        .on_url = on_http_url,
        .on_message_complete = on_http_message_complete,
    };

    // create http parser
    llhttp_t parser{ };
    llhttp_init(&parser, HTTP_REQUEST, &settings);

    // set http parser's private data
    http_request request{ };
    parser.data = &request;

    while (true) {
        char buf[1024];
        auto const n = recv(buf, sizeof buf);
        if (n == 0) {
            break; // TODO: connection closed by peer
        }

        auto const result = parse_http(parser, buf, n);
        if (result == 0) {
            break;
        }

        if (result == -1) {
            throw std::runtime_error{ "cannot parse http request" };
        }
    }
}

connection * connection::allocate(server &server, int sock, size_t stack_size)
{
    // get page size
    auto const page_size = sysconf(_SC_PAGESIZE);

    // get page mask
    auto const page_mask = page_size - 1;

    // align stack size to page size
    stack_size = (stack_size + page_mask) & ~page_mask;

    // calculate total size, add one page at the top of the stack as guard page
    auto total_size = page_size + stack_size + sizeof(connection);

    // align to page size
    total_size = (total_size + page_mask) & ~page_mask;

    // allocate memory
    auto const mem = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        throw std::system_error{ errno, std::system_category(), "cannot allocate memory" };
    }

    // set guard page unreadable and unwritable
    if (mprotect(mem, page_size, PROT_NONE) < 0) {
        munmap(mem, total_size);
        throw std::system_error{ errno, std::system_category(), "cannot set guard page unreadable and unwritable" };
    }

    // create connection object at then bottom of the stack
    auto const conn = ::new(static_cast<char *>(mem) + page_size + stack_size) connection{ server, sock, stack_size };
    return conn;
}

void connection::deallocate(connection * conn)
{
    // get page size
    auto const page_size = sysconf(_SC_PAGESIZE);

    // get page mask
    auto const page_mask = page_size - 1;

    // calculate total size, add one page at the top of the stack as guard page
    auto total_size = page_size + conn->stack_size_ + sizeof(connection);

    // align to page size
    total_size = (total_size + page_mask) & ~page_mask;

    // calculate start of memory
    auto const mem = reinterpret_cast<char *>(conn) - page_size - conn->stack_size_;

    // destroy connection object
    conn->~connection();

    // deallocate memory
    munmap(mem, total_size);
}

server::server(event_dispatcher &dispatcher, const char * path)
    : io_listener{ listen(path) }
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
        conn_list_.push_back(*conn);

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