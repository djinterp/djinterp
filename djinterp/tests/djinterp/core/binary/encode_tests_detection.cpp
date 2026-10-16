/*******************************************************************************
* djinterp [core]                                     encode_tests_detection.cpp
*
*
* path:      /tests/djinterp/core/binary/encode_tests_detection.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "encode_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_encode_has_push_back_byte
  the sink probe recognises exactly a push_back(byte) surface
  Tests the following:
  - has_push_back_byte is true for the default byte_string sink
  - it is true for a minimal user sink exposing only push_back(byte)
  - it is false for a type with no push_back at all
  - it is false for a fundamental type (int) that cannot be a sink
  - the has_push_back_byte_v companion mirrors the trait's value
*/
bool
tests_encode_has_push_back_byte()
{
    // positive: the default medium and a minimal hand-rolled sink
    D_EN_CHECK(has_push_back_byte<byte_string>::value);
    D_EN_CHECK(has_push_back_byte<recording_sink>::value);

    // negative: a struct without push_back, and a fundamental type
    D_EN_CHECK(!has_push_back_byte<not_a_sink>::value);
    D_EN_CHECK(!has_push_back_byte<int>::value);

    // the _v companion tracks the trait
    D_EN_CHECK(has_push_back_byte_v<byte_string> == has_push_back_byte<byte_string>::value);
    D_EN_CHECK(has_push_back_byte_v<recording_sink> == true);
    D_EN_CHECK(has_push_back_byte_v<not_a_sink> == false);

    return true;
}

/*
tests_encode_has_member_encode_into
  the member-encoder probe recognises exactly a member encode_into(sink)
  Tests the following:
  - has_member_encode_into is true for a leaf carrying encode_into, per sink
  - it holds for both the default byte_string sink and a minimal sink
  - it is false for a plain aggregate with no such member
  - it is false for a fundamental type (int)
  - the probe inspects a const object (a const value can still encode itself)
*/
bool
tests_encode_has_member_encode_into()
{
    // positive: the custom leaf writes itself into either sink type
    D_EN_CHECK((has_member_encode_into<enc_custom_leaf, byte_string>::value));
    D_EN_CHECK((has_member_encode_into<enc_custom_leaf, recording_sink>::value));

    // negative: a plain aggregate and a fundamental type have no member surface
    D_EN_CHECK((!has_member_encode_into<enc_plain, byte_string>::value));
    D_EN_CHECK((!has_member_encode_into<int, byte_string>::value));

    // the probe strips to a const object; a const-qualified argument still matches
    D_EN_CHECK((has_member_encode_into<const enc_custom_leaf, byte_string>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
