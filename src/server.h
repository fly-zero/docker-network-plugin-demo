#pragma once

#include "event_dispatcher.h"

#include <memory>

#include <boost/coroutine2/coroutine.hpp>
#include <boost/coroutine2/fixedsize_stack.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>

class server;

/**
 * @brief the connection
 */
class connection
    : public io_listener
{
    friend server;

    enum class status : uint8_t
    {
        running,
        waiting_on_read,
        waiting_on_write,
        closing
    };

    using list_hook = boost::intrusive::list_member_hook<>;
    using stack     = boost::coroutines2::fixedsize_stack;
    using push_type = boost::coroutines2::coroutine<void>::push_type;
    using pull_type = boost::coroutines2::coroutine<void>::pull_type;

public:
    /**
     * @brief Move constructor is deleted
     */
    connection(connection &&) noexcept = delete;

    /**
     * @brief Move assignment is deleted
     */
    void operator=(connection &&) noexcept = delete;

    /**
     * @brief Copy constructor is deleted
     */
    connection(const connection &) = delete;

    /**
     * @brief Copy assignment is deleted
     */
    void operator=(const connection &) = delete;

protected:
    /**
     * @brief Construct a new connection object
     *
     * @param server the server object
     * @param fd the file descriptor
     * @param stack_size the stack size
     */
    connection(server & server, int fd, size_t stack_size);

    /**
     * @brief Destroy the connection object
     */
    ~connection() = default;

    /**
     * @brief The on read callback
     */
    void on_read() override;

    /**
     * @brief The on write callback
     */
    void on_write() override;

    /**
     * @brief Run the connection coroutine
     */
    void run();

    /**
     * @brief Suspend the connection coroutine
     *
     * @param status the status
     */
    void yield(status status);

    /**
     * @brief Resume the connection coroutine
     */
    void resume();

    /**
     * @brief Receive data from the socket
     *
     * @param buf the buffer
     * @param len the buffer length
     * @return ssize_t the received data length
     */
    ssize_t recv(void * buf, size_t len);

    /**
     * @brief Send data to the socket
     *
     * @param buf the buffer
     * @param len the buffer length
     * @return ssize_t the sent data length
     */
    ssize_t send(const void * buf, size_t len);

    /**
     * @brief Allocate a new connection object
     *
     * @param server the server object
     * @param sock the socket file descriptor
     * @param stack_size the stack size
     * @return connection* the pointer to the connection object
     */
    static connection * allocate(server &server, int sock, size_t stack_size);

    /**
     * @brief Deallocate the connection object
     *
     * @param conn the pointer to the connection object
     */
    static void deallocate(connection * conn);

private:
    list_hook  list_hook_{ };               ///< the list hook
    size_t     stack_size_{ 0 };            ///< the stack size
    push_type  source_;                     ///< the push type
    pull_type *sink_{ nullptr };            ///< the pull type
    status     status_{ status::running };  ///< the status
    server    &server_;                     ///< the server
};

/**
 * @brief The server
 */
class server
    : public io_listener
    , public loop_listener
{
    using connection_list = boost::intrusive::list
        < connection
        , boost::intrusive::member_hook
            < connection
            , connection::list_hook
            , &connection::list_hook_
            >
        >;

    using pull_type = boost::coroutines2::coroutine<void>::pull_type;

public:
    /**
     * @brief Construct a new server object
     *
     * @param dispatcher the event dispatcher
     * @param path the unix socket path
     */
    server(event_dispatcher &dispatcher, const char * path);

    /**
     * @brief Destroy the server object
     */
    ~server() = default;

    /**
     * @brief Move constructor is deleted
     */
    server(server && other) noexcept = delete;

    /**
     * @brief Move assignment is deleted
     */
    server & operator=(server && other) noexcept = delete;

    /**
     * @brief get the event dispatcher
     */
    event_dispatcher &dispatcher() const;

    /**
     * @brief Move the connection to the closing list
     *
     * @param conn the connection object
     */
    void move_to_closing(connection &conn);

protected:
    /**
     * @brief The on read callback
     */
    void on_read() override;

    /**
     * @brief The on write callback
     */
    void on_write() override;

    /**
     * @brief The on loop callback
     */
    void on_loop() override;

    /**
     * @brief Get the socket file descriptor
     *
     * @return int the socket file descriptor
     */
    int sock() const;

private:
    pull_type        *sink_{ nullptr };  ///< the push type
    connection_list   active_list_{ };   ///< the active connection list
    connection_list   closing_list_{ };  ///< the closing connection list
    event_dispatcher &dispatcher_;       ///< the event dispatcher
};

inline void connection::yield(status status)
{
    // set the status
    status_ = status;

    // give up the cpu
    (*sink_)();
}

inline void connection::resume()
{
    // set the status
    status_ = status::running;

    // take back the cpu
    source_();
}

inline event_dispatcher & server::dispatcher() const
{
    return dispatcher_;
}

inline void server::move_to_closing(connection & conn)
{
    // move the connection to the closing list
    closing_list_.splice(closing_list_.begin(), active_list_, active_list_.iterator_to(conn));
}

inline int server::sock() const
{
    return fd();
}
