#pragma once

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>

class event_dispatcher;

/**
 * @brief The io listener
 */
class io_listener
{
    friend event_dispatcher;

public:
    /**
     * @brief Construct a new io listener object
     *
     * @param fd the file descriptor
     */
    explicit io_listener(int fd);

    /**
     * @brief Destroy the io listener object
     */
    virtual ~io_listener();

    /**
     * @brief Move constructor
     *
     * @param other the other io listener object
     */
    io_listener(io_listener &&) noexcept;

    /**
     * @brief Move assignment
     *
     * @param other the other io listener object
     * @return io_listener& the reference to this object
     */
    io_listener & operator=(io_listener &&) noexcept;

    /**
     * @brief Copy constructor is deleted
     */
    io_listener(const io_listener &) = delete;

    /**
     * @brief Copy assignment is deleted
     */
    void operator=(const io_listener &) = delete;

    /**
     * @brief Check if the io listener object is valid
     *
     * @return true if the io listener object is valid
     * @return false if the io listener object is invalid
     */
    operator bool() const;

    /**
     * @brief Check if the io listener object is invalid
     *
     * @return true if the io listener object is invalid
     * @return false if the io listener object is valid
     */
    bool operator!() const;

    /**
     * @brief Get the file descriptor
     *
     * @return int the file descriptor
     */
    int fd() const;

protected:
    /**
     * @brief The on read callback
     */
    virtual void on_read() = 0;

    /**
     * @brief The on write callback
     */
    virtual void on_write() = 0;

private:
    int fd_{ -1 };  ///< the file descriptor
};

/**
    * @brief The loop listener
    */
class loop_listener
{
    friend event_dispatcher;

    using list_hook = boost::intrusive::list_member_hook<>;

public:
    loop_listener() = default;

    virtual ~loop_listener() = default;

protected:
    /**
     * @brief The on loop callback
     */
    virtual void on_loop() = 0;

private:
    list_hook list_hook_{ };  ///< the list hook
};

/**
 * @brief The event dispatcher
 */
class event_dispatcher
{
public:


private:
    using on_loop_list = boost::intrusive::list
        < loop_listener
        , boost::intrusive::member_hook
            < loop_listener
            , loop_listener::list_hook
            , &loop_listener::list_hook_
            >
        >;

public:
    enum { readable = 1, writable = 2 };

    /**
     * @brief Construct a new event dispatcher object
     */
    event_dispatcher();

    /**
     * @brief Destroy the event dispatcher object
     */
    ~event_dispatcher();

    /**
     * @brief Move constructor
     *
     * @param other the other event dispatcher object
     */
    event_dispatcher(event_dispatcher && other) noexcept;

    /**
     * @brief Move assignment
     *
     * @param other the other event dispatcher object
     * @return event_dispatcher& the reference to this object
     */
    event_dispatcher & operator=(event_dispatcher && other) noexcept;

    /**
     * @brief Copy constructor is deleted
     */
    event_dispatcher(const event_dispatcher &) = delete;

    /**
     * @brief Copy assignment is deleted
     */
    void operator=(const event_dispatcher &) = delete;

    /**
     * @brief Check if the event dispatcher object is valid
     *
     * @return true if the event dispatcher object is valid
     * @return false if the event dispatcher object is invalid
     */
    operator bool() const;

    /**
     * @brief Check if the event dispatcher object is invalid
     *
     * @return true if the event dispatcher object is invalid
     * @return false if the event dispatcher object is valid
     */
    bool operator!() const;

    /**
     * @brief Run the event dispatcher
     */
    void run();

    /**
     * @brief Stop the event dispatcher
     */
    void stop();

    /**
     * @brief Subscribe the io listener
     *
     * @param listener the io listener
     * @param e the event
     * @return true if the io listener is subscribed
     * @return false if the io listener is not subscribed
     */
    bool subscribe(io_listener & listener, int e);

    /**
     * @brief Unsubscribe the io listener
     *
     * @param listener the io listener
     * @return true if the io listener is unsubscribed
     * @return false if the io listener is not unsubscribed
     */
    bool unsubscribe(io_listener & listener);

    /**
     * @brief Subscribe the loop listener
     *
     * @param listener the loop listener
     * @return true if the loop listener is subscribed
     * @return false if the loop listener is not subscribed
     */
    bool subscribe(loop_listener & listener);

    /**
     * @brief Unsubscribe the loop listener
     *
     * @param listener the loop listener
     * @return true if the loop listener is unsubscribed
     * @return false if the loop listener is not unsubscribed
     */
    bool unsubscribe(loop_listener & listener);

private:
    bool         running_{ false };  ///< the running flag
    int          epfd_{ -1 };        ///< the epoll file descriptor
    on_loop_list on_loop_list_{ };   ///< the loop listener list
};

inline io_listener::io_listener(int fd)
    : fd_{ fd }
{
}

inline io_listener::io_listener(io_listener && other) noexcept
    : fd_{ other.fd_ }
{
    other.fd_ = -1;
}

inline io_listener::operator bool() const
{
    return fd_ >= 0;
}

inline bool io_listener::operator!() const
{
    return fd_ < 0;
}

inline int io_listener::fd() const
{
    return fd_;
}

inline event_dispatcher::event_dispatcher(event_dispatcher && other) noexcept
    : epfd_{ other.epfd_ }
{
    other.epfd_ = -1;
}

inline void event_dispatcher::stop()
{
    running_ = false;
}

inline event_dispatcher::operator bool() const
{
    return epfd_ >= 0;
}

inline bool event_dispatcher::operator!() const
{
    return epfd_ < 0;
}
