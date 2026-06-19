#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "core/HooCompiler.h"
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

namespace hooc {
namespace repl {

class REPLSession {
public:
    explicit REPLSession(std::istream& in = std::cin,
                          std::ostream& out = std::cout,
                          std::ostream& err = std::cerr);
    ~REPLSession() = default;

    void run(); // Main interactive loop
    bool eval(const std::string& input, std::string& outResult, std::string& outError);

private:
    std::istream& in_;
    std::ostream& out_;
    std::ostream& err_;

    DefaultIOProvider io_;
    std::unique_ptr<HVMJIT> jit_;
    std::unique_ptr<HooCompiler> compiler_;
    std::string accumulatedDeclarations_;
    uint64_t lineCounter_ = 0;

    bool isDeclaration(const std::string& line) const;
    std::string buildWrapperFunction(const std::string& statement);
};

} // namespace repl
} // namespace hooc
