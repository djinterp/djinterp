/*******************************************************************************
* djinterp [test]                                  test_report_tests_verdict.cpp
*
*   Section I of the test_report suite: THE VERDICT.
*
*   verdict_from_counts_helper is the FOLD THE WHOLE MODEL RESTS ON.  It is called
* to derive a unit's verdict from its checks, a module's from its units, and a
* run's from its modules - three levels, one function.  Its precedence order
* (empty -> failed -> pending -> passed) is therefore the single decision that
* determines whether any given run is green, and every test below is really asking
* the same question: under exactly which counts does this thing say PASS?
*
*   The answer has one surprise in it, and it is the last test in this file.
*
*
* path:      /tests/djinterp/test/output/test_report_tests_verdict.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstddef>
#include <string>
// djinterp
#include "test_report_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_report_verdict_has_four_members
//   the ordinals are pinned because they are the enum's wire form: a member
// inserted in the middle silently re-points every persisted or transmitted value.
bool
tests_report_verdict_has_four_members()
{
    D_RT_CHECK(static_cast<int>(dt::report_verdict::empty)   == 0);
    D_RT_CHECK(static_cast<int>(dt::report_verdict::passed)  == 1);
    D_RT_CHECK(static_cast<int>(dt::report_verdict::failed)  == 2);
    D_RT_CHECK(static_cast<int>(dt::report_verdict::pending) == 3);

    D_RT_CHECK(k_all_verdicts().size() == 4u);

    return true;
}


// tests_report_verdict_word_names_every_member
//   the banner word for each verdict.  Note that PENDING and EMPTY are both
// non-pass words, which is the header's stated intent: "a banner never claims
// success on an unfinished run".
bool
tests_report_verdict_word_names_every_member()
{
    D_RT_CHECK(std::string(dt::report_verdict_word(dt::report_verdict::passed))  == "PASS");
    D_RT_CHECK(std::string(dt::report_verdict_word(dt::report_verdict::failed))  == "FAIL");
    D_RT_CHECK(std::string(dt::report_verdict_word(dt::report_verdict::pending)) == "PENDING");
    D_RT_CHECK(std::string(dt::report_verdict_word(dt::report_verdict::empty))   == "EMPTY");

    // the four words are pairwise distinct - a reader can always tell them apart
    std::size_t _i = 0;
    std::size_t _j = 0;

    for (_i = 0; _i < k_all_verdicts().size(); ++_i)
    {
        for (_j = _i + 1; _j < k_all_verdicts().size(); ++_j)
        {
            D_RT_CHECK(std::string(dt::report_verdict_word(k_all_verdicts()[_i])) !=
                       std::string(dt::report_verdict_word(k_all_verdicts()[_j])));
        }
    }

    return true;
}


// tests_report_verdict_word_default_arm_reads_empty
//   `empty` is reached through the switch's `default:` rather than by an explicit
// case, so an OUT-OF-RANGE verdict also reads as EMPTY.  That is the safe way
// round (an unknown verdict is not reported as a pass), but it does mean a corrupt
// value is indistinguishable from a legitimately empty one.
bool
tests_report_verdict_word_default_arm_reads_empty()
{
    D_RT_CHECK(std::string(dt::report_verdict_word(static_cast<dt::report_verdict>(99))) ==
               "EMPTY");
    D_RT_CHECK(std::string(dt::report_verdict_word(static_cast<dt::report_verdict>(-1))) ==
               "EMPTY");

    // ...and it is never mistaken for a pass
    D_RT_CHECK(!dt::report_verdict_is_pass(static_cast<dt::report_verdict>(99)));

    return true;
}


// tests_report_verdict_is_pass_only_for_passed
//   the whole point of the predicate: exactly ONE of the four verdicts is a pass.
// pending is not, and - importantly - neither is empty.  A run that did nothing
// did not succeed.
bool
tests_report_verdict_is_pass_only_for_passed()
{
    D_RT_CHECK(dt::report_verdict_is_pass(dt::report_verdict::passed));

    D_RT_CHECK(!dt::report_verdict_is_pass(dt::report_verdict::empty));
    D_RT_CHECK(!dt::report_verdict_is_pass(dt::report_verdict::failed));
    D_RT_CHECK(!dt::report_verdict_is_pass(dt::report_verdict::pending));

    // exactly one of the four
    std::size_t _passes = 0;
    std::size_t _i      = 0;

    for (_i = 0; _i < k_all_verdicts().size(); ++_i)
    {
        if (dt::report_verdict_is_pass(k_all_verdicts()[_i]))
        {
            ++_passes;
        }
    }

    D_RT_CHECK(_passes == 1u);

    return true;
}


// tests_report_counts_helper_zero_total_is_empty
//   nothing observed is `empty` - and it is the FIRST thing the fold decides, so
// no other rule can pre-empt it.
bool
tests_report_counts_helper_zero_total_is_empty()
{
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(0, 0, 0, 0, 0) ==
               dt::report_verdict::empty);

    return true;
}


// tests_report_counts_helper_any_failure_is_failed
//   one failure among any number of passes is a failure.  The fold is not a
// majority vote.
bool
tests_report_counts_helper_any_failure_is_failed()
{
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(0, 1, 0, 0, 0) ==
               dt::report_verdict::failed);
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(1000, 1, 0, 0, 0) ==
               dt::report_verdict::failed);

    return true;
}


// tests_report_counts_helper_an_error_is_a_failure
//   errors and failures are the same terminal state to the fold; either alone is
// enough to sink the verdict.
bool
tests_report_counts_helper_an_error_is_a_failure()
{
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(0, 0, 0, 0, 1) ==
               dt::report_verdict::failed);
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(1000, 0, 0, 0, 1) ==
               dt::report_verdict::failed);

    return true;
}


// tests_report_counts_helper_pending_outranks_passed
//   unfinished work blocks a pass, so a run with anything still pending reads as
// pending rather than green.
bool
tests_report_counts_helper_pending_outranks_passed()
{
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(0, 0, 0, 1, 0) ==
               dt::report_verdict::pending);
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(1000, 0, 0, 1, 0) ==
               dt::report_verdict::pending);

    return true;
}


// tests_report_counts_helper_failure_outranks_pending
//   the precedence between the two non-pass verdicts: a failure is reported even
// when work is also outstanding.  A red run is red, not "not finished yet".
bool
tests_report_counts_helper_failure_outranks_pending()
{
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(0, 1, 0, 1, 0) ==
               dt::report_verdict::failed);
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(0, 0, 0, 1, 1) ==
               dt::report_verdict::failed);

    // the full precedence chain, in one mixed count
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(5, 1, 2, 3, 4) ==
               dt::report_verdict::failed);

    return true;
}


// tests_report_counts_helper_all_passed_is_passed
bool
tests_report_counts_helper_all_passed_is_passed()
{
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(1, 0, 0, 0, 0) ==
               dt::report_verdict::passed);
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(1000, 0, 0, 0, 0) ==
               dt::report_verdict::passed);

    return true;
}


// tests_report_counts_helper_skipped_alone_reads_as_passed
//   A FINDING, pinned as fact.
//
//   `skipped` is counted into the total - so it is enough to make the fold
// non-EMPTY - but it is then consulted by NO other rule.  It cannot cause a
// failure, and it cannot cause a pending.  So a unit, module, or run whose every
// observation was SKIPPED falls all the way through to the final `return passed`.
//
//   The consequence is a green verdict over work that never ran:
//
//       verdict_from_counts_helper(0, 0, 5, 0, 0)  ==  passed
//
// with zero passed checks behind it.  A suite that skips everything - a bad
// filter, a mis-set tag, an early bail - reports PASS.  Contrast the treatment of
// `pending`, which the fold explicitly refuses to let through, on the stated
// grounds that "a banner never claims success on an unfinished run": a skipped
// test is, in exactly that sense, unfinished too.
//
//   Whether skipped should read as `passed` (it did not fail), as `pending` (it
// did not run), or as its own verdict is a design call.  This test does not take a
// side - it pins the current behaviour so the call is a decision rather than an
// accident.  If it is ever reconsidered, this is the test that will say so.
bool
tests_report_counts_helper_skipped_alone_reads_as_passed()
{
    // nothing but skips -> a clean PASS
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(0, 0, 5, 0, 0) ==
               dt::report_verdict::passed);

    // ...and it really is reported as a pass, not merely as non-failed
    D_RT_CHECK(dt::report_verdict_is_pass(
                   dt::internal::verdict_from_counts_helper(0, 0, 5, 0, 0)));

    // the skip DID count towards the total, which is what lifted it out of `empty`
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(0, 0, 0, 0, 0) ==
               dt::report_verdict::empty);

    // ...yet it is powerless against every other rule
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(0, 1, 5, 0, 0) ==
               dt::report_verdict::failed);
    D_RT_CHECK(dt::internal::verdict_from_counts_helper(0, 0, 5, 1, 0) ==
               dt::report_verdict::pending);

    // for contrast: pending alone is NOT allowed to read as a pass
    D_RT_CHECK(!dt::report_verdict_is_pass(
                    dt::internal::verdict_from_counts_helper(0, 0, 0, 5, 0)));

    return true;
}


NS_END   // testing
NS_END   // djinterp
