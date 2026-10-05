#include "simulator.hpp"

#include <unordered_set>

SimulationResult run(const Trace& trace, Policy& policy) {
    SimulationResult result;
    std::unordered_set<uint32_t> dirty;  // resident pages written since they were loaded
    for (const Access& access : trace) {
        ++result.accesses;
        const AccessResult outcome = policy.access(access.page);
        if (outcome.page_fault) {
            ++result.page_faults;
        }
        // A victim leaves its frame clean or dirty; erasing it means a reload starts clean.
        if (outcome.victim && dirty.erase(*outcome.victim) != 0) {
            ++result.writebacks;
        }
        if (access.is_write) {
            dirty.insert(access.page);
        }
    }
    return result;
}
