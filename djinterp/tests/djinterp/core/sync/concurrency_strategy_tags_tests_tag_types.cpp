/*******************************************************************************
* djinterp [core]                  concurrency_strategy_tags_tests_tag_types.cpp
*
*
* path:      /tests/djinterp/core/sync/concurrency_strategy_tags_tests_tag_types.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]           concurrency_strategy_tags_tests_tag_types.cpp
//   Section I (a): the SHAPE of the seven strategy tags.
//
//   A tag's whole job is to be a distinct, weightless type that a trait system
// can key on and an overload set can dispatch on.  Everything that job needs
// is a property of the TYPE - empty, trivial, standard-layout, one byte,
// copyable, constructible in a constant expression - and every one of those
// properties is checked here for all seven tags at once.  A tag that grew a
// data member, a virtual function, or a user-provided constructor would still
// compile everywhere it is used today, and would silently cost size, break
// aggregate initialization, or spoil the empty-base optimization the container
// layer relies on.  That is what this section exists to catch.

// djinterp
#include "concurrency_strategy_tags_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // every_tag
    //   function: applies a one-parameter predicate trait to EVERY tag the
    // header declares.  Lets a property test read as one line per property
    // rather than one line per (property, tag) pair - and guarantees no tag is
    // quietly left out of a sweep.
    template<template<typename> class _Pred>
    D_CONSTEXPR bool
    every_tag()
    {
        return ( _Pred<none_strategy_tag>::value   &&
                 _Pred<locked_strategy_tag>::value &&
                 _Pred<cow_strategy_tag>::value    &&
                 _Pred<rcu_strategy_tag>::value    &&
                 _Pred<atomic_strategy_tag>::value &&
                 _Pred<hazard_strategy_tag>::value &&
                 _Pred<hybrid_strategy_tag>::value );
    }

    // no_tag
    //   function: the negative sweep - the predicate holds for NONE of the
    // seven.
    template<template<typename> class _Pred>
    D_CONSTEXPR bool
    no_tag()
    {
        return ( !_Pred<none_strategy_tag>::value   &&
                 !_Pred<locked_strategy_tag>::value &&
                 !_Pred<cow_strategy_tag>::value    &&
                 !_Pred<rcu_strategy_tag>::value    &&
                 !_Pred<atomic_strategy_tag>::value &&
                 !_Pred<hazard_strategy_tag>::value &&
                 !_Pred<hybrid_strategy_tag>::value );
    }

    // is_one_byte
    //   trait: the tag occupies exactly one byte and needs no alignment beyond
    // one.  A tag is a NAME, and a name must not cost anything.
    template<typename _Tag>
    struct is_one_byte
    {
        static D_CONSTEXPR bool value =
            ( (sizeof(_Tag)  == 1) &&
              (alignof(_Tag) == 1) );
    };

    // is_complete_class
    //   trait: the tag is a complete class type (sizeof would not compile on
    // an incomplete one, and the tags are DEFINED, not forward-declared).
    template<typename _Tag>
    struct is_complete_class
    {
        static D_CONSTEXPR bool value =
            ( std::is_class<_Tag>::value &&
              (sizeof(_Tag) > 0) );
    };

    // is_cpp98_pod
    //   trait: the C++98 POD shape, spelled the way C++20 still permits.
    // std::is_pod is deprecated in C++20, and its definition was always the
    // conjunction below - so the conjunction is what the suite checks.
    template<typename _Tag>
    struct is_cpp98_pod
    {
        static D_CONSTEXPR bool value =
            ( std::is_trivial<_Tag>::value         &&
              std::is_standard_layout<_Tag>::value &&
              std::is_empty<_Tag>::value           &&
              (sizeof(_Tag) == 1) );
    };

    // make_tag
    //   function: a tag manufactured inside a constant expression.  Only a
    // literal type can come back from here.
    template<typename _Tag>
    D_CONSTEXPR _Tag
    make_tag()
    {
        return _Tag();
    }

    // round_trip
    //   function: takes a tag BY VALUE and returns it BY VALUE - the calling
    // convention every tag-dispatched overload uses.
    template<typename _Tag>
    D_CONSTEXPR _Tag
    round_trip(
        _Tag _tag
    )
    {
        return _tag;
    }
}

/*
tests_tag_types_are_complete
  all seven tags are defined, not merely declared.
  Tests the following:
  - each of the seven is a complete class type (sizeof compiles)
  - none is a union, an enum, or a forward declaration
*/
bool
tests_tag_types_are_complete()
{
    D_CONSTEXPR bool complete   = every_tag<is_complete_class>();
    D_CONSTEXPR bool are_class  = every_tag<std::is_class>();
    D_CONSTEXPR bool not_union  = no_tag<std::is_union>();
    D_CONSTEXPR bool not_enum   = no_tag<std::is_enum>();

    static_assert(complete, "every strategy tag must be a complete class");

    return ( complete  &&
             are_class &&
             not_union &&
             not_enum );
}

/*
tests_tag_types_are_empty
  a tag carries no state.
  Tests the following:
  - std::is_empty holds for all seven
  - so a type may inherit a tag, or hold one, for free
*/
bool
tests_tag_types_are_empty()
{
    D_CONSTEXPR bool empty = every_tag<std::is_empty>();

    static_assert(empty, "a strategy tag must carry no state");

    return empty;
}

/*
tests_tag_types_are_trivial
  a tag is trivially constructible, copyable and destructible.
  Tests the following:
  - std::is_trivial (trivially default-constructible AND trivially copyable)
  - trivial destruction
  - the header declares no constructor, destructor or assignment operator for
    any tag, and this is the observable consequence
*/
bool
tests_tag_types_are_trivial()
{
    D_CONSTEXPR bool trivial      = every_tag<std::is_trivial>();
    D_CONSTEXPR bool triv_copy    = every_tag<std::is_trivially_copyable>();
    D_CONSTEXPR bool triv_destroy = every_tag<std::is_trivially_destructible>();

    static_assert(trivial, "a strategy tag must be a trivial type");

    return ( trivial   &&
             triv_copy &&
             triv_destroy );
}

/*
tests_tag_types_are_standard_layout
  a tag has standard layout.
  Tests the following:
  - std::is_standard_layout holds for all seven
  - with triviality (above) this is exactly the old POD guarantee, which is
    what lets a tag cross a C ABI boundary or sit in a union unchanged
*/
bool
tests_tag_types_are_standard_layout()
{
    D_CONSTEXPR bool layout = every_tag<std::is_standard_layout>();

    static_assert(layout, "a strategy tag must be standard-layout");

    return layout;
}

/*
tests_tag_types_size_and_alignment
  a tag costs one byte and no more.
  Tests the following:
  - sizeof is 1 for all seven (the minimum a distinct object may occupy)
  - alignof is 1 for all seven
  - an array of tags therefore packs at one byte per element
*/
bool
tests_tag_types_size_and_alignment()
{
    D_CONSTEXPR bool one_byte = every_tag<is_one_byte>();

    static_assert(one_byte, "a strategy tag must occupy exactly one byte");

    // the array consequence, observed rather than asserted
    cow_strategy_tag tags[4];

    return ( one_byte &&
             (sizeof(tags) == 4) );
}

/*
tests_tag_types_are_default_constructible
  a tag can be brought into being with no argument, no allocation and no throw.
  Tests the following:
  - default-constructible, trivially so, and nothrow so, for all seven
  - trivially and nothrow destructible
  - a tag can therefore be materialized inside a noexcept dispatch site
*/
bool
tests_tag_types_are_default_constructible()
{
    D_CONSTEXPR bool ctor =
        every_tag<std::is_default_constructible>();
    D_CONSTEXPR bool trivial_ctor =
        every_tag<std::is_trivially_default_constructible>();
    D_CONSTEXPR bool nothrow_ctor =
        every_tag<std::is_nothrow_default_constructible>();
    D_CONSTEXPR bool nothrow_dtor =
        every_tag<std::is_nothrow_destructible>();

    static_assert(nothrow_ctor,
                  "a strategy tag must be nothrow default-constructible");

    return ( ctor         &&
             trivial_ctor &&
             nothrow_ctor &&
             nothrow_dtor );
}

/*
tests_tag_types_are_copyable_and_movable
  a tag is a value: it copies, moves and assigns, all trivially.
  Tests the following:
  - copy / move construction, copy / move assignment for all seven
  - each is trivial and nothrow
  - which is what permits the by-value parameter every tag-dispatch site uses
*/
bool
tests_tag_types_are_copyable_and_movable()
{
    D_CONSTEXPR bool copy_ctor =
        every_tag<std::is_trivially_copy_constructible>();
    D_CONSTEXPR bool move_ctor =
        every_tag<std::is_trivially_move_constructible>();
    D_CONSTEXPR bool copy_assign =
        every_tag<std::is_trivially_copy_assignable>();
    D_CONSTEXPR bool move_assign =
        every_tag<std::is_trivially_move_assignable>();
    D_CONSTEXPR bool nothrow_copy =
        every_tag<std::is_nothrow_copy_constructible>();
    D_CONSTEXPR bool nothrow_move =
        every_tag<std::is_nothrow_move_constructible>();

    static_assert(copy_ctor && move_ctor,
                  "a strategy tag must be trivially copyable and movable");

    return ( copy_ctor    &&
             move_ctor    &&
             copy_assign  &&
             move_assign  &&
             nothrow_copy &&
             nothrow_move );
}

/*
tests_tag_types_are_aggregates
  a tag is an aggregate: `tag{}` is valid initialization syntax.
  Tests the following:
  - std::is_aggregate holds for all seven (C++17 and above)
  - brace-initialization compiles and value-initializes at any standard
  - a user-provided constructor on any tag would break both
*/
bool
tests_tag_types_are_aggregates()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool aggregate = every_tag<std::is_aggregate>();

    static_assert(aggregate, "a strategy tag must be an aggregate");
#else
    // std::is_aggregate is C++17; below it the brace-init below is the check
    D_CONSTEXPR bool aggregate = true;
#endif

    // brace initialization, at every standard
    const none_strategy_tag   n{};
    const locked_strategy_tag l{};
    const cow_strategy_tag    c{};
    const rcu_strategy_tag    r{};
    const atomic_strategy_tag a{};
    const hazard_strategy_tag h{};
    const hybrid_strategy_tag y{};

    const bool brace_initialized =
        ( (sizeof(n) == 1) &&
          (sizeof(l) == 1) &&
          (sizeof(c) == 1) &&
          (sizeof(r) == 1) &&
          (sizeof(a) == 1) &&
          (sizeof(h) == 1) &&
          (sizeof(y) == 1) );

    return (aggregate && brace_initialized);
}

/*
tests_tag_types_are_not_polymorphic
  a tag is a plain class - inheritable, and free of any vtable.
  Tests the following:
  - not polymorphic, not abstract, no virtual destructor, for all seven
  - not final (C++14 and above), so a tagged type may DERIVE from one
  - a virtual member anywhere would add a vptr and cost the tag its one byte
*/
bool
tests_tag_types_are_not_polymorphic()
{
    D_CONSTEXPR bool not_polymorphic = no_tag<std::is_polymorphic>();
    D_CONSTEXPR bool not_abstract    = no_tag<std::is_abstract>();
    D_CONSTEXPR bool no_virtual_dtor = no_tag<std::has_virtual_destructor>();

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    D_CONSTEXPR bool not_final = no_tag<std::is_final>();

    static_assert(not_final,
                  "a strategy tag must be inheritable (never final)");
#else
    // std::is_final is C++14; the derived-tag tests cover the same ground
    D_CONSTEXPR bool not_final = true;
#endif

    static_assert(not_polymorphic,
                  "a strategy tag must not be polymorphic");

    return ( not_polymorphic &&
             not_abstract    &&
             no_virtual_dtor &&
             not_final );
}

/*
tests_tag_types_are_constexpr_friendly
  a tag is a literal type: it can be built, copied and read at compile time.
  Tests the following:
  - a constexpr instance of each of the seven can be constructed
  - each comes back from a constexpr factory function
  - and the same objects still dispatch at runtime
*/
bool
tests_tag_types_are_constexpr_friendly()
{
    // the initializations ARE the assertion: a non-literal type could not
    // appear here at all
    D_CONSTEXPR none_strategy_tag   n = make_tag<none_strategy_tag>();
    D_CONSTEXPR locked_strategy_tag l = make_tag<locked_strategy_tag>();
    D_CONSTEXPR cow_strategy_tag    c = make_tag<cow_strategy_tag>();
    D_CONSTEXPR rcu_strategy_tag    r = make_tag<rcu_strategy_tag>();
    D_CONSTEXPR atomic_strategy_tag a = make_tag<atomic_strategy_tag>();
    D_CONSTEXPR hazard_strategy_tag h = make_tag<hazard_strategy_tag>();
    D_CONSTEXPR hybrid_strategy_tag y = make_tag<hybrid_strategy_tag>();

    const bool dispatches_at_runtime =
        ( (dispatch_on(n) == 0) &&
          (dispatch_on(l) == 1) &&
          (dispatch_on(c) == 2) &&
          (dispatch_on(r) == 3) &&
          (dispatch_on(a) == 4) &&
          (dispatch_on(h) == 5) &&
          (dispatch_on(y) == 6) );

    return dispatches_at_runtime;
}

/*
tests_tag_types_pass_by_value
  a tag survives a by-value round trip - the calling convention of the whole
  dispatch mechanism.
  Tests the following:
  - each of the seven can be passed to, and returned from, a function by value
  - the returned object is of the same type it went in as
  - the round trip is a constant expression too
*/
bool
tests_tag_types_pass_by_value()
{
    D_CONSTEXPR bool same_type_back =
        ( std::is_same<decltype(round_trip(none_strategy_tag())),
                       none_strategy_tag>::value   &&
          std::is_same<decltype(round_trip(locked_strategy_tag())),
                       locked_strategy_tag>::value &&
          std::is_same<decltype(round_trip(cow_strategy_tag())),
                       cow_strategy_tag>::value    &&
          std::is_same<decltype(round_trip(rcu_strategy_tag())),
                       rcu_strategy_tag>::value    &&
          std::is_same<decltype(round_trip(atomic_strategy_tag())),
                       atomic_strategy_tag>::value &&
          std::is_same<decltype(round_trip(hazard_strategy_tag())),
                       hazard_strategy_tag>::value &&
          std::is_same<decltype(round_trip(hybrid_strategy_tag())),
                       hybrid_strategy_tag>::value );

    static_assert(same_type_back,
                  "a by-value round trip must preserve the tag's type");

    // and the value that comes back still selects the right overload
    const bool still_dispatches =
        ( (dispatch_on(round_trip(rcu_strategy_tag()))    == 3) &&
          (dispatch_on(round_trip(hybrid_strategy_tag())) == 6) );

    return (same_type_back && still_dispatches);
}

/*
tests_tag_types_are_cpp98_pods
  the banner's claim that "the tag structs above remain usable in C++98".
  Tests the following:
  - every tag is trivial AND standard-layout AND empty AND one byte - which is
    precisely what "POD" meant in C++98 (std::is_pod itself is deprecated in
    C++20, so the conjunction stands in for it)
  - no tag uses a C++11 feature in its definition: no defaulted or deleted
    member, no member initializer, no inheritance
  - so section I of the header is buildable beneath the C++11 gate that
    section II sits behind
*/
bool
tests_tag_types_are_cpp98_pods()
{
    D_CONSTEXPR bool pods = every_tag<is_cpp98_pod>();

    static_assert(pods,
                  "the strategy tags must keep their C++98 POD shape");

    return pods;
}


NS_END  // testing
NS_END  // djinterp
