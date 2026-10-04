#include "lru_approx.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

LruApprox::LruApprox(std::size_t frame_count, unsigned history_bits,
                     std::size_t aging_interval)
    : frame_count_(frame_count), history_bits_(history_bits), aging_interval_(aging_interval) {
    if (frame_count == 0) {
        throw std::invalid_argument("o número de frames deve ser pelo menos 1");
    }
    if (history_bits == 0 || history_bits > MAX_HISTORY_BITS) {
        throw std::invalid_argument("os bits de histórico devem estar entre 1 e 32");
    }
    if (aging_interval == 0) {
        throw std::invalid_argument("o intervalo de envelhecimento deve ser pelo menos 1");
    }
}

AccessResult LruApprox::access(uint32_t page) {
    AccessResult result{false, std::nullopt};
    const auto resident = resident_.find(page);
    if (resident != resident_.end()) {
        if (!resident->second->referenced) {
            RankSet::node_type node = victim_first_.extract(resident->second);
            node.value().referenced = true;
            resident->second = victim_first_.insert(std::move(node)).position;
        }
    } else {
        result.page_fault = true;
        if (resident_.size() == frame_count_) {
            result.victim = victim_first_.begin()->page;
            victim_first_.erase(victim_first_.begin());
            resident_.erase(*result.victim);
        }
        resident_.emplace(page, victim_first_.insert({true, 0, loads_++, page}).first);
    }

    if (++accesses_since_aging_ == aging_interval_) {
        accesses_since_aging_ = 0;
        age();
    }
    return result;
}

void LruApprox::age() {
    const uint32_t newest_bit = uint32_t{1} << (history_bits_ - 1);
    aging_nodes_.clear();
    while (!victim_first_.empty()) {
        RankSet::node_type node = victim_first_.extract(victim_first_.begin());
        Rank& rank = node.value();
        rank.history = (rank.history >> 1) | (rank.referenced ? newest_bit : 0);
        rank.referenced = false;
        aging_nodes_.push_back(std::move(node));
    }
    std::sort(aging_nodes_.begin(), aging_nodes_.end(),
              [](const RankSet::node_type& a, const RankSet::node_type& b) {
                  return a.value() < b.value();
              });
    for (RankSet::node_type& node : aging_nodes_) {
        const uint32_t page = node.value().page;
        resident_[page] = victim_first_.insert(victim_first_.end(), std::move(node));
    }
}
