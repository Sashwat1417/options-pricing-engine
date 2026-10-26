#include "api/HealthController.hpp"
#include "api/PricingController.hpp"
#include "config/ServerConfig.hpp"
#include "ope/version.hpp"
#include "service/PricingService.hpp"

#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <string>
#include <thread>

using nlohmann::json;
using ope::server::applyServerLimits;
using ope::server::HealthController;
using ope::server::PricingController;
using ope::server::PricingService;
using ope::server::ServerConfig;
using std::string;
using std::thread;

namespace {

// Real server configured like main.cpp, on a random localhost port.
class HealthControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        applyServerLimits(server_);
        health_.registerRoutes(server_);
        pricing_.registerRoutes(server_);
        port_ = server_.bind_to_any_port("127.0.0.1");
        ASSERT_GT(port_, 0);
        thread_ = thread([this] { server_.listen_after_bind(); });
        server_.wait_until_ready();
    }

    void TearDown() override {
        server_.stop();
        thread_.join();
    }

    httplib::Client client() const { return httplib::Client("127.0.0.1", port_); }

private:
    const PricingService service_{};
    const PricingController pricing_{service_};
    const HealthController health_{"06dc8b7abc"};
    httplib::Server server_;
    int port_ = 0;
    thread thread_;
};

}  // namespace

TEST_F(HealthControllerTest, HealthReturnsOk) {
    const auto res = client().Get("/health");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(json::parse(res->body), json({{"status", "ok"}}));
}

TEST_F(HealthControllerTest, VersionReportsLibraryVersionAndCommit) {
    const auto res = client().Get("/version");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    const json body = json::parse(res->body);
    EXPECT_EQ(body.at("version"), string(ope::version()));
    EXPECT_EQ(body.at("commit"), "06dc8b7abc");
}

TEST_F(HealthControllerTest, OversizedBodyIs413) {
    const string body(ServerConfig::kMaxPayloadBytes + 1, ' ');
    const auto res = client().Post("/api/v1/price", body, "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 413);
}

TEST_F(HealthControllerTest, SetsNoSniffHeader) {
    const auto res = client().Get("/health");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->get_header_value("X-Content-Type-Options"), "nosniff");
}
