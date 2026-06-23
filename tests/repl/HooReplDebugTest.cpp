#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "repl/REPLSession.h"

using namespace hooc::repl;

TEST(HooReplDebug, VarDeclOnly) {
    std::stringstream in(
        "var x = 100;\n"
        "/exit\n");
    std::stringstream out;
    std::stringstream err;

    REPLSession session(in, out, err);
    session.run();

    std::string outStr = out.str();
    EXPECT_NE(outStr.find("Declaration defined."), std::string::npos);
    EXPECT_EQ(err.str(), "");
}
