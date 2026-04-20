#include <gtest/gtest.h>
#include <memory>
#include "src/parsing/ProcessIsolatedParser.h"
#include "HoocParser.h"

using namespace hooc;

class NetworkModuleParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<ProcessIsolatedParser>();
    }

    std::unique_ptr<ProcessIsolatedParser> parser;
};

TEST_F(NetworkModuleParsingTest, ImportNetworkModule) {
    std::string code = R"(
        import hoo.net;
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(NetworkModuleParsingTest, URLClassInstantiation) {
    std::string code = R"(
        import hoo.net;

        func:string getHost() { var url = new hoo.net.URL("https://example.com:8080/path"); return url.getHost(); }
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(NetworkModuleParsingTest, URLGetScheme) {
    std::string code = R"(
        import hoo.net;

        func:string getScheme() { var url = new hoo.net.URL("https://example.com"); return url.getScheme(); }
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(NetworkModuleParsingTest, URLGetPort) {
    std::string code = R"(
        import hoo.net;

        func:int64 getPort() { var url = new hoo.net.URL("https://example.com:8443/api"); return url.getPort(); }
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(NetworkModuleParsingTest, HttpClientGetRequest) {
    std::string code = R"(
        import hoo.net;

        func:int64 fetch() { var client = new hoo.net.HttpClient(); var response = client.get("https://example.com/api"); return response.getStatusCode(); }
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(NetworkModuleParsingTest, HttpClientPostRequest) {
    std::string code = R"(
        import hoo.net;

        func:string postData() { var client = new hoo.net.HttpClient(); var response = client.post("https://example.com/api", "{\"name\":\"test\"}"); return response.getBody(); }
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(NetworkModuleParsingTest, HttpClientSetHeader) {
    std::string code = R"(
        import hoo.net;

        func:bool testHeader() { var client = new hoo.net.HttpClient(); return client.setHeader("Content-Type", "application/json"); }
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(NetworkModuleParsingTest, HttpResponseIsSuccess) {
    std::string code = R"(
        import hoo.net;

        func:bool checkSuccess() { var client = new hoo.net.HttpClient(); var response = client.get("https://example.com/success"); return response.isSuccess(); }
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(NetworkModuleParsingTest, FullURLChain) {
    std::string code = R"(
        import hoo.net;

        func:string parseURL() { var url = new hoo.net.URL("https://user:pass@example.com:443/api/v1?query=test#section"); return url.toString(); }
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}