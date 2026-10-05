#include "trace.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

/// Parses an address of at most 8 hex digits; false if it is not one.
bool parse_address(const std::string& text, uint32_t& address) {
    if (text.empty() || text.size() > 8) {
        return false;
    }
    for (const char digit : text) {
        if (!std::isxdigit(static_cast<unsigned char>(digit))) {
            return false;
        }
    }
    address = static_cast<uint32_t>(std::stoul(text, nullptr, 16));
    return true;
}

}  // namespace

Trace read_trace(std::istream& input) {
    Trace trace;
    std::string line;
    for (std::size_t line_number = 1; std::getline(input, line); ++line_number) {
        std::istringstream fields(line);
        std::string address_text;
        std::string operation;
        std::string extra;
        if (!(fields >> address_text)) {
            continue;  // blank line
        }
        uint32_t address = 0;
        const bool valid = parse_address(address_text, address) && (fields >> operation) &&
                           (operation == "R" || operation == "W") && !(fields >> extra);
        if (!valid) {
            throw std::runtime_error("trace malformado na linha " + std::to_string(line_number) +
                                     ": '" + line + "'");
        }
        trace.push_back({address >> PAGE_OFFSET_BITS, operation == "W"});
    }
    return trace;
}

Trace read_trace(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("não foi possível abrir o trace: " + path);
    }
    return read_trace(file);
}
