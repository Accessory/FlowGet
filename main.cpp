#include <string>
#include <FlowUtils/FlowArgParser.h>
#include <FlowHttp/Request.h>
#include <FlowHttp/Response.h>
#include <thread>
#include <FlowUtils/WorkerPool.h>
#include <FlowHttp/routes/Router.h>
#include <FlowHttp/routes/FileNotFound.h>
#include <FlowHttp/routes/InfoRoute.h>
#include <FlowHttp/routes/GetRoute.h>
#include <FlowHttp/routes/GetBrotliRoute.h>
#include <FlowHttp/routes/ValidateRoute.h>
#include <FlowHttp/routes/IfModifiedSince.h>
#include <FlowHttp/routes/ListFilesBack.h>
#include <memory>
#include <FlowHttp/util/ArgParserUtil.h>

int main(int argc, char *argv[]) {
    FlowArgParser fap = ArgParserUtil::defaultArgParser();;
    fap.parse("localhost 1337 .");
    fap.parse(argc, argv);

    std::string address = fap.getString("address");
    std::string port = fap.getString("port");
    std::string path = fap.getString("path");
    FlowString::replaceAll(path, "\\", "/");

    LOG_INFO << "Listening on: " << address << ":" << port;
    LOG_INFO << "Path: " << path;

    size_t threadCount = fap.hasOption("threads") ? std::stoul(fap.getString("threads"), nullptr, 10) :
                         std::thread::hardware_concurrency() * 2;

    WorkerPool workerPool(threadCount);

    boost::asio::io_context io_context;
    unique_ptr<boost::asio::ssl::context> ssl_context;
    bool useSSL = false;

    if (fap.hasOption("dh") && fap.hasOption("key") && fap.hasOption("cert")) {
        useSSL = true;
        ssl_context = std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

        ssl_context->set_options(
                boost::asio::ssl::context::default_workarounds
                | boost::asio::ssl::context::no_sslv2);

        ssl_context->use_certificate_chain_file(fap.getString("cert"));
        ssl_context->use_private_key_file(fap.getString("key"),
                                          boost::asio::ssl::context::pem);
        ssl_context->use_tmp_dh_file(fap.getString("dh"));
    }

    boost::asio::ip::tcp::resolver::query query(address, port);
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto resolverResult = resolver.resolve(query);

    boost::asio::ip::tcp::acceptor acceptor(io_context, resolverResult.begin()->endpoint());
    acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    Semaphore semaphore;



    Router router;
    router.addRoute(std::make_shared<ValidateRoute>());
    router.addRoute(std::make_shared<InfoRoute>());
    router.addRoute(std::make_shared<IfModifiedSince>(path));
    router.addRoute(std::make_shared<GetBrotliRoute>(path));
    router.addRoute(std::make_shared<GetRoute>(path));
    router.addRoute(std::make_shared<ListFilesBack>(path));
    router.addRoute(std::make_shared<FileNotFound>());
    size_t keepRunning = 10;
    while (keepRunning) {
        semaphore.lock();
        workerPool.addTask(make_shared<function<void() >>([&] { // Workerfunction
            boost::system::error_code error;

            Socket socket(io_context);

            acceptor.accept(*socket.GetSocket(), error);
            if (useSSL) {
                socket.SetSSL(*ssl_context);
                socket.GetSSLSocket()->handshake(boost::asio::ssl::stream_base::server, error);
                if (error) {
                    LOG_WARNING << "Bad Request";
                    semaphore.unlock();
                    return;
                }
            }
            semaphore.unlock();
            auto req = FlowAsio::readBytes(socket);
//            LOG_INFO << std::string(req.begin(), req.end());
            Request request;
            request << req;

            Response response(socket.IsSSL());
            router.execRoute(request, response, socket);
        })); // Workerfunction
        workerPool.start();
    }

    workerPool.join();

    return 0;
}