/*******************************************************************************
* djinterp [testing]               iterable_container_traits_tests_aggregate.cpp
*
*   Section V - iterable_container_class, the assembled snapshot: the four detection
* signals, the base classification, and the mode sub-axis (both raw probes, the
* resolved mode, and its name), checked member-for-member across a representative of
* every shape.  The `_v` companions are verified here too, gated exactly as the
* D_TYPE_TRAIT_VALUE_BOOL engine emits them.
*
*   The spread deliberately includes the dissociations: mutable_only_iter has
* provides_mutable true with provides_const FALSE (and no iterator aliases at all);
* no_value_type_iter is iterable yet NOT a non-iterable container and reads
* const_only; and begin_only shows every layer disagreeing at once - has_begin true
* with has_end false, is_iterable false but is_non_iterable TRUE, and
* provides_mutable true while the resolved mode is none.
*
*
* path:      /tests/djinterp/core/container/traits/iterable_container_traits_tests_aggregate.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "iterable_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_iterable_container_class
//   the assembled snapshot, one representative per shape.
bool
tests_iterable_container_class()
{
    bool _ic_ok = true;

    // -- std::vector<int>: fully iterable, settable --
    {
        using C = iterable_container_class<std::vector<int>>;
        D_IC_CHECK( (C::has_begin && C::has_end) );
        D_IC_CHECK( (C::has_iter_alias && C::has_const_iter_alias) );
        D_IC_CHECK( (C::is_iterable && !C::is_non_iterable) );
        D_IC_CHECK( (C::provides_const && C::provides_mutable) );
        D_IC_CHECK( (C::mode == iteration_mode::non_const) );
        D_IC_CHECK( (ic_cstr_eq(C::mode_name, "non_const")) );
    }

    // -- std::set<int>: iterable, const element -> const_only --
    {
        using C = iterable_container_class<std::set<int>>;
        D_IC_CHECK( (C::has_begin && C::has_end) );
        D_IC_CHECK( (C::is_iterable && !C::is_non_iterable) );
        D_IC_CHECK( (C::provides_const && !C::provides_mutable) );
        D_IC_CHECK( (C::mode == iteration_mode::const_only) );
        D_IC_CHECK( (ic_cstr_eq(C::mode_name, "const_only")) );
    }

    // -- std::map<int,int>: const KEY (not a const element) -> also const_only --
    {
        using C = iterable_container_class<std::map<int, int>>;
        D_IC_CHECK( (C::is_iterable && C::provides_const && !C::provides_mutable) );
        D_IC_CHECK( (C::mode == iteration_mode::const_only) );
    }

    // -- mutable_only_iter: settable, but nothing callable on a const lvalue --
    {
        using C = iterable_container_class<mutable_only_iter>;
        D_IC_CHECK( (C::has_begin && C::has_end) );
        D_IC_CHECK( (!C::has_iter_alias && !C::has_const_iter_alias) );  // traversal without aliases
        D_IC_CHECK( (C::is_iterable && !C::is_non_iterable) );
        D_IC_CHECK( (!C::provides_const) );                              // no const overload ...
        D_IC_CHECK( (C::provides_mutable) );                             // ... yet settable
        D_IC_CHECK( (C::mode == iteration_mode::non_const) );
    }

    // -- const_only_iter: a const-only traversal --
    {
        using C = iterable_container_class<const_only_iter>;
        D_IC_CHECK( (C::has_begin && C::has_end && C::is_iterable) );
        D_IC_CHECK( (C::provides_const && !C::provides_mutable) );
        D_IC_CHECK( (C::mode == iteration_mode::const_only) );
    }

    // -- no_value_type_iter: iterable, but not container-shaped --
    {
        using C = iterable_container_class<no_value_type_iter>;
        D_IC_CHECK( (C::is_iterable) );
        D_IC_CHECK( (!C::is_non_iterable) );        // no value_type, so neither classification
        D_IC_CHECK( (!C::provides_mutable) );       // the probe names value_type
        D_IC_CHECK( (C::mode == iteration_mode::const_only) );
    }

    // -- positional_only: the canonical NON-iterable container --
    {
        using C = iterable_container_class<positional_only>;
        D_IC_CHECK( (!C::has_begin && !C::has_end) );
        D_IC_CHECK( (!C::is_iterable && C::is_non_iterable) );
        D_IC_CHECK( (!C::provides_const && !C::provides_mutable) );
        D_IC_CHECK( (C::mode == iteration_mode::none) );
        D_IC_CHECK( (ic_cstr_eq(C::mode_name, "none")) );
    }

    // -- begin_only: every layer disagreeing at once --
    {
        using C = iterable_container_class<begin_only>;
        D_IC_CHECK( (C::has_begin && !C::has_end) );     // half a traversal ...
        D_IC_CHECK( (!C::is_iterable) );                  // ... so not iterable ...
        D_IC_CHECK( (C::is_non_iterable) );               // ... and it has a value_type
        D_IC_CHECK( (C::provides_mutable) );              // the raw probe fires (begin() only) ...
        D_IC_CHECK( (C::mode == iteration_mode::none) );  // ... but the mode gates on iterability
    }

    // -- alias_only: aliases with no traversal and no value_type --
    {
        using C = iterable_container_class<alias_only>;
        D_IC_CHECK( (C::has_iter_alias && C::has_const_iter_alias) );
        D_IC_CHECK( (!C::has_begin && !C::has_end) );
        D_IC_CHECK( (!C::is_iterable && !C::is_non_iterable) );
        D_IC_CHECK( (C::mode == iteration_mode::none) );
    }

    // -- int: nothing at all --
    {
        using C = iterable_container_class<int>;
        D_IC_CHECK( (!C::has_begin && !C::has_end) );
        D_IC_CHECK( (!C::has_iter_alias && !C::has_const_iter_alias) );
        D_IC_CHECK( (!C::is_iterable && !C::is_non_iterable) );
        D_IC_CHECK( (!C::provides_const && !C::provides_mutable) );
        D_IC_CHECK( (C::mode == iteration_mode::none) );
        D_IC_CHECK( (ic_cstr_eq(C::mode_name, "none")) );
    }

    // -- const std::vector<int>&: cv-ref stripped, so it matches the bare type --
    {
        using C = iterable_container_class<const std::vector<int>&>;
        D_IC_CHECK( (C::is_iterable && C::provides_mutable) );
        D_IC_CHECK( (C::mode == iteration_mode::non_const) );
    }

    return _ic_ok;
}


// tests_variable_templates
//   the `_v` companions agree with their `::value` forms.  Gated exactly as the
//   D_TYPE_TRAIT_VALUE_BOOL engine emits them (inline variables OR variable
//   templates).
bool
tests_variable_templates()
{
    bool _ic_ok = true;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES || D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // section-I detection companions
    D_IC_CHECK( (has_begin_method_v<std::vector<int>>
                     == has_begin_method<std::vector<int>>::value) );
    D_IC_CHECK( (has_begin_method_v<std::vector<int>>) );
    D_IC_CHECK( (has_end_method_v<std::vector<int>>) );
    D_IC_CHECK( (!has_end_method_v<begin_only>) );
    D_IC_CHECK( (has_iterator_alias_v<alias_only>) );
    D_IC_CHECK( (has_const_iterator_alias_v<std::vector<int>>) );
    D_IC_CHECK( (has_value_type_alias_v<positional_only>) );
    D_IC_CHECK( (!has_value_type_alias_v<no_value_type_iter>) );

    // section-II classification companions
    D_IC_CHECK( (is_iterable_container_v<std::vector<int>>
                     == is_iterable_container<std::vector<int>>::value) );
    D_IC_CHECK( (is_iterable_container_v<std::set<int>>) );
    D_IC_CHECK( (!is_iterable_container_v<positional_only>) );
    D_IC_CHECK( (is_non_iterable_container_v<positional_only>) );
    D_IC_CHECK( (!is_non_iterable_container_v<int>) );

    // section-III mode companions
    D_IC_CHECK( (provides_const_iteration_v<std::vector<int>>
                     == provides_const_iteration<std::vector<int>>::value) );
    D_IC_CHECK( (!provides_const_iteration_v<mutable_only_iter>) );
    D_IC_CHECK( (provides_mutable_iteration_v<std::vector<int>>) );
    D_IC_CHECK( (!provides_mutable_iteration_v<std::set<int>>) );

    D_IC_CHECK( (iteration_mode_of_v<std::vector<int>>
                     == iteration_mode_of<std::vector<int>>::value) );
    D_IC_CHECK( (iteration_mode_of_v<std::set<int>> == iteration_mode::const_only) );
    D_IC_CHECK( (iteration_mode_of_v<int>          == iteration_mode::none) );

    D_IC_CHECK( (is_const_iterable_container_v<std::vector<int>>) );   // observation, not const-only
    D_IC_CHECK( (is_mutable_iterable_container_v<std::vector<int>>) );
    D_IC_CHECK( (is_const_iterable_container_v<std::set<int>>) );
    D_IC_CHECK( (!is_mutable_iterable_container_v<std::set<int>>) );
    D_IC_CHECK( (!is_const_iterable_container_v<int>) );

#endif  // variable templates

    return _ic_ok;
}


NS_END  // testing
NS_END  // djinterp
