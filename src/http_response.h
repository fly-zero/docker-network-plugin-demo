#pragma once

#include <string>

class http_response
{
public:
    /**
     * @brief Construct a new http response object
     */
    http_response() = default;

    /**
     * @brief Destroy the http response object
     */
    ~http_response() = default;

    /**
     * @brief get the response body
     */
    std::string &body();

    /**
     * @brief get the response body
     */
    const std::string &body() const;

    /**
     * @brief get the response status
     */
    unsigned status() const;

    /**
     * @brief set the response status
     */
    void status(unsigned status);

private:
    std::string body_{};
    unsigned    status_{ 0 };
};

inline std::string &http_response::body()
{
    return body_;
}

inline const std::string &http_response::body() const
{
    return body_;
}

inline unsigned http_response::status() const
{
    return status_;
}

inline void http_response::status(unsigned status)
{
    status_ = status;
}
