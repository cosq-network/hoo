#include <gtest/gtest.h>
#include <sstream>
#include <cstring>
#include "repl/REPLSession.h"

using namespace hooc::repl;

// Test that variable declarations persist across statements and are cleared after /reset
TEST(HooReplAdvancedTest, ReplVariablePersistence) {
    std::stringstream in(
        "var x = 42;\n"
        "x;\n"
        "/reset\n"
        "x;\n"
        "/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    // First call should produce result 42
    EXPECT_NE(outStr.find("42"), std::string::npos);
    // After reset, the second call to x should cause an error (undefined symbol)
    EXPECT_NE(err.str().find("Error"), std::string::npos);
}

// Blank lines should be ignored without producing errors
TEST(HooReplAdvancedTest, ReplEmptyInputIgnored) {
    std::stringstream in("\n   \n\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    // Only the welcome message should be present, no errors
    std::string outStr = out.str();
    EXPECT_NE(outStr.find("Welcome to the Hoo REPL!"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

// Single‑line comments should be ignored and not affect REPL execution
TEST(HooReplAdvancedTest, ReplCommentIgnored) {
    std::stringstream in(
        "// this is a comment\n"
        "1 + 1;\n"
        "/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    // Result of 1+1 should be 2
    std::string outStr = out.str();
    EXPECT_NE(outStr.find("2"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

// Multi‑line function definition with nested braces should be handled correctly
TEST(HooReplAdvancedTest, ReplNestedBraceBlock) {
    std::stringstream in(
        "func :int64 complex() {\n"
        "    if (true) {\n"
        "        return 7;\n"
        "    } else {\n"
        "        return 0;\n"
        "    }\n"
        "}\n"
        "complex();\n"
        "/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    // The call to complex() should yield 7
    std::string outStr = out.str();
    EXPECT_NE(outStr.find("7"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}
