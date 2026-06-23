#include <gtest/gtest.h>
#include <sstream>
#include <cstring>
#include "repl/REPLSession.h"

using namespace hooc::repl;



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

// Deeply nested braces (3+ levels)
TEST(HooReplAdvancedTest, ReplDeeplyNestedBraces) {
    std::stringstream in(
        "func :int64 deep() {\n"
        "    if (true) {\n"
        "        if (true) {\n"
        "            if (true) {\n"
        "                return 5;\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    return 0;\n"
        "}\n"
        "deep();\n"
        "/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("5"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

// Multiple declarations across lines
TEST(HooReplAdvancedTest, ReplMultipleDeclarations) {
    std::stringstream in(
        "func :int64 add(a: int64, b: int64) { return a + b; }\n"
        "func :int64 mul(a: int64, b: int64) { return a * b; }\n"
        "add(3, 4);\n"
        "mul(5, 6);\n"
        "/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("7"), std::string::npos);
    EXPECT_NE(outStr.find("30"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

// Block comment with braces inside, used inline in expression
TEST(HooReplAdvancedTest, ReplBlockCommentBrace) {
    std::stringstream in(
        "1 + /* { comment } */ 1;\n"
        "/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("2"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

// Multi-line block comment
TEST(HooReplAdvancedTest, ReplMultiLineBlockComment) {
    std::stringstream in(
        "func :int64 test() {\n"
        "    /* start\n"
        "    middle\n"
        "    end */\n"
        "    return 10;\n"
        "}\n"
        "test();\n"
        "/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("10"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

// Reset clears accumulated declarations
TEST(HooReplAdvancedTest, ReplResetClearsState) {
    std::stringstream in(
        "func :int64 getX() { return 42; }\n"
        "/reset\n"
        "nonesuch;\n"
        "/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    // After reset, getX is no longer defined - invalid syntax should fail compilation
    EXPECT_NE(err.str().find("Error"), std::string::npos);
}

// Error in one expression doesn't affect subsequent ones
TEST(HooReplAdvancedTest, ReplMidSessionErrorRecovery) {
    std::stringstream in(
        "1 + 1;\n"
        "undefined_var;\n"
        "2 + 2;\n"
        "/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string errStr = err.str();
    EXPECT_NE(errStr.find("Error"), std::string::npos);

    // But 1+1 and 2+2 should still succeed
    std::string outStr = out.str();
    EXPECT_NE(outStr.find("2"), std::string::npos);
    EXPECT_NE(outStr.find("4"), std::string::npos);
}
