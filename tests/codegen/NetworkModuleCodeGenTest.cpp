#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "src/jit/HoocJIT.h"

using namespace hooc;

class NetworkModuleCodeGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        jit = std::make_unique<HoocJIT>();
    }

    std::unique_ptr<HoocJIT> jit;
};

TEST_F(NetworkModuleCodeGenTest, CreateHttpClient) {
    std::string code = R"(
        func main() { 
            var client = new hoo.net.HttpClient(); 
        }
    )";
    auto result = jit->compile("test", code);
    EXPECT_TRUE(result.success) << "Error: " << result.error;
}