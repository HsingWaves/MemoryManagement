## 1. Motivation: Why Memory Pools Exist

In performance-critical C++ systems, **memory fragmentation and allocation overhead** are recurring problems.

Traditionally, developers solved this by **hand-rolling memory pools**:

- Pre-allocating large memory blocks
- Managing free lists manually
- Routing allocations by size class

This works, but leads to:

- Boilerplate code
- Error-prone lifetime management
- Non-standard interfaces

Starting with **C++17**, the standard library introduced **`std::pmr` (Polymorphic Memory Resources)** — a **standardized, extensible memory management framework** designed to solve exactly these problems.

Fragmentation usually shows up as:

> **“Plenty of memory exists, but allocations get slower, larger, or start failing earlier than expected.”**
>
> | Problem                    | Symptom                | Fragmentation?              |
> | -------------------------- | ---------------------- | --------------------------- |
> | Virtual address exhaustion | `mmap` fails on 32-bit | External fragmentation      |
> | RSS keeps growing          | Memory not returned    | Often allocator behavior    |
> | Allocations slow / jittery | Latency spikes         | Fragmentation or contention |
> | OOM with free memory       | Classic embedded issue | External fragmentation      |

------

## 2. Conceptual Layering of `std::pmr`

`std::pmr` cleanly separates **data structure logic** from **memory management strategy**.

STL container + runtime **switchable** allocator

The layers are:

### 2.1 Container Layer

- Focuses on **CRUD semantics** (create, read, update, delete)
- Example: `std::pmr::vector`, `std::pmr::string`, `std::pmr::unordered_map`
- Completely unaware of how memory is allocated

### 2.2 Allocator Layer

- `std::pmr::polymorphic_allocator<T>`
- Adapts a `memory_resource` to the STL allocator interface
- Enables **runtime selection** of allocation strategy

### 2.3 Memory Resource Layer

- `std::pmr::memory_resource`
- Abstract base class for **raw memory allocation**
- Defines:
  - `do_allocate`
  - `do_deallocate`
  - `do_is_equal`

This separation is the key design insight of `std::pmr`.

------

## 3. Core Memory Pool Strategies (Allocator Theory)

Whether you hand-roll a memory pool or use `std::pmr`, the **underlying principles are the same**:

1. **Block Pre-allocation**
    Allocate large chunks up front to avoid frequent small allocations.
2. **Fixed-size Buckets**
    Route different allocation sizes to different sub-pools.
3. **Deferred Deallocation**
    Freed blocks are cached and reused instead of returned to the OS immediately.
4. **Block Coalescing**
    Adjacent free blocks are merged to reduce fragmentation.

These are exactly the techniques implemented in modern allocators — including `std::pmr`.

Improvement 1: **Reduce the number of malloc/new calls (**most direct)

For example, for **frequently allocated** small blocks of memory like vector<string>/unordered_map:

Default allocator: numerous small malloc/free

pmr + pool/monotonic: many allocations become O(1) bump or freelist hits

Improvement 2: **More controllable fragmentation and better locality**

monotonic_buffer_resource: similar to arena, basically no external fragmentation (because it is not released separately)

pool_resource: bucketed by size class, reducing external fragmentation and improving reusability

Improvement 3: **Make "changing allocator" no longer a refactoring project**

Previously you had to change a bunch of template parameters/custom containers; now:

Code unchanged

Only change resource

------

## 4. Key Components Provided by `std::pmr`

### 4.1 `std::pmr::memory_resource`

| Property | Description                                                  |
| -------- | ------------------------------------------------------------ |
| Type     | Abstract base class                                          |
| Role     | Defines raw memory allocation interface                      |
| Key APIs | `do_allocate`, `do_deallocate`                               |
| Use case | Base class for custom allocators (arenas, pools, region allocators) |

This is the **foundation** of the entire PMR ecosystem.

------

### 4.2 `std::pmr::polymorphic_allocator<T>`

| Property    | Description                                   |
| ----------- | --------------------------------------------- |
| Type        | STL-compatible allocator                      |
| Role        | Bridges containers and memory resources       |
| Key feature | Runtime polymorphism                          |
| Use case    | Bind containers to specific memory strategies |

This allows code like:

```
std::pmr::vector<int> v{my_resource};
```

without changing container logic.

------

### 4.3 `std::pmr::synchronized_pool_resource`

| Property         | Description                     |
| ---------------- | ------------------------------- |
| Thread safety    | ✅ Yes                           |
| Allocation model | Size-segregated pools           |
| Fragmentation    | Automatic coalescing            |
| Overhead         | Higher (locks)                  |
| Use case         | Multi-threaded servers, engines |

This is a **general-purpose, thread-safe memory pool**, suitable for long-lived objects in concurrent environments.

------

### 4.4 `std::pmr::unsynchronized_pool_resource`

| Property         | Description                  |
| ---------------- | ---------------------------- |
| Thread safety    | ❌ No                         |
| Allocation model | Same as synchronized version |
| Performance      | Higher (no locking)          |
| Use case         | Single-threaded hot paths    |

This is ideal when **maximum allocation speed** is required and concurrency is controlled externally.

------

### 4.5 `std::pmr::monotonic_buffer_resource`

| Property        | Description                    |
| --------------- | ------------------------------ |
| Allocation      | Monotonic (bump-pointer)       |
| Individual free | ❌ Not supported                |
| Fragmentation   | Zero                           |
| Performance     | Extremely high                 |
| Use case        | Short-lived, batch allocations |

This is effectively a **standardized Arena allocator**.

## 5. Why `std::pmr` Is a Big Deal

Before C++17:

- Memory pools were **ad-hoc**
- Containers were **allocator-hostile**
- Switching allocation strategy required code changes

With `std::pmr`:

- Memory strategy is **decoupled**
- Containers remain **unchanged**
- Allocation policy becomes **runtime-configurable**

This dramatically lowers the barrier to applying advanced memory optimizations.

memory pools and `std::pmr` are primarily about controlling fragmentation, not eliminating it universally.

Arena-style allocators avoid external fragmentation entirely by never freeing individual objects, while pool-based allocators reduce fragmentation by size segregation and reuse.

`std::pmr` standardizes these strategies and makes memory management policies explicit and swappable without changing container code.





In C:

```
typedef struct Allocator {
    void* (*alloc)(void* ctx, size_t size);
    void  (*free)(void* ctx, void* ptr);
    void* ctx;
} Allocator;

typedef struct {
    char* ptr;
    size_t remaining;
} Arena;

void arena_init(Arena* a, void* buf, size_t size) {
    a->ptr = buf;
    a->remaining = size;
}

void* arena_alloc(Arena* a, size_t size) {
    if (size > a->remaining) return NULL;
    void* p = a->ptr;
    a->ptr += size;
    a->remaining -= size;
    return p;
}
```

TLSF（Two-Level Segregated Fit）.In C, the equivalents of `std::pmr` are explicit allocator structs, arenas, slabs, and pools passed manually through APIs. `std::pmr` doesn’t invent new allocation strategies—it standardizes decades of C allocator practice and integrates them cleanly with C++ containers.

```
typedef struct FreeNode {
    struct FreeNode* next;
} FreeNode;

typedef struct {
    FreeNode* free_list;
    size_t obj_size;
} Pool;
