#include "http_request.h"
#include "llhttp.h"

const llhttp_settings_t http_request::settings_ = {
    .on_message_begin                  = &http_request::on_message_begin,
    .on_url                            = &http_request::on_url,
    .on_status                         = &http_request::on_status,
    .on_method                         = &http_request::on_method,
    .on_version                        = &http_request::on_version,
    .on_header_field                   = &http_request::on_field_header,
    .on_header_value                   = &http_request::on_field_value,
    .on_chunk_extension_name           = &http_request::on_chunk_extension_name,
    .on_chunk_extension_value          = &http_request::on_chunk_extension_value,
    .on_headers_complete               = &http_request::on_headers_complete,
    .on_body                           = &http_request::on_body,
    .on_message_complete               = &http_request::on_message_complete,
    .on_url_complete                   = &http_request::on_url_complete,
    .on_status_complete                = &http_request::on_method_complete,
    .on_method_complete                = &http_request::on_method_complete,
    .on_version_complete               = &http_request::on_version_complete,
    .on_header_field_complete          = &http_request::on_header_field_complete,
    .on_header_value_complete          = &http_request::on_header_value_complete,
    .on_chunk_extension_name_complete  = &http_request::on_chunk_extension_name_complete,
    .on_chunk_extension_value_complete = &http_request::on_chunk_extension_value_complete,
    .on_chunk_header                   = &http_request::on_chunk_header,
    .on_chunk_complete                 = &http_request::on_chunk_complete,
    .on_reset                          = &http_request::on_reset
};

int http_request::on_message_begin(llhttp_t *parser)
{
    return HPE_OK;
}

int http_request::on_url(llhttp_t *parser, const char *data, size_t size)
{
    auto & request = *static_cast<http_request *>(parser->data);
    request.url_.append(data, size);
    return HPE_OK;
}

int http_request::on_status(llhttp_t *parser, const char *data, size_t size)
{
    return HPE_OK;
}

int http_request::on_method(llhttp_t *parser, const char *data, size_t size)
{
    return HPE_OK;
}

int http_request::on_version(llhttp_t *parser, const char *data, size_t size)
{
    return HPE_OK;
}

int http_request::on_field_header(llhttp_t *parser, const char *data, size_t size)
{
    return HPE_OK;
}

int http_request::on_field_value(llhttp_t *parser, const char *data, size_t size)
{
    return HPE_OK;
}

int http_request::on_chunk_extension_name(llhttp_t *parser, const char *data, size_t size)
{
    return HPE_OK;
}

int http_request::on_chunk_extension_value(llhttp_t *parser, const char *data, size_t size)
{
    return HPE_OK;
}

int http_request::on_headers_complete(llhttp_t *parser)
{
    return HPE_OK;
}

int http_request::on_body(llhttp_t *parser, const char *data, size_t size)
{
    return HPE_OK;
}

int http_request::on_message_complete(llhttp_t *parser)
{
    auto & request = *static_cast<http_request *>(parser->data);
    request.is_completed_ = true;
    return HPE_OK;
}

int http_request::on_url_complete(llhttp_t *parser)
{
    return HPE_OK;
}

int http_request::on_status_complete(llhttp_t *parser)
{
    return HPE_OK;
}

int http_request::on_method_complete(llhttp_t *parser)
{
    return HPE_OK;
}

int http_request::on_version_complete(llhttp_t *parser)
{
    return HPE_OK;
}

int http_request::on_header_field_complete(llhttp_t *parser)
{
    return HPE_OK;
}

int http_request::on_header_value_complete(llhttp_t *parser)
{
    return HPE_OK;
}

int http_request::on_chunk_extension_name_complete(llhttp_t *parser)
{
    return HPE_OK;
}

int http_request::on_chunk_extension_value_complete(llhttp_t *parser)
{
    return HPE_OK;
}

int http_request::on_chunk_header(llhttp_t *parser)
{
    return HPE_OK;
}

int http_request::on_chunk_complete(llhttp_t *parser)
{
    return HPE_OK;
}

int http_request::on_reset(llhttp_t *parser)
{
    return HPE_OK;
}
