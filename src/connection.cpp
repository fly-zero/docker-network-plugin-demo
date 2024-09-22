#include "connection.h"

#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http_request.h"
#include "server.h"

template <typename Receiver>
void recv_http_request(http_request &request, Receiver &&receiver)
{
    while (!request.is_completed()) {
        char buf[1024];
        auto const n = std::forward<Receiver>(receiver)(buf, sizeof buf);
        if (n == 0) {
            throw std::runtime_error{ "connection closed by peer" };
        }

        auto const err = request.parse(buf, n);
        if (err < 0) {
            throw std::runtime_error{ "cannot parse http request" };
        }
    }
}

connection::connection(server & server, int fd, size_t stack_size)
    : io_listener{ fd }
    , stack_size_{ stack_size }
    , source_{ get_stack() , [this](pull_type & sink) {
        this->sink_ = &sink;
        this->run();
    } }
    , server_{ server }
{
}

void connection::on_read()
{
    if (status_ == status::waiting_on_read) {
        resume();
    }
}

void connection::on_write()
{
    if (status_ == status::waiting_on_write) {
        resume();
    }
}

void connection::run()
{
    try{
        // receive http request
        http_request request;
        recv_http_request(request, [this](void * buf, size_t len) {
            return recv(buf, len);
        });

        // send http response
        std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        send(response.data(), response.size());
    } catch (std::exception const & e) {
        // TODO: log error
    }

    // close connection
    server_.move_to_closing(*this);

    // set status
    status_ = status::closing;
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

ssize_t connection::send(const void * buf, size_t len)
{
    auto curr = static_cast<const char *>(buf);
    auto const end = curr + len;

    while (curr < end) {
        auto const n = ::send(fd(), curr, end - curr, 0);
        if (n > 0) {
            curr += n;
        } else if (n == 0) {
            throw std::runtime_error{ "cannot send data" };
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            yield(status::waiting_on_write);
        } else {
            throw std::system_error{ errno, std::system_category(), "cannot send data" };
        }
    }

    return len;
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

inline co_stack connection::get_stack()
{
    auto const vp = reinterpret_cast<char *>(this) - stack_size_;
    return co_stack{ vp, stack_size_ };
}