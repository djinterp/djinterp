/******************************************************************************
* djinterp [test]                                     test_object_concepts.hpp
*
* Test object concepts:
*   C++20 concepts layered over test_object_traits.hpp. These concepts provide
* readable constraints for test-object-like types without replacing the
* existing SFINAE trait surface.
*
*   The concepts mirror the structural and classification axes from
* test_object_traits.hpp:
*   - protocol detection
*   - test-object classification
*   - template-instantiation detection
*   - aggregate shorthand concepts
*
* path:      /inc/test/meta/test_object_concepts.hpp
* link(s):   TBA
* author(s): OpenAI ChatGPT                                   date: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_TEST_OBJECT_CONCEPTS_
#define DJINTERP_TEST_OBJECT_CONCEPTS_ 1

#ifndef __cplusplus
    #error "test_object_concepts.hpp requires C++ compilation"
#endif

#include "test_object_traits.hpp"


NS_DJINTERP
NS_TEST
NS_TRAITS

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

///////////////////////////////////////////////////////////////////////////////
///                I.   PROTOCOL CONCEPTS                                   ///
///////////////////////////////////////////////////////////////////////////////

// bool_convertible_test_type
//   concept: the type is explicitly or implicitly convertible to bool.
template<typename _Type>
concept bool_convertible_test_type =
    has_bool_conversion<_Type>::value;

// status_access_test_type
//   concept: the type exposes status().
template<typename _Type>
concept status_access_test_type =
    has_status_accessor<_Type>::value;

// status_typed_test_type
//   concept: the type exposes a status_type alias.
template<typename _Type>
concept status_typed_test_type =
    has_status_type<_Type>::value;

// options_aliased_test_type
//   concept: the type exposes an option_set_type alias.
template<typename _Type>
concept options_aliased_test_type =
    has_option_set_type<_Type>::value;

// named_protocol_test_type
//   concept: the type exposes name() returning const char*.
template<typename _Type>
concept named_protocol_test_type =
    has_name_accessor<_Type>::value;

// message_protocol_test_type
//   concept: the type exposes message() returning const char*.
template<typename _Type>
concept message_protocol_test_type =
    has_message_accessor<_Type>::value;

// event_handler_aware_test_type
//   concept: the type exposes event_handler().
template<typename _Type>
concept event_handler_aware_test_type =
    has_event_handler_accessor<_Type>::value;

// child_access_test_type
//   concept: the type exposes children().
template<typename _Type>
concept child_access_test_type =
    has_children_accessor<_Type>::value;

// evaluable_method_test_type
//   concept: the type exposes some form of evaluate().
template<typename _Type>
concept evaluable_method_test_type =
    has_evaluate_method<_Type>::value;

// result_access_test_type
//   concept: the type exposes result().
template<typename _Type>
concept result_access_test_type =
    has_result_accessor<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                II.  CLASSIFICATION CONCEPTS                             ///
///////////////////////////////////////////////////////////////////////////////

// test_evaluable_type
//   concept: minimum protocol -- bool conversion.
template<typename _Type>
concept test_evaluable_type =
    is_test_evaluable<_Type>::value;

// test_object_type
//   concept: full core test-object protocol -- bool conversion plus status().
template<typename _Type>
concept test_object_type =
    is_test_object<_Type>::value;

// leaf_test_object_type
//   concept: test object without children().
template<typename _Type>
concept leaf_test_object_type =
    is_leaf_test_object<_Type>::value;

// interior_test_object_type
//   concept: test object with children().
template<typename _Type>
concept interior_test_object_type =
    is_interior_test_object<_Type>::value;

// named_test_object_type
//   concept: test object with name().
template<typename _Type>
concept named_test_object_type =
    is_named_test_object<_Type>::value;

// event_aware_test_object_type
//   concept: test object with event-handler support.
template<typename _Type>
concept event_aware_test_object_type =
    is_event_aware_test_object<_Type>::value;

// options_aware_test_object_type
//   concept: test object built with the options system.
template<typename _Type>
concept options_aware_test_object_type =
    is_options_aware_test_object<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                III. TEMPLATE-DETECTION CONCEPTS                         ///
///////////////////////////////////////////////////////////////////////////////

// test_object_template_type
//   concept: the type is an instantiation of test_object<...>.
template<typename _Type>
concept test_object_template_type =
    is_test_object_template<_Type>::value;

// named_template_test_object_type
//   concept: template test object with naming support.
template<typename _Type>
concept named_template_test_object_type =
    test_object_template_type<_Type> &&
    named_test_object_type<_Type>;

// event_aware_template_test_object_type
//   concept: template test object with event-handler support.
template<typename _Type>
concept event_aware_template_test_object_type =
    test_object_template_type<_Type> &&
    event_aware_test_object_type<_Type>;

// options_aware_template_test_object_type
//   concept: template test object with option-set awareness.
template<typename _Type>
concept options_aware_template_test_object_type =
    test_object_template_type<_Type> &&
    options_aware_test_object_type<_Type>;


///////////////////////////////////////////////////////////////////////////////
///                IV.  AGGREGATE SHORTHAND CONCEPTS                        ///
///////////////////////////////////////////////////////////////////////////////

// classified_test_object_type
//   concept: shorthand for any type recognized as a test object by the
// aggregate classification struct.
template<typename _Type>
concept classified_test_object_type =
    test_object_class<_Type>::is_test_object;

// fully_described_test_object_type
//   concept: test object with core status, naming, evaluation, and result
// access.
template<typename _Type>
concept fully_described_test_object_type =
    test_object_type<_Type>         &&
    named_test_object_type<_Type>   &&
    evaluable_method_test_type<_Type> &&
    result_access_test_type<_Type>;

// interactive_test_object_type
//   concept: test object with event-handler awareness and evaluable behavior.
template<typename _Type>
concept interactive_test_object_type =
    test_object_type<_Type>              &&
    event_aware_test_object_type<_Type> &&
    evaluable_method_test_type<_Type>;

// composite_test_object_type
//   concept: interior test object with evaluation support.
template<typename _Type>
concept composite_test_object_type =
    interior_test_object_type<_Type> &&
    evaluable_method_test_type<_Type>;

#endif  // __cpp_concepts >= 201907L


NS_END  // traits
NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_OBJECT_CONCEPTS_
