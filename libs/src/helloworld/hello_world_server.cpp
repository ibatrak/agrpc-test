
#include "helloworld/hello_world_server.h"

// std
#include <cassert>
#include <stdexcept>

// boost
#include "boost/asio/io_context.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/executor_work_guard.hpp>

// grpc
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

// agrpc
#include <agrpc/grpc_context.hpp>
#include <agrpc/register_awaitable_rpc_handler.hpp>
#include <agrpc/register_callback_rpc_handler.hpp>
#include <agrpc/run.hpp>

// gen
#include "helloworld.grpc.pb.h"

struct RethrowFirstArg
{
    template <class... T>
    void operator()(std::exception_ptr ep, T&&...)
    {
        if (ep)
        {
            std::rethrow_exception(ep);
        }
    }

    template <class... T>
    void operator()(T&&...)
    {
    }
};

using rpc_say_hello = agrpc::ServerRPC<&helloworld::Greeter::AsyncService::RequestSayHello>;

struct hello_world_server::grpc_impl
{
    grpc_impl()
        : grpc_ctx(builder.AddCompletionQueue())
    {
    }

    helloworld::Greeter::AsyncService service;
    std::unique_ptr<grpc::Server> server;
    grpc::ServerBuilder builder;
    agrpc::GrpcContext grpc_ctx;
};

hello_world_server::hello_world_server(boost::asio::io_context& ioc)
    : ioc_(ioc)
    , impl_(std::make_unique<hello_world_server::grpc_impl>())
    , shutdown_(false)
{
}

hello_world_server::~hello_world_server()
{
    if (shutdown_thread_.joinable())
    {
        shutdown_thread_.join();
    }
    else
    {
        impl_->server->Shutdown();
    }
}

void hello_world_server::start()
{
    impl_->builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    impl_->builder.RegisterService(&impl_->service);
    impl_->server = impl_->builder.BuildAndStart();
    if (!impl_->server)
    {
        throw std::logic_error("Unable to start gRPC server");
    }

    // agrpc::register_awaitable_rpc_handler<rpc_say_hello>(
    //     impl_->grpc_ctx, impl_->service,
    //     [&](rpc_say_hello& rpc, helloworld::HelloRequest& request) -> boost::asio::awaitable<void>
    //     {
    //         std::clog << "received: " << request.name() << ", preparing response...\n";
    //         helloworld::HelloReply response;
    //         response.set_message("Hello" + request.name());
    //         co_await rpc.finish(response, grpc::Status::OK);
    //     },
    //     RethrowFirstArg{});

    agrpc::register_callback_rpc_handler<rpc_say_hello>(
        impl_->grpc_ctx, impl_->service,
        [&](rpc_say_hello::Ptr rpc_ptr, rpc_say_hello::Request& request)
        {
            std::clog << "received: " << request.name() << ", preparing response...\n";
            rpc_say_hello::Response response;
            response.set_message("Hello " + request.name());
            auto& rpc = *rpc_ptr;
            return rpc.finish(response, grpc::Status::OK,
                              [p = std::move(rpc_ptr)](bool ok)
                              {
                                  if (ok)
                                  {
                                      std::clog << "response has been sent\n";
                                  }
                                  else
                                  {
                                      std::cerr << "failed to send response\n";
                                  }
                              });
        },
        [](std::exception_ptr ep)
        {
            try
            {
                if (ep)
                {
                    std::rethrow_exception(ep);
                }
            }
            catch (std::exception const& ex)
            {
                std::cerr << "RPC handler exception: " << ex.what() << std::endl;
            }
        });

    boost::asio::post(ioc_,
                      [&]()
                      {
                          ioc_.get_executor().on_work_finished();
                          agrpc::run(impl_->grpc_ctx, ioc_);
                          ioc_.get_executor().on_work_started();
                      });
}

void hello_world_server::stop()
{
    if (!shutdown_.exchange(true))
    {
        shutdown_thread_ = std::thread([&]() { impl_->server->Shutdown(); });
    }
}
