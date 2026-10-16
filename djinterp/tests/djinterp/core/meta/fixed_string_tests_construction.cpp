/*******************************************************************************
* djinterp [core]                            fixed_string_tests_construction.cpp
*
*
* path:      /tests/djinterp/core/meta/fixed_string_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "fixed_string_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_fixed_string_construction
  The copy loop: every byte of the source array reaches the payload.
  Tests the following:
  - all _N bytes are copied, the TERMINATING NULL INCLUDED.  The null is
    copied out of the source rather than written separately, which is why
    the loop bound is _N and not _N - 1; a loop that stopped one short
    would leave the last byte indeterminate and every c_str() reader
    would run off the end
  - the individual bytes land in the right order, so nothing is reversed
    or shifted
  - a long payload works, giving a bug that only appears after the first
    few iterations somewhere to show
  - the constructor runs at COMPILE time and at RUNTIME alike; constexpr
    permits constant evaluation, it does not require it, and the fixtures
    above only ever exercise the first of the two
  - copies of a constructed value preserve every byte, through both the
    copy constructor and copy assignment
*/
bool
tests_fixed_string_construction()
{
    bool ok = true;

    // every byte, the terminating null included
    D_FS_CHECK(ok, fs_bytes_equal(fs_abc, "abc"));
    D_FS_CHECK(ok, fs_null_terminated(fs_abc));
    D_FS_CHECK(ok, !fs_bytes_equal(fs_abc, "abd"));

    // in the right order
    D_FS_CHECK(ok, fs_abc.data[0] == 'a');
    D_FS_CHECK(ok, fs_abc.data[1] == 'b');
    D_FS_CHECK(ok, fs_abc.data[2] == 'c');
    D_FS_CHECK(ok, fs_abc.data[3] == '\0');

    // the shortest payload there is: one byte, and it is the null
    D_FS_CHECK(ok, fs_bytes_equal(fs_empty, ""));
    D_FS_CHECK(ok, fs_empty.data[0] == '\0');
    D_FS_CHECK(ok, fs_null_terminated(fs_empty));

    // a long payload, so the loop runs many iterations
    D_FS_CHECK(ok, fs_bytes_equal(fs_long, "the quick brown fox jumped over"));
    D_FS_CHECK(ok, fs_null_terminated(fs_long));
    D_FS_CHECK(ok, fs_long.data[0] == 't');
    D_FS_CHECK(ok, fs_long.data[30] == 'r');

    // the same constructor at RUNTIME.  The source is written to after
    // it is declared, so this cannot collapse into a constant
    char source[4] = {'a', 'b', 'c', '\0'};
    source[1] = 'B';

    const fixed_string<4> runtime{source};

    D_FS_CHECK(ok, runtime.data[0] == 'a');
    D_FS_CHECK(ok, runtime.data[1] == 'B');
    D_FS_CHECK(ok, runtime.data[3] == '\0');
    D_FS_CHECK(ok, fs_bytes_equal(runtime, "aBc"));

    // copies preserve every byte, both ways round
    const fixed_string<4> copied{fs_abc};

    D_FS_CHECK(ok, fs_bytes_equal(copied, "abc"));

    fixed_string<4> assigned{"..."};
    assigned = fs_abc;

    D_FS_CHECK(ok, fs_bytes_equal(assigned, "abc"));
    D_FS_CHECK(ok, assigned.data[3] == '\0');

    return ok;
}

/*
tests_fixed_string_deduction_guide
  CTAD from a string literal, and what _N comes out as.
  Tests the following:
  - _N is the literal's ARRAY length, so it counts the terminating null;
    a guide that deduced the visible character count instead would make
    size() off by one everywhere and truncate every payload by a byte
  - that holds at every length, down to the empty literal, which deduces
    fixed_string<1> rather than fixed_string<0> - the smallest _N the
    guide can produce, and the reason fixed_string<0> never arises
  - CTAD and the explicit spelling name the same type
  - the deduced length is recoverable from the value, so a caller can
    check what it got rather than assume
*/
bool
tests_fixed_string_deduction_guide()
{
    bool ok = true;

    // _N counts the terminating null
    D_FS_CHECK(ok, std::is_same_v<decltype(fixed_string{"abc"}),
                                  fixed_string<4>>);
    D_FS_CHECK(ok, std::is_same_v<decltype(fixed_string{"a"}),
                                  fixed_string<2>>);
    D_FS_CHECK(ok, std::is_same_v<decltype(fixed_string{"hello"}),
                                  fixed_string<6>>);

    // ... including for the empty literal, which is fixed_string<1>
    D_FS_CHECK(ok, std::is_same_v<decltype(fixed_string{""}),
                                  fixed_string<1>>);
    D_FS_CHECK(ok, !std::is_same_v<decltype(fixed_string{""}),
                                   fixed_string<2>>);

    // CTAD and the explicit spelling agree
    D_FS_CHECK(ok, std::is_same_v<decltype(fixed_string{"abc"}),
                                  decltype(fixed_string<4>{"abc"})>);
    D_FS_CHECK(ok, std::is_same_v<
                       std::remove_const_t<decltype(fs_abc)>,
                       decltype(fixed_string{"abc"})>);

    // the deduced length is recoverable from the value
    D_FS_CHECK(ok, fs_extent(fixed_string{"abc"}) == 4);
    D_FS_CHECK(ok, fs_extent(fixed_string{""}) == 1);
    D_FS_CHECK(ok, fs_extent(fixed_string{"hello"}) == 6);
    D_FS_CHECK(ok, fs_extent(fs_long) == 32);

    // and it is always one more than the reported size
    D_FS_CHECK(ok, fs_extent(fs_abc) == fs_abc.size() + 1);
    D_FS_CHECK(ok, fs_extent(fs_empty) == fs_empty.size() + 1);
    D_FS_CHECK(ok, fs_extent(fs_long) == fs_long.size() + 1);

    return ok;
}

/*
tests_fixed_string_embedded_nulls
  A payload containing an INTERIOR null, which is where the byte-count
  contract and the C-string contract come apart.
  Tests the following:
  - the interior null is copied like any other byte, so the payload is
    the literal's array verbatim
  - size() reports the byte count and does NOT search for a null: for
    "a\0b" it is 3, while strlen(c_str()) is 1
  - view() spans all three bytes, interior null included, so it is not a
    C-string view either
  - c_str(), by contrast, behaves exactly as a C string should and stops
    at the first null
  This is the only fixture that can tell a correct size() apart from one
  that had quietly become strlen-based; every ordinary literal agrees
  with both.
*/
bool
tests_fixed_string_embedded_nulls()
{
    bool ok = true;

    // copied verbatim, interior null and all
    D_FS_CHECK(ok, fs_bytes_equal(fs_embedded, "a\0b"));
    D_FS_CHECK(ok, fs_embedded.data[0] == 'a');
    D_FS_CHECK(ok, fs_embedded.data[1] == '\0');
    D_FS_CHECK(ok, fs_embedded.data[2] == 'b');
    D_FS_CHECK(ok, fs_embedded.data[3] == '\0');
    D_FS_CHECK(ok, fs_extent(fs_embedded) == 4);

    // size() counts bytes; it does not look for a null
    D_FS_CHECK(ok, fs_embedded.size() == 3);
    D_FS_CHECK(ok, std::strlen(fs_embedded.c_str()) == 1);
    D_FS_CHECK(ok, fs_embedded.size() != std::strlen(fs_embedded.c_str()));

    // view() spans every counted byte, interior null included
    D_FS_CHECK(ok, fs_embedded.view().size() == 3);
    D_FS_CHECK(ok, fs_embedded.view() == std::string_view(&"a\0b"[0], 3));
    D_FS_CHECK(ok, fs_embedded.view()[1] == '\0');
    D_FS_CHECK(ok, fs_embedded.view() != std::string_view("a"));

    // c_str(), by contrast, is an honest C string and stops early
    D_FS_CHECK(ok, fs_embedded.c_str()[0] == 'a');
    D_FS_CHECK(ok, fs_embedded.c_str()[1] == '\0');
    D_FS_CHECK(ok, std::strcmp(fs_embedded.c_str(), "a") == 0);

    // for an ordinary literal the two contracts coincide, which is why
    // the checks above need a literal that is not ordinary
    D_FS_CHECK(ok, fs_abc.size() == std::strlen(fs_abc.c_str()));
    D_FS_CHECK(ok, fs_empty.size() == std::strlen(fs_empty.c_str()));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
