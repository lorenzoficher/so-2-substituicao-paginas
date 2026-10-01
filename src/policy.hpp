/**
 * Common interface of the replacement policies.
 *
 * A policy owns its frames and its metadata and only decides which page leaves on
 * a page fault. Counting page faults and writebacks is the simulator's job, and no
 * policy knows whether a page is dirty.
 */
#pragma once

#include <cstdint>
#include <optional>

/// Outcome of one access, as seen by the policy.
struct AccessResult {
    bool page_fault;
    /// Page evicted to make room, when the fault found every frame occupied.
    std::optional<uint32_t> victim;
};

class Policy {
public:
    virtual ~Policy() = default;

    /// Accesses `page`, loading it into a frame on a page fault.
    virtual AccessResult access(uint32_t page) = 0;
};
