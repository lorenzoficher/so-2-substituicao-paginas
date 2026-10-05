#include "opt.hpp"

#include <stdexcept>

Opt::Opt(const Trace& trace, std::size_t frame_count)
    : frame_count_(frame_count), next_use_(trace.size(), NEVER) {
    if (frame_count == 0) {
        throw std::invalid_argument("o número de frames deve ser pelo menos 1");
    }
    pages_.reserve(trace.size());
    for (const Access& access : trace) {
        pages_.push_back(access.page);
    }
    // One backward pass: the próximo uso of access i is the last position seen
    // for its page among the accesses after i.
    std::unordered_map<uint32_t, std::size_t> seen_at;
    for (std::size_t i = pages_.size(); i-- > 0;) {
        const auto seen = seen_at.find(pages_[i]);
        if (seen != seen_at.end()) {
            next_use_[i] = seen->second;
        }
        seen_at[pages_[i]] = i;
    }
}

AccessResult Opt::access(uint32_t page) {
    if (position_ >= pages_.size() || pages_[position_] != page) {
        throw std::logic_error("OPT: acesso fora da ordem do trace");
    }
    const std::size_t next_use = next_use_[position_++];

    const auto resident = resident_.find(page);
    if (resident != resident_.end()) {
        by_next_use_.erase({resident->second, page});
        by_next_use_.insert({next_use, page});
        resident->second = next_use;
        return {false, std::nullopt};
    }

    std::optional<uint32_t> victim;
    if (resident_.size() == frame_count_) {
        victim = by_next_use_.begin()->second;
        by_next_use_.erase(by_next_use_.begin());
        resident_.erase(*victim);
    }
    by_next_use_.insert({next_use, page});
    resident_.emplace(page, next_use);
    return {true, victim};
}
