// djinterp [test]  structural_traits_tests_members.cpp
//   Section I -- has_match_result_type and has_find_method: the pattern
//   protocol's structural shapes, made machine-checkable.

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "structural_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_has_match_result_type_positive
  A type exposing the pattern protocol's extraction-face result is detected.
  Tests the following:
  - a nested match_result_type is found, whatever it names
  - a full pattern (find + the nested type) carries both faces
*/
bool
tests_has_match_result_type_positive()
{
    bool ok = true;

    ok = ok && (has_match_result_type<mrt_only>::value);
    ok = ok && (has_match_result_type<pat_scan>::value);

    // the full pattern protocol: the scan face AND the extraction face.
    ok = ok && (has_find_method<pat_scan, std::string, int>::value);
    ok = ok && (has_match_result_type<pat_scan>::value);

    return ok;
}


/*
tests_has_match_result_type_negative
  The detector requires a nested TYPE -- a data member of the same name is not
  one.
  Tests the following:
  - a type without the member is rejected
  - a DATA member named match_result_type does not satisfy it
  - a scalar is rejected (and does not hard-error)
*/
bool
tests_has_match_result_type_negative()
{
    bool ok = true;

    ok = ok && (!has_match_result_type<pat_none>::value);
    ok = ok && (!has_match_result_type<mrt_data>::value);
    ok = ok && (!has_match_result_type<int>::value);
    ok = ok && (!has_match_result_type<std::string>::value);

    return ok;
}


/*
tests_has_match_result_type_cvref
  The generated detector strips cv-qualifiers and references before looking.
  Tests the following:
  - const / reference / rvalue forms all resolve to the underlying type
*/
bool
tests_has_match_result_type_cvref()
{
    bool ok = true;

    ok = ok && (has_match_result_type<const mrt_only>::value);
    ok = ok && (has_match_result_type<mrt_only&>::value);
    ok = ok && (has_match_result_type<const mrt_only&>::value);
    ok = ok && (has_match_result_type<mrt_only&&>::value);

    ok = ok && (!has_match_result_type<const pat_none&>::value);

    return ok;
}


/*
tests_has_find_method_positive
  The searchable-pattern shape -- find(const In&, size_t&, Result&) -> bool -- is
  detected, so pattern_scanner can static_assert the requirement instead of
  failing deep inside an instantiation.
  Tests the following:
  - the canonical pattern is detected
  - detection is per (Input, Result) pair, not a property of the type alone
*/
bool
tests_has_find_method_positive()
{
    bool ok = true;

    ok = ok && (has_find_method<pat_scan, std::string, int>::value);

    // the trait is indexed by the input and result types it is asked about.
    ok = ok && (!has_find_method<pat_scan, int, int>::value);
    ok = ok && (!has_find_method<pat_scan, std::string, std::string>::value);

    return ok;
}


/*
tests_has_find_method_scan_protocol
  The detected shape really is the "scan for the next occurrence" protocol the
  pattern scanner pulls on repeatedly: find writes the next position and the
  captures, and returns false once no further occurrence exists.
  Tests the following:
  - repeated calls walk every match in order
  - the terminating call returns false and the loop ends
*/
bool
tests_has_find_method_scan_protocol()
{
    bool ok = true;

    // the shape the scanner relies on, driven exactly as the scanner would.
    const pat_scan    pattern;
    const std::string input("axbxxc");     // 'x' at 1, 3, 4

    std::size_t pos   = 0;
    int         match = -1;
    int         found[8];
    int         n     = 0;

    while (n < 8 && pattern.find(input, pos, match))
    {
        found[n] = match;
        ++n;
    }

    ok = ok && (n == 3);
    ok = ok && (found[0] == 1);
    ok = ok && (found[1] == 3);
    ok = ok && (found[2] == 4);

    // and it is exhausted: another call yields nothing.
    ok = ok && (!pattern.find(input, pos, match));

    return ok;
}


/*
tests_has_find_method_negative
  Types that do not carry the shape are rejected, cleanly.
  Tests the following:
  - no find at all; a find of the wrong arity; a find taking the wrong input
  - a `find` that is a DATA member rather than a member function
  - a scalar
*/
bool
tests_has_find_method_negative()
{
    bool ok = true;

    ok = ok && (!has_find_method<pat_none, std::string, int>::value);
    ok = ok && (!has_find_method<pat_arity, std::string, int>::value);
    ok = ok && (!has_find_method<pat_wrong_in, std::string, int>::value);
    ok = ok && (!has_find_method<pat_data, std::string, int>::value);
    ok = ok && (!has_find_method<int, std::string, int>::value);

    return ok;
}


/*
tests_has_find_method_return_conversion
  The result is taken through static_cast<bool> -- an EXPLICIT conversion. So the
  bar is explicit convertibility, not implicit: a find returning a type with an
  explicit operator bool passes, where an implicit-convertibility check (as
  is_predicate uses) would refuse it.
  Tests the following:
  - bool and int results are accepted
  - an EXPLICITLY bool-convertible result is accepted
  - void and a non-convertible class are rejected
*/
bool
tests_has_find_method_return_conversion()
{
    bool ok = true;

    ok = ok && (has_find_method<pat_scan, std::string, int>::value);   // bool
    ok = ok && (has_find_method<pat_int, std::string, int>::value);    // int

    // static_cast reaches an explicit operator bool.
    ok = ok && (has_find_method<pat_explicit, std::string, int>::value);
    static_assert(!std::is_convertible<explicit_bool, bool>::value,
                  "explicit_bool is not IMPLICITLY convertible");

    ok = ok && (!has_find_method<pat_void, std::string, int>::value);
    ok = ok && (!has_find_method<pat_string, std::string, int>::value);

    return ok;
}


/*
tests_has_find_method_const_lvalue
  has_find_method probes `declval<const _Type&>().find(...)` -- a CONST lvalue --
  so a non-const find is not detected. Worth pinning: it is the opposite of the
  arity trilogy in this same header, which probes a mutable lvalue.
  Tests the following:
  - a non-const find is rejected
  - the const counterpart is accepted (the contrast)
  - the rejected pattern really does have a working find, just a non-const one
*/
bool
tests_has_find_method_const_lvalue()
{
    bool ok = true;

    ok = ok && (!has_find_method<pat_nonconst, std::string, int>::value);
    ok = ok && (has_find_method<pat_scan, std::string, int>::value);

    // it is callable -- only not on a const lvalue.
    pat_nonconst p;
    std::size_t  pos = 0;
    int          res = 0;
    ok = ok && (p.find(std::string("x"), pos, res));

    // the same header's arity traits take the OPPOSITE view of constness: a
    // non-const operator() is accepted there.
    ok = ok && (is_unary_callable<mut_step, int>::value);

    return ok;
}


/*
tests_has_find_method_parameter_binding
  The probe passes lvalues -- a const Input&, a size_t&, and a Result& -- so any
  find whose parameters bind to those lvalues qualifies.
  Tests the following:
  - a find taking size_t by value and the result by const ref still binds
  - the canonical by-reference shape binds
*/
bool
tests_has_find_method_parameter_binding()
{
    bool ok = true;

    ok = ok && (has_find_method<pat_byvalue, std::string, int>::value);
    ok = ok && (has_find_method<pat_scan, std::string, int>::value);

    return ok;
}


/*
tests_has_find_method_cvref
  clean_t is applied to all three parameters, so cv-ref spellings of the pattern,
  the input, and the result all resolve to the same query.
  Tests the following:
  - reference and const forms of each of the three agree with the plain form
*/
bool
tests_has_find_method_cvref()
{
    bool ok = true;

    ok = ok && (has_find_method<pat_scan&, std::string, int>::value);
    ok = ok && (has_find_method<const pat_scan&, std::string, int>::value);
    ok = ok && (has_find_method<pat_scan, const std::string&, int>::value);
    ok = ok && (has_find_method<pat_scan, std::string, int&>::value);
    ok = ok && (has_find_method<const pat_scan&, const std::string&,
                                int&>::value);

    // a negative stays negative through the same decay.
    ok = ok && (!has_find_method<const pat_none&, const std::string&,
                                 int&>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
