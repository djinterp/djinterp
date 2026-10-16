/*******************************************************************************
* djinterp [core]               observational_comparison_traits_tests_probes.cpp
*
*
* path:      /tests/djinterp/core/container/traits/observational_comparison_traits_tests_probes.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "observational_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   Interface-operation probes
// ===========================================================================
//   One SFINAE probe per inspection operation of the interface I.  All of them
// strip cv-ref through clean_t and read on a CONST lvalue, which is the
// operative constraint: observation must leave the container invariant, so an
// operation that is not const-callable is not an observation and is correctly
// invisible here.  That single rule is what excludes a std::map's inserting
// operator[] without any special case for maps.


/*
tests_has_size_member
  Verifies the probe for a const-callable size() - the equal-size interface,
  and the coarsest content step the header recognises.
  Tests the following:
  - the fixtures and standard containers that expose size() are detected
  - std::forward_list is NOT detected, since it deliberately has no size() -
    a container may be fully iterable and still fail this probe, which is why
    size is a rung of its own rather than a precondition of the finer ones
  - a raw array is not detected either: it has a bound, but no member
  - non-containers are not detected, and void does not hard-error
*/
bool
tests_has_size_member()
{
    bool ok = true;

    // present
    D_OC_CHECK(has_size_member<oct_size_only>::value       == true);
    D_OC_CHECK(has_size_member<oct_contains_and_size>::value == true);
    D_OC_CHECK(has_size_member<oct_full_interface>::value   == true);
    D_OC_CHECK(has_size_member<oct_vector>::value           == true);
    D_OC_CHECK(has_size_member<oct_array>::value            == true);
    D_OC_CHECK(has_size_member<oct_set>::value              == true);
    D_OC_CHECK(has_size_member<oct_map_int>::value          == true);
    D_OC_CHECK(has_size_member<oct_string>::value           == true);

    // a fully iterable container that has no size()
    D_OC_CHECK(has_size_member<oct_flist>::value == false);
    D_OC_CHECK(   ( is_iterable_container<oct_flist>::value == true )
               && ( has_size_member<oct_flist>::value       == false ));

    // a bound is not a member
    D_OC_CHECK(has_size_member<oct_raw_array>::value == false);

    // absent
    D_OC_CHECK(has_size_member<oct_bare>::value          == false);
    D_OC_CHECK(has_size_member<oct_iterable_only>::value == false);
    D_OC_CHECK(has_size_member<oct_pair>::value          == false);
    D_OC_CHECK(has_size_member<int>::value               == false);
    D_OC_CHECK(has_size_member<void>::value              == false);

    return ok;
}

/*
tests_has_subscript_index
  Verifies the probe for a const-callable operator[](size_t).
  Tests the following:
  - the sequence containers and the raw array are detected, the latter through
    the BUILT-IN subscript rather than a member - the probe asks whether the
    expression compiles, not what supplies it
  - a std::map is NOT detected: its operator[] inserts and so is non-const.
    This is the header's stated reason for reading on a const lvalue, and it
    falls out of the rule rather than being special-cased
  - a std::set is not detected: it has no subscript at all
  - the probe is independent of at(), witnessed in both directions
*/
bool
tests_has_subscript_index()
{
    bool ok = true;

    // present, by member
    D_OC_CHECK(has_subscript_index<oct_subscript_only>::value == true);
    D_OC_CHECK(has_subscript_index<oct_vector>::value         == true);
    D_OC_CHECK(has_subscript_index<oct_deque>::value          == true);
    D_OC_CHECK(has_subscript_index<oct_array>::value          == true);
    D_OC_CHECK(has_subscript_index<oct_string>::value         == true);

    // present, by the built-in subscript
    D_OC_CHECK(has_subscript_index<oct_raw_array>::value == true);

    // an inserting, non-const subscript is not an observation
    D_OC_CHECK(has_subscript_index<oct_map_int>::value          == false);
    D_OC_CHECK(has_subscript_index<oct_nonconst_subscript>::value == false);

    // absent
    D_OC_CHECK(has_subscript_index<oct_set>::value       == false);
    D_OC_CHECK(has_subscript_index<oct_flist>::value     == false);
    D_OC_CHECK(has_subscript_index<oct_at_only>::value   == false);
    D_OC_CHECK(has_subscript_index<oct_bare>::value      == false);
    D_OC_CHECK(has_subscript_index<int>::value           == false);
    D_OC_CHECK(has_subscript_index<void>::value          == false);

    // independent of at(), in both directions
    D_OC_CHECK(   ( has_subscript_index<oct_subscript_only>::value == true )
               && ( has_at_index<oct_subscript_only>::value        == false ));
    D_OC_CHECK(   ( has_subscript_index<oct_at_only>::value == false )
               && ( has_at_index<oct_at_only>::value        == true ));

    return ok;
}

/*
tests_has_at_index
  Verifies the probe for a const-callable at(size_t).
  Tests the following:
  - the sequence containers are detected
  - a std::map<int, V> IS detected, because its keyed at(key) accepts a size_t
    through the integral conversion.  The header names this approximation
    openly; the test pins it so it stays a known, bounded inaccuracy rather
    than drifting.  It costs nothing at the level, since such a map is iterable
    and so resolves seq regardless - which the test also checks
  - a std::map<std::string, V> is NOT detected, since no conversion exists,
    so the approximation is confined to integral keys
  - a raw array is not detected: the built-in subscript is not a member at()
  - an at() taking an unrelated parameter type is not detected
*/
bool
tests_has_at_index()
{
    bool ok = true;

    // present
    D_OC_CHECK(has_at_index<oct_at_only>::value       == true);
    D_OC_CHECK(has_at_index<oct_indexed_flat>::value  == true);
    D_OC_CHECK(has_at_index<oct_vector>::value        == true);
    D_OC_CHECK(has_at_index<oct_deque>::value         == true);
    D_OC_CHECK(has_at_index<oct_array>::value         == true);
    D_OC_CHECK(has_at_index<oct_string>::value        == true);

    // the integral-key approximation
    D_OC_CHECK(has_at_index<oct_map_int>::value    == true);
    D_OC_CHECK(has_at_index<oct_map_string>::value == false);

    // ... and it does not disturb the level, the map being iterable anyway
    D_OC_CHECK(observational_level_of<oct_map_int>::value
                   == observational_level::seq);
    D_OC_CHECK(observational_level_of<oct_map_int>::value
                   == observational_level_of<oct_map_string>::value);

    // absent
    D_OC_CHECK(has_at_index<oct_raw_array>::value      == false);
    D_OC_CHECK(has_at_index<oct_subscript_only>::value == false);
    D_OC_CHECK(has_at_index<oct_set>::value            == false);
    D_OC_CHECK(has_at_index<oct_at_wrong_arg>::value   == false);
    D_OC_CHECK(has_at_index<oct_nonconst_at>::value    == false);
    D_OC_CHECK(has_at_index<oct_bare>::value           == false);
    D_OC_CHECK(has_at_index<int>::value                == false);
    D_OC_CHECK(has_at_index<void>::value               == false);

    return ok;
}

/*
tests_has_indexed_access
  Verifies the disjunction of the two positional probes - the interface that,
  over a nested shape, supplies str, and over a flat one supplies a positional
  seq read.
  Tests the following:
  - either operand alone promotes it, witnessed on a fixture carrying only one
  - it equals its definition, term by term, across carriers lighting different
    subsets
  - a non-const subscript promotes neither operand and so leaves it dark
  - it holds for a raw array, through the built-in subscript
*/
bool
tests_has_indexed_access()
{
    bool ok = true;

    // either operand alone
    D_OC_CHECK(has_indexed_access<oct_subscript_only>::value == true);
    D_OC_CHECK(has_indexed_access<oct_at_only>::value        == true);

    // both together
    D_OC_CHECK(has_indexed_access<oct_indexed_flat>::value == true);
    D_OC_CHECK(has_indexed_access<oct_vector>::value       == true);

    // one, via the map approximation
    D_OC_CHECK(has_indexed_access<oct_map_int>::value == true);

    // built-in subscript
    D_OC_CHECK(has_indexed_access<oct_raw_array>::value == true);

    // neither
    D_OC_CHECK(has_indexed_access<oct_set>::value                == false);
    D_OC_CHECK(has_indexed_access<oct_flist>::value              == false);
    D_OC_CHECK(has_indexed_access<oct_map_string>::value         == false);
    D_OC_CHECK(has_indexed_access<oct_nonconst_subscript>::value == false);
    D_OC_CHECK(has_indexed_access<oct_nonconst_at>::value        == false);
    D_OC_CHECK(has_indexed_access<oct_bare>::value               == false);
    D_OC_CHECK(has_indexed_access<int>::value                    == false);
    D_OC_CHECK(has_indexed_access<void>::value                   == false);

    // term-by-term agreement with the definition
    D_OC_CHECK(has_indexed_access<oct_subscript_only>::value
                   == (    has_subscript_index<oct_subscript_only>::value
                        || has_at_index<oct_subscript_only>::value ));
    D_OC_CHECK(has_indexed_access<oct_at_only>::value
                   == (    has_subscript_index<oct_at_only>::value
                        || has_at_index<oct_at_only>::value ));
    D_OC_CHECK(has_indexed_access<oct_map_int>::value
                   == (    has_subscript_index<oct_map_int>::value
                        || has_at_index<oct_map_int>::value ));
    D_OC_CHECK(has_indexed_access<oct_set>::value
                   == (    has_subscript_index<oct_set>::value
                        || has_at_index<oct_set>::value ));

    return ok;
}

/*
tests_has_count_member
  Verifies the probe for a const-callable count(x) taking an element - the
  per-class (bag) interface.
  Tests the following:
  - std::set and std::multiset are detected, their element type being the key
  - a std::map is NOT detected: the probe passes a value_type, which for a map
    is a pair, and count() takes a key.  The header states this limitation
    openly and the test pins it
  - a sequence container has no count() and is not detected
  - a count() taking an unrelated type, or one that is not const-callable, is
    not detected
  - the probe is independent of contains(), witnessed in both directions
*/
bool
tests_has_count_member()
{
    bool ok = true;

    // present
    D_OC_CHECK(has_count_member<oct_count_only>::value          == true);
    D_OC_CHECK(has_count_member<oct_count_and_contains>::value  == true);
    D_OC_CHECK(has_count_member<oct_hierarchical_count>::value  == true);
    D_OC_CHECK(has_count_member<oct_full_interface>::value      == true);
    D_OC_CHECK(has_count_member<oct_set>::value                 == true);
    D_OC_CHECK(has_count_member<oct_multiset>::value            == true);

    // the keyed-container limitation
    D_OC_CHECK(has_count_member<oct_map_int>::value    == false);
    D_OC_CHECK(has_count_member<oct_map_string>::value == false);

    // absent
    D_OC_CHECK(has_count_member<oct_vector>::value           == false);
    D_OC_CHECK(has_count_member<oct_deque>::value            == false);
    D_OC_CHECK(has_count_member<oct_contains_only>::value    == false);
    D_OC_CHECK(has_count_member<oct_count_wrong_arg>::value  == false);
    D_OC_CHECK(has_count_member<oct_count_nonconst>::value   == false);
    D_OC_CHECK(has_count_member<oct_bare>::value             == false);
    D_OC_CHECK(has_count_member<int>::value                  == false);
    D_OC_CHECK(has_count_member<void>::value                 == false);

    // independent of contains(), in both directions
    D_OC_CHECK(   ( has_count_member<oct_count_only>::value    == true )
               && ( has_contains_member<oct_count_only>::value == false ));
    D_OC_CHECK(   ( has_count_member<oct_contains_only>::value    == false )
               && ( has_contains_member<oct_contains_only>::value == true ));

    return ok;
}

/*
tests_has_contains_member
  Verifies the probe for a const-callable contains(x) - the membership (set)
  interface.  This is a C++20 member for the standard associative containers;
  the probe is generic, so it simply reads false for them under an older
  dialect without disturbing any level, since an iterable container already
  resolves the finer seq.
  Tests the following:
  - the fixtures that declare contains() are detected
  - the same keyed-container limitation applies as for count()
  - a sequence container is not detected
  - the level of a std::set is seq either way, which is what makes the
    dialect-dependence harmless - checked explicitly rather than asserted
  - a contains() taking an unrelated type, or not const-callable, is not
    detected
*/
bool
tests_has_contains_member()
{
    bool ok = true;

    // present
    D_OC_CHECK(has_contains_member<oct_contains_only>::value       == true);
    D_OC_CHECK(has_contains_member<oct_count_and_contains>::value  == true);
    D_OC_CHECK(has_contains_member<oct_contains_and_size>::value   == true);
    D_OC_CHECK(has_contains_member<oct_full_interface>::value      == true);

    // keyed containers
    D_OC_CHECK(has_contains_member<oct_map_int>::value    == false);
    D_OC_CHECK(has_contains_member<oct_map_string>::value == false);

    // absent
    D_OC_CHECK(has_contains_member<oct_vector>::value              == false);
    D_OC_CHECK(has_contains_member<oct_count_only>::value          == false);
    D_OC_CHECK(has_contains_member<oct_contains_wrong_arg>::value  == false);
    D_OC_CHECK(has_contains_member<oct_contains_nonconst>::value   == false);
    D_OC_CHECK(has_contains_member<oct_bare>::value                == false);
    D_OC_CHECK(has_contains_member<int>::value                     == false);
    D_OC_CHECK(has_contains_member<void>::value                    == false);

    // whatever the dialect answers for a std::set, its level is seq, reached
    // through iteration - so the probe's dialect-dependence cannot move it
    D_OC_CHECK(observational_level_of<oct_set>::value
                   == observational_level::seq);
    D_OC_CHECK(   ( is_iterable_container<oct_set>::value == true )
               && ( observational_level_of<oct_set>::value
                        == observational_level::seq ));

    return ok;
}

/*
tests_probe_method_shapes
  Verifies WHAT the probes admit, using size() as the representative, since the
  question they ask is "does this expression compile on a const lvalue" rather
  than "is a member of this name declared".
  Tests the following:
  - a void return counts: the probe names the expression's type and never
    constrains it
  - a private member does NOT count - access control participates in the probe
  - a required parameter breaks the nullary call; a DEFAULTED one does not
  - a data member named size does not count, being uncallable
  - a static member function DOES count, being callable through an object
  - the admitted shapes propagate to the level, and the rejected ones do not
*/
bool
tests_probe_method_shapes()
{
    bool ok = true;

    // admitted
    D_OC_CHECK(has_size_member<oct_void_size>::value          == true);
    D_OC_CHECK(has_size_member<oct_default_arg_size>::value   == true);
    D_OC_CHECK(has_size_member<oct_static_size_member>::value == true);

    // rejected
    D_OC_CHECK(has_size_member<oct_private_size>::value  == false);
    D_OC_CHECK(has_size_member<oct_arg_size>::value      == false);
    D_OC_CHECK(has_size_member<oct_data_size>::value     == false);
    D_OC_CHECK(has_size_member<oct_nonconst_size>::value == false);

    // the distinction between the two parameter lists is the nullary call
    D_OC_CHECK(   ( has_size_member<oct_default_arg_size>::value == true )
               && ( has_size_member<oct_arg_size>::value         == false ));

    // and every admitted shape carries through to the level
    D_OC_CHECK(observational_level_of<oct_void_size>::value
                   == observational_level::size);
    D_OC_CHECK(observational_level_of<oct_default_arg_size>::value
                   == observational_level::size);
    D_OC_CHECK(observational_level_of<oct_static_size_member>::value
                   == observational_level::size);

    // ... while every rejected one leaves the type at none
    D_OC_CHECK(observational_level_of<oct_private_size>::value
                   == observational_level::none);
    D_OC_CHECK(observational_level_of<oct_arg_size>::value
                   == observational_level::none);
    D_OC_CHECK(observational_level_of<oct_data_size>::value
                   == observational_level::none);
    D_OC_CHECK(observational_level_of<oct_nonconst_size>::value
                   == observational_level::none);

    return ok;
}

/*
tests_probe_const_observation
  Verifies the rule that unifies section I: an operation is an observation only
  if it is callable on a CONST lvalue.  Observation must leave the container
  invariant, so a mutating operation of the right name is not an inspection
  surface.
  Tests the following:
  - every one of the four member probes rejects its non-const counterpart
  - the std::map case falls out of the rule with no special handling: its
    inserting operator[] is invisible, so a map's indexed access comes from
    at() alone
  - a type whose whole interface is non-const resolves none, not size
*/
bool
tests_probe_const_observation()
{
    bool ok = true;

    // each probe against its non-const counterpart
    D_OC_CHECK(has_size_member<oct_nonconst_size>::value           == false);
    D_OC_CHECK(has_subscript_index<oct_nonconst_subscript>::value  == false);
    D_OC_CHECK(has_at_index<oct_nonconst_at>::value                == false);
    D_OC_CHECK(has_count_member<oct_count_nonconst>::value         == false);
    D_OC_CHECK(has_contains_member<oct_contains_nonconst>::value   == false);

    // the map's inserting subscript, excluded by the same rule
    D_OC_CHECK(   ( has_subscript_index<oct_map_int>::value == false )
               && ( has_at_index<oct_map_int>::value        == true )
               && ( has_indexed_access<oct_map_int>::value  == true ));

    // an interface that is entirely non-const observes nothing
    D_OC_CHECK(observational_level_of<oct_nonconst_subscript>::value
                   == observational_level::none);
    D_OC_CHECK(observational_level_of<oct_count_nonconst>::value
                   == observational_level::none);
    D_OC_CHECK(observational_level_of<oct_contains_nonconst>::value
                   == observational_level::none);

    return ok;
}

/*
tests_probe_void_element_safety
  Verifies that the element-taking probes survive a type with NO element type.

  This is a robustness property worth its own test.  count and contains are
  written as declval<element_type_of_t<_Type>>(), and element_type_of_t
  resolves to void for a type with no value_type - so the probes form
  declval<void>() and pass a void expression as an argument.  The sibling
  header element_relation_traits.hpp guards its own element probes behind an
  explicit is_void gate for exactly this reason; this header carries no such
  gate and instead relies on the failure being a substitution failure in the
  immediate context rather than a hard error.  It is, but the reliance is
  implicit, so the test makes it explicit: if a refactor moves the probe out of
  the immediate context, this stops compiling rather than silently misreading.

  Tests the following:
  - the element type really is void for the carriers used below
  - a type declaring a perfectly good count(int) and contains(int) but NO
    value_type reads false for both, rather than failing to compile
  - scalars, void and a bare struct do likewise
  - the level of such a type falls through to whatever its non-element
    operations support
*/
bool
tests_probe_void_element_safety()
{
    bool ok = true;

    // the element type is genuinely void for these
    D_OC_STATIC_CHECK(std::is_void<element_type_of_t<int> >::value);
    D_OC_STATIC_CHECK(std::is_void<element_type_of_t<oct_bare> >::value);
    D_OC_STATIC_CHECK(
        std::is_void<element_type_of_t<oct_count_no_value_type> >::value);

    // ... and the probes read false rather than failing to compile
    D_OC_CHECK(has_count_member<oct_count_no_value_type>::value    == false);
    D_OC_CHECK(has_contains_member<oct_count_no_value_type>::value == false);
    D_OC_CHECK(has_count_member<int>::value                        == false);
    D_OC_CHECK(has_contains_member<int>::value                     == false);
    D_OC_CHECK(has_count_member<void>::value                       == false);
    D_OC_CHECK(has_contains_member<void>::value                    == false);
    D_OC_CHECK(has_count_member<oct_bare>::value                   == false);
    D_OC_CHECK(has_contains_member<oct_size_only>::value           == false);

    // the members are declared and callable - it is only the ELEMENT that is
    // missing, which is what makes this fixture the right witness
    D_OC_CHECK(observational_level_of<oct_count_no_value_type>::value
                   == observational_level::none);

    // a type WITH a value_type is detected, all else equal
    D_OC_CHECK(   ( has_count_member<oct_count_only>::value == true )
               && ( has_count_member<oct_count_no_value_type>::value
                        == false ));

    return ok;
}

/*
tests_probe_cv_ref
  Verifies that every section I probe honours the clean_t contract.  Each
  writes clean_t<_Type> before forming its const lvalue, so cv-qualification
  and reference-ness are stripped and all seven spellings of a type answer
  alike - including the volatile ones, which a probe phrased on the unstripped
  type would lose.
  Tests the following:
  - all six probes, across a positive and a negative carrier each, in all seven
    cv- and reference-qualified spellings
  - a volatile container is still fully observable, since clean_t removes the
    qualifier before the const lvalue is formed
*/
bool
tests_probe_cv_ref()
{
    bool ok = true;

    D_OC_CV_REF_AGREE(has_size_member, oct_size_only, true);
    D_OC_CV_REF_AGREE(has_size_member, oct_vector,    true);
    D_OC_CV_REF_AGREE(has_size_member, oct_bare,      false);

    D_OC_CV_REF_AGREE(has_subscript_index, oct_vector,   true);
    D_OC_CV_REF_AGREE(has_subscript_index, oct_set,      false);

    D_OC_CV_REF_AGREE(has_at_index, oct_at_only, true);
    D_OC_CV_REF_AGREE(has_at_index, oct_set,     false);

    D_OC_CV_REF_AGREE(has_indexed_access, oct_subscript_only, true);
    D_OC_CV_REF_AGREE(has_indexed_access, oct_at_only,        true);
    D_OC_CV_REF_AGREE(has_indexed_access, oct_set,            false);

    D_OC_CV_REF_AGREE(has_count_member, oct_count_only, true);
    D_OC_CV_REF_AGREE(has_count_member, oct_vector,     false);

    D_OC_CV_REF_AGREE(has_contains_member, oct_contains_only, true);
    D_OC_CV_REF_AGREE(has_contains_member, oct_vector,        false);

    // spelled out on one carrier, since a volatile container surviving the
    // probe is the point of writing clean_t
    D_OC_CHECK(has_size_member<volatile oct_vector>::value       == true);
    D_OC_CHECK(has_subscript_index<volatile oct_vector>::value   == true);
    D_OC_CHECK(has_indexed_access<volatile oct_vector>::value    == true);
    D_OC_CHECK(has_count_member<volatile oct_set>::value         == true);

    return ok;
}

/*
tests_probe_companions
  Verifies the `_v` companions of the section I probes.
  Tests the following:
  - each of the six companions agrees with its trait's ::value on a positive
    and a negative carrier
  - the companions are usable in a constant expression
  - gated on variable-template support, since below C++14 the engine emits
    nothing and ::value remains the portable spelling
*/
bool
tests_probe_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_OC_STATIC_CHECK(has_size_member_v<oct_vector>      == true);
    D_OC_STATIC_CHECK(has_indexed_access_v<oct_set>      == false);

    D_OC_CHECK(has_size_member_v<oct_size_only>
                   == has_size_member<oct_size_only>::value);
    D_OC_CHECK(has_size_member_v<oct_bare>
                   == has_size_member<oct_bare>::value);

    D_OC_CHECK(has_subscript_index_v<oct_vector>
                   == has_subscript_index<oct_vector>::value);
    D_OC_CHECK(has_subscript_index_v<oct_set>
                   == has_subscript_index<oct_set>::value);

    D_OC_CHECK(has_at_index_v<oct_at_only>
                   == has_at_index<oct_at_only>::value);
    D_OC_CHECK(has_at_index_v<oct_set>
                   == has_at_index<oct_set>::value);

    D_OC_CHECK(has_indexed_access_v<oct_subscript_only>
                   == has_indexed_access<oct_subscript_only>::value);
    D_OC_CHECK(has_indexed_access_v<oct_flist>
                   == has_indexed_access<oct_flist>::value);

    D_OC_CHECK(has_count_member_v<oct_count_only>
                   == has_count_member<oct_count_only>::value);
    D_OC_CHECK(has_count_member_v<oct_vector>
                   == has_count_member<oct_vector>::value);

    D_OC_CHECK(has_contains_member_v<oct_contains_only>
                   == has_contains_member<oct_contains_only>::value);
    D_OC_CHECK(has_contains_member_v<oct_vector>
                   == has_contains_member<oct_vector>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
