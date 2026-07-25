#include <vector>

#include "parse_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_parse_parseable_aliases
  Verifies that the parseable carrier is a pure alias pair.
  Tests the following:
  - minor and major name exactly the two type arguments,
  - distinct arguments, identical arguments, and qualified arguments are
    all carried verbatim,
  - the carrier adds no state (it is an empty type),
  - the aliases survive being read back through a dependent context.
*/
bool
tests_parse_parseable_aliases()
{
    using char_to_string = dp::parseable<char, std::string>;
    using byte_to_vector = dp::parseable<unsigned char, std::vector<int> >;
    using int_to_int     = dp::parseable<int, int>;
    using cv_pair        = dp::parseable<const char, const std::string>;

    // the ordinary token -> aggregate case
    D_PA_CHECK(std::is_same<char_to_string::minor, char>::value);
    D_PA_CHECK(std::is_same<char_to_string::major, std::string>::value);

    // a non-character token stream mapping onto a container
    D_PA_CHECK(std::is_same<byte_to_vector::minor, unsigned char>::value);
    D_PA_CHECK(
        std::is_same<byte_to_vector::major, std::vector<int> >::value);

    // the degenerate domain where minor and major coincide
    D_PA_CHECK(std::is_same<int_to_int::minor, int>::value);
    D_PA_CHECK(std::is_same<int_to_int::major, int>::value);
    D_PA_CHECK(std::is_same<int_to_int::minor, int_to_int::major>::value);

    // qualifiers are carried, not stripped
    D_PA_CHECK(std::is_same<cv_pair::minor, const char>::value);
    D_PA_CHECK(std::is_same<cv_pair::major, const std::string>::value);
    D_PA_CHECK(!std::is_same<cv_pair::minor, char>::value);

    // the carrier is a pure type-level pair, carrying no storage
    D_PA_CHECK(std::is_empty<char_to_string>::value);
    D_PA_CHECK(std::is_class<char_to_string>::value);

    return true;
}

/*
tests_parse_is_parseable_primary
  Verifies the primary is_parseable template rejects everything that has no
nested alias pair.
  Tests the following:
  - fundamental types, void, and enums are not parseable,
  - pointers, arrays, references, and function types are not parseable,
  - an ordinary class with data members but no aliases is not parseable,
  - a standard container is not parseable.
*/
bool
tests_parse_is_parseable_primary()
{
    // fundamental and void types carry no alias pair
    D_PA_CHECK(!dp::is_parseable<int>::value);
    D_PA_CHECK(!dp::is_parseable<char>::value);
    D_PA_CHECK(!dp::is_parseable<unsigned char>::value);
    D_PA_CHECK(!dp::is_parseable<double>::value);
    D_PA_CHECK(!dp::is_parseable<bool>::value);
    D_PA_CHECK(!dp::is_parseable<void>::value);
    D_PA_CHECK(!dp::is_parseable<std::size_t>::value);

    // pointers other than the three explicitly specialised character
    // pointers are rejected
    D_PA_CHECK(!dp::is_parseable<int*>::value);
    D_PA_CHECK(!dp::is_parseable<void*>::value);
    D_PA_CHECK(!dp::is_parseable<char**>::value);
    D_PA_CHECK(!dp::is_parseable<unsigned char*>::value);
    D_PA_CHECK(!dp::is_parseable<wchar_t*>::value);

    // arrays, references, and functions have no members at all
    D_PA_CHECK(!dp::is_parseable<char[8]>::value);
    D_PA_CHECK(!dp::is_parseable<int&>::value);
    D_PA_CHECK(!dp::is_parseable<int (*)(int)>::value);

    // a plain aggregate and a standard container are both rejected
    D_PA_CHECK(!dp::is_parseable<pa_probe_plain>::value);
    D_PA_CHECK(!dp::is_parseable<std::vector<char> >::value);

    return true;
}

/*
tests_parse_is_parseable_detection
  Verifies the SFINAE specialisation demands BOTH nested aliases.
  Tests the following:
  - a type exposing minor and major is detected,
  - a type exposing only minor is rejected,
  - a type exposing only major is rejected,
  - the degenerate minor == major pair is still detected,
  - parseable itself satisfies the trait that describes it.
*/
bool
tests_parse_is_parseable_detection()
{
    // both aliases present: the detection idiom matches
    D_PA_CHECK(dp::is_parseable<pa_probe_pair>::value);

    // exactly one alias present: void_t<...> is ill-formed, so the
    // specialisation is discarded and the primary answers false
    D_PA_CHECK(!dp::is_parseable<pa_probe_minor_only>::value);
    D_PA_CHECK(!dp::is_parseable<pa_probe_major_only>::value);

    // a legal degenerate domain is still a domain
    D_PA_CHECK(dp::is_parseable<pa_probe_same>::value);

    // the carrier the trait exists to describe satisfies it
    D_PA_CHECK(dp::is_parseable<dp::parseable<char, std::string> >::value);
    D_PA_CHECK(dp::is_parseable<dp::parseable<int, int> >::value);

    return true;
}

/*
tests_parse_is_parseable_access_and_inheritance
  Verifies how detection interacts with C++ name lookup.
  Tests the following:
  - private aliases are not nameable and so are not detected,
  - aliases inherited from a base ARE detected,
  - an inheriting type reports the base's minor and major.
*/
bool
tests_parse_is_parseable_access_and_inheritance()
{
    // access control participates: the aliases exist but cannot be named
    D_PA_CHECK(!dp::is_parseable<pa_probe_private>::value);

    // nested-name lookup finds inherited members, so deriving from
    // parseable is a legitimate way to declare a domain
    D_PA_CHECK(dp::is_parseable<pa_probe_derived>::value);
    D_PA_CHECK(std::is_same<pa_probe_derived::minor, unsigned char>::value);
    D_PA_CHECK(std::is_same<pa_probe_derived::major, std::string>::value);

    // and the derivation is visible to the type system
    D_PA_CHECK(
        (std::is_base_of<dp::parseable<unsigned char, std::string>,
                         pa_probe_derived>::value));

    return true;
}

/*
tests_parse_is_parseable_explicit_specialisations
  Verifies the three hand-written specialisations.
  Tests the following:
  - std::string, const char*, and char* are all parseable,
  - none of them carries nested aliases, so the answer can only come from
    the explicit specialisations,
  - near-miss spellings of those types are not swept in.
*/
bool
tests_parse_is_parseable_explicit_specialisations()
{
    // the three character-stream keys the module maps by hand
    D_PA_CHECK(dp::is_parseable<std::string>::value);
    D_PA_CHECK(dp::is_parseable<const char*>::value);
    D_PA_CHECK(dp::is_parseable<char*>::value);

    // none of them has nested aliases, so only an explicit specialisation
    // can be answering -- confirm the SFINAE arm could not have fired
    D_PA_CHECK(!dp::is_parseable<std::wstring>::value);
    D_PA_CHECK(!dp::is_parseable<signed char*>::value);
    D_PA_CHECK(!dp::is_parseable<const unsigned char*>::value);

    return true;
}

/*
tests_parse_is_parseable_no_cv_ref_decay
  PINNED BEHAVIOUR.  Verifies that is_parseable matches its argument
exactly rather than decaying it, so a qualified or referenced spelling of a
parseable type is NOT parseable.  These assertions are expected to invert if
the module ever adds a std::decay step.
  Tests the following:
  - const / volatile qualified std::string is rejected,
  - references to parseable types are rejected,
  - a top-level const on the pointer keys is rejected,
  - the unqualified spellings still succeed, isolating the cause.
*/
bool
tests_parse_is_parseable_no_cv_ref_decay()
{
    // the unqualified spellings are the ones that are specialised
    D_PA_CHECK(dp::is_parseable<std::string>::value);
    D_PA_CHECK(dp::is_parseable<char*>::value);

    // adding cv-qualification changes the type, and the specialisation is
    // keyed on the exact type -- so detection is lost
    D_PA_CHECK(!dp::is_parseable<const std::string>::value);
    D_PA_CHECK(!dp::is_parseable<volatile std::string>::value);
    D_PA_CHECK(!dp::is_parseable<char* const>::value);
    D_PA_CHECK(!dp::is_parseable<const char* const>::value);

    // references likewise
    D_PA_CHECK(!dp::is_parseable<std::string&>::value);
    D_PA_CHECK(!dp::is_parseable<const std::string&>::value);
    D_PA_CHECK(!dp::is_parseable<std::string&&>::value);

    // the same holds for the SFINAE arm, not just the explicit ones
    D_PA_CHECK(dp::is_parseable<pa_probe_pair>::value);
    D_PA_CHECK(!dp::is_parseable<pa_probe_pair&>::value);

    return true;
}

/*
tests_parse_is_parseable_trait_shape
  Verifies is_parseable presents the standard bool-trait surface, so generic
code can consume it wherever an integral_constant is expected.
  Tests the following:
  - it derives from std::true_type / std::false_type as appropriate,
  - value_type is bool and type is the matching integral_constant,
  - the constexpr conversion operator and call operator both work,
  - the second template parameter defaults, so one argument suffices.
*/
bool
tests_parse_is_parseable_trait_shape()
{
    using yes = dp::is_parseable<std::string>;
    using no  = dp::is_parseable<int>;

    // the canonical bool-trait bases
    D_PA_CHECK((std::is_base_of<std::true_type, yes>::value));
    D_PA_CHECK((std::is_base_of<std::false_type, no>::value));

    // the integral_constant member surface
    D_PA_CHECK(std::is_same<yes::value_type, bool>::value);
    D_PA_CHECK(std::is_same<yes::type, std::true_type>::value);
    D_PA_CHECK(std::is_same<no::type, std::false_type>::value);

    // usable as a value, both by conversion and by call
    D_PA_CHECK(static_cast<bool>(yes()) == true);
    D_PA_CHECK(no()() == false);

    // the void default on the second parameter means the one-argument
    // spelling and the explicit spelling name the same trait
    D_PA_CHECK((std::is_same<dp::is_parseable<int>,
                             dp::is_parseable<int, void> >::value));

    return true;
}

/*
tests_parse_traits_specialisations
  Verifies the parse_traits lookup for the three mapped keys.
  Tests the following:
  - every mapped key parses from char,
  - each key's major is the key itself,
  - each specialisation derives from the corresponding parseable,
  - the mapping agrees with is_parseable on exactly those keys.
*/
bool
tests_parse_traits_specialisations()
{
    // std::string: a character stream folded into a string
    D_PA_CHECK(std::is_same<dp::parse_traits<std::string>::minor,
                            char>::value);
    D_PA_CHECK(std::is_same<dp::parse_traits<std::string>::major,
                            std::string>::value);
    D_PA_CHECK((std::is_base_of<dp::parseable<char, std::string>,
                                dp::parse_traits<std::string> >::value));

    // const char*: the same minor, a C-string major
    D_PA_CHECK(std::is_same<dp::parse_traits<const char*>::minor,
                            char>::value);
    D_PA_CHECK(std::is_same<dp::parse_traits<const char*>::major,
                            const char*>::value);
    D_PA_CHECK((std::is_base_of<dp::parseable<char, const char*>,
                                dp::parse_traits<const char*> >::value));

    // char*: likewise, with the mutable spelling preserved
    D_PA_CHECK(std::is_same<dp::parse_traits<char*>::minor, char>::value);
    D_PA_CHECK(std::is_same<dp::parse_traits<char*>::major, char*>::value);
    D_PA_CHECK((std::is_base_of<dp::parseable<char, char*>,
                                dp::parse_traits<char*> >::value));

    // every mapped key is a key is_parseable also accepts
    D_PA_CHECK(dp::is_parseable<std::string>::value);
    D_PA_CHECK(dp::is_parseable<const char*>::value);
    D_PA_CHECK(dp::is_parseable<char*>::value);

    // the three majors are pairwise distinct, so the mapping is injective
    // on the keys it covers
    D_PA_CHECK(!(std::is_same<dp::parse_traits<char*>::major,
                              dp::parse_traits<const char*>::major>::value));

    return true;
}

/*
tests_parse_traits_primary_declared
  Verifies the parse_traits primary template is declared but left undefined,
so an unmapped key is an incomplete type rather than a silently wrong
mapping.
  Tests the following:
  - the primary template name exists and can be named,
  - an unmapped key forms a well-formed pointer type (pointers to
    incomplete types are legal), which is only possible if the primary is
    declared,
  - the mapped keys are, by contrast, complete.
*/
bool
tests_parse_traits_primary_declared()
{
    // naming a pointer to an unmapped specialisation is legal precisely
    // because the primary template is declared; it would not compile if the
    // template did not exist, and it does not require a definition
    dp::parse_traits<int>*         unmapped_int    = nullptr;
    dp::parse_traits<std::wstring>* unmapped_wide  = nullptr;

    D_PA_CHECK(unmapped_int == nullptr);
    D_PA_CHECK(unmapped_wide == nullptr);

    // the mapped keys are complete: sizeof only compiles on a complete type
    D_PA_CHECK(sizeof(dp::parse_traits<std::string>) > 0);
    D_PA_CHECK(sizeof(dp::parse_traits<const char*>) > 0);
    D_PA_CHECK(sizeof(dp::parse_traits<char*>) > 0);

    // being empty carriers, the complete specialisations are minimal
    D_PA_CHECK(std::is_empty<dp::parse_traits<std::string> >::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
