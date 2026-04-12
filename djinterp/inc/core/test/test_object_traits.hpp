/******************************************************************************
* djinterp [test]                                       test_object_traits.hpp
*
*   DTest framework test object compile-time classification traits.
* Provides two tiers of detection:
*
*   1. STRUCTURAL DETECTION — purely structural SFINAE that classifies
*      any type exposing the test object protocol. No tag types or base
*      classes required. Detects: boolean conversion, status accessor,
*      naming, event handler, children, evaluate method.
*
*   2. TEMPLATE DETECTION — identifies types that are instantiations of
*      the test_object class template and extracts their template
*      parameters (_Type, _StatusType, option pack). Enables compile-time
*      queries like "is this a test_object with event support?"
*
*   All traits operate on the `clean_t` (cv-ref stripped) form of the
* type and produce `static constexpr bool` values. `_v` variable
* templates are provided for every public trait (C++14+).
*
*   TEST OBJECT PROTOCOL (detected members):
*     Boolean conversion:    operator bool() const
*     Status:                status()  -> any enum type
*     Naming:                name()    -> const char*
*     Event handler:         set_event_handler(handler)
*                            event_handler() -> handler
*     Children (interior):   children() -> container reference
*                         or begin() / end()
*     Evaluate:              evaluate(result) or evaluate()
*     Status type alias:     status_type member typedef
*     Option set alias:      option_set_type member typedef
*
*
* path:      /inc/test/meta/test_object_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.05
******************************************************************************/

#ifndef DJINTERP_TEST_OBJECT_TRAITS_
#define DJINTERP_TEST_OBJECT_TRAITS_ 1

#include <type_traits>
#include <utility>
#include "../djinterp.hpp"
#include "../type_traits.hpp"
#include "./test_common.hpp"
#include "./test_options.hpp"


NS_DJINTERP
NS_TEST
NS_TRAITS


///////////////////////////////////////////////////////////////////////////////
///                I.   STRUCTURAL PROTOCOL DETECTION                       ///
///////////////////////////////////////////////////////////////////////////////

// ================================================================
//  has_bool_conversion
// ================================================================

// has_bool_conversion
//   trait: detects whether a type is convertible to bool.
template<typename _Type,
         typename = void>
struct has_bool_conversion : std::false_type
{
};

template<typename _Type>
struct has_bool_conversion<_Type, D_VOID_T<
    decltype(static_cast<bool>(std::declval<const _Type&>()))
>> : std::true_type
{
};


// ================================================================
//  has_status_accessor
// ================================================================

// has_status_accessor
//   trait: detects whether a type has a status() const member
// function. Does not constrain the return type — any enum
// status type is accepted.
template<typename _Type,
         typename = void>
struct has_status_accessor : std::false_type
{
};

template<typename _Type>
struct has_status_accessor<_Type, D_VOID_T<
    decltype(std::declval<const _Type&>().status())
>> : std::true_type
{
};


// ================================================================
//  has_status_type
// ================================================================

// has_status_type
//   trait: detects whether a type exposes a status_type
// member alias.
template<typename _Type,
         typename = void>
struct has_status_type : std::false_type
{
};

template<typename _Type>
struct has_status_type<_Type, D_VOID_T<
    typename _Type::status_type
>> : std::true_type
{
};


// ================================================================
//  has_option_set_type
// ================================================================

// has_option_set_type
//   trait: detects whether a type exposes an option_set_type
// member alias, indicating it was built with the options system.
template<typename _Type,
         typename = void>
struct has_option_set_type : std::false_type
{
};

template<typename _Type>
struct has_option_set_type<_Type, D_VOID_T<
    typename _Type::option_set_type
>> : std::true_type
{
};


// ================================================================
//  has_name_accessor
// ================================================================

// has_name_accessor
//   trait: detects whether a type has a name() const member
// function returning const char*.
template<typename _Type,
         typename = void>
struct has_name_accessor : std::false_type
{
};

template<typename _Type>
struct has_name_accessor<_Type, D_VOID_T<
    decltype(std::declval<const _Type&>().name())
>> : std::is_same<
    decltype(std::declval<const _Type&>().name()),
    const char*
>
{
};


// ================================================================
//  has_message_accessor
// ================================================================

// has_message_accessor
//   trait: detects whether a type has a message() const member
// function returning const char*.
template<typename _Type,
         typename = void>
struct has_message_accessor : std::false_type
{
};

template<typename _Type>
struct has_message_accessor<_Type, D_VOID_T<
    decltype(std::declval<const _Type&>().message())
>> : std::is_same<
    decltype(std::declval<const _Type&>().message()),
    const char*
>
{
};


// ================================================================
//  has_event_handler_accessor
// ================================================================

// has_event_handler_accessor
//   trait: detects whether a type has an event_handler() const
// member function.
template<typename _Type,
         typename = void>
struct has_event_handler_accessor : std::false_type
{
};

template<typename _Type>
struct has_event_handler_accessor<_Type, D_VOID_T<
    decltype(std::declval<const _Type&>().event_handler())
>> : std::true_type
{
};


// ================================================================
//  has_children_accessor
// ================================================================

// has_children_accessor
//   trait: detects whether a type has a children() const member
// function, indicating an interior node.
template<typename _Type,
         typename = void>
struct has_children_accessor : std::false_type
{
};

template<typename _Type>
struct has_children_accessor<_Type, D_VOID_T<
    decltype(std::declval<const _Type&>().children())
>> : std::true_type
{
};


// ================================================================
//  has_evaluate_method
// ================================================================

NS_INTERNAL

    // has_evaluate_unary_helper
    //   trait: detects evaluate(value) overload (leaf-style).
    template<typename _Type,
             typename = void>
    struct has_evaluate_unary_helper : std::false_type
    {
    };

    template<typename _Type>
    struct has_evaluate_unary_helper<_Type, D_VOID_T<
        decltype(std::declval<_Type&>().evaluate(
            std::declval<typename _Type::value_type>()))
    >> : std::true_type
    {
    };

    // has_evaluate_void_helper
    //   trait: detects evaluate() overload (group-style).
    template<typename _Type,
             typename = void>
    struct has_evaluate_void_helper : std::false_type
    {
    };

    template<typename _Type>
    struct has_evaluate_void_helper<_Type, D_VOID_T<
        decltype(std::declval<_Type&>().evaluate())
    >> : std::true_type
    {
    };

NS_END  // internal

// has_evaluate_method
//   trait: detects whether a type has any form of evaluate()
// member function.
template<typename _Type>
struct has_evaluate_method
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( internal::has_evaluate_unary_helper<clean_type>::value ||
          internal::has_evaluate_void_helper<clean_type>::value );
};


// ================================================================
//  has_result_accessor
// ================================================================

// has_result_accessor
//   trait: detects whether a type has a result() const member
// function.
template<typename _Type,
         typename = void>
struct has_result_accessor : std::false_type
{
};

template<typename _Type>
struct has_result_accessor<_Type, D_VOID_T<
    decltype(std::declval<const _Type&>().result())
>> : std::true_type
{
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST OBJECT CLASSIFICATION                          ///
///////////////////////////////////////////////////////////////////////////////

// is_test_evaluable
//   trait: minimum protocol — boolean conversion.
template<typename _Type>
struct is_test_evaluable
{
    static constexpr bool value =
        has_bool_conversion<clean_t<_Type>>::value;
};

// is_test_object
//   trait: full test object protocol — boolean conversion
// plus status accessor.
template<typename _Type>
struct is_test_object
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_bool_conversion<clean_type>::value &&
          has_status_accessor<clean_type>::value );
};

// is_leaf_test_object
//   trait: test object without children.
template<typename _Type>
struct is_leaf_test_object
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_test_object<clean_type>::value          &&
          !has_children_accessor<clean_type>::value );
};

// is_interior_test_object
//   trait: test object with children.
template<typename _Type>
struct is_interior_test_object
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_test_object<clean_type>::value         &&
          has_children_accessor<clean_type>::value );
};

// is_named_test_object
//   trait: test object with name accessor.
template<typename _Type>
struct is_named_test_object
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_test_object<clean_type>::value     &&
          has_name_accessor<clean_type>::value );
};

// is_event_aware_test_object
//   trait: test object with event handler support.
template<typename _Type>
struct is_event_aware_test_object
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_test_object<clean_type>::value                &&
          has_event_handler_accessor<clean_type>::value );
};

// is_options_aware_test_object
//   trait: test object built with the options system,
// exposing an option_set_type alias.
template<typename _Type>
struct is_options_aware_test_object
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_test_object<clean_type>::value          &&
          has_option_set_type<clean_type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                III. TEMPLATE DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // is_test_object_instantiation
    //   trait: detects whether a type is an instantiation of the
    // test_object class template.
    template<typename _Type>
    struct is_test_object_instantiation : std::false_type
    {
    };

    template<typename    _T,
             typename    _S,
             typename... _Opts>
    struct is_test_object_instantiation<
        test_object<_T, _S, _Opts...>> : std::true_type
    {
    };

NS_END  // internal

// is_test_object_template
//   trait: true if the type is an instantiation of test_object.
template<typename _Type>
struct is_test_object_template
    : internal::is_test_object_instantiation<clean_t<_Type>>
{
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  VARIABLE TEMPLATES                                  ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    constexpr bool has_bool_conversion_v =
        has_bool_conversion<_Type>::value;

    template<typename _Type>
    constexpr bool has_status_accessor_v =
        has_status_accessor<_Type>::value;

    template<typename _Type>
    constexpr bool has_status_type_v =
        has_status_type<_Type>::value;

    template<typename _Type>
    constexpr bool has_option_set_type_v =
        has_option_set_type<_Type>::value;

    template<typename _Type>
    constexpr bool has_name_accessor_v =
        has_name_accessor<_Type>::value;

    template<typename _Type>
    constexpr bool has_message_accessor_v =
        has_message_accessor<_Type>::value;

    template<typename _Type>
    constexpr bool has_event_handler_accessor_v =
        has_event_handler_accessor<_Type>::value;

    template<typename _Type>
    constexpr bool has_children_accessor_v =
        has_children_accessor<_Type>::value;

    template<typename _Type>
    constexpr bool has_evaluate_method_v =
        has_evaluate_method<_Type>::value;

    template<typename _Type>
    constexpr bool has_result_accessor_v =
        has_result_accessor<_Type>::value;

    template<typename _Type>
    constexpr bool is_test_evaluable_v =
        is_test_evaluable<_Type>::value;

    template<typename _Type>
    constexpr bool is_test_object_v =
        is_test_object<_Type>::value;

    template<typename _Type>
    constexpr bool is_leaf_test_object_v =
        is_leaf_test_object<_Type>::value;

    template<typename _Type>
    constexpr bool is_interior_test_object_v =
        is_interior_test_object<_Type>::value;

    template<typename _Type>
    constexpr bool is_named_test_object_v =
        is_named_test_object<_Type>::value;

    template<typename _Type>
    constexpr bool is_event_aware_test_object_v =
        is_event_aware_test_object<_Type>::value;

    template<typename _Type>
    constexpr bool is_options_aware_test_object_v =
        is_options_aware_test_object<_Type>::value;

    template<typename _Type>
    constexpr bool is_test_object_template_v =
        is_test_object_template<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


///////////////////////////////////////////////////////////////////////////////
///                V.   COMBINED CLASSIFICATION                             ///
///////////////////////////////////////////////////////////////////////////////

// test_object_class
//   struct: complete classification of a test object type.
// All classification is compile-time using static constexpr
// bool members.
template<typename _Type>
struct test_object_class
{
    // protocol detection
    static constexpr bool has_bool_conversion  = has_bool_conversion<_Type>::value;
    static constexpr bool has_status           = has_status_accessor<_Type>::value;
    static constexpr bool has_status_type      = traits::has_status_type<_Type>::value;
    static constexpr bool has_option_set       = has_option_set_type<_Type>::value;
    static constexpr bool has_name             = has_name_accessor<_Type>::value;
    static constexpr bool has_message          = has_message_accessor<_Type>::value;
    static constexpr bool has_event_handler    = has_event_handler_accessor<_Type>::value;
    static constexpr bool has_children         = has_children_accessor<_Type>::value;
    static constexpr bool has_evaluate         = has_evaluate_method<_Type>::value;
    static constexpr bool has_result           = has_result_accessor<_Type>::value;

    // classification
    static constexpr bool is_evaluable         = is_test_evaluable<_Type>::value;
    static constexpr bool is_test_object       = traits::is_test_object<_Type>::value;
    static constexpr bool is_leaf              = is_leaf_test_object<_Type>::value;
    static constexpr bool is_interior          = is_interior_test_object<_Type>::value;
    static constexpr bool is_named             = is_named_test_object<_Type>::value;
    static constexpr bool is_event_aware       = is_event_aware_test_object<_Type>::value;
    static constexpr bool is_options_aware     = is_options_aware_test_object<_Type>::value;
    static constexpr bool is_template          = is_test_object_template<_Type>::value;
};


NS_END  // traits
NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_OBJECT_TRAITS_
