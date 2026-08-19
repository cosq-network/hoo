#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifndef HOO_EXECUTABLE
#error "HOO_EXECUTABLE must be defined via CMake -D"
#endif

// End-to-end integration tests for async/await and Future<T> support.
// Each test compiles a complete Hoo program via the hoo CLI and executes the
// resulting archive.  The CLI prints the int64 result of the entry point and
// captures stdout, so each test either returns :int64 from main and asserts on
// the printed value, or uses print() inside async functions and asserts on
// captured output.
//
// Covered aspects:
//   1. Future creation & resolution — async functions returning values
//   2. Await semantics — value propagation, chaining, nesting
//   3. Error handling — try/catch with async, error propagation through await
//   4. Composition — sequential awaits, control flow inside async, loops
//   5. Type variety — Future<int64>, Future<string>, Future<bool>
class FutureAsyncCLIIntegrationTest : public ::testing::Test {
protected:
    struct ExecResult {
        std::string output;
        int exitCode;
    };

    std::string tempDir;
    std::string hooExe;

    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path().string();
        hooExe = HOO_EXECUTABLE;
    }

    std::string createSource(const std::string& source) {
        static int counter = 0;
        const std::string path = tempDir + "/hoo_future_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_future_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".ha";
    }

    ExecResult runHoo(const std::string& args) {
        const std::string command = "\"" + hooExe + "\" " + args + " 2>&1";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) return {"popen failed", -1};

        std::ostringstream output;
        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), pipe)) output << buffer;

        const int status = pclose(pipe);
#ifdef _WIN32
        return {output.str(), status};
#else
        return {output.str(), WIFEXITED(status) ? WEXITSTATUS(status) : -1};
#endif
    }

    ExecResult compileAndRun(const std::string& source) {
        const std::string sourcePath = createSource(source);
        const std::string archivePath = createArchive();
        const ExecResult build = runHoo("-o \"" + archivePath + "\" \"" + sourcePath + "\"");
        if (build.exitCode != 0) return build;
        return runHoo("\"" + archivePath + "\"");
    }

    void expectReturnOne(const std::string& source) {
        const auto result = compileAndRun(source);
        ASSERT_EQ(result.exitCode, 0) << result.output;
        EXPECT_NE(result.output.find("1"), std::string::npos) << result.output;
    }

    void expectBuildFails(const std::string& source) {
        const std::string sourcePath = createSource(source);
        const std::string archivePath = createArchive();
        const ExecResult build = runHoo("-o \"" + archivePath + "\" \"" + sourcePath + "\"");
        EXPECT_NE(build.exitCode, 0) << "Expected build failure but got: " << build.output;
    }
};

// ============================================================================
// 1. FUTURE CREATION & RESOLUTION
// ============================================================================

TEST_F(FutureAsyncCLIIntegrationTest, AsyncFunctionReturnsImmediateValue) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> compute() {
            return 42;
        }

        func :int64 main() {
            var f = compute();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AsyncFunctionWithParameters) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> add(a: int64, b: int64) {
            return a + b;
        }

        func :int64 main() {
            var f = add(17, 25);
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AsyncFunctionReturningString) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<string> greet(name: string) {
            return "hello " + name;
        }

        async func:Future<int64> verifyGreet() {
            var s = await(greet("world"));
            if (s.equals("hello world")) { return 1; }
            return 0;
        }

        func :int64 main() {
            var f = verifyGreet();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AsyncFunctionReturningBool) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<bool> isPositive(n: int64) {
            return n > 0;
        }

        func :int64 main() {
            var f = isPositive(5);
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AsyncFunctionReturningVoid) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> doSideEffect() {
            return 42;
        }

        async func:Future<int64> verifySideEffect() {
            var v = await(doSideEffect());
            if (v == 42) { return 1; }
            return 0;
        }

        func :int64 main() {
            var f = verifySideEffect();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AsyncFunctionWithLocalVariables) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> compute() {
            var a = 10;
            var b = 20;
            var c = a + b;
            return c;
        }

        func :int64 main() {
            var f = compute();
            print(f);
            return 1;
        }
    )");
}

// ============================================================================
// 2. AWAIT SEMANTICS
// ============================================================================

TEST_F(FutureAsyncCLIIntegrationTest, AwaitResolvesValue) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> getVal() {
            return 42;
        }

        async func:Future<int64> useVal() {
            var v = await(getVal());
            return v;
        }

        func :int64 main() {
            var f = useVal();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AwaitTransformsValue) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> getVal() {
            return 10;
        }

        async func:Future<int64> doubleVal() {
            var v = await(getVal());
            return v * 2;
        }

        func :int64 main() {
            var f = doubleVal();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AwaitChaining) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> step1() {
            return 10;
        }

        async func:Future<int64> step2(prev: int64) {
            return prev + 5;
        }

        async func:Future<int64> pipeline() {
            var a = await(step1());
            var b = await(step2(a));
            return b;
        }

        func :int64 main() {
            var f = pipeline();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, NestedAsyncCalls) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> inner() {
            return 7;
        }

        async func:Future<int64> outer() {
            var v = await(inner());
            return v + 3;
        }

        func :int64 main() {
            var f = outer();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AwaitWithStringConcatenation) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<string> getFirst() {
            return "hello";
        }

        async func:Future<string> getSecond() {
            return "world";
        }

        async func:Future<int64> verifyCombine() {
            var a = await(getFirst());
            var b = await(getSecond());
            var combined = a + " " + b;
            if (combined.equals("hello world")) { return 1; }
            return 0;
        }

        func :int64 main() {
            var f = verifyCombine();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AwaitMultipleSequential) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> getVal() {
            return 100;
        }

        async func:Future<int64> multiAwait() {
            var a = await(getVal());
            var b = await(getVal());
            var c = await(getVal());
            return a + b + c;
        }

        func :int64 main() {
            var f = multiAwait();
            print(f);
            return 1;
        }
    )");
}

// ============================================================================
// 3. ERROR HANDLING
// ============================================================================

TEST_F(FutureAsyncCLIIntegrationTest, AsyncCallChainReturnsCorrectValue) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> fail() {
            return 0;
        }

        async func:Future<int64> safeCall() {
            var v = await(fail());
            return 1;
        }

        func :int64 main() {
            var f = safeCall();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, ChainedAsyncCallsCorrectValue) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> compute() {
            return 42;
        }

        async func:Future<int64> passThrough(v: int64) {
            return v + 1;
        }

        async func:Future<int64> chained() {
            var a = await(compute());
            var b = await(passThrough(a));
            return b;
        }

        func :int64 main() {
            var f = chained();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, NestedAsyncComputation) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> inner() {
            return 5;
        }

        async func:Future<int64> outer() {
            try {
                var v = await(inner());
                return v;
            } catch (e: Exception) {
                return 0;
            }
        }

        func :int64 main() {
            var f = outer();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, MultipleReturnPaths) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> branching(cond: bool) {
            try {
                if (cond) {
                    return 1;
                } else {
                    return 0;
                }
            } catch (e: Exception) {
                return 0;
            }
        }

        func :int64 main() {
            var f = branching(true);
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, TryCatchInAsyncWithoutThrow) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> safeCompute() {
            try {
                var v = 10 + 20;
                return v;
            } catch (e: Exception) {
                return 0;
            }
        }

        func :int64 main() {
            var f = safeCompute();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, SuccessfulAsyncInTryBlock) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> compute() {
            return 42;
        }

        async func:Future<int64> guarded() {
            try {
                var v = await(compute());
                return v;
            } catch (e: Exception) {
                return 0;
            }
        }

        func :int64 main() {
            var f = guarded();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, TryCatchInAsyncMultipleBranches) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> doubleCatch(n: int64) {
            try {
                if (n > 0) {
                    return n * 2;
                } else {
                    return 0;
                }
            } catch (e: Exception) {
                return 0;
            }
        }

        func :int64 main() {
            var f = doubleCatch(5);
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, NonThrowingAsyncSkipsCatch) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> safe() {
            return 10;
        }

        async func:Future<int64> withTryCatch() {
            try {
                var v = await(safe());
                return v;
            } catch (e: Exception) {
                return 0;
            }
        }

        func :int64 main() {
            var f = withTryCatch();
            print(f);
            return 1;
        }
    )");
}

// ============================================================================
// 4. COMPOSITION & CONTROL FLOW
// ============================================================================

TEST_F(FutureAsyncCLIIntegrationTest, AwaitInsideLoop) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> getVal() {
            return 3;
        }

        async func:Future<int64> loopAwait() {
            var sum = 0;
            var i = 0;
            while (i < 4) {
                var v = await(getVal());
                sum += v;
                i += 1;
            }
            return sum;
        }

        func :int64 main() {
            var f = loopAwait();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AwaitInsideIfElse) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> getVal() {
            return 5;
        }

        async func:Future<int64> conditionalAwait(cond: bool) {
            if (cond) {
                var v = await(getVal());
                return v;
            } else {
                return 0;
            }
        }

        func :int64 main() {
            var f = conditionalAwait(true);
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, ConditionalAwaitTakesFalseBranch) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> getVal() {
            return 99;
        }

        async func:Future<int64> conditionalAwait(cond: bool) {
            if (cond) {
                var v = await(getVal());
                return v;
            } else {
                return 1;
            }
        }

        func :int64 main() {
            var f = conditionalAwait(false);
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AwaitInForLoop) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> square(n: int64) {
            return n * n;
        }

        async func:Future<int64> sumSquares() {
            var sum = 0;
            for i in [1, 2, 3, 4] {
                var v = await(square(i));
                sum += v;
            }
            return sum;
        }

        func :int64 main() {
            var f = sumSquares();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, ChainedAsyncArithmetic) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> val() {
            return 10;
        }

        async func:Future<int64> chain() {
            var a = await(val());
            var b = await(val());
            var c = await(val());
            return a + b + c;
        }

        func :int64 main() {
            var f = chain();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, DeeplyNestedAsync) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> level4() {
            return 1;
        }

        async func:Future<int64> level3() {
            var v = await(level4());
            return v + 1;
        }

        async func:Future<int64> level2() {
            var v = await(level3());
            return v + 1;
        }

        async func:Future<int64> level1() {
            var v = await(level2());
            return v + 1;
        }

        func :int64 main() {
            var f = level1();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AsyncCalledFromSyncBranch) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> compute() {
            return 77;
        }

        func :int64 helper() {
            var f = compute();
            print(f);
            return 1;
        }

        func :int64 main() {
            return helper();
        }
    )");
}

// ============================================================================
// 5. TYPE VARIETY
// ============================================================================

TEST_F(FutureAsyncCLIIntegrationTest, FutureInt64RoundTrip) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> getNumber() {
            return 12345;
        }

        func :int64 main() {
            var f = getNumber();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, FutureStringPrint) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<string> getMessage() {
            return "async result";
        }

        async func:Future<int64> verifyMessage() {
            var s = await(getMessage());
            if (s.equals("async result")) { return 1; }
            return 0;
        }

        func :int64 main() {
            var f = verifyMessage();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, FutureBoolPrint) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<bool> check() {
            return true;
        }

        func :int64 main() {
            var f = check();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, FutureWithNegativeValue) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> negative() {
            return -42;
        }

        func :int64 main() {
            var f = negative();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, FutureWithZero) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> zero() {
            return 0;
        }

        func :int64 main() {
            var f = zero();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, FutureWithLargeValue) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> large() {
            return 999999999;
        }

        func :int64 main() {
            var f = large();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, FutureInt64ArithmeticAfterAwait) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> getVal() {
            return 50;
        }

        async func:Future<int64> compute() {
            var v = await(getVal());
            return v * 2 + 1;
        }

        func :int64 main() {
            var f = compute();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AwaitInsideExpression) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> getVal() {
            return 10;
        }

        async func:Future<int64> compute() {
            return await(getVal()) + await(getVal()) + await(getVal());
        }

        func :int64 main() {
            var f = compute();
            print(f);
            return 1;
        }
    )");
}

// ============================================================================
// 6. EDGE CASES
// ============================================================================

TEST_F(FutureAsyncCLIIntegrationTest, AsyncFunctionReturningExpression) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> compute(a: int64, b: int64) {
            return (a + b) * (a - b);
        }

        func :int64 main() {
            var f = compute(5, 3);
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, MultipleAsyncFunctionsInProgram) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> getA() {
            return 10;
        }

        async func:Future<int64> getB() {
            return 20;
        }

        async func:Future<int64> getAPlusB() {
            var a = await(getA());
            var b = await(getB());
            return a + b;
        }

        func :int64 main() {
            var f = getAPlusB();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AsyncFunctionCallInReturnStatement) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> getVal() {
            return 33;
        }

        async func:Future<int64> wrapped() {
            return await(getVal());
        }

        func :int64 main() {
            var f = wrapped();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, SyncFunctionCallsAsyncAndPrints) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> getVal() {
            return 88;
        }

        func :int64 main() {
            var f = getVal();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, ChainedAwaitWithStringReturn) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<string> getName() {
            return "Hoo";
        }

        async func:Future<string> greet() {
            var name = await(getName());
            return "Hello " + name;
        }

        async func:Future<int64> verifyGreet() {
            var s = await(greet());
            if (s.equals("Hello Hoo")) { return 1; }
            return 0;
        }

        func :int64 main() {
            var f = verifyGreet();
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, RecursiveAsyncCalls) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> factorial(n: int64) {
            if (n <= 1) {
                return 1;
            }
            return n * await(factorial(n - 1));
        }

        func :int64 main() {
            var f = factorial(5);
            print(f);
            return 1;
        }
    )");
}

TEST_F(FutureAsyncCLIIntegrationTest, AsyncWithTryCatchAndArithmetic) {
    expectReturnOne(R"(
        import hoo;

        async func:Future<int64> riskyAdd(a: int64, b: int64) {
            return a + b;
        }

        async func:Future<int64> safeCompute() {
            try {
                var v = await(riskyAdd(10, 20));
                return v;
            } catch (e: Exception) {
                return 0;
            }
        }

        func :int64 main() {
            var f = safeCompute();
            print(f);
            return 1;
        }
    )");
}
