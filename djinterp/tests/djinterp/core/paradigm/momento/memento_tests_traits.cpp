/*******************************************************************************
* djinterp [core]                                       memento_tests_traits.cpp
*
*
* path:      /tests/djinterp/core/paradigm/momento/memento_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : memento_tests_traits.cpp
//   Feature gates (section I) and the SFINAE capability-detection traits
// (section II): the internal has_*_method probes, the public is_*_capable
// classifiers, the memento_capability aggregate, and the _v aliases. Each
// probe is exercised against a fixture that satisfies it and one that does not.

// std
#include <type_traits>
// djinterp
#include "memento_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace di = ::djinterp::internal;


/*
tests_feature_gates
  Verifies the configuration macros track the detected language level.
  Tests the following:
  - D_MEMENTO_HAS_OPTIONAL mirrors the C++17 gate
  - D_MEMENTO_HAS_CONCEPTS / D_MEMENTO_HAS_SPAN mirror the C++20 gate
  - D_MEMENTO_DEFAULT_HISTORY_CAPACITY defaults to 64
*/
bool
tests_feature_gates()
{
    bool ok = true;

    ok = ok && (D_MEMENTO_HAS_OPTIONAL == (D_ENV_LANG_IS_CPP17_OR_HIGHER ? 1 : 0));
    ok = ok && (D_MEMENTO_HAS_CONCEPTS == (D_ENV_LANG_IS_CPP20_OR_HIGHER ? 1 : 0));
    ok = ok && (D_MEMENTO_HAS_SPAN     == (D_ENV_LANG_IS_CPP20_OR_HIGHER ? 1 : 0));
    ok = ok && (D_MEMENTO_DEFAULT_HISTORY_CAPACITY == 64);

    return ok;
}

/*
tests_has_save_state_method
  Verifies internal::has_save_state_method.
  Tests the following:
  - true for a type exposing save_state() (sr_state)
  - false for a type without it (plain_state)
*/
bool
tests_has_save_state_method()
{
    static_assert( di::has_save_state_method<sr_state>::value,    "present");
    static_assert(!di::has_save_state_method<plain_state>::value, "absent");

    return ( di::has_save_state_method<sr_state>::value &&
            !di::has_save_state_method<plain_state>::value);
}

/*
tests_has_restore_state_method
  Verifies internal::has_restore_state_method.
  Tests the following:
  - true for a type accepting restore_state(save_state()) (sr_state)
  - false for a type without it (plain_state)
*/
bool
tests_has_restore_state_method()
{
    static_assert( di::has_restore_state_method<sr_state>::value,    "present");
    static_assert(!di::has_restore_state_method<plain_state>::value, "absent");

    return ( di::has_restore_state_method<sr_state>::value &&
            !di::has_restore_state_method<plain_state>::value);
}

/*
tests_has_clone_method
  Verifies internal::has_clone_method.
  Tests the following:
  - true for a type exposing clone() (clonable_state)
  - false for a type without it (plain_state)
*/
bool
tests_has_clone_method()
{
    return ( di::has_clone_method<clonable_state>::value &&
            !di::has_clone_method<plain_state>::value);
}

/*
tests_has_serialize_method
  Verifies internal::has_serialize_method.
  Tests the following:
  - true for a type exposing serialize() (serializable_state)
  - false for a type without it (plain_state)
*/
bool
tests_has_serialize_method()
{
    return ( di::has_serialize_method<serializable_state>::value &&
            !di::has_serialize_method<plain_state>::value);
}

/*
tests_has_deserialize_method
  Verifies internal::has_deserialize_method.
  Tests the following:
  - true for a type accepting deserialize(serialize()) (serializable_state)
  - false for a type without it (plain_state)
*/
bool
tests_has_deserialize_method()
{
    return ( di::has_deserialize_method<serializable_state>::value &&
            !di::has_deserialize_method<plain_state>::value);
}

/*
tests_has_diff_method
  Verifies internal::has_diff_method.
  Tests the following:
  - true for a type exposing diff(other) (diffable_state)
  - false for a type without it (plain_state)
*/
bool
tests_has_diff_method()
{
    return ( di::has_diff_method<diffable_state>::value &&
            !di::has_diff_method<plain_state>::value);
}

/*
tests_has_apply_diff_method
  Verifies internal::has_apply_diff_method.
  Tests the following:
  - true for a type accepting apply_diff(diff()) (diffable_state)
  - false for a type without it (plain_state)
*/
bool
tests_has_apply_diff_method()
{
    return ( di::has_apply_diff_method<diffable_state>::value &&
            !di::has_apply_diff_method<plain_state>::value);
}

/*
tests_has_equality_operator
  Verifies internal::has_equality_operator.
  Tests the following:
  - true for an equality-comparable type (eq_state)
  - false for a type without operator== (plain_state)
*/
bool
tests_has_equality_operator()
{
    return ( di::has_equality_operator<eq_state>::value &&
            !di::has_equality_operator<plain_state>::value);
}

/*
tests_is_memento_capable
  Verifies is_memento_capable (save_state AND restore_state).
  Tests the following:
  - true for sr_state (both methods)
  - false for plain_state (neither) and clonable_state (clone only)
*/
bool
tests_is_memento_capable()
{
    static_assert( is_memento_capable<sr_state>::value,    "capable");
    static_assert(!is_memento_capable<plain_state>::value, "not capable");

    bool ok = true;

    ok = ok && ( is_memento_capable<sr_state>::value);
    ok = ok && (!is_memento_capable<plain_state>::value);
    ok = ok && (!is_memento_capable<clonable_state>::value);

    return ok;
}

/*
tests_is_copyable_memento_capable
  Verifies is_copyable_memento_capable (copy-constructibility).
  Tests the following:
  - true for a copyable type (plain_state)
  - false for a non-copyable type
*/
bool
tests_is_copyable_memento_capable()
{
    struct noncopy
    {
        noncopy() {}
        noncopy(const noncopy&) = delete;
    };

    bool ok = true;

    ok = ok && ( is_copyable_memento_capable<plain_state>::value);
    ok = ok && (!is_copyable_memento_capable<noncopy>::value);

    return ok;
}

/*
tests_is_clonable_memento_capable
  Verifies is_clonable_memento_capable (clone()).
  Tests the following:
  - true for clonable_state
  - false for plain_state
*/
bool
tests_is_clonable_memento_capable()
{
    return ( is_clonable_memento_capable<clonable_state>::value &&
            !is_clonable_memento_capable<plain_state>::value);
}

/*
tests_is_serializable_memento_capable
  Verifies is_serializable_memento_capable (serialize AND deserialize).
  Tests the following:
  - true for serializable_state
  - false for plain_state
*/
bool
tests_is_serializable_memento_capable()
{
    return ( is_serializable_memento_capable<serializable_state>::value &&
            !is_serializable_memento_capable<plain_state>::value);
}

/*
tests_is_diff_memento_capable
  Verifies is_diff_memento_capable (diff AND apply_diff).
  Tests the following:
  - true for diffable_state
  - false for plain_state
*/
bool
tests_is_diff_memento_capable()
{
    return ( is_diff_memento_capable<diffable_state>::value &&
            !is_diff_memento_capable<plain_state>::value);
}

/*
tests_memento_capability_aggregate
  Verifies the memento_capability aggregate across a fully-capable type and a
  copy-only type.
  Tests the following:
  - full_state: every capability flag true, is_snapshottable true
  - plain_state: only has_copy / is_snapshottable true, the rest false
*/
bool
tests_memento_capability_aggregate()
{
    using full  = memento_capability<full_state>;
    using plain = memento_capability<plain_state>;

    bool ok = true;

    ok = ok && ( full::has_save_restore);
    ok = ok && ( full::has_copy);
    ok = ok && ( full::has_clone);
    ok = ok && ( full::has_serialization);
    ok = ok && ( full::has_diff);
    ok = ok && ( full::has_equality);
    ok = ok && ( full::is_snapshottable);

    ok = ok && (!plain::has_save_restore);
    ok = ok && ( plain::has_copy);
    ok = ok && (!plain::has_clone);
    ok = ok && (!plain::has_serialization);
    ok = ok && (!plain::has_diff);
    ok = ok && (!plain::has_equality);
    ok = ok && ( plain::is_snapshottable);   // copyable => snapshottable

    return ok;
}

/*
tests_trait_v_forms
  Verifies the _v variable-template aliases mirror the class-template ::value.
  Tests the following:
  - each of the five capability _v aliases equals its trait's ::value (checked
    only where variable templates are available, C++14+)
*/
bool
tests_trait_v_forms()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    bool ok = true;

    ok = ok && (is_memento_capable_v<sr_state> ==
                is_memento_capable<sr_state>::value);
    ok = ok && (is_copyable_memento_capable_v<plain_state> ==
                is_copyable_memento_capable<plain_state>::value);
    ok = ok && (is_clonable_memento_capable_v<clonable_state> ==
                is_clonable_memento_capable<clonable_state>::value);
    ok = ok && (is_serializable_memento_capable_v<serializable_state> ==
                is_serializable_memento_capable<serializable_state>::value);
    ok = ok && (is_diff_memento_capable_v<diffable_state> ==
                is_diff_memento_capable<diffable_state>::value);

    return ok;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
