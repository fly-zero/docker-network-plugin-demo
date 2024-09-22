#pragma once

#include <unordered_map>

#include "event_dispatcher.h"
#include "connection.h"
#include "http_request.h"
#include "http_response.h"

/**
 * @brief The server
 */
class server
    : public io_listener
    , public loop_listener
{
public:
    class http_request_handler
    {
    public:
        http_request_handler(bool (*handler)(void *, const http_request &, http_response *),
                             void * user);

        bool operator()(void * user, const http_request & request, http_response * response) const;

    private:
        bool (*handler_)(void *, const http_request &, http_response *);
        void * user_;
    };

private:
    using connection_list = boost::intrusive::list
        < connection
        , boost::intrusive::member_hook
            < connection
            , connection::list_hook
            , &connection::list_hook_
            >
        >;

    using pull_type = boost::coroutines2::coroutine<void>::pull_type;

    using uri_handler_map = std::unordered_map<std::string, http_request_handler>;

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

    /**
     * @brief Add a uri handler
     *
     * @param uri the uri
     * @param handler the handler
     * @param user the user data
     */
    void register_uri_handler(const char *uri,
                              bool (*handler)(void *, const http_request &, http_response *),
                              void *user);

    /**
     * @brief Find the uri handler
     *
     * @param uri The uri string
     * @return const http_request_handler* The uri handler if found, otherwise nullptr
     */
    const http_request_handler * find_uri_handler(const std::string &uri) const;

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
    pull_type        *sink_{ nullptr };     ///< the push type
    connection_list   active_list_{ };      ///< the active connection list
    connection_list   closing_list_{ };     ///< the closing connection list
    event_dispatcher &dispatcher_;          ///< the event dispatcher
    uri_handler_map   uri_handler_map_{ };  ///< the uri handler map
};

inline server::http_request_handler::http_request_handler(
    bool (*handler)(void *, const http_request &, http_response *), void * user)
    : handler_{ handler }
    , user_{ user }
{
}

inline bool server::http_request_handler::operator()(void *user,
                                                     const http_request &request,
                                                     http_response *response) const
{
    return handler_(user, request, response);
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

inline void server::register_uri_handler(const char *uri,
                                         bool (*handler)(void *, const http_request &, http_response *),
                                         void *user)
{
    uri_handler_map_.emplace(uri, http_request_handler{ handler, user });
}

inline const server::http_request_handler * server::find_uri_handler(const std::string &uri) const
{
    auto const it = uri_handler_map_.find(uri);
    return it != uri_handler_map_.end() ? &it->second : nullptr;
}

inline int server::sock() const
{
    return fd();
}
