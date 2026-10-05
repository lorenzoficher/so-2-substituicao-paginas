#include "fifo.hpp"

#include <stdexcept>

Fifo::Fifo(std::size_t frame_count) : frame_count_(frame_count) {
    if (frame_count == 0) {
        throw std::invalid_argument("o número de frames deve ser pelo menos 1");
    }
}

AccessResult Fifo::access(uint32_t page) {
    if (resident_.count(page) != 0) {
        return {false, std::nullopt};
    }
    std::optional<uint32_t> victim;
    if (resident_.size() == frame_count_) {
        victim = load_order_.front();
        load_order_.pop_front();
        resident_.erase(*victim);
    }
    load_order_.push_back(page);
    resident_.insert(page);
    return {true, victim};
}
