/******************************************************************************
* djinterp [test]                                          test_concepts.hpp
*
*   DTest framework compile-time constraints for test_object template
* parameters. Provides both C++20 concepts and pre-C++20 SFINAE traits
* for constraining:
*
*   - _Type:       the evaluation result type (must convert to bool)
*   - _StatusType: the status classification type (must be a scoped enum)
*   - _Options:    validity of individual option tags
*
*   PORTABILITY:
*   On C++20 and later, proper `concept` definitions are provided and
* used directly in requires-clauses. On C++14/C++17, equivalent SFINAE
* trait structs are provided under the same names (prefixed with `is_`).
* On C++11, only the trait structs are available; relaxed constexpr is
* not available.
*
*   The portable macro D_TEST_REQUIRES(_Constraint) expands to a
* requires-clause on C++20+ and to an std::enable_if SFINAE guard on
* C++14/C++17, enabling a single call-site to work across standards.
*
*   CONSTRAINTS DEFINED:
*     is_test_result_type<_Type>       - convertible to bool
*     is_test_status_type<_Type>       - scoped enum
*     is_test_event_handler<_Type>     - callable with const test_event&
*     is_valid_test_option<_Type>      - recognized option tag type
*
*   C++20 CONCEPTS (when available):
*     test_result_type<_Type>
*     test_status_type<_Type>
*     test_event_handler_type<_Type>
*     valid_test_option<_Type>
*
*
* path:      /inc/test/test_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.05
******************************************************************************/

#ifndef DJINTERP_TEST_CONCEPTS_
#define DJINTERP_TEST_CONCEPTS_ 1

// std
#include <type_traits>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_common.hpp"
#include "./test_options.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   SFINAE CONSTRAINT TRAITS                            ///
///////////////////////////////////////////////////////////////////////////////
// Always available from C++11 onward. Each trait yields
// std::true_type or std::false_type.

// ================================================================
//  is_test_result_type
// ================================================================

// is_test_result_type
//   trait: true if _Type is convertible to bool, making it
// suitable as the evaluation result type of a test_object.
template<typename _Type,
         typename = void>
struct is_test_result_type : std::false_type
{};

template<typename _Type>
struct is_test_result_type<_Type, void_t<
    decltype(static_cast<bool>(std::declval<const _Type&>()))
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_test_result_type_v
    //   variable template: value of is_test_result_type<_Type>.
    template<typename _Type>
    constexpr bool is_test_result_type_v =
        is_test_result_type<_Type>::value;
#endif


// ================================================================
//  is_test_status_type
// ================================================================

// is_test_status_type
//   trait: true if _StatusType is a scoped enumeration
// suitable for use as a test object's status classifier.
// Requires the type to be an enum; unscoped enums are
// permitted but scoped enums are preferred.
template<typename _StatusType>
struct is_test_status_type
    : std::is_enum<_StatusType>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_test_status_type_v
    //   variable template: value of is_test_status_type<_StatusType>.
    template<typename _StatusType>
    constexpr bool is_test_status_type_v =
        is_test_status_type<_StatusType>::value;
#endif


// ================================================================
//  is_test_event_handler
// ================================================================

NS_INTERNAL

    // event_handler_callable_check
    //   trait: detects if _Handler can be invoked with a
    // const test_event&.
    template<typename _Handler,
             typename = void>
    struct event_handler_callable_check : std::false_type
    {};

    template<typename _Handler>
    struct event_handler_callable_check<_Handler, void_t<
        decltype(std::declval<_Handler&>()(
            std::declval<const test_event&>()))
    >> : std::true_type
    {};

NS_END  // internal

// is_test_event_handler
//   trait: true if _Handler is callable with a const
// test_event& argument. Accepts function pointers,
// functors, and lambdas.
template<typename _Handler>
struct is_test_event_handler
    : internal::event_handler_callable_check<_Handler>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_test_event_handler_v
    //   variable template: value of is_test_event_handler<_Handler>.
    template<typename _Handler>
    constexpr bool is_test_event_handler_v =
        is_test_event_handler<_Handler>::value;
#endif


// ================================================================
//  is_valid_test_option
// ================================================================

NS_INTERNAL

    // is_recognized_option
    //   trait: true if _Opt is one of the recognized option
    // tag types.
    template<typename _Opt>
    struct is_recognized_option
    {
        static constexpr bool value =
            ( std::is_same<_Opt, test_opt_named>::value     ||
              std::is_same<_Opt, test_opt_message>::value   ||
              std::is_same<_Opt, test_opt_skippable>::value ||
              is_instance_of<test_opt_events, _Opt>::value  ||
              is_instance_of<test_opt_children, _Opt>::value );
    };

    // all_valid_options
    //   trait: true if every type in the pack is a recognized
    // option tag. Base case (empty pack) is true.
    template<typename... _Options>
    struct all_valid_options : std::true_type
    {};

    template<typename _Head,
             typename... _Tail>
    struct all_valid_options<_Head, _Tail...>
        : std::conditional<
              is_recognized_option<_Head>::value,
              all_valid_options<_Tail...>,
              std::false_type
          >::type
    {};

NS_END  // internal

// is_valid_test_option
//   trait: true if _Opt is a recognized DTest option tag.
template<typename _Opt>
struct is_valid_test_option
    : internal::is_recognized_option<_Opt>
{};

// are_valid_test_options
//   trait: true if every type in the pack is a recognized
// DTest option tag.
template<typename... _Options>
struct are_valid_test_options
    : internal::all_valid_options<_Options...>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_valid_test_option_v
    //   variable template: value of is_valid_test_option<_Opt>.
    template<typename _Opt>
    constexpr bool is_valid_test_option_v =
        is_valid_test_option<_Opt>::value;

    // are_valid_test_options_v
    //   variable template: value of are_valid_test_options<...>.
    template<typename... _Options>
    constexpr bool are_valid_test_options_v =
        are_valid_test_options<_Options...>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///                II.  C++20 CONCEPTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // test_result_type
    //   concept: constrains a type to be convertible to bool,
    // suitable as the evaluation result of a test_object.
    template<typename _Type>
    concept test_result_type = is_test_result_type<_Type>::value;

    // test_status_type
    //   concept: constrains a type to be an enumeration suitable
    // for test object status classification.
    template<typename _StatusType>
    concept test_status_type = is_test_status_type<_StatusType>::value;

    // test_event_handler_type
    //   concept: constrains a type to be callable with a const
    // test_event& argument.
    template<typename _Handler>
    concept test_event_handler_type =
        is_test_event_handler<_Handler>::value;

    // valid_test_option
    //   concept: constrains a type to be a recognized DTest
    // option tag.
    template<typename _Opt>
    concept valid_test_option =
        is_valid_test_option<_Opt>::value;

    // valid_test_options
    //   concept: constrains a parameter pack to contain only
    // recognized DTest option tags.
    template<typename... _Options>
    concept valid_test_options =
        are_valid_test_options<_Options...>::value;

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///                III. PORTABLE CONSTRAINT MACROS                          ///
///////////////////////////////////////////////////////////////////////////////

// D_TEST_REQUIRES
//   macro: portable requires-clause. On C++20+, expands to
// `requires (_Constraint)`. On pre-C++20, expands to nothing;
// use the corresponding SFINAE traits directly in enable_if.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #define D_TEST_REQUIRES(...)    requires (__VA_ARGS__)
#else
    #define D_TEST_REQUIRES(...)
#endif

// D_TEST_CONCEPT_OR_SFINAE
//   macro: expands to a concept constraint on C++20+ or
// a static_assert on pre-C++20 for use inside class bodies.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #define D_TEST_CONCEPT_OR_SFINAE(_concept, _type, _msg)
#else
    #define D_TEST_CONCEPT_OR_SFINAE(_concept, _type, _msg)     \
        static_assert(_concept<_type>::value, _msg)
#endif


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_CONCEPTS_
