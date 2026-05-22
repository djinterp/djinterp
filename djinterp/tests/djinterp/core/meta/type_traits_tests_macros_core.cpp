/******************************************************************************
* djinterp [test]                              type_traits_tests_macros_core.cpp
*
*   Unit tests for the core detection-macro family in Section 0.3 of
* type_traits.hpp:
*     - D_VOID_T
*     - D_TRAIT_VALUE_BOOL
*     - D_TRAIT_TYPE_ALIAS
*     - D_TRAIT_IS_DETECTED_AS
*     - D_TRAIT_IS_DETECTED
*     - D_TRAIT_IS_DETECTED_FROM
*
*   The macros expand to trait definitions, so the tests work by
* INSTANTIATING the macros at file scope (inside djinterp::) to produce
* test-only traits, then static_assert'ing the resulting behaviour.
*
*   Coverage notes:
*   - D_VOID_T: applied with one and multiple template arguments
*     (variadic), both well-formed and ill-formed (the latter triggers
*     SFINAE, not a hard error)
*   - D_TRAIT_IS_DETECTED: variadic — one expression, multiple
*     expressions (AND-shape). Each expansion must produce both the
*     primary `false` template and the `_v` variable-template alias.
*   - D_TRAIT_IS_DETECTED_AS: the base macro. Tested by checking that
*     the success specialization inherits from the supplied INHERIT_EXPR
*     (here we use std::integral_constant<int, 7> to make the chosen
*     base distinguishable).
*   - D_TRAIT_IS_DETECTED_FROM: tested via a trait that succeeds based
*     on detection AND inherits from std::is_integral<_Type> — so the
*     final answer depends on BOTH the probe AND the delegate trait.
*   - D_TRAIT_VALUE_BOOL: tested implicitly through every macro that
*     emits a `_v` alias (every IS_DETECTED variant).
*   - D_TRAIT_TYPE_ALIAS: applied to a trait that exposes a `::type`
*     member; the alias must yield that same type.
*
*
* path:      /inc/djinterp/test/type_traits_tests_macros_core.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST

namespace
{
// ----- subjects -----

// with_value_type_int / without_value_type
//   Two minimal subjects: one has a `value_type` typedef (int), the
// other doesn't.  All probes here are forms of "does this type have
// member X?", so a typedef-bearing and typedef-lacking pair is the
// natural test matrix.
struct with_value_type_int
{
    using value_type = int;
};

struct without_value_type
{};

// with_type_and_data
//   subject: has both a `value_type` typedef AND a `data` member of the
// same type. Used by the multi-expression variant of D_TRAIT_IS_DETECTED
// to verify the AND-shape requirement.
struct with_type_and_data
{
    using value_type = int;
    value_type data;
};

// with_type_only_no_data
//   subject: has the typedef but NOT the data member. Negative case for
// the multi-expression form (one expression OK, other fails -> overall
// false).
struct with_type_only_no_data
{
    using value_type = int;
};

}  // namespace


// =========================================================================
// I.   D_VOID_T  (compile-time)
// =========================================================================
//   D_VOID_T<X> -> void if X is well-formed, ill-formed otherwise (which
// in SFINAE contexts means "substitution fails").  Since we can't
// statically observe a substitution-failure outside a detection idiom,
// we instead verify that D_VOID_T<...> evaluates to void for well-formed
// inputs and use the detection idiom for the negative side.

static_assert(std::is_same<D_VOID_T<int>, void>::value,
              "D_VOID_T<int> should be void");
static_assert(std::is_same<D_VOID_T<int, char>, void>::value,
              "D_VOID_T<int, char> should be void (variadic)");
static_assert(std::is_same<D_VOID_T<int, char, float, double>, void>::value,
              "D_VOID_T<...4 types> should be void (variadic)");
static_assert(std::is_same<D_VOID_T<>, void>::value,
              "D_VOID_T<> should be void (empty pack)");


// =========================================================================
// II.  D_TRAIT_IS_DETECTED  (compile-time)
// =========================================================================

// macro_trait_value_type
//   trait: single-expression form. True iff `_Type::value_type` exists.
D_TRAIT_IS_DETECTED(macro_trait_value_type,
    typename _Type::value_type)

static_assert(macro_trait_value_type<with_value_type_int>::value == true,
              "D_TRAIT_IS_DETECTED single-expr: detects value_type");
static_assert(macro_trait_value_type<without_value_type>::value == false,
              "D_TRAIT_IS_DETECTED single-expr: rejects when value_type absent");
static_assert(macro_trait_value_type<int>::value == false,
              "D_TRAIT_IS_DETECTED single-expr: builtin int -> false");

// _v alias must come from D_TRAIT_VALUE_BOOL emitted by the macro
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(macro_trait_value_type_v<with_value_type_int> == true,
                  "D_TRAIT_IS_DETECTED emits _v alias (true case)");
    static_assert(macro_trait_value_type_v<without_value_type> == false,
                  "D_TRAIT_IS_DETECTED emits _v alias (false case)");
#endif

// macro_trait_value_type_and_data
//   trait: multi-expression (variadic) form. True iff BOTH probes
// well-formed. Demonstrates the AND-shape of D_VOID_T<...> over multiple
// expressions.
D_TRAIT_IS_DETECTED(macro_trait_value_type_and_data,
    typename _Type::value_type,
    decltype(std::declval<_Type&>().data))

static_assert(macro_trait_value_type_and_data<with_type_and_data>::value == true,
              "D_TRAIT_IS_DETECTED multi-expr: both probes succeed -> true");
static_assert(macro_trait_value_type_and_data<with_type_only_no_data>::value == false,
              "D_TRAIT_IS_DETECTED multi-expr: second probe fails -> false");
static_assert(macro_trait_value_type_and_data<without_value_type>::value == false,
              "D_TRAIT_IS_DETECTED multi-expr: first probe fails -> false");
static_assert(macro_trait_value_type_and_data<int>::value == false,
              "D_TRAIT_IS_DETECTED multi-expr: builtin -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(macro_trait_value_type_and_data_v<with_type_and_data> == true,
                  "D_TRAIT_IS_DETECTED multi-expr _v alias (true)");
    static_assert(macro_trait_value_type_and_data_v<with_type_only_no_data> == false,
                  "D_TRAIT_IS_DETECTED multi-expr _v alias (false)");
#endif


// =========================================================================
// III. D_TRAIT_IS_DETECTED_AS  (compile-time)
// =========================================================================
//   The base macro emits a primary `false` plus a SUCCESS specialization
// whose base class is the supplied INHERIT_EXPR. We pin the chosen base
// to a distinguishable type so we can observe inheritance directly.

// macro_trait_count_7
//   trait: detects `value_type`. On success, inherits from
// std::integral_constant<int, 7> instead of std::true_type. This lets us
// verify the base macro's INHERIT_EXPR plumbing.
//   The INHERIT_EXPR argument has a comma inside its template-arg list,
// which the C preprocessor would otherwise treat as a fourth macro
// argument — so we alias the type first and pass the alias name.
using macro_trait_count_7_base = std::integral_constant<int, 7>;

D_TRAIT_IS_DETECTED_AS(macro_trait_count_7,
    typename _Type::value_type,
    macro_trait_count_7_base)

static_assert(macro_trait_count_7<with_value_type_int>::value == 7,
              "D_TRAIT_IS_DETECTED_AS success inherits from supplied base");
static_assert(std::is_base_of<std::integral_constant<int, 7>,
                              macro_trait_count_7<with_value_type_int>>::value,
              "D_TRAIT_IS_DETECTED_AS success: base is the supplied INHERIT_EXPR");

// failure path -- primary template inherits from std::false_type
static_assert(macro_trait_count_7<without_value_type>::value == false,
              "D_TRAIT_IS_DETECTED_AS failure: ::value is false");
static_assert(std::is_base_of<std::false_type,
                              macro_trait_count_7<without_value_type>>::value,
              "D_TRAIT_IS_DETECTED_AS failure: base is std::false_type");


// =========================================================================
// IV.  D_TRAIT_IS_DETECTED_FROM  (compile-time)
// =========================================================================
//   On detection success the trait inherits from BASE_TRAIT<_Type>
// instead of std::true_type. Use it with std::is_integral so the final
// answer is "has value_type AND is integral" — a behaviour-meaningful
// composition rather than a constant.

// macro_trait_has_vt_and_integral
//   trait: detects `value_type` AND inherits from std::is_integral.
// True iff both the probe is well-formed AND the type is integral.
D_TRAIT_IS_DETECTED_FROM(macro_trait_has_vt_and_integral,
    typename _Type::value_type,
    std::is_integral)

// integer type WITH value_type -- delegated trait is_integral is true
//   (note: int doesn't actually have value_type — this exercises the
//   FAILURE path: probe fails so the answer is the primary `false`)
static_assert(macro_trait_has_vt_and_integral<int>::value == false,
              "D_TRAIT_IS_DETECTED_FROM: int has no value_type -> false");

// class WITH value_type but NOT integral itself
//   (probe succeeds, but the delegate is_integral<with_value_type_int>
//   is false, so the overall trait is false)
static_assert(macro_trait_has_vt_and_integral<with_value_type_int>::value == false,
              "D_TRAIT_IS_DETECTED_FROM: probe ok, delegate is_integral false -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(macro_trait_has_vt_and_integral_v<int> == false,
                  "D_TRAIT_IS_DETECTED_FROM _v alias (false)");
#endif


// =========================================================================
// V.   D_TRAIT_TYPE_ALIAS  (compile-time)
// =========================================================================
//   Emits a `TRAIT_NAME_t` alias that resolves to `TRAIT_NAME<_Type>::type`.
// To test it we need a trait with a `::type` member. The simplest is to
// use std::add_const directly — but D_TRAIT_TYPE_ALIAS expects a NAME,
// so we wrap it.

template<typename _Type>
struct macro_type_alias_subject
{
    using type = typename std::add_const<_Type>::type;
};

D_TRAIT_TYPE_ALIAS(macro_type_alias_subject)

static_assert(std::is_same<macro_type_alias_subject_t<int>, const int>::value,
              "D_TRAIT_TYPE_ALIAS: alias resolves to wrapped ::type");
static_assert(std::is_same<macro_type_alias_subject_t<float>, const float>::value,
              "D_TRAIT_TYPE_ALIAS: alias works for arbitrary types");


// =========================================================================
// VI.  D_TRAIT_VALUE_BOOL  (compile-time)
// =========================================================================
//   Emits a `TRAIT_NAME_v` `inline constexpr bool` alias for a unary
// trait. We exercise it directly on a hand-rolled trait — independent
// of D_TRAIT_IS_DETECTED.

template<typename _Type>
struct macro_value_bool_subject
    : std::integral_constant<bool, std::is_pointer<_Type>::value>
{};

D_TRAIT_VALUE_BOOL(macro_value_bool_subject)

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(macro_value_bool_subject_v<int*>    == true,
                  "D_TRAIT_VALUE_BOOL emits _v alias (true case)");
    static_assert(macro_value_bool_subject_v<int>     == false,
                  "D_TRAIT_VALUE_BOOL emits _v alias (false case)");
    static_assert(macro_value_bool_subject_v<int**>   == true,
                  "D_TRAIT_VALUE_BOOL emits _v alias (pointer-to-pointer)");
#endif


// =========================================================================
// VII. RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_macros_core(
    test_handler& _test_handler
)
{
    // ---- D_VOID_T ----
    record_assertion(_test_handler, 
        std::is_same<D_VOID_T<int>, void>::value,
        "D_VOID_T<int> -> void");
    record_assertion(_test_handler, 
        std::is_same<D_VOID_T<int, char>, void>::value,
        "D_VOID_T<int, char> -> void (variadic)");
    record_assertion(_test_handler, 
        std::is_same<D_VOID_T<>, void>::value,
        "D_VOID_T<> -> void (empty pack)");

    // ---- D_TRAIT_IS_DETECTED (single expr) ----
    record_assertion(_test_handler, 
        macro_trait_value_type<with_value_type_int>::value == true,
        "D_TRAIT_IS_DETECTED single: with value_type -> true");
    record_assertion(_test_handler, 
        macro_trait_value_type<without_value_type>::value == false,
        "D_TRAIT_IS_DETECTED single: without value_type -> false");
    record_assertion(_test_handler, 
        macro_trait_value_type<int>::value == false,
        "D_TRAIT_IS_DETECTED single: builtin int -> false");

    // ---- D_TRAIT_IS_DETECTED (multi expr) ----
    record_assertion(_test_handler, 
        macro_trait_value_type_and_data<with_type_and_data>::value == true,
        "D_TRAIT_IS_DETECTED multi: both probes succeed -> true");
    record_assertion(_test_handler, 
        macro_trait_value_type_and_data<with_type_only_no_data>::value == false,
        "D_TRAIT_IS_DETECTED multi: second probe fails -> false");
    record_assertion(_test_handler, 
        macro_trait_value_type_and_data<without_value_type>::value == false,
        "D_TRAIT_IS_DETECTED multi: first probe fails -> false");

    // ---- D_TRAIT_IS_DETECTED_AS ----
    record_assertion(_test_handler, 
        macro_trait_count_7<with_value_type_int>::value == 7,
        "D_TRAIT_IS_DETECTED_AS: success inherits supplied base (value=7)");
    record_assertion(_test_handler, 
        std::is_base_of<std::integral_constant<int, 7>,
                        macro_trait_count_7<with_value_type_int>>::value,
        "D_TRAIT_IS_DETECTED_AS: success base type matches INHERIT_EXPR");
    record_assertion(_test_handler, 
        macro_trait_count_7<without_value_type>::value == false,
        "D_TRAIT_IS_DETECTED_AS: failure ::value is false");
    record_assertion(_test_handler, 
        std::is_base_of<std::false_type,
                        macro_trait_count_7<without_value_type>>::value,
        "D_TRAIT_IS_DETECTED_AS: failure base is std::false_type");

    // ---- D_TRAIT_IS_DETECTED_FROM ----
    record_assertion(_test_handler, 
        macro_trait_has_vt_and_integral<int>::value == false,
        "D_TRAIT_IS_DETECTED_FROM: int (no value_type) -> false");
    record_assertion(_test_handler, 
        macro_trait_has_vt_and_integral<with_value_type_int>::value == false,
        "D_TRAIT_IS_DETECTED_FROM: probe ok but delegate false -> false");

    // ---- D_TRAIT_TYPE_ALIAS ----
    record_assertion(_test_handler, 
        std::is_same<macro_type_alias_subject_t<int>, const int>::value,
        "D_TRAIT_TYPE_ALIAS: alias resolves to ::type (int -> const int)");
    record_assertion(_test_handler, 
        std::is_same<macro_type_alias_subject_t<float>, const float>::value,
        "D_TRAIT_TYPE_ALIAS: alias resolves to ::type (float -> const float)");

    // ---- D_TRAIT_VALUE_BOOL ----
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    record_assertion(_test_handler, 
        macro_value_bool_subject_v<int*> == true,
        "D_TRAIT_VALUE_BOOL: _v alias (true case)");
    record_assertion(_test_handler, 
        macro_value_bool_subject_v<int> == false,
        "D_TRAIT_VALUE_BOOL: _v alias (false case)");
#endif

    return;
}


NS_END  // test
NS_END  // djinterp
