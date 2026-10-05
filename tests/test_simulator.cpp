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

TEST_CASE("vítima suja conta uma escrita de volta; vítima limpa não conta") {
    Fifo fifo(1);

    // Page 1 is written and evicted dirty; page 2 is only read and evicted clean.
    const SimulationResult result = run({{1, true}, {2, false}, {3, false}}, fifo);

    CHECK(result.page_faults == 3);
    CHECK(result.writebacks == 1);
}

TEST_CASE("página escrita, substituída e recarregada só com leituras volta limpa") {
    Fifo fifo(1);

    // The second eviction of page 1 finds it clean: only the first one counts.
    const SimulationResult result = run({{1, true}, {2, false}, {1, false}, {2, false}}, fifo);

    CHECK(result.page_faults == 4);
    CHECK(result.writebacks == 1);
}

TEST_CASE("páginas ainda sujas ao fim do trace não geram escrita de volta") {
    Fifo fifo(2);

    const SimulationResult result = run({{1, true}, {2, true}, {1, true}}, fifo);

    CHECK(result.page_faults == 2);
    CHECK(result.writebacks == 0);
}
