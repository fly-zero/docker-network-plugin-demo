#pragma once

#include "event_dispatcher.h"
#include "connection.h"

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
