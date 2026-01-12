```
#include <iostream>
#include <memory>

struct S {
    std::unique_ptr<S> s;
    int id;

    explicit S(int i) : id(i) {
        std::cout << "S(" << id << ") ctor\n";
    }
    ~S() {
        std::cout << "S(" << id << ") dtor\n";
    }
};

int main() {
    auto a = std::make_unique<S>(1);
    auto b = std::make_unique<S>(2);

    // Create cycle: A owns B, B owns A  => no external owner remains => leak
    a->s = std::move(b);

    // Avoid potential UB from evaluation order by taking raw pointer first
    S* b_raw = a->s.get();          // points to object 2
    b_raw->s = std::move(a);        // object 2 now owns object 1

    std::cout << "End of scope. If destructors don't print, it leaked.\n";
    return 0;
}

```

end up with a **cycle of `unique_ptr` ownership**:

- `A.s` owns `B`
- `B.s` owns `A`

Then both local variables are empty:

- `b == nullptr` (moved-from)
- `a == nullptr` (moved-from)

When the scope ends, there is **no remaining root owner** to start destruction, so neither `A` nor `B` gets deleted → **memory leak**.

```
#include <iostream>
#include <memory>

struct S : std::enable_shared_from_this<S> {
    std::shared_ptr<S> child;  // owning edge
    std::weak_ptr<S> parent;   // non-owning back edge
    int id;

    explicit S(int i) : id(i) {
        std::cout << "S(" << id << ") ctor\n";
    }
    ~S() {
        std::cout << "S(" << id << ") dtor\n";
    }
};

int main() {
    auto a = std::make_shared<S>(1);
    auto b = std::make_shared<S>(2);

    // a owns b
    a->child = b;

    // b refers back to a without owning it
    b->parent = a;

    std::cout << "End of scope. Destructors SHOULD print now.\n";
    return 0;
}

```

