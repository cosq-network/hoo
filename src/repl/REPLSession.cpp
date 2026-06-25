#include "repl/REPLSession.h"
#include "core/SymbolMangler.h"
#include "runtime/lib/hoo_runtime.h"
#include "runtime/lib/hoo_string.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <csignal>
#include <atomic>

namespace hooc {
namespace repl {

const std::string kReplBootstrap = "func :any __hoo_repl_bootstrap() { return 0; }\n";

static std::string formatResult(int64_t rc) {
    void* ptr = reinterpret_cast<void*>(rc);
    if (hoo_is_managed_object(ptr)) {
        int64_t type_id = hoo_get_type_id(ptr);
        HooString str = hoo_string_from_any(rc, type_id);
        if (str) {
            const char* data = hoo_string_data(str);
            std::string result(data);
            hoo_release(str);
            return result;
        }
    }
    return std::to_string(rc);
}

// Static members for signal handling
std::atomic<bool> REPLSession::interrupted_{false};

void REPLSession::signalHandler(int signum) {
    if (signum == SIGINT) {
        interrupted_ = true;
    }
}



// Stream constructor
REPLSession::REPLSession(std::istream& in, std::ostream& out, std::ostream& err)
    : in_(in), out_(out), err_(err), io_(),
      jit_(std::make_unique<HVMJIT>(io_)),
      compiler_(std::make_unique<HooCompiler>()),
      lineCounter_(0) {
    // Install Ctrl‑C (SIGINT) handler for this REPL session
    std::signal(SIGINT, REPLSession::signalHandler);
}

// Default constructor delegates to standard streams
REPLSession::REPLSession()
    : REPLSession(std::cin, std::cout, std::cerr) {}

static int calculateBraceDepthChange(const std::string& text, bool& inDoubleQuotes, bool& inSingleQuotes, bool& inBlockComment) {
    int depth = 0;
    size_t i = 0;
    while (i < text.size()) {
        if (inBlockComment) {
            if (i + 1 < text.size() && text[i] == '*' && text[i+1] == '/') {
                inBlockComment = false;
                i += 2;
            } else {
                ++i;
            }
            continue;
        }
        if (inDoubleQuotes) {
            if (text[i] == '\\' && i + 1 < text.size()) {
                i += 2;
            } else if (text[i] == '"') {
                inDoubleQuotes = false;
                ++i;
            } else {
                ++i;
            }
            continue;
        }
        if (inSingleQuotes) {
            if (text[i] == '\\' && i + 1 < text.size()) {
                i += 2;
            } else if (text[i] == '\'') {
                inSingleQuotes = false;
                ++i;
            } else {
                ++i;
            }
            continue;
        }
        // Single line comment
        if (i + 1 < text.size() && text[i] == '/' && text[i+1] == '/') {
            break;
        }
        // Block comment start
        if (i + 1 < text.size() && text[i] == '/' && text[i+1] == '*') {
            inBlockComment = true;
            i += 2;
            continue;
        }
        if (text[i] == '"') {
            inDoubleQuotes = true;
            ++i;
            continue;
        }
        if (text[i] == '\'') {
            inSingleQuotes = true;
            ++i;
            continue;
        }
        if (text[i] == '{' || text[i] == '(' || text[i] == '[') ++depth;
        else if (text[i] == '}' || text[i] == ')' || text[i] == ']') --depth;
        ++i;
    }
    return depth;
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool REPLSession::isDeclaration(const std::string& line) const {
    std::string t = trim(line);
    if (t.rfind("func", 0) == 0) return true;
    if (t.rfind("class", 0) == 0) return true;
    if (t.rfind("import", 0) == 0) return true;
    if (t.rfind("var", 0) == 0) return true;
    if (t.rfind("const", 0) == 0) return true;
    return false;
}

std::string REPLSession::buildWrapperFunction(const std::string& statement) {
    ++lineCounter_;
    std::string funcName = "__repl_line_" + std::to_string(lineCounter_);
    std::ostringstream oss;
    // Treat assignments and control flow as void-returning wrappers
    if (statement.find('=') != std::string::npos ||
        statement.rfind("var ", 0) == 0 ||
        statement.rfind("if ", 0) == 0 ||
        statement.rfind("while ", 0) == 0 ||
        statement.rfind("for ", 0) == 0) {
        oss << "func :any " << funcName << "() {\n"
            << "    " << statement << "\n"
            << "    return 0;\n"
            << "}\n";
    } else {
        std::string stmt = statement;
        if (!stmt.empty() && stmt.back() != ';') stmt += ";";
        oss << "func :any " << funcName << "() {\n"
            << "    return " << stmt << "\n"
            << "}\n";
    }
    return oss.str();
}

bool REPLSession::eval(const std::string& input, std::string& outResult, std::string& outError) {
    outResult.clear();
    outError.clear();
    bool decl = isDeclaration(input);
    std::string source;
    std::string targetSymbol;
    if (decl) {
        source = kReplBootstrap + accumulatedDeclarations_ + "\n" + input;
    } else {
        std::string wrapper = buildWrapperFunction(input);
        source = kReplBootstrap + accumulatedDeclarations_ + "\n" + wrapper;
        // Generate the correct mangled symbol using the compiler's mangler
        std::string funcName = "__repl_line_" + std::to_string(lineCounter_);
        MangledFunctionParams mp;
        mp.modulePath = {"__repl_session"};
        mp.functionName = funcName;
        mp.returnType = "any";
        targetSymbol = SymbolMangler::mangleFunctionName(mp);
    }
    auto module = compiler_->compile("__repl_session", source);
    if (!module) {
        outError = compiler_->getLastError();
        return false;
    }
    if (!jit_->loadModule(std::move(module))) {
        outError = jit_->getLastError();
        return false;
    }
    if (decl) {
        accumulatedDeclarations_ += "\n" + input;
        outResult = "Declaration defined.";
        return true;
    }
    int64_t rc = jit_->run(targetSymbol);
    if (jit_->hasError()) {
        outError = std::string("Runtime JIT Execution Error: ") + jit_->getLastError();
        return false;
    }
    outResult = formatResult(rc);
    return true;
}

bool REPLSession::loadSource(const std::string& source, std::string& outError) {
    outError.clear();
    auto module = compiler_->compile("__repl_session", kReplBootstrap + accumulatedDeclarations_ + "\n" + source);
    if (!module) {
        outError = compiler_->getLastError();
        return false;
    }
    if (!jit_->loadModule(std::move(module))) {
        outError = jit_->getLastError();
        return false;
    }
    accumulatedDeclarations_ += "\n" + source;
    return true;
}

void REPLSession::run() {
    out_ << "Welcome to the Hoo REPL!" << std::endl;
    out_ << "Type /help for help, or /exit to quit." << std::endl;
    std::string line;
    std::string block;
    int braceDepth = 0;
    bool inDoubleQuotes = false, inSingleQuotes = false, inBlockComment = false;
    while (true) {
        // If Ctrl‑C was pressed, exit gracefully
        if (interrupted_) {
            out_ << "\n[Interrupted] Exiting REPL." << std::endl;
            break;
        }
        out_ << (block.empty() ? ">>> " : "... ") << std::flush;
        if (!std::getline(in_, line)) break;
        std::string trimmed = trim(line);
        // Command handling only when not inside a multi‑line block
        if (block.empty() && !trimmed.empty() && trimmed[0] == '/') {
            // Skip single-line comments entirely
            if (trimmed.size() >= 2 && trimmed[1] == '/') {
                continue;
            }
            // Block comment lines fall through to multi-line block accumulation
            if (trimmed.size() >= 2 && trimmed[1] == '*') {
                // handled by brace depth tracking below
            } else {
                if (trimmed == "/exit" || trimmed == "/quit") break;
                if (trimmed == "/help") {
                    out_ << "Available commands:\n"
                         << "  /help   Show this help message\n"
                         << "  /reset  Reset REPL session state\n"
                         << "  /exit   Exit the REPL session\n"
                         << "  /quit   Exit the REPL session" << std::endl;
                    continue;
                }
                if (trimmed == "/reset") {
                    accumulatedDeclarations_.clear();
                    lineCounter_ = 0;
                    jit_ = std::make_unique<HVMJIT>(io_);
                    compiler_ = std::make_unique<HooCompiler>();
                    block.clear();
                    braceDepth = 0;
                    inDoubleQuotes = inSingleQuotes = inBlockComment = false;
                    out_ << "REPL session reset." << std::endl;
                    continue;
                }
                err_ << "Unknown command: " << trimmed << ". Type /help for help." << std::endl;
                continue;
            }
        }
        if (!block.empty()) block += "\n";
        block += line;
        braceDepth += calculateBraceDepthChange(line, inDoubleQuotes, inSingleQuotes, inBlockComment);
        if (braceDepth <= 0 && !inDoubleQuotes && !inSingleQuotes && !inBlockComment) {
            braceDepth = 0;
            inDoubleQuotes = inSingleQuotes = inBlockComment = false;
            std::string blkTrim = trim(block);
            if (!blkTrim.empty()) {
                std::string result, error;
                if (eval(blkTrim, result, error)) {
                    if (!result.empty()) out_ << result << std::endl;
                } else {
                    err_ << "Error: " << error << std::endl;
                }
            }
            block.clear();
        }
    }
}

} // namespace repl
} // namespace hooc
