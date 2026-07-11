/******************************************************************************
* djinterp [test]                                             pattern_tests.hpp
*
*   Unit-test suite header for inc/djinterp/core/paradigm/pattern/pattern.hpp -
* the type-agnostic pattern primitive (capture map, match result, the CRTP
* four-face base, the SFINAE traits, and the and/or/not combinators). Two
* faces, selected by DTEST_SPEC_MODE:
*
*     - default face: the flat declarations of every tests_* predicate plus the
*       shared concrete-pattern fixture (tag_pattern) the base and combinator
*       sections build against. Section-local non-conforming fixtures live in
*       anonymous namespaces in the owning TU.
*
*     - spec-provider face (DTEST_SPEC_MODE defined): the above PLUS
*       pattern_spec(), which the runner hands to run_module. Only the runner
*       defines DTEST_SPEC_MODE, so the spec is compiled exactly once.
*
*   LANGUAGE FLOOR: pattern.hpp uses std::void_t and [[nodiscard]]
* unconditionally, so C++17 is the floor. The suite targets C++17 and up; the
* only dialect-conditional surface is the C++20 pattern_type concept, guarded by
* D_ENV_CPP_FEATURE_LANG_CONCEPTS.
*
*   BUILD PREREQUISITE (see the accompanying report): as shipped, the CRTP base
* pattern<_Derived> names _Derived::input_type / key_type / value_type in its
* public method SIGNATURES. Because the base is instantiated as part of a
* derived pattern's base-clause - while _Derived is still incomplete - those
* signatures fail to instantiate on a conforming compiler (GCC/Clang), so no
* concrete pattern (nor any instantiated combinator) compiles. MSVC's lax CRTP
* handling masks it. The fix defers each face's signature to call time by making
* it a member template <typename _D = derived_type>; the suite targets the fixed
* header (patch + fixed header accompany this suite).
*
* path:      /tests/djinterp/core/paradigm/pattern/pattern_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.10
******************************************************************************/

#ifndef DJINTERP_TEST_PATTERN_TESTS_
#define DJINTERP_TEST_PATTERN_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <utility>
// djinterp
#include "pattern.hpp"                        // the header under test (bare form)
#include "djinterp/test/test_defaults.hpp"    // module_spec, run_module


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                SHARED CONCRETE-PATTERN FIXTURE                          ///
///////////////////////////////////////////////////////////////////////////////

// tag_pattern
//   type: a fully configurable conforming pattern over int input with
// std::string keys and int values. It is parameterised at construction by a
// single key/value binding and a match flag, which lets a test dictate exactly
// what each face returns:
//     - do_match     : returns the configured flag
//     - do_extract   : {key -> value} on a match, NoMatch otherwise
//     - do_render    : the value bound to the configured key, or a sentinel
//     - do_rewrite   : replaces the input with the new value iff the key matches
// Two tag_patterns with different keys compose to exercise the combinators'
// capture merge / selection and render / rewrite delegation rules.
struct tag_pattern
    : pattern<tag_pattern>
{
    using input_type = int;
    using key_type   = std::string;
    using value_type = int;

    using capture_map_type  = pattern_capture_map<std::string, int>;
    using match_result_type = pattern_match_result<std::string, int>;

    std::string m_key;
    int         m_value;
    bool        m_matches;

    tag_pattern(std::string _key,
                int         _value,
                bool        _matches = true)
        : m_key    (std::move(_key)),
          m_value  (_value),
          m_matches(_matches)
    {}

    D_NODISCARD
    bool
    do_match(const int& /*_in*/) const
    {
        return m_matches;
    }

    D_NODISCARD
    match_result_type
    do_extract(const int& /*_in*/) const
    {
        if (!m_matches)
        {
            return match_result_type(DPatternStatusNoMatch);
        }

        capture_map_type c;
        c.set(m_key, m_value);

        return match_result_type(std::move(c));
    }

    D_NODISCARD
    int
    do_render(const capture_map_type& _c) const
    {
        const int* p = _c.find(m_key);

        return (p != nullptr) ? *p : -99;
    }

    D_NODISCARD
    int
    do_rewrite(const int&         _in,
               const std::string& _key,
               const int&         _value) const
    {
        return (_key == m_key) ? _value : _in;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                TEST DECLARATIONS                                         ///
///////////////////////////////////////////////////////////////////////////////

// -- section: status codes + capture map (pattern_tests_capture.cpp)
bool tests_status_codes();
bool tests_capture_construction();
bool tests_map_capacity();
bool tests_map_find();
bool tests_map_find_mutable();
bool tests_map_has();
bool tests_map_set_replace();
bool tests_map_set_move();
bool tests_map_erase();
bool tests_map_merge();
bool tests_map_iteration();
bool tests_map_entries();

// -- section: match result (pattern_tests_match_result.cpp)
bool tests_match_result_default();
bool tests_match_result_success();
bool tests_match_result_failure();
bool tests_match_result_operator_bool();
bool tests_match_result_captures();

// -- section: CRTP base faces (pattern_tests_base.cpp)
bool tests_base_match();
bool tests_base_operator_call();
bool tests_base_extract_matched();
bool tests_base_extract_unmatched();
bool tests_base_render();
bool tests_base_rewrite_matching();
bool tests_base_rewrite_nonmatching();

// -- section: capability traits + concept (pattern_tests_traits.cpp)
bool tests_trait_has_input_type();
bool tests_trait_has_key_type();
bool tests_trait_has_value_type();
bool tests_trait_has_do_match();
bool tests_trait_has_do_extract();
bool tests_trait_has_do_render();
bool tests_trait_has_do_rewrite();
bool tests_trait_is_pattern_conforming();
bool tests_trait_is_pattern_nonconforming();
bool tests_trait_is_pattern_v();
bool tests_trait_pattern_type_concept();

// -- section: combinators (pattern_tests_combinators.cpp)
bool tests_and_match();
bool tests_and_extract_merge();
bool tests_and_extract_collision();
bool tests_and_short_circuit();
bool tests_and_render();
bool tests_and_rewrite();
bool tests_and_introspection();
bool tests_or_match();
bool tests_or_extract_left();
bool tests_or_extract_right();
bool tests_or_render();
bool tests_or_rewrite();
bool tests_not_match();
bool tests_not_extract();
bool tests_not_render();
bool tests_not_rewrite();
bool tests_not_introspection();
bool tests_combinator_nesting();
bool tests_combinator_factory_decay();


///////////////////////////////////////////////////////////////////////////////
///                SPEC-PROVIDER FACE  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// pattern_spec
//   function: assembles the module_spec - one block per semantic section of
// pattern.hpp, one test per tests_* predicate. Compiled only in the runner TU.
inline ::djinterp::test::module_spec
pattern_spec()
{
    namespace dt = ::djinterp::test;

    dt::module_spec m;

    m.name       = "pattern";
    m.descriptor =
        "Type-agnostic pattern primitive: status codes, the capture map, the "
        "match result, the CRTP four-face base, the SFINAE conformance traits, "
        "and the and/or/not combinators.";

    m.blocks = std::vector<dt::block_spec>
    {
        dt::block_spec
        {
            "status codes & capture map",
            "status constants, pattern_capture, and pattern_capture_map.",
            std::vector<dt::test_spec>
            {
                { "status codes",     "the standard status constants.",     &tests_status_codes         },
                { "capture ctors",    "default and value/move construction.", &tests_capture_construction },
                { "map capacity",     "empty / size / clear.",              &tests_map_capacity         },
                { "map find",         "const find returns a value pointer.", &tests_map_find             },
                { "map find mutable", "mutable find allows in-place edit.",  &tests_map_find_mutable     },
                { "map has",          "key presence query.",                &tests_map_has              },
                { "map set replace",  "set inserts then replaces + chains.", &tests_map_set_replace      },
                { "map set move",     "the rvalue set overload.",           &tests_map_set_move         },
                { "map erase",        "erase removes and reports.",         &tests_map_erase            },
                { "map merge",        "merge with / without overwrite.",    &tests_map_merge            },
                { "map iteration",    "begin/end visit every entry.",       &tests_map_iteration        },
                { "map entries",      "storage access exposes the vector.", &tests_map_entries          }
            }
        },
        dt::block_spec
        {
            "match result",
            "pattern_match_result construction and inspection.",
            std::vector<dt::test_spec>
            {
                { "default",       "default is an unmatched NoMatch.",   &tests_match_result_default       },
                { "success",       "capture-map ctor is a matched Ok.",  &tests_match_result_success       },
                { "failure",       "status ctor is unmatched with code.", &tests_match_result_failure      },
                { "operator bool", "bool conversion reflects matched.",  &tests_match_result_operator_bool },
                { "captures",      "the capture map travels with it.",   &tests_match_result_captures      }
            }
        },
        dt::block_spec
        {
            "CRTP base faces",
            "the four public faces via a conforming pattern.",
            std::vector<dt::test_spec>
            {
                { "match",             "match forwards to do_match.",         &tests_base_match              },
                { "operator()",        "predicate face aliases match.",       &tests_base_operator_call      },
                { "extract (matched)", "extract yields captures on a match.",  &tests_base_extract_matched   },
                { "extract (unmatched)","extract yields no match otherwise.",  &tests_base_extract_unmatched },
                { "render",            "render forwards to do_render.",       &tests_base_render             },
                { "rewrite (match)",   "rewrite replaces the keyed value.",   &tests_base_rewrite_matching  },
                { "rewrite (no match)","rewrite passes through other keys.",  &tests_base_rewrite_nonmatching }
            }
        },
        dt::block_spec
        {
            "capability traits & concept",
            "the seven SFINAE detectors, is_pattern, and the C++20 concept.",
            std::vector<dt::test_spec>
            {
                { "has_input_type",   "detects the input_type typedef.",     &tests_trait_has_input_type          },
                { "has_key_type",     "detects the key_type typedef.",       &tests_trait_has_key_type            },
                { "has_value_type",   "detects the value_type typedef.",     &tests_trait_has_value_type          },
                { "has_do_match",     "detects a bool do_match.",            &tests_trait_has_do_match            },
                { "has_do_extract",   "detects do_extract.",                 &tests_trait_has_do_extract          },
                { "has_do_render",    "detects do_render.",                  &tests_trait_has_do_render           },
                { "has_do_rewrite",   "detects do_rewrite.",                 &tests_trait_has_do_rewrite          },
                { "is_pattern (yes)", "true for a conforming type.",         &tests_trait_is_pattern_conforming   },
                { "is_pattern (no)",  "false when a requirement is missing.", &tests_trait_is_pattern_nonconforming },
                { "is_pattern_v",     "the _v alias mirrors ::value.",       &tests_trait_is_pattern_v            },
                { "pattern_type",     "the C++20 concept (C++20+).",         &tests_trait_pattern_type_concept    }
            }
        },
        dt::block_spec
        {
            "combinators",
            "pattern_and / pattern_or / pattern_not and nesting.",
            std::vector<dt::test_spec>
            {
                { "and match",          "matches iff both sides match.",       &tests_and_match             },
                { "and extract merge",  "merges both capture maps.",           &tests_and_extract_merge     },
                { "and collision",      "right-hand keys win on collision.",   &tests_and_extract_collision },
                { "and short-circuit",  "a failing side yields no match.",     &tests_and_short_circuit     },
                { "and render",         "render delegates to the right.",      &tests_and_render            },
                { "and rewrite",        "rewrite delegates to the right.",     &tests_and_rewrite           },
                { "and introspection",  "first() / second() expose children.", &tests_and_introspection     },
                { "or match",           "matches iff either side matches.",    &tests_or_match              },
                { "or extract (left)",  "left result wins when it matches.",   &tests_or_extract_left       },
                { "or extract (right)", "right result on left failure.",       &tests_or_extract_right      },
                { "or render",          "render delegates to the left.",       &tests_or_render             },
                { "or rewrite",         "rewrite delegates to the matcher.",   &tests_or_rewrite            },
                { "not match",          "negates the wrapped predicate.",      &tests_not_match             },
                { "not extract",        "empty captures on success.",          &tests_not_extract           },
                { "not render",         "renders a default input.",            &tests_not_render            },
                { "not rewrite",        "returns the input unchanged.",        &tests_not_rewrite           },
                { "not introspection",  "inner() exposes the wrapped pattern.", &tests_not_introspection    },
                { "nesting",            "combinators compose recursively.",    &tests_combinator_nesting    },
                { "factory decay",      "factories decay forwarded patterns.", &tests_combinator_factory_decay }
            }
        }
    };

    return m;
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_PATTERN_TESTS_
