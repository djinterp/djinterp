/*******************************************************************************
* djinterp [core]                               fixed_string_tests_structure.cpp
*
*
* path:      /tests/djinterp/core/meta/fixed_string_tests_structure.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "fixed_string_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_fixed_string_layout
  The object's shape in memory.
  Tests the following:
  - the payload IS the object: sizeof(fixed_string<N>) is exactly N, with
    no padding, no bookkeeping and no hidden length field, so the type
    costs nothing beyond the literal it carries
  - the member is a genuine char[N], not a pointer and not a decayed
    array, and its extent tracks the template argument
  - alignment is a char's, so the type may sit anywhere
  - the payload is PUBLIC and NOT mutable - both are structural-type
    requirements, and the second is checked by observing that a const
    object's bytes are const, which a mutable member would not be
*/
bool
tests_fixed_string_layout()
{
    bool ok = true;

    // the payload is the whole object, at every length
    D_FS_CHECK(ok, sizeof(fixed_string<1>) == 1);
    D_FS_CHECK(ok, sizeof(fixed_string<4>) == 4);
    D_FS_CHECK(ok, sizeof(fixed_string<32>) == 32);
    D_FS_CHECK(ok, sizeof(fs_abc) == 4);
    D_FS_CHECK(ok, sizeof(fs_abc) == sizeof(fs_abc.data));

    // a real array member, of the right extent
    D_FS_CHECK(ok, std::is_same_v<decltype(fixed_string<4>::data), char[4]>);
    D_FS_CHECK(ok, std::is_array_v<decltype(fixed_string<4>::data)>);
    D_FS_CHECK(ok, std::extent_v<decltype(fixed_string<4>::data)> == 4);
    D_FS_CHECK(ok, std::extent_v<decltype(fixed_string<32>::data)> == 32);
    D_FS_CHECK(ok, std::is_same_v<
                       std::remove_extent_t<
                           decltype(fixed_string<4>::data)>,
                       char>);

    // char alignment, standard layout, and genuinely stateful
    D_FS_CHECK(ok, alignof(fixed_string<4>) == alignof(char));
    D_FS_CHECK(ok, std::is_standard_layout_v<fixed_string<4>>);
    D_FS_CHECK(ok, !std::is_empty_v<fixed_string<4>>);
    D_FS_CHECK(ok, std::is_class_v<fixed_string<4>>);

    // the payload is public - that this line compiles is the claim - and
    // it is not mutable, so a const object's bytes are const
    D_FS_CHECK(ok, fs_abc.data[0] == 'a');
    D_FS_CHECK(ok, std::is_const_v<
                       std::remove_reference_t<decltype(fs_abc.data[0])>>);

    return ok;
}

/*
tests_fixed_string_structural_requirements
  The properties that make fixed_string usable as a class-type NTTP.
  Tests the following:
  - every special member the standard cares about is trivial or absent:
    trivially copy- and move-constructible, trivially copy- and
    move-assignable, trivially destructible, no virtuals
  - it is trivially copyable but NOT trivial, and the difference is the
    point: the user-provided constructor removes the implicit default
    constructor, so there is no way to produce an uninitialised payload
  - for the same reason it is not an aggregate, so brace-init goes
    through the constructor rather than around it
  - the constructor is not explicit, so a literal converts implicitly
  - the constructor binds _N EXACTLY: an array of any other length is not
    a viable argument, and neither is a decayed pointer, which is what
    keeps the deduced _N honest
  There is no std::is_structural to ask, so these are the individual
  requirements; section IV states the conclusion by using the type as an
  NTTP outright.
*/
bool
tests_fixed_string_structural_requirements()
{
    bool ok = true;

    // trivial copies and moves
    D_FS_CHECK(ok, std::is_trivially_copy_constructible_v<fixed_string<4>>);
    D_FS_CHECK(ok, std::is_trivially_move_constructible_v<fixed_string<4>>);
    D_FS_CHECK(ok, std::is_trivially_copy_assignable_v<fixed_string<4>>);
    D_FS_CHECK(ok, std::is_trivially_move_assignable_v<fixed_string<4>>);
    D_FS_CHECK(ok, std::is_trivially_destructible_v<fixed_string<4>>);
    D_FS_CHECK(ok, std::is_nothrow_destructible_v<fixed_string<4>>);
    D_FS_CHECK(ok, std::is_trivially_copyable_v<fixed_string<4>>);

    // no polymorphism anywhere
    D_FS_CHECK(ok, !std::is_polymorphic_v<fixed_string<4>>);
    D_FS_CHECK(ok, !std::has_virtual_destructor_v<fixed_string<4>>);
    D_FS_CHECK(ok, !std::is_abstract_v<fixed_string<4>>);
    D_FS_CHECK(ok, !std::is_final_v<fixed_string<4>>);

    // trivially copyable, yet not trivial - the user-provided constructor
    // is what separates the two, and it is why there is no way to obtain
    // an uninitialised payload
    D_FS_CHECK(ok, !std::is_trivial_v<fixed_string<4>>);
    D_FS_CHECK(ok, !std::is_default_constructible_v<fixed_string<4>>);
    D_FS_CHECK(ok, !std::is_trivially_default_constructible_v<
                       fixed_string<4>>);

    // ... and why it is not an aggregate: brace-init runs the
    // constructor rather than initialising the array directly
    D_FS_CHECK(ok, !std::is_aggregate_v<fixed_string<4>>);

    // the constructor is not explicit, so a literal converts implicitly
    D_FS_CHECK(ok, (std::is_convertible_v<const char(&)[4],
                                          fixed_string<4>>));
    D_FS_CHECK(ok, (std::is_constructible_v<fixed_string<4>,
                                            const char(&)[4]>));

    // and it binds _N exactly - a mismatched length is not viable, and
    // neither is a pointer that has lost its extent
    D_FS_CHECK(ok, !(std::is_constructible_v<fixed_string<4>,
                                             const char(&)[5]>));
    D_FS_CHECK(ok, !(std::is_constructible_v<fixed_string<4>,
                                             const char(&)[3]>));
    D_FS_CHECK(ok, !(std::is_constructible_v<fixed_string<4>,
                                             const char*>));
    D_FS_CHECK(ok, !(std::is_constructible_v<fixed_string<4>, char>));

    return ok;
}

/*
tests_fixed_string_distinctness
  Different lengths are different types.
  Tests the following:
  - fixed_string<N> and fixed_string<M> are unrelated types for N != M,
    with no conversion in either direction
  - two spellings of the same length name the same type
  - the length is recoverable from the type, so nothing about _N is lost
  This matters because _N is the only template argument, and because two
  literals of different length must therefore never collide when used as
  template arguments - a property section IV leans on.
*/
bool
tests_fixed_string_distinctness()
{
    bool ok = true;

    // distinct lengths, distinct types
    D_FS_CHECK(ok, !std::is_same_v<fixed_string<4>, fixed_string<5>>);
    D_FS_CHECK(ok, !std::is_same_v<fixed_string<1>, fixed_string<2>>);
    D_FS_CHECK(ok, sizeof(fixed_string<4>) != sizeof(fixed_string<5>));

    // ... and with no conversion between them
    D_FS_CHECK(ok, !(std::is_convertible_v<fixed_string<4>,
                                           fixed_string<5>>));
    D_FS_CHECK(ok, !(std::is_convertible_v<fixed_string<5>,
                                           fixed_string<4>>));
    D_FS_CHECK(ok, !(std::is_constructible_v<fixed_string<4>,
                                             fixed_string<5>>));

    // the same length is the same type, however it was reached
    D_FS_CHECK(ok, std::is_same_v<fixed_string<4>, fixed_string<4>>);
    D_FS_CHECK(ok, std::is_same_v<decltype(fs_abc), const fixed_string<4>>);
    D_FS_CHECK(ok, std::is_same_v<
                       std::remove_const_t<decltype(fs_abc)>,
                       std::remove_const_t<decltype(fs_embedded)>>);

    // the length survives in the type, and is recoverable from a value
    D_FS_CHECK(ok, fs_extent(fs_abc) == 4);
    D_FS_CHECK(ok, fs_extent(fs_empty) == 1);
    D_FS_CHECK(ok, fs_extent(fs_abc) == sizeof(fixed_string<4>));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
