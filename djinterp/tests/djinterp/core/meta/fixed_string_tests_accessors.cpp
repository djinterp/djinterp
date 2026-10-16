/*******************************************************************************
* djinterp [core]                               fixed_string_tests_accessors.cpp
*
*
* path:      /tests/djinterp/core/meta/fixed_string_tests_accessors.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "fixed_string_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_fixed_string_c_str
  c_str(): a borrowed pointer to the payload itself.
  Tests the following:
  - it returns the address of the payload rather than a copy, so no
    allocation or static buffer is involved and repeated calls agree
  - what it points at is null-terminated, which is what makes it safe to
    hand to a C interface
  - its type is const char*, it is const-callable and it is noexcept, all
    three as declared
  - it is usable in a constant expression.  That is stated by making its
    result a TEMPLATE ARGUMENT, since nothing but a constant expression
    can be one; the surrounding check then verifies the value at runtime
*/
bool
tests_fixed_string_c_str()
{
    bool ok = true;

    // the payload itself, not a copy
    D_FS_CHECK(ok, fs_abc.c_str() == fs_abc.data);
    D_FS_CHECK(ok, fs_abc.c_str() == fs_abc.c_str());
    D_FS_CHECK(ok, fs_empty.c_str() == fs_empty.data);
    D_FS_CHECK(ok, fs_long.c_str() == fs_long.data);

    // pointing at a null-terminated sequence
    D_FS_CHECK(ok, std::strcmp(fs_abc.c_str(), "abc") == 0);
    D_FS_CHECK(ok, std::strcmp(fs_empty.c_str(), "") == 0);
    D_FS_CHECK(ok, std::strcmp(fs_long.c_str(),
                               "the quick brown fox jumped over") == 0);
    D_FS_CHECK(ok, fs_empty.c_str()[0] == '\0');
    D_FS_CHECK(ok, fs_abc.c_str()[3] == '\0');

    // the declared shape
    D_FS_CHECK(ok, std::is_same_v<decltype(fs_abc.c_str()),
                                  const char*>);
    D_FS_CHECK(ok, noexcept(fs_abc.c_str()));

    // usable in a constant expression - the result is a template
    // argument below, which only a constant expression may be
    D_FS_CHECK(ok, (std::integral_constant<char,
                                           fs_abc.c_str()[0]>::value
                    == 'a'));
    D_FS_CHECK(ok, (std::integral_constant<char,
                                           fs_abc.c_str()[3]>::value
                    == '\0'));

    return ok;
}

/*
tests_fixed_string_size
  size(): the payload length, excluding the terminating null.
  Tests the following:
  - it is _N - 1 at every length, and it is derived from _N rather than
    from scanning the payload, so it counts bytes and not characters
  - the empty literal reports 0, which is the one place the _N - 1
    arithmetic reaches its floor.  _N cannot be smaller: a literal always
    carries its null, so fixed_string<0> never arises and the subtraction
    can never wrap
  - it agrees with view().size() and with the deduced extent minus one,
    so the three ways of asking the same question cannot drift apart
  - it is const-callable, noexcept, returns std::size_t, and is usable in
    a constant expression
*/
bool
tests_fixed_string_size()
{
    bool ok = true;

    // _N - 1 at every length
    D_FS_CHECK(ok, fs_abc.size() == 3);
    D_FS_CHECK(ok, fs_long.size() == 31);
    D_FS_CHECK(ok, fixed_string{"a"}.size() == 1);
    D_FS_CHECK(ok, fixed_string{"hello"}.size() == 5);

    // the floor: the empty literal reports zero, and _N was 1
    D_FS_CHECK(ok, fs_empty.size() == 0);
    D_FS_CHECK(ok, fs_extent(fs_empty) == 1);

    // the three ways of asking agree, at every fixture
    D_FS_CHECK(ok, fs_abc.size() == fs_abc.view().size());
    D_FS_CHECK(ok, fs_abc.size() == fs_extent(fs_abc) - 1);
    D_FS_CHECK(ok, fs_empty.size() == fs_empty.view().size());
    D_FS_CHECK(ok, fs_empty.size() == fs_extent(fs_empty) - 1);
    D_FS_CHECK(ok, fs_embedded.size() == fs_embedded.view().size());
    D_FS_CHECK(ok, fs_embedded.size() == fs_extent(fs_embedded) - 1);
    D_FS_CHECK(ok, fs_long.size() == fs_long.view().size());
    D_FS_CHECK(ok, fs_long.size() == fs_extent(fs_long) - 1);

    // the declared shape
    D_FS_CHECK(ok, std::is_same_v<decltype(fs_abc.size()), std::size_t>);
    D_FS_CHECK(ok, noexcept(fs_abc.size()));

    // usable in a constant expression
    D_FS_CHECK(ok, (std::integral_constant<std::size_t,
                                           fs_abc.size()>::value == 3));
    D_FS_CHECK(ok, (std::integral_constant<std::size_t,
                                           fs_empty.size()>::value == 0));

    return ok;
}

/*
tests_fixed_string_view
  view(): a std::string_view over the payload, null excluded.
  Tests the following:
  - it spans exactly size() bytes starting at c_str(), so it borrows the
    payload rather than copying it
  - it EXCLUDES the terminating null: an ordinary literal's view contains
    no null at all, which is what makes it comparable against an ordinary
    string_view without a trailing-byte mismatch
  - the empty literal yields an empty but still valid view, whose data
    pointer is the payload rather than null
  - it is const-callable, noexcept, returns std::string_view, and is
    usable in a constant expression
  - comparison is meant to go THROUGH view(): fixed_string ships no
    operator==, so two payloads are compared as string_views.  Both
    halves of that are stated below, because the absence of operator== is
    a design decision a reader will otherwise mistake for an oversight
*/
bool
tests_fixed_string_view()
{
    bool ok = true;

    // borrows the payload: size() bytes starting at c_str()
    D_FS_CHECK(ok, fs_abc.view().size() == fs_abc.size());
    D_FS_CHECK(ok, fs_abc.view().data() == fs_abc.c_str());
    D_FS_CHECK(ok, fs_abc.view() == std::string_view("abc"));
    D_FS_CHECK(ok, fs_long.view() ==
                   std::string_view("the quick brown fox jumped over"));

    // the terminating null is excluded
    D_FS_CHECK(ok, fs_abc.view().find('\0') == std::string_view::npos);
    D_FS_CHECK(ok, fs_long.view().find('\0') == std::string_view::npos);
    D_FS_CHECK(ok, fs_abc.view().back() == 'c');
    D_FS_CHECK(ok, fs_abc.view().size() == sizeof(fs_abc) - 1);

    // ... but an INTERIOR null is not, since exclusion is positional
    D_FS_CHECK(ok, fs_embedded.view().find('\0') == 1);

    // the empty literal: empty, yet still a valid view of the payload
    D_FS_CHECK(ok, fs_empty.view().empty());
    D_FS_CHECK(ok, fs_empty.view().size() == 0);
    D_FS_CHECK(ok, fs_empty.view().data() == fs_empty.c_str());
    D_FS_CHECK(ok, fs_empty.view().data() != nullptr);
    D_FS_CHECK(ok, fs_empty.view() == std::string_view());

    // the declared shape
    D_FS_CHECK(ok, std::is_same_v<decltype(fs_abc.view()),
                                  std::string_view>);
    D_FS_CHECK(ok, noexcept(fs_abc.view()));

    // usable in a constant expression
    D_FS_CHECK(ok, (std::integral_constant<
                        std::size_t, fs_abc.view().size()>::value == 3));
    D_FS_CHECK(ok, (std::integral_constant<
                        char, fs_abc.view()[0]>::value == 'a'));

    // comparison goes through view(): the carrier ships no operator==,
    // the view it hands back does
    D_FS_CHECK(ok, !(fs_is_equality_comparable<fixed_string<4>,
                                               fixed_string<4>>::value));
    D_FS_CHECK(ok, (fs_is_equality_comparable<std::string_view,
                                              std::string_view>::value));
    D_FS_CHECK(ok, fs_abc.view() != fs_embedded.view());
    D_FS_CHECK(ok, fs_abc.view() == fixed_string{"abc"}.view());

    return ok;
}


NS_END  // testing
NS_END  // djinterp
