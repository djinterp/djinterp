/*******************************************************************************
* djinterp [core]                                    cow_tests_control_block.cpp
*
*
* path:      /tests/djinterp/core/sync/cow_tests_control_block.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                       cow_tests_control_block.cpp
//   Section I: internal::cow_control_block - the refcount and the value, in one
// allocation.
//
//   The header's design note is a claim about MEMORY: the control block is
// intrusive, "embedded directly in the cow allocation alongside the managed
// object, avoiding the double allocation of shared_ptr".  That is not a claim a
// value can confirm - so the tests below confirm it by address: the value lives
// INSIDE the block, at a fixed offset, and one `new` produced both.
//
//   The rest of the section pins the refcount protocol the whole header rests
// on: a block is born at ONE, add_ref raises it, and release() returns TRUE
// exactly once - to the caller who took the last reference and must therefore
// delete the block.  Every leak and every double-free in a refcounted type is a
// misreading of that one boolean.

// djinterp
#include "cow_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    typedef internal::cow_control_block<int>     int_block;
    typedef internal::cow_control_block<tracked> tracked_block;
}

/*
tests_control_block_starts_at_one
  a fresh block is born owned.
  Tests the following:
  - a newly constructed block has a refcount of 1, not 0
  - so the cow_ptr that created it already holds the reference it will release
*/
bool
tests_control_block_starts_at_one()
{
    int_block* block = new int_block(5);

    const bool born_at_one =
        ( (block->use_count() == 1) &&
          block->is_unique()        &&
          (block->value == 5) );

    // the creator holds the only reference, so it is the one who deletes it
    if (block->release())
    {
        delete block;
    }

    return born_at_one;
}

/*
tests_control_block_add_ref
  add_ref raises the count.
  Tests the following:
  - each add_ref raises use_count by exactly one
  - the block stops being unique the moment a second reference exists
*/
bool
tests_control_block_add_ref()
{
    int_block* block = new int_block(1);

    block->add_ref();
    block->add_ref();

    const bool raised =
        ( (block->use_count() == 3) &&
          (!block->is_unique()) );

    // give back the two we took, then the one it was born with
    (void)block->release();
    (void)block->release();

    if (block->release())
    {
        delete block;
    }

    return raised;
}

/*
tests_control_block_release_reports_the_last_reference
  release() answers exactly one question: "was that the last one?"
  Tests the following:
  - releasing a block whose count is 2 returns FALSE - somebody else still holds
    it, and deleting it now would be a use-after-free for them
  - releasing it again, at 1, returns TRUE - the caller is the last owner and
    must delete
  - the count falls by one each time
  Every leak in a refcounted type is a TRUE that was ignored; every double free
  is a FALSE that was not.
*/
bool
tests_control_block_release_reports_the_last_reference()
{
    int_block* block = new int_block(7);

    block->add_ref();

    const bool not_the_last  = (!block->release());
    const bool down_to_one   = (block->use_count() == 1);
    const bool now_the_last  = block->release();

    // it said TRUE, so it is ours to delete
    delete block;

    return ( not_the_last &&
             down_to_one  &&
             now_the_last );
}

/*
tests_control_block_is_unique
  is_unique is "the count is exactly one".
  Tests the following:
  - true at one, false at two, true again once the second reference goes
  - this is the predicate cow_ptr::write() consults before deciding to clone, so
    an off-by-one here would either clone on every write or never clone at all
*/
bool
tests_control_block_is_unique()
{
    int_block* block = new int_block(1);

    const bool unique_at_one = block->is_unique();

    block->add_ref();

    const bool shared_at_two = (!block->is_unique());

    (void)block->release();

    const bool unique_again = block->is_unique();

    if (block->release())
    {
        delete block;
    }

    return ( unique_at_one &&
             shared_at_two &&
             unique_again );
}

/*
tests_control_block_refcount_ops_are_noexcept
  none of the refcount operations may throw.
  Tests the following:
  - add_ref, release, is_unique and use_count are all noexcept
  - which is what lets cow_ptr's copy constructor, copy assignment, move and
    DESTRUCTOR be noexcept in turn.  A throwing release() in a destructor would
    terminate the process
*/
bool
tests_control_block_refcount_ops_are_noexcept()
{
    D_CONSTEXPR bool all_noexcept =
        ( noexcept(std::declval<int_block&>().add_ref())        &&
          noexcept(std::declval<int_block&>().release())        &&
          noexcept(std::declval<const int_block&>().is_unique()) &&
          noexcept(std::declval<const int_block&>().use_count()) );

    static_assert(all_noexcept,
                  "the refcount operations must be noexcept");

    return all_noexcept;
}

/*
tests_control_block_holds_the_value_inline
  ONE allocation, not two - the whole reason the block is intrusive.
  Tests the following:
  - the value lives INSIDE the block: its address falls within the block's own
    storage
  - the block is at least as big as an atomic count plus the value
  - the refcount is a std::atomic<std::size_t>, not a plain integer
  std::shared_ptr's control block is a second allocation, reached through a
  pointer.  Here there is no pointer to follow and nothing else to free, which
  is what the design note claims and what these addresses show.
*/
bool
tests_control_block_holds_the_value_inline()
{
    int_block* block = new int_block(3);

    const char* const block_start = reinterpret_cast<const char*>(block);
    const char* const value_at    =
        reinterpret_cast<const char*>(&block->value);

    const bool value_is_inside =
        ( (value_at >= block_start) &&
          ((value_at + sizeof(int)) <=
           (block_start + sizeof(int_block))) );

    const bool big_enough =
        (sizeof(int_block) >=
         (sizeof(std::atomic<std::size_t>) + sizeof(int)));

    D_CONSTEXPR bool the_count_is_atomic =
        std::is_same<decltype(int_block::refcount),
                     std::atomic<std::size_t>>::value;

    static_assert(the_count_is_atomic,
                  "the refcount must be atomic");

    if (block->release())
    {
        delete block;
    }

    return ( value_is_inside &&
             big_enough      &&
             the_count_is_atomic );
}

/*
tests_control_block_constructs_the_value_in_place
  the variadic constructor builds the value where it will live.
  Tests the following:
  - a block built from two arguments constructs the payload ONCE, in place: no
    temporary, no copy, no move
  - the arguments arrive in order
*/
bool
tests_control_block_constructs_the_value_in_place()
{
    probe_reset();

    tracked_block* block = new tracked_block(3, 4);

    const bool built_in_place =
        ( (probe_constructions().load() == 1) &&
          (probe_copies().load() == 0)        &&
          (probe_moves().load() == 0)         &&
          (block->value.first == 3)           &&
          (block->value.second == 4) );

    if (block->release())
    {
        delete block;
    }

    const bool nothing_leaked = (probe_live().load() == 0);

    return (built_in_place && nothing_leaked);
}

/*
tests_control_block_copies_an_lvalue
  a block built from a const lvalue copies it, exactly once.
  Tests the following:
  - one copy construction, no moves
  - the copy is the block's own, independent of the source
*/
bool
tests_control_block_copies_an_lvalue()
{
    probe_reset();

    const tracked source(1, 2);

    tracked_block* block = new tracked_block(source);

    const bool copied_once =
        ( (probe_copies().load() == 1) &&
          (probe_moves().load() == 0)  &&
          (block->value.first == 1)    &&
          (&block->value != &source) );

    if (block->release())
    {
        delete block;
    }

    return copied_once;
}

/*
tests_control_block_moves_an_rvalue
  a block built from an rvalue moves it, exactly once.
  Tests the following:
  - one move construction, no copies - the _Type&& overload was chosen over the
    variadic one
  - so a heavy payload can be handed to a cow_ptr without being copied
*/
bool
tests_control_block_moves_an_rvalue()
{
    probe_reset();

    tracked_block* block = new tracked_block(tracked(8, 9));

    const bool moved_once =
        ( (probe_moves().load() == 1)  &&
          (probe_copies().load() == 0) &&
          (block->value.first == 8) );

    if (block->release())
    {
        delete block;
    }

    return moved_once;
}

/*
tests_control_block_lives_in_internal
  it is an implementation detail, spelled like one - and re-exported anyway.
  Tests the following:
  - cow_control_block is ::djinterp::internal's
  - and cow_ptr<T>::control_block is a PUBLIC alias to it, so a consumer can
    name the detail without ever writing `internal::`
  Pinned rather than judged: the alias is what makes the block testable from
  here, and also what makes it part of cow_ptr's public surface whether or not
  that was the intent.
*/
bool
tests_control_block_lives_in_internal()
{
    D_CONSTEXPR bool in_internal =
        std::is_same<internal::cow_control_block<int>,
                     ::djinterp::internal::cow_control_block<int>>::value;

    D_CONSTEXPR bool publicly_aliased =
        std::is_same<cow_ptr<int>::control_block,
                     internal::cow_control_block<int>>::value;

    static_assert(in_internal && publicly_aliased,
                  "the control block is internal, and cow_ptr re-exports it");

    return (in_internal && publicly_aliased);
}


NS_END  // testing
NS_END  // djinterp
