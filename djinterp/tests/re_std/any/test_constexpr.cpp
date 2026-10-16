/*******************************************************************************
* djinterp [test]                                             test_constexpr.cpp
*
* re_std::any compile-time evaluation tests:
*
*   LITERAL-TYPE REQUIREMENT:
*   For `any` to be usable as a constexpr variable, it must satisfy
* the literal-type rules of [basic.types].  In particular, it needs
* either a trivial destructor or - from C++20 onward - a constexpr
* destructor.  Because re_std::any must run heap cleanup, the
* destructor is non-trivial; consequently:
*
*     - C++11 / C++14 / C++17: `any` is NOT a literal type.
*       Member functions may be marked constexpr (and they are),
*       but they cannot actually be invoked in a constant-
*       expression context because no constexpr `any` instance
*       can exist.  What DOES work at compile time:
*         - the any_type_id_of<T>::value traits
*         - the internal::any_category_of<T>::value traits
*         - the internal::is_sbo_type<T>::value predicate
*
*     - C++20+: with the C++20 constexpr-destructor extension,
*       re_std::any becomes a literal type and SBO-stored values
*       become fully constexpr-usable.  This is when the tier
*       ladder pays off:
*         - construction, observers, get<T>()        (C++11+)
*         - move, copy/move assign, reset, swap      (C++14+)
*         - copy constructor with branch             (C++17+)
*       all become invocable at compile time.
*
*
* path:      /tests/djinterp/re_std/any/test_constexpr.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "restd_any_tests.hpp"


using re_std::any;
using re_std::DAnyCategory;


// ===========================================================================
// TIER 0: TRAITS (WORKS ON EVERY STANDARD)
// ===========================================================================

TEST_CASE(constexpr_tier, type_id_distinct_per_type)
{
    using re_std::any_type_id_of;

    // type id is a function-pointer constant.  In C++11+ these are
    // initializer-constant-expressions; in C++98 they are link-time
    // constants.  Either way, distinctness is observable at runtime.

    EXPECT_NE(any_type_id_of<int>::value,    any_type_id_of<long>::value);
    EXPECT_NE(any_type_id_of<int>::value,    any_type_id_of<unsigned>::value);
    EXPECT_NE(any_type_id_of<float>::value,  any_type_id_of<double>::value);
    EXPECT_NE(any_type_id_of<int*>::value,
              any_type_id_of<const int*>::value);
}


TEST_CASE(constexpr_tier, category_traits_are_compile_time)
{
    using re_std::internal::any_category_of;

    EXPECT_EQ(any_category_of<bool>::value,   int(DAnyCategory::cat_bool));
    EXPECT_EQ(any_category_of<int>::value,    int(DAnyCategory::cat_signed));
    EXPECT_EQ(any_category_of<unsigned int>::value,
              int(DAnyCategory::cat_unsigned));
    EXPECT_EQ(any_category_of<double>::value,
              int(DAnyCategory::cat_floating));
    EXPECT_EQ(any_category_of<int*>::value,
              int(DAnyCategory::cat_pointer));
    EXPECT_EQ(any_category_of<const int*>::value,
              int(DAnyCategory::cat_cpointer));

    // verify these are usable in constant-expression contexts on
    // every standard via a switch (an ICE context)
    int x = 0;
    switch (any_category_of<int>::value)
    {
        case DAnyCategory::cat_signed: x = 1; break;
        default:                        x = 0; break;
    }
    EXPECT_EQ(x, 1);
}


TEST_CASE(constexpr_tier, is_sbo_predicate_compile_time)
{
    using re_std::internal::is_sbo_type;

    EXPECT_TRUE(is_sbo_type<int>::value);
    EXPECT_TRUE(is_sbo_type<bool>::value);
    EXPECT_TRUE(is_sbo_type<double>::value);
    EXPECT_TRUE(is_sbo_type<int*>::value);
    EXPECT_FALSE(is_sbo_type<restd_any_tests::Aggregate>::value);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // usable as non-type template argument (ICE)
    static_assert(is_sbo_type<int>::value,                       "");
    static_assert(is_sbo_type<double>::value,                    "");
    static_assert(!is_sbo_type<restd_any_tests::Aggregate>::value, "");
#endif
}


// ===========================================================================
// C++20 TIER: ANY-AS-CONSTEXPR-VARIABLE
// ===========================================================================
// Only here, with a constexpr destructor, can `any` be a literal
// type and produce real compile-time results from the SBO path.

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

TEST_CASE(constexpr_tier, cpp20_construction)
{
    constexpr any a_empty;
    constexpr any a_true(true);
    constexpr any a_int(42);
    constexpr any a_uint(42u);
    constexpr any a_long(42L);
    constexpr any a_double(3.14);

    static_assert(!a_empty.has_value());
    static_assert(a_true.has_value());
    static_assert(a_int.has_value());

    static_assert(static_cast<bool>(a_true));
    static_assert(!static_cast<bool>(a_empty));

    static_assert(a_empty.category()  == DAnyCategory::cat_empty);
    static_assert(a_true.category()   == DAnyCategory::cat_bool);
    static_assert(a_int.category()    == DAnyCategory::cat_signed);
    static_assert(a_uint.category()   == DAnyCategory::cat_unsigned);
    static_assert(a_double.category() == DAnyCategory::cat_floating);

    static_assert(!a_empty.is_sbo());
    static_assert(a_int.is_sbo());
    static_assert(a_double.is_sbo());

    static_assert(a_int.holds<int>());
    static_assert(!a_int.holds<long>());

    static_assert(a_int.get<int>()         == 42);
    static_assert(a_uint.get<unsigned>()   == 42u);
    static_assert(a_long.get<long>()       == 42L);
    static_assert(a_true.get<bool>()       == true);
    static_assert(a_double.get<double>()   == 3.14);

    EXPECT_TRUE(true);
}


TEST_CASE(constexpr_tier, cpp20_pointer_storage_runtime)
{
    // note: SBO stores pointers as void* / const void*; the
    // round-trip static_cast<T*>(void*) is not allowed in constant
    // expressions even in C++20 ([expr.const]/(5.10)).  So pointer
    // values via any are usable only at runtime, even on C++20+.
    int* p = nullptr;
    any  a(p);
    EXPECT_TRUE(a.holds<int*>());
    EXPECT_EQ(a.get<int*>(), p);
}


namespace
{

constexpr int
cpp20_make_via_move_assign()
{
    any a(10);
    any b(20);
    b = a;
    a = any(30);
    return a.get<int>() + b.get<int>();
}


constexpr int
cpp20_make_via_reset()
{
    any a(5);
    a.reset();
    return a.has_value() ? -1 : 99;
}


constexpr int
cpp20_make_via_swap()
{
    any a(1);
    any b(2);
    a.swap(b);
    return a.get<int>() * 10 + b.get<int>();
}


constexpr int
cpp20_make_via_move_ctor()
{
    any a(7);
    any b(static_cast<any&&>(a));
    return b.get<int>();
}


constexpr int
cpp20_copy_construct_sbo()
{
    any a(123);
    any b(a);
    return b.get<int>();
}

}  // namespace


TEST_CASE(constexpr_tier, cpp20_relaxed_body_operations)
{
    static_assert(cpp20_make_via_move_assign() == 40);
    static_assert(cpp20_make_via_reset()       == 99);
    static_assert(cpp20_make_via_swap()        == 21);
    static_assert(cpp20_make_via_move_ctor()   == 7);
    static_assert(cpp20_copy_construct_sbo()   == 123);

    EXPECT_TRUE(true);
}


TEST_CASE(constexpr_tier, cpp20_chained_operations)
{
    constexpr auto compute = []() constexpr
    {
        any a(1);
        any b(2);
        any c(3);

        any temp = a;
        a = b;
        b = c;
        c = temp;

        return a.get<int>() * 100 + b.get<int>() * 10 + c.get<int>();
    };

    static_assert(compute() == 231);
    EXPECT_TRUE(true);
}

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


// ===========================================================================
// RUNTIME COUNTERPART: VALUES MATCH WHAT THE CONSTEXPR FORM WOULD YIELD
// ===========================================================================

TEST_CASE(constexpr_tier, runtime_construction_matches)
{
    any a_int(42);
    any a_double(3.14);
    any a_bool(true);

    EXPECT_TRUE(a_int.has_value());
    EXPECT_EQ(a_int.get<int>(),     42);
    EXPECT_EQ(a_double.get<double>(), 3.14);
    EXPECT_EQ(a_bool.get<bool>(),   true);
}


TEST_CASE(constexpr_tier, runtime_move_and_swap)
{
    any a(1);
    any b(2);
    a.swap(b);
    EXPECT_EQ(a.get<int>(), 2);
    EXPECT_EQ(b.get<int>(), 1);

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    any c(7);
    any d(static_cast<any&&>(c));
    EXPECT_EQ(d.get<int>(), 7);
    EXPECT_FALSE(c.has_value());
#endif
}


// ===========================================================================
// HEAP PATH NEVER ENTERS A CONSTANT EXPRESSION
// ===========================================================================
// New/delete are not constant-expression operations (as of C++20
// transient allocations, they may live inside one but not survive),
// so heap-stored any can never produce a useful constexpr result.
// We document this only at runtime.

TEST_CASE(constexpr_tier, heap_runtime_only)
{
#if D_ENV_CPP98_HAS_NEW
    any a(restd_any_tests::Aggregate(1, 2.0));
    EXPECT_TRUE(a.holds<restd_any_tests::Aggregate>());
    EXPECT_FALSE(a.is_sbo());
#else
    EXPECT_TRUE(true);
#endif
}
