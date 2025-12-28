
#include <cstdlib>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <utility>

#include "helloworld/hello_world_server.h"

#include "base/callback_registry.h"

template <typename R, typename... Args, typename Class>
auto make_member_callback(R (Class::*method)(Args...), Class* obj)
{
    return std::function<R(Args...)>([obj, method](Args... args) { return (obj->*method)(std::forward<Args>(args)...); });
}

template <typename R, typename... Args, typename Class>
auto make_member_callback(R (Class::*method)(Args...), Class& obj)
{
    return std::function<R(Args...)>([&obj, method](Args... args) { return (obj.*method)(std::forward<Args>(args)...); });
}

enum callback_kind
{
    open_stream,
    close_stream
};

template <>
struct std::formatter<callback_kind> : std::formatter<std::string_view>
{
    auto format(callback_kind kind, std::format_context& ctx) const
    {
        std::string_view name;
        switch (kind)
        {
            case open_stream:
                name = "open_stream";
                break;
            case close_stream:
                name = "close_stream";
                break;
            default:
                name = "unknown";
                break;
        }
        return std::formatter<std::string_view>::format(name, ctx);
    }
};

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

std::string on_open_stream(std::string id, int32_t param)
{
    std::cout << "on_open_stream: id=" << id << ", param=" << param << '\n';
    return "00000000";
}

void on_close_stream(std::string id) { std::cout << "on_close_stream: id=" << id << '\n'; }

class X
{
  public:
    std::string on_open_stream(std::string id, int32_t param)
    {
        std::cout << "X::on_open_stream: id=" << id << ", param=" << param << '\n';
        return "111111111111";
    }
};

int main(int argc, char** argv)
{
    X x;
    callback_registry<callback_kind> cr;
    // cr.register_callback(callback_kind::open_stream, on_open_stream);
    cr.register_callback(callback_kind::close_stream, [](std::string id) { std::cout << "on_close_stream: id=" << id << '\n'; });

    // std::function<std::string(std::string, int32_t)> func = on_open_stream;
    // cr.register_callback(callback_kind::open_stream, func);

    // std::function<std::string(std::string, int32_t)> bound = std::bind(&on_open_stream, std::placeholders::_1, std::placeholders::_2);
    // cr.register_callback(callback_kind::open_stream, bound);

    // cr.register_callback(callback_kind::open_stream,
    //                      std::function<std::string(std::string, int32_t)>(std::bind(&on_open_stream, std::placeholders::_1, std::placeholders::_2)));

    // cr.register_callback(callback_kind::open_stream, [&x](std::string key, int32_t p) { return x.on_open_stream(key, p); });

    cr.register_callback(callback_kind::open_stream, make_member_callback(&X::on_open_stream, x));

    auto id = cr.invoke<std::string>(callback_kind::open_stream, "X1234567X", 20);
    std::cout << "invoking result: " << id << '\n';
    cr.invoke(callback_kind::close_stream, "0xBEBEFF");

    return 0;

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
