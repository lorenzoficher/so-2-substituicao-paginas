/**
 * Command line of the simulator: one CSV line per simulation.
 *
 * Responsibilities:
 * - Validate every argument before any output, so a bad argument never leaves a
 *   partial CSV behind.
 * - Read the trace once and run one simulation per number of frames.
 * - Print the CSV header and rows.
 *
 * Usage: sim <trace> fifo|opt --frames 4,8,16
 *        sim <trace> lru-approx --bits 8 --interval 1000 --frames 4,8,16
 */
#pragma once

#include <ostream>
#include <string>
#include <vector>

/// Runs the simulator with `args` (argv without the program name).
/// @param out Receives the CSV.
/// @param err Receives error and usage messages.
/// @return The process exit code: 0 on success.
int run_cli(const std::vector<std::string>& args, std::ostream& out, std::ostream& err);
