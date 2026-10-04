#include <cstdint>
#include <vector>

#include "doctest.h"
#include "fifo.hpp"
#include "simulator.hpp"

namespace {

/// Trace of reads over `pages`.
Trace reads(const std::vector<uint32_t>& pages) {
    Trace trace;
    for (const uint32_t page : pages) {
        trace.push_back({page, false});
    }
    return trace;
}

}  // namespace

TEST_CASE("simulação conta acessos e falhas de página, incluindo as compulsórias") {
    Fifo fifo(3);

    const SimulationResult result = run(reads({7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1}), fifo);

    CHECK(result.accesses == 20);
    CHECK(result.page_faults == 15);
}

TEST_CASE("simulação de trace vazio não tem acessos nem falhas") {
    Fifo fifo(1);

    const SimulationResult result = run(Trace{}, fifo);

    CHECK(result.accesses == 0);
    CHECK(result.page_faults == 0);
    CHECK(result.writebacks == 0);
}
