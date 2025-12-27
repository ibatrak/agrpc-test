
#include <cstdlib>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include "helloworld/hello_world_server.h"

void on_handle_signal(boost::system::error_code const& ec, int32_t signal, hello_world_server& server)
{
    std::clog << "signal SIG" << boost::algorithm::to_upper_copy(std::string(sys_signame[signal])) << " received...\n";
    if (ec)
    {
        std::cerr << "an error occured, details: " << ec.message() << std::endl;
    }

    std::clog << "shutdown server...\n";
    server.stop();
}

int main(int argc, char** argv)
{
    boost::asio::io_context io_ctx(1);
    boost::asio::signal_set signals(io_ctx, SIGINT, SIGTERM);
    hello_world_server server(io_ctx);

    signals.async_wait(std::bind(&on_handle_signal, std::placeholders::_1, std::placeholders::_2, std::ref(server)));

    std::cout << "start gRPC server..." << std::endl;
    try
    {
        server.start();
        io_ctx.run();
    }
    catch (std::exception const& ex)
    {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
