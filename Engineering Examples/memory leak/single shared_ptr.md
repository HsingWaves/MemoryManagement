```
#include <functional>
#include <iostream>
#include <memory>

struct Service : std::enable_shared_from_this<Service> {
    std::function<void()> callback;

    Service()  { std::cout << "Service ctor\n"; }
    ~Service() { std::cout << "Service dtor\n"; }

    void setup() {
        auto self = shared_from_this();
        callback = [self]() {
            self->process();
        };
    }

    void process() {
        std::cout << "process()\n";
    }
};

int main() {
    {
        auto s = std::make_shared<Service>();
        s->setup();
        s->callback(); // works
    }
    std::cout << "scope ended (if no dtor printed -> leaked)\n";
}

```

 **对象关系**：`Service` 里有一个 `std::function` 成员，`std::function` 里保存了一个 **捕获了 `std::shared_ptr<Service>` 的 lambda**。这在汇编里表现为：

1. **lambda 对象里包含一个 `shared_ptr<Service>` 成员**
    你这段就直接说明了捕获的是 `shared_ptr`（而不是 `weak_ptr`）：

- `Service::setup()::'lambda'()::~()` 里调用了 `std::shared_ptr<Service>::~shared_ptr()`
- 这意味着：**lambda 内部确实持有一个 shared_ptr**（析构时要把它析构掉）。

1. **这个 lambda 被赋给了 `Service::callback`（成员 std::function）**
    在 `Service::setup()` 里，你能看到 `std::function::operator=` 把 lambda 塞进 `callback`：