#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "doctest.h"
#include "fifo.hpp"

namespace {

/// Page faults of a FIFO with `frame_count` frames over `pages`.
std::size_t count_page_faults(const std::vector<uint32_t>& pages, std::size_t frame_count) {
    Fifo fifo(frame_count);
    std::size_t page_faults = 0;
    for (const uint32_t page : pages) {
        if (fifo.access(page).page_fault) {
            ++page_faults;
        }
    }
    return page_faults;
}

}  // namespace

TEST_CASE("FIFO: sequência de Silberschatz com 3 frames dá 15 falhas de página") {
    const std::vector<uint32_t> pages = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};

    CHECK(count_page_faults(pages, 3) == 15);
}

TEST_CASE("FIFO: anomalia de Belady — 9 falhas com 3 frames e 10 com 4") {
    const std::vector<uint32_t> pages = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};

    CHECK(count_page_faults(pages, 3) == 9);
    CHECK(count_page_faults(pages, 4) == 10);
}

TEST_CASE("FIFO: a vítima é a página carregada há mais tempo, mesmo que recém-usada") {
    Fifo fifo(2);
    CHECK(fifo.access(1).page_fault);
    CHECK_FALSE(fifo.access(2).victim);  // free frame: no victim
    CHECK_FALSE(fifo.access(1).page_fault);  // a hit keeps the load order

    const AccessResult result = fifo.access(3);

    CHECK(result.page_fault);
    REQUIRE(result.victim);
    CHECK(*result.victim == 1);
}

TEST_CASE("FIFO: número de frames zero é rejeitado") {
    CHECK_THROWS_AS(Fifo(0), std::invalid_argument);
}
