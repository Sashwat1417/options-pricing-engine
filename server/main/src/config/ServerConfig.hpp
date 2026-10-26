#pragma once

#include <httplib.h>

#include <cstddef>
#include <ctime>
#include <string>

namespace ope::server {

struct ServerConfig {
    static constexpr std::size_t kMaxPayloadBytes = 64 * 1024;
    static constexpr std::time_t kTimeoutSeconds = 5;

    int port = 8080;
    std::string webDir;
    std::string commit;  // deployed git commit, "unknown" if not set

    // PORT, WEB_DIR, RENDER_GIT_COMMIT. Throws std::invalid_argument on a bad PORT.
    static ServerConfig fromEnv(const std::string& defaultWebDir);
};

// Payload cap (413 above it), read/write timeouts, SO_REUSEADDR only, security headers.
void applyServerLimits(httplib::Server& server);

}  // namespace ope::server
