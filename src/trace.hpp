/**
 * Trace reader: turns a file of memory accesses into a sequence of accesses.
 *
 * Responsibilities:
 * - Parse each `<32-bit hex address> <R|W>` line.
 * - Turn the address into its page number (address >> PAGE_OFFSET_BITS).
 */
#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <vector>

/// Pages are 4096 bytes: the low 12 bits of an address are the offset.
constexpr unsigned PAGE_OFFSET_BITS = 12;

/// One line of the trace.
struct Access {
    uint32_t page;
    bool is_write;
};

using Trace = std::vector<Access>;

/// Reads every access from a stream, in order.
/// @param input Stream with one access per line.
/// @return The accesses, in trace order.
/// @throws std::runtime_error on a malformed line (the message names the line).
Trace read_trace(std::istream& input);

/// Reads the whole trace file at `path`.
/// @throws std::runtime_error if the file cannot be opened or has a malformed line.
Trace read_trace(const std::string& path);
