/*******************************************************************************
* djinterp [test]                                     concepts_tests_numeric.cpp
*
*   Tests for concepts.hpp section II.10: size and numeric concepts.
* Covers: valid_size_type, nonzero_size, zero_size.
*
*   Edge cases of particular interest:
*     - valid_size_type requires BOTH `is_unsigned` AND `is_arithmetic`.
*       The arithmetic check is what makes a hypothetical unsigned-
*       qualified non-arithmetic type fail (no such type exists in
*       built-ins, but the concept is defensive about it).
*     - nonzero_size and zero_size take a `std::size_t _N` non-type
*       template parameter and partition it: every N is in exactly one.
*
*
* path:      /tests/djinterp/core/meta/concepts_tests_numeric.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <climits>
// djinterp
#include "concepts_tests.hpp"


NS_DJINTERP_TEST


void
concepts_tests_numeric(
    test_handler& _test_handler,
    test_option   _options...)
{
    using fixtures::unscoped_enum_t;
    using fixtures::trivial_class;

    // -------------------------------------------------------------------
    // valid_size_type -- unsigned AND arithmetic
    // -------------------------------------------------------------------

    DT_CONCEPT    (valid_size_type<unsigned int>);
    DT_CONCEPT    (valid_size_type<unsigned long>);
    DT_CONCEPT    (valid_size_type<unsigned long long>);
    DT_CONCEPT    (valid_size_type<unsigned short>);
    DT_CONCEPT    (valid_size_type<unsigned char>);
    DT_CONCEPT    (valid_size_type<std::size_t>);
    DT_CONCEPT    (valid_size_type<bool>);                 // bool is "unsigned arithmetic"

    //   signed types fail
    DT_NOT_CONCEPT(valid_size_type<int>);
    DT_NOT_CONCEPT(valid_size_type<long>);
    DT_NOT_CONCEPT(valid_size_type<signed char>);

    //   non-arithmetic fails
    DT_NOT_CONCEPT(valid_size_type<void>);
    DT_NOT_CONCEPT(valid_size_type<int*>);
    DT_NOT_CONCEPT(valid_size_type<trivial_class>);
    DT_NOT_CONCEPT(valid_size_type<unscoped_enum_t>);      // enum is NOT arithmetic

    //   floating-point fails (signed)
    DT_NOT_CONCEPT(valid_size_type<float>);
    DT_NOT_CONCEPT(valid_size_type<double>);

    // -------------------------------------------------------------------
    // nonzero_size -- _N != 0
    // -------------------------------------------------------------------

    DT_CONCEPT    (nonzero_size<1>);
    DT_CONCEPT    (nonzero_size<2>);
    DT_CONCEPT    (nonzero_size<42>);
    DT_CONCEPT    (nonzero_size<SIZE_MAX>);
    DT_NOT_CONCEPT(nonzero_size<0>);

    // -------------------------------------------------------------------
    // zero_size -- _N == 0
    // -------------------------------------------------------------------

    DT_CONCEPT    (zero_size<0>);
    DT_NOT_CONCEPT(zero_size<1>);
    DT_NOT_CONCEPT(zero_size<42>);
    DT_NOT_CONCEPT(zero_size<SIZE_MAX>);

    //   structural invariant: nonzero_size and zero_size partition
    // std::size_t.  For any N exactly one of them is satisfied.
    DT_CONCEPT(zero_size<0>     != nonzero_size<0>);
    DT_CONCEPT(zero_size<1>     != nonzero_size<1>);
    DT_CONCEPT(zero_size<100>   != nonzero_size<100>);
    DT_CONCEPT(zero_size<0>      || nonzero_size<0>);
    DT_CONCEPT(zero_size<7>      || nonzero_size<7>);

    return;
}


NS_END  // test
NS_END  // djinterp
