#include "api/HealthController.hpp"
#include "api/HedgingController.hpp"
#include "api/MonteCarloController.hpp"
#include "api/PricingController.hpp"
#include "api/WebController.hpp"
#include "config/ServerConfig.hpp"
#include "ope/version.hpp"
#include "service/HedgingService.hpp"
#include "service/MonteCarloService.hpp"
#include "service/PricingService.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <pthread.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <exception>
#include <iostream>
#include <string>
#include <thread>

using nlohmann::json;
using ope::server::applyServerLimits;
using ope::server::HealthController;
using ope::server::HedgingController;
using ope::server::HedgingService;
using ope::server::MonteCarloController;
using ope::server::MonteCarloService;
using ope::server::PricingController;
using ope::server::PricingService;
using ope::server::ServerConfig;
using ope::server::WebController;
using std::cerr;
using std::cout;
using std::exception;
using std::flush;
using std::thread;
using std::chrono::duration;
using std::chrono::steady_clock;

namespace {

// httplib handles each request start to finish on one worker thread.
thread_local steady_clock::time_point requestStart;

void logRequest(const httplib::Request& req, const httplib::Response& res) {
    const double ms = duration<double, std::milli>(steady_clock::now() - requestStart).count();
    const json line = {{"method", req.method}, {"path", req.path}, {"status", res.status},
                       {"ms", static_cast<double>(static_cast<long long>(ms * 1000)) / 1000}};
    cout << line.dump() << '\n' << flush;
}

}  // namespace

int main() {
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &signals, nullptr);

    ServerConfig config;
    try {
        config = ServerConfig::fromEnv(OPE_WEB_DIR);
    } catch (const exception& e) {
        cerr << "error: " << e.what() << '\n';
        return 1;
    }

    const PricingService service{};
    const PricingController pricing(service);
    const HedgingService hedgingService{};
    const HedgingController hedging(hedgingService);
    const MonteCarloService monteCarloService{};
    const MonteCarloController monteCarlo(monteCarloService);
    const HealthController health(config.commit);
    const WebController web(config.webDir);

    httplib::Server server;
    applyServerLimits(server);
    health.registerRoutes(server);
    pricing.registerRoutes(server);
    hedging.registerRoutes(server);
    monteCarlo.registerRoutes(server);
    try {
        web.registerRoutes(server);
    } catch (const exception& e) {
        cerr << "error: " << e.what() << '\n';
        return 1;
    }

    server.set_pre_routing_handler([](const httplib::Request&, httplib::Response&) {
        requestStart = steady_clock::now();
        return httplib::Server::HandlerResponse::Unhandled;
    });
    server.set_logger(logRequest);

    thread shutdown([&server, signals] {
        int signal = 0;
        sigwait(&signals, &signal);
        cout << "ope-server shutting down (signal " << signal << ")\n" << flush;
        server.stop();
    });

    cout << "ope-server " << ope::version() << " (commit " << config.commit
         << ") listening on 0.0.0.0:" << config.port << " (web: " << config.webDir << ")\n"
         << flush;
    const bool ok = server.listen("0.0.0.0", config.port);
    if (!ok) {
        cerr << "error: could not listen on port " << config.port << '\n';
        kill(getpid(), SIGTERM);  // wake the shutdown thread so it can be joined
    }
    shutdown.join();
    return ok ? 0 : 1;
}
