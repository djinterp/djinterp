/*******************************************************************************
* djinterp [core]                                        decode_tests_result.cpp
*
*
* path:      /tests/djinterp/core/binary/decode_tests_result.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "decode_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_decode_result_type
  decode_result is the partial outcome - an ok flag beside a value
  Tests the following:
  - it is an aggregate: brace-initialisation fills ok then value
  - a true result carries the supplied value
  - a false result may still carry a (default) value slot
  - the element type is preserved (decode_result<double> holds a double)
*/
bool
tests_decode_result_type()
{
    decode_result<int> _ok{ true, 42 };
    D_DE_CHECK(_ok.ok);
    D_DE_CHECK(_ok.value == 42);

    decode_result<int> _no{ false, 0 };
    D_DE_CHECK(!_no.ok);
    D_DE_CHECK(_no.value == 0);

    decode_result<double> _d{ true, 0.5 };
    D_DE_CHECK(_d.ok);
    D_DE_CHECK(_d.value == 0.5);

    return true;
}

/*
tests_decode_success_failure
  decode_success / decode_failure build the two outcomes
  Tests the following:
  - decode_success yields ok = true carrying the value
  - decode_success strips cv/ref from the deduced element type (an lvalue in
    yields a value-typed result)
  - decode_success moves a movable custom value into place
  - decode_failure yields ok = false with a default-constructed value
*/
bool
tests_decode_success_failure()
{
    // basic success
    decode_result<int> _s = decode_success(7);
    D_DE_CHECK(_s.ok);
    D_DE_CHECK(_s.value == 7);

    // an lvalue argument still deduces a value-typed result (clean_t strip)
    int _lv = 99;
    decode_result<int> _s2 = decode_success(_lv);
    D_DE_CHECK(_s2.ok);
    D_DE_CHECK(_s2.value == 99);

    // a movable custom value carried through
    decode_result<rt_leaf> _sc = decode_success(rt_leaf(0xABCDEF01u));
    D_DE_CHECK(_sc.ok);
    D_DE_CHECK(_sc.value.payload == 0xABCDEF01u);

    // failure carries a default-constructed value
    decode_result<int> _f = decode_failure<int>();
    D_DE_CHECK(!_f.ok);
    D_DE_CHECK(_f.value == 0);

    decode_result<rt_leaf> _fc = decode_failure<rt_leaf>();
    D_DE_CHECK(!_fc.ok);
    D_DE_CHECK(_fc.value.payload == 0u);

    return true;
}

/*
tests_decode_has_member_decode
  the member-decode probe recognises exactly a correctly-typed static decode
  Tests the following:
  - it is true for a leaf whose static decode returns decode_result<itself>
  - it is false for a type with no decode member at all
  - it is false for a type whose decode returns the WRONG type (the return-type
    check, not merely the name check)
  - it is false for a fundamental type
*/
bool
tests_decode_has_member_decode()
{
    D_DE_CHECK(has_member_decode<rt_leaf>::value);

    D_DE_CHECK(!has_member_decode<dec_no_decode>::value);
    D_DE_CHECK(!has_member_decode<dec_wrong_decode>::value);
    D_DE_CHECK(!has_member_decode<int>::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
