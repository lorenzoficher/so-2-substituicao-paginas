/**
 * FIFO policy: the victim is the page that has been in memory the longest,
 * regardless of use.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <unordered_set>

#include "policy.hpp"

class Fifo : public Policy {
public:
    /// @param frame_count Number of frames, all starting empty.
    /// @throws std::invalid_argument if `frame_count` is 0.
    explicit Fifo(std::size_t frame_count);

    AccessResult access(uint32_t page) override;

private:
    std::size_t frame_count_;
    std::deque<uint32_t> load_order_;  ///< Resident pages, oldest first.
    std::unordered_set<uint32_t> resident_;
};
