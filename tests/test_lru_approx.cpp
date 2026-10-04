#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include "doctest.h"
#include "lru_approx.hpp"

namespace {

/// Victim chosen by the last of `pages`, which must fault after the others.
uint32_t victim_after(LruApprox& lru, const std::vector<uint32_t>& pages) {
    for (std::size_t i = 0; i + 1 < pages.size(); ++i) {
        lru.access(pages[i]);
    }
    const AccessResult result = lru.access(pages.back());
    REQUIRE(result.page_fault);
    REQUIRE(result.victim);
    return *result.victim;
}

}  // namespace

TEST_CASE("LRU aproximado: bit de referência desligado vem antes do histórico na escolha da vítima") {
    // 2 frames, aging every 2 accesses. After 1 and 2, aging leaves both with the
    // bit off and the same histórico; 3 evicts 1 (loaded first). Then 2 has the bit
    // off and histórico 0x80, 3 has the bit on and histórico zerado: 4 must evict 2.
    // Comparing histórico before the bit would evict 3, the page loaded since the
    // last envelhecimento.
    LruApprox lru(2, 8, 2);

    CHECK(victim_after(lru, {1, 2, 3}) == 1);
    CHECK(victim_after(lru, {4}) == 2);
}

TEST_CASE("LRU aproximado: com bits de referência iguais, vence o menor histórico") {
    // Aging after every access. 1 is loaded first but accessed again after 2, so at
    // the fault on 3 its histórico (0xA0) beats 2's (0x40): the victim is 2.
    LruApprox lru(2, 8, 1);

    CHECK(victim_after(lru, {1, 2, 1, 3}) == 2);
}

TEST_CASE("LRU aproximado: com histórico igual, vence a página carregada há mais tempo") {
    // No envelhecimento before 3: both pages have the bit on and histórico zerado.
    LruApprox lru(2, 8, 1000);

    CHECK(victim_after(lru, {1, 2, 3}) == 1);
}

TEST_CASE("LRU aproximado: o histórico tem exatamente N bits") {
    // 3 frames, aging after every access, sequence 1 2 1 3 then the fault on 4.
    // N = 2: 1 has histórico 01 and 2 has 00, so 2 is the victim.
    // N = 1: the bit that tells 1 from 2 falls off; both have 0 and the tie goes to
    // 1, loaded first.
    LruApprox two_bits(3, 2, 1);
    LruApprox one_bit(3, 1, 1);

    CHECK(victim_after(two_bits, {1, 2, 1, 3, 4}) == 2);
    CHECK(victim_after(one_bit, {1, 2, 1, 3, 4}) == 1);
}

TEST_CASE("LRU aproximado: com N = 32 o bit mais antigo sobrevive 32 envelhecimentos") {
    // 3 frames, aging after every access: 1 2 1, then 3 accessed `hits` times, then
    // the fault on 4. Page 1's newest bit was set by the 3rd envelhecimento, page 2's
    // only bit by the 2nd. With 31 hits on 3, 1's bit is the last one of the 32 and
    // 2's has fallen off: 2 is the victim. One more envelhecimento drops 1's bit too,
    // and the tie between two zeroed históricos goes to 1, loaded first.
    for (const auto& [hits, victim] : {std::pair<int, uint32_t>{31, 2}, {32, 1}}) {
        CAPTURE(hits);
        LruApprox lru(3, 32, 1);
        std::vector<uint32_t> pages = {1, 2, 1};
        pages.insert(pages.end(), static_cast<std::size_t>(hits), 3);
        pages.push_back(4);

        CHECK(victim_after(lru, pages) == victim);
    }
}

TEST_CASE("LRU aproximado: o envelhecimento ocorre exatamente a cada I acessos") {
    // 2 frames, sequence 1 2 1 then the fault on 3.
    // I = 2: aging after 2 turns both bits off; the access to 1 turns its bit back
    // on, so 2 is the victim.
    // I = 3: the first aging comes only after the third access, turning both bits
    // off with the same histórico; the tie goes to 1, loaded first.
    // I = 4: no aging before the fault; both bits on, histórico zerado: 1 again.
    for (const auto& [interval, victim] :
         {std::pair<std::size_t, uint32_t>{2, 2}, {3, 1}, {4, 1}}) {
        CAPTURE(interval);
        LruApprox lru(2, 8, interval);

        CHECK(victim_after(lru, {1, 2, 1, 3}) == victim);
    }
}

TEST_CASE("LRU aproximado: o envelhecimento se repete a cada I acessos, não só uma vez") {
    // 2 frames, I = 2, sequence 1 2 1 2 1 2 1 then the fault on 3. Agings after
    // accesses 2, 4 and 6 leave both pages with the bit off and the same histórico;
    // access 7 turns 1's bit back on, so 2 is the victim. Aging only once would
    // leave both bits on and the tie would go to 1.
    LruApprox lru(2, 8, 2);

    CHECK(victim_after(lru, {1, 2, 1, 2, 1, 2, 1, 3}) == 2);
}

TEST_CASE("LRU aproximado: parâmetros fora do intervalo são rejeitados") {
    CHECK_THROWS_AS(LruApprox(0, 8, 1000), std::invalid_argument);
    CHECK_THROWS_AS(LruApprox(4, 0, 1000), std::invalid_argument);
    CHECK_THROWS_AS(LruApprox(4, 33, 1000), std::invalid_argument);
    CHECK_THROWS_AS(LruApprox(4, 8, 0), std::invalid_argument);
    CHECK_NOTHROW(LruApprox(4, 1, 1));
    CHECK_NOTHROW(LruApprox(4, 32, 1));
}
