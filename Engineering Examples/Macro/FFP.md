```
#ifndef FP_H
#define FP_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h> // for memmove

// =========================
// 1) 生成“类型化”的 API
// =========================
// 用法：FP_DEFINE(int, Int, int)
// 生成：
//   - Int_map_inplace(int* a, size_t n, int(*f)(void*, int), void* ctx)
//   - Int_filter_inplace(int* a, size_t* n, bool(*pred)(void*, int), void* ctx)
//   - Int_reduce(int* a, size_t n, Acc init, Acc(*f)(void*, Acc, int), void* ctx)
// 说明：
//   - filter 是“原地压缩”，会修改 *n
//   - reduce 的 Acc 类型你可以自己指定（下面用宏再生成）

#define FP_DEFINE(T, Prefix) 
    static inline void Prefix##_map_inplace( 
        T* a, size_t n, T (*f)(void* ctx, T x), void* ctx) { 
        for (size_t i = 0; i < n; i++) { 
            a[i] = f(ctx, a[i]); 
        } 
    } 
    static inline void Prefix##_foreach( 
        const T* a, size_t n, void (*f)(void* ctx, T x), void* ctx) { 
        for (size_t i = 0; i < n; i++) { 
            f(ctx, a[i]); 
        } 
    } 
    static inline void Prefix##_filter_inplace( 
        T* a, size_t* n, bool (*pred)(void* ctx, T x), void* ctx) { 
        size_t j = 0; 
        for (size_t i = 0; i < *n; i++) { 
            T x = a[i]; 
            if (pred(ctx, x)) { 
                a[j++] = x; 
            } 
        } 
        *n = j; 
    }

// reduce 需要 Acc 类型：再提供一个宏
#define FP_DEFINE_REDUCE(T, Prefix, Acc) 
    static inline Acc Prefix##_reduce( 
        const T* a, size_t n, Acc init, Acc (*f)(void* ctx, Acc acc, T x), void* ctx) { 
        Acc acc = init; 
        for (size_t i = 0; i < n; i++) { 
            acc = f(ctx, acc, a[i]); 
        } 
        return acc; 
    }

// =========================
// 2) 组合子：compose / pipe
// =========================
// f(g(x))：compose2
#define FP_DEFINE_COMPOSE2(T, Prefix) 
    typedef struct { 
        T (*f)(void*, T); void* fctx; 
        T (*g)(void*, T); void* gctx; 
    } Prefix##_Compose2Ctx; 
    static inline T Prefix##_compose2_fn(void* ctx, T x) { 
        Prefix##_Compose2Ctx* c = (Prefix##_Compose2Ctx*)ctx; 
        return c->f(c->fctx, c->g(c->gctx, x)); 
    }

// =========================
// 3) slice / take / drop（纯数组视图）
// =========================
typedef struct {
    void* data;
    size_t len;
    size_t elem_size;
} FP_Slice;

static inline FP_Slice fp_slice(void* data, size_t len, size_t elem_size) {
    FP_Slice s; s.data = data; s.len = len; s.elem_size = elem_size; return s;
}
static inline FP_Slice fp_take(FP_Slice s, size_t n) {
    if (n < s.len) s.len = n;
    return s;
}
static inline FP_Slice fp_drop(FP_Slice s, size_t n) {
    if (n >= s.len) { s.data = (char*)s.data + s.len * s.elem_size; s.len = 0; return s; }
    s.data = (char*)s.data + n * s.elem_size;
    s.len -= n;
    return s;
}

#endif // FP_H

```

```
#include <stdio.h>
#include "fp.h"

// 为 int 生成 API
FP_DEFINE(int, Int)
FP_DEFINE_REDUCE(int, Int, int)
FP_DEFINE_COMPOSE2(int, Int)

// ========== 1) map：平方 ==========
static int square(void* ctx, int x) {
    (void)ctx;
    return x * x;
}

// ========== 2) filter：只保留偶数 ==========
static bool is_even(void* ctx, int x) {
    (void)ctx;
    return (x % 2) == 0;
}

// ========== 3) reduce：求和 ==========
static int sum(void* ctx, int acc, int x) {
    (void)ctx;
    return acc + x;
}

// ========== 4) closure：捕获一个参数（加偏移） ==========
typedef struct { int add; } AddCtx;
static int add_k(void* ctx, int x) {
    AddCtx* c = (AddCtx*)ctx;
    return x + c->add;
}

// ========== 5) closure：捕获两个参数（线性变换 ax+b） ==========
typedef struct { int a, b; } LinCtx;
static int lin(void* ctx, int x) {
    LinCtx* c = (LinCtx*)ctx;
    return c->a * x + c->b;
}

static void print_one(void* ctx, int x) {
    (void)ctx;
    printf("%d ", x);
}

int main() {
    int a[] = {1,2,3,4,5};
    size_t n = 5;

    // map square
    Int_map_inplace(a, n, square, NULL);
    Int_foreach(a, n, print_one, NULL);
    printf("\n"); // 1 4 9 16 25

    // filter even
    Int_filter_inplace(a, &n, is_even, NULL);
    Int_foreach(a, n, print_one, NULL);
    printf("\n"); // 4 16

    // reduce sum
    int total = Int_reduce(a, n, 0, sum, NULL);
    printf("Sum: %d\n", total); // 20

    // closure：加 10
    int b[] = {1,2,3};
    size_t bn = 3;
    AddCtx add10 = {.add = 10};
    Int_map_inplace(b, bn, add_k, &add10);
    Int_foreach(b, bn, print_one, NULL);
    printf("\n"); // 11 12 13

    // 组合：f(g(x)) 其中 f = lin(ax+b), g = add_k(+10)
    LinCtx lin2 = {.a = 2, .b = 1}; // 2x+1
    Int_Compose2Ctx comp = {
        .f = lin, .fctx = &lin2,
        .g = add_k, .gctx = &add10
    };
    int c[] = {1,2,3};
    size_t cn = 3;
    Int_map_inplace(c, cn, Int_compose2_fn, &comp);
    Int_foreach(c, cn, print_one, NULL);
    printf("\n"); // ( (x+10)*2 +1 ) => 23 25 27

    return 0;
}

```

Why is the ctx version truly a "functional library"?

Because it solves four engineering problems that your original macro approach struggles to address:

Capturing closures: ctx captures the environment.

Testable/reusable: Functions are normal C functions, allowing for unit testing and breakpoints.

Cross-platform: It runs without statements, expressions, or typeof.

Composable: It can be done using compose/pipe (I mentioned compose2 above).