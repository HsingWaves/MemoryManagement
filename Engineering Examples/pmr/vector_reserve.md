In practice, `std::vector::reserve()` usually has a much larger performance impact than choosing between `push_back` and `emplace_back`.
 `reserve()` eliminates repeated reallocations and element moves, while `emplace_back()` often only saves a small temporary construction cost.
 Therefore, capacity planning comes first; emplacement is a secondary optimization.



```
static void BM_EmplaceBack_NoReserve(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::string> vec;
        for (int i = 0; i < state.range(0); ++i) {
            vec.emplace_back("This is a long string to avoid SSO");
        }
    }
}

BENCHMARK(BM_EmplaceBack_NoReserve)->Arg(10000);


// reserve
static void BM_PushBack_WithReserve(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::string> vec;
        vec.reserve(state.range(0));   //  keypoint
        for (int i = 0; i < state.range(0); ++i) {
            vec.push_back("This is a long string to avoid SSO");
        }
    }
}

BENCHMARK(BM_PushBack_WithReserve)->Arg(10000);
// emplace 
static void BM_EmplaceBack_ConstructArgs(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::string> vec;
        vec.reserve(state.range(0));
        for (int i = 0; i < state.range(0); ++i) {
            vec.emplace_back(100, 'x');  // 
        }
    }
}

static void BM_PushBack_Temporary(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::string> vec;
        vec.reserve(state.range(0));
        for (int i = 0; i < state.range(0); ++i) {
            vec.push_back(std::string(100, 'x')); // 构造临时对
        }
    }
}

BENCHMARK(BM_EmplaceBack_ConstructArgs)->Arg(10000);
BENCHMARK(BM_PushBack_Temporary)->Arg(10000);

```

`reserve()` is not a "general optimization technique," but rather a structural optimization specifically for contiguous memory or bucket-based containers; `emplace_*`, on the other hand, represents a micro-optimization at the general interface level.

**std::string, std::unordered_map, std::unordered_set.**