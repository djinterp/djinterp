/*******************************************************************************
* djinterp [test]                                     cofree_tests_recursion.cpp
*
*   Sections IV-VI of the cofree.hpp suite: the RECURSION face of cofree, merged
* into the header so a translation unit that sees cofree.hpp sees the fold too.
* Section IV is env_layer<B, F, X> -- the environment pairing B x F<X> that is
* cofree's base functor -- and its Functor instance, which maps the recursive
* holes while leaving the annotation alone.  Section V registers
* recursive_traits<cofree<F, B>> over that base, so the universal cata peels a
* cofree.  Section VI is annotated_cata, the binary-algebra face that hands an
* algebra a node's annotation and its ALREADY-FOLDED children side by side.
*
*   THE REGISTRATION TEST IS A REGRESSION TEST.  is_recursive<cofree<opt,int>>
* must be true from cofree.hpp ALONE.  While the registration lived in a
* separate header a TU that included only cofree.hpp saw it as false and one
* that included the extra header saw true -- the same specialisation answering
* differently per TU, which is an ODR violation with no diagnostic.  The first
* test below pins that down: this file includes nothing but cofree_tests.hpp
* (hence cofree.hpp), so if the registration is ever split out again, it fails.
*
*   Fixtures are the suite's existing ones: opt<T> as F, so a cofree<opt, A> is
* a non-empty descending chain, and the named algebras (sum_alg / len_alg /
* max_alg / join_alg) declared in cofree_tests.hpp -- named rather than lambdas
* so they may appear in trailing return types on every language floor.
*
*
* path:      /tests/djinterp/core/functional/cofree_tests_recursion.cpp
* link(s):   ch-recursion.tex
* author(s): djinterp DTest suite                            created: 2026.07.23
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "cofree_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                IV.  env_layer AND ITS FUNCTOR                            ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_env_layer_is_functor
  Tests the following:
  - env_layer is registered as a Functor, so cata can rebuild an annotated
    layer at the result type while folding.
*/
static bool
tests_env_layer_is_functor()
{
    return ::djinterp::is_functor<
        ::djinterp::env_layer< int, opt, int > >::value;
}

/*
tests_env_layer_markers
  Tests the following:
  - the env_layer functor instance publishes is_specialized == true_type,
    value_type == X (the hole type, not the annotation), and
    rebind<To> == env_layer<B, F, To>.
*/
static bool
tests_env_layer_markers()
{
    typedef ::djinterp::env_layer< int, opt, int > env_t;

    const bool specialized =
        ::djinterp::functor_traits< env_t >::is_specialized::value;

    const bool value_is_hole =
        std::is_same<
            ::djinterp::functor_traits< env_t >::value_type,
            int >::value;

    const bool rebinds =
        std::is_same<
            ::djinterp::functor_traits< env_t >::rebind<std::string>,
            ::djinterp::env_layer< int, opt, std::string > >::value;

    return (specialized && value_is_hole && rebinds);
}

/*
tests_env_layer_map_holes
  Tests the following:
  - mapping an env_layer applies the function to the recursive holes and leaves
    the annotation untouched: *2 over {ann 7, layer just(4)} gives
    {ann 7, layer just(8)}.
*/
static bool
tests_env_layer_map_holes()
{
    const ::djinterp::env_layer< int, opt, int > e = { 7, opt<int>(4) };

    const ::djinterp::env_layer< int, opt, int > m =
        ::djinterp::functor_map(e, dbl());

    return ( (m.ann == 7) &&
             m.layer.has &&
             (m.layer.val == 8) );
}

/*
tests_env_layer_map_empty
  Tests the following:
  - an empty F-layer is preserved by map, and the annotation still passes
    through (the leaf case cata hits at the bottom of every fold).
*/
static bool
tests_env_layer_map_empty()
{
    const ::djinterp::env_layer< int, opt, int > e = { 7, opt<int>() };

    const ::djinterp::env_layer< int, opt, int > m =
        ::djinterp::functor_map(e, dbl());

    return ( (m.ann == 7) &&
             (!m.layer.has) );
}

/*
tests_env_layer_map_type_change
  Tests the following:
  - map may change the HOLE type while the annotation type is fixed:
    int -> string over the holes yields env_layer<int, opt, std::string>.
*/
static bool
tests_env_layer_map_type_change()
{
    const ::djinterp::env_layer< int, opt, int > e = { 7, opt<int>(4) };

    const ::djinterp::env_layer< int, opt, std::string > m =
        ::djinterp::functor_map(e, show());

    return ( (m.ann == 7) &&
             m.layer.has &&
             (m.layer.val == "4") );
}

/*
tests_env_layer_map_result_type
  Tests the following:
  - the result type of mapping an env_layer is env_layer<B, F, mapped hole>.
*/
static bool
tests_env_layer_map_result_type()
{
    return std::is_same<
        decltype(::djinterp::functor_map(
            std::declval< const ::djinterp::env_layer< int, opt, int >& >(),
            std::declval< show >())),
        ::djinterp::env_layer< int, opt, std::string > >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                V.   RECURSION REGISTRATION                               ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_is_recursive_cofree
  Tests the following:
  - cofree is registered as a foldable carrier, seen from cofree.hpp ALONE.
    This is the regression test for the split-registration ODR hazard: this TU
    includes only cofree_tests.hpp (hence cofree.hpp), so if recursive_traits
    is ever moved back out to a companion header, this fails.
*/
static bool
tests_is_recursive_cofree()
{
    return ::djinterp::is_recursive< ::djinterp::cofree< opt, int > >::value;
}

/*
tests_recursive_traits_markers
  Tests the following:
  - the cofree recursion instance publishes is_specialized == true_type.
*/
static bool
tests_recursive_traits_markers()
{
    return ::djinterp::recursive_traits<
        ::djinterp::cofree< opt, int > >::is_specialized::value;
}

/*
tests_recursive_base_is_env_layer
  Tests the following:
  - the base functor of cofree<F, B> is env_layer<B, F, X> -- the annotated
    layer, NOT the bare F<X> that mu's instance uses.
*/
static bool
tests_recursive_base_is_env_layer()
{
    typedef ::djinterp::cofree< opt, int > cf_t;

    const bool base_is_env =
        std::is_same<
            ::djinterp::recursive_traits< cf_t >::base<int>,
            ::djinterp::env_layer< int, opt, int > >::value;

    const bool base_is_not_bare_layer =
        (!std::is_same<
            ::djinterp::recursive_traits< cf_t >::base<int>,
            opt<int> >::value);

    return (base_is_env && base_is_not_bare_layer);
}

/*
tests_project_reads_head_and_children
  Tests the following:
  - project peels one layer as (head, F(cofree)): over [3,2,1] the annotation is
    3 and the layer holds the child whose own head is 2.
*/
static bool
tests_project_reads_head_and_children()
{
    typedef ::djinterp::cofree< opt, int > cf_t;

    const cf_t chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const ::djinterp::env_layer< int, opt, cf_t > peeled =
        ::djinterp::recursive_traits< cf_t >::project(chain);

    return ( (peeled.ann == 3) &&
             peeled.layer.has &&
             (peeled.layer.val.head() == 2) );
}

/*
tests_project_derefs_children
  Tests the following:
  - project dereferences the stored shared_ptr children, so the recursive holes
    are BARE cofree values -- the positions cata folds.
*/
static bool
tests_project_derefs_children()
{
    typedef ::djinterp::cofree< opt, int > cf_t;

    return std::is_same<
        decltype(::djinterp::recursive_traits< cf_t >::project(
            std::declval< const cf_t& >())),
        ::djinterp::env_layer< int, opt, cf_t > >::value;
}

/*
tests_project_leaf
  Tests the following:
  - projecting a leaf gives its annotation and an empty layer (the base case of
    every fold).
*/
static bool
tests_project_leaf()
{
    typedef ::djinterp::cofree< opt, int > cf_t;

    const cf_t leaf = cf_t::make(5, cf_t::layer_type());

    const ::djinterp::env_layer< int, opt, cf_t > peeled =
        ::djinterp::recursive_traits< cf_t >::project(leaf);

    return ( (peeled.ann == 5) &&
             (!peeled.layer.has) );
}


///////////////////////////////////////////////////////////////////////////////
///                VI.  ANNOTATED FOLD                                       ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_annotated_cata_sum
  Tests the following:
  - annotated_cata folds the whole tree: summing the heads of [3,2,1] gives 6.
*/
static bool
tests_annotated_cata_sum()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    return (::djinterp::annotated_cata<int>(sum_alg(), chain) == 6);
}

/*
tests_annotated_cata_children_pre_folded
  Tests the following:
  - the algebra receives F<Result>, not F<cofree>: len_alg only ever adds one to
    an already-computed child count, so a five-node chain folds to 5.  If the
    children arrived unfolded this could not typecheck, let alone count.
*/
static bool
tests_annotated_cata_children_pre_folded()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(5, id_head(), descend());

    return (::djinterp::annotated_cata<std::size_t>(len_alg(), chain) ==
            static_cast<std::size_t>(5));
}

/*
tests_annotated_cata_leaf
  Tests the following:
  - a leaf folds to its own annotation (the empty layer contributes nothing).
*/
static bool
tests_annotated_cata_leaf()
{
    typedef ::djinterp::cofree< opt, int > cf_t;

    const cf_t leaf = cf_t::make(5, cf_t::layer_type());

    return (::djinterp::annotated_cata<int>(sum_alg(), leaf) == 5);
}

/*
tests_annotated_cata_result_type_change
  Tests the following:
  - the Result type is independent of the annotation type: folding a
    cofree<opt,int> with join_alg yields the std::string "3,2,1".
*/
static bool
tests_annotated_cata_result_type_change()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    return (::djinterp::annotated_cata<std::string>(join_alg(), chain) ==
            "3,2,1");
}

/*
tests_annotated_cata_many_algebras
  Tests the following:
  - one fold, many algebras: the SAME tree yields its head sum (6), its node
    count (3), and its greatest head (3) under three different algebras, with
    the structural recursion written once.
*/
static bool
tests_annotated_cata_many_algebras()
{
    const ::djinterp::cofree< opt, int > chain =
        ::djinterp::unfold_cofree< opt >(3, id_head(), descend());

    const int         total = ::djinterp::annotated_cata<int>(sum_alg(), chain);
    const std::size_t count =
        ::djinterp::annotated_cata<std::size_t>(len_alg(), chain);
    const int         top   = ::djinterp::annotated_cata<int>(max_alg(), chain);

    return ( (total == 6) &&
             (count == static_cast<std::size_t>(3)) &&
             (top   == 3) );
}

/*
tests_annotated_cata_matches_raw_cata
  Tests the following:
  - annotated_cata is exactly a face over cata: folding with the binary algebra
    agrees with folding the same tree through cata with an env_layer algebra.
*/
static bool
tests_annotated_cata_matches_raw_cata()
{
    typedef ::djinterp::cofree< opt, int > cf_t;

    const cf_t chain =
        ::djinterp::unfold_cofree< opt >(4, id_head(), descend());

    const int via_annotated =
        ::djinterp::annotated_cata<int>(sum_alg(), chain);

    const int via_raw =
        ::djinterp::cata<int>(
            [](const ::djinterp::env_layer< int, opt, int >& _env) -> int
            {
                return _env.ann + (_env.layer.has ? _env.layer.val : 0);
            },
            chain);

    return (via_annotated == via_raw);
}

/*
tests_annotated_cata_result_type
  Tests the following:
  - the result type of annotated_cata is the explicitly supplied Result.
*/
static bool
tests_annotated_cata_result_type()
{
    return std::is_same<
        decltype(::djinterp::annotated_cata<std::string>(
            std::declval< join_alg >(),
            std::declval< const ::djinterp::cofree< opt, int >& >())),
        std::string >::value;
}


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

/*
tests_is_recursive_v_cofree
  Tests the following:
  - the _v shorthand agrees with the trait for cofree.  (C++14+.)
*/
static bool
tests_is_recursive_v_cofree()
{
    return ::djinterp::is_recursive_v< ::djinterp::cofree< opt, int > >;
}

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

/*
tests_recursive_concept_cofree
  Tests the following:
  - cofree satisfies the Recursive concept and a scalar does not.  (C++20.)
*/
static bool
tests_recursive_concept_cofree()
{
    const bool cof_recursive =
        ::djinterp::Recursive< ::djinterp::cofree< opt, int > >;
    const bool int_recursive = ::djinterp::Recursive< int >;

    return (cof_recursive && (!int_recursive));
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
cofree_recursion_block()
{
    dt::block_spec block;

    block.name       = "IV-VI. env_layer / recursion / annotated_cata";
    block.descriptor =
        "annotated base functor; recursive_traits registration; annotated fold";

    // -- IV. env_layer and its Functor --

    block.tests.push_back(dt::test_spec{
        "is_functor: env_layer",
        "the annotated layer is a registered functor",
        &tests_env_layer_is_functor });

    block.tests.push_back(dt::test_spec{
        "env_layer instance markers",
        "is_specialized / value_type is the hole / rebind",
        &tests_env_layer_markers });

    block.tests.push_back(dt::test_spec{
        "map: holes mapped, annotation kept",
        "*2 over {7, just 4} -> {7, just 8}",
        &tests_env_layer_map_holes });

    block.tests.push_back(dt::test_spec{
        "map: empty layer preserved",
        "the leaf case, annotation still passes through",
        &tests_env_layer_map_empty });

    block.tests.push_back(dt::test_spec{
        "map: hole type change",
        "int->string holes, annotation type fixed",
        &tests_env_layer_map_type_change });

    block.tests.push_back(dt::test_spec{
        "map result type",
        "env_layer<B, F, mapped hole>",
        &tests_env_layer_map_result_type });

    // -- V. recursion registration --

    block.tests.push_back(dt::test_spec{
        "is_recursive: cofree (from cofree.hpp alone)",
        "regression: registration must not be split out again",
        &tests_is_recursive_cofree });

    block.tests.push_back(dt::test_spec{
        "recursive instance markers",
        "is_specialized",
        &tests_recursive_traits_markers });

    block.tests.push_back(dt::test_spec{
        "base functor is env_layer",
        "cofree<F,B> peels to B x F<X>, not bare F<X>",
        &tests_recursive_base_is_env_layer });

    block.tests.push_back(dt::test_spec{
        "project: head and children",
        "[3,2,1] -> ann 3, child head 2",
        &tests_project_reads_head_and_children });

    block.tests.push_back(dt::test_spec{
        "project: children dereferenced",
        "holes are bare cofree, not shared_ptr",
        &tests_project_derefs_children });

    block.tests.push_back(dt::test_spec{
        "project: leaf",
        "annotation plus an empty layer",
        &tests_project_leaf });

    // -- VI. annotated fold --

    block.tests.push_back(dt::test_spec{
        "annotated_cata: sum of heads",
        "[3,2,1] -> 6",
        &tests_annotated_cata_sum });

    block.tests.push_back(dt::test_spec{
        "children arrive already folded",
        "algebra sees F<Result>: 5-node chain counts to 5",
        &tests_annotated_cata_children_pre_folded });

    block.tests.push_back(dt::test_spec{
        "annotated_cata: leaf",
        "a leaf folds to its own annotation",
        &tests_annotated_cata_leaf });

    block.tests.push_back(dt::test_spec{
        "result type independent of annotation",
        "cofree<opt,int> folded to \"3,2,1\"",
        &tests_annotated_cata_result_type_change });

    block.tests.push_back(dt::test_spec{
        "one fold, many algebras",
        "sum / count / max over one tree",
        &tests_annotated_cata_many_algebras });

    block.tests.push_back(dt::test_spec{
        "annotated_cata == raw cata",
        "binary face agrees with an env_layer algebra",
        &tests_annotated_cata_matches_raw_cata });

    block.tests.push_back(dt::test_spec{
        "annotated_cata result type",
        "the explicitly supplied Result",
        &tests_annotated_cata_result_type });

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    block.tests.push_back(dt::test_spec{
        "is_recursive_v",
        "shorthand agrees with the trait (C++14+)",
        &tests_is_recursive_v_cofree });
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    block.tests.push_back(dt::test_spec{
        "Recursive concept",
        "cofree satisfies it, int does not (C++20)",
        &tests_recursive_concept_cofree });
#endif

    return block;
}


NS_END  // testing
NS_END  // djinterp
