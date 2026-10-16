/*******************************************************************************
* djinterp [testing]                                      pack_element_tests.hpp
*
*   Unit-test surface for the positional pack accessor (core/meta/
* pack_element.hpp).  This header carries ONLY declarations and the fixtures
* the section TUs share; every `tests_pack_element_*` function is defined in
* its own section .cpp and reported by pack_element_tests_runner.cpp.
*
*   WHAT IS TESTED, AND HOW:
*   The module is three declarations - a recursive struct, its index-0
* specialization, and an alias - so the risk is not that some case is
* complicated but that some case is never VISITED.  Two devices below make
* the coverage exhaustive rather than anecdotal:
*
*     1. pe_rebuild walks index_sequence over a pack and reconstructs it
*        through pack_element_t, one index at a time.  Comparing the
*        reconstruction against the original states "every index of this
*        pack answers with its own element" as a SINGLE claim, so the whole
*        26-type adversarial zoo can be checked in one line - void,
*        function, array, unbounded-array, reference, member-pointer and
*        incomplete types included.  pe_rebuild_via_struct does the same
*        through ::type, which is what makes the alias equivalence
*        exhaustive instead of spot-checked, and pe_rebuild_via_tuple does
*        it through std::tuple_element_t as an independent oracle.
*
*     2. pe_tag_list builds a pack of _N distinct indexed types, and
*        pe_indices_match asserts that index K answers with tag K for every
*        K below _N.  That turns recursion DEPTH into a parameter, so the
*        deep cases are stated the same way the shallow ones are.
*
*   THE OUT-OF-RANGE BOUNDARY:
*   The header documents an out-of-range index as a hard error, and it is
* worth being precise about where that error lives, because the two halves
* behave differently and only one of them is testable:
*
*     - NAMING pack_element<9, int> is well formed.  Nothing is
*       instantiated, so nothing fails; pe_is_nameable reports true.
*     - RESOLVING ::type on it instantiates the primary template, whose
*       member names pack_element<8> with an empty tail.  That is "wrong
*       number of template arguments", raised inside an instantiation and
*       therefore a hard error rather than a substitution failure.  It can
*       be neither caught nor probed, so pe_is_resolvable MUST NOT be
*       pointed at an out-of-range index - it would fail the BUILD, not
*       return false.  Section IV states the half that is checkable and
*       documents the half that is not.
*
*   What IS checkable at the boundary is the arity requirement: naming
* pack_element with an empty pack is a genuine substitution failure,
* because too-few-template-arguments is diagnosed during naming rather
* than during instantiation.  And the header's prescribed remedy - guard
* the index before accessing it - is checkable end to end: pe_element_or
* mirrors pack_element_or in bsearch.hpp, and the point of it is that the
* out-of-range branch never NAMES pack_element at all.
*
*
* path:      /tests/djinterp/core/meta/pack_element_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.20
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    check plumbing            (D_PE_CHECK)
      --------------------------------------

II.   element fixtures          (pe_first, pe_tag, pe_list)
      -----------------------------------------------------

III.  rebuild machinery         (pe_rebuild, pe_indices_match)
      --------------------------------------------------------

IV.   boundary probes           (pe_is_nameable, pe_binds_*, pe_element_or)
      ---------------------------------------------------------------------

V.    test declarations
      -----------------
*/

#ifndef DJINTERP_TESTING_PACK_ELEMENT_TESTS_
#define DJINTERP_TESTING_PACK_ELEMENT_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../../inc/djinterp/core/meta/pack_element.hpp"                // the module under test
#include "djinterp/test/test_traits.hpp"   // hostile type zoo + quantifiers


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   check plumbing
// ===========================================================================

// pack_element_check
//   function: records one assertion.  Clears _all on failure and emits a
// line naming the failing expression and its source line, so a red run
// says WHICH claim broke without a debugger.  Returns the outcome so a
// caller may also branch on it.
D_INLINE bool
pack_element_check(
    bool&       _all,
    bool        _ok,
    const char* _expression,
    int         _line
)
{
    // report the failure; the section keeps running so one break does
    // not mask the state of every claim after it
    if (!_ok)
    {
        std::printf("      ! line %d: %s\n", _line, _expression);
        _all = false;
    }

    return _ok;
}

// D_PE_CHECK
//   macro: states one claim about pack_element.hpp.  The condition is
// passed as __VA_ARGS__ so that a template argument list may contain
// commas (`std::is_same_v<a, b>` is one argument here, not two).
#define D_PE_CHECK(_all, ...)                                                 \
    ::djinterp::testing::pack_element_check((_all),                           \
                                            (__VA_ARGS__),                    \
                                            #__VA_ARGS__,                     \
                                            __LINE__)


// ===========================================================================
// II.  element fixtures
// ===========================================================================

// pe_first / pe_second / pe_third / pe_fourth
//   type: four distinct, ordinary element types.  Distinct so that a wrong
// index is a visible failure rather than an accidental match.
struct pe_first
{};

struct pe_second
{};

struct pe_third
{};

struct pe_fourth
{};

// pe_incomplete
//   type: declared and never defined.  pack_element only ever CARRIES its
// elements, so an incomplete type must survive at any position; this is
// the fixture that states it.
struct pe_incomplete;

// pe_tag
//   type: a distinct element type per index, so a positional answer can be
// checked against the very index it came from.  This is what lets recursion
// depth become a parameter rather than a hand-written battery.
template<std::size_t _Number>
struct pe_tag
{
    static constexpr std::size_t value = _Number;
};

// pe_list
//   type: an inert type list.  Used as the comparison shape for a rebuilt
// pack - comparing two pe_list instantiations compares every element and
// its position in one std::is_same.
template<typename... _Types>
struct pe_list
{};


// ===========================================================================
// III. rebuild machinery
// ===========================================================================

// pe_rebuild_helper
//   trait: reconstructs a pack by visiting index 0.._N-1 through three
// independent routes - the alias, the struct's ::type, and
// std::tuple_element_t.  The index_sequence supplies the indices; the pack
// is passed alongside so that the two expansions do not share a pattern.
template<typename    _Seq,
         typename... _Pack>
struct pe_rebuild_helper;

// pe_rebuild_helper (sequence case)
//   trait: the expansion over every index of the pack.
template<std::size_t... _Index,
         typename...    _Pack>
struct pe_rebuild_helper<std::index_sequence<_Index...>, _Pack...>
{
    // via_alias
    //   type: the pack rebuilt through pack_element_t.
    using via_alias = pe_list<pack_element_t<_Index, _Pack...>...>;

    // via_struct
    //   type: the pack rebuilt through pack_element<...>::type.
    using via_struct =
        pe_list<typename pack_element<_Index, _Pack...>::type...>;

    // via_tuple
    //   type: the pack rebuilt through std::tuple_element_t - an
    // independent oracle for the same mapping.
    using via_tuple =
        pe_list<std::tuple_element_t<_Index, std::tuple<_Pack...>>...>;
};

// pe_rebuild
//   trait: pe_rebuild_helper with the index sequence supplied.  Naming
// pe_rebuild<A, B, C>::via_alias and comparing it with pe_list<A, B, C> is
// the whole "every index answers correctly" claim in one expression.
template<typename... _Pack>
using pe_rebuild = pe_rebuild_helper<std::index_sequence_for<_Pack...>,
                                     _Pack...>;

// pe_origin
//   type: the pack in its original order, for comparison against a rebuild.
template<typename... _Pack>
using pe_origin = pe_list<_Pack...>;

// ---------------------------------------------------------------------------
//  one-parameter probes
// ---------------------------------------------------------------------------
//   pe_rebuild states "every index of ONE pack is right".  These state "ONE
// index is right for every type", which is the other axis: they are the
// shape the type-set quantifiers take, so a single line can put all 26
// adversarial types through the same position.

// pe_singleton_holds
//   trait: index 0 of the single-element pack _Type is _Type.  The base
// case, run over every type the zoo can produce.
template<typename _Type>
struct pe_singleton_holds
    : std::is_same<pack_element_t<0, _Type>, _Type>
{};

// pe_after_head_holds
//   trait: index 1 finds _Type sitting behind an ordinary head, so the
// RECURSIVE case carries an arbitrary element intact.
template<typename _Type>
struct pe_after_head_holds
    : std::is_same<pack_element_t<1, pe_first, _Type>, _Type>
{};

// pe_head_survives_tail
//   trait: index 0 still answers with the head when an arbitrary type sits
// behind it - the tail is carried, never inspected.
template<typename _Type>
struct pe_head_survives_tail
    : std::is_same<pack_element_t<0, pe_first, _Type>, pe_first>
{};

// pe_routes_agree
//   trait: true iff ALL THREE rebuild routes - the alias, the struct's
// ::type, and std::tuple_element_t - reproduce the pack exactly, in order.
// This is the strongest single statement available about a whole pack, and
// it lets a pack be spelled once rather than once per route.
template<typename... _Pack>
struct pe_routes_agree
    : std::bool_constant<
        ( std::is_same_v<typename pe_rebuild<_Pack...>::via_alias,
                         pe_origin<_Pack...>>   &&
          std::is_same_v<typename pe_rebuild<_Pack...>::via_struct,
                         pe_origin<_Pack...>>   &&
          std::is_same_v<typename pe_rebuild<_Pack...>::via_tuple,
                         pe_origin<_Pack...>> )>
{};

// pe_at_list
//   trait: indexes into a pe_list rather than a loose pack, so a long pack
// can be built once and then queried.
template<std::size_t _Index,
         typename    _List>
struct pe_at_list;

// pe_at_list (list case)
//   trait: unpacks the list and forwards to the module.
template<std::size_t _Index,
         typename... _Pack>
struct pe_at_list<_Index, pe_list<_Pack...>>
{
    using type = pack_element_t<_Index, _Pack...>;
};

// pe_tag_list_helper
//   trait: builds pe_list<pe_tag<0>, pe_tag<1>, ...> of a chosen length.
template<typename _Seq>
struct pe_tag_list_helper;

// pe_tag_list_helper (sequence case)
//   trait: one tag per index.
template<std::size_t... _Index>
struct pe_tag_list_helper<std::index_sequence<_Index...>>
{
    using type = pe_list<pe_tag<_Index>...>;
};

// pe_tag_list
//   type: a pack of _Count distinct indexed types.
template<std::size_t _Count>
using pe_tag_list =
    typename pe_tag_list_helper<std::make_index_sequence<_Count>>::type;

// pe_indices_match_helper
//   trait: whether index K of _List is pe_tag<K>, for every K in the
// sequence.  The fold has no short circuit, so every index is really
// instantiated.
template<typename _List,
         typename _Seq>
struct pe_indices_match_helper;

// pe_indices_match_helper (sequence case)
//   trait: the fold over every index.
template<typename    _List,
         std::size_t... _Index>
struct pe_indices_match_helper<_List, std::index_sequence<_Index...>>
    : std::bool_constant<
        ( std::is_same_v<typename pe_at_list<_Index, _List>::type,
                         pe_tag<_Index>> && ... )>
{};

// pe_indices_match
//   trait: every index below _Count of a _Count-long tag pack answers with
// its own tag.  This is the depth stress, stated as a parameter.
template<std::size_t _Count>
struct pe_indices_match
    : pe_indices_match_helper<pe_tag_list<_Count>,
                              std::make_index_sequence<_Count>>
{};


// ===========================================================================
// IV.  boundary probes
// ===========================================================================

// pe_is_nameable
//   trait: whether pack_element<_Index, _Pack...> can be NAMED.  Naming
// does not instantiate, so this is true even for an out-of-range index;
// what it does catch is the arity requirement, since too-few-template-
// arguments is diagnosed while naming and is therefore a genuine
// substitution failure.
template<std::size_t _Index,
         typename... _Pack>
struct pe_is_nameable
    : std::bool_constant<requires { typename pack_element<_Index,
                                                          _Pack...>; }>
{};

// pe_is_resolvable
//   trait: whether ::type can be resolved on pack_element<_Index,
// _Pack...>.
//
//   WARNING: resolving ::type INSTANTIATES the primary template, and an
// out-of-range index fails inside that instantiation - a hard error, not a
// substitution failure.  This trait must therefore only ever be pointed at
// an index that is in range, or at a pack so short that naming fails
// first (an empty pack, where the arity check rejects it before any
// instantiation happens).  Pointing it at an out-of-range index would fail
// the BUILD rather than return false.
template<std::size_t _Index,
         typename... _Pack>
struct pe_is_resolvable
    : std::bool_constant<requires { typename pack_element<_Index,
                                                          _Pack...>::type; }>
{};

// pe_binds_indexed
//   trait: true when _Op has the STRUCT's exact template signature - a
// std::size_t index, one required type, then a pack.  Binding a template
// as a template template argument checks its signature without
// instantiating it.
template<template<std::size_t, typename, typename...> class _Op>
struct pe_binds_indexed : std::true_type
{};

// pe_binds_alias
//   trait: true when _Op has the ALIAS's signature - a std::size_t index
// and a pack, with no separately required head.
template<template<std::size_t, typename...> class _Op>
struct pe_binds_alias : std::true_type
{};

// pe_element_or_helper
//   trait: the out-of-range branch of the guarded accessor.  It yields the
// fallback and, crucially, never NAMES pack_element - which is what keeps
// the hard error from firing.
template<bool        _InRange,
         std::size_t _Index,
         typename    _Fallback,
         typename... _Pack>
struct pe_element_or_helper
{
    using type = _Fallback;
};

// pe_element_or_helper (in-range case)
//   trait: the index is known good, so the module may be consulted.
template<std::size_t _Index,
         typename    _Fallback,
         typename... _Pack>
struct pe_element_or_helper<true, _Index, _Fallback, _Pack...>
{
    using type = pack_element_t<_Index, _Pack...>;
};

// pe_element_or
//   type: the guard the header prescribes for a caller that may pass an
// invalid index - the _Index-th element when the pack is long enough, and
// _Fallback otherwise.  Mirrors pack_element_or in bsearch.hpp, and exists
// here so the header's stated remedy is tested rather than assumed.
template<std::size_t _Index,
         typename    _Fallback,
         typename... _Pack>
using pe_element_or =
    typename pe_element_or_helper<(_Index < sizeof...(_Pack)),
                                  _Index,
                                  _Fallback,
                                  _Pack...>::type;


// ===========================================================================
// V.   test declarations
// ===========================================================================

// section I - indexing (pack_element_tests_indexing.cpp)
bool tests_pack_element_base_case();
bool tests_pack_element_recursion();
bool tests_pack_element_trait_shape();

// section II - the alias (pack_element_tests_alias.cpp)
bool tests_pack_element_alias_equivalence();
bool tests_pack_element_alias_arity();

// section III - element fidelity (pack_element_tests_types.cpp)
bool tests_pack_element_type_preservation();
bool tests_pack_element_hostile_pack();
bool tests_pack_element_duplicates();

// section IV - bounds and the guard (pack_element_tests_bounds.cpp)
bool tests_pack_element_arity_contract();
bool tests_pack_element_guarded_access();
bool tests_pack_element_depth();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_PACK_ELEMENT_TESTS_
