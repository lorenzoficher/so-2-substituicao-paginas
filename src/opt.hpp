/**
 * OPT policy: the victim is the page whose próximo uso is farthest in the future
 * of the trace, a page never used again counting as farthest of all.
 *
 * The only policy that sees the future: it receives the whole trace and
 * precomputes, for every access, the próximo uso of its page. Each access then
 * costs O(log frames), never a scan of the future.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "policy.hpp"
#include "trace.hpp"

class Opt : public Policy {
public:
    /// @param trace The trace this policy will see, in order, one access per call.
    ///              Its pages are copied, so it need not outlive the policy.
    /// @param frame_count Number of frames, all starting empty.
    /// @throws std::invalid_argument if `frame_count` is 0.
    Opt(const Trace& trace, std::size_t frame_count);

    /// Accesses `page`, which must be the next page of the trace.
    /// Among pages never used again, the victim is the lowest page number.
    /// @throws std::logic_error if `page` is not the next access of the trace.
    AccessResult access(uint32_t page) override;

private:
    /// Próximo uso of a page never accessed again.
    static constexpr std::size_t NEVER = static_cast<std::size_t>(-1);

    /// Orders resident pages so that the victim comes first: farthest próximo uso,
    /// then lowest page number.
    struct VictimFirst {
        bool operator()(const std::pair<std::size_t, uint32_t>& a,
                        const std::pair<std::size_t, uint32_t>& b) const {
            if (a.first != b.first) {
                return a.first > b.first;
            }
            return a.second < b.second;
        }
    };

    std::size_t frame_count_;
    std::vector<uint32_t> pages_;       ///< Page of each access, in trace order.
    std::vector<std::size_t> next_use_; ///< Próximo uso after each access.
    std::size_t position_ = 0;          ///< Index of the next access expected.
    std::set<std::pair<std::size_t, uint32_t>, VictimFirst> by_next_use_;
    std::unordered_map<uint32_t, std::size_t> resident_;  ///< Page → its próximo uso.
};
