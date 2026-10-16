/*******************************************************************************
* djinterp [test]                                    test_report_tests_rates.cpp
*
*   Section VI of the test_report suite: THE PASS-RATE HELPERS.
*
*   Two small formatters, and the only two places in the model where a number turns
* into a string.  Both are printed on every summary page in every dialect, so their
* exact shape is a contract, not a detail.
*
*   report_pass_rate's one real job is to not divide by zero, and it does that.  Its
* one real hazard is the 16-byte buffer it formats into, which is comfortable for
* any input that respects the passed <= total contract (the longest such string is
* "100.00%") and silently truncates for inputs that do not.  Pinned at the end.
*
*
* path:      /tests/djinterp/test/output/test_report_tests_rates.cpp
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


// tests_report_pass_rate_of_a_zero_total_is_zero_not_a_division
//   the guard that matters: an empty run divides 0 by 0 unless something stops it,
// and something does.  "0.00%" - not "nan%", not "-nan%", not a crash.
bool
tests_report_pass_rate_of_a_zero_total_is_zero_not_a_division()
{
    D_RT_CHECK(dt::report_pass_rate(0, 0) == "0.00%");

    //   the guard is on the TOTAL, so a nonsense passed-count with a zero total is
    // still caught (it never reaches the division)
    D_RT_CHECK(dt::report_pass_rate(5, 0) == "0.00%");

    // ...and there is no "nan" anywhere in the result
    D_RT_CHECK(!contains(dt::report_pass_rate(0, 0), "nan"));
    D_RT_CHECK(!contains(dt::report_pass_rate(0, 0), "inf"));

    return true;
}


// tests_report_pass_rate_formats_two_decimal_places
//   two decimals, always, with a trailing percent sign - the shape the summaries
// are laid out around.
bool
tests_report_pass_rate_formats_two_decimal_places()
{
    D_RT_CHECK(dt::report_pass_rate(1, 2)  == "50.00%");
    D_RT_CHECK(dt::report_pass_rate(1, 4)  == "25.00%");
    D_RT_CHECK(dt::report_pass_rate(3, 4)  == "75.00%");
    D_RT_CHECK(dt::report_pass_rate(0, 7)  == "0.00%");     // none of seven
    D_RT_CHECK(dt::report_pass_rate(1, 8)  == "12.50%");

    // the decimals are always present, even on a whole number
    D_RT_CHECK(dt::report_pass_rate(1, 1) == "100.00%");
    D_RT_CHECK(contains(dt::report_pass_rate(1, 1), ".00"));

    // ...and the sign is always there
    D_RT_CHECK(dt::report_pass_rate(1, 3).back() == '%');

    return true;
}


// tests_report_pass_rate_rounds_rather_than_truncates
//   %.2f rounds; it does not chop.  1/3 is 33.33% and 2/3 is 66.67% - the second
// would read 66.66% under truncation, which is the classic off-by-a-cent tell.
bool
tests_report_pass_rate_rounds_rather_than_truncates()
{
    D_RT_CHECK(dt::report_pass_rate(1, 3) == "33.33%");
    D_RT_CHECK(dt::report_pass_rate(2, 3) == "66.67%");     // rounded UP, not 66.66%

    D_RT_CHECK(dt::report_pass_rate(1, 6) == "16.67%");     // 16.666... -> 16.67
    D_RT_CHECK(dt::report_pass_rate(5, 6) == "83.33%");     // 83.333... -> 83.33

    // a rate that lands exactly on a half-cent still formats to two places
    D_RT_CHECK(dt::report_pass_rate(1, 16) == "6.25%");

    // the ratio the model itself produces for a realistic run
    D_RT_CHECK(dt::report_pass_rate(4, 6) == "66.67%");

    return true;
}


// tests_report_pass_rate_of_everything_is_one_hundred
//   the happy case, and the LONGEST string the contract can produce - seven
// characters, comfortably inside the buffer.
bool
tests_report_pass_rate_of_everything_is_one_hundred()
{
    D_RT_CHECK(dt::report_pass_rate(1, 1)     == "100.00%");
    D_RT_CHECK(dt::report_pass_rate(93, 93)   == "100.00%");
    D_RT_CHECK(dt::report_pass_rate(9999, 9999) == "100.00%");

    //   the longest legal result is 7 characters, so the 16-byte buffer is more
    // than sufficient for any input honouring passed <= total
    D_RT_CHECK(dt::report_pass_rate(1, 1).size() == 7u);

    return true;
}


// tests_report_pass_rate_exceeds_one_hundred_when_passed_exceeds_total
//   the function does not clamp.  Passing more than the total is a caller error,
// and the formatter simply reports it rather than hiding it - which is arguably the
// right call, since a silently clamped 100% would conceal the bug.
bool
tests_report_pass_rate_exceeds_one_hundred_when_passed_exceeds_total()
{
    D_RT_CHECK(dt::report_pass_rate(2, 1)  == "200.00%");
    D_RT_CHECK(dt::report_pass_rate(3, 2)  == "150.00%");

    // it is not clamped, and it is not an error
    D_RT_CHECK(dt::report_pass_rate(2, 1) != "100.00%");

    return true;
}


// tests_report_pass_rate_overflows_its_buffer_on_absurd_input
//   A ROBUSTNESS NOTE, pinned as fact.
//
//   report_pass_rate formats into `char buf[16]`.  That is ample for any input
// respecting passed <= total (max "100.00%", 7 chars), and it does not overflow -
// snprintf truncates rather than smashing the stack.  But it TRUNCATES SILENTLY,
// and the truncated result is not a percentage at all: it loses its decimals and
// its '%' sign, and comes back as a bare run of digits.
//
//       report_pass_rate(SIZE_MAX, 1)  ->  "184467440737095"
//
//   which is 15 characters of a number that should have read
// "1844674407370955161600.00%".  No '%', no decimals, no indication anything went
// wrong.  Only reachable with a wildly out-of-contract count, so this is a note
// rather than a bug - but a caller that hands in a garbage tally gets a garbage
// string back rather than a diagnostic, and the string still looks plausible.
bool
tests_report_pass_rate_overflows_its_buffer_on_absurd_input()
{
    const std::string _absurd =
        dt::report_pass_rate(static_cast<std::size_t>(-1), 1);

    //   it does not crash, and it does not overflow - snprintf is safe
    D_RT_CHECK(!_absurd.empty());

    //   ...but it was truncated to the buffer, losing the '%' entirely
    D_RT_CHECK(_absurd.size() == 15u);              // 16 - 1 for the NUL
    D_RT_CHECK(_absurd.back() != '%');              // <- no percent sign at all
    D_RT_CHECK(!contains(_absurd, "%"));
    D_RT_CHECK(!contains(_absurd, "."));            // ...and no decimal point

    //   what came back is a bare digit run - a plausible-looking, meaningless string
    D_RT_CHECK(is_digits(_absurd));

    //   for contrast, every in-contract input keeps its shape
    D_RT_CHECK(dt::report_pass_rate(0, 1).back()   == '%');
    D_RT_CHECK(dt::report_pass_rate(1, 1).back()   == '%');
    D_RT_CHECK(dt::report_pass_rate(500, 999).back() == '%');

    return true;
}


// tests_report_ratio_formats_the_fraction
//   the "passed/total" figure, printed beside the percentage everywhere.
bool
tests_report_ratio_formats_the_fraction()
{
    D_RT_CHECK(dt::report_ratio(80, 93) == "80/93");
    D_RT_CHECK(dt::report_ratio(0, 0)   == "0/0");      // the empty run, not an error
    D_RT_CHECK(dt::report_ratio(0, 5)   == "0/5");
    D_RT_CHECK(dt::report_ratio(5, 5)   == "5/5");

    // no clamping here either - the ratio reports what it was given
    D_RT_CHECK(dt::report_ratio(7, 3) == "7/3");

    // there is exactly one separator
    D_RT_CHECK(dt::report_ratio(1, 2).find('/') != std::string::npos);
    D_RT_CHECK(dt::report_ratio(1, 2).size() == 3u);

    return true;
}


// tests_report_ratio_survives_the_largest_counts
//   report_ratio formats into char buf[48].  Two 20-digit values plus a slash is 41
// characters - so unlike report_pass_rate, this buffer really is sized for the
// worst case its type can produce, and the largest possible ratio round-trips
// intact.
bool
tests_report_ratio_survives_the_largest_counts()
{
    const std::size_t _max = static_cast<std::size_t>(-1);

    const std::string _r = dt::report_ratio(_max, _max);

    //   not truncated: both operands are present in full, with the separator
    D_RT_CHECK(contains(_r, "/"));

    const std::size_t _slash = _r.find('/');

    D_RT_CHECK(_slash != std::string::npos);

    const std::string _lhs = _r.substr(0, _slash);
    const std::string _rhs = _r.substr(_slash + 1);

    D_RT_CHECK(is_digits(_lhs));
    D_RT_CHECK(is_digits(_rhs));
    D_RT_CHECK(_lhs == _rhs);                       // the same number, twice

    //   the whole thing fits well inside the 48-byte buffer
    D_RT_CHECK(_r.size() < 48u);
    D_RT_CHECK(_r.size() == (_lhs.size() * 2u) + 1u);

    return true;
}


NS_END   // testing
NS_END   // djinterp
