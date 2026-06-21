#pragma once

#include <string>
#include <vector>
#include <memory>
#include <istream>
#include <ostream>

#include "core/HooCompiler.h"
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

namespace hooc {
namespace repl {

class REPLSession {
public:
    // Constructor accepting custom streams (used by tests)
    REPLSession(std::istream& in, std::ostream& out, std::ostream& err);
    // Default constructor uses standard input/output streams.
    REPLSession();
    ~REPLSession() = default;

    // Run the interactive REPL loop.
    void run();
    // Evaluate a single line of input. Returns true on success; result or error is filled accordingly.
    bool eval(const std::string& input, std::string& outResult, std::string& outError);

private:
    // IO provider abstracts stdin/stdout/stderr; default constructed uses std::cin/std::cout/std::cerr.
    DefaultIOProvider io_;
    std::unique_ptr<HVMJIT> jit_;
    std::unique_ptr<HooCompiler> compiler_;
    // Accumulated top‑level declarations that persist across evaluations.
    std::string accumulatedDeclarations_;
    uint64_t lineCounter_ = 0;

    // Stream references for I/O (used by custom ctor)
    std::istream& in_;
    std::ostream& out_;
    std::ostream& err_;

    // Signal handling for Ctrl‑C (SIGINT)
    static std::atomic<bool> interrupted_; // set by signal handler
    static void signalHandler(int signum);

    bool isDeclaration(const std::string& line) const;
    std::string buildWrapperFunction(const std::string& statement);
};

} // namespace repl
} // namespace hooc
