/**
 * Simulator: runs a trace through a replacement policy and does the counting.
 *
 * Responsibilities:
 * - Feed every access of the trace to the policy, in order.
 * - Count page faults (compulsory ones included) and writebacks.
 */
#pragma once

#include <cstdint>

#include "policy.hpp"
#include "trace.hpp"

/// Counts produced by one simulation.
struct SimulationResult {
    uint64_t accesses = 0;
    uint64_t page_faults = 0;
    uint64_t writebacks = 0;  ///< Dirty victims; stays 0 until dirty tracking (#3).
};

/// Runs `trace` through `policy`, whose frames must start empty.
SimulationResult run(const Trace& trace, Policy& policy);
