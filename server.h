#pragma once

class server
{
public:
    /**
     * @brief Construct a new server object
     */
    server() = default;

    /**
     * @brief Construct a new server object
     *
     * @param path the unix socket path
     */
    explicit server(const char * path);

    /**
     * @brief Destroy the server object
     */
    ~server();

    /**
     * @brief Move constructor
     *
     * @param other the other server object
     */
    server(server && other) noexcept;

    /**
     * @brief Move assignment
     *
     * @param other the other server object
     * @return server& the reference to this object
     */
    server & operator=(server && other) noexcept;

    /**
     * @brief Check if the server object is valid
     *
     * @return true if the server object is valid
     * @return false if the server object is invalid
     */
    operator bool() const;

    /**
     * @brief Check if the server object is invalid
     *
     * @return true if the server object is invalid
     * @return false if the server object is valid
     */
    bool operator!() const;

protected:
    /**
     * @brief Construct a new server object
     *
     * @param sock the socket file descriptor
     */
    explicit server(int sock);

private:
    int sock_{ -1 };  ///< the socket file descriptor
};

inline server::server(server && other) noexcept
    : sock_{ other.sock_ }
{
    other.sock_ = -1;
}

inline server::server(int sock)
    : sock_{ sock }
{
}

inline server::operator bool() const
{
    return sock_ >= 0;
}

inline bool server::operator!() const
{
    return sock_ < 0;
}