/******************************************************************************
* djinterp [test]                                             test_options.hpp
*
*   Test option set: typed configuration for test objects in the DTest
* framework.  Each test_object in a test_tree may hold a non-owning
* pointer to a dtest_option_set.  Interior nodes may override individual
* options; descendants inherit their ancestor's values for keys they do
* not override.
*
*   DESIGN:
*   dtest_option_set is an alias for option_set<DTestOption, djinterp::any>,
* reusing the keyed-collection infrastructure from option_set.hpp.
* Heterogeneous value storage is achieved through djinterp::any — each
* DTestOption key maps to a different runtime type (std::size_t, bool,
* function pointers, etc.) stored uniformly.
*
*   Typed access is provided by free-function helpers that forward to
* any::get<T>() and any::holds<T>(), keeping the option_set itself
* type-agnostic.
*
*   CASCADING:
*   When a test_tree node holds an option set pointer, the tree's
* evaluation logic resolves each key by walking up the ancestor chain.
* Override permission is controlled externally by an
* option_override_policy (see option_override_policy.hpp).  This
* header does not impose a policy — it only defines the option
* enumeration, the option set type, and the default values.
*
*   PORTABILITY:
*   C++11 minimum.  Uses env.h for version detection and djinterp.hpp
* for namespace macros and constexpr support.
*
*
* TABLE OF CONTENTS
* =================
* I.    OPTION ENUMERATION
* II.   ENUM_INFO SPECIALIZATION
* III.  DTEST OPTION SET
* IV.   TYPED ACCESS HELPERS
* V.    DTEST DEFAULTS
* VI.   CONFIGURABILITY DETECTION
*
*
* path:      /inc/djinterp/test/test_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.12
******************************************************************************/

#ifndef DJINTERP_TEST_OPTIONS_
#define DJINTERP_TEST_OPTIONS_ 1

#include <cstddef>
#include <type_traits>
// restd
#include "../restd/any/any.hpp"
// djinterp
#include "../core/djinterp.hpp"
#include "../core/options/option_pair.hpp"
#include "../core/options/option_set.hpp"
#include "./test_common.hpp"


NS_DJINTERP
NS_TEST

using djinterp::restd::any;

///////////////////////////////////////////////////////////////////////////////
///                I.   OPTION ENUMERATION                                  ///
///////////////////////////////////////////////////////////////////////////////

// DTestOption
//   enum: identifies a configurable parameter for test
// objects.  Each enumerator maps to a value of a specific
// runtime type stored in a dtest_option_set via
// djinterp::any.
//
//   max_failures  — std::size_t, stop after N failures (0 = no limit)
//   max_successes — std::size_t, stop after N successes (0 = no limit)
//   handler       — fn_test_event_handler, event callback (nullptr = none)
//   verbose       — bool, emit detailed output
//   metadata      — djinterp::any, user-defined payload
enum class DTestOption
{
    max_failures  = 0,
    max_successes = 1,
    handler       = 2,
    verbose       = 3,
    metadata      = 4
};

// D_TEST_OPTION_COUNT
//   constant: total number of DTestOption enumerators.
D_STATIC_CONSTEXPR std::size_t D_TEST_OPTION_COUNT = 5;


///////////////////////////////////////////////////////////////////////////////
///                III. DTEST OPTION SET                                     ///
///////////////////////////////////////////////////////////////////////////////

// dtest_option_set
//   type: the default DTest option set.  Maps DTestOption
// enumerators to heterogeneous values via djinterp::any,
// backed by the generic option_set container.
//
// Usage:
//   dtest_option_set opts;
//   opts.insert(DTestOption::verbose, djinterp::any(true));
//   bool v = opts.at(DTestOption::verbose).template get<bool>();
using dtest_option_set = option_set<DTestOption, any>;


///////////////////////////////////////////////////////////////////////////////
///                IV.  TYPED ACCESS HELPERS                                 ///
///////////////////////////////////////////////////////////////////////////////

// dtest_option_get
//   function: retrieves the value for _key from _opts,
// cast to _Type.  Undefined behaviour if the key is absent
// or the stored type does not match _Type.
template<typename _Type>
D_INLINE _Type
dtest_option_get(
    const dtest_option_set& _opts,
    DTestOption             _key
)
{
    return _opts.at(_key).template get<_Type>();
}

// dtest_option_get_or
//   function: retrieves the value for _key if present and
// holding the correct type, otherwise returns _fallback.
template<typename _Type>
D_INLINE _Type
dtest_option_get_or(
    const dtest_option_set& _opts,
    DTestOption             _key,
    const _Type&            _fallback
)
{
    auto it = _opts.find(_key);

    if (it == _opts.end())
    {
        return _fallback;
    }

    // check that the stored any holds _Type
    if (!it->value.template holds<_Type>())
    {
        return _fallback;
    }

    return it->value.template get<_Type>();
}

// dtest_option_set_value
//   function: sets the value for _key in _opts.  Inserts
// a new entry if the key is absent, or overwrites the
// existing value.
template<typename _Type>
D_INLINE void
dtest_option_set_value(
    dtest_option_set& _opts,
    DTestOption       _key,
    _Type&&           _value
)
{
    _opts.insert_or_assign(_key, any(static_cast<_Type&&>(_value)));

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                V.   DTEST DEFAULTS                                       ///
///////////////////////////////////////////////////////////////////////////////

// make_default_test_options
//   function: constructs a dtest_option_set pre-populated
// with default values for each DTestOption key.
//
// Defaults:
//   max_failures  = 0          (no limit)
//   max_successes = 0          (no limit)
//   handler       = nullptr    (no event callback)
//   verbose       = false
//   metadata      = any()      (empty)
D_INLINE dtest_option_set
make_default_test_options()
{
    dtest_option_set opts;

    opts.insert(DTestOption::max_failures, any(std::size_t{0}));

    opts.insert(DTestOption::max_successes, any(std::size_t{0}));

    opts.insert(DTestOption::handler,
                static_cast<fn_test_event_handler>(nullptr));

    opts.insert(DTestOption::verbose, any(false));

    opts.insert(DTestOption::metadata, any());

    return opts;
}


///////////////////////////////////////////////////////////////////////////////
///                VI.  CONFIGURABILITY DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////

NS_TRAITS

NS_INTERNAL

    // detect_option_set_type
    //   helper: extracts ::option_set_type from _Type.
    template<typename _Type>
    using detect_option_set_type =
        typename _Type::option_set_type;

NS_END  // internal

// is_test_option_configurable
//   trait: true if _Type exposes an option_set_type alias,
// indicating that it participates in the test option system.
template<typename _Type,
         typename = void>
struct is_test_option_configurable : std::false_type
{};

template<typename _Type>
struct is_test_option_configurable<_Type, void_t<
    internal::detect_option_set_type<_Type>
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_test_option_configurable_v =
        is_test_option_configurable<_Type>::value;
#endif

NS_END  // traits


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_OPTIONS_
