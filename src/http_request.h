#pragma once

#include <llhttp.h>
#include <string>

class http_request
{
public:
    http_request();

    ~http_request() = default;

    http_request(const http_request &) = delete;

    void operator=(const http_request &) = delete;

    http_request(http_request &&) = delete;

    void operator=(http_request &&) = delete;

    int parse(const char *data, size_t size);

    bool is_completed() const;

    const std::string &url() const;

protected:
    static int on_message_begin(llhttp_t *parser);

    static int on_url(llhttp_t *parser, const char *data, size_t size);

    static int on_status(llhttp_t *parser, const char *data, size_t size);

    static int on_method(llhttp_t *parser, const char *data, size_t size);

    static int on_version(llhttp_t *parser, const char *data, size_t size);

    static int on_field_header(llhttp_t *parser, const char *data, size_t size);

    static int on_field_value(llhttp_t *parser, const char *data, size_t size);

    static int on_chunk_extension_name(llhttp_t *parser, const char *data, size_t size);

    static int on_chunk_extension_value(llhttp_t *parser, const char *data, size_t size);

    static int on_headers_complete(llhttp_t *parser);

    static int on_body(llhttp_t *parser, const char *data, size_t size);

    static int on_message_complete(llhttp_t *parser);

    static int on_url_complete(llhttp_t *parser);

    static int on_status_complete(llhttp_t *parser);

    static int on_method_complete(llhttp_t *parser);

    static int on_version_complete(llhttp_t *parser);

    static int on_header_field_complete(llhttp_t *parser);

    static int on_header_value_complete(llhttp_t *parser);

    static int on_chunk_extension_name_complete(llhttp_t *parser);

    static int on_chunk_extension_value_complete(llhttp_t *parser);

    static int on_chunk_header(llhttp_t *parser);

    static int on_chunk_complete(llhttp_t *parser);

    static int on_reset(llhttp_t *parser);

private:
    llhttp_t    parser_{ };
    std::string url_{ };
    std::string body_{ };
    bool        is_completed_{ false };

    static const llhttp_settings_t settings_;
};

inline http_request::http_request()
{
    // initialize parser
    llhttp_init(&parser_, HTTP_REQUEST, &settings_);

    // set parser's private data
    parser_.data = this;
}

inline int http_request::parse(const char *data, size_t size)
{
    auto const err = llhttp_execute(&parser_, data, size);
    if (err != HPE_OK) {
        return -1;
    }

    return 0;
}

inline bool http_request::is_completed() const
{
    return is_completed_;
}

inline const std::string &http_request::url() const
{
    return url_;
}