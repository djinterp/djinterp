// djinterp [test] : pattern_tests_traits.cpp
//   The pattern traits (section V) and the C++20 concept (section VII): the
// seven SFINAE detectors, the composite is_pattern / is_pattern_v, and the
// pattern_type concept. Each detector is exercised against the conforming
// tag_pattern and against local fixtures that each drop exactly one requirement.

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
    struct plain_conforming
    {
        using input_type = int;
        using key_type   = std::string;
        using value_type = int;

        bool do_match(const int&) const { return true; }

        pattern_match_result<std::string, int>
        do_extract(const int&) const
        {
            return pattern_match_result<std::string, int>(DPatternStatusNoMatch);
        }

        int do_render(const pattern_capture_map<std::string, int>&) const { return 0; }

        int do_rewrite(const int& _in, const std::string&, const int&) const { return _in; }
    };

    // each of the following drops exactly one requirement.
    struct no_input_type
    {
        using key_type   = std::string;
        using value_type = int;
        bool do_match(const int&) const { return true; }
    };

    struct no_do_match
    {
        using input_type = int;
        using key_type   = std::string;
        using value_type = int;
        // no do_match
    };

    struct wrong_do_match   // do_match present but not bool-convertible
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
bool
tests_trait_has_input_type()
{
    static_assert( tr::pattern_has_input_type<tag_pattern>::value,   "present");
    static_assert(!tr::pattern_has_input_type<no_input_type>::value, "absent");

    return ( tr::pattern_has_input_type<tag_pattern>::value &&
            !tr::pattern_has_input_type<no_input_type>::value );
}

/*
tests_trait_has_key_type
  Verifies pattern_has_key_type.
  Tests the following:
  - true for tag_pattern, false for a type lacking key_type (int)
*/
bool
tests_trait_has_key_type()
{
    return ( tr::pattern_has_key_type<tag_pattern>::value &&
            !tr::pattern_has_key_type<int>::value );
}

/*
tests_trait_has_value_type
  Verifies pattern_has_value_type.
  Tests the following:
  - true for tag_pattern, false for a type lacking value_type (int)
*/
bool
tests_trait_has_value_type()
{
    return ( tr::pattern_has_value_type<tag_pattern>::value &&
            !tr::pattern_has_value_type<int>::value );
}

/*
tests_trait_has_do_match
  Verifies pattern_has_do_match (requires a bool-convertible do_match).
  Tests the following:
  - true for tag_pattern
  - false when do_match is absent
  - false when do_match returns a non-bool-convertible type
*/
bool
tests_trait_has_do_match()
{
    bool ok = true;

    ok = ok && ( tr::pattern_has_do_match<tag_pattern>::value);
    ok = ok && (!tr::pattern_has_do_match<no_do_match>::value);
    ok = ok && (!tr::pattern_has_do_match<wrong_do_match>::value);

    return ok;
}

/*
tests_trait_has_do_extract
  Verifies pattern_has_do_extract.
  Tests the following:
  - true for tag_pattern, false for a type without do_extract
*/
bool
tests_trait_has_do_extract()
{
    return ( tr::pattern_has_do_extract<tag_pattern>::value &&
            !tr::pattern_has_do_extract<no_do_match>::value );
}

/*
tests_trait_has_do_render
  Verifies pattern_has_do_render.
  Tests the following:
  - true for tag_pattern, false for a type without do_render
*/
bool
tests_trait_has_do_render()
{
    return ( tr::pattern_has_do_render<tag_pattern>::value &&
            !tr::pattern_has_do_render<no_do_match>::value );
}

/*
tests_trait_has_do_rewrite
  Verifies pattern_has_do_rewrite.
  Tests the following:
  - true for tag_pattern, false for a type without do_rewrite
*/
bool
tests_trait_has_do_rewrite()
{
    return ( tr::pattern_has_do_rewrite<tag_pattern>::value &&
            !tr::pattern_has_do_rewrite<no_do_match>::value );
}

/*
tests_trait_is_pattern_conforming
  Verifies is_pattern on conforming types.
  Tests the following:
  - true for the base-derived tag_pattern
  - true for a plain conforming type that does not derive from pattern<>
*/
bool
tests_trait_is_pattern_conforming()
{
    static_assert(tr::is_pattern<tag_pattern>::value,      "derived conforms");
    static_assert(tr::is_pattern<plain_conforming>::value, "plain conforms");

    return ( tr::is_pattern<tag_pattern>::value &&
             tr::is_pattern<plain_conforming>::value );
}

/*
tests_trait_is_pattern_nonconforming
  Verifies is_pattern on non-conforming types.
  Tests the following:
  - false for an unrelated type (int)
  - false for each type missing exactly one requirement
*/
bool
tests_trait_is_pattern_nonconforming()
{
    bool ok = true;

    ok = ok && (!tr::is_pattern<int>::value);
    ok = ok && (!tr::is_pattern<no_input_type>::value);
    ok = ok && (!tr::is_pattern<no_do_match>::value);
    ok = ok && (!tr::is_pattern<wrong_do_match>::value);

    return ok;
}

/*
tests_trait_is_pattern_v
  Verifies the is_pattern_v alias.
  Tests the following:
  - is_pattern_v mirrors is_pattern<>::value for conforming and non-conforming
*/
bool
tests_trait_is_pattern_v()
{
    bool ok = true;

    ok = ok && (tr::is_pattern_v<tag_pattern> == tr::is_pattern<tag_pattern>::value);
    ok = ok && (tr::is_pattern_v<int> == tr::is_pattern<int>::value);
    ok = ok && (tr::is_pattern_v<tag_pattern>);
    ok = ok && (!tr::is_pattern_v<int>);

    return ok;
}

/*
tests_trait_pattern_type_concept
  Verifies the C++20 pattern_type concept.
  Tests the following:
  - satisfied by a conforming type and not by a non-conforming one (C++20+)
*/
bool
tests_trait_pattern_type_concept()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert( pattern_type<tag_pattern>, "conforms");
    static_assert(!pattern_type<int>,         "does not");

    return ( pattern_type<tag_pattern> && !pattern_type<int> );
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
