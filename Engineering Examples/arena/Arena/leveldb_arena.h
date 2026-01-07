#pragma once
#include <cstddef>
#include <vector>

class LevelDBArena {
public:
    LevelDBArena();
    ~LevelDBArena();

    void* allocate(size_t bytes);

    template <typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

private:
    char* AllocateFallback(size_t bytes);
    char* AllocateNewBlock(size_t block_bytes);

    static constexpr size_t kBlockSize = 4096;

    char* alloc_ptr_ = nullptr;
    size_t alloc_bytes_remaining_ = 0;
    std::vector<char*> blocks_;
};
