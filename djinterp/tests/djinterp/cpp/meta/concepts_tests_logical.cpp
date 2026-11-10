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
* path:      /tests/djinterp/cpp/meta/concepts_tests_logical.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "concepts_tests.hpp"


namespace {

    using DTT = std::true_type;
    using DTF = std::false_type;

}  // unnamed namespace


NS_DJINTERP
NS_TESTING
bool concepts_tests_logical()
{
    bool verdict = true;

    // -------------------------------------------------------------------
    // all_of -- conjunction
    // -------------------------------------------------------------------
    //   Empty pack folds to true (the identity element of &&).

    DT_CONCEPT    ((AllOf<>));                              // empty -> true
    DT_CONCEPT    ((AllOf<DTT>));
    DT_CONCEPT    ((AllOf<DTT, DTT>));
    DT_CONCEPT    ((AllOf<DTT, DTT, DTT>));
    DT_NOT_CONCEPT((AllOf<DTF>));
    DT_NOT_CONCEPT((AllOf<DTT, DTF>));
    DT_NOT_CONCEPT((AllOf<DTF, DTT>));
    DT_NOT_CONCEPT((AllOf<DTF, DTF>));
    DT_NOT_CONCEPT((AllOf<DTT, DTT, DTF>));

    // -------------------------------------------------------------------
    // any_of -- disjunction
    // -------------------------------------------------------------------
    //   Empty pack folds to false (the identity element of ||).

    DT_NOT_CONCEPT((AnyOf<>));                              // empty -> false
    DT_CONCEPT    ((AnyOf<DTT>));
    DT_CONCEPT    ((AnyOf<DTT, DTF>));
    DT_CONCEPT    ((AnyOf<DTF, DTT>));
    DT_CONCEPT    ((AnyOf<DTF, DTF, DTT>));
    DT_NOT_CONCEPT((AnyOf<DTF>));
    DT_NOT_CONCEPT((AnyOf<DTF, DTF>));
    DT_NOT_CONCEPT((AnyOf<DTF, DTF, DTF>));

    // -------------------------------------------------------------------
    // none_of -- negation of any_of
    // -------------------------------------------------------------------

    DT_CONCEPT    ((NoneOf<>));                             // empty -> true
    DT_CONCEPT    ((NoneOf<DTF>));
    DT_CONCEPT    ((NoneOf<DTF, DTF>));
    DT_CONCEPT    ((NoneOf<DTF, DTF, DTF>));
    DT_NOT_CONCEPT((NoneOf<DTT>));
    DT_NOT_CONCEPT((NoneOf<DTT, DTF>));
    DT_NOT_CONCEPT((NoneOf<DTF, DTT>));

    //   structural invariant: any_of and none_of are negations.
    // The outer parens shield the inner template-arg commas from the
    // preprocessor (same pattern as the multi-arg DT_CONCEPT calls
    // above).
    DT_CONCEPT    ((AnyOf<DTT, DTF>  != NoneOf<DTT, DTF>));
    DT_CONCEPT    ((AnyOf<DTF, DTF>  != NoneOf<DTF, DTF>));

    // -------------------------------------------------------------------
    // exactly_one_of -- "one-hot": exactly one predicate true
    // -------------------------------------------------------------------

    DT_NOT_CONCEPT((exactly_one_of<>));                      // zero true ones
    DT_CONCEPT    ((exactly_one_of<DTT>));
    DT_NOT_CONCEPT((exactly_one_of<DTF>));
    DT_CONCEPT    ((exactly_one_of<DTT, DTF>));
    DT_CONCEPT    ((exactly_one_of<DTF, DTT>));
    DT_NOT_CONCEPT((exactly_one_of<DTT, DTT>));                // two true ones
    DT_NOT_CONCEPT((exactly_one_of<DTF, DTF>));
    DT_CONCEPT    ((exactly_one_of<DTT, DTF, DTF>));
    DT_CONCEPT    ((exactly_one_of<DTF, DTT, DTF>));
    DT_CONCEPT    ((exactly_one_of<DTF, DTF, DTT>));
    DT_NOT_CONCEPT((exactly_one_of<DTT, DTT, DTF>));
    DT_NOT_CONCEPT((exactly_one_of<DTT, DTT, DTT>));

    //   the critical divergence vs xor_of
    DT_CONCEPT    ((exactly_one_of<DTT, DTF, DTF, DTF>));        // one-hot TRUE
    DT_NOT_CONCEPT((exactly_one_of<DTT, DTF, DTT, DTF>));        // one-hot FALSE

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
    DT_CONCEPT    ((xor_of<DTT>));
    DT_NOT_CONCEPT((xor_of<DTF>));

    //   2 args -- pure XOR
    DT_CONCEPT    ((xor_of<DTT, DTF>));                        //  T XOR F  = T
    DT_CONCEPT    ((xor_of<DTF, DTT>));                        //  F XOR T  = T
    DT_NOT_CONCEPT((xor_of<DTT, DTT>));                        //  T XOR T  = F
    DT_NOT_CONCEPT((xor_of<DTF, DTF>));                        //  F XOR F  = F

    //   3 args -- (B1 XOR B2) AND xor_of<[B3]> = (B1 XOR B2) AND B3
    DT_CONCEPT    ((xor_of<DTT, DTF, DTT>));                    // (T!=F) && T  = T
    DT_CONCEPT    ((xor_of<DTF, DTT, DTT>));                    // (F!=T) && T  = T
    DT_NOT_CONCEPT((xor_of<DTT, DTF, DTF>));                    // (T!=F) && F  = F
    DT_NOT_CONCEPT((xor_of<DTF, DTT, DTF>));                    // (F!=T) && F  = F
    DT_NOT_CONCEPT((xor_of<DTT, DTT, DTT>));                    // (T!=T) && T  = F
    DT_NOT_CONCEPT((xor_of<DTF, DTF, DTT>));                    // (F!=F) && T  = F
    DT_NOT_CONCEPT((xor_of<DTT, DTT, DTF>));                    // (T!=T) && F  = F
    DT_NOT_CONCEPT((xor_of<DTF, DTF, DTF>));                    // (F!=F) && F  = F

    //   4 args -- (B1 XOR B2) AND xor_of<[B3, B4]> = (B1!=B2) && (B3!=B4)
    DT_CONCEPT    ((xor_of<DTT, DTF, DTT, DTF>));                // T && T  = T
    DT_CONCEPT    ((xor_of<DTF, DTT, DTF, DTT>));                // T && T  = T
    DT_CONCEPT    ((xor_of<DTT, DTF, DTF, DTT>));                // T && T  = T
    DT_NOT_CONCEPT((xor_of<DTT, DTT, DTT, DTF>));                // F && T  = F
    DT_NOT_CONCEPT((xor_of<DTT, DTF, DTT, DTT>));                // T && F  = F
    DT_NOT_CONCEPT((xor_of<DTT, DTT, DTF, DTF>));                // F && F  = F
    DT_NOT_CONCEPT((xor_of<DTF, DTF, DTF, DTF>));                // F && F  = F

    //   the critical divergence vs exactly_one_of, completing the
    // commentary at the top of the file
    DT_NOT_CONCEPT((xor_of<DTT, DTF, DTF, DTF>));                // exactly_one_of: TRUE; xor_of: FALSE
    DT_CONCEPT    ((xor_of<DTT, DTF, DTT, DTF>));                // exactly_one_of: FALSE; xor_of: TRUE

    //   shared cases where both agree.  Multi-arg template
    // instantiations need the outer parens; single-arg ones don't.
    DT_CONCEPT    ((exactly_one_of<DTT, DTF>      == xor_of<DTT, DTF>));
    DT_CONCEPT    (exactly_one_of<DTT>          == xor_of<DTT>);
    DT_CONCEPT    (exactly_one_of<DTF>          == xor_of<DTF>);
    DT_CONCEPT    ((exactly_one_of<DTT, DTT>      == xor_of<DTT, DTT>));
    DT_CONCEPT    ((exactly_one_of<DTF, DTF>      == xor_of<DTF, DTF>));

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
