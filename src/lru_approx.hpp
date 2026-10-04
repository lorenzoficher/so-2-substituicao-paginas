/**
 * LRU aproximado: each page in a frame carries a bit de referência and a histórico
 * of N bits. The victim is the page with the bit off, then the lowest histórico,
 * then the one loaded longest ago.
 *
 * The bit comes before the histórico because it is the most recent information: in
 * the reverse order, a page loaded since the last envelhecimento (histórico
 * zerado) would always be the next victim.
 *
 * Resident pages are kept ordered victim-first, so a page fault costs O(log frames).
 * Only an envelhecimento reorders them all, once every I accesses, in
 * O(frames log frames) and without allocating.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "policy.hpp"

class LruApprox : public Policy {
public:
    /// Widest histórico supported (N).
    static constexpr unsigned MAX_HISTORY_BITS = 32;

    /// @param frame_count Number of frames, all starting empty.
    /// @param history_bits Bits de histórico (N), from 1 to MAX_HISTORY_BITS.
    /// @param aging_interval Intervalo de envelhecimento (I): every I accesses,
    ///                       counting from the first, every page in a frame ages.
    /// @throws std::invalid_argument if a parameter is out of range.
    LruApprox(std::size_t frame_count, unsigned history_bits, std::size_t aging_interval);

    /// Accesses `page`, turning its bit de referência on. The I-th access since the
    /// last envelhecimento triggers the next one, after the access itself.
    AccessResult access(uint32_t page) override;

private:
    /// What decides the victim, in comparison order: smallest first.
    struct Rank {
        bool referenced;
        uint32_t history;
        uint64_t loaded_at;  ///< Load sequence: smaller means loaded longer ago.
        uint32_t page;

        bool operator<(const Rank& other) const {
            return std::tie(referenced, history, loaded_at) <
                   std::tie(other.referenced, other.history, other.loaded_at);
        }
    };

    using RankSet = std::set<Rank>;

    /// Ages every page in a frame and reorders them, reusing the set's nodes.
    void age();

    std::size_t frame_count_;
    unsigned history_bits_;
    std::size_t aging_interval_;
    std::size_t accesses_since_aging_ = 0;
    uint64_t loads_ = 0;
    RankSet victim_first_;
    std::unordered_map<uint32_t, RankSet::iterator> resident_;  ///< Page → its rank.
    std::vector<RankSet::node_type> aging_nodes_;  ///< Scratch space for age().
};
