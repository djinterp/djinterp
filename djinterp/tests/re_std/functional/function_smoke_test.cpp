/*******************************************************************************
* djinterp [re_std]                                      function_smoke_test.cpp
*
*
* path:      /tests/djinterp/re_std/functional/function_smoke_test.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <cassert>
#include <cstdio>
#include <string>
// djinterp
#include "djinterp/re_std/functional/function.hpp"
#include "djinterp/re_std/functional/reference_wrapper.hpp"

// free function target
static int add1(int x) { return x + 1; }
// void-returning target sink
static int g_sink = 0;
static void store(int x) { g_sink = x; }
// a class for PMF/PMD tests
struct Widget {
    int base;
    Widget(int b) : base(b) {}
    int scaled(int m) const { return base * m; }
};
// stateful mutable functor
struct Counter {
    int n;
    Counter() : n(0) {}
    int operator()() { return ++n; }   // non-const: mutates
};

int main()
{
    using re_std::function;
    int fails = 0;
    #define CHECK(c) do { if(!(c)){ std::printf("FAIL: %s\n", #c); ++fails; } } while(0)

    // 1. default-empty: bool false, ==nullptr, throws on call
    {
        function<int(int)> f;
        CHECK(!f);
        CHECK(f == nullptr);
        CHECK(nullptr == f);
        bool threw = false;
        try { f(3); } catch (const re_std::bad_function_call&) { threw = true; }
        CHECK(threw);
    }
    // 2. lambda target
    {
        function<int(int)> f = [](int x){ return x * 2; };
        CHECK(bool(f));
        CHECK(f != nullptr);
        CHECK(f(21) == 42);
    }
    // 3. function-pointer target
    {
        function<int(int)> f(&add1);
        CHECK(f(9) == 10);
        function<int(int)> f2(add1);   // implicit decay to ptr
        CHECK(f2(9) == 10);
    }
    // 4. NULL function pointer -> empty
    {
        int(*fp)(int) = 0;
        function<int(int)> f(fp);
        CHECK(!f);
    }
    // 5. copy independence
    {
        function<int(int)> a = [](int x){ return x + 100; };
        function<int(int)> b = a;      // copy
        CHECK(a(1) == 101);
        CHECK(b(1) == 101);
        a = [](int x){ return x; };    // mutate a; b must be unaffected
        CHECK(b(1) == 101);
    }
    // 6. move leaves source empty
    {
        function<int(int)> a = [](int x){ return x + 7; };
        function<int(int)> b = static_cast<function<int(int)>&&>(a);
        CHECK(bool(b));
        CHECK(!a);
        CHECK(b(0) == 7);
    }
    // 7. assign-from-lambda, assign nullptr clears
    {
        function<int(int)> f;
        f = [](int x){ return x - 1; };
        CHECK(f(5) == 4);
        f = nullptr;
        CHECK(!f);
    }
    // 8. swap (member + non-member)
    {
        function<int(int)> a = [](int){ return 1; };
        function<int(int)> b = [](int){ return 2; };
        a.swap(b);
        CHECK(a(0) == 2 && b(0) == 1);
        swap(a, b);
        CHECK(a(0) == 1 && b(0) == 2);
    }
    // 9. target<T>() correct + null on mismatch; target_type()
    {
        function<int(int)> f(&add1);
        typedef int(*fp_t)(int);
        fp_t* tp = f.target<fp_t>();
        CHECK(tp != 0);
        CHECK(*tp == &add1);
        CHECK(f.target<int>() == 0);           // wrong type -> null
        CHECK(f.target_type() == typeid(fp_t)); // RTTI parity
        const function<int(int)>& cf = f;
        CHECK(cf.target<fp_t>() != 0);          // const overload
        function<int(int)> e;
        CHECK(e.target_type() == typeid(void)); // empty -> typeid(void)
    }
    // 10. void-return callable (discards non-void result too)
    {
        function<void(int)> f = &store;
        g_sink = 0; f(77); CHECK(g_sink == 77);
        function<void(int)> f2 = [](int x){ return x; }; // int result discarded
        f2(5); // must compile and run
    }
    // 11. reference_wrapper target (invoke handles it)
    {
        Counter c;
        function<int()> f = re_std::ref(c);   // wraps a reference; shares state with c
        CHECK(f() == 1);
        CHECK(f() == 2);
        CHECK(c.n == 2);                      // mutations visible through the wrapper
    }
    // 12. stateful mutable functor stored by value
    {
        function<int()> f = Counter();
        CHECK(f() == 1);
        CHECK(f() == 2);   // internal state persists across calls
    }
    // 13. PMF via function<R(const Widget&, int)>
    {
        function<int(const Widget&, int)> f = &Widget::scaled;
        Widget w(10);
        CHECK(f(w, 3) == 30);
    }
    // 14. return type convertible (long <- int)
    {
        function<long(int)> f = [](int x){ return x + 1; };
        CHECK(f(41) == 42L);
    }
    // 15. reference-returning target (result type can't be pointer-formed)
    {
        static int cell = 5;
        function<int&()> f = []() -> int& { return cell; };
        f() = 99;
        CHECK(cell == 99);
    }
    // 16. args by value are moved through (move-only arg)
    {
        function<int(std::string)> f = [](std::string s){ return (int)s.size(); };
        std::string s = "hello";
        CHECK(f(s) == 5);
    }

    if (fails == 0) std::printf("ALL FUNCTION SMOKE TESTS PASSED\n");
    else            std::printf("%d CHECK(S) FAILED\n", fails);
    return fails ? 1 : 0;
}
