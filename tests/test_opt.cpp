#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "doctest.h"
#include "fifo.hpp"
#include "opt.hpp"

namespace {

/// Trace of reads over `pages`.
Trace reads(const std::vector<uint32_t>& pages) {
    Trace trace;
    for (const uint32_t page : pages) {
        trace.push_back({page, false});
    }
    return trace;
}

/// Page faults of an OPT with `frame_count` frames over `pages`.
std::size_t count_page_faults(const std::vector<uint32_t>& pages, std::size_t frame_count) {
    const Trace trace = reads(pages);
    Opt opt(trace, frame_count);
    std::size_t page_faults = 0;
    for (const Access& access : trace) {
        if (opt.access(access.page).page_fault) {
            ++page_faults;
        }
    }
    return page_faults;
}

}  // namespace

TEST_CASE("OPT: sequência de Silberschatz com 3 frames dá 9 falhas de página") {
    const std::vector<uint32_t> pages = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};

    CHECK(count_page_faults(pages, 3) == 9);
}

TEST_CASE("OPT: página que nunca mais será usada é preferida como vítima") {
    // With 2 frames, at the access to 3: page 1 is used again, page 2 never is.
    const Trace trace = reads({1, 2, 3, 1});
    Opt opt(trace, 2);
    opt.access(1);
    opt.access(2);

    const AccessResult result = opt.access(3);

    CHECK(result.page_fault);
    REQUIRE(result.victim);
    CHECK(*result.victim == 2);
    CHECK_FALSE(opt.access(1).page_fault);
}

TEST_CASE("OPT: entre páginas que nunca mais serão usadas, a vítima é a de menor número") {
    // Pages 5 and 4 are never used again when 9 faults. The tie is broken by the
    // lowest page number, not by load order (5 was loaded first).
    const Trace trace = reads({5, 4, 9});
    Opt opt(trace, 2);
    opt.access(5);
    opt.access(4);

    const AccessResult result = opt.access(9);

    REQUIRE(result.victim);
    CHECK(*result.victim == 4);
}

TEST_CASE("OPT: nunca produz mais falhas de página que o FIFO") {
    const std::vector<std::vector<uint32_t>> sequences = {
        {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1},  // Silberschatz
        {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5},                          // Belady
        {1, 2, 3, 1, 2, 3, 1, 2, 3, 4, 4, 4, 1, 5, 6, 2, 1, 2, 3, 7},
    };
    for (const std::vector<uint32_t>& pages : sequences) {
        for (std::size_t frame_count = 1; frame_count <= 6; ++frame_count) {
            CAPTURE(frame_count);
            Fifo fifo(frame_count);
            std::size_t fifo_page_faults = 0;
            for (const uint32_t page : pages) {
                fifo_page_faults += fifo.access(page).page_fault ? 1 : 0;
            }

            CHECK(count_page_faults(pages, frame_count) <= fifo_page_faults);
        }
    }
}

TEST_CASE("OPT: com 4 frames a sequência de Belady dá 6 falhas de página") {
    CHECK(count_page_faults({1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5}, 4) == 6);
}

TEST_CASE("OPT: número de frames zero é rejeitado") {
    CHECK_THROWS_AS(Opt(reads({1}), 0), std::invalid_argument);
}

TEST_CASE("OPT: acesso fora da ordem do trace é rejeitado") {
    Opt opt(reads({1, 2}), 2);

    CHECK_THROWS_AS(opt.access(2), std::logic_error);
}
