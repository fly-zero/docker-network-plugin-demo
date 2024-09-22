#pragma once

#include <cstddef>

#include <boost/context/stack_context.hpp>

class co_stack
{
public:
    /**
     * @brief Construct a new co stack object
     *
     * @param vp the stack pointer
     * @param size the stack size
     */
    co_stack(void *vp, size_t size);

    /**
     * @brief Destroy the co stack object
     */
    ~co_stack() = default;

    /**
     * @brief Allocate the stack context
     *
     * @return boost::context::stack_context the stack context
     */
    boost::context::stack_context allocate();

    /**
     * @brief Deallocate the stack context
     *
     * @param ctx the stack context
     */
    void deallocate(boost::context::stack_context &ctx);

private:
    void *vp_{nullptr};  ///< the stack pointer
    size_t size_{0};     ///< the stack size
};

inline co_stack::co_stack(void *vp, size_t size)
    : vp_{vp}, size_{size}
{
}

inline boost::context::stack_context co_stack::allocate()
{
    boost::context::stack_context ctx;
    ctx.size = size_;
    ctx.sp = static_cast<char *>(vp_) + ctx.size;
    return ctx;
}

inline void co_stack::deallocate(boost::context::stack_context &)
{
}