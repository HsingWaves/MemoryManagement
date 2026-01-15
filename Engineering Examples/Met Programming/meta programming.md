### Why？	

1. Templates do not solve the problem of "generics" itself, but rather the problem of calculating, dispatching, validating, and generating code for unknown types at compile time.

"Generate efficient, type-safe, zero-cost code for unknown types"

This is the core contradiction of C++:

want to write code, but want it to: Be true for any type Have no runtime overhead Have no virtual functions Have no RTTI Be as fast as hand-written code

Without templates, you only have three options:

```
void* (C-style) → Unsafe

Inheritance + virtual → Runtime overhead

Write N copies of code by hand → Unmaintainable
```

2. The second problem that templates truly solve:

"The correctness of a program depends on type relationships, not values." Once enter the world of generics, many errors are not "wrong values," but rather: Types do not support a certain operation Illegal type composition Invalid relationships between types

```
template <typename T>
void sort(T* data, size_t n);
//The implicit conditions are:
//T must be comparable
//T must be commutative
//The cost of moving/copying T must be acceptable
```

These conditions are checked at compile time; instantiation is rejected if the condition is not met; correct code is generated if the condition is met.

```
std::is_same
std::is_trivially_copyable
requries
concepts
```

3. "Compile-time dispatch, not runtime dispatch"

   Consider a key comparison:

   Runtime dispatch (traditional OOP)

   ```
   Shape* s = get_shape();
   s->draw(); // virtual dispatch
   Compile-time dispatch (templates)
   template <typename Shape>
   void draw(Shape& s) {
   s.draw(); // compile-time dispatch
   }
   ```

   Templates allow you to:

   Different types → Different implementation paths

   No if/switch statements No virtual tables Compiler can fully inline. This isn't "generics," it's:

   **Static polymorphism**

   4. "Make 'decisions' at compile time, not fix them at runtime."

      Many STL optimization logics look like this:

      ```
      if constexpr (std::is_trivially_copyable_v<T>) {
      memcpy(...)
      } else {
      for (...) copy(...)
      }
      ```

The decision happens at compile time
Branches that don't meet the condition won't generate any code at all
There are no runtime branches.
This solves the problem that:
Abstraction should not incur runtime costs.
This is the famous C++ term:
**Zero-cost abstraction**

C++ templates are not about "writing fewer copies of code," but about performing calculations, constraints, dispatches, and code generation on unknown types at compile time, thereby supporting highly composable abstractions without sacrificing performance and type safety.

------

# 1. Code Generation

**What problem does it solve?**
Generate multiple versions of code from a single definition.

## 1.1 Macro-based code generation (pre-template era)

### Idea

Use macros + token pasting to simulate generics.

### Runnable example (C)

```c
#include <stdio.h>

#define ADD_IMPL(T)        \
T add_##T(T a, T b) {      \
    return a + b;          \
}

ADD_IMPL(int)
ADD_IMPL(float)

int main() {
    printf("%d\n", add_int(1, 2));
    printf("%f\n", add_float(1.0f, 2.0f));
}
```

### Problems

- Pure text substitution
- No type checking
- Terrible error messages
- Manual instantiation

------

## 1.2 Template-based code generation (modern C++)

### Runnable example

```cpp
#include <iostream>

template <typename T>
T add(T a, T b) {
    return a + b;
}

int main() {
    std::cout << add(1, 2) << "\n";
    std::cout << add(1.5, 2.5) << "\n";
}
```

### Why templates are better

- Type-aware
- Implicit instantiation
- Compiler diagnostics point to real code
- Enables STL

------

# 2. Type Constraints

**What problem does it solve?**
Ensure templates are only instantiated for valid types.

------

## 2.1 Without constraints (error explosion)

```cpp
#include <iostream>

template <typename T>
void print(T x) {
    std::cout << x << "\n";
}

struct A {};

int main() {
    print(A{}); // hundreds of lines of errors
}
```

### Why errors are huge

- Template argument deduction succeeds
- Instantiation happens
- Errors propagate through the instantiation stack

------

## 2.2 With C++20 `requires` (explains *why* it fails)

```cpp
#include <iostream>

template <typename T>
requires requires(T x) { std::cout << x; }
void print(T x) {
    std::cout << x << "\n";
}

struct A {};

int main() {
    print(A{}); // short, clear error
}
```

### Key insight

> Constraints stop invalid templates **before instantiation**, preventing error cascades.

------

## 2.3 Pre-C++20: SFINAE (works, but unreadable)

```cpp
#include <iostream>
#include <type_traits>

template <typename T,
          typename = decltype(std::cout << std::declval<T>())>
void print(T x) {
    std::cout << x << "\n";
}
```

### Reality

- Technically powerful
- Practically unreadable
- Concepts exist to replace this

------

# 3. Compile-time Computation

**What problem does it solve?**
Evaluate logic at compile time for performance and correctness.

------

## 3.1 constexpr value computation (works)

```cpp
constexpr int square(int x) {
    return x * x;
}

static_assert(square(5) == 25);
```

------

## 3.2 Why `consteval` parameters are NOT template constants

```cpp
#include <array>

consteval auto make_array(std::size_t n) {
    return std::array<int, n>{}; // ❌ n is not a constant expression
}
```

### Key rule

> **Function parameters are never template constants**, even in `consteval` functions.

------

## 3.3 Why `std::make_index_sequence` exists

```cpp
#include <tuple>
#include <utility>
#include <iostream>

template <typename Tuple, std::size_t... I>
void print_impl(const Tuple& t, std::index_sequence<I...>) {
    ((std::cout << std::get<I>(t) << " "), ...);
}

template <typename... Ts>
void print(const std::tuple<Ts...>& t) {
    print_impl(t, std::index_sequence_for<Ts...>{});
}

int main() {
    print(std::make_tuple(1, 2.5, "hi"));
}
```

### Why this looks ugly

Because **values must be lifted into the template world** to participate in expansion.

------

# 4. Type Manipulation

**What problem does it solve?**
Compute relationships *between types*, not values.

------

## 4.1 Matching types (`T == int` is illegal)

### What we want (but can’t do):

```cpp
// if (T == int) ❌
```

### What C++ allows:

```cpp
#include <type_traits>

template <typename T>
void test() {
    if constexpr (std::is_same_v<T, int>) {
        // int-specific logic
    }
}
```

------

## 4.2 Type-level algorithms are painful

### Type-list subsequence (template version)

```cpp
#include <type_traits>

template <typename... Ts>
struct type_list {};

template <typename Sub, typename Super>
struct is_subsequence;

template <typename... Super>
struct is_subsequence<type_list<>, type_list<Super...>> : std::true_type {};

template <typename SubHead, typename... SubTail,
          typename SuperHead, typename... SuperTail>
struct is_subsequence<type_list<SubHead, SubTail...>,
                      type_list<SuperHead, SuperTail...>>
    : std::conditional_t<
          std::is_same_v<SubHead, SuperHead>,
          is_subsequence<type_list<SubTail...>, type_list<SuperTail...>>,
          is_subsequence<type_list<SubHead, SubTail...>, type_list<SuperTail...>>> {};

static_assert(is_subsequence<
    type_list<int, double>,
    type_list<int, double, float>>::value);
```

------

## 4.3 Value-level equivalent (much simpler)

```cpp
#include <array>

constexpr bool is_subsequence(auto sub, auto super) {
    std::size_t i = 0;
    for (auto x : super)
        if (i < sub.size() && x == sub[i])
            ++i;
    return i == sub.size();
}

static_assert(is_subsequence(
    std::array{1, 2},
    std::array{1, 2, 3}));
```

### Key takeaway

> Complexity comes from **types not being values**, not from the algorithm.

------

# 5. Type as Value: Zig Comparison (Conceptual)

### Zig can do this:

```zig
comptime var types = [_]type{ i32, f32, i64 };
```

- Types are values
- Compile-time loops exist
- No template recursion

### C++ cannot

- Types are not first-class
- Cannot iterate over types with variables
- Must encode logic into templates

------

### 

------

# Final Summary (Key Insight)

> Templates exist not to be clever, but because **generic programming requires compile-time reasoning about types**.
>
> C++ splits:
>
> - value computation → `constexpr`
> - type computation → templates
>
> This split makes type-level logic powerful but painful.
> New features (`requires`, reflection proposals) reduce pain, but **types will never be first-class values in C++**.

If you want, next we can:

- Compare **C++ vs Rust vs Zig** in one table
- Show **how `std::variant::visit` is implemented**
- Or explain **why static reflection still won’t fully fix this**

Just tell me.