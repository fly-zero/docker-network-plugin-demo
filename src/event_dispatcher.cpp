#include "event_dispatcher.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>

io_listener::~io_listener()
{
    if (fd_ >= 0) {
        close(fd_);
    }
}

io_listener & io_listener::operator=(io_listener && other) noexcept
{
    if (this != &other) {
        if (fd_ >= 0) {
            close(fd_);
        }

        fd_ = other.fd_;
        other.fd_ = -1;
    }

    return *this;
}

event_dispatcher::event_dispatcher()
    : epfd_{ epoll_create1(EPOLL_CLOEXEC) }
{
}

event_dispatcher::~event_dispatcher()
{
    if (epfd_ >= 0) {
        close(epfd_);
    }
}

event_dispatcher & event_dispatcher::operator=(event_dispatcher && other) noexcept
{
    if (this != &other) {
        if (epfd_ >= 0) {
            close(epfd_);
        }

        epfd_ = other.epfd_;
        other.epfd_ = -1;
    }

    return *this;
}

void event_dispatcher::run()
{
    running_ = true;

    epoll_event events[64];
    while (running_) {
        auto const n = epoll_wait(epfd_, events, sizeof(events) / sizeof(events[0]), 50);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        for (auto i = 0; i < n; ++i) {
            auto const ev = events[i];
            auto const listener = static_cast<io_listener *>(ev.data.ptr);
            if (ev.events & EPOLLIN) {
                listener->on_read();
            }

            if (ev.events & EPOLLOUT) {
                listener->on_write();
            }
        }

        for (auto & l : on_loop_list_) {
            l.on_loop();
        }
    }
}

bool event_dispatcher::subscribe(io_listener & listener, int e)
{
    epoll_event ev;
    ev.events = EPOLLET;
    ev.events |= (e & readable ? EPOLLIN : 0);
    ev.events |= (e & writable ? EPOLLOUT : 0);
    ev.data.ptr = &listener;

    auto const err = epoll_ctl(epfd_, EPOLL_CTL_ADD, listener.fd(), &ev);
    return err == 0;
}

bool event_dispatcher::unsubscribe(io_listener & listener)
{
    auto const err = epoll_ctl(epfd_, EPOLL_CTL_DEL, listener.fd(), nullptr);
    return err == 0;
}

bool event_dispatcher::subscribe(loop_listener &listener)
{
    for (auto & l : on_loop_list_) {
        if (&listener == &l) {
            return false;
        }
    }

    on_loop_list_.push_back(listener);
    return true;
}

bool event_dispatcher::unsubscribe(loop_listener &listener)
{
    if (listener.list_hook_.is_linked()) {
        on_loop_list_.erase(on_loop_list_.iterator_to(listener));
        return true;
    }

    return false;
}