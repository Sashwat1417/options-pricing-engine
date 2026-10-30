#include "api/PricingController.hpp"
#include "service/PricingService.hpp"

#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <cmath>
#include <string>
#include <thread>
#include <vector>

using nlohmann::json;
using ope::server::PricingController;
using ope::server::PricingService;
using std::atomic;
using std::fabs;
using std::string;
using std::thread;
using std::vector;

namespace {

// Hull Ch 15 worked example.
const string kHullCall =
    R"({"type":"call","spot":42,"strike":40,"expiry":0.5,"rate":0.1,"vol":0.2})";
const string kHullIv =
    R"({"type":"call","spot":42,"strike":40,"expiry":0.5,"rate":0.1,"price":4.76})";
const string kNegativeVol =
    R"({"type":"call","spot":42,"strike":40,"expiry":0.5,"rate":0.1,"vol":-0.2})";

// Real server on a random localhost port, one per test.
class PricingControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        controller_.registerRoutes(server_);
        port_ = server_.bind_to_any_port("127.0.0.1");
        ASSERT_GT(port_, 0);
        thread_ = thread([this] { server_.listen_after_bind(); });
        server_.wait_until_ready();
    }

    void TearDown() override {
        server_.stop();
        thread_.join();
    }

    httplib::Result get(const char* path) const { return client().Get(path); }

    httplib::Result post(const char* path, const string& body) const {
        return client().Post(path, body, "application/json");
    }

    httplib::Client client() const { return httplib::Client("127.0.0.1", port_); }

private:
    const PricingService service_{};
    const PricingController controller_{service_};
    httplib::Server server_;
    int port_ = 0;
    thread thread_;
};

}  // namespace

TEST_F(PricingControllerTest, PriceReturnsHullValueAsJson) {
    const auto res = post("/api/v1/price", kHullCall);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->get_header_value("Content-Type"), "application/json");
    EXPECT_NEAR(json::parse(res->body).at("price").get<double>(), 4.759422392871535, 1e-12);
}

TEST_F(PricingControllerTest, IvReturnsHullVolatility) {
    const auto res = post("/api/v1/iv", kHullIv);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_NEAR(json::parse(res->body).at("implied_vol").get<double>(), 0.200066, 1e-6);
}

TEST_F(PricingControllerTest, SweepReturnsArrays) {
    const auto res = post("/api/v1/sweep", R"({"spot":42,"strike":40,"expiry":0.5,"rate":0.1,
        "vol":0.2,"vary":"spot","from":30,"to":50,"steps":3})");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(json::parse(res->body).at("x"), json({30.0, 40.0, 50.0}));
}

TEST_F(PricingControllerTest, GreeksReturnsAllGreeks) {
    const auto res = post("/api/v1/greeks", R"({"type":"put","spot":49,"strike":50,
        "expiry":0.3846,"rate":0.05,"vol":0.2})");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    const json body = json::parse(res->body);
    EXPECT_NEAR(body.at("delta").get<double>(), -0.4783983660284239, 1e-12);
    EXPECT_NEAR(body.at("theta").get<double>(), -1.853005672196868, 1e-11);
    EXPECT_NEAR(body.at("gamma").get<double>(), 0.06554537725247868, 1e-12);
    EXPECT_NEAR(body.at("vega").get<double>(), 12.105242754243843, 1e-10);
    EXPECT_NEAR(body.at("rho").get<double>(), -9.95716587794938, 1e-10);
}

TEST_F(PricingControllerTest, MalformedJsonIs400) {
    const auto res = post("/api/v1/price", "{oops");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    EXPECT_EQ(json::parse(res->body).at("error").get<string>().rfind("invalid JSON", 0), 0u);
}

TEST_F(PricingControllerTest, MissingFieldIs400) {
    const auto res = post("/api/v1/price", R"({"type":"call","spot":42})");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    EXPECT_EQ(json::parse(res->body).at("error"), "missing field: strike");
}

TEST_F(PricingControllerTest, LibraryValidationErrorIs400) {
    const auto res = post("/api/v1/price", kNegativeVol);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    EXPECT_NE(json::parse(res->body).at("error").get<string>().find("volatility"), string::npos);
}

TEST_F(PricingControllerTest, UnknownRouteIs404) {
    const auto res = get("/api/v1/nope");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST_F(PricingControllerTest, WrongMethodIsNotServed) {
    const auto res = get("/api/v1/price");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST_F(PricingControllerTest, HandlesConcurrentRequests) {
    constexpr int kThreads = 8;
    constexpr int kRequestsPerThread = 25;
    atomic<int> correct{0};

    vector<thread> clients;
    for (int t = 0; t < kThreads; ++t) {
        clients.emplace_back([this, &correct] {
            for (int i = 0; i < kRequestsPerThread; ++i) {
                const auto res = post("/api/v1/price", kHullCall);
                if (res && res->status == 200 &&
                    fabs(json::parse(res->body).at("price").get<double>() - 4.759422392871535) <
                        1e-12) {
                    ++correct;
                }
            }
        });
    }
    for (auto& c : clients) {
        c.join();
    }
    EXPECT_EQ(correct.load(), kThreads * kRequestsPerThread);
}
