/******************************************************************************
* djinterp [test]                                  cofree_tests_protocol.cpp
*
*   Section III of the cofree.hpp suite: the typeclass registration.  cofree<F,A>
* registers functor_traits AND comonad_traits, so this section checks that the
* generic protocol vocabulary sees it as both: is_functor / is_comonad (and the
* _v shorthands), functor_value_type / comonad_value_type (and cv/ref decay),
* the instance markers (is_specialized / value_type / functor rebind), the
* C++20 Functor and Comonad concepts, and the layering that distinguishes a
* plain functor (opt, a functor but not a comonad) from cofree (both) and from a
* scalar (neither).
*
* path:      /tests/djinterp/core/functional/cofree_tests_protocol.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "cofree_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_is_functor_cofree
  Tests the following:
  - cofree is registered as a functor.
*/
static bool
tests_is_functor_cofree()
{
    return ::djinterp::is_functor< ::djinterp::cofree< opt, int > >::value;
}

/*
tests_is_comonad_cofree
  Tests the following:
  - cofree is registered as a comonad (not derivable from any monad bridge, so
    this exercises the explicit comonad_traits registration).
*/
static bool
tests_is_comonad_cofree()
{
    return ::djinterp::is_comonad< ::djinterp::cofree< opt, int > >::value;
}

/*
tests_functor_value_type_cofree
  Tests the following:
  - functor_value_type_t of a cofree is its head type A.
*/
static bool
tests_functor_value_type_cofree()
{
    return std::is_same<
        ::djinterp::functor_value_type_t< ::djinterp::cofree< opt, int > >,
        int >::value;
}

/*
tests_comonad_value_type_cofree
  Tests the following:
  - comonad_value_type_t of a cofree is its head type A.
*/
static bool
tests_comonad_value_type_cofree()
{
    return std::is_same<
        ::djinterp::comonad_value_type_t< ::djinterp::cofree< opt, int > >,
        int >::value;
}

/*
tests_value_types_decay
  Tests the following:
  - both value-type traits decay their argument (a cv/ref cofree yields the same
    head type).
*/
static bool
tests_value_types_decay()
{
    const bool functor_ok =
        std::is_same<
            ::djinterp::functor_value_type_t< const ::djinterp::cofree< opt, int >& >,
            int >::value;

    const bool comonad_ok =
        std::is_same<
            ::djinterp::comonad_value_type_t< const ::djinterp::cofree< opt, int >& >,
            int >::value;

    return (functor_ok && comonad_ok);
}

/*
tests_functor_traits_markers
  Tests the following:
  - the cofree functor instance publishes is_specialized == true_type,
    value_type == A, and rebind<To> == cofree<F, To>.
*/
static bool
tests_functor_traits_markers()
{
    const bool specialized =
        ::djinterp::functor_traits< ::djinterp::cofree< opt, int > >::is_specialized::value;

    const bool value_is_int =
        std::is_same<
            ::djinterp::functor_traits< ::djinterp::cofree< opt, int > >::value_type,
            int >::value;

    const bool rebinds =
        std::is_same<
            ::djinterp::functor_traits< ::djinterp::cofree< opt, int > >::rebind<std::string>,
            ::djinterp::cofree< opt, std::string > >::value;

    return (specialized && value_is_int && rebinds);
}

/*
tests_comonad_traits_markers
  Tests the following:
  - the cofree comonad instance publishes is_specialized == true_type and
    value_type == A.
*/
static bool
tests_comonad_traits_markers()
{
    const bool specialized =
        ::djinterp::comonad_traits< ::djinterp::cofree< opt, int > >::is_specialized::value;

    const bool value_is_int =
        std::is_same<
            ::djinterp::comonad_traits< ::djinterp::cofree< opt, int > >::value_type,
            int >::value;

    return (specialized && value_is_int);
}

/*
tests_protocol_layering
  Tests the following:
  - a plain functor is not automatically a comonad: opt is a functor but not a
    comonad; cofree is both; a scalar is neither.
*/
static bool
tests_protocol_layering()
{
    const bool opt_functor = ::djinterp::is_functor< opt<int> >::value;         // true
    const bool opt_comonad = ::djinterp::is_comonad< opt<int> >::value;         // false

    const bool cof_functor = ::djinterp::is_functor< ::djinterp::cofree< opt, int > >::value;  // true
    const bool cof_comonad = ::djinterp::is_comonad< ::djinterp::cofree< opt, int > >::value;  // true

    const bool int_functor = ::djinterp::is_functor< int >::value;             // false
    const bool int_comonad = ::djinterp::is_comonad< int >::value;             // false

    return ( opt_functor && (!opt_comonad) &&
             cof_functor && cof_comonad &&
             (!int_functor) && (!int_comonad) );
}


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

/*
tests_is_functor_comonad_v
  Tests the following:
  - the _v shorthands agree with the traits for cofree.  (C++14+.)
*/
static bool
tests_is_functor_comonad_v()
{
    return ( ::djinterp::is_functor_v< ::djinterp::cofree< opt, int > > &&
             ::djinterp::is_comonad_v< ::djinterp::cofree< opt, int > > );
}

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

/*
tests_cofree_concepts
  Tests the following:
  - cofree satisfies both the Functor and the Comonad concepts, and a scalar
    satisfies neither.  (C++20.)
*/
static bool
tests_cofree_concepts()
{
    const bool cof_functor = ::djinterp::Functor< ::djinterp::cofree< opt, int > >;
    const bool cof_comonad = ::djinterp::Comonad< ::djinterp::cofree< opt, int > >;
    const bool int_functor = ::djinterp::Functor< int >;
    const bool int_comonad = ::djinterp::Comonad< int >;

    return ( cof_functor && cof_comonad &&
             (!int_functor) && (!int_comonad) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
cofree_protocol_block()
{
    dt::block_spec block;

    block.name       = "III. registration & protocol";
    block.descriptor =
        "is_functor / is_comonad, value types, markers, concepts, layering";

    block.tests.push_back(dt::test_spec{
        "is_functor: cofree",
        "cofree registered as a functor",
        &tests_is_functor_cofree });

    block.tests.push_back(dt::test_spec{
        "is_comonad: cofree",
        "cofree registered as a comonad",
        &tests_is_comonad_cofree });

    block.tests.push_back(dt::test_spec{
        "functor_value_type",
        "head type A recovered",
        &tests_functor_value_type_cofree });

    block.tests.push_back(dt::test_spec{
        "comonad_value_type",
        "head type A recovered",
        &tests_comonad_value_type_cofree });

    block.tests.push_back(dt::test_spec{
        "value types: cv/ref decay",
        "both traits decay their argument",
        &tests_value_types_decay });

    block.tests.push_back(dt::test_spec{
        "functor instance markers",
        "is_specialized / value_type / rebind",
        &tests_functor_traits_markers });

    block.tests.push_back(dt::test_spec{
        "comonad instance markers",
        "is_specialized / value_type",
        &tests_comonad_traits_markers });

    block.tests.push_back(dt::test_spec{
        "protocol layering",
        "opt: functor not comonad; cofree: both; int: neither",
        &tests_protocol_layering });

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    block.tests.push_back(dt::test_spec{
        "is_functor_v / is_comonad_v",
        "shorthands agree with the traits (C++14+)",
        &tests_is_functor_comonad_v });
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    block.tests.push_back(dt::test_spec{
        "Functor + Comonad concepts",
        "cofree satisfies both, int neither (C++20)",
        &tests_cofree_concepts });
#endif

    return block;
}


NS_END  // testing
NS_END  // djinterp
