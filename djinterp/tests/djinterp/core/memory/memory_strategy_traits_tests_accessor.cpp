/*******************************************************************************
* djinterp [testing]                   memory_strategy_traits_tests_accessor.cpp
*
* Section: uniform accessor, aggregate snapshot, and cv/ref agnosticism.
*   memory_strategy_traits.hpp IX  - memory_strategy_traits<S>
*   memory_strategy_traits.hpp X   - memory_strategy_class<S>
*
*   memory_strategy_traits gathers the descriptive surface behind one name and
* is the axis' single customization point: a foreign type is adapted by
* specializing THIS struct, not by being modified.  Both facts are tested - the
* primary template against several strategies field by field, and an explicit
* specialization that reports a surface detection would never grant.
*   memory_strategy_class is the read-only snapshot: every classifier at once,
* which is convenient exactly when a debugger or static_assert wants the whole
* picture.  It is checked for agreement with the individual traits, so it can
* never silently drift from them.
*   The cv/ref test is placed here because the accessors are where agnosticism
* matters most: a caller writes memory_strategy_traits<decltype(x)> without
* stripping references first.  The composites and accessors clean their input;
* the RAW detectors do not, and that asymmetry is asserted directly so it reads
* as designed rather than accidental.
*
*
* path:      /tests/djinterp/core/memory/memory_strategy_traits_tests_accessor.cpp
*                                         memory_strategy_tests_accessor.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <memory>
#include <type_traits>
// djinterp
#include "memory_strategy_traits_tests.hpp"


namespace
{
    // ms_foreign_unadapted
    //   struct: not a strategy by detection - no kind constant, no typed
    // surface.  Used below to demonstrate the accessor customization point: the
    // explicit specialization reports a full element surface that structural
    // detection would never grant this type.
    struct ms_foreign_unadapted
    {};
}


NS_DJINTERP

// memory_strategy_traits<ms_foreign_unadapted>
//   an explicit specialization: adapt a non-conforming type by hand.  Detection
// still sees nothing on this type; the accessor sees whatever we declare here.
template<>
struct memory_strategy_traits<ms_foreign_unadapted>
{
    using strategy_type = ms_foreign_unadapted;
    using value_type    = short;

    static constexpr strategy_typing typing = strategy_typing::element_typed;
    static constexpr storage_kind    kind   = storage_kind::dynamic_storage;

    static constexpr bool is_element     = true;
    static constexpr bool is_byte        = false;
    static constexpr bool allocates      = true;
    static constexpr bool pointer_stable = false;
    static constexpr bool monotonic      = false;
    static constexpr bool generational   = false;
};

NS_END  // djinterp


NS_DJINTERP
NS_TESTING


/*
tests_memory_strategy_traits_accessor
  memory_strategy_traits<S>: the uniform descriptive surface (core IX).
  Tests the following:
  - the primary template reports each field correctly for a byte strategy
    (arena: void value_type, byte-typed, static, allocating, monotonic) and for
    an element strategy (static buffer: int value_type, element-typed, static,
    monotonic) and for a dynamic releasing element strategy (allocator wrapper)
  - value_type is the declared alias, or void when none is present
  - is_element / is_byte are read from typing, and never both true
  - a descriptive-only strategy reports typing none, allocates false, and a real
    kind (static) - the accessor still carries the discipline
  - the customization point works: the hand-written specialization for a foreign
    type reports an element surface, even though the detection traits still
    classify that same type as a non-strategy.  Adaptation by specialization,
    exactly as documented
*/
bool
tests_memory_strategy_traits_accessor()
{

    // --- a byte strategy: the arena ---
    using ta = memory_strategy_traits<monotonic_byte_arena<256>>;
    D_MT_CHECK((std::is_same<ta::strategy_type,
                             monotonic_byte_arena<256>>::value));
    D_MT_CHECK((std::is_same<ta::value_type, void>::value));
    D_MT_CHECK(ta::typing == strategy_typing::byte_typed);
    D_MT_CHECK(ta::kind == storage_kind::static_storage);
    D_MT_CHECK(!ta::is_element);
    D_MT_CHECK(ta::is_byte);
    D_MT_CHECK(ta::allocates);
    D_MT_CHECK(ta::pointer_stable);
    D_MT_CHECK(ta::monotonic);
    D_MT_CHECK(!ta::generational);

    // --- an element strategy: the static buffer ---
    using tb = memory_strategy_traits<static_buffer_strategy<int, 8>>;
    D_MT_CHECK((std::is_same<tb::value_type, int>::value));
    D_MT_CHECK(tb::typing == strategy_typing::element_typed);
    D_MT_CHECK(tb::kind == storage_kind::static_storage);
    D_MT_CHECK(tb::is_element);
    D_MT_CHECK(!tb::is_byte);
    D_MT_CHECK(tb::allocates);
    D_MT_CHECK(tb::pointer_stable);
    D_MT_CHECK(tb::monotonic);

    // --- a dynamic releasing element strategy: the allocator wrapper ---
    using tc = memory_strategy_traits<
        allocator_memory_strategy<std::allocator<int>>>;
    D_MT_CHECK((std::is_same<tc::value_type, int>::value));
    D_MT_CHECK(tc::typing == strategy_typing::element_typed);
    D_MT_CHECK(tc::kind == storage_kind::dynamic_storage);
    D_MT_CHECK(tc::allocates);
    D_MT_CHECK(!tc::pointer_stable);
    D_MT_CHECK(!tc::monotonic);

    // --- a generational element fixture ---
    using tg = memory_strategy_traits<ms_element_generational>;
    D_MT_CHECK((std::is_same<tg::value_type, long>::value));
    D_MT_CHECK(tg::allocates);
    D_MT_CHECK(!tg::monotonic);
    D_MT_CHECK(tg::generational);

    // --- a descriptive-only strategy ---
    using td = memory_strategy_traits<ms_descriptive_static>;
    D_MT_CHECK((std::is_same<td::value_type, void>::value));
    D_MT_CHECK(td::typing == strategy_typing::none);
    D_MT_CHECK(td::kind == storage_kind::static_storage);
    D_MT_CHECK(!td::is_element);
    D_MT_CHECK(!td::is_byte);
    D_MT_CHECK(!td::allocates);

    // --- is_element / is_byte are never both true ---
    D_MT_CHECK(!(ta::is_element && ta::is_byte));
    D_MT_CHECK(!(tb::is_element && tb::is_byte));
    D_MT_CHECK(!(td::is_element && td::is_byte));

    // --- the customization point: specialization overrides detection ---
    using tf = memory_strategy_traits<ms_foreign_unadapted>;
    D_MT_CHECK((std::is_same<tf::value_type, short>::value));
    D_MT_CHECK(tf::typing == strategy_typing::element_typed);
    D_MT_CHECK(tf::kind == storage_kind::dynamic_storage);
    D_MT_CHECK(tf::is_element);
    D_MT_CHECK(tf::allocates);
    //     yet the detection traits still see a non-strategy
    D_MT_CHECK(!is_memory_strategy<ms_foreign_unadapted>::value);
    D_MT_CHECK(!is_element_strategy<ms_foreign_unadapted>::value);

    return true;
}


/*
tests_memory_strategy_class_snapshot
  memory_strategy_class<S>: the whole classification at once (core X).
  Tests the following:
  - every field for a byte monotonic arena, an element monotonic buffer, a
    dynamic releasing allocator wrapper, and a descriptive-only fixture
  - the snapshot AGREES with the individual traits, field for field - it is a
    convenience view and must never drift from the sources it summarizes
  - is_static / is_fixed / is_dynamic appear here (they are absent from the
    accessor), and remain mutually exclusive
  - is_allocating and is_descriptive_only remain complementary over members
*/
bool
tests_memory_strategy_class_snapshot()
{

    // --- byte monotonic arena ---
    using ca = memory_strategy_class<monotonic_byte_arena<256>>;
    D_MT_CHECK(ca::is_strategy);
    D_MT_CHECK(ca::is_allocating);
    D_MT_CHECK(!ca::is_descriptive_only);
    D_MT_CHECK(!ca::is_element);
    D_MT_CHECK(ca::is_byte);
    D_MT_CHECK(ca::pointer_stable);
    D_MT_CHECK(!ca::individual_release);
    D_MT_CHECK(ca::monotonic);
    D_MT_CHECK(!ca::generational);
    D_MT_CHECK(ca::is_static);
    D_MT_CHECK(!ca::is_fixed);
    D_MT_CHECK(!ca::is_dynamic);
    D_MT_CHECK(ca::typing == strategy_typing::byte_typed);
    D_MT_CHECK(ca::kind == storage_kind::static_storage);

    // --- element monotonic buffer ---
    using cb = memory_strategy_class<static_buffer_strategy<int, 8>>;
    D_MT_CHECK(cb::is_strategy);
    D_MT_CHECK(cb::is_allocating);
    D_MT_CHECK(cb::is_element);
    D_MT_CHECK(!cb::is_byte);
    D_MT_CHECK(cb::pointer_stable);
    D_MT_CHECK(cb::monotonic);
    D_MT_CHECK(cb::is_static);

    // --- dynamic releasing allocator wrapper ---
    using cc = memory_strategy_class<
        allocator_memory_strategy<std::allocator<int>>>;
    D_MT_CHECK(cc::is_strategy);
    D_MT_CHECK(cc::is_allocating);
    D_MT_CHECK(cc::is_element);
    D_MT_CHECK(!cc::pointer_stable);
    D_MT_CHECK(cc::individual_release);
    D_MT_CHECK(!cc::monotonic);
    D_MT_CHECK(cc::is_dynamic);

    // --- descriptive-only fixture ---
    using cd = memory_strategy_class<ms_descriptive_static>;
    D_MT_CHECK(cd::is_strategy);
    D_MT_CHECK(!cd::is_allocating);
    D_MT_CHECK(cd::is_descriptive_only);
    D_MT_CHECK(!cd::is_element);
    D_MT_CHECK(!cd::is_byte);
    D_MT_CHECK(cd::is_static);

    // --- the snapshot agrees with the individual traits, field for field ---
    D_MT_CHECK(ca::is_strategy
                   == is_memory_strategy<monotonic_byte_arena<256>>::value);
    D_MT_CHECK(ca::is_allocating
                   == is_allocating_strategy<monotonic_byte_arena<256>>::value);
    D_MT_CHECK(ca::is_byte
                   == is_byte_strategy<monotonic_byte_arena<256>>::value);
    D_MT_CHECK(ca::monotonic
                   == is_monotonic_strategy<monotonic_byte_arena<256>>::value);
    D_MT_CHECK(ca::pointer_stable
                   == is_pointer_stable_strategy<monotonic_byte_arena<256>>
                          ::value);
    D_MT_CHECK(cc::individual_release
                   == supports_individual_release_strategy<
                          allocator_memory_strategy<std::allocator<int>>>
                              ::value);
    D_MT_CHECK(cc::is_dynamic
                   == is_dynamic_strategy<
                          allocator_memory_strategy<std::allocator<int>>>
                              ::value);
    D_MT_CHECK(cd::is_descriptive_only
                   == is_descriptive_only_strategy<ms_descriptive_static>
                          ::value);

    // --- static / fixed / dynamic mutually exclusive ---
    D_MT_CHECK(( int(ca::is_static) + int(ca::is_fixed)
                 + int(ca::is_dynamic) ) == 1);
    D_MT_CHECK(( int(cc::is_static) + int(cc::is_fixed)
                 + int(cc::is_dynamic) ) == 1);

    // --- is_allocating and is_descriptive_only complementary over members ---
    D_MT_CHECK(ca::is_allocating != ca::is_descriptive_only);
    D_MT_CHECK(cd::is_allocating != cd::is_descriptive_only);

    return true;
}


/*
tests_cv_ref_agnosticism
  cv/ref handling across the axis: composites clean, raw detectors do not.
  Tests the following:
  - the RAW detectors are cv/ref SENSITIVE, and in DIFFERENT ways: the kind
    constant detector fails on a reference (decltype of a member on a reference
    type), while the element-allocate detector fails on a const object (a
    non-const member function).  Both asymmetries are asserted, because they are
    the reason the composites must clean
  - every COMPOSITE trait is fully cv/ref agnostic: membership, typing, kind,
    the refinements, over S / const S / S& / const S& / volatile S /
    const volatile S& / S&&
  - the ACCESSOR structs are agnostic too: memory_strategy_traits and
    memory_strategy_class give identical answers on S and const S&, including
    the cleaned strategy_type and value_type
*/
bool
tests_cv_ref_agnosticism()
{

    // --- raw detectors are cv/ref sensitive (the asymmetry the rest hides) ---
    //     kind-constant detector: fails on a reference type
    D_MT_CHECK(has_strategy_kind_constant_v<ms_descriptive_static>);
    D_MT_CHECK(has_strategy_kind_constant_v<const ms_descriptive_static>);
    D_MT_CHECK(!has_strategy_kind_constant_v<ms_descriptive_static&>);
    //     element-allocate detector: fails on a const object
    D_MT_CHECK(has_element_allocate_v<ms_element_dynamic>);
    D_MT_CHECK(!has_element_allocate_v<const ms_element_dynamic>);
    D_MT_CHECK(has_element_allocate_v<ms_element_dynamic&>);

    // --- composites are fully agnostic: membership ---
    using B = monotonic_byte_arena<256>;
    D_MT_CHECK(is_memory_strategy_v<B>);
    D_MT_CHECK(is_memory_strategy_v<const B>);
    D_MT_CHECK(is_memory_strategy_v<B&>);
    D_MT_CHECK(is_memory_strategy_v<const B&>);
    D_MT_CHECK(is_memory_strategy_v<volatile B>);
    D_MT_CHECK(is_memory_strategy_v<const volatile B&>);
    D_MT_CHECK(is_memory_strategy_v<B&&>);

    // --- composites are agnostic: typing, kind, refinements ---
    D_MT_CHECK(is_byte_strategy_v<const B&>);
    D_MT_CHECK(strategy_typing_of_v<const B&> == strategy_typing::byte_typed);
    D_MT_CHECK(memory_strategy_kind_of_v<const volatile B&>
                   == storage_kind::static_storage);
    D_MT_CHECK(is_static_strategy_v<B&&>);
    D_MT_CHECK(is_pointer_stable_strategy_v<const B&>);
    D_MT_CHECK(is_monotonic_strategy_v<const B&>);
    D_MT_CHECK(is_monotonic_strategy_v<B&&>);

    // --- an element strategy is equally agnostic ---
    using E = static_buffer_strategy<int, 8>;
    D_MT_CHECK(is_element_strategy_v<const E&>);
    D_MT_CHECK(is_element_strategy_v<E&&>);
    D_MT_CHECK(is_allocating_strategy_v<const E&>);

    // --- the accessor structs are agnostic, including cleaned members ---
    D_MT_CHECK(memory_strategy_traits<B>::kind
                   == memory_strategy_traits<const B&>::kind);
    D_MT_CHECK(memory_strategy_traits<B>::typing
                   == memory_strategy_traits<const B&>::typing);
    D_MT_CHECK(memory_strategy_traits<B>::monotonic
                   == memory_strategy_traits<const B&>::monotonic);
    D_MT_CHECK((std::is_same<memory_strategy_traits<const E&>::strategy_type,
                             E>::value));
    D_MT_CHECK((std::is_same<memory_strategy_traits<const E&>::value_type,
                             int>::value));
    D_MT_CHECK(memory_strategy_class<B>::is_strategy
                   == memory_strategy_class<const B&>::is_strategy);
    D_MT_CHECK(memory_strategy_class<B>::monotonic
                   == memory_strategy_class<const volatile B&>::monotonic);

    return true;
}


NS_END  // testing
NS_END  // djinterp
