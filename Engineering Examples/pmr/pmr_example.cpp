#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <memory_resource>
#include <scoped_allocator>


#include <atomic>
#include <cstdlib>
#include <new>
#include <cstddef>

static std::atomic<uint64_t> g_new_calls{ 0 };
static std::atomic<uint64_t> g_delete_calls{ 0 };
static std::atomic<uint64_t> g_new_bytes{ 0 };
static std::atomic<uint64_t> g_small_new_calls{ 0 }; // <= 256B
static std::atomic<uint64_t> g_small_new_bytes{ 0 };

static inline void reset_alloc_stats() {
    g_new_calls = 0;
    g_delete_calls = 0;
    g_new_bytes = 0;
    g_small_new_calls = 0;
    g_small_new_bytes = 0;
}

static inline void print_alloc_stats(const char* label) {
    auto nc = g_new_calls.load();
    auto dc = g_delete_calls.load();
    auto nb = g_new_bytes.load();
    auto snc = g_small_new_calls.load();
    auto snb = g_small_new_bytes.load();
    std::printf("[%s] new=%llu delete=%llu bytes=%llu small_new=%llu small_bytes=%llu\n",
        label,
        (unsigned long long)nc,
        (unsigned long long)dc,
        (unsigned long long)nb,
        (unsigned long long)snc,
        (unsigned long long)snb);
}

// global operator new/delete hooks
void* operator new(std::size_t sz) {
    g_new_calls.fetch_add(1, std::memory_order_relaxed);
    g_new_bytes.fetch_add(sz, std::memory_order_relaxed);
    if (sz <= 256) {
        g_small_new_calls.fetch_add(1, std::memory_order_relaxed);
        g_small_new_bytes.fetch_add(sz, std::memory_order_relaxed);
    }
    if (void* p = std::malloc(sz)) return p;
    throw std::bad_alloc();
}

void operator delete(void* p) noexcept {
    g_delete_calls.fetch_add(1, std::memory_order_relaxed);
    std::free(p);
}

void operator delete(void* p, std::size_t) noexcept {
    g_delete_calls.fetch_add(1, std::memory_order_relaxed);
    std::free(p);
}



static constexpr int N = 300000;

template <class F>
double time_it(const char* name, F&& f) {
    auto t0 = std::chrono::high_resolution_clock::now();
    f();
    auto t1 = std::chrono::high_resolution_clock::now();
    double s = std::chrono::duration<double>(t1 - t0).count();
    std::cout << name << ": " << s << " s\n";
    return s;
}

int main() {
    const char* payload = "abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789";

    reset_alloc_stats();
    time_it("baseline std::vector<std::string>", [&] {
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.emplace_back(payload);
        });
    print_alloc_stats("...");

    reset_alloc_stats();
    time_it("pmr monotonic_buffer_resource", [&] {
        std::vector<std::byte> backing(128ull * 1024 * 1024);
        std::pmr::monotonic_buffer_resource arena(backing.data(), backing.size());

        using PString = std::pmr::string;
        using VecAlloc = std::scoped_allocator_adaptor<std::pmr::polymorphic_allocator<PString>>;

        std::pmr::vector<PString> v{ VecAlloc{ &arena } };
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.emplace_back(payload);
        });
    print_alloc_stats("...");

    reset_alloc_stats();
    time_it("pmr unsynchronized_pool_resource", [&] {
        std::pmr::unsynchronized_pool_resource pool;

        using PString = std::pmr::string;
        using VecAlloc = std::scoped_allocator_adaptor<std::pmr::polymorphic_allocator<PString>>;

        std::pmr::vector<PString> v{ VecAlloc{ &pool } };
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.emplace_back(payload);
        });
    print_alloc_stats("...");

    return 0;
}
