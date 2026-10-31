/*******************************************************************************
* djinterp [cpp]                                        pattern_tests_traits.cpp
*
*
* path:      /tests/djinterp/cpp/paradigm/pattern/pattern_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : pattern_tests_traits.cpp
//   The pattern traits (section V) and the C++20 concept (section VII): the
// seven SFINAE detectors, the composite is_pattern / is_pattern_v, and the
// pattern_type concept. Each detector is exercised against the conforming
// DTagPattern and against local fixtures that each drop exactly one requirement.

// std
#include <string>
// djinterp
#include "pattern_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace tr = ::djinterp::traits;

namespace
{
    // a plain conforming pattern that does NOT derive from pattern<> - proves
    // the traits check the protocol shape, not the base class.
    struct DPlainConforming
    {
        using input_type = int;
        using key_type   = std::string;
        using value_type = int;

        bool do_match(const int&) const { return true; }

        DPatternMatchResult<std::string, int> do_extract(
            const int&
        ) const
        {
            return DPatternMatchResult<std::string, int>(DPatternStatusNoMatch);
        }

        int do_render(const DPatternCaptureMap<std::string, int>&) const { return 0; }

        int do_rewrite(const int& _in, const std::string&, const int&) const { return _in; }
    };

    // each of the following drops exactly one requirement.
    struct no_input_type
    {
        using key_type   = std::string;
        using value_type = int;
        bool do_match(const int&) const { return true; }
    };

    struct DNoDoMatch
    {
        using input_type = int;
        using key_type   = std::string;
        using value_type = int;
        // no do_match
    };

    struct DWrongDoMatch   // do_match present but not bool-convertible
    {
        using input_type = int;
        using key_type   = std::string;
        using value_type = int;
        struct opaque {};
        opaque do_match(const int&) const { return opaque{}; }
    };
}


/*
tests_trait_has_input_type
  Verifies pattern_has_input_type.
  Tests the following:
  - true for a type with an input_type typedef, false without
*/
bool tests_trait_has_input_type()
{
    static_assert( tr::pattern_has_input_type<DTagPattern>::value,   "present");
    static_assert(!tr::pattern_has_input_type<no_input_type>::value, "absent");

    return ( tr::pattern_has_input_type<DTagPattern>::value &&
            !tr::pattern_has_input_type<no_input_type>::value );
}

/*
tests_trait_has_key_type
  Verifies pattern_has_key_type.
  Tests the following:
  - true for DTagPattern, false for a type lacking key_type (int)
*/
bool tests_trait_has_key_type()
{
    return ( tr::pattern_has_key_type<DTagPattern>::value &&
            !tr::pattern_has_key_type<int>::value );
}

/*
tests_trait_has_value_type
  Verifies pattern_has_value_type.
  Tests the following:
  - true for DTagPattern, false for a type lacking value_type (int)
*/
bool tests_trait_has_value_type()
{
    return ( tr::pattern_has_value_type<DTagPattern>::value &&
            !tr::pattern_has_value_type<int>::value );
}

/*
tests_trait_has_do_match
  Verifies pattern_has_do_match (requires a bool-convertible do_match).
  Tests the following:
  - true for DTagPattern
  - false when do_match is absent
  - false when do_match returns a non-bool-convertible type
*/
bool tests_trait_has_do_match()
{
    bool ok = true;

    ok = ok && ( tr::pattern_has_do_match<DTagPattern>::value);
    ok = ok && (!tr::pattern_has_do_match<DNoDoMatch>::value);
    ok = ok && (!tr::pattern_has_do_match<DWrongDoMatch>::value);

    return ok;
}

/*
tests_trait_has_do_extract
  Verifies pattern_has_do_extract.
  Tests the following:
  - true for DTagPattern, false for a type without do_extract
*/
bool tests_trait_has_do_extract()
{
    return ( tr::pattern_has_do_extract<DTagPattern>::value &&
            !tr::pattern_has_do_extract<DNoDoMatch>::value );
}

/*
tests_trait_has_do_render
  Verifies pattern_has_do_render.
  Tests the following:
  - true for DTagPattern, false for a type without do_render
*/
bool tests_trait_has_do_render()
{
    return ( tr::pattern_has_do_render<DTagPattern>::value &&
            !tr::pattern_has_do_render<DNoDoMatch>::value );
}

/*
tests_trait_has_do_rewrite
  Verifies pattern_has_do_rewrite.
  Tests the following:
  - true for DTagPattern, false for a type without do_rewrite
*/
bool tests_trait_has_do_rewrite()
{
    return ( tr::pattern_has_do_rewrite<DTagPattern>::value &&
            !tr::pattern_has_do_rewrite<DNoDoMatch>::value );
}

/*
tests_trait_is_pattern_conforming
  Verifies is_pattern on conforming types.
  Tests the following:
  - true for the base-derived DTagPattern
  - true for a plain conforming type that does not derive from pattern<>
*/
bool tests_trait_is_pattern_conforming()
{
    static_assert(tr::is_pattern<DTagPattern>::value,      "derived conforms");
    static_assert(tr::is_pattern<DPlainConforming>::value, "plain conforms");

    return ( tr::is_pattern<DTagPattern>::value &&
             tr::is_pattern<DPlainConforming>::value );
}

/*
tests_trait_is_pattern_nonconforming
  Verifies is_pattern on non-conforming types.
  Tests the following:
  - false for an unrelated type (int)
  - false for each type missing exactly one requirement
*/
bool tests_trait_is_pattern_nonconforming()
{
    bool ok = true;

    ok = ok && (!tr::is_pattern<int>::value);
    ok = ok && (!tr::is_pattern<no_input_type>::value);
    ok = ok && (!tr::is_pattern<DNoDoMatch>::value);
    ok = ok && (!tr::is_pattern<DWrongDoMatch>::value);

    return ok;
}

/*
tests_trait_is_pattern_v
  Verifies the is_pattern_v alias.
  Tests the following:
  - is_pattern_v mirrors is_pattern<>::value for conforming and non-conforming
*/
bool tests_trait_is_pattern_v()
{
    bool ok = true;

    ok = ok && (tr::is_pattern_v<DTagPattern> == tr::is_pattern<DTagPattern>::value);
    ok = ok && (tr::is_pattern_v<int> == tr::is_pattern<int>::value);
    ok = ok && (tr::is_pattern_v<DTagPattern>);
    ok = ok && (!tr::is_pattern_v<int>);

    return ok;
}

/*
tests_trait_pattern_type_concept
  Verifies the C++20 pattern_type concept.
  Tests the following:
  - satisfied by a conforming type and not by a non-conforming one (C++20+)
*/
bool tests_trait_pattern_type_concept()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert( pattern_type<DTagPattern>, "conforms");
    static_assert(!pattern_type<int>,         "does not");

    return ( pattern_type<DTagPattern> && !pattern_type<int> );
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
