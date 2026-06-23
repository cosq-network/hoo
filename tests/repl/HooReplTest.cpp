#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "repl/REPLSession.h"

using namespace hooc::repl;

TEST(HooReplTest, DefaultConstruction) {
    std::stringstream in;
    std::stringstream out;
    std::stringstream err;
    REPLSession session(in, out, err);
    // Should construct successfully
}

TEST(HooReplTest, ReplExitCommands) {
    std::stringstream in("/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("Welcome to the Hoo REPL!"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

TEST(HooReplTest, ReplQuitCommands) {
    std::stringstream in("/quit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("Welcome to the Hoo REPL!"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

TEST(HooReplTest, ReplHelpCommand) {
    std::stringstream in("/help\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("Available commands:"), std::string::npos);
    EXPECT_NE(outStr.find("/help"), std::string::npos);
    EXPECT_NE(outStr.find("/reset"), std::string::npos);
    EXPECT_NE(outStr.find("/exit"), std::string::npos);
    EXPECT_NE(outStr.find("/quit"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

TEST(HooReplTest, ReplResetCommand) {
    std::stringstream in("/reset\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("REPL session reset."), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

TEST(HooReplTest, ReplUnknownCommand) {
    std::stringstream in("/unknown_cmd\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string errStr = err.str();
    EXPECT_NE(errStr.find("Unknown command: /unknown_cmd"), std::string::npos);
}

TEST(HooReplTest, ReplMultiLineBraceMatching) {
    std::stringstream in("func :int64 getVal() {\n  return 42;\n}\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    // Check that we got the multiline prompt "... "
    EXPECT_NE(outStr.find("... "), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

TEST(HooReplTest, ReplCompileErrorRecovery) {
    std::stringstream in("func :int64 getVal() { invalid_syntax }\nfunc :int64 getVal() { return 42; }\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string errStr = err.str();
    // The first one should have failed compilation
    EXPECT_NE(errStr.find("Error:"), std::string::npos);

    // The second function declaration should compile successfully.
    // Count error occurrences — should be exactly 1 (only the first fails)
    size_t firstErr = errStr.find("Error:");
    size_t secondErr = errStr.find("Error:", firstErr + 1);
    EXPECT_EQ(secondErr, std::string::npos);
}

TEST(HooReplTest, ReplErrorThenValidRecovery) {
    std::stringstream in("invalid syntax\n42;\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    // First line fails with compilation error
    EXPECT_NE(err.str().find("Error:"), std::string::npos);
    // Second line should still evaluate and produce output
    std::string outStr = out.str();
    EXPECT_NE(outStr.find("42"), std::string::npos);
}

TEST(HooReplTest, ReplStatementWrapping) {
    std::stringstream in("1 + 2;\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    // 1 + 2; should evaluate to 3
    std::string outStr = out.str();
    EXPECT_NE(outStr.find("3"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

TEST(HooReplTest, ReplExpressionResult) {
    std::stringstream in("42;\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("42"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

TEST(HooReplTest, ReplMultipleExpressions) {
    std::stringstream in("1 + 2;\n3 * 4;\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("3"), std::string::npos);
    EXPECT_NE(outStr.find("12"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

TEST(HooReplTest, ReplMathExpression) {
    std::stringstream in("(10 + 5) * 2;\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("30"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

TEST(HooReplTest, ReplDeclarationAndUse) {
    std::stringstream in("func :int64 getTen() { return 10; }\ngetTen();\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    // Both lines should compile cleanly, getTen() should return 10
    std::string outStr = out.str();
    EXPECT_NE(outStr.find("10"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

TEST(HooReplTest, ReplIncrementalDeclarations) {
    std::stringstream in(
        "func :int64 doubleVal(x: int64) { return x * 2; }\n"
        "doubleVal(21);\n"
        "/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    // doubleVal(21) should return 42
    std::string outStr = out.str();
    EXPECT_NE(outStr.find("42"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

TEST(HooReplTest, ReplMultiLineString) {
    std::stringstream in(
        "func :any test() {\n"
        "    return 99;\n"
        "}\n"
        "test();\n"
        "/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("99"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}



