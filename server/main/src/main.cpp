#include "api/PricingController.hpp"
#include "api/WebController.hpp"
#include "service/PricingService.hpp"

#include <httplib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <thread>

using ope::server::PricingController;
using ope::server::PricingService;
using ope::server::WebController;
using std::cerr;
using std::cout;
using std::exception;
using std::flush;
using std::getenv;
using std::stoi;
using std::string;
using std::thread;

int main() {
    // Block SIGINT/SIGTERM in every thread; a dedicated thread waits for them and stops the
    // server, so `docker stop` and Ctrl-C shut down cleanly instead of being killed.
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &signals, nullptr);

    int port = 8080;
    try {
        if (const char* env = getenv("PORT")) {
            port = stoi(env);
        }
    } catch (const exception&) {
        cerr << "error: PORT must be a number\n";
        return 1;
    }

    const char* webDirEnv = getenv("WEB_DIR");
    const string webDir = webDirEnv ? webDirEnv : OPE_WEB_DIR;

    const PricingService service{};
    const PricingController pricing(service);
    const WebController web(webDir);

    httplib::Server server;
    // httplib's default adds SO_REUSEPORT, which lets a second process silently share the
    // port. Keep SO_REUSEADDR only, so a busy port is an error.
    server.set_socket_options([](socket_t sock) {
        const int yes = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, static_cast<socklen_t>(sizeof(yes)));
    });
    pricing.registerRoutes(server);
    try {
        web.registerRoutes(server);
    } catch (const exception& e) {
        cerr << "error: " << e.what() << '\n';
        return 1;
    }
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        cout << req.method << ' ' << req.path << ' ' << res.status << '\n' << flush;
    });

    thread shutdown([&server, signals] {
        int signal = 0;
        sigwait(&signals, &signal);
        cout << "ope-server shutting down (signal " << signal << ")\n" << flush;
        server.stop();
    });

    cout << "ope-server listening on 0.0.0.0:" << port << " (web: " << webDir << ")\n" << flush;
    const bool ok = server.listen("0.0.0.0", port);
    if (!ok) {
        cerr << "error: could not listen on port " << port << '\n';
        kill(getpid(), SIGTERM);  // wake the shutdown thread so it can be joined
    }
    shutdown.join();
    return ok ? 0 : 1;
}
