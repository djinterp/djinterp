/*******************************************************************************
* djinterp [test]                                     concepts_tests_logical.cpp
*
*   Tests for concepts.hpp section II.8: logical concepts.
* Covers: all_of, any_of, none_of, exactly_one_of, xor_of.
*
*   These concepts take type predicates - types that expose a `::value`
* member - and fold them.  The fixture aliases `tT` and `tF`
* (true_type / false_type) keep the call sites compact while still
* exercising the full truth table for each fold.
*
*   exactly_one_of vs xor_of:
*     The two concepts disagree on inputs whose Boolean structure
*   diverges between one-hot and cumulative pairwise XOR.  This file
*   exercises both the cases where they agree and the cases where they
*   diverge.  Example divergences (T = true_type, F = false_type):
*       <T, F, F, F>        exactly_one_of: TRUE   xor_of: FALSE
*       <T, F, T, F>        exactly_one_of: FALSE  xor_of: TRUE
*     The shared cases:
*       <T>                 both TRUE
*       <F>                 both FALSE
*       <T, F>              both TRUE  (one true, one false)
*       <T, T>              both FALSE
*       <F, F>              both FALSE
*       <T, F, F>           both TRUE
*       <T, T, T>           both FALSE
*
*
* path:      /tests/djinterp/core/meta/concepts_tests_logical.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "concepts_tests.hpp"


namespace {

    using tT = std::true_type;
    using tF = std::false_type;

}  // unnamed namespace


NS_DJINTERP_TEST


void
concepts_tests_logical(
    test_handler& _test_handler,
    test_option   _options...)
{
    // -------------------------------------------------------------------
    // all_of -- conjunction
    // -------------------------------------------------------------------
    //   Empty pack folds to true (the identity element of &&).

    DT_CONCEPT    ((all_of<>));                              // empty -> true
    DT_CONCEPT    ((all_of<tT>));
    DT_CONCEPT    ((all_of<tT, tT>));
    DT_CONCEPT    ((all_of<tT, tT, tT>));
    DT_NOT_CONCEPT((all_of<tF>));
    DT_NOT_CONCEPT((all_of<tT, tF>));
    DT_NOT_CONCEPT((all_of<tF, tT>));
    DT_NOT_CONCEPT((all_of<tF, tF>));
    DT_NOT_CONCEPT((all_of<tT, tT, tF>));

    // -------------------------------------------------------------------
    // any_of -- disjunction
    // -------------------------------------------------------------------
    //   Empty pack folds to false (the identity element of ||).

    DT_NOT_CONCEPT((any_of<>));                              // empty -> false
    DT_CONCEPT    ((any_of<tT>));
    DT_CONCEPT    ((any_of<tT, tF>));
    DT_CONCEPT    ((any_of<tF, tT>));
    DT_CONCEPT    ((any_of<tF, tF, tT>));
    DT_NOT_CONCEPT((any_of<tF>));
    DT_NOT_CONCEPT((any_of<tF, tF>));
    DT_NOT_CONCEPT((any_of<tF, tF, tF>));

    // -------------------------------------------------------------------
    // none_of -- negation of any_of
    // -------------------------------------------------------------------

    DT_CONCEPT    ((none_of<>));                             // empty -> true
    DT_CONCEPT    ((none_of<tF>));
    DT_CONCEPT    ((none_of<tF, tF>));
    DT_CONCEPT    ((none_of<tF, tF, tF>));
    DT_NOT_CONCEPT((none_of<tT>));
    DT_NOT_CONCEPT((none_of<tT, tF>));
    DT_NOT_CONCEPT((none_of<tF, tT>));

    //   structural invariant: any_of and none_of are negations.
    // The outer parens shield the inner template-arg commas from the
    // preprocessor (same pattern as the multi-arg DT_CONCEPT calls
    // above).
    DT_CONCEPT    ((any_of<tT, tF>  != none_of<tT, tF>));
    DT_CONCEPT    ((any_of<tF, tF>  != none_of<tF, tF>));

    // -------------------------------------------------------------------
    // exactly_one_of -- "one-hot": exactly one predicate true
    // -------------------------------------------------------------------

    DT_NOT_CONCEPT((exactly_one_of<>));                      // zero true ones
    DT_CONCEPT    ((exactly_one_of<tT>));
    DT_NOT_CONCEPT((exactly_one_of<tF>));
    DT_CONCEPT    ((exactly_one_of<tT, tF>));
    DT_CONCEPT    ((exactly_one_of<tF, tT>));
    DT_NOT_CONCEPT((exactly_one_of<tT, tT>));                // two true ones
    DT_NOT_CONCEPT((exactly_one_of<tF, tF>));
    DT_CONCEPT    ((exactly_one_of<tT, tF, tF>));
    DT_CONCEPT    ((exactly_one_of<tF, tT, tF>));
    DT_CONCEPT    ((exactly_one_of<tF, tF, tT>));
    DT_NOT_CONCEPT((exactly_one_of<tT, tT, tF>));
    DT_NOT_CONCEPT((exactly_one_of<tT, tT, tT>));

    //   the critical divergence vs xor_of
    DT_CONCEPT    ((exactly_one_of<tT, tF, tF, tF>));        // one-hot TRUE
    DT_NOT_CONCEPT((exactly_one_of<tT, tF, tT, tF>));        // one-hot FALSE

    // -------------------------------------------------------------------
    // xor_of -- cumulative pairwise XOR (new concept; parallels
    //           djinterp::exclusive_disjunction)
    // -------------------------------------------------------------------
    //   Recurrence in concepts.hpp internal::xor_of_impl:
    //     0 args         -> false_type
    //     1 arg          -> the predicate itself
    //     2 args         -> _B1::value != _B2::value
    //     3+ args        -> (_B1 != _B2) AND xor_of<_Bs...>
    //   Walk through every shape up to four args.

    //   0 args -- documented false
    DT_NOT_CONCEPT((xor_of<>));

    //   1 arg -- takes the predicate's own value
    DT_CONCEPT    ((xor_of<tT>));
    DT_NOT_CONCEPT((xor_of<tF>));

    //   2 args -- pure XOR
    DT_CONCEPT    ((xor_of<tT, tF>));                        //  T XOR F  = T
    DT_CONCEPT    ((xor_of<tF, tT>));                        //  F XOR T  = T
    DT_NOT_CONCEPT((xor_of<tT, tT>));                        //  T XOR T  = F
    DT_NOT_CONCEPT((xor_of<tF, tF>));                        //  F XOR F  = F

    //   3 args -- (B1 XOR B2) AND xor_of<[B3]> = (B1 XOR B2) AND B3
    DT_CONCEPT    ((xor_of<tT, tF, tT>));                    // (T!=F) && T  = T
    DT_CONCEPT    ((xor_of<tF, tT, tT>));                    // (F!=T) && T  = T
    DT_NOT_CONCEPT((xor_of<tT, tF, tF>));                    // (T!=F) && F  = F
    DT_NOT_CONCEPT((xor_of<tF, tT, tF>));                    // (F!=T) && F  = F
    DT_NOT_CONCEPT((xor_of<tT, tT, tT>));                    // (T!=T) && T  = F
    DT_NOT_CONCEPT((xor_of<tF, tF, tT>));                    // (F!=F) && T  = F
    DT_NOT_CONCEPT((xor_of<tT, tT, tF>));                    // (T!=T) && F  = F
    DT_NOT_CONCEPT((xor_of<tF, tF, tF>));                    // (F!=F) && F  = F

    //   4 args -- (B1 XOR B2) AND xor_of<[B3, B4]> = (B1!=B2) && (B3!=B4)
    DT_CONCEPT    ((xor_of<tT, tF, tT, tF>));                // T && T  = T
    DT_CONCEPT    ((xor_of<tF, tT, tF, tT>));                // T && T  = T
    DT_CONCEPT    ((xor_of<tT, tF, tF, tT>));                // T && T  = T
    DT_NOT_CONCEPT((xor_of<tT, tT, tT, tF>));                // F && T  = F
    DT_NOT_CONCEPT((xor_of<tT, tF, tT, tT>));                // T && F  = F
    DT_NOT_CONCEPT((xor_of<tT, tT, tF, tF>));                // F && F  = F
    DT_NOT_CONCEPT((xor_of<tF, tF, tF, tF>));                // F && F  = F

    //   the critical divergence vs exactly_one_of, completing the
    // commentary at the top of the file
    DT_NOT_CONCEPT((xor_of<tT, tF, tF, tF>));                // exactly_one_of: TRUE; xor_of: FALSE
    DT_CONCEPT    ((xor_of<tT, tF, tT, tF>));                // exactly_one_of: FALSE; xor_of: TRUE

    //   shared cases where both agree.  Multi-arg template
    // instantiations need the outer parens; single-arg ones don't.
    DT_CONCEPT    ((exactly_one_of<tT, tF>      == xor_of<tT, tF>));
    DT_CONCEPT    (exactly_one_of<tT>          == xor_of<tT>);
    DT_CONCEPT    (exactly_one_of<tF>          == xor_of<tF>);
    DT_CONCEPT    ((exactly_one_of<tT, tT>      == xor_of<tT, tT>));
    DT_CONCEPT    ((exactly_one_of<tF, tF>      == xor_of<tF, tF>));

    return;
}


NS_END  // test
NS_END  // djinterp
