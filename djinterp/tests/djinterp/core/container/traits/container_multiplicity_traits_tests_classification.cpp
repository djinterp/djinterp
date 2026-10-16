/*******************************************************************************
* djinterp [testing]      container_multiplicity_traits_tests_classification.cpp
*
*   Section IV - multiplicity_container_class, the assembled snapshot (the four
* raw signals, the kind and its numeric bound and name, and the three kind
* shorthands), checked member-for-member against the standalone traits across a
* representative of every kind - including the interval route to unique and the
* opt-in route to bounded_multiset.  The `_v` variable-template companions of the
* section-I/III bool traits (and multiplicity_bound_of_v) are verified here too,
* gated exactly as the header emits them.
*
*
* path:      /tests/djinterp/core/container/traits/container_multiplicity_traits_tests_classification.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_multiplicity_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_multiplicity_container_class
//   the assembled snapshot, one representative per kind (and per route).
bool
tests_multiplicity_container_class()
{
    bool _cmp_ok = true;

    // -- vector<int>: comparator-less sequence --
    {
        using C = multiplicity_container_class<std::vector<int>>;
        D_CMP_CHECK( (!C::has_equivalence) );
        D_CMP_CHECK( (!C::unique_insert) );
        D_CMP_CHECK( (!C::interval_domain) );
        D_CMP_CHECK( (!C::opt_in_bound) );
        D_CMP_CHECK( (C::kind == multiplicity_kind::sequence) );
        D_CMP_CHECK( (C::bound == unbounded_multiplicity) );
        D_CMP_CHECK( (cmp_cstr_eq(C::kind_name, "sequence")) );
        D_CMP_CHECK( (C::is_sequence) );
        D_CMP_CHECK( (!C::is_unique) );
        D_CMP_CHECK( (!C::is_multiset) );
    }

    // -- set<int>: unique associative (keyed + unique insert) --
    {
        using C = multiplicity_container_class<std::set<int>>;
        D_CMP_CHECK( (C::has_equivalence) );
        D_CMP_CHECK( (C::unique_insert) );
        D_CMP_CHECK( (!C::interval_domain) );
        D_CMP_CHECK( (!C::opt_in_bound) );
        D_CMP_CHECK( (C::kind == multiplicity_kind::unique) );
        D_CMP_CHECK( (C::bound == unique_multiplicity) );
        D_CMP_CHECK( (C::bound == 1u) );
        D_CMP_CHECK( (cmp_cstr_eq(C::kind_name, "unique")) );
        D_CMP_CHECK( (!C::is_sequence) );
        D_CMP_CHECK( (C::is_unique) );
        D_CMP_CHECK( (!C::is_multiset) );
    }

    // -- multiset<int>: keyed, no unique insert -> unbounded_multiset --
    {
        using C = multiplicity_container_class<std::multiset<int>>;
        D_CMP_CHECK( (C::has_equivalence) );
        D_CMP_CHECK( (!C::unique_insert) );
        D_CMP_CHECK( (C::kind == multiplicity_kind::unbounded_multiset) );
        D_CMP_CHECK( (C::bound == unbounded_multiplicity) );
        D_CMP_CHECK( (cmp_cstr_eq(C::kind_name, "unbounded_multiset")) );
        D_CMP_CHECK( (!C::is_unique) );
        D_CMP_CHECK( (C::is_multiset) );
    }

    // -- interval_seq: unique via the interval route (no key, no opt-in) --
    {
        using C = multiplicity_container_class<interval_seq>;
        D_CMP_CHECK( (!C::has_equivalence) );
        D_CMP_CHECK( (C::interval_domain) );
        D_CMP_CHECK( (!C::opt_in_bound) );
        D_CMP_CHECK( (C::kind == multiplicity_kind::unique) );
        D_CMP_CHECK( (C::bound == 1u) );
        D_CMP_CHECK( (C::is_unique) );
    }

    // -- optin_bounded: bounded_multiset via the opt-in route --
    {
        using C = multiplicity_container_class<optin_bounded>;
        D_CMP_CHECK( (!C::has_equivalence) );
        D_CMP_CHECK( (!C::interval_domain) );
        D_CMP_CHECK( (C::opt_in_bound) );
        D_CMP_CHECK( (C::kind == multiplicity_kind::bounded_multiset) );
        D_CMP_CHECK( (C::bound == 5u) );
        D_CMP_CHECK( (cmp_cstr_eq(C::kind_name, "bounded_multiset")) );
        D_CMP_CHECK( (!C::is_unique) );
        D_CMP_CHECK( (C::is_multiset) );
    }

    // -- int: unknown --
    {
        using C = multiplicity_container_class<int>;
        D_CMP_CHECK( (!C::has_equivalence) );
        D_CMP_CHECK( (!C::unique_insert) );
        D_CMP_CHECK( (!C::interval_domain) );
        D_CMP_CHECK( (!C::opt_in_bound) );
        D_CMP_CHECK( (C::kind == multiplicity_kind::unknown) );
        D_CMP_CHECK( (cmp_cstr_eq(C::kind_name, "unknown")) );
        D_CMP_CHECK( (!C::is_sequence) );
        D_CMP_CHECK( (!C::is_unique) );
        D_CMP_CHECK( (!C::is_multiset) );
    }

    return _cmp_ok;
}


// tests_multiplicity_variable_templates
//   the `_v` companions agree with their `::value` forms.  Gated on variable-
//   template support exactly as the header emits them.
bool
tests_multiplicity_variable_templates()
{
    bool _cmp_ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // section-I signal companions
    D_CMP_CHECK( (is_countable_container_v<std::vector<int>>
                     == is_countable_container<std::vector<int>>::value) );
    D_CMP_CHECK( (has_interval_bounds_signal_v<interval_seq>
                     == has_interval_bounds_signal<interval_seq>::value) );
    D_CMP_CHECK( (has_unique_insert_v<std::set<int>>
                     == has_unique_insert<std::set<int>>::value) );
    D_CMP_CHECK( (has_multiplicity_bound_v<optin_bounded>
                     == has_multiplicity_bound<optin_bounded>::value) );

    // direct reads
    D_CMP_CHECK( (is_countable_container_v<std::vector<int>>) );
    D_CMP_CHECK( (!is_countable_container_v<std::forward_list<int>>) );
    D_CMP_CHECK( (has_unique_insert_v<std::set<int>>) );
    D_CMP_CHECK( (!has_unique_insert_v<std::multiset<int>>) );

    // section-III predicate companions
    D_CMP_CHECK( (is_sequence_container_v<std::vector<int>>
                     == is_sequence_container<std::vector<int>>::value) );
    D_CMP_CHECK( (is_unique_container_v<std::set<int>>
                     == is_unique_container<std::set<int>>::value) );
    D_CMP_CHECK( (is_multiset_container_v<std::multiset<int>>
                     == is_multiset_container<std::multiset<int>>::value) );
    D_CMP_CHECK( (is_sequence_container_v<std::vector<int>>) );
    D_CMP_CHECK( (is_unique_container_v<interval_seq>) );
    D_CMP_CHECK( (is_multiset_container_v<optin_bounded>) );

    // the numeric-bound companion
    D_CMP_CHECK( (multiplicity_bound_of_v<std::set<int>>
                     == multiplicity_bound_of<std::set<int>>::value) );
    D_CMP_CHECK( (multiplicity_bound_of_v<optin_bounded> == 5u) );
    D_CMP_CHECK( (multiplicity_bound_of_v<std::vector<int>> == unbounded_multiplicity) );

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return _cmp_ok;
}


NS_END  // testing
NS_END  // djinterp
