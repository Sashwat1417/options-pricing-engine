#include "api/MonteCarloController.hpp"
#include "service/MonteCarloService.hpp"

#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <string>
#include <thread>

using nlohmann::json;
using ope::server::MonteCarloController;
using ope::server::MonteCarloService;
using std::string;
using std::thread;

namespace {

const string kHullCall = R"({"type":"call","spot":42,"strike":40,"expiry":0.5,"rate":0.1,
    "vol":0.2,"paths":50000,"seed":42})";

// Real server on a random localhost port, one per test.
class MonteCarloControllerTest : public ::testing::Test {
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

    httplib::Result post(const char* path, const string& body) const {
        return httplib::Client("127.0.0.1", port_).Post(path, body, "application/json");
    }

private:
    const MonteCarloService service_{};
    const MonteCarloController controller_{service_};
    httplib::Server server_;
    int port_ = 0;
    thread thread_;
};

}  // namespace

TEST_F(MonteCarloControllerTest, PriceReturnsEstimateAndBlackScholes) {
    const auto res = post("/api/v1/mc/price", kHullCall);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    const json body = json::parse(res->body);
    for (const char* key : {"price", "std_error", "ci_low", "ci_high", "bs_price", "paths", "seed"}) {
        EXPECT_TRUE(body.contains(key)) << key;
    }
}

TEST_F(MonteCarloControllerTest, ConvergenceReturnsOneRowPerPathCount) {
    json request = json::parse(kHullCall);
    request["paths"] = {1000, 4000};
    const auto res = post("/api/v1/mc/convergence", request.dump());
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(json::parse(res->body).at("rows").size(), 2u);
}

TEST_F(MonteCarloControllerTest, BadInputIs400) {
    json request = json::parse(kHullCall);
    request["expiry"] = 0;
    const auto res = post("/api/v1/mc/price", request.dump());
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    EXPECT_TRUE(json::parse(res->body).contains("error"));
}
