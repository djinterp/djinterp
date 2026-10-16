/*******************************************************************************
* djinterp [core]                                    fixed_string_tests_nttp.cpp
*
*
* path:      /tests/djinterp/core/meta/fixed_string_tests_nttp.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "fixed_string_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_fixed_string_as_nttp
  The module's whole reason for existing, stated by use.
  Tests the following:
  - a fixed_string may be a template argument in every position an NTTP
    can occupy: a class template, a variable template, a concept, and a
    constrained function's requires-clause.  There is no std::is_structural
    to consult, so this function COMPILING is the claim; the checks then
    confirm the argument was usable rather than merely accepted
  - a bare string literal reaches those positions directly, because the
    parameter is a placeholder for a deduced class type - so the guide
    runs during template-argument deduction, not just at object creation
  - the two spellings, deduced and explicit, name the same specialization
  - the empty literal is a legal argument too, and a constraint can tell
    it apart from a non-empty one
*/
bool
tests_fixed_string_as_nttp()
{
    bool ok = true;

    // a class template on the NTTP
    D_FS_CHECK(ok, fs_holder<fixed_string{"abc"}>::value.size() == 3);
    D_FS_CHECK(ok, fs_binds_nttp<fs_holder>::value);

    // a variable template
    D_FS_CHECK(ok, fs_size_of<fixed_string{"abc"}> == 3);
    D_FS_CHECK(ok, fs_size_of<fixed_string{""}> == 0);
    D_FS_CHECK(ok, fs_size_of<fs_long> == 31);

    // a concept, evaluated on the argument
    D_FS_CHECK(ok, fs_nonempty<fixed_string{"x"}>);
    D_FS_CHECK(ok, !fs_nonempty<fixed_string{""}>);
    D_FS_CHECK(ok, fs_nonempty<fs_abc>);
    D_FS_CHECK(ok, !fs_nonempty<fs_empty>);

    // a constrained function, whose requires-clause is checked before
    // the body is instantiated
    D_FS_CHECK(ok, fs_guarded<fixed_string{"yz"}>() == 2);
    D_FS_CHECK(ok, fs_guarded<fs_abc>() == 3);
    D_FS_CHECK(ok, fs_guarded<fs_long>() == 31);

    // a bare literal reaches all of them, because deduction runs during
    // template-argument deduction and not only at object creation
    D_FS_CHECK(ok, fs_holder<"abc">::value.size() == 3);
    D_FS_CHECK(ok, fs_size_of<"hello"> == 5);
    D_FS_CHECK(ok, fs_nonempty<"x">);
    D_FS_CHECK(ok, !fs_nonempty<"">);
    D_FS_CHECK(ok, fs_guarded<"yz">() == 2);

    // ... and the deduced spelling names the same specialization as the
    // explicit one
    D_FS_CHECK(ok, std::is_same_v<fs_holder<"abc">,
                                  fs_holder<fixed_string{"abc"}>>);
    D_FS_CHECK(ok, std::is_same_v<fs_holder<"">,
                                  fs_holder<fixed_string{""}>>);

    return ok;
}

/*
tests_fixed_string_argument_equivalence
  When do two fixed_string template arguments name the SAME specialization?
  Tests the following:
  - equal payloads do, and unequal payloads do not; differing lengths do
    not either, since those are different types before values are even
    considered
  - equivalence is decided MEMBER-WISE by the compiler, not by any
    operator== - the module ships none, as the accessor section states -
    so the same bytes reached by different routes still collide correctly:
    a named constexpr object, a freshly deduced temporary, and an
    explicitly spelled specialization all agree
  - every byte participates, including bytes PAST an interior null.  Two
    payloads that a C string would treat as identical, "a\0b" and "a\0c",
    name different specializations, which is what rules out any
    strlen-shaped shortcut in the argument's identity
*/
bool
tests_fixed_string_argument_equivalence()
{
    bool ok = true;

    // equal payloads collide, unequal ones do not
    D_FS_CHECK(ok, std::is_same_v<fs_holder<fixed_string{"abc"}>,
                                  fs_holder<fixed_string{"abc"}>>);
    D_FS_CHECK(ok, !std::is_same_v<fs_holder<fixed_string{"abc"}>,
                                   fs_holder<fixed_string{"abd"}>>);
    D_FS_CHECK(ok, !std::is_same_v<fs_holder<fixed_string{"abc"}>,
                                   fs_holder<fixed_string{"cba"}>>);

    // a differing length differs before values are compared at all
    D_FS_CHECK(ok, !std::is_same_v<fs_holder<fixed_string{"ab"}>,
                                   fs_holder<fixed_string{"abc"}>>);
    D_FS_CHECK(ok, !std::is_same_v<fs_holder<fixed_string{""}>,
                                   fs_holder<fixed_string{"a"}>>);

    // the same bytes reached by different routes still collide: a named
    // object, a deduced temporary and an explicit specialization agree
    D_FS_CHECK(ok, std::is_same_v<fs_holder<fs_abc>,
                                  fs_holder<fixed_string{"abc"}>>);
    D_FS_CHECK(ok, std::is_same_v<fs_holder<fs_abc>,
                                  fs_holder<fixed_string<4>{"abc"}>>);
    D_FS_CHECK(ok, std::is_same_v<fs_holder<fs_abc>, fs_holder<"abc">>);
    D_FS_CHECK(ok, std::is_same_v<fs_holder<fs_empty>, fs_holder<"">>);

    // ... even though the type itself is not equality-comparable, which
    // is what shows the compiler is comparing members rather than
    // calling anything the module supplied
    D_FS_CHECK(ok, !(fs_is_equality_comparable<fixed_string<4>,
                                               fixed_string<4>>::value));

    // every byte participates, including those past an interior null
    D_FS_CHECK(ok, std::is_same_v<fs_holder<fs_embedded>,
                                  fs_holder<fixed_string{"a\0b"}>>);
    D_FS_CHECK(ok, !std::is_same_v<fs_holder<fixed_string{"a\0b"}>,
                                   fs_holder<fixed_string{"a\0c"}>>);
    D_FS_CHECK(ok, !std::is_same_v<fs_holder<fixed_string{"a\0b"}>,
                                   fs_holder<fixed_string{"abb"}>>);

    return ok;
}

/*
tests_fixed_string_nttp_round_trip
  What comes back out of a specialization is what went in.
  Tests the following:
  - the stored argument has the same type and the same bytes as the
    original, at every fixture length
  - the accessors work on it exactly as they do on an ordinary object, so
    a consumer can carry a literal through a template parameter and still
    read it back as a string
  - an interior null survives the round trip through the compiler's
    template-argument machinery, and so does the empty payload; those are
    the two shapes most likely to be lost by an implementation that took
    a C-string shortcut anywhere along the way
*/
bool
tests_fixed_string_nttp_round_trip()
{
    bool ok = true;

    // the stored argument keeps its type
    D_FS_CHECK(ok, std::is_same_v<
                       std::remove_const_t<
                           decltype(fs_holder<"abc">::value)>,
                       fixed_string<4>>);
    D_FS_CHECK(ok, std::is_same_v<
                       std::remove_const_t<
                           decltype(fs_holder<"">::value)>,
                       fixed_string<1>>);

    // ... and its bytes, terminating null included
    D_FS_CHECK(ok, fs_bytes_equal(fs_holder<"abc">::value, "abc"));
    D_FS_CHECK(ok, fs_bytes_equal(fs_holder<"">::value, ""));
    D_FS_CHECK(ok, fs_bytes_equal(fs_holder<fs_long>::value,
                                  "the quick brown fox jumped over"));
    D_FS_CHECK(ok, fs_null_terminated(fs_holder<"abc">::value));

    // the accessors behave as they do on an ordinary object
    D_FS_CHECK(ok, fs_holder<"abc">::value.size() == 3);
    D_FS_CHECK(ok, fs_holder<"abc">::value.view() ==
                   std::string_view("abc"));
    D_FS_CHECK(ok, std::strcmp(fs_holder<"abc">::value.c_str(),
                               "abc") == 0);
    D_FS_CHECK(ok, fs_extent(fs_holder<"abc">::value) == 4);

    // the empty payload survives
    D_FS_CHECK(ok, fs_holder<"">::value.size() == 0);
    D_FS_CHECK(ok, fs_holder<"">::value.view().empty());
    D_FS_CHECK(ok, fs_holder<"">::value.data[0] == '\0');

    // and so does an interior null - the shape a C-string shortcut
    // anywhere in the pipeline would have truncated
    D_FS_CHECK(ok, fs_bytes_equal(fs_holder<fs_embedded>::value, "a\0b"));
    D_FS_CHECK(ok, fs_holder<fs_embedded>::value.size() == 3);
    D_FS_CHECK(ok, fs_holder<fs_embedded>::value.data[1] == '\0');
    D_FS_CHECK(ok, fs_holder<fs_embedded>::value.data[2] == 'b');
    D_FS_CHECK(ok, std::strlen(fs_holder<fs_embedded>::value.c_str())
                   == 1);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
