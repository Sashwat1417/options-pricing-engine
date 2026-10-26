#include "config/ServerConfig.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <stdexcept>

using ope::server::ServerConfig;
using std::invalid_argument;

namespace {

class ServerConfigTest : public ::testing::Test {
protected:
    void SetUp() override { clear(); }
    void TearDown() override { clear(); }

    static void clear() {
        unsetenv("PORT");
        unsetenv("WEB_DIR");
        unsetenv("RENDER_GIT_COMMIT");
    }
};

}  // namespace

TEST_F(ServerConfigTest, DefaultsWhenEnvIsEmpty) {
    const auto config = ServerConfig::fromEnv("/default/web");
    EXPECT_EQ(config.port, 8080);
    EXPECT_EQ(config.webDir, "/default/web");
    EXPECT_EQ(config.commit, "unknown");
}

TEST_F(ServerConfigTest, ReadsEnvironment) {
    setenv("PORT", "10000", 1);
    setenv("WEB_DIR", "/app/web", 1);
    setenv("RENDER_GIT_COMMIT", "06dc8b7abc", 1);
    const auto config = ServerConfig::fromEnv("/default/web");
    EXPECT_EQ(config.port, 10000);
    EXPECT_EQ(config.webDir, "/app/web");
    EXPECT_EQ(config.commit, "06dc8b7abc");
}

TEST_F(ServerConfigTest, RejectsBadPorts) {
    for (const char* bad : {"abc", "8080abc", "", "0", "70000", "-1"}) {
        setenv("PORT", bad, 1);
        EXPECT_THROW((void)ServerConfig::fromEnv("/web"), invalid_argument) << "PORT=" << bad;
    }
}
