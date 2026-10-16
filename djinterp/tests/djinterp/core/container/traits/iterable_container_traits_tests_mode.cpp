/*******************************************************************************
* djinterp [testing]                    iterable_container_traits_tests_mode.cpp
*
*   Section III, type level - the mode a container's traversal actually grants.
* provides_const_iteration asks whether begin() AND end() are callable on a CONST
* lvalue; provides_mutable_iteration asks whether a value_type may be written
* through *begin(); iteration_mode_of resolves the two into none / const_only /
* non_const, gating on iterability first; and is_const_iterable_container /
* is_mutable_iterable_container project that mode back to bools.
*
*   Four edges are pinned here.  (1) is_const_iterable_container is
* mode_grants_observation - "may be observed", NOT "const-only" - so it is TRUE for
* std::vector<int>; mutable_only_iter separates it from provides_const_iteration,
* which is FALSE there.  (2) The const_only verdict on std::set and std::map arises
* for different reasons - a const element versus a pair with a const key - but the
* whole-element-shaped probe reports both alike, exactly as the file header notes.
* (3) provides_mutable_iteration names value_type, so no_value_type_iter reads
* const_only despite a genuinely settable traversal.  (4) The mode gates on
* iterability first: begin_only has provides_mutable_iteration TRUE (that probe
* never touches end()) yet its mode is none.
*
*
* path:      /tests/djinterp/core/container/traits/iterable_container_traits_tests_mode.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "iterable_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_provides_const_iteration
//   begin() AND end() callable on a const lvalue.
bool
tests_provides_const_iteration()
{
    bool _ic_ok = true;

    // every standard container offers const overloads
    D_IC_CHECK( (provides_const_iteration<std::vector<int>>::value) );
    D_IC_CHECK( (provides_const_iteration<std::set<int>>::value) );
    D_IC_CHECK( (provides_const_iteration<std::map<int, int>>::value) );
    D_IC_CHECK( (provides_const_iteration<std::forward_list<int>>::value) );
    D_IC_CHECK( (provides_const_iteration<std::array<int, 4>>::value) );
    D_IC_CHECK( (provides_const_iteration<std::string>::value) );
    D_IC_CHECK( (provides_const_iteration<const_only_iter>::value) );

    // a traversal with NO const overload - the trait that separates "callable on a
    // const lvalue" from the observation-shaped is_const_iterable_container
    D_IC_CHECK( (!provides_const_iteration<mutable_only_iter>::value) );
    D_IC_CHECK( (!provides_const_iteration<no_value_type_iter>::value) );

    // both halves are required, and a non-traversable type has neither
    D_IC_CHECK( (!provides_const_iteration<begin_only>::value) );
    D_IC_CHECK( (!provides_const_iteration<positional_only>::value) );
    D_IC_CHECK( (!provides_const_iteration<int>::value) );

    // cv-ref is stripped before the probe
    D_IC_CHECK( (provides_const_iteration<const std::vector<int>&>::value) );

    return _ic_ok;
}


// tests_provides_mutable_iteration
//   a value_type may be written through *begin().
bool
tests_provides_mutable_iteration()
{
    bool _ic_ok = true;

    // settable sequences
    D_IC_CHECK( (provides_mutable_iteration<std::vector<int>>::value) );
    D_IC_CHECK( (provides_mutable_iteration<std::deque<int>>::value) );
    D_IC_CHECK( (provides_mutable_iteration<std::list<int>>::value) );
    D_IC_CHECK( (provides_mutable_iteration<std::forward_list<int>>::value) );
    D_IC_CHECK( (provides_mutable_iteration<std::string>::value) );
    D_IC_CHECK( (provides_mutable_iteration<std::array<int, 4>>::value) );
    D_IC_CHECK( (provides_mutable_iteration<mutable_only_iter>::value) );
    // a PROXY reference satisfies the whole-element probe too
    D_IC_CHECK( (provides_mutable_iteration<std::vector<bool>>::value) );

    // associative overlays read as NOT settable, for two different reasons that the
    // whole-element-shaped probe reports alike:
    D_IC_CHECK( (!provides_mutable_iteration<std::set<int>>::value) );        // const element
    D_IC_CHECK( (!provides_mutable_iteration<std::multiset<int>>::value) );
    D_IC_CHECK( (!provides_mutable_iteration<std::unordered_set<int>>::value) );
    D_IC_CHECK( (!provides_mutable_iteration<std::map<int, int>>::value) );   // pair with a const key
    // a const-only traversal
    D_IC_CHECK( (!provides_mutable_iteration<const_only_iter>::value) );

    // the probe NAMES value_type: a settable traversal without one fails SFINAE
    D_IC_CHECK( (!provides_mutable_iteration<no_value_type_iter>::value) );

    // the probe touches begin() only - so half a traversal can still satisfy it
    D_IC_CHECK( (provides_mutable_iteration<begin_only>::value) );
    D_IC_CHECK( (!provides_mutable_iteration<end_only>::value) );             // no begin()

    // no traversal at all
    D_IC_CHECK( (!provides_mutable_iteration<positional_only>::value) );
    D_IC_CHECK( (!provides_mutable_iteration<int>::value) );

    // cv-ref is stripped, so a const view still reads as settable
    D_IC_CHECK( (provides_mutable_iteration<const std::vector<int>&>::value) );

    return _ic_ok;
}


// tests_iteration_mode_of
//   the resolved mode - none unless iterable, then non_const if settable, else
//   const_only - plus the ::type carrier.
bool
tests_iteration_mode_of()
{
    bool _ic_ok = true;

    // non_const: settable traversals
    D_IC_CHECK( (iteration_mode_of<std::vector<int>>::value  == iteration_mode::non_const) );
    D_IC_CHECK( (iteration_mode_of<std::deque<int>>::value   == iteration_mode::non_const) );
    D_IC_CHECK( (iteration_mode_of<std::list<int>>::value    == iteration_mode::non_const) );
    D_IC_CHECK( (iteration_mode_of<std::string>::value       == iteration_mode::non_const) );
    D_IC_CHECK( (iteration_mode_of<std::array<int, 4>>::value == iteration_mode::non_const) );
    D_IC_CHECK( (iteration_mode_of<std::vector<bool>>::value == iteration_mode::non_const) );  // via the proxy
    D_IC_CHECK( (iteration_mode_of<mutable_only_iter>::value == iteration_mode::non_const) );

    // const_only: iterable, but not settable through the element
    D_IC_CHECK( (iteration_mode_of<std::set<int>>::value      == iteration_mode::const_only) );
    D_IC_CHECK( (iteration_mode_of<std::map<int, int>>::value == iteration_mode::const_only) );
    D_IC_CHECK( (iteration_mode_of<std::unordered_set<int>>::value == iteration_mode::const_only) );
    D_IC_CHECK( (iteration_mode_of<const_only_iter>::value    == iteration_mode::const_only) );
    // ... including the value_type-less traversal, whose settability cannot be probed
    D_IC_CHECK( (iteration_mode_of<no_value_type_iter>::value == iteration_mode::const_only) );

    // none: iterability is checked FIRST, so a settable half-traversal still reads none
    D_IC_CHECK( (iteration_mode_of<begin_only>::value == iteration_mode::none) );
    D_IC_CHECK( (provides_mutable_iteration<begin_only>::value) );   // ... despite this being true
    D_IC_CHECK( (iteration_mode_of<end_only>::value        == iteration_mode::none) );
    D_IC_CHECK( (iteration_mode_of<positional_only>::value == iteration_mode::none) );
    D_IC_CHECK( (iteration_mode_of<alias_only>::value      == iteration_mode::none) );
    D_IC_CHECK( (iteration_mode_of<int>::value             == iteration_mode::none) );
    D_IC_CHECK( (iteration_mode_of<int[4]>::value          == iteration_mode::none) );

    // cv-ref is stripped: a const view of a vector reads non_const, NOT const_only -
    // the trait classifies the underlying type, not the qualified view
    D_IC_CHECK( (iteration_mode_of<const std::vector<int>&>::value == iteration_mode::non_const) );

    // ::type carrier
    D_IC_CHECK( (iteration_mode_of_t<std::vector<int>>::value == iteration_mode::non_const) );
    D_IC_CHECK( (iteration_mode_of_t<std::set<int>>::value    == iteration_mode::const_only) );
    D_IC_CHECK( (iteration_mode_of_t<int>::value              == iteration_mode::none) );

    return _ic_ok;
}


// tests_derived_mode_predicates
//   is_const_iterable_container (observation - NOT "const-only") and
//   is_mutable_iterable_container (mutation).
bool
tests_derived_mode_predicates()
{
    bool _ic_ok = true;

    // THE naming trap: is_const_iterable_container is mode_grants_observation, so it
    // holds for a MUTABLY iterable container too
    D_IC_CHECK( (is_const_iterable_container<std::vector<int>>::value) );
    D_IC_CHECK( (is_mutable_iterable_container<std::vector<int>>::value) );   // both true at once
    D_IC_CHECK( (is_const_iterable_container<std::string>::value) );
    D_IC_CHECK( (is_const_iterable_container<std::array<int, 4>>::value) );

    // ... and it is distinct from provides_const_iteration, which is FALSE here
    D_IC_CHECK( (is_const_iterable_container<mutable_only_iter>::value) );
    D_IC_CHECK( (!provides_const_iteration<mutable_only_iter>::value) );

    // const-only containers observe but do not mutate
    D_IC_CHECK( (is_const_iterable_container<std::set<int>>::value) );
    D_IC_CHECK( (!is_mutable_iterable_container<std::set<int>>::value) );
    D_IC_CHECK( (is_const_iterable_container<std::map<int, int>>::value) );
    D_IC_CHECK( (!is_mutable_iterable_container<std::map<int, int>>::value) );
    D_IC_CHECK( (is_const_iterable_container<const_only_iter>::value) );
    D_IC_CHECK( (!is_mutable_iterable_container<const_only_iter>::value) );
    D_IC_CHECK( (is_const_iterable_container<no_value_type_iter>::value) );
    D_IC_CHECK( (!is_mutable_iterable_container<no_value_type_iter>::value) );

    // non-iterable types grant neither
    D_IC_CHECK( (!is_const_iterable_container<positional_only>::value) );
    D_IC_CHECK( (!is_mutable_iterable_container<positional_only>::value) );
    D_IC_CHECK( (!is_const_iterable_container<begin_only>::value) );          // mode is none
    D_IC_CHECK( (!is_mutable_iterable_container<begin_only>::value) );
    D_IC_CHECK( (!is_const_iterable_container<int>::value) );
    D_IC_CHECK( (!is_mutable_iterable_container<int>::value) );

    // mutation implies observation over every type tested (the capability order)
    D_IC_CHECK( (!is_mutable_iterable_container<std::vector<int>>::value
                 || is_const_iterable_container<std::vector<int>>::value) );
    D_IC_CHECK( (!is_mutable_iterable_container<int>::value
                 || is_const_iterable_container<int>::value) );

    // is_const_iterable_container agrees with iterability exactly (both are mode != none)
    D_IC_CHECK( (is_const_iterable_container<std::set<int>>::value
                     == is_iterable_container<std::set<int>>::value) );
    D_IC_CHECK( (is_const_iterable_container<begin_only>::value
                     == is_iterable_container<begin_only>::value) );

    return _ic_ok;
}


NS_END  // testing
NS_END  // djinterp
