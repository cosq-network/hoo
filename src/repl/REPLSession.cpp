#include "repl/REPLSession.h"
#include <sstream>
#include <algorithm>

namespace hooc {
namespace repl {

REPLSession::REPLSession(std::istream& in, std::ostream& out, std::ostream& err)
    : in_(in), out_(out), err_(err), io_(),
      jit_(std::make_unique<HVMJIT>(io_)),
      compiler_(std::make_unique<HooCompiler>()) {}

static int calculateBraceDepthChange(const std::string& text, bool& inDoubleQuotes, bool& inSingleQuotes, bool& inBlockComment) {
    int depth = 0;
    size_t i = 0;
    while (i < text.size()) {
        if (inBlockComment) {
            if (i + 1 < text.size() && text[i] == '*' && text[i+1] == '/') {
                inBlockComment = false;
                i += 2;
            } else {
                i++;
            }
            continue;
        }
        if (inDoubleQuotes) {
            if (text[i] == '\\' && i + 1 < text.size()) {
                i += 2;
            } else if (text[i] == '"') {
                inDoubleQuotes = false;
                i++;
            } else {
                i++;
            }
            continue;
        }
        if (inSingleQuotes) {
            if (text[i] == '\\' && i + 1 < text.size()) {
                i += 2;
            } else if (text[i] == '\'') {
                inSingleQuotes = false;
                i++;
            } else {
                i++;
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
            i++;
            continue;
        }
        if (text[i] == '\'') {
            inSingleQuotes = true;
            i++;
            continue;
        }

        if (text[i] == '{' || text[i] == '(' || text[i] == '[') {
            depth++;
        } else if (text[i] == '}' || text[i] == ')' || text[i] == ']') {
            depth--;
        }
        i++;
    }
    return depth;
}

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

void REPLSession::run() {
    out_ << "Welcome to the Hoo REPL!" << std::endl;
    out_ << "Type /help for help, or /exit to quit." << std::endl;

    std::string line;
    std::string currentBlock;
    int braceDepth = 0;
    bool inDoubleQuotes = false;
    bool inSingleQuotes = false;
    bool inBlockComment = false;

    while (true) {
        if (currentBlock.empty()) {
            out_ << ">>> " << std::flush;
        } else {
            out_ << "... " << std::flush;
        }

        if (!std::getline(in_, line)) {
            break; // EOF
        }

        std::string trimmedLine = trim(line);

        // Check for commands
        if (currentBlock.empty() && !trimmedLine.empty() && trimmedLine[0] == '/') {
            if (trimmedLine == "/exit" || trimmedLine == "/quit") {
                break;
            } else if (trimmedLine == "/help") {
                out_ << "Available commands:\n"
                     << "  /help       Show this help message\n"
                     << "  /reset      Reset the REPL session state\n"
                     << "  /exit       Exit the REPL session\n"
                     << "  /quit       Exit the REPL session" << std::endl;
                continue;
            } else if (trimmedLine == "/reset") {
                accumulatedDeclarations_.clear();
                lineCounter_ = 0;
                jit_ = std::make_unique<HVMJIT>(io_);
                compiler_ = std::make_unique<HooCompiler>();
                currentBlock.clear();
                braceDepth = 0;
                inDoubleQuotes = false;
                inSingleQuotes = false;
                inBlockComment = false;
                out_ << "REPL session reset." << std::endl;
                continue;
            } else {
                err_ << "Unknown command: " << trimmedLine << ". Type /help for help." << std::endl;
                continue;
            }
        }

        if (!currentBlock.empty()) {
            currentBlock += "\n";
        }
        currentBlock += line;

        braceDepth += calculateBraceDepthChange(line, inDoubleQuotes, inSingleQuotes, inBlockComment);

        if (braceDepth <= 0 && !inDoubleQuotes && !inSingleQuotes && !inBlockComment) {
            // We have a complete statement block to evaluate
            braceDepth = 0; // clamp to 0 if it went negative
            
            std::string trimmedBlock = trim(currentBlock);
            if (!trimmedBlock.empty()) {
                std::string result, error;
                if (eval(trimmedBlock, result, error)) {
                    if (!result.empty()) {
                        out_ << result << std::endl;
                    }
                } else {
                    err_ << "Error: " << error << std::endl;
                }
            }
            currentBlock.clear();
        }
    }
}

bool REPLSession::isDeclaration(const std::string& line) const {
    std::string trimmed = trim(line);
    if (trimmed.rfind("func", 0) == 0) return true;
    if (trimmed.rfind("class", 0) == 0) return true;
    if (trimmed.rfind("import", 0) == 0) return true;
    return false;
}

std::string REPLSession::buildWrapperFunction(const std::string& statement) {
    lineCounter_++;
    std::string funcName = "__repl_line_" + std::to_string(lineCounter_);
    std::string trimmed = trim(statement);
    
    // Check if it's a control flow statement or an assignment that doesn't return value
    if (trimmed.rfind("var ", 0) == 0 || 
        trimmed.rfind("if ", 0) == 0 || 
        trimmed.rfind("while ", 0) == 0 || 
        trimmed.rfind("for ", 0) == 0 || 
        trimmed.find('=') != std::string::npos) {
        return "func :any " + funcName + "() {\n" + statement + "\nreturn 0;\n}\n";
    } else {
        std::string stmt = statement;
        if (!stmt.empty() && stmt.back() != ';') {
            stmt += ";";
        }
        return "func :any " + funcName + "() {\nreturn " + stmt + "\n}\n";
    }
}

bool REPLSession::eval(const std::string& input, std::string& outResult, std::string& outError) {
    outResult.clear();
    outError.clear();

    if (isDeclaration(input)) {
        std::string testCode = accumulatedDeclarations_ + "\n" + input;
        auto mod = compiler_->compile("__repl_session", testCode);
        if (!mod) {
            outError = compiler_->getLastError();
            return false;
        }
        accumulatedDeclarations_ += "\n" + input;
        return true;
    } else {
        std::string wrapper = buildWrapperFunction(input);
        std::string testCode = accumulatedDeclarations_ + "\n" + wrapper;
        auto mod = compiler_->compile("__repl_session", testCode);
        if (!mod) {
            outError = compiler_->getLastError();
            return false;
        }
        return true;
    }
}

} // namespace repl
} // namespace hooc
