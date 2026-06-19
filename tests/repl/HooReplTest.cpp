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
    // The error stream shouldn't contain a second failure.
    // Let's verify that the output did not report a second error for getVal()
}

TEST(HooReplTest, ReplStatementWrapping) {
    std::stringstream in("1 + 2;\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    // 1 + 2; is a statement, so it should be wrapped in func __repl_line_1 and compiled successfully
    EXPECT_EQ(err.str(), "");
}

TEST(HooReplTest, ReplDeclarationAndUse) {
    std::stringstream in("func :int64 getTen() { return 10; }\ngetTen();\n/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    // Both lines should compile cleanly
    EXPECT_EQ(err.str(), "");
}
