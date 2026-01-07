#include "leveldb_arena.h"
#include <cstdlib>
#include <cassert>
#include <vector>

LevelDBArena::LevelDBArena() = default;

LevelDBArena::~LevelDBArena() {
    for (char* block : blocks_) {
        std::free(block);
    }
}

void* LevelDBArena::allocate(size_t bytes) {
    if (bytes <= alloc_bytes_remaining_) {
        char* result = alloc_ptr_;
        alloc_ptr_ += bytes;
        alloc_bytes_remaining_ -= bytes;
        return result;
    }
    return AllocateFallback(bytes);
}

char* LevelDBArena::AllocateFallback(size_t bytes) {
    if (bytes > kBlockSize / 4) {
        return AllocateNewBlock(bytes);
    }
    alloc_ptr_ = AllocateNewBlock(kBlockSize);
    alloc_bytes_remaining_ = kBlockSize;

    char* result = alloc_ptr_;
    alloc_ptr_ += bytes;
    alloc_bytes_remaining_ -= bytes;
    return result;
}

char* LevelDBArena::AllocateNewBlock(size_t block_bytes) {
    char* block = static_cast<char*>(std::malloc(block_bytes));
    assert(block);
    blocks_.push_back(block);
    return block;
}
