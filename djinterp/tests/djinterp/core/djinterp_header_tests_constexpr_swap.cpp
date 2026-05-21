/******************************************************************************
* djinterp [test]                    djinterp_header_tests_constexpr_swap.cpp
*
*   Section III.iv tests: constexpr_swap.
******************************************************************************/

#include "./djinterp_header_tests.hpp"


// D_INTERNAL_TEST_CONSTEXPR_ACTIVE
//   macro: 1 when D_CONSTEXPR is genuinely `constexpr`, 0 when stripped by
// D_TESTING_CONSTEXPR.  Mirrors the header gate so the compile-time swap
// check is skipped if constexpr was stripped.
#if ( defined(D_TESTING_CONSTEXPR) &&  \
      (D_TESTING_CONSTEXPR == 1) )
    #define D_INTERNAL_TEST_CONSTEXPR_ACTIVE  0
#else
    #define D_INTERNAL_TEST_CONSTEXPR_ACTIVE  1
#endif


NS_DJINTERP
NS_TEST


#if ( D_ENV_LANG_IS_CPP14_OR_HIGHER &&  \
      D_INTERNAL_TEST_CONSTEXPR_ACTIVE )

// swap_first_of
//   helper: swaps two locals in a constant-evaluation context and returns the
// first, used to drive a static_assert on C++14+ where constexpr_swap is a
// genuine constexpr function (the C++11 fallback is runtime-only).
D_CONSTEXPR int
swap_first_of(
    int _a,
    int _b
)
{
    constexpr_swap(_a, _b);

    return _a;
}

#endif  // C++14+ && constexpr active


/*
tests_constexpr_swap
  Verifies constexpr_swap exchanges its operands and derives the correct
  noexcept specification.
  Tests the following:
  - two fundamentals are exchanged at runtime
  - a move-only type is exchanged (proving the body needs only move semantics,
    not copyability) and the moved-from payloads land as expected
  - swapping twice restores the original ordering
  - the call expression is of type void
  - the computed noexcept spec is true for nothrow-movable operands (int,
    move_only) and false for a throwing-move operand (throwing_move)
  - on C++14+ (where constexpr_swap is truly constexpr and not stripped), the
    swap is verified inside a constant expression
*/
bool
tests_constexpr_swap()
{
    bool ok = true;

    // fundamental exchange.
    int a = 1;
    int b = 2;
    constexpr_swap(a, b);
    ok = ok && (a == 2) && (b == 1);

    // swapping back restores the original ordering.
    constexpr_swap(a, b);
    ok = ok && (a == 1) && (b == 2);

    // move-only exchange: no copy constructor is ever required.
    internal::move_only m1(10);
    internal::move_only m2(20);
    constexpr_swap(m1, m2);
    ok = ok && (m1.value == 20) && (m2.value == 10);

    // the call yields void.
    static_assert(std::is_same<
                      decltype(constexpr_swap(std::declval<int&>(),
                                              std::declval<int&>())),
                      void>::value,
                  "constexpr_swap must return void.");

    // noexcept derivation.
    int x = 0;
    int y = 0;
    static_assert(noexcept(constexpr_swap(x, y)),
                  "swapping nothrow-movable ints must be noexcept.");

    internal::move_only mo1(0);
    internal::move_only mo2(0);
    static_assert(noexcept(constexpr_swap(mo1, mo2)),
                  "swapping a nothrow-movable type must be noexcept.");

    internal::throwing_move t1(0);
    internal::throwing_move t2(0);
    static_assert(!noexcept(constexpr_swap(t1, t2)),
                  "swapping a throwing-move type must not be noexcept.");

    ok = ok && noexcept(constexpr_swap(x, y));
    ok = ok && (!noexcept(constexpr_swap(t1, t2)));

#if ( D_ENV_LANG_IS_CPP14_OR_HIGHER &&  \
      D_INTERNAL_TEST_CONSTEXPR_ACTIVE )
    // compile-time exchange (relaxed-constexpr standards only).
    static_assert((swap_first_of(3, 9) == 9),
                  "constexpr_swap must exchange in a constant expression.");

    D_CONSTEXPR int swapped = swap_first_of(3, 9);
    ok = ok && (swapped == 9);
#endif  // C++14+ && constexpr active

    return ok;
}


NS_END  // test
NS_END  // djinterp
