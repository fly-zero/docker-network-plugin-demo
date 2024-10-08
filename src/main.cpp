#include "event_dispatcher.h"
#include "server.h"

#include <cassert>

#include <iostream>

inline bool is_all_digit(const char * str)
{
    for (auto p = str; *p; ++p) {
        if (!isdigit(*p)) {
            return false;
        }
    }

    return true;
}

int main(int argc, char ** argv)
{
    // create event dispatcher
    event_dispatcher dispatcher;
    assert(dispatcher);
    std::cout << "dispatcher created" << std::endl;

    // create server
    char server_storage[sizeof(server)] __attribute__((aligned(alignof(server))));

    // init server
    if (is_all_digit(argv[1])) {
        // init server with port
        ::new (server_storage) server{ dispatcher, static_cast<unsigned short>(atoi(argv[1])) };
    } else {
        // init server with path
        ::new (server_storage) server{ dispatcher, argv[1] };
    }

    auto & svr = *reinterpret_cast<server *>(server_storage);
    std::cout << "server created" << std::endl;

    // docker network plugin api, refer: https://github.com/moby/moby/blob/master/libnetwork/docs/remote.md

    // handshake
    // register plugin activate handler
    svr.register_uri_handler("/Plugin.Activate", [](void *, const http_request &request, http_response * response) {
        std::cout << "request: " << request.url() << std::endl;
        response->status(200);
        response->body() = R"({"Implements":["NetworkDriver"]})";
        return true;
    }, nullptr);

    // set capabilities
    // register network driver get capabilities handler
    svr.register_uri_handler("/NetworkDriver.GetCapabilities", [](void *, const http_request &request, http_response * response) {
        std::cout << "request: " << request.url() << std::endl;
        response->status(200);
        response->body() = R"({"Scope":"local"})";
        return true;
    }, nullptr);

    // create network
    // register network driver create network handler
    svr.register_uri_handler("/NetworkDriver.CreateNetwork", [](void *, const http_request &request, http_response * response) {
        std::cout << "request: " << request.url() << std::endl;
        response->status(200);
        response->body() = R"({})";
        return true;
    }, nullptr);

    // delete network
    // register network driver delete network handler
    svr.register_uri_handler("/NetworkDriver.DeleteNetwork", [](void *, const http_request &request, http_response * response) {
        std::cout << "request: " << request.url() << std::endl;
        response->status(200);
        response->body() = R"({})";
        return true;
    }, nullptr);

    // create endpoint
    // register network driver create endpoint handler
    svr.register_uri_handler("/NetworkDriver.CreateEndpoint", [](void *, const http_request &request, http_response * response) {
        std::cout << "request: " << request.url() << std::endl;
        response->status(200);
        response->body() = R"({})";
        return true;
    }, nullptr);

    // delete endpoint
    // register network driver delete endpoint handler
    svr.register_uri_handler("/NetworkDriver.DeleteEndpoint", [](void *, const http_request &request, http_response * response) {
        std::cout << "request: " << request.url() << std::endl;
        response->status(200);
        response->body() = R"({})";
        return true;
    }, nullptr);

    // join
    // register network driver join handler
    svr.register_uri_handler("/NetworkDriver.Join", [](void *, const http_request &request, http_response * response) {
        std::cout << "request: " << request.url() << std::endl;
        response->status(200);
        response->body() = R"({"InterfaceName":{"SrcName":"ens160","DstPrefix":"eth"}})"; // Docker libNetwork will move host interface
                                                                                          // with name "ens160" to container, and rename
                                                                                          // it to "ethN", where N is a index number.
        return true;
    }, nullptr);

    // leave
    // register network driver leave handler
    svr.register_uri_handler("/NetworkDriver.Leave", [](void *, const http_request &request, http_response * response) {
        std::cout << "request: " << request.url() << std::endl;
        response->status(200);
        response->body() = R"({})";
        return true;
    }, nullptr);

    // endpoint operational info
    // register network driver endpoint operational info handler
    svr.register_uri_handler("/NetworkDriver.EndpointOperInfo", [](void *, const http_request &request, http_response * response) {
        std::cout << "request: " << request.url() << std::endl;
        response->status(200);
        response->body() = R"({"Value":{}})";
        return true;
    }, nullptr);

    // subscribe server io event
    if (!dispatcher.subscribe(svr, event_dispatcher::readable)) {
        std::cerr << "cannot subscribe io event for server" << std::endl;
        return 1;
    }

    // subscribe server loop event
    if (!dispatcher.subscribe(svr)) {
        std::cerr << "cannot subscribe loop event for server" << std::endl;
        return 1;
    }

    // run dispatcher
    dispatcher.run();

    // unsubscribe server loop event
    dispatcher.unsubscribe(static_cast<loop_listener &>(svr));

    // unsubscribe server io event
    dispatcher.unsubscribe(static_cast<io_listener &>(svr));

    // destroy server
    svr.~server();

    return 0;
}