#include "event_dispatcher.h"
#include "server.h"

#include <cassert>
#include <iostream>

int main(int argc, char ** argv)
{
    event_dispatcher dispatcher;
    assert(dispatcher);
    std::cout << "dispatcher created" << std::endl;

    server svr{ dispatcher, argv[1] };
    assert(svr);
    std::cout << "server created" << std::endl;

    if (!dispatcher.subscribe(svr, event_dispatcher::readable)) {
        std::cerr << "cannot subscribe io event for server" << std::endl;
        return 1;
    }

    dispatcher.run();
}