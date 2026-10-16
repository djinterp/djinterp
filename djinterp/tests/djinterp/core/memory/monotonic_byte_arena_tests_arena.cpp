/*******************************************************************************
* djinterp [testing]                        monotonic_byte_arena_tests_arena.cpp
*
* Section: the byte arenas and their alignment helper.
*   monotonic_byte_arena.hpp - align_up, monotonic_byte_arena<Bytes, MaxAlign>,
*                              dynamic_byte_arena, byte_arena_strategy_c
*
*   Two byte-typed monotonic bump arenas that differ only in where the bytes
* live: monotonic_byte_arena holds them in-object (static storage), while
* dynamic_byte_arena owns a heap block whose size is fixed at construction
* (fixed storage).  Both are tested for the same runtime behaviour - aligned,
* contiguous, bounded allocation with throw-on-overflow and wholesale reset -
* and the one contract that separates them, their storage kind.
*   align_up is the arithmetic under both, so it is tested first and directly:
* rounding up to a power-of-two boundary, including the two boundary cases that
* matter (already-aligned stays put; alignment of 1 is the identity).  The
* arenas are byte-typed, so the recognition CONCEPT (byte + monotonic) accepts
* them and rejects a releasing byte strategy - being an arena is about the
* discipline, not the class.  N is kept positive: a zero-byte arena is an
* ill-formed array and out of scope.
*
*
* path:      /tests/djinterp/core/memory/monotonic_byte_arena_tests_arena.cpp
*                                           memory_strategy_tests_arena.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
// djinterp
#include "monotonic_byte_arena_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_align_up_helper
  internal::align_up(n, align): round n up to the next multiple of align.
  Tests the following:
  - rounding up: a value between boundaries goes to the next boundary
  - an already-aligned value is unchanged (the idempotent case)
  - alignment of 1 is the identity for every n
  - zero rounds to zero at any alignment
  - a range of alignments (2, 4, 8, 16) behaves consistently
*/
bool
tests_align_up_helper()
{

    // --- rounding up to the next boundary ---
    D_BA_CHECK(internal::align_up(1, 8) == 8);
    D_BA_CHECK(internal::align_up(7, 8) == 8);
    D_BA_CHECK(internal::align_up(9, 8) == 16);
    D_BA_CHECK(internal::align_up(5, 4) == 8);
    D_BA_CHECK(internal::align_up(3, 2) == 4);

    // --- already aligned: unchanged ---
    D_BA_CHECK(internal::align_up(8, 8) == 8);
    D_BA_CHECK(internal::align_up(16, 8) == 16);
    D_BA_CHECK(internal::align_up(4, 4) == 4);

    // --- alignment of 1 is the identity ---
    D_BA_CHECK(internal::align_up(0, 1) == 0);
    D_BA_CHECK(internal::align_up(5, 1) == 5);
    D_BA_CHECK(internal::align_up(1000, 1) == 1000);

    // --- zero rounds to zero ---
    D_BA_CHECK(internal::align_up(0, 8) == 0);
    D_BA_CHECK(internal::align_up(0, 16) == 0);

    // --- a range of alignments ---
    D_BA_CHECK(internal::align_up(1, 16) == 16);
    D_BA_CHECK(internal::align_up(17, 16) == 32);
    D_BA_CHECK(internal::align_up(2, 2) == 2);
    D_BA_CHECK(internal::align_up(3, 4) == 4);

    return true;
}


/*
tests_monotonic_arena_contract
  monotonic_byte_arena<Bytes, MaxAlign>: descriptive surface and properties.
  Tests the following:
  - value_type is void, and the declared discipline is static + pointer-stable +
    NOT individually releasing
  - it is byte-typed (NOT element: the void value_type fails the element guard),
    a member, allocating, monotonic, and static
  - it is default-constructible and NON-COPYABLE
  - cv-ref agnostic through the composites
*/
bool
tests_monotonic_arena_contract()
{

    using arena = monotonic_byte_arena<256>;

    // --- aliases and declared discipline ---
    D_BA_CHECK((std::is_same<arena::value_type, void>::value));
    D_BA_CHECK(arena::strategy_storage_kind == storage_kind::static_storage);
    D_BA_CHECK(arena::pointer_stable);
    D_BA_CHECK(!arena::supports_individual_release);

    // --- byte-typed, not element (void guard), member, monotonic, static ---
    D_BA_CHECK(is_byte_strategy<arena>::value);
    D_BA_CHECK(!is_element_strategy<arena>::value);
    D_BA_CHECK(is_memory_strategy<arena>::value);
    D_BA_CHECK(is_allocating_strategy<arena>::value);
    D_BA_CHECK(is_monotonic_strategy<arena>::value);
    D_BA_CHECK(!supports_individual_release_strategy<arena>::value);
    D_BA_CHECK(is_static_strategy<arena>::value);
    D_BA_CHECK(is_pointer_stable_strategy<arena>::value);
    D_BA_CHECK(strategy_typing_of<arena>::value
                   == strategy_typing::byte_typed);

    // --- constructibility ---
    D_BA_CHECK(std::is_default_constructible<arena>::value);
    D_BA_CHECK(!std::is_copy_constructible<arena>::value);
    D_BA_CHECK(!std::is_copy_assignable<arena>::value);

    // --- cv-ref agnostic ---
    D_BA_CHECK(is_byte_strategy<const arena&>::value);
    D_BA_CHECK(is_monotonic_strategy<arena&&>::value);
    D_BA_CHECK(memory_strategy_kind_of<const arena&>::value
                   == storage_kind::static_storage);

    return true;
}


/*
tests_monotonic_arena_runtime
  monotonic_byte_arena<Bytes, MaxAlign>: aligned, bounded bump allocation.
  Tests the following:
  - a fresh arena reports capacity Bytes and zero in use
  - allocate(bytes) from offset zero is non-null and maximally aligned, and
    advances usage by exactly the request
  - a second allocate with a larger alignment introduces the expected padding:
    the returned pointer is aligned and sits past the first block
  - the storage is writable and reads back
  - deallocate is a no-op (usage unchanged)
  - an allocation that exactly fills the arena succeeds; one more byte throws
  - reset returns usage to zero and the capacity is available again
*/
bool
tests_monotonic_arena_runtime()
{

    monotonic_byte_arena<256> a;

    // --- fresh state ---
    D_BA_CHECK(a.bytes_capacity() == 256);
    D_BA_CHECK(a.bytes_in_use() == 0);

    // --- first block: aligned, exact cost ---
    void* p = a.allocate(10);
    D_BA_CHECK(p != nullptr);
    D_BA_CHECK(reinterpret_cast<std::uintptr_t>(p) % 16 == 0);
    D_BA_CHECK(a.bytes_in_use() == 10);

    // --- second block: 16-aligned, padded past the first ---
    void* q = a.allocate(10, 16);
    D_BA_CHECK(reinterpret_cast<std::uintptr_t>(q) % 16 == 0);
    D_BA_CHECK(static_cast<unsigned char*>(q)
                   == static_cast<unsigned char*>(p) + 16);
    D_BA_CHECK(a.bytes_in_use() == 26);

    // --- writable, reads back ---
    unsigned char* b = static_cast<unsigned char*>(p);
    b[0] = 0xEE;
    b[9] = 0x11;
    D_BA_CHECK(b[0] == 0xEE && b[9] == 0x11);

    // --- deallocate is a no-op ---
    a.deallocate(p, 10);
    D_BA_CHECK(a.bytes_in_use() == 26);

    // --- exact fill succeeds, one more byte throws ---
    monotonic_byte_arena<256> full;
    void* whole = full.allocate(256);
    D_BA_CHECK(whole != nullptr);
    D_BA_CHECK(full.bytes_in_use() == 256);
    bool threw = false;
    try
    {
        full.allocate(1, 1);
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }
    D_BA_CHECK(threw);

    // --- reset reclaims wholesale ---
    a.reset();
    D_BA_CHECK(a.bytes_in_use() == 0);
    void* again = a.allocate(100);
    D_BA_CHECK(again != nullptr);
    D_BA_CHECK(a.bytes_in_use() == 100);

    // --- overflow-safe bounds: a request so large that aligned + bytes would
    //     wrap still throws, and does not advance the head ---
    monotonic_byte_arena<256> ov;
    bool ov_threw = false;
    try
    {
        ov.allocate(SIZE_MAX, 1);
    }
    catch (const std::bad_alloc&)
    {
        ov_threw = true;
    }
    D_BA_CHECK(ov_threw);
    D_BA_CHECK(ov.bytes_in_use() == 0);

    return true;
}


/*
tests_dynamic_arena_contract
  dynamic_byte_arena: descriptive surface and properties.
  Tests the following:
  - value_type is void, and the declared discipline is FIXED (not static:
    heap-owned but non-growable) + pointer-stable + not releasing
  - it is byte-typed, a member, allocating, monotonic, and fixed
  - it is NON-COPYABLE and NOT default-constructible (the size is a constructor
    argument)
  - cv-ref agnostic through the composites
*/
bool
tests_dynamic_arena_contract()
{

    // --- aliases and declared discipline ---
    D_BA_CHECK((std::is_same<dynamic_byte_arena::value_type, void>::value));
    D_BA_CHECK(dynamic_byte_arena::strategy_storage_kind
                   == storage_kind::fixed_storage);
    D_BA_CHECK(dynamic_byte_arena::pointer_stable);
    D_BA_CHECK(!dynamic_byte_arena::supports_individual_release);

    // --- byte-typed, member, monotonic, fixed ---
    D_BA_CHECK(is_byte_strategy<dynamic_byte_arena>::value);
    D_BA_CHECK(!is_element_strategy<dynamic_byte_arena>::value);
    D_BA_CHECK(is_memory_strategy<dynamic_byte_arena>::value);
    D_BA_CHECK(is_allocating_strategy<dynamic_byte_arena>::value);
    D_BA_CHECK(is_monotonic_strategy<dynamic_byte_arena>::value);
    D_BA_CHECK(is_fixed_strategy<dynamic_byte_arena>::value);
    D_BA_CHECK(!is_static_strategy<dynamic_byte_arena>::value);
    D_BA_CHECK(is_pointer_stable_strategy<dynamic_byte_arena>::value);

    // --- constructibility ---
    D_BA_CHECK(!std::is_default_constructible<dynamic_byte_arena>::value);
    D_BA_CHECK(!std::is_copy_constructible<dynamic_byte_arena>::value);
    D_BA_CHECK(!std::is_copy_assignable<dynamic_byte_arena>::value);

    // --- cv-ref agnostic ---
    D_BA_CHECK(is_byte_strategy<const dynamic_byte_arena&>::value);
    D_BA_CHECK(is_fixed_strategy<dynamic_byte_arena&&>::value);
    D_BA_CHECK(memory_strategy_kind_of<const dynamic_byte_arena&>::value
                   == storage_kind::fixed_storage);

    return true;
}


/*
tests_dynamic_arena_runtime
  dynamic_byte_arena: aligned, bounded bump allocation over a heap block.
  Tests the following:
  - construction with a byte size reports that capacity and zero in use
  - allocate consumes the block until it is exactly full; the next request throws
  - the storage is writable and reads back
  - reset returns usage to zero
  - construction with a larger alignment yields correspondingly aligned storage
  - the destructor frees the block (exercised implicitly by scope exit; a leak
    or double-free would surface under a sanitizer build)
*/
bool
tests_dynamic_arena_runtime()
{

    dynamic_byte_arena a(128);

    // --- fresh state ---
    D_BA_CHECK(a.bytes_capacity() == 128);
    D_BA_CHECK(a.bytes_in_use() == 0);

    // --- fill the block exactly ---
    void* p = a.allocate(64);
    D_BA_CHECK(p != nullptr);
    D_BA_CHECK(a.bytes_in_use() == 64);
    void* q = a.allocate(64);
    D_BA_CHECK(q != nullptr);
    D_BA_CHECK(a.bytes_in_use() == 128);

    // --- writable, reads back ---
    unsigned char* b = static_cast<unsigned char*>(p);
    b[0] = 0x7F;
    b[63] = 0x80;
    D_BA_CHECK(b[0] == 0x7F && b[63] == 0x80);

    // --- one more request throws ---
    bool threw = false;
    try
    {
        a.allocate(1);
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }
    D_BA_CHECK(threw);

    // --- reset reclaims ---
    a.reset();
    D_BA_CHECK(a.bytes_in_use() == 0);
    void* again = a.allocate(32);
    D_BA_CHECK(again != nullptr);
    D_BA_CHECK(a.bytes_in_use() == 32);

    // --- larger alignment at construction ---
    dynamic_byte_arena aligned(256, 64);
    void* ap = aligned.allocate(32, 64);
    D_BA_CHECK(ap != nullptr);
    D_BA_CHECK(reinterpret_cast<std::uintptr_t>(ap) % 64 == 0);

    // --- overflow-safe bounds: a request so large that aligned + bytes would
    //     wrap still throws, and does not advance the head ---
    dynamic_byte_arena ov(128);
    bool ov_threw = false;
    try
    {
        ov.allocate(SIZE_MAX, 1);
    }
    catch (const std::bad_alloc&)
    {
        ov_threw = true;
    }
    D_BA_CHECK(ov_threw);
    D_BA_CHECK(ov.bytes_in_use() == 0);

    return true;
}


/*
tests_byte_arena_recognition
  byte_arena_strategy_c: a byte strategy that is monotonic.
  Tests the following:
  - both arenas satisfy it (byte-typed and monotonic), in both disciplines
  - a byte fixture that is monotonic (non-releasing) also satisfies it: the
    concept is about the discipline, not the arena classes
  - a RELEASING byte strategy does NOT satisfy it - monotonicity is required
  - an element strategy does not satisfy it (wrong typing), including the static
    buffer and the allocator wrapper
  - a non-strategy does not satisfy it
  - the concept equals the conjunction it is defined as, checked directly
*/
bool
tests_byte_arena_recognition()
{

    // --- both arenas, both disciplines ---
    D_BA_CHECK(byte_arena_strategy_c<monotonic_byte_arena<256>>);
    D_BA_CHECK(byte_arena_strategy_c<dynamic_byte_arena>);

    // --- a monotonic byte fixture qualifies; a releasing one does not ---
    D_BA_CHECK(byte_arena_strategy_c<ms_byte_fixed>);
    D_BA_CHECK(!byte_arena_strategy_c<ms_byte_releasing>);

    // --- element strategies do not qualify (wrong typing) ---
    D_BA_CHECK(!byte_arena_strategy_c<static_buffer_strategy<int, 8>>);
    D_BA_CHECK(
        !byte_arena_strategy_c<allocator_memory_strategy<std::allocator<int>>>);

    // --- a non-strategy does not qualify ---
    D_BA_CHECK(!byte_arena_strategy_c<ms_plain_type>);
    D_BA_CHECK(!byte_arena_strategy_c<int>);

    // --- the concept equals its defining conjunction ---
    D_BA_CHECK(byte_arena_strategy_c<monotonic_byte_arena<256>>
                   == ( is_byte_strategy_v<monotonic_byte_arena<256>>
                        && is_monotonic_strategy_v<monotonic_byte_arena<256>> ));
    D_BA_CHECK(byte_arena_strategy_c<ms_byte_releasing>
                   == ( is_byte_strategy_v<ms_byte_releasing>
                        && is_monotonic_strategy_v<ms_byte_releasing> ));

    // --- cv-ref agnostic ---
    D_BA_CHECK(byte_arena_strategy_c<const monotonic_byte_arena<256>&>);
    D_BA_CHECK(byte_arena_strategy_c<dynamic_byte_arena&&>);

    return true;
}


NS_END  // testing
NS_END  // djinterp
