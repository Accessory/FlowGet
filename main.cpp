#include <string>
#include <FlowUtils/FlowArgParser.h>
#include <FlowHttp/Request.h>
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
#include <FlowHttp/FlowHttp3.h>
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

    const std::string dh = fap.getString("dh");
    const std::string key = fap.getString("key");
    const std::string cert = fap.getString("cert");


    Router router;
    router.addRoute(std::make_shared<InfoRoute>());
    router.addRoute(std::make_shared<ValidateRoute>(std::set<std::string> ({"GET"})));
    router.addRoute(std::make_shared<IfModifiedSince>(path));
    router.addRoute(std::make_shared<GetBrotliRoute>(path));
    router.addRoute(std::make_shared<GetRoute>(path));
    router.addRoute(std::make_shared<ListFilesBack>(path));
    router.addRoute(std::make_shared<FileNotFound>());
    FlowHttp3 flowHttp (address, port, router, threadCount, dh, key, cert);
    flowHttp.Run();
    return EXIT_SUCCESS;
}