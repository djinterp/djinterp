/*******************************************************************************
* djinterp [test]                                    test_report_tests_check.cpp
*
*   Section II of the test_report suite: THE CHECK.
*
*   report_check is the leaf of the model - one assertion line - and the smallest
* thing in the header.  It has exactly one behaviour worth pinning beyond its
* fields: a DEFAULT-CONSTRUCTED check is `pending`, not `passed`.  That is the
* safe default (an unfilled check does not silently count as a success) and it is
* the one an aggregate-initialising caller is most likely to get wrong.
*
*
* path:      /tests/djinterp/test/output/test_report_tests_check.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "test_report_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_report_check_defaults_to_pending
//   the safe default.  A check nobody filled in is UNFINISHED, not passing - so a
// half-built report reads as pending rather than green.
bool
tests_report_check_defaults_to_pending()
{
    const dt::report_check _c;

    D_RT_CHECK(_c.description.empty());
    D_RT_CHECK(_c.status == dt::test_status::pending);

    // ...and therefore does not read as a pass
    D_RT_CHECK(!_c.passed());

    return true;
}


// tests_report_check_carries_its_description_and_status
bool
tests_report_check_carries_its_description_and_status()
{
    const dt::report_check _c("the sky is blue", dt::test_status::passed);

    D_RT_CHECK(_c.description == "the sky is blue");
    D_RT_CHECK(_c.status == dt::test_status::passed);
    D_RT_CHECK(_c.passed());

    // an empty description is legal - the model does not police content
    const dt::report_check _e("", dt::test_status::failed);

    D_RT_CHECK(_e.description.empty());
    D_RT_CHECK(_e.status == dt::test_status::failed);

    return true;
}


// tests_report_check_passed_is_true_only_for_passed
//   `passed()` means PASSED - not "did not fail".  A skipped check is not a pass,
// and neither is a pending one.  Swept over every status so no member is missed.
bool
tests_report_check_passed_is_true_only_for_passed()
{
    std::size_t _passes = 0;
    std::size_t _i      = 0;

    for (_i = 0; _i < k_all_statuses().size(); ++_i)
    {
        const dt::report_check _c("d", k_all_statuses()[_i]);

        if (_c.passed())
        {
            ++_passes;

            D_RT_CHECK(k_all_statuses()[_i] == dt::test_status::passed);
        }
    }

    // exactly one of the five statuses reads as a pass
    D_RT_CHECK(_passes == 1u);

    // spelled out, for the record
    D_RT_CHECK(dt::report_check("d", dt::test_status::passed).passed());
    D_RT_CHECK(!dt::report_check("d", dt::test_status::failed).passed());
    D_RT_CHECK(!dt::report_check("d", dt::test_status::skipped).passed());
    D_RT_CHECK(!dt::report_check("d", dt::test_status::pending).passed());
    D_RT_CHECK(!dt::report_check("d", dt::test_status::error).passed());

    // an out-of-range status is not a pass either
    D_RT_CHECK(!dt::report_check("d", static_cast<dt::test_status>(99)).passed());

    return true;
}


// tests_report_check_is_a_copyable_value
//   checks are stored BY VALUE in a vector and copied freely by the fixtures and
// the renderers; copying one must not alias its description.
bool
tests_report_check_is_a_copyable_value()
{
    dt::report_check _a("original", dt::test_status::passed);
    dt::report_check _b = _a;

    _b.description = "changed";
    _b.status      = dt::test_status::failed;

    D_RT_CHECK(_a.description == "original");
    D_RT_CHECK(_a.status == dt::test_status::passed);
    D_RT_CHECK(_b.description == "changed");
    D_RT_CHECK(_b.status == dt::test_status::failed);

    // and survives a round trip through a vector
    std::vector<dt::report_check> _v;

    _v.push_back(_a);
    _v.push_back(_b);

    D_RT_CHECK(_v.size() == 2u);
    D_RT_CHECK(_v[0].description == "original");
    D_RT_CHECK(_v[1].description == "changed");

    return true;
}


NS_END   // testing
NS_END   // djinterp
