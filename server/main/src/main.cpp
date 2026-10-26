#include "api/PricingController.hpp"
#include "service/PricingService.hpp"

#include <httplib.h>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

using ope::server::PricingController;
using ope::server::PricingService;
using std::cerr;
using std::cout;
using std::exception;
using std::flush;
using std::getenv;
using std::stoi;

int main() {
    int port = 8080;
    try {
        if (const char* env = getenv("PORT")) {
            port = stoi(env);
        }
    } catch (const exception&) {
        cerr << "error: PORT must be a number\n";
        return 1;
    }

    const PricingService service;
    const PricingController controller(service);

    httplib::Server server;
    controller.registerRoutes(server);
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        cout << req.method << ' ' << req.path << ' ' << res.status << '\n' << flush;
    });

    cout << "ope-server listening on 0.0.0.0:" << port << '\n' << flush;
    if (!server.listen("0.0.0.0", port)) {
        cerr << "error: could not listen on port " << port << '\n';
        return 1;
    }
    return 0;
}
