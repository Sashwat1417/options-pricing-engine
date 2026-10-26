#include "config/ServerConfig.hpp"

#include <sys/socket.h>

#include <cstdlib>
#include <stdexcept>

using std::getenv;
using std::invalid_argument;
using std::stoi;
using std::string;

namespace ope::server {

namespace {

int parsePort(const string& text) {
    const string error = "PORT must be an integer in [1, 65535], got \"" + text + "\"";
    size_t used = 0;
    int port = 0;
    try {
        port = stoi(text, &used);
    } catch (const std::exception&) {
        throw invalid_argument(error);
    }
    if (used != text.size() || port < 1 || port > 65535) {
        throw invalid_argument(error);
    }
    return port;
}

}  // namespace

ServerConfig ServerConfig::fromEnv(const string& defaultWebDir) {
    ServerConfig config;
    config.webDir = defaultWebDir;
    config.commit = "unknown";
    if (const char* port = getenv("PORT")) {
        config.port = parsePort(port);
    }
    if (const char* webDir = getenv("WEB_DIR")) {
        config.webDir = webDir;
    }
    if (const char* commit = getenv("RENDER_GIT_COMMIT")) {
        config.commit = commit;
    }
    return config;
}

void applyServerLimits(httplib::Server& server) {
    server.set_payload_max_length(ServerConfig::kMaxPayloadBytes);
    server.set_read_timeout(ServerConfig::kTimeoutSeconds, 0);
    server.set_write_timeout(ServerConfig::kTimeoutSeconds, 0);
    server.set_default_headers({{"X-Content-Type-Options", "nosniff"}});
    // httplib's default adds SO_REUSEPORT, which lets a second process silently share the
    // port. Keep SO_REUSEADDR only, so a busy port is an error.
    server.set_socket_options([](socket_t sock) {
        const int yes = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, static_cast<socklen_t>(sizeof(yes)));
    });
}

}  // namespace ope::server
