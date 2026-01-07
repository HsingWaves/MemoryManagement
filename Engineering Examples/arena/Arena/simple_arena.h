#pragma once
#include <cstddef>
#include <cstdlib>
#include <vector>
#include <cassert>

class SimpleArena {
public:
    explicit SimpleArena(size_t block_size = 4096)
        : block_size_(block_size) {}

    ~SimpleArena() {
        for (void* block : blocks_) {
            std::free(block);
        }
    }

    void* allocate(size_t bytes, size_t alignment = alignof(std::max_align_t)) {
        size_t aligned = align(bytes, alignment);
        if (aligned > bytes_remaining_) {
            allocate_block(aligned);
        }
        void* result = alloc_ptr_;
        alloc_ptr_ += aligned;
        bytes_remaining_ -= aligned;
        return result;
    }

    template <typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

private:
    size_t block_size_;
    std::vector<void*> blocks_;
    char* alloc_ptr_ = nullptr;
    size_t bytes_remaining_ = 0;

    static size_t align(size_t n, size_t alignment) {
        return (n + alignment - 1) & ~(alignment - 1);
    }

    void allocate_block(size_t min_bytes) {
        size_t size = min_bytes > block_size_ / 4 ? min_bytes : block_size_;
        void* block = std::malloc(size);
        assert(block);
        blocks_.push_back(block);
        alloc_ptr_ = static_cast<char*>(block);
        bytes_remaining_ = size;
    }
};
