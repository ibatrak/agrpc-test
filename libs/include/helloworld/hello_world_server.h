#pragma once

#include <memory>
#include <thread>

namespace boost::asio
{
class io_context;
}

class hello_world_server
{
  public:
    hello_world_server(boost::asio::io_context& ioc);
    ~hello_world_server();

    void start();
    void stop();

  private:
    struct grpc_impl;

    boost::asio::io_context& ioc_;
    std::unique_ptr<grpc_impl> impl_;
    std::thread shutdown_thread_;
    std::atomic<bool> shutdown_;
};
