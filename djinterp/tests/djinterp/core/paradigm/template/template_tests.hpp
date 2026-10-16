/*******************************************************************************
* djinterp [test]                                             template_tests.hpp
*
*   Unit-test suite header for inc/djinterp/core/paradigm/template/template.hpp
* - the template-source-sink algebra.  Two faces, selected by DTEST_SPEC_MODE:
*
*     - default face: the flat declarations of every tests_* predicate plus the
*       shared fixtures the section TUs build against.  Each
*       template_tests_*.cpp includes the header this way and DEFINES the
*       predicates it owns.
*
*     - spec-provider face (DTEST_SPEC_MODE defined): the above PLUS
*       template_spec(), which the runner hands to run_module.  Only the runner
*       defines DTEST_SPEC_MODE, so the spec is compiled exactly once.
*
*   LANGUAGE FLOOR: the module leans on C++14 return-type deduction and
* self-suppresses on C++11 (it contributes nothing rather than failing to
* compile).  The suite therefore targets C++14 and up.  The concept section is
* gated on D_ENV_CPP_FEATURE_LANG_CONCEPTS; on a dialect without concepts those
* predicates stand down and report success, so the module spec is stable across
* dialects.
*
*   BUILD PREREQUISITE (see the accompanying report): as shipped, the tagless
* detectors deduce `invoke_result_t<_Fn, ...>` -- probing _Fn as an RVALUE --
* while the algebra stores F by value and invokes it from CONST member
* functions (transformer_helper::operator(), template_system::apply).  A functor
* whose operator() is non-const therefore satisfies is_transformation (and the
* transformation_for concept), and then fails to compile deep inside the algebra
* rather than at the constraint.  The fix probes through `const _Fn&`, matching
* what the algebra actually does.  This suite asserts the sound behaviour and
* targets the fixed header; patch and fixed header accompany it.
*
*
* path:      /tests/djinterp/core/paradigm/template/template_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.11
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_TEMPLATE_TESTS_
#define DJINTERP_TEST_TEMPLATE_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/core/paradigm/template/template.hpp"                       // the header under test (bare form)
#include "djinterp/test/test_defaults.hpp"    // module_spec, run_module


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                SHARED FIXTURES                                          ///
///////////////////////////////////////////////////////////////////////////////
//   Two carriers.  std::string is the readable one and carries most of the
// suite; int is the constexpr one, since std::string is not usable in a
// constant expression before C++20 and the whole module is D_CONSTEXPR.
//
//   Definitions only - no shared mutable state - so the header is safe to
// include from every section TU.

// carrier
//   type: tau.  A template and a source are both inhabitants of it; they are
// roles, not distinct types, and that homogeneity is what the algebra rests on.
using carrier  = std::string;

// err_type
//   type: E, the error arm of the parser sink sigma = (rho x tau) + E.
using err_type = std::string;


// -- transformations F : tau x tau -> sigma ---------------------------------

// concat
//   fixture: F(t, alpha) = t ":" alpha.  The canonical transformation, with the
// sink equal to the carrier (sigma == tau).
struct concat
{
    std::string
    operator()(
        const std::string& _bound_template,
        const std::string& _source
    ) const
    {
        return _bound_template + ":" + _source;
    }
};

// total_len
//   fixture: F(t, alpha) = |t| + |alpha|.  A transformation whose sink is NOT
// the carrier (sigma != tau), which is what keeps the algebra honest about
// deducing sigma rather than assuming it.
struct total_len
{
    std::size_t
    operator()(
        const std::string& _bound_template,
        const std::string& _source
    ) const
    {
        return _bound_template.size() + _source.size();
    }
};


// -- unary stages g : x -> y ------------------------------------------------

// exclaim
//   fixture: a unary stage, string -> string.
struct exclaim
{
    std::string
    operator()(
        const std::string& _x
    ) const
    {
        return _x + "!";
    }
};

// measure
//   fixture: a unary stage that CHANGES the type, string -> size_t.
struct measure
{
    std::size_t
    operator()(
        const std::string& _x
    ) const
    {
        return _x.size();
    }
};

// twice
//   fixture: a unary stage, size_t -> size_t.
struct twice
{
    std::size_t
    operator()(
        std::size_t _x
    ) const
    {
        return _x * 2;
    }
};


// -- ambient-template (Reader) stages f : (t, x) -> y ------------------------

// wrap
//   fixture: an ambient stage; sees the template on every call.
struct wrap
{
    std::string
    operator()(
        const std::string& _bound_template,
        const std::string& _x
    ) const
    {
        return _bound_template + "<" + _x + ">";
    }
};

// suffix_template
//   fixture: a second ambient stage; the template must still be visible to it,
// which is the whole point of the Reader convention.
struct suffix_template
{
    std::string
    operator()(
        const std::string& _bound_template,
        const std::string& _x
    ) const
    {
        return _x + "/" + _bound_template;
    }
};


// -- non-conforming shapes (the traits must reject these) --------------------

// not_callable
//   fixture: no operator() at all.
struct not_callable
{};

// unary_only
//   fixture: callable with ONE source - a source-transformer, not a
// transformation.
struct unary_only
{
    std::string
    operator()(
        const std::string& _source
    ) const
    {
        return _source;
    }
};

// needs_mutable_ref
//   fixture: binary, but demands non-const references.  The algebra hands a
// transformation `const _Tau&` arguments, so this is not a transformation over
// the carrier however plausible its arity looks.
struct needs_mutable_ref
{
    std::string
    operator()(
        std::string&,
        std::string&
    ) const
    {
        return std::string();
    }
};

// mutable_binary
//   fixture: binary and correctly shaped, but its operator() is NOT const.  The
// algebra stores F by value and invokes it from const members, so this cannot
// be held - and the traits must say so.  Probing _Fn as an rvalue (as the
// shipped detector did) reports it as a transformation and defers the failure
// to a hard error deep inside transformer_helper / template_system.
struct mutable_binary
{
    std::string
    operator()(
        const std::string& _bound_template,
        const std::string& _source
    )
    {
        return _bound_template + _source;
    }
};

// mutable_unary
//   fixture: the source-transformer counterpart of mutable_binary.  The kleisli
// helpers store parsers by value and invoke them from const members, and the
// canonical F_t (transformer_helper) is itself const-callable, so a
// source-transformer must be const-callable too.
struct mutable_unary
{
    std::string
    operator()(
        const std::string& _source
    )
    {
        return _source;
    }
};


// -- noexcept witnesses (evaluate carries D_NOEXCEPT_IF) ---------------------

// noexcept_unary
//   fixture: a non-throwing source-transformer.
struct noexcept_unary
{
    int
    operator()(
        const int& _source
    ) const D_NOEXCEPT
    {
        return _source;
    }
};

// throwing_unary
//   fixture: a potentially-throwing source-transformer.
struct throwing_unary
{
    int
    operator()(
        const int& _source
    ) const
    {
        return _source;
    }
};


// -- constexpr fixtures over carrier tau = int -------------------------------
//   Every public entry point in the module is D_CONSTEXPR.  std::string is not
// a literal type before C++20, so the constant-expression tests run over int.

// mul
//   fixture: F(t, alpha) = t * alpha, usable in a constant expression.
struct mul
{
    D_CONSTEXPR int
    operator()(
        const int& _bound_template,
        const int& _source
    ) const
    {
        return _bound_template * _source;
    }
};

// add_one / doubler
//   fixture: constexpr unary stages.
struct add_one
{
    D_CONSTEXPR int
    operator()(
        const int& _x
    ) const
    {
        return _x + 1;
    }
};

struct doubler
{
    D_CONSTEXPR int
    operator()(
        const int& _x
    ) const
    {
        return _x * 2;
    }
};

// reader_add
//   fixture: a constexpr ambient stage, (t, x) -> t + x.
struct reader_add
{
    D_CONSTEXPR int
    operator()(
        const int& _bound_template,
        const int& _x
    ) const
    {
        return _bound_template + _x;
    }
};


// -- parsers: sigma = (rho x tau) + E ----------------------------------------

// int_outcome / char_outcome
//   type: the two parser sinks the fixtures below produce.  They share a source
// type and an error type and differ only in rho, which is exactly the shape
// kleisli_then is specified for.
using int_outcome  = parse_outcome<int,  carrier, err_type>;
using char_outcome = parse_outcome<char, carrier, err_type>;

// is_digit / is_alpha
//   helper: character classification, spelled out rather than pulled from
// <cctype> so the fixtures stay free of locale behaviour.
inline bool
is_digit(
    char _c
)
{
    return (_c >= '0') && (_c <= '9');
}

inline bool
is_alpha(
    char _c
)
{
    return ((_c >= 'a') && (_c <= 'z')) ||
           ((_c >= 'A') && (_c <= 'Z'));
}

// digit_p
//   fixture: an instantiated parser P_t : tau -> parse_outcome.  Consumes one
// leading digit, yielding its value and the rest of the input.
struct digit_p
{
    int_outcome
    operator()(
        const carrier& _source
    ) const
    {
        if (!_source.empty() && is_digit(_source[0]))
        {
            return parse_success<err_type>(
                static_cast<int>(_source[0] - '0'),
                _source.substr(1));
        }

        return parse_failure<int, carrier>(err_type("expected digit"));
    }
};

// alpha_p
//   fixture: a second parser, with a DIFFERENT result type (char) but the same
// source and error types.  Consumes one leading letter.
struct alpha_p
{
    char_outcome
    operator()(
        const carrier& _source
    ) const
    {
        if (!_source.empty() && is_alpha(_source[0]))
        {
            return parse_success<err_type>(
                _source[0],
                _source.substr(1));
        }

        return parse_failure<char, carrier>(err_type("expected alpha"));
    }
};

// digit_grammar
//   fixture: a transformation of parser shape, P : tau x tau -> parse_outcome.
// The template is the grammar (unused here); the source is the input.  This is
// the F of a parser_system.
struct digit_grammar
{
    int_outcome
    operator()(
        const carrier&,
        const carrier& _source
    ) const
    {
        return digit_p()(_source);
    }
};

// second_digit
//   fixture: the parser a continuation hands back - it folds one further digit
// into the value the first parser produced.  Carries that value, which is how
// the suite can prove the continuation actually SAW the first result.
struct second_digit
{
    int accumulated;

    int_outcome
    operator()(
        const carrier& _source
    ) const
    {
        if (!_source.empty() && is_digit(_source[0]))
        {
            return parse_success<err_type>(
                (accumulated * 10) + static_cast<int>(_source[0] - '0'),
                _source.substr(1));
        }

        return parse_failure<int, carrier>(err_type("expected second digit"));
    }
};

// fold_continuation
//   fixture: k : const rho& -> parser, the continuation kleisli_bind takes.
struct fold_continuation
{
    second_digit
    operator()(
        const int& _value
    ) const
    {
        second_digit next;
        next.accumulated = _value;

        return next;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                TEST DECLARATIONS                                         ///
///////////////////////////////////////////////////////////////////////////////

// -- section I: transformation traits (template_tests_traits.cpp)
bool tests_transformation_sink_deduces_sigma();
bool tests_transformation_sink_sigma_differs_from_carrier();
bool tests_transformation_sink_is_nonesuch_when_not_callable();
bool tests_transformation_sink_t_alias();
bool tests_is_transformation_accepts_binary();
bool tests_is_transformation_rejects_unary();
bool tests_is_transformation_rejects_not_callable();
bool tests_is_transformation_rejects_mutable_ref_parameters();
bool tests_is_transformation_rejects_non_const_callable();
bool tests_is_transformation_v_agrees_with_trait();
bool tests_source_transformer_sink_deduces_sigma();
bool tests_source_transformer_sink_is_nonesuch_when_not_callable();
bool tests_source_transformer_sink_t_alias();
bool tests_is_source_transformer_accepts_unary();
bool tests_is_source_transformer_rejects_binary();
bool tests_is_source_transformer_rejects_non_const_callable();
bool tests_is_source_transformer_v_agrees_with_trait();
bool tests_traits_accept_lambdas_and_function_pointers();

// -- section II: the algebra (template_tests_algebra.cpp)
bool tests_evaluate_applies_transformer();
bool tests_evaluate_accepts_rvalue_transformer();
bool tests_evaluate_propagates_noexcept();
bool tests_instantiate_binds_the_template();
bool tests_instantiate_decays_its_arguments();
bool tests_transformer_typedefs();
bool tests_factorization_identity();
bool tests_template_system_apply();
bool tests_template_system_operator_call();
bool tests_template_system_instantiate();
bool tests_template_system_identity();
bool tests_template_system_transformation_accessor();
bool tests_template_system_typedefs();
bool tests_template_system_ctor_is_explicit();
bool tests_template_system_sink_differs_from_carrier();
bool tests_template_system_is_itself_a_transformation();
bool tests_make_template_system();
bool tests_algebra_is_constexpr();

// -- section III: stage pipelines (template_tests_pipelines.cpp)
bool tests_stages_binary_stage_only();
bool tests_stages_one_unary_stage();
bool tests_stages_many_unary_stages();
bool tests_stages_composition_order();
bool tests_stages_is_a_transformation();
bool tests_stages_plugs_into_template_system();
bool tests_stages_decays_its_arguments();
bool tests_reader_stages_single_stage();
bool tests_reader_stages_two_stages();
bool tests_reader_stages_threads_the_template();
bool tests_reader_stages_is_a_transformation();
bool tests_pipelines_are_constexpr();

// -- section IV.i-v: the parser sink (template_tests_parse_outcome.cpp)
bool tests_parse_success_construction();
bool tests_parse_failure_construction();
bool tests_parse_outcome_is_ok_and_is_err();
bool tests_parse_outcome_operator_bool();
bool tests_parse_outcome_operator_bool_is_explicit();
bool tests_parse_outcome_value_and_remaining();
bool tests_parse_outcome_error();
bool tests_parse_outcome_failure_value_initializes_inactive();
bool tests_parse_outcome_typedefs();
bool tests_parse_success_error_type_is_explicit();
bool tests_parse_failure_result_and_source_are_explicit();
bool tests_is_parse_outcome();
bool tests_is_parse_outcome_v_agrees_with_trait();
bool tests_parser_system_pins_the_sink();
bool tests_parser_system_apply_and_instantiate();
bool tests_parse_outcome_is_constexpr();

// -- section IV.vi-vii: kleisli sequencing (template_tests_kleisli.cpp)
bool tests_kleisli_then_threads_the_remaining_source();
bool tests_kleisli_then_keeps_the_second_result();
bool tests_kleisli_then_result_type_is_the_second();
bool tests_kleisli_then_short_circuits_on_first_failure();
bool tests_kleisli_then_propagates_second_failure();
bool tests_kleisli_then_nests();
bool tests_kleisli_bind_passes_the_result_to_the_continuation();
bool tests_kleisli_bind_threads_the_remaining_source();
bool tests_kleisli_bind_short_circuits_on_first_failure();
bool tests_kleisli_bind_propagates_continuation_failure();
bool tests_kleisli_helpers_are_source_transformers();
bool tests_kleisli_decays_its_arguments();

// -- section V: concepts, C++20 (template_tests_concepts.cpp)
bool tests_concept_transformation_for();
bool tests_concept_transformation_for_rejects_non_const_callable();
bool tests_concept_source_transformer_for();
bool tests_concept_source_transformer_for_rejects_non_const_callable();
bool tests_concept_parse_outcome_c();
bool tests_concepts_agree_with_their_traits();
bool tests_concept_constrains_a_template();


///////////////////////////////////////////////////////////////////////////////
///                SPEC-PROVIDER FACE  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// template_spec
//   function: assembles the module_spec - one block per section of the module's
// table of contents, one test per tests_* predicate.  Compiled only in the
// runner TU.
inline ::djinterp::test::module_spec
template_spec()
{
    namespace dt = ::djinterp::test;

    dt::module_spec m;

    m.name       = "template";
    m.descriptor =
        "The template-source-sink algebra: transformation traits, the "
        "curry/evaluate factorization F = ev . (F_hat x id), stage pipelines, "
        "the parser sink (rho x tau) + E with its kleisli sequencing, and the "
        "C++20 concept parallels.";

    m.blocks = std::vector<dt::block_spec>
    {
        dt::block_spec
        {
            "I. transformation traits",
            "sink deduction and the shape predicates built on it.",
            std::vector<dt::test_spec>
            {
                { "sink deduces sigma",        "sigma is read off F(tau, tau).",           &tests_transformation_sink_deduces_sigma            },
                { "sink != carrier",           "sigma need not be tau.",                   &tests_transformation_sink_sigma_differs_from_carrier },
                { "sink is nonesuch",          "an uncallable F has no sink.",             &tests_transformation_sink_is_nonesuch_when_not_callable },
                { "sink_t alias",              "the alias tracks the trait.",              &tests_transformation_sink_t_alias                  },
                { "accepts a binary F",        "F : tau x tau -> sigma.",                  &tests_is_transformation_accepts_binary             },
                { "rejects a unary F",         "arity must be two.",                       &tests_is_transformation_rejects_unary              },
                { "rejects the uncallable",    "no operator(), no transformation.",        &tests_is_transformation_rejects_not_callable       },
                { "rejects mutable refs",      "F is handed const tau&.",                  &tests_is_transformation_rejects_mutable_ref_parameters },
                { "rejects non-const-callable","the algebra invokes F from const.",        &tests_is_transformation_rejects_non_const_callable },
                { "_v agrees",                 "the variable template tracks the trait.",  &tests_is_transformation_v_agrees_with_trait        },
                { "F_t sink deduces sigma",    "sigma is read off g(tau).",                &tests_source_transformer_sink_deduces_sigma        },
                { "F_t sink is nonesuch",      "an uncallable g has no sink.",             &tests_source_transformer_sink_is_nonesuch_when_not_callable },
                { "F_t sink_t alias",          "the alias tracks the trait.",              &tests_source_transformer_sink_t_alias              },
                { "accepts a unary g",         "g : tau -> sigma.",                        &tests_is_source_transformer_accepts_unary          },
                { "rejects a binary g",        "a transformation is not an F_t.",          &tests_is_source_transformer_rejects_binary         },
                { "rejects non-const-callable","F_t is stored and const-invoked.",         &tests_is_source_transformer_rejects_non_const_callable },
                { "F_t _v agrees",             "the variable template tracks the trait.",  &tests_is_source_transformer_v_agrees_with_trait    },
                { "lambdas + fn pointers",     "the ordinary callables all qualify.",      &tests_traits_accept_lambdas_and_function_pointers  }
            }
        },
        dt::block_spec
        {
            "II. the algebra",
            "evaluate, instantiate, template_system - and the identity binding them.",
            std::vector<dt::test_spec>
            {
                { "evaluate",              "ev applies an F_t to a source.",          &tests_evaluate_applies_transformer          },
                { "evaluate (rvalue)",     "ev perfect-forwards its transformer.",    &tests_evaluate_accepts_rvalue_transformer   },
                { "evaluate (noexcept)",   "ev is as noexcept as the call it makes.", &tests_evaluate_propagates_noexcept          },
                { "instantiate",           "F_hat at a point yields F_t.",            &tests_instantiate_binds_the_template        },
                { "instantiate (decay)",   "F and t are stored decayed.",             &tests_instantiate_decays_its_arguments      },
                { "transformer typedefs",  "carrier_type and sink_type.",             &tests_transformer_typedefs                  },
                { "FACTORIZATION",         "ev(F_hat(t), a) == F(t, a).",             &tests_factorization_identity                },
                { "system apply",          "the uncurried F.",                        &tests_template_system_apply                 },
                { "system operator()",     "an alias for apply.",                     &tests_template_system_operator_call         },
                { "system instantiate",    "F_hat, packaged.",                        &tests_template_system_instantiate           },
                { "system IDENTITY",       "apply(t,a) == ev(instantiate(t), a).",    &tests_template_system_identity              },
                { "system transformation", "the underlying F is reachable.",          &tests_template_system_transformation_accessor },
                { "system typedefs",       "carrier, sink, transformation, F_t.",     &tests_template_system_typedefs              },
                { "system ctor explicit",  "an F does not become a system.",          &tests_template_system_ctor_is_explicit      },
                { "system sink != carrier","sigma may differ from tau.",              &tests_template_system_sink_differs_from_carrier },
                { "system is closed",      "a system is itself a transformation.",    &tests_template_system_is_itself_a_transformation },
                { "make_template_system",  "tau explicit, sigma deduced.",            &tests_make_template_system                  },
                { "constexpr",             "the whole algebra folds at compile time.", &tests_algebra_is_constexpr                 }
            }
        },
        dt::block_spec
        {
            "III. stage pipelines",
            "stages (template consumed at stage 1) and reader_stages (template ambient).",
            std::vector<dt::test_spec>
            {
                { "stages (binary only)",   "an empty unary tail is the identity.",   &tests_stages_binary_stage_only        },
                { "stages (one unary)",     "f_2 . f_1.",                             &tests_stages_one_unary_stage          },
                { "stages (many)",          "the sink type may change per stage.",    &tests_stages_many_unary_stages        },
                { "stages order",           "f_1 runs first, f_n last.",              &tests_stages_composition_order        },
                { "stages is an F",         "the chain is a transformation.",         &tests_stages_is_a_transformation      },
                { "stages plugs in",        "usable as the F of a system.",           &tests_stages_plugs_into_template_system },
                { "stages (decay)",         "stages are stored decayed.",             &tests_stages_decays_its_arguments     },
                { "reader (one stage)",     "f(t, alpha).",                           &tests_reader_stages_single_stage      },
                { "reader (two stages)",    "f_2(t, f_1(t, alpha)).",                 &tests_reader_stages_two_stages        },
                { "reader THREADS t",       "every stage sees the template.",         &tests_reader_stages_threads_the_template },
                { "reader is an F",         "the chain is a transformation.",         &tests_reader_stages_is_a_transformation },
                { "constexpr",              "both chains fold at compile time.",      &tests_pipelines_are_constexpr         }
            }
        },
        dt::block_spec
        {
            "IV. the parser sink",
            "parse_outcome = (rho x tau) + E, its factories, its trait, and parser_system.",
            std::vector<dt::test_spec>
            {
                { "parse_success",          "a result paired with the leftover.",     &tests_parse_success_construction      },
                { "parse_failure",          "an error, no result.",                   &tests_parse_failure_construction      },
                { "is_ok / is_err",         "the two arms are exclusive.",            &tests_parse_outcome_is_ok_and_is_err  },
                { "operator bool",          "truthiness follows the ok arm.",         &tests_parse_outcome_operator_bool     },
                { "operator bool explicit", "an outcome is not a bool.",              &tests_parse_outcome_operator_bool_is_explicit },
                { "value / remaining",      "the success payload.",                   &tests_parse_outcome_value_and_remaining },
                { "error",                  "the failure payload.",                   &tests_parse_outcome_error             },
                { "inactive value-init",    "the unused arm is value-initialized.",   &tests_parse_outcome_failure_value_initializes_inactive },
                { "typedefs",               "result_type, source_type, error_type.",  &tests_parse_outcome_typedefs          },
                { "success E explicit",     "no E is present to deduce from.",        &tests_parse_success_error_type_is_explicit },
                { "failure rho/tau explicit","no rho or tau is present to deduce.",   &tests_parse_failure_result_and_source_are_explicit },
                { "is_parse_outcome",       "the sink is recognized.",                &tests_is_parse_outcome                },
                { "is_parse_outcome_v",     "the variable template tracks it.",       &tests_is_parse_outcome_v_agrees_with_trait },
                { "parser_system pins",     "sigma is pinned to the parser shape.",   &tests_parser_system_pins_the_sink     },
                { "parser_system works",    "apply and instantiate, as a system.",    &tests_parser_system_apply_and_instantiate },
                { "constexpr",              "the sink folds at compile time.",        &tests_parse_outcome_is_constexpr      }
            }
        },
        dt::block_spec
        {
            "IV. kleisli sequencing",
            "Q <> P (kleisli_then) and P >>= k (kleisli_bind).",
            std::vector<dt::test_spec>
            {
                { "then threads leftover",  "P's remaining source feeds Q.",          &tests_kleisli_then_threads_the_remaining_source },
                { "then keeps Q's result",  "the first result is discarded.",         &tests_kleisli_then_keeps_the_second_result },
                { "then result type",       "the outcome is Q's.",                    &tests_kleisli_then_result_type_is_the_second },
                { "then short-circuits",    "P failing skips Q.",                     &tests_kleisli_then_short_circuits_on_first_failure },
                { "then propagates Q fail", "Q failing surfaces.",                    &tests_kleisli_then_propagates_second_failure },
                { "then nests",             "a chain is itself a parser.",            &tests_kleisli_then_nests              },
                { "bind sees the result",   "k is applied to P's result.",            &tests_kleisli_bind_passes_the_result_to_the_continuation },
                { "bind threads leftover",  "k's parser runs on P's remaining.",      &tests_kleisli_bind_threads_the_remaining_source },
                { "bind short-circuits",    "P failing means k never runs.",          &tests_kleisli_bind_short_circuits_on_first_failure },
                { "bind propagates k fail", "the continuation's failure surfaces.",   &tests_kleisli_bind_propagates_continuation_failure },
                { "helpers are F_t",        "a sequenced parser is a source-transformer.", &tests_kleisli_helpers_are_source_transformers },
                { "decay",                  "P, Q and k are stored decayed.",         &tests_kleisli_decays_its_arguments    }
            }
        },
        dt::block_spec
        {
            "V. concepts (C++20)",
            "the concept parallels of the section-I and section-IV traits.",
            std::vector<dt::test_spec>
            {
                { "transformation_for",        "the concept admits a transformation.",   &tests_concept_transformation_for      },
                { "transformation_for is sound","and rejects a non-const-callable F.",   &tests_concept_transformation_for_rejects_non_const_callable },
                { "source_transformer_for",    "the concept admits an F_t.",             &tests_concept_source_transformer_for  },
                { "source_transformer sound",  "and rejects a non-const-callable F_t.",  &tests_concept_source_transformer_for_rejects_non_const_callable },
                { "parse_outcome_c",           "the concept admits the parser sink.",    &tests_concept_parse_outcome_c         },
                { "concepts == traits",        "each concept mirrors its trait.",        &tests_concepts_agree_with_their_traits },
                { "constrains a template",     "a constrained call is gated correctly.", &tests_concept_constrains_a_template    }
            }
        }
    };

    return m;
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_TEMPLATE_TESTS_
