#include "simulator.hpp"

SimulationResult run(const Trace& trace, Policy& policy) {
    SimulationResult result;
    for (const Access& access : trace) {
        ++result.accesses;
        if (policy.access(access.page).page_fault) {
            ++result.page_faults;
        }
    }
    return result;
}
