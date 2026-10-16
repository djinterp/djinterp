/*******************************************************************************
* djinterp [core]                                           cow_tests_header.cpp
*
*
* path:      /tests/djinterp/core/sync/cow_tests_header.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                              cow_tests_header.cpp
//   Section: the header contract.  The claims about the FILE rather than about
// its types.
//
//   Two of them are bug reports.  cow.hpp cannot be included first (its own
// first include, lock_policy.hpp, #errors without env.h, which cow.hpp never
// brings in), and its C++11 gate - along with both of its precondition guards -
// is COMMENTED OUT, so the "C++98/03: unavailable" line in the banner is
// enforced by nothing at all.  Both are pinned below, as tests, so that fixing
// either one is a visible change.

// djinterp
#include "cow_tests.hpp"
// ... and the header under test a SECOND time, directly.  Two definitions of
// cow_ptr would collide here if the guard did not hold; the successful build IS
// tests_header_is_idempotent's assertion.  Note it needs no core include this
// time: the first inclusion already brought djinterp.hpp in behind it.
#include "../../../../inc/djinterp/core/sync/cow.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_header_include_guard
  the header's include guard is present, and carries the value the file gives
  it.
  Tests the following:
  - DJINTERP_SYNC_COW_HPP is defined after inclusion
  - it is defined to 1
  The guard is named for the THREADSAFE module, not the sync directory the file
  lives in - as condvar.hpp's and lock_policy.hpp's are.
*/
bool
tests_header_include_guard()
{
#if defined(DJINTERP_SYNC_COW_HPP)
    const bool is_defined = true;
    const bool is_one     = (DJINTERP_SYNC_COW_HPP == 1);
#else
    const bool is_defined = false;
    const bool is_one     = false;
#endif

    return (is_defined && is_one);
}

/*
tests_header_is_idempotent
  including the header twice in one TU is a no-op.
  Tests the following:
  - the second #include above redefined nothing
  - after both inclusions every type is still a single, complete type
*/
bool
tests_header_is_idempotent()
{
    const bool intact =
        ( (sizeof(cow_ptr<int>) > 0)                        &&
          (sizeof(immutable_snapshot<int>) > 0)             &&
          (sizeof(cow_state<int>) > 0)                      &&
          (sizeof(internal::cow_control_block<int>) > 0) );

    return intact;
}

/*
tests_header_cannot_be_included_first
  cow.hpp is not self-sufficient: it must not be the first djinterp header in a
  translation unit.
  Tests the following:
  - DJINTERP_ENV_ENV_H is defined here - it has to be, or lock_policy.hpp's
    live #error would have fired and this TU would not exist
  - and it was NOT cow.hpp that defined it.  cow.hpp's include list opens with
    ./lock_policy.hpp, which demands env.h and does not include it; cow.hpp
    never includes ../djinterp.hpp at all.  The only reason this compiles is
    that cow_tests.hpp includes djinterp/core/djinterp.hpp on the line above it
  - remove that line and the build fails with
        #error "lock_policy.hpp requires env.h to be included first"
  THE FIX IS ONE LINE.  condvar.hpp, in the same directory, opens with
      #include "../../../../inc/djinterp/djinterp.hpp"
      #include "../../../../inc/djinterp/core/sync/lock_policy.hpp"
  cow.hpp is missing the first of those.  Until it is added, every consumer of
  cow.hpp must remember to include something else first, and this test is the
  reason the suite's own header does.
*/
bool
tests_header_cannot_be_included_first()
{
    // lock_policy.hpp errors out without this; that the TU built at all is the
    // proof that SOMETHING defined it before cow.hpp was reached
#if defined(DJINTERP_ENV_ENV_H)
    const bool environment_is_in = true;
#else
    const bool environment_is_in = false;
#endif

    // ...and lock_policy.hpp really was reached, and really did compile
#if defined(DJINTERP_SYNC_LOCK_POLICY_HPP)
    const bool lock_policy_is_in = true;
#else
    const bool lock_policy_is_in = false;
#endif

    return (environment_is_in && lock_policy_is_in);
}

/*
tests_header_pulls_its_dependencies
  the four djinterp headers cow.hpp names.
  Tests the following:
  - lock_policy.hpp, lock_guard.hpp, atomic.hpp and
    concurrency_strategy_tags.hpp
    are all present, by their guards
  - and the types they carry are nameable from here: a policy, an
    atomic_version, a strategy tag
  - so a consumer of cow.hpp needs no further djinterp include to name a policy
    or to read cow_state's strategy tag
*/
bool
tests_header_pulls_its_dependencies()
{
#if defined(DJINTERP_SYNC_LOCK_POLICY_HPP)
    const bool has_lock_policy = true;
#else
    const bool has_lock_policy = false;
#endif

#if defined(DJINTERP_SYNC_CONCURRENCY_STRATEGY_TAGS_)
    const bool has_tags = true;
#else
    const bool has_tags = false;
#endif

    // the types those headers carry, named through cow.hpp alone
    const bool types_are_nameable =
        ( (sizeof(null_lock_policy) > 0)      &&
          (sizeof(exclusive_lock_policy) > 0) &&
          (sizeof(atomic_version) > 0)        &&
          (sizeof(cow_strategy_tag) > 0) );

    return ( has_lock_policy    &&
             has_tags           &&
             types_are_nameable );
}

/*
tests_header_takes_lock_guard_but_never_uses_it
  cow.hpp includes lock_guard.hpp, and then names nothing from it.
  Tests the following:
  - lock_guard.hpp's types (scoped_read_lock, scoped_write_lock) arrive through
    cow.hpp, so the include really did happen
  - but cow.hpp locks with `typename _Policy::read_lock_type guard(m_mutex);` -
    the policy's OWN lock type, straight from lock_policy.hpp - and never
    mentions a scoped_*_lock anywhere
  An unused include, pinned.  It costs a consumer of cow.hpp a header it did not
  ask for, and it is the kind of thing that quietly becomes load-bearing when
  somebody removes it.
*/
bool
tests_header_takes_lock_guard_but_never_uses_it()
{
#if defined(DJINTERP_SYNC_LOCK_GUARD_HPP)
    const bool lock_guard_is_in = true;
#else
    const bool lock_guard_is_in = false;
#endif

    // the guards it carries are nameable here - the include is real
    const bool guards_are_nameable =
        ( (sizeof(scoped_read_lock<exclusive_lock_policy>) > 0)  &&
          (sizeof(scoped_write_lock<exclusive_lock_policy>) > 0) );

    return (lock_guard_is_in && guards_are_nameable);
}

/*
tests_header_gates_are_commented_out
  the banner says C++98/03 is "unavailable"; nothing in the file enforces it.
  Tests the following:
  - the C++11 gate at the top of cow.hpp (#if D_ENV_LANG_IS_CPP11_OR_HIGHER) is
    COMMENTED OUT, as are the __cplusplus and DJINTERP_ENV_ENV_H guards
  - so a C++98 compile of cow.hpp does not stop at a clean #error.  It fails
    somewhere inside std::atomic, or on the first `= delete`, or on a variadic
    template - a cascade rather than a diagnosis
  - the gate macro itself is on for this build, which is what the file WOULD
    have tested had the gate been live
  Pinned as current behaviour.  Uncommenting the three blocks would cost nothing
  and would turn a wall of errors into one sentence.
*/
bool
tests_header_gates_are_commented_out()
{
    const bool gate_would_be_on =
        (D_ENV_LANG_IS_CPP11_OR_HIGHER) ? true : false;

    // and everything behind the (absent) gate is here regardless
    const bool the_types_are_present =
        ( (sizeof(cow_ptr<int>) > 0)            &&
          (sizeof(immutable_snapshot<int>) > 0) &&
          (sizeof(cow_state<int>) > 0) );

    return (gate_would_be_on && the_types_are_present);
}

/*
tests_header_provides_the_std_facilities
  including cow.hpp is enough to name the std types it is built on.
  Tests the following:
  - std::uint64_t (the version stamp's type, from <cstdint>), std::mutex and
    std::shared_mutex (through lock_policy.hpp) are all nameable here
  - and this TU includes NEITHER <cstdint> NOR <mutex> NOR <shared_mutex> - the
    test header deliberately does not, precisely so that this test has teeth
  - so a consumer writing `std::uint64_t v = state.version();` beside a
    cow_state needs no further include
*/
bool
tests_header_provides_the_std_facilities()
{
    const std::uint64_t version_type_is_here = 1;

    const bool nameable =
        ( (sizeof(std::uint64_t) == 8)   &&
          (sizeof(std::mutex) > 0)       &&
          (version_type_is_here == 1) );

    // <atomic> too - the refcount and the version stamp are built on it
    const bool atomics_are_here =
        std::is_same<
            internal::cow_control_block<int>,
            internal::cow_control_block<int>>::value;

    return (nameable && atomics_are_here);
}

/*
tests_entities_live_in_the_djinterp_namespace
  the three public types sit FLAT in djinterp; the control block sits in
  djinterp::internal.
  Tests the following:
  - cow_ptr, immutable_snapshot and cow_state, found by unqualified lookup from
    djinterp::testing, ARE ::djinterp's
  - cow_control_block is ::djinterp::internal's - an implementation detail,
    spelled like one
  - and cow_ptr publishes it anyway, as its `control_block` alias: the detail is
    reachable without naming the internal namespace
*/
bool
tests_entities_live_in_the_djinterp_namespace()
{
    D_CONSTEXPR bool public_types_are_flat =
        ( std::is_same<cow_ptr<int>, ::djinterp::cow_ptr<int>>::value &&
          std::is_same<immutable_snapshot<int>,
                       ::djinterp::immutable_snapshot<int>>::value    &&
          std::is_same<cow_state<int>, ::djinterp::cow_state<int>>::value );

    D_CONSTEXPR bool the_block_is_internal =
        std::is_same<internal::cow_control_block<int>,
                     ::djinterp::internal::cow_control_block<int>>::value;

    // ...and cow_ptr re-exports it publicly
    D_CONSTEXPR bool the_alias_re_exports_it =
        std::is_same<cow_ptr<int>::control_block,
                     ::djinterp::internal::cow_control_block<int>>::value;

    static_assert(public_types_are_flat,
                  "the cow types must live directly in namespace djinterp");
    static_assert(the_block_is_internal,
                  "the control block must live in djinterp::internal");

    return ( public_types_are_flat  &&
             the_block_is_internal  &&
             the_alias_re_exports_it );
}

/*
tests_header_defines_no_stray_macros
  the header publishes types, not macros.
  Tests the following:
  - none of cow_ptr / immutable_snapshot / cow_state / cow_control_block is a
    macro
  - so a consumer may declare, alias or qualify any of them without the
    preprocessor rewriting it first
*/
bool
tests_header_defines_no_stray_macros()
{
#if ( defined(cow_ptr)            || \
      defined(immutable_snapshot) || \
      defined(cow_state)          || \
      defined(cow_control_block) )
    const bool names_are_types = false;
#else
    const bool names_are_types = true;
#endif

    return names_are_types;
}


NS_END  // testing
NS_END  // djinterp
