#include <gtest/gtest.h>

#include <memory>

#include "src/core/DefaultIOProvider.h"
#include "src/hvm/HVMJIT.h"

using namespace hooc;

TEST(HVMJITLifecycleTest, LoadSourceCodeThenDestroy) {
    auto io = std::make_unique<DefaultIOProvider>();
    auto jit = std::make_unique<HVMJIT>(*io);

    ASSERT_TRUE(jit->loadSourceCode("test", "func :int64 test() { return 42; }"))
        << jit->getLastError();
}
