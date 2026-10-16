/*******************************************************************************
* djinterp [core]                                hazard_pointer_tests_record.cpp
*
*
* path:      /tests/djinterp/core/sync/hazard_pointer_tests_record.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                    hazard_pointer_tests_record.cpp
//   Section II: hazard_record - one atomic slot for one protected pointer.
//
//   The record is the atom of the whole scheme: a void* that a reader publishes
// and a scanner reads, plus an active flag that says whether the slot is
// claimed.  Both are atomic because they cross threads with no lock between
// them.  There is almost nothing to it, which is the point - the tests here fix
// the default state (inactive, null), the two members' types, and that a record
// cannot be copied out from under the atomics that make it work.
//
//   Everything in this section is gated on D_HAS_CUSTOM_HAZARD: under C++23
// with
// <hazard_pointer> present, hazard_record does not exist, and these tests state
// that and pass.

// djinterp
#include "hazard_pointer_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_record_default_is_inactive_and_null
  a fresh record protects nothing and is not claimed.
  Tests the following:
  - protected_ptr is null and active is false immediately after construction
  - so a record array, freshly allocated, is entirely idle: no slot claims to
    protect anything, and a scan over it finds nothing
  - the default constructor is noexcept
*/
bool
tests_record_default_is_inactive_and_null()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_record record;

    D_CONSTEXPR bool ctor_is_noexcept =
        std::is_nothrow_default_constructible<hazard_record>::value;

    static_assert(ctor_is_noexcept,
                  "hazard_record's default constructor must be noexcept");

    return ( (record.protected_ptr.load() == nullptr) &&
             (record.active.load() == false)          &&
             ctor_is_noexcept );
#else
    return true;
#endif
}

/*
tests_record_holds_two_atomics
  the record is exactly its two atomic members.
  Tests the following:
  - protected_ptr is a std::atomic<void*> and active is a std::atomic<bool>
  - the struct is at least as big as those two atomics together
  - it is a plain aggregate-like struct with no vtable
*/
bool
tests_record_holds_two_atomics()
{
#if D_HAS_CUSTOM_HAZARD
    D_CONSTEXPR bool members_are_atomic =
        ( std::is_same<decltype(hazard_record::protected_ptr),
                       std::atomic<void*> >::value &&
          std::is_same<decltype(hazard_record::active),
                       std::atomic<bool> >::value );

    const bool big_enough =
        (sizeof(hazard_record) >=
         (sizeof(std::atomic<void*>) + sizeof(std::atomic<bool>)));

    D_CONSTEXPR bool no_vtable =
        !std::is_polymorphic<hazard_record>::value;

    static_assert(members_are_atomic && no_vtable,
                  "hazard_record must be two atomics and no vtable");

    return (members_are_atomic && big_enough && no_vtable);
#else
    return true;
#endif
}

/*
tests_record_pointer_is_a_void_pointer
  the protected slot is type-erased.
  Tests the following:
  - protected_ptr holds a void*, not a T* - the domain protects RAW addresses,
    and the type only comes back at the typed_hazard_ptr layer
  - which is why one domain can protect nodes of any type, and why is_protected
    takes a void*
*/
bool
tests_record_pointer_is_a_void_pointer()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_record record;

    int          object = 0;
    void* const  address = static_cast<void*>(&object);

    record.protected_ptr.store(address);

    D_CONSTEXPR bool value_type_is_void_ptr =
        std::is_same<decltype(record.protected_ptr.load()), void*>::value;

    static_assert(value_type_is_void_ptr,
                  "the protected pointer must be type-erased to void*");

    return ( value_type_is_void_ptr &&
             (record.protected_ptr.load() == address) );
#else
    return true;
#endif
}

/*
tests_record_is_not_copyable
  a record cannot be copied.
  Tests the following:
  - hazard_record is not copy-constructible - std::atomic deletes its own copy,
    and that propagates to the aggregate
  - which is right: a record is shared BY ADDRESS between a reader and a
    scanner, and a copy would be a second slot nobody is looking at
*/
bool
tests_record_is_not_copyable()
{
#if D_HAS_CUSTOM_HAZARD
    D_CONSTEXPR bool no_copy =
        !std::is_copy_constructible<hazard_record>::value;

    static_assert(no_copy, "hazard_record must not be copyable");

    return no_copy;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
