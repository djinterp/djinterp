/*******************************************************************************
* djinterp [cpp][container]                                 tree_floor_tests.hpp
*
*   Declarations, the per-suite assertion macro and the spec for the rest of
* the tree floor: tree_path.hpp, avl/AVLtree.hpp, nary/nary_tree_traits.hpp
* and nary/nary_tree_concepts.hpp, the container/tree headers that neither
* the file tree's suite nor the radix trees' covers, and that no other test
* reached. Two faces, selected by DTEST_SPEC_MODE: normal mode (the section
* TUs) and spec mode (the runner, which exposes tree_floor_spec()). Each
* section includes its own header and nothing else of the trees, so each is
* compiled alone.
*
*   The section .cpp files that DEFINE these predicates:
*     tree_floor_tests_path.cpp           - tree_path.hpp
*     tree_floor_tests_avl.cpp            - avl/AVLtree.hpp
*     tree_floor_tests_nary_traits.cpp    - nary/nary_tree_traits.hpp
*     tree_floor_tests_nary_concepts.cpp  - nary/nary_tree_concepts.hpp
*
*
* path:      /tests/djinterp/cpp/container/tree/tree_floor_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#ifndef DJINTERP_TESTS_TREE_FLOOR_TESTS_
#define DJINTERP_TESTS_TREE_FLOOR_TESTS_ 1

// std
#include <cstdio>  // std::printf
// djinterp
#include <djinterp/cpp/djinterp.hpp>  // framework root
#ifdef DTEST_SPEC_MODE
#include <djinterp/cpp/modules/test/test_defaults.hpp>  // module_spec (spec)
#else
#include <array>   // the edge fixture's edges()
#include <vector>  // the container fixture's children()
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest's entities (djinterp::test).
namespace dt = ::djinterp::test;


// tree_floor_check
//   function: routes one D_TF_CHECK evaluation: prints the failing
// expression and its place, and hands the condition back.
inline
bool tree_floor_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}

// D_TF_CHECK
//   macro: fail the current test unless the condition holds, printing the
// expression that failed. Variadic, so a condition may hold top-level commas.
#define D_TF_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::tree_floor_check(                           \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// the n-ary node fixtures, one per child model, for sections III and IV

// DTfLcrsFields
//   fixture: left-child, right-sibling links as fields, and a parent.
struct DTfLcrsFields
{
    DTfLcrsFields* first_child;
    DTfLcrsFields* next_sibling;
    DTfLcrsFields* parent;
};

// DTfLcrsMethods
//   fixture: the same links as const methods.
struct DTfLcrsMethods
{
    DTfLcrsMethods* first_child() const { return nullptr; }
    DTfLcrsMethods* next_sibling() const { return nullptr; }
    DTfLcrsMethods* prev_sibling() const { return nullptr; }
    DTfLcrsMethods* last_child() const { return nullptr; }
};

// DTfContainerNode
//   fixture: children() returns an iterable container.
struct DTfContainerNode
{
    std::vector<DTfContainerNode*> children() const { return {}; }
};

// DTfEdgeNode
//   fixture: edges() returns an array; edge_count() counts it.
struct DTfEdgeNode
{
    std::array<DTfEdgeNode*, 4> edges() const { return {}; }
    std::size_t                 edge_count() const { return 0; }
};

// DTfHybridNode
//   fixture: two models at once, LCRS fields and children().
struct DTfHybridNode
{
    DTfHybridNode* first_child;
    DTfHybridNode* next_sibling;

    std::vector<DTfHybridNode*> children() const { return {}; }
};

// DTfPlain
//   fixture: no child model at all.
struct DTfPlain
{
    int value;
};

// DTfRootedTree
//   fixture: a tree, root() and children(); DTfRootlessTree lacks root().
struct DTfRootedTree
{
    DTfContainerNode*              root() const { return nullptr; }
    std::vector<DTfContainerNode*> children() const { return {}; }
};

struct DTfRootlessTree
{
    std::vector<DTfContainerNode*> children() const { return {}; }
};

#endif  // !DTEST_SPEC_MODE


// I.    tree_path.hpp   (tree_floor_tests_path.cpp)
bool tests_tree_floor_path_resolve_and_build();
bool tests_tree_floor_path_relations();

// II.   avl/AVLtree.hpp   (tree_floor_tests_avl.cpp)
bool tests_tree_floor_avl_alone();

// III.  nary/nary_tree_traits.hpp   (tree_floor_tests_nary_traits.cpp)
bool tests_tree_floor_nary_traits_access();
bool tests_tree_floor_nary_traits_classification();

// IV.   nary/nary_tree_concepts.hpp   (tree_floor_tests_nary_concepts.cpp)
bool tests_tree_floor_nary_concepts();


#ifdef DTEST_SPEC_MODE

// tree_floor_spec
//   function: the suite's description: one block per section, one row per
// tests_* body.
inline
dt::module_spec tree_floor_spec()
{
    return dt::module_spec{
        "tree_floor",
        "The rest of the tree floor: the arena path helper, the AVL header, "
        "and the n-ary trees' traits and concepts, each compiled alone and "
        "checked against what it documents.",
        {
            dt::block_spec{
                "path",
                "tree_path.hpp over a node arena of names.",
                {
                    { "tests_tree_floor_path_resolve_and_build",
                      "resolve walks names from a node; build joins them.",
                      &tests_tree_floor_path_resolve_and_build },
                    { "tests_tree_floor_path_relations",
                      "depth, ancestors, lca, is_ancestor and the relative "
                      "path.",
                      &tests_tree_floor_path_relations },
                }
            },
            dt::block_spec{
                "avl",
                "avl/AVLtree.hpp, which declares nothing yet.",
                {
                    { "tests_tree_floor_avl_alone",
                      "The header compiles alone, at the suite's level.",
                      &tests_tree_floor_avl_alone },
                }
            },
            dt::block_spec{
                "nary_traits",
                "nary/nary_tree_traits.hpp on node fixtures of each child "
                "model.",
                {
                    { "tests_tree_floor_nary_traits_access",
                      "Each access probe finds a field or a method, and "
                      "nothing on a type without one.",
                      &tests_tree_floor_nary_traits_access },
                    { "tests_tree_floor_nary_traits_classification",
                      "child_access_strategy names each model, hybrid for "
                      "two, none for none; is_nary_tree_node agrees.",
                      &tests_tree_floor_nary_traits_classification },
                }
            },
            dt::block_spec{
                "nary_concepts",
                "nary/nary_tree_concepts.hpp (C++20).",
                {
                    { "tests_tree_floor_nary_concepts",
                      "The concepts agree with the traits on every fixture.",
                      &tests_tree_floor_nary_concepts },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_TREE_FLOOR_TESTS_
