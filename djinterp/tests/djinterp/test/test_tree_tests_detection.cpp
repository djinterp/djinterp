// djinterp
#include "test_tree_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_tree_is_test_tree_true_for_instantiation
  Verifies is_test_tree recognises instantiations of the class template,
  including a non-default (rank-disabled) instantiation.
  Tests the following:
  - is_test_tree<test_tree<...>>::value == true (default + variant)
*/
bool
tests_tree_is_test_tree_true_for_instantiation()
{
    using tree_no_rank =
        test_tree<basic_test,
                  nary_tree<basic_test>,
                  std::vector<test_kind>,
                  false>;

    static_assert(is_test_tree<test_tree<basic_test>>::value,
                  "default test_tree should be recognised");
    static_assert(is_test_tree<tree_no_rank>::value,
                  "rank-disabled test_tree should be recognised");

    return ( is_test_tree<test_tree<basic_test>>::value &&
             is_test_tree<tree_no_rank>::value );
}

/*
tests_tree_is_test_tree_false_for_non_tree
  Verifies is_test_tree rejects unrelated types - scalars, the element type,
  the kind container, and the bare backing.
  Tests the following:
  - is_test_tree<int>, <basic_test>, <std::vector<test_kind>>,
    <nary_tree<basic_test>> are all false
*/
bool
tests_tree_is_test_tree_false_for_non_tree()
{
    static_assert(!is_test_tree<int>::value, "");
    static_assert(!is_test_tree<basic_test>::value, "");
    static_assert(!is_test_tree<std::vector<test_kind>>::value, "");
    static_assert(!is_test_tree<nary_tree<basic_test>>::value, "");

    return ( !is_test_tree<int>::value                    &&
             !is_test_tree<basic_test>::value             &&
             !is_test_tree<std::vector<test_kind>>::value &&
             !is_test_tree<nary_tree<basic_test>>::value );
}

/*
tests_tree_is_test_tree_strips_cv_ref
  Verifies is_test_tree applies clean_t, so cv- and reference-qualified forms
  agree with the bare type.
  Tests the following:
  - const, lvalue-ref, const-lvalue-ref, rvalue-ref, and volatile forms are
    all recognised
*/
bool
tests_tree_is_test_tree_strips_cv_ref()
{
    using tree = test_tree<basic_test>;

    static_assert(is_test_tree<const tree>::value, "");
    static_assert(is_test_tree<tree&>::value, "");
    static_assert(is_test_tree<const tree&>::value, "");
    static_assert(is_test_tree<tree&&>::value, "");
    static_assert(is_test_tree<volatile tree>::value, "");

    return ( is_test_tree<const tree>::value  &&
             is_test_tree<tree&>::value       &&
             is_test_tree<const tree&>::value &&
             is_test_tree<tree&&>::value      &&
             is_test_tree<volatile tree>::value );
}

/*
tests_tree_is_test_tree_value_alias
  Verifies the is_test_tree_v variable-template companion mirrors the trait,
  where variable templates are available.
  Tests the following:
  - is_test_tree_v is true for a test_tree and false for a non-tree (C++14+)
*/
bool
tests_tree_is_test_tree_value_alias()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(::djinterp::test::is_test_tree_v<test_tree<basic_test>>,
                  "");
    static_assert(!::djinterp::test::is_test_tree_v<int>, "");

    return ( (::djinterp::test::is_test_tree_v<test_tree<basic_test>>
                  == true) &&
             (::djinterp::test::is_test_tree_v<int>
                  == false) );
#else
    // variable templates unavailable (pre-C++14): nothing to check
    return true;
#endif
}

/*
tests_tree_test_tree_concept
  Verifies the test_tree_type concept mirrors is_test_tree, where concepts are
  available.
  Tests the following:
  - test_tree_type is satisfied by a test_tree and not by a non-tree (C++20+)
*/
bool
tests_tree_test_tree_concept()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(::djinterp::test::test_tree_type<test_tree<basic_test>>,
                  "");
    static_assert(!::djinterp::test::test_tree_type<int>, "");

    return ( ::djinterp::test::test_tree_type<test_tree<basic_test>> &&
             !::djinterp::test::test_tree_type<int> );
#else
    // concepts unavailable (pre-C++20): nothing to check
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
