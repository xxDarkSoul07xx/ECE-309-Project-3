// src/tools/tool_registry.cpp — YOU IMPLEMENT (spec §4.3).
//
// A hand-written hash table. No std::unordered_map / std::map / std::set.
// Every function below is a stub so the starter compiles. Replace each body.

#include "tools/tool_registry.h"

// start with 16 buckets that are all empty
ToolRegistry::ToolRegistry() : buckets_(16), count_(0) {}

ToolRegistry::~ToolRegistry() {
    // nothing to do bc unique_ptrs
}

std::size_t ToolRegistry::hash(std::string_view key) noexcept {
    // fnv-1a hash
    std::uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : key) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return static_cast<std::size_t>(h);
}

bool ToolRegistry::add(std::unique_ptr<Tool> tool) {
    // reject null tools
    if (tool == nullptr) {
        return false;
    }

    std::string name = tool->name();

    // check if this name already exists, if so reject and let tool die
    if (find(name) != nullptr) {
        return false;
    }

    // resize before inserting if we would go over the 0.75 load factor
    if ((double)(count_ + 1) / (double)buckets_.size() > 0.75) {
        resize();
    }

    std::size_t idx = hash(name) % buckets_.size();

    Entry e;
    e.name = name;
    e.tool = std::move(tool);
    buckets_[idx].push_back(std::move(e));
    count_++;

    return true;
}

Tool* ToolRegistry::find(std::string_view name) const {
    std::size_t idx = hash(name) % buckets_.size();

    for (const Entry& e : buckets_[idx]) {
        if (e.name == name) {
            return e.tool.get();
        }
    }

    return nullptr;
}

std::size_t ToolRegistry::size() const noexcept {
    return count_;
}

void ToolRegistry::for_each(const std::function<void(const Tool&)>& fn) const {
    for (const std::vector<Entry>& bucket : buckets_) {
        for (const Entry& e : bucket) {
            fn(*e.tool);
        }
    }
}

std::size_t ToolRegistry::bucket_count() const noexcept {
    return buckets_.size();
}

double ToolRegistry::load_factor() const noexcept {
    return (double)count_ / (double)buckets_.size();
}

std::size_t ToolRegistry::max_probe_length() const {
    std::size_t longest = 0;
    for (const std::vector<Entry>& bucket : buckets_) {
        if (bucket.size() > longest) {
            longest = bucket.size();
        }
    }
    return longest;
}

void ToolRegistry::resize() {
    // make a new bigger bucket array
    std::size_t new_count = buckets_.size() * 2;
    std::vector<std::vector<Entry>> new_buckets(new_count);

    // go through every old bucket and every entry, move it to the new array
    for (std::vector<Entry>& bucket : buckets_) {
        for (Entry& e : bucket) {
            std::size_t idx = hash(e.name) % new_count;
            new_buckets[idx].push_back(std::move(e));
        }
    }

    buckets_ = std::move(new_buckets);
}