#include "api/PricingController.hpp"
#include "api/WebController.hpp"
#include "service/PricingService.hpp"

#include <httplib.h>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

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

    const char* webDirEnv = getenv("WEB_DIR");
    const string webDir = webDirEnv ? webDirEnv : OPE_WEB_DIR;

    const PricingService service;
    const PricingController pricing(service);
    const WebController web(webDir);

    httplib::Server server;
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

    cout << "ope-server listening on 0.0.0.0:" << port << " (web: " << webDir << ")\n" << flush;
    if (!server.listen("0.0.0.0", port)) {
        cerr << "error: could not listen on port " << port << '\n';
        return 1;
    }
    return 0;
}
