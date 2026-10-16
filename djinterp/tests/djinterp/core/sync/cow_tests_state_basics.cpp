/*******************************************************************************
* djinterp [core]                                     cow_tests_state_basics.cpp
*
*
* path:      /tests/djinterp/core/sync/cow_tests_state_basics.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                       cow_tests_state_basics.cpp
//   Section IV (a): cow_state's SHAPE - what it is before anything mutates it.
//
//   cow_state is the header's payoff: a cow_ptr, a lock policy and a version
// stamp, bolted together into the thing a copy-on-write container is actually
// built from.  This section pins the shape - the aliases it publishes
// (including
// the two that make it legible to the trait layer), the fact that it can be
// neither copied nor moved, and the state it starts in.
//
//   The two aliases matter more than they look.  `cow_state_type` is a
// self-marker, so a trait can ask "is this a cow container?" without knowing
// the
// policy; `concurrency_strategy_tag` is the fast path that
// concurrency_strategy_traits.hpp reads to classify the type as copy-on-write.
// That tag is cow_strategy_tag, from concurrency_strategy_tags.hpp - so this is
// the seam where the two headers meet, and the test that holds it shut.

// djinterp
#include "cow_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_state_default_constructs_its_value
  a default cow_state holds a default-constructed value, already allocated.
  Tests the following:
  - the payload exists after construction - the state is never empty, so its
    read() cannot hit the null-block trap that cow_ptr's can
  - exactly one payload was constructed, and nothing was copied
  - the version starts at 0
*/
bool
tests_state_default_constructs_its_value()
{
    probe_reset();

    {
        cow_state<tracked> state;

        const bool ready =
            ( (probe_constructions().load() == 1) &&
              (probe_copies().load() == 0)        &&
              (probe_live().load() == 1)          &&
              (state.read().first == 0)           &&
              (state.version() == 0) );

        // check if the state failed to bring a value into being
        if (!ready)
        {
            return false;
        }
    }

    return (probe_live().load() == 0);
}

/*
tests_state_constructs_from_an_lvalue
  cow_state(const T&) copies the value in.
  Tests the following:
  - exactly one copy, no moves
  - the state's value is independent of the source
  - the constructor is explicit: a T does not silently become a cow_state
*/
bool
tests_state_constructs_from_an_lvalue()
{
    probe_reset();

    const tracked source(4, 5);

    cow_state<tracked> state(source);

    D_CONSTEXPR bool is_explicit =
        !std::is_convertible<const tracked&, cow_state<tracked> >::value;

    static_assert(is_explicit,
                  "the value constructor must be explicit");

    return ( (probe_copies().load() == 1) &&
             (probe_moves().load() == 0)  &&
             (state.read().first == 4)    &&
             (&state.read() != &source)   &&
             is_explicit );
}

/*
tests_state_constructs_from_an_rvalue
  cow_state(T&&) moves the value in.
  Tests the following:
  - exactly one move, no copies
  - so a heavy initial value costs a move, not a deep copy
*/
bool
tests_state_constructs_from_an_rvalue()
{
    probe_reset();

    cow_state<tracked> state{ tracked(6, 7) };

    return ( (probe_moves().load() == 1)  &&
             (probe_copies().load() == 0) &&
             (state.read().first == 6) );
}

/*
tests_state_is_not_copyable
  a cow_state cannot be copied.
  Tests the following:
  - the copy constructor and copy assignment are deleted
  - and they would have to be: the state OWNS a mutex, and a mutex is not
    copyable.  The header deletes them explicitly rather than leaving the reader
    to work out why the implicit ones vanished
*/
bool
tests_state_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<cow_state<int> >::value &&
          !std::is_copy_assignable<cow_state<int> >::value );

    static_assert(no_copy, "a cow_state must not be copyable");

    return no_copy;
}

/*
tests_state_is_not_movable
  ...and it cannot be moved either, which the header never says.
  Tests the following:
  - is_move_constructible and is_move_assignable are both false
  - for two independent reasons, either of which alone would settle it:
    declaring the deleted copy suppressed the implicit moves, AND the members
    (a mutex, and an atomic_version whose copy is deleted) are not movable
    anyway
  So a cow_state cannot be returned from a factory, stored by value in a vector,
  or held in a type that wants to be movable.  It must be constructed where it
  lives - or held behind a pointer.
*/
bool
tests_state_is_not_movable()
{
    D_CONSTEXPR bool no_move =
        ( !std::is_move_constructible<cow_state<int> >::value &&
          !std::is_move_assignable<cow_state<int> >::value );

    // the version stamp alone would have been enough
    D_CONSTEXPR bool the_version_is_pinned_too =
        !std::is_copy_constructible<atomic_version>::value;

    static_assert(no_move, "a cow_state must not be movable");

    return (no_move && the_version_is_pinned_too);
}

/*
tests_state_exposes_its_policy_aliases
  the state says which policy it was given, and which mutex that implies.
  Tests the following:
  - lock_policy_type is the policy the state was instantiated with
  - mutex_type is that policy's mutex_type, forwarded
  - so a container built on cow_state can name its own mutex without repeating
    the policy
*/
bool
tests_state_exposes_its_policy_aliases()
{
    D_CONSTEXPR bool policy_is_remembered =
        ( std::is_same<cow_state<int, exclusive_lock_policy>::lock_policy_type,
                       exclusive_lock_policy>::value &&
          std::is_same<cow_state<int, null_lock_policy>::lock_policy_type,
                       null_lock_policy>::value );

    D_CONSTEXPR bool mutex_is_forwarded =
        ( std::is_same<cow_state<int, exclusive_lock_policy>::mutex_type,
                       exclusive_lock_policy::mutex_type>::value &&
          std::is_same<cow_state<int, null_lock_policy>::mutex_type,
                       no_op_mutex>::value );

    static_assert(policy_is_remembered && mutex_is_forwarded,
                  "cow_state must publish its policy and its mutex type");

    return (policy_is_remembered && mutex_is_forwarded);
}

/*
tests_state_marks_itself_with_cow_state_type
  the self-marker a trait probes for.
  Tests the following:
  - cow_state<T, P>::cow_state_type IS cow_state<T, P> - the type names itself
  - so `has_cow_state_type<X>` answers true for a cow_state, and for any
    container that forwards the alias, without either of them naming the other
  - the marker survives a change of policy: each instantiation names its own
    self
*/
bool
tests_state_marks_itself_with_cow_state_type()
{
    D_CONSTEXPR bool names_itself =
        ( std::is_same<cow_state<int>::cow_state_type,
                       cow_state<int> >::value &&
          std::is_same<cow_state<tracked,
          exclusive_lock_policy>::cow_state_type,
                       cow_state<tracked, exclusive_lock_policy> >::value );

    // and two different instantiations are two different markers
    D_CONSTEXPR bool the_marker_is_specific =
        !std::is_same<cow_state<int, null_lock_policy>::cow_state_type,
                      cow_state<int, exclusive_lock_policy> >::value;

    static_assert(names_itself,
                  "cow_state's self-marker must name cow_state");

    return (names_itself && the_marker_is_specific);
}

/*
tests_state_declares_the_cow_strategy_tag
  the seam between this header and concurrency_strategy_tags.hpp.
  Tests the following:
  - cow_state::concurrency_strategy_tag IS cow_strategy_tag - not rcu, not
    locked, not a tag of its own invention
  - the alias is PUBLIC, which it must be: the trait layer's fast path reads it
    by name, and a private alias would be invisible to it
  - it holds for every policy: a cow_state is copy-on-write whatever mutex it
    happens to hold
  This is the whole of what cow.hpp owes the strategy layer, and the only line
  that ties them together.
*/
bool
tests_state_declares_the_cow_strategy_tag()
{
    D_CONSTEXPR bool tagged_cow =
        ( std::is_same<cow_state<int>::concurrency_strategy_tag,
                       cow_strategy_tag>::value &&
          std::is_same<
              cow_state<int, null_lock_policy>::concurrency_strategy_tag,
              cow_strategy_tag>::value          &&
          std::is_same<
              cow_state<tracked,
              exclusive_lock_policy>::concurrency_strategy_tag,
              cow_strategy_tag>::value );

    D_CONSTEXPR bool not_some_other_tag =
        ( !std::is_same<cow_state<int>::concurrency_strategy_tag,
                        locked_strategy_tag>::value &&
          !std::is_same<cow_state<int>::concurrency_strategy_tag,
                        rcu_strategy_tag>::value    &&
          !std::is_same<cow_state<int>::concurrency_strategy_tag,
                        none_strategy_tag>::value );

    static_assert(tagged_cow,
                  "cow_state must declare the cow strategy tag");

    return (tagged_cow && not_some_other_tag);
}

/*
tests_state_read_returns_the_value
  read() hands back the current value, under a read lock.
  Tests the following:
  - the value is the one the state was constructed with
  - the return is a CONST reference: read() offers no mutable path
  - reading copies nothing
  - and read() is CONST, so it can be called on a const cow_state - which is why
    the mutex is `mutable`
*/
bool
tests_state_read_returns_the_value()
{
    probe_reset();

    const cow_state<tracked> state(tracked(1, 2));

    const long copies_after_construction = probe_copies().load();

    D_CONSTEXPR bool returns_const_reference =
        std::is_same<decltype(std::declval<const cow_state<tracked>&>().read()),
                     const tracked&>::value;

    static_assert(returns_const_reference,
                  "read() must hand back a const reference");

    const tracked& value = state.read();

    return ( returns_const_reference &&
             (value.first == 1)      &&
             (value.second == 2)     &&
             (probe_copies().load() == copies_after_construction) );
}

/*
tests_state_version_starts_at_zero
  a fresh state has never been mutated, and says so.
  Tests the following:
  - version() is 0 immediately after every constructor
  - it is a std::uint64_t, and const, and noexcept
  - so a reader that caches version 0 will see its first change on the first
    write
*/
bool
tests_state_version_starts_at_zero()
{
    cow_state<int> defaulted;
    cow_state<int> from_value(7);
    cow_state<int> from_rvalue(int(8));

    D_CONSTEXPR bool is_noexcept =
        noexcept(std::declval<const cow_state<int>&>().version());

    D_CONSTEXPR bool is_64_bit =
        std::is_same<decltype(std::declval<const cow_state<int>&>().version()),
                     std::uint64_t>::value;

    static_assert(is_64_bit, "the version must be a uint64");

    return ( (defaulted.version() == 0)   &&
             (from_value.version() == 0)  &&
             (from_rvalue.version() == 0) &&
             is_noexcept                  &&
             is_64_bit );
}

/*
tests_state_exposes_its_mutex
  the state hands out the mutex it locks with.
  Tests the following:
  - mutex() returns a NON-const reference to the policy's mutex, even from a
    CONST cow_state - the member is `mutable`, which is what lets read() and
    snapshot() lock at all
  - it is noexcept
  - so a container built on cow_state can take the same lock, for an operation
    the state does not offer - and can also, of course, deadlock itself
*/
bool
tests_state_exposes_its_mutex()
{
    typedef cow_state<int, exclusive_lock_policy> state_type;

    const state_type state(1);

    D_CONSTEXPR bool hands_out_a_mutable_mutex =
        std::is_same<decltype(std::declval<const state_type&>().mutex()),
                     state_type::mutex_type&>::value;

    D_CONSTEXPR bool is_noexcept =
        noexcept(std::declval<const state_type&>().mutex());

    static_assert(hands_out_a_mutable_mutex,
                   "mutex() must hand out a mutable reference from a "
                   "const state");

    // it is the real mutex, and it is unlocked right now
    const bool lockable = state.mutex().try_lock();

    if (lockable)
    {
        state.mutex().unlock();
    }

    return ( hands_out_a_mutable_mutex &&
             is_noexcept               &&
             lockable );
}

/*
tests_state_defaults_to_the_default_policy
  cow_state<T> is cow_state<T, default_lock_policy>.
  Tests the following:
  - the second template parameter defaults to default_lock_policy
  - so an unqualified cow_state<T> gets the most capable policy the standard
    allows: shared at C++17 and above, exclusive below
*/
bool
tests_state_defaults_to_the_default_policy()
{
    D_CONSTEXPR bool defaulted =
        std::is_same<cow_state<int>,
                     cow_state<int, default_lock_policy> >::value;

    static_assert(defaulted,
                  "cow_state must default to default_lock_policy");

    return defaulted;
}


NS_END  // testing
NS_END  // djinterp
