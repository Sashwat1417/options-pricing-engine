#include "api/PricingController.hpp"
#include "api/WebController.hpp"
#include "service/PricingService.hpp"

#include <gtest/gtest.h>
#include <httplib.h>

#include <stdexcept>
#include <string>
#include <thread>

using ope::server::PricingController;
using ope::server::PricingService;
using ope::server::WebController;
using std::runtime_error;
using std::string;
using std::thread;

namespace {

// Real server with both controllers, like main.cpp, on a random localhost port.
class WebControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        pricing_.registerRoutes(server_);
        web_.registerRoutes(server_);
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
    const WebController web_{OPE_WEB_DIR};
    httplib::Server server_;
    int port_ = 0;
    thread thread_;
};

}  // namespace

TEST_F(WebControllerTest, RootServesIndexHtml) {
    const auto res = client().Get("/");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->get_header_value("Content-Type").rfind("text/html", 0), 0u);
    EXPECT_NE(res->body.find("<title>Options Pricing Engine</title>"), string::npos);
}

TEST_F(WebControllerTest, ServesScriptAndStylesheet) {
    const auto js = client().Get("/app.js");
    ASSERT_TRUE(js);
    EXPECT_EQ(js->status, 200);
    EXPECT_NE(js->get_header_value("Content-Type").find("javascript"), string::npos);

    const auto css = client().Get("/app.css");
    ASSERT_TRUE(css);
    EXPECT_EQ(css->status, 200);
    EXPECT_EQ(css->get_header_value("Content-Type").rfind("text/css", 0), 0u);
}

TEST_F(WebControllerTest, ApiStillRoutedAlongsideStaticFiles) {
    const auto res = client().Post(
        "/api/v1/price",
        R"({"type":"call","spot":42,"strike":40,"expiry":0.5,"rate":0.1,"vol":0.2})",
        "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
}

TEST_F(WebControllerTest, MissingFileIs404) {
    const auto res = client().Get("/nope.html");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST(WebControllerConfig, ThrowsWhenDirectoryMissing) {
    httplib::Server server;
    EXPECT_THROW(WebController("/no/such/dir").registerRoutes(server), runtime_error);
}
