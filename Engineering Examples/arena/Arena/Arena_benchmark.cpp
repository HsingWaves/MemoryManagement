#include <iostream>
#include <vector>
#include <chrono>

#include "simple_arena.h"
#include "leveldb_arena.h"

struct Obj {
    int x, y, z;
};

constexpr int N = 1'000'000;

template <typename F>
void run(const char* name, F&& f) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << name << ": "
        << std::chrono::duration<double>(end - start).count()
        << " s\n";
}

int main() {
    run("new/delete", [] {
        std::vector<Obj*> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i)
            v.push_back(new Obj{ i, i, i });
        for (auto p : v) delete p;
        });

    run("SimpleArena", [] {
        SimpleArena arena;
        std::vector<Obj*> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i)
            v.push_back(arena.create<Obj>(Obj{ i, i, i }));
        });

    run("LevelDBArena", [] {
        LevelDBArena arena;
        std::vector<Obj*> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i)
            v.push_back(arena.create<Obj>(Obj{ i, i, i }));
        });
}
