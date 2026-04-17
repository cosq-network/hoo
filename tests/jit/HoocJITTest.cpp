#include <gtest/gtest.h>
#include <string>
#include <utility>
#include "src/jit/HoocJIT.h"

using namespace hooc;

class HoocJITLifecycleTest : public ::testing::Test {};

TEST_F(HoocJITLifecycleTest, DefaultConstruction) {
    EXPECT_NO_THROW({
        HoocJIT localJit;
    });
}

TEST_F(HoocJITLifecycleTest, MoveConstruction) {
    HoocJIT firstJit;
    HoocJIT secondJit(std::move(firstJit));
}

TEST_F(HoocJITLifecycleTest, MoveAssignment) {
    HoocJIT firstJit;
    HoocJIT secondJit;
    firstJit = std::move(secondJit);
}

TEST_F(HoocJITLifecycleTest, SelfMoveAssignment) {
    HoocJIT jit;
    jit = std::move(jit);
}

TEST_F(HoocJITLifecycleTest, NonCopyable) {
    static_assert(!std::is_copy_constructible<HoocJIT>::value,
                  "HoocJIT should not be copy constructible");
    static_assert(!std::is_copy_assignable<HoocJIT>::value,
                  "HoocJIT should not be copy assignable");
}

TEST_F(HoocJITLifecycleTest, MoveAssignable) {
    static_assert(std::is_move_constructible<HoocJIT>::value,
                  "HoocJIT should be move constructible");
    static_assert(std::is_move_assignable<HoocJIT>::value,
                  "HoocJIT should be move assignable");
}

class HoocJITResultTypesTest : public ::testing::Test {};

TEST_F(HoocJITResultTypesTest, CompileResultOk) {
    auto result = CompileResult::ok("test IR");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());
    EXPECT_EQ(result.ir, "test IR");
}

TEST_F(HoocJITResultTypesTest, CompileResultFail) {
    auto result = CompileResult::fail("error message");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "error message");
    EXPECT_TRUE(result.ir.empty());
}

TEST_F(HoocJITResultTypesTest, CompileResultOkEmptyIR) {
    auto result = CompileResult::ok();
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.ir.empty());
}

TEST_F(HoocJITResultTypesTest, ExecutionResultOk) {
    auto result = ExecutionResult::ok();
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());
}

TEST_F(HoocJITResultTypesTest, ExecutionResultFail) {
    auto result = ExecutionResult::fail("execution error");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "execution error");
}

TEST_F(HoocJITResultTypesTest, TypedExecutionResultSuccess) {
    auto result = TypedExecutionResult<int64_t>::successResult(42);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.value, 42);
    EXPECT_TRUE(result.error.empty());
}

TEST_F(HoocJITResultTypesTest, TypedExecutionResultFailure) {
    auto result = TypedExecutionResult<int64_t>::failure("test error");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "test error");
}

TEST_F(HoocJITResultTypesTest, TypedExecutionResultVoidSuccess) {
    auto result = TypedExecutionResult<void>::successResult();
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());
}

TEST_F(HoocJITResultTypesTest, TypedExecutionResultVoidFailure) {
    auto result = TypedExecutionResult<void>::failure("void error");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "void error");
}

class HoocJITAccessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        jit = std::make_unique<HoocJIT>();
    }

    std::unique_ptr<HoocJIT> jit;
};

TEST_F(HoocJITAccessorTest, GetJITReference) {
    auto& jitRef = jit->getJIT();
    EXPECT_NE(&jitRef, nullptr);

    const auto& constJitRef = jit->getJIT();
    EXPECT_NE(&constJitRef, nullptr);
}

TEST_F(HoocJITAccessorTest, GetJITReturnsSameReference) {
    auto& jitRef1 = jit->getJIT();
    auto& jitRef2 = jit->getJIT();
    EXPECT_EQ(&jitRef1, &jitRef2);
}

class HoocJITErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        jit = std::make_unique<HoocJIT>();
    }

    std::unique_ptr<HoocJIT> jit;
};

TEST_F(HoocJITErrorHandlingTest, GetLastErrorInitiallyEmpty) {
    EXPECT_FALSE(jit->hasError());
    EXPECT_TRUE(jit->getLastError().empty());
}

TEST_F(HoocJITErrorHandlingTest, ClearError) {
    auto result = jit->execute("nonExistent");
    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(jit->hasError());

    jit->clearError();
    EXPECT_FALSE(jit->hasError());
    EXPECT_TRUE(jit->getLastError().empty());
}

TEST_F(HoocJITErrorHandlingTest, ErrorAfterNonExistentExecution) {
    auto result = jit->execute("nonExistentFunction");
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(jit->getLastError().empty());
}

TEST_F(HoocJITErrorHandlingTest, ErrorMessageContainsFunctionName) {
    auto result = jit->execute("myFunction");
    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(jit->getLastError().find("myFunction") != std::string::npos);
}

class HoocJITLookupTest : public ::testing::Test {
protected:
    void SetUp() override {
        jit = std::make_unique<HoocJIT>();
    }

    std::unique_ptr<HoocJIT> jit;
};

TEST_F(HoocJITLookupTest, LookupNonExistentSymbol) {
    auto symbol = jit->lookup("nonExistentSymbol");
    EXPECT_FALSE(symbol.has_value());
    EXPECT_TRUE(jit->hasError());
}

TEST_F(HoocJITLookupTest, LookupClearsPreviousError) {
    auto fail = jit->lookup("nonExistent");
    EXPECT_FALSE(fail.has_value());
    EXPECT_TRUE(jit->hasError());

    auto fail2 = jit->lookup("anotherNonExistent");
    EXPECT_FALSE(fail2.has_value());
    EXPECT_TRUE(jit->hasError());
}

TEST_F(HoocJITLookupTest, LookupAfterCompilationFailure) {
    auto fail = jit->execute("nonExistent");
    EXPECT_FALSE(fail.has_value());
    EXPECT_TRUE(jit->hasError());

    auto symbol = jit->lookup("anotherMissing");
    EXPECT_FALSE(symbol.has_value());
}

class HoocJITExecutionTest : public ::testing::Test {
protected:
    void SetUp() override {
        jit = std::make_unique<HoocJIT>();
    }

    std::unique_ptr<HoocJIT> jit;
};

TEST_F(HoocJITExecutionTest, ExecuteNonExistentFunction) {
    auto execResult = jit->execute("nonExistentFunction");
    EXPECT_FALSE(execResult.has_value());
    EXPECT_TRUE(jit->hasError());
}

TEST_F(HoocJITExecutionTest, ExecuteClearsPreviousLookupError) {
    auto failLookup = jit->lookup("nonExistent");
    EXPECT_FALSE(failLookup.has_value());
    EXPECT_TRUE(jit->hasError());

    auto failExec = jit->execute("nonExistent");
    EXPECT_FALSE(failExec.has_value());
    EXPECT_TRUE(jit->hasError());
}

TEST_F(HoocJITExecutionTest, ExecuteFunctionNonExistent) {
    auto result = jit->executeFunction<void>("nonExistent");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}

TEST_F(HoocJITExecutionTest, ExecuteFunctionWithArgsNonExistent) {
    auto result = jit->executeFunction<int64_t>("nonExistent", 1, 2);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}

TEST_F(HoocJITExecutionTest, ExecuteFunctionWithArgsNonExistentDifferentTypes) {
    auto result = jit->executeFunction<double>("func", 1.5, 2);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}
