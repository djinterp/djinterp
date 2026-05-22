/******************************************************************************
* djinterp [test]                                type_traits_tests_macros_op.cpp
*
*   Unit tests for the operator-detection macros and the legacy
* HAS_METHOD_OF_TYPE family in Section 0.3 of type_traits.hpp:
*     - D_TRAIT_DETECT_BINARY_OP   (covered indirectly via HAS_BINARY_OP)
*     - D_TRAIT_HAS_BINARY_OP
*     - D_TRAIT_DETECT_UNARY_OP    (covered indirectly via HAS_UNARY_OP)
*     - D_TRAIT_HAS_UNARY_OP
*     - HAS_METHOD_OF_TYPE         (legacy enable_if-expression family)
*     - HAS_METHOD_OF_TYPE_ARGS
*     - HAS_METHOD_OF_TYPE_V
*     - HAS_METHOD_OF_TYPE_ARGS_V
*
*   D_TRAIT_HAS_BINARY_OP / D_TRAIT_HAS_UNARY_OP rely on operator overload
* resolution: the trait should fire for built-in arithmetic types AND for
* user-defined types that overload the operator, and should NOT fire for
* a class that doesn't.  Specific tested operators:
*     - binary +, ==, <
*     - unary  -, !, *  (dereference)
*
*   The HAS_METHOD_OF_TYPE family is a separate set of macros that expand
* to `std::enable_if_t<std::is_same_v<...>>` expressions rather than to
* trait definitions.  They're tested via SFINAE-overload tag-dispatch
* helpers defined in an anonymous namespace.
*
*
* path:      /inc/djinterp/test/type_traits_tests_macros_op.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST

namespace
{

// addable_t
//   subject: overloads binary `+` and unary `-`, accepts equality and
// dereference (operator*). Drives the positive case for every operator
// trait below.
struct addable_t
{
    int data;

    addable_t() : data(0) { return; }
    explicit addable_t(int _v) : data(_v) { return; }

    friend addable_t
    operator+(const addable_t& _a, const addable_t& _b)
    {
        return addable_t(_a.data + _b.data);
    }

    friend bool
    operator==(const addable_t& _a, const addable_t& _b)
    {
        return _a.data == _b.data;
    }

    friend bool
    operator<(const addable_t& _a, const addable_t& _b)
    {
        return _a.data < _b.data;
    }

    addable_t
    operator-() const
    {
        return addable_t(-data);
    }

    bool
    operator!() const
    {
        return data == 0;
    }

    int
    operator*() const
    {
        return data;
    }
};

// inert_t
//   subject: defines nothing operator-related. Negative case for every
// operator trait below.
struct inert_t
{
    int data;
};

// sized_box
//   subject: has a `count()` method returning std::size_t.  Drives the
// HAS_METHOD_OF_TYPE family — checks both the positive and negative
// branches of strict return-type matching.
struct sized_box
{
    std::size_t
    count() const
    {
        return 0;
    }

    int
    width(int _x) const
    {
        (void)_x;
        return _x * 2;
    }
};

// bare_box
//   subject: no methods at all.  Negative case for HAS_METHOD_OF_TYPE
// family.
struct bare_box
{};

}  // namespace


// =========================================================================
// I.   D_TRAIT_HAS_BINARY_OP  (compile-time)
// =========================================================================

D_TRAIT_HAS_BINARY_OP(has_op_plus,      +)
D_TRAIT_HAS_BINARY_OP(has_op_equal,    ==)
D_TRAIT_HAS_BINARY_OP(has_op_less,      <)

// addable_t supports +, ==, <
static_assert(has_op_plus<addable_t>::value  == true,
              "D_TRAIT_HAS_BINARY_OP: addable_t + addable_t -> true");
static_assert(has_op_equal<addable_t>::value == true,
              "D_TRAIT_HAS_BINARY_OP: addable_t == addable_t -> true");
static_assert(has_op_less<addable_t>::value  == true,
              "D_TRAIT_HAS_BINARY_OP: addable_t < addable_t -> true");

// builtin int supports all three
static_assert(has_op_plus<int>::value  == true,
              "D_TRAIT_HAS_BINARY_OP: int + int -> true (builtin)");
static_assert(has_op_equal<int>::value == true,
              "D_TRAIT_HAS_BINARY_OP: int == int -> true (builtin)");
static_assert(has_op_less<int>::value  == true,
              "D_TRAIT_HAS_BINARY_OP: int < int -> true (builtin)");

// inert_t supports none
static_assert(has_op_plus<inert_t>::value  == false,
              "D_TRAIT_HAS_BINARY_OP: inert_t + inert_t -> false");
static_assert(has_op_equal<inert_t>::value == false,
              "D_TRAIT_HAS_BINARY_OP: inert_t == inert_t -> false");
static_assert(has_op_less<inert_t>::value  == false,
              "D_TRAIT_HAS_BINARY_OP: inert_t < inert_t -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(has_op_plus_v<addable_t>  == true,
                  "D_TRAIT_HAS_BINARY_OP _v alias (true case)");
    static_assert(has_op_plus_v<inert_t>    == false,
                  "D_TRAIT_HAS_BINARY_OP _v alias (false case)");
#endif


// =========================================================================
// II.  D_TRAIT_HAS_UNARY_OP  (compile-time)
// =========================================================================

D_TRAIT_HAS_UNARY_OP(has_op_neg,    -)
D_TRAIT_HAS_UNARY_OP(has_op_not,    !)
D_TRAIT_HAS_UNARY_OP(has_op_deref,  *)

// addable_t supports -, !, and *
static_assert(has_op_neg<addable_t>::value   == true,
              "D_TRAIT_HAS_UNARY_OP: -addable_t -> true");
static_assert(has_op_not<addable_t>::value   == true,
              "D_TRAIT_HAS_UNARY_OP: !addable_t -> true");
static_assert(has_op_deref<addable_t>::value == true,
              "D_TRAIT_HAS_UNARY_OP: *addable_t -> true");

// builtin int supports - and !; pointer types support *
static_assert(has_op_neg<int>::value == true,
              "D_TRAIT_HAS_UNARY_OP: -int -> true (builtin)");
static_assert(has_op_not<int>::value == true,
              "D_TRAIT_HAS_UNARY_OP: !int -> true (builtin)");
static_assert(has_op_deref<int*>::value == true,
              "D_TRAIT_HAS_UNARY_OP: *int* -> true (builtin)");

// negative cases
static_assert(has_op_neg<inert_t>::value   == false,
              "D_TRAIT_HAS_UNARY_OP: -inert_t -> false");
static_assert(has_op_deref<int>::value     == false,
              "D_TRAIT_HAS_UNARY_OP: *int -> false (cannot deref non-pointer)");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(has_op_neg_v<addable_t> == true,
                  "D_TRAIT_HAS_UNARY_OP _v alias (true case)");
    static_assert(has_op_neg_v<inert_t>   == false,
                  "D_TRAIT_HAS_UNARY_OP _v alias (false case)");
#endif


// =========================================================================
// III. HAS_METHOD_OF_TYPE family  (compile-time + runtime)
// =========================================================================
//   These macros expand to SFINAE expressions, not to trait definitions,
// so we exercise them through overload sets in an anonymous namespace.
// A SFINAE-positive call selects the constrained overload (-> int);
// a SFINAE-negative call falls through to the variadic catch-all
// (-> long).

namespace
{

// probe_returns_size_t
//   constrained overload selectable only if `_Type::count()` returns
// exactly `std::size_t`. Implements the HAS_METHOD_OF_TYPE macro.
template<typename _Type,
         typename = HAS_METHOD_OF_TYPE(count, std::size_t)>
int
probe_returns_size_t(int)
{
    return 0;
}

template<typename>
long
probe_returns_size_t(...)
{
    return 0;
}

// probe_returns_int_with_args
//   constrained overload selectable only if `_Type::width(int)` returns
// exactly `int`. Implements the HAS_METHOD_OF_TYPE_ARGS macro.
template<typename _Type,
         typename = HAS_METHOD_OF_TYPE_ARGS(width, int, std::declval<int>())>
int
probe_returns_int_with_args(int)
{
    return 0;
}

template<typename>
long
probe_returns_int_with_args(...)
{
    return 0;
}

}  // namespace


// HAS_METHOD_OF_TYPE -- sized_box::count returns std::size_t (positive)
static_assert(std::is_same<decltype(probe_returns_size_t<sized_box>(0)),
                           int>::value,
              "HAS_METHOD_OF_TYPE: sized_box::count() returns std::size_t -> selects constrained");

// HAS_METHOD_OF_TYPE -- bare_box has no count() (negative)
static_assert(std::is_same<decltype(probe_returns_size_t<bare_box>(0)),
                           long>::value,
              "HAS_METHOD_OF_TYPE: bare_box has no count() -> falls to variadic");

// HAS_METHOD_OF_TYPE -- builtin int has no count() (negative)
static_assert(std::is_same<decltype(probe_returns_size_t<int>(0)),
                           long>::value,
              "HAS_METHOD_OF_TYPE: int has no count() -> falls to variadic");

// HAS_METHOD_OF_TYPE_ARGS -- sized_box::width(int) returns int (positive)
static_assert(std::is_same<decltype(probe_returns_int_with_args<sized_box>(0)),
                           int>::value,
              "HAS_METHOD_OF_TYPE_ARGS: sized_box::width(int) returns int -> selects constrained");

// HAS_METHOD_OF_TYPE_ARGS -- bare_box has no width (negative)
static_assert(std::is_same<decltype(probe_returns_int_with_args<bare_box>(0)),
                           long>::value,
              "HAS_METHOD_OF_TYPE_ARGS: bare_box has no width() -> falls to variadic");

// HAS_METHOD_OF_TYPE_V -- bool-valued variant
static_assert(HAS_METHOD_OF_TYPE_V(sized_box, count, std::size_t) == true,
              "HAS_METHOD_OF_TYPE_V: sized_box::count() returns size_t -> true");
static_assert(HAS_METHOD_OF_TYPE_V(sized_box, count, int) == false,
              "HAS_METHOD_OF_TYPE_V: sized_box::count() does NOT return int -> false");

// HAS_METHOD_OF_TYPE_ARGS_V -- bool-valued variant for argful methods
static_assert(HAS_METHOD_OF_TYPE_ARGS_V(sized_box, width, int,
                                         std::declval<int>()) == true,
              "HAS_METHOD_OF_TYPE_ARGS_V: width(int) returns int -> true");
static_assert(HAS_METHOD_OF_TYPE_ARGS_V(sized_box, width, long,
                                         std::declval<int>()) == false,
              "HAS_METHOD_OF_TYPE_ARGS_V: width(int) does NOT return long -> false");


// =========================================================================
// IV.  RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_macros_op(
    test_handler& _test_handler
)
{
    // ---- HAS_BINARY_OP ----
    record_assertion(_test_handler, 
        has_op_plus<addable_t>::value  == true,
        "D_TRAIT_HAS_BINARY_OP: addable_t + addable_t");
    record_assertion(_test_handler, 
        has_op_equal<addable_t>::value == true,
        "D_TRAIT_HAS_BINARY_OP: addable_t == addable_t");
    record_assertion(_test_handler, 
        has_op_less<addable_t>::value  == true,
        "D_TRAIT_HAS_BINARY_OP: addable_t < addable_t");
    record_assertion(_test_handler, 
        ( has_op_plus<int>::value  == true &&
          has_op_equal<int>::value == true &&
          has_op_less<int>::value  == true ),
        "D_TRAIT_HAS_BINARY_OP: builtin int supports +, ==, <");
    record_assertion(_test_handler, 
        has_op_plus<inert_t>::value == false,
        "D_TRAIT_HAS_BINARY_OP: inert_t -> false for +");
    record_assertion(_test_handler, 
        has_op_equal<inert_t>::value == false,
        "D_TRAIT_HAS_BINARY_OP: inert_t -> false for ==");

    // ---- HAS_UNARY_OP ----
    record_assertion(_test_handler, 
        has_op_neg<addable_t>::value == true,
        "D_TRAIT_HAS_UNARY_OP: -addable_t");
    record_assertion(_test_handler, 
        has_op_not<addable_t>::value == true,
        "D_TRAIT_HAS_UNARY_OP: !addable_t");
    record_assertion(_test_handler, 
        has_op_deref<addable_t>::value == true,
        "D_TRAIT_HAS_UNARY_OP: *addable_t");
    record_assertion(_test_handler, 
        ( has_op_neg<int>::value == true &&
          has_op_not<int>::value == true ),
        "D_TRAIT_HAS_UNARY_OP: -int and !int (builtin)");
    record_assertion(_test_handler, 
        has_op_deref<int*>::value == true,
        "D_TRAIT_HAS_UNARY_OP: *int*");
    record_assertion(_test_handler, 
        has_op_neg<inert_t>::value == false,
        "D_TRAIT_HAS_UNARY_OP: -inert_t -> false");
    record_assertion(_test_handler, 
        has_op_deref<int>::value == false,
        "D_TRAIT_HAS_UNARY_OP: *int -> false (cannot deref non-pointer)");

    // ---- HAS_METHOD_OF_TYPE ----
    record_assertion(_test_handler, 
        std::is_same<decltype(probe_returns_size_t<sized_box>(0)), int>::value,
        "HAS_METHOD_OF_TYPE: positive case selects constrained overload");
    record_assertion(_test_handler, 
        std::is_same<decltype(probe_returns_size_t<bare_box>(0)), long>::value,
        "HAS_METHOD_OF_TYPE: negative case falls to variadic");
    record_assertion(_test_handler, 
        HAS_METHOD_OF_TYPE_V(sized_box, count, std::size_t) == true,
        "HAS_METHOD_OF_TYPE_V: returns true for matching return type");
    record_assertion(_test_handler, 
        HAS_METHOD_OF_TYPE_V(sized_box, count, int) == false,
        "HAS_METHOD_OF_TYPE_V: returns false for non-matching return type");

    // ---- HAS_METHOD_OF_TYPE_ARGS ----
    record_assertion(_test_handler, 
        std::is_same<decltype(probe_returns_int_with_args<sized_box>(0)),
                     int>::value,
        "HAS_METHOD_OF_TYPE_ARGS: positive case selects constrained overload");
    record_assertion(_test_handler, 
        std::is_same<decltype(probe_returns_int_with_args<bare_box>(0)),
                     long>::value,
        "HAS_METHOD_OF_TYPE_ARGS: negative case falls to variadic");
    record_assertion(_test_handler, 
        HAS_METHOD_OF_TYPE_ARGS_V(sized_box, width, int,
                                  std::declval<int>()) == true,
        "HAS_METHOD_OF_TYPE_ARGS_V: matching return type -> true");
    record_assertion(_test_handler, 
        HAS_METHOD_OF_TYPE_ARGS_V(sized_box, width, long,
                                  std::declval<int>()) == false,
        "HAS_METHOD_OF_TYPE_ARGS_V: non-matching return type -> false");

    return;
}


NS_END  // test
NS_END  // djinterp
