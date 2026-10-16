/*******************************************************************************
* djinterp [core]                                  cow_tests_ptr_detach_swap.cpp
*
*
* path:      /tests/djinterp/core/sync/cow_tests_ptr_detach_swap.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                    cow_tests_ptr_detach_swap.cpp
//   Section II (c): detach() and swap().
//
//   detach() is write()'s unconditional cousin: it clones even when the pointer
// is ALREADY unique, and there is no way to observe that from the value - only
// from the address.  The header explains why it exists: "to guarantee that
// subsequent writes will not be visible through any snapshot taken before
// detach()".  A snapshot taken afterwards shares the NEW block, so an earlier
// snapshot can never be reached by a later write, whatever the refcount
// happened
// to be.
//
//   swap() moves two pointers past each other without touching a refcount at
// all, which is what makes it noexcept.

// djinterp
#include "cow_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_detach_clones_even_when_unique
  detach() reallocates a pointer that had no reason to reallocate.
  Tests the following:
  - a UNIQUE pointer - one that write() would have left alone entirely - comes
    back from detach() at a different address
  - the value survives the move to the new block
  - it is still unique, and still the only owner
  The value cannot tell you this happened.  The address can, and does.
*/
bool
tests_detach_clones_even_when_unique()
{
    probe_reset();

    cow_ptr<tracked> pointer = cow_ptr<tracked>::make(6, 7);

    const tracked* const before = &pointer.read();

    // sanity: write() here would NOT have cloned
    if (!pointer.is_unique())
    {
        return false;
    }

    pointer.detach();

    return ( (&pointer.read() != before)   &&
             (pointer.read().first == 6)   &&
             (pointer.read().second == 7)  &&
             (pointer.use_count() == 1)    &&
             pointer.is_unique() );
}

/*
tests_detach_copies_the_value_exactly_once
  ...and the reallocation costs exactly one copy.
  Tests the following:
  - one copy construction, no moves
  - the OLD block is destroyed, because the detaching pointer held the last
    reference to it - so the live payload count is unchanged: one in, one out
*/
bool
tests_detach_copies_the_value_exactly_once()
{
    probe_reset();

    cow_ptr<tracked> pointer = cow_ptr<tracked>::make(1, 1);

    const long live_before = probe_live().load();

    pointer.detach();

    return ( (probe_copies().load() == 1)       &&
             (probe_moves().load() == 0)        &&
             (probe_destructions().load() == 1) &&
             (probe_live().load() == live_before) );
}

/*
tests_detach_when_shared_leaves_the_others_alone
  on a shared pointer, detach() does exactly what write() would have.
  Tests the following:
  - the detaching pointer takes its own copy and becomes unique
  - the other holder keeps the original block, its address and its value, and is
    unique too
  - one copy, no destructions - the old block still has an owner
*/
bool
tests_detach_when_shared_leaves_the_others_alone()
{
    probe_reset();

    cow_ptr<tracked> holder    = cow_ptr<tracked>::make(2, 3);
    cow_ptr<tracked> detacher  = holder;

    const tracked* const original = &holder.read();

    detacher.detach();

    return ( (probe_copies().load() == 1)       &&
             (probe_destructions().load() == 0) &&
             (&detacher.read() != original)     &&
             (&holder.read() == original)       &&
             (detacher.read().first == 2)       &&
             (holder.read().first == 2)         &&
             detacher.is_unique()               &&
             holder.is_unique() );
}

/*
tests_detach_on_an_empty_pointer_is_a_no_op
  there is nothing to detach from nothing.
  Tests the following:
  - detach() on an empty cow_ptr allocates nothing, copies nothing and does not
    dereference the null block - it is guarded, unlike read() and write()
  - the pointer is still empty afterwards
*/
bool
tests_detach_on_an_empty_pointer_is_a_no_op()
{
    probe_reset();

    cow_ptr<tracked> empty;

    empty.detach();
    empty.detach();

    return ( (!static_cast<bool>(empty))         &&
             (empty.use_count() == 0)            &&
             (probe_constructions().load() == 0) &&
             (probe_copies().load() == 0) );
}

/*
tests_member_swap_exchanges_the_blocks
  swap() exchanges what two pointers own.
  Tests the following:
  - each pointer ends up naming the other's payload, at the other's address
  - an empty pointer can be swapped with a full one: emptiness travels too
*/
bool
tests_member_swap_exchanges_the_blocks()
{
    cow_ptr<tracked> first  = cow_ptr<tracked>::make(1, 1);
    cow_ptr<tracked> second = cow_ptr<tracked>::make(2, 2);

    const tracked* const first_block  = &first.read();
    const tracked* const second_block = &second.read();

    first.swap(second);

    const bool exchanged =
        ( (&first.read() == second_block) &&
          (&second.read() == first_block) &&
          (first.read().first == 2)       &&
          (second.read().first == 1) );

    // emptiness swaps as readily as a block does
    cow_ptr<tracked> empty;

    empty.swap(first);

    return ( exchanged                        &&
             static_cast<bool>(empty)         &&
             (!static_cast<bool>(first))      &&
             (empty.read().first == 2)        &&
             (first.use_count() == 0) );
}

/*
tests_free_swap_is_found_by_adl
  the free swap() is a customization point, and it is found without qualifying
  it.
  Tests the following:
  - an unqualified swap(a, b) on two cow_ptrs, with no using-declaration in
    sight, resolves to djinterp::swap by argument-dependent lookup
  - it does the same thing the member does
  This is what makes cow_ptr work inside any algorithm that follows the
  `using std::swap; swap(a, b);` convention
*/
bool
tests_free_swap_is_found_by_adl()
{
    cow_ptr<tracked> first  = cow_ptr<tracked>::make(10, 0);
    cow_ptr<tracked> second = cow_ptr<tracked>::make(20, 0);

    // unqualified, and no using-declaration: ADL is the only way this resolves
    swap(first, second);

    const bool swapped =
        ( (first.read().first == 20) &&
          (second.read().first == 10) );

    // and the std convention, which is how a generic algorithm would call it
    {
        using std::swap;

        swap(first, second);
    }

    return ( swapped                      &&
             (first.read().first == 10)   &&
             (second.read().first == 20) );
}

/*
tests_swap_touches_no_refcounts
  a swap is two pointer writes, and nothing else.
  Tests the following:
  - swapping two SHARED pointers leaves every refcount exactly where it was
  - no payload is copied, constructed or destroyed
  - which is why it can be noexcept: there is nothing in it that can fail
*/
bool
tests_swap_touches_no_refcounts()
{
    probe_reset();

    cow_ptr<tracked> first        = cow_ptr<tracked>::make(1, 1);
    cow_ptr<tracked> first_sharer = first;
    cow_ptr<tracked> second       = cow_ptr<tracked>::make(2, 2);

    const long copies_before       = probe_copies().load();
    const long destructions_before = probe_destructions().load();

    first.swap(second);

    return ( (probe_copies().load() == copies_before)             &&
             (probe_destructions().load() == destructions_before) &&
             (first.use_count() == 1)                             &&
             (second.use_count() == 2)                            &&
             (first_sharer.use_count() == 2)                      &&
             (&second.read() == &first_sharer.read()) );
}

/*
tests_swap_is_noexcept
  both spellings promise not to throw.
  Tests the following:
  - the member swap and the free swap are both noexcept
  - so a container can use them on an unwind path
*/
bool
tests_swap_is_noexcept()
{
    typedef cow_ptr<tracked> pointer_type;

    D_CONSTEXPR bool member_is_noexcept =
        noexcept(std::declval<pointer_type&>().swap(
                     std::declval<pointer_type&>()));

    D_CONSTEXPR bool free_is_noexcept =
        noexcept(swap(std::declval<pointer_type&>(),
                      std::declval<pointer_type&>()));

    static_assert(member_is_noexcept && free_is_noexcept,
                  "both swaps must be noexcept");

    return (member_is_noexcept && free_is_noexcept);
}


NS_END  // testing
NS_END  // djinterp
