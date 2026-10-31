#include "api/HedgingController.hpp"
#include "service/HedgingService.hpp"

#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <string>
#include <thread>

using nlohmann::json;
using ope::server::HedgingController;
using ope::server::HedgingService;
using std::string;
using std::thread;

namespace {

const string kHullPath = R"({"spot":49,"strike":50,"expiry":0.3846,"rate":0.05,"vol":0.2,
    "quantity":100000,"premium":3,"drift":0.13,"rebalances":20,"seed":42})";
const string kHullFrequency = R"({"spot":49,"strike":50,"expiry":0.3846,"rate":0.05,"vol":0.2,
    "quantity":100000,"drift":0.13,"paths":200,"seed":7,"rebalances":[5,20]})";

// Real server on a random localhost port, one per test.
class HedgingControllerTest : public ::testing::Test {
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
    const HedgingService service_{};
    const HedgingController controller_{service_};
    httplib::Server server_;
    int port_ = 0;
    thread thread_;
};

}  // namespace

TEST_F(HedgingControllerTest, PathReturnsHullStyleRows) {
    const auto res = post("/api/v1/hedge/path", kHullPath);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    const json body = json::parse(res->body);
    EXPECT_EQ(body.at("steps").size(), 21u);
    EXPECT_EQ(body.at("seed").get<int>(), 42);
    EXPECT_DOUBLE_EQ(body.at("steps")[0].at("spot").get<double>(), 49.0);
}

TEST_F(HedgingControllerTest, FrequencyReturnsOneRowPerRebalanceCount) {
    const auto res = post("/api/v1/hedge/frequency", kHullFrequency);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(json::parse(res->body).at("rows").size(), 2u);
}

TEST_F(HedgingControllerTest, BadInputIs400) {
    const auto res = post("/api/v1/hedge/path", R"({"spot":49})");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    EXPECT_NE(json::parse(res->body).at("error").get<string>().find("missing field"),
              string::npos);
}
