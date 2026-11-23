/*******************************************************************************
* djinterp [cpp][container]                                      arena_tests.hpp
*
*   Declarations, fixtures, the per-suite assertion macro and the spec for the
* container/arena/ unit tests (the floor suite): arena.hpp, arena_traits.hpp,
* arena_concepts.hpp, and tree/'s arena_tree.hpp, arena_tree_traits.hpp and
* arena_tree_concepts.hpp.  Two faces, selected by DTEST_SPEC_MODE: normal mode
* (the section TUs) pulls in the fixtures; spec mode (the runner) pulls in
* test_defaults.hpp and exposes arena_spec().  Each section includes its own
* header and nothing else of the arena, so every header is compiled alone.
*
*   The section .cpp files that DEFINE these predicates:
*     arena_tests_links.cpp        - arena.hpp I to IV: flags, policies, presets
*     arena_tests_node.cpp         - arena.hpp V: arena_node
*     arena_tests_arena.cpp        - arena.hpp VI: the arena container
*     arena_tests_traits.cpp       - arena_traits.hpp
*     arena_tests_concepts.cpp     - arena_concepts.hpp (C++20)
*     arena_tests_tree.cpp         - tree/arena_tree.hpp
*     arena_tests_tree_traits.cpp  - tree/arena_tree_traits.hpp and
*                                    tree/arena_tree_concepts.hpp
*
*
* path:      /tests/djinterp/cpp/container/arena/arena_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
#ifndef DJINTERP_CONTAINER_ARENA_TESTS_
#define DJINTERP_CONTAINER_ARENA_TESTS_ 1

// std
#include <cstdio>   // std::printf
#include <memory>   // std::unique_ptr
#include <vector>   // std::vector
// djinterp
#include <djinterp/cpp/djinterp.hpp>  // framework root
#ifdef DTEST_SPEC_MODE
#include <djinterp/cpp/modules/test/test_defaults.hpp>  // module_spec (spec)
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest's entities (djinterp::test).
namespace dt = ::djinterp::test;


// arena_check
//   function: routes one D_AN_CHECK evaluation.  Prints the failing
// expression and its location and hands the condition back.
inline
bool arena_check(
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

// D_AN_CHECK
//   macro: fail the current test - return false from the enclosing predicate -
// unless the condition holds, printing the expression that failed.  Variadic
// so a condition may contain top-level commas.  Unique letters (AN) so
// co-compiled suites never collide on the macro name.
#define D_AN_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::arena_check(                                \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // fixtures -- normal mode only

// ===========================================================================
// Fixtures - payloads
// ===========================================================================

// an_payload
//   type: a small payload with equality, for the node and arena tests.
struct an_payload
{
    int value;

    friend bool
    operator==(const an_payload& _a,
               const an_payload& _b)
    {
        return (_a.value == _b.value);
    }
};

// an_move_only
//   type: movable but not copyable - still a valid payload.
struct an_move_only
{
    std::unique_ptr<int> p;
};

// an_immovable
//   type: neither copyable nor movable - not a valid payload.
struct an_immovable
{
    an_immovable() = default;
    an_immovable(const an_immovable&) = delete;
    an_immovable(an_immovable&&)      = delete;
};

// an_indestructible
//   type: its destructor is deleted - not a valid payload.
struct an_indestructible
{
    ~an_indestructible() = delete;
};


// ===========================================================================
// Fixtures - structural look-alikes
// ===========================================================================

// an_arena_like
//   type: the three things is_arena looks for (payload_type, link_policy and
// operator[](node_id)), and nothing of arena<>'s.
struct an_arena_like
{
    using payload_type = int;
    using link_policy  = int;

    int operator[](unsigned) const
    {
        return 0;
    }
};

// an_rooted_like
//   type: an_arena_like with root() and has_root(): the arena tree
// protocol, structurally.
struct an_rooted_like : an_arena_like
{
    unsigned root() const
    {
        return 0;
    }

    bool has_root() const
    {
        return false;
    }
};

// an_node_like
//   type: the five members is_arena_node looks for.
struct an_node_like
{
    using payload_type = int;
    using link_policy  = int;

    static constexpr unsigned num_links = 0;

    unsigned long long stable_id;
    bool               alive;
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// ===========================================================================
// Declarations  (grouped by section file)
// ===========================================================================

// links: arena.hpp I to IV
bool tests_arena_link_flags();
bool tests_arena_link_policy_presets();
bool tests_arena_link_policy_indices();

// node: arena.hpp V
bool tests_arena_node_default_state();
bool tests_arena_node_constructed_state();
bool tests_arena_node_links();
bool tests_arena_node_equality();

// arena: arena.hpp VI
bool tests_arena_construction();
bool tests_arena_allocation();
bool tests_arena_free_list();
bool tests_arena_append_prepend();
bool tests_arena_insert();
bool tests_arena_detach_linked();
bool tests_arena_detach_unparented();
bool tests_arena_move_subtree();
bool tests_arena_collection_and_depth();
bool tests_arena_traversal();
bool tests_arena_identity();
bool tests_arena_clear_and_swap();
bool tests_arena_equality();
bool tests_arena_equivalence();

// traits: arena_traits.hpp
bool tests_arena_traits_payload();
bool tests_arena_traits_link_policy();
bool tests_arena_traits_node();
bool tests_arena_traits_arena();
bool tests_arena_traits_class();

// concepts: arena_concepts.hpp
bool tests_arena_concepts_payload_and_policy();
bool tests_arena_concepts_node_and_arena();

// tree: tree/arena_tree.hpp
bool tests_arena_tree_root();
bool tests_arena_tree_add();
bool tests_arena_tree_remove();
bool tests_arena_tree_remove_subtree();
bool tests_arena_tree_clear_swap_equality();
bool tests_arena_tree_equivalence();
bool tests_arena_tree_aliases();

// tree traits: tree/arena_tree_traits.hpp, tree/arena_tree_concepts.hpp
bool tests_arena_tree_traits_identity();
bool tests_arena_tree_traits_methods();
bool tests_arena_tree_traits_navigation();
bool tests_arena_tree_traits_class();
bool tests_arena_tree_concepts();


// ===========================================================================
// The spec  (spec mode only)
// ===========================================================================
#ifdef DTEST_SPEC_MODE

// arena_spec
//   function: the authoritative description of this suite, one block per
// section file.
inline
dt::module_spec arena_spec()
{
    return dt::module_spec{
        "arena",
        "container/arena/: the link flags and policies, arena_node, the arena "
        "container, its traits and concepts, and the arena tree with its "
        "traits and concepts.",
        {
            dt::block_spec{
                "links",
                "arena.hpp I to IV: the flags, the policy generator, the presets.",
                {
                    { "tests_arena_link_flags",
                      "The seven link flags are distinct single bits.",
                      &tests_arena_link_flags },
                    { "tests_arena_link_policy_presets",
                      "Each preset has the links its name says, and only those.",
                      &tests_arena_link_policy_presets },
                    { "tests_arena_link_policy_indices",
                      "Enabled links take sequential indices in flag order; "
                      "disabled ones read std::size_t(-1).",
                      &tests_arena_link_policy_indices },
                }
            },
            dt::block_spec{
                "node",
                "arena.hpp V: arena_node.",
                {
                    { "tests_arena_node_default_state",
                      "A default node is dead, unlinked and anonymous.",
                      &tests_arena_node_default_state },
                    { "tests_arena_node_constructed_state",
                      "A constructed node is live, version 1, unlinked.",
                      &tests_arena_node_constructed_state },
                    { "tests_arena_node_links",
                      "Links by index and by name; children and leaves; "
                      "reset.",
                      &tests_arena_node_links },
                    { "tests_arena_node_equality",
                      "Nodes compare their observable state.",
                      &tests_arena_node_equality },
                }
            },
            dt::block_spec{
                "arena",
                "arena.hpp VI: the arena container.",
                {
                    { "tests_arena_construction",
                      "A new arena is empty, with stable ids from 1.",
                      &tests_arena_construction },
                    { "tests_arena_allocation",
                      "Allocation assigns slots, stable ids and version 1.",
                      &tests_arena_allocation },
                    { "tests_arena_free_list",
                      "A freed slot is reused first, with a fresh identity.",
                      &tests_arena_free_list },
                    { "tests_arena_append_prepend",
                      "append_child and prepend_child keep every link.",
                      &tests_arena_append_prepend },
                    { "tests_arena_insert",
                      "insert_after and insert_before keep every link.",
                      &tests_arena_insert },
                    { "tests_arena_detach_linked",
                      "detach with a parent link unlinks and keeps the subtree.",
                      &tests_arena_detach_linked },
                    { "tests_arena_detach_unparented",
                      "detach without a parent link unlinks correctly.",
                      &tests_arena_detach_unparented },
                    { "tests_arena_move_subtree",
                      "move_subtree re-parents a whole subtree.",
                      &tests_arena_move_subtree },
                    { "tests_arena_collection_and_depth",
                      "Subtree, children, ancestors, depth and child count.",
                      &tests_arena_collection_and_depth },
                    { "tests_arena_traversal",
                      "Depth-first is pre-order, breadth-first by level.",
                      &tests_arena_traversal },
                    { "tests_arena_identity",
                      "Versions, stable ids and the next id.",
                      &tests_arena_identity },
                    { "tests_arena_clear_and_swap",
                      "clear resets everything; swap exchanges everything.",
                      &tests_arena_clear_and_swap },
                    { "tests_arena_equality",
                      "Arenas compare everything a caller can observe.",
                      &tests_arena_equality },
                    { "tests_arena_equivalence",
                      "equivalent(): the same forest, wherever it sits.",
                      &tests_arena_equivalence },
                }
            },
            dt::block_spec{
                "traits",
                "arena_traits.hpp.",
                {
                    { "tests_arena_traits_payload",
                      "A payload is destructible and movable or copyable.",
                      &tests_arena_traits_payload },
                    { "tests_arena_traits_link_policy",
                      "Link policies, their flags and their link counts.",
                      &tests_arena_traits_link_policy },
                    { "tests_arena_traits_node",
                      "Node detection and the node's two type extractors.",
                      &tests_arena_traits_node },
                    { "tests_arena_traits_arena",
                      "Arena detection, extraction, cross-referencing.",
                      &tests_arena_traits_arena },
                    { "tests_arena_traits_class",
                      "arena_class reports each preset's capabilities.",
                      &tests_arena_traits_class },
                }
            },
            dt::block_spec{
                "concepts",
                "arena_concepts.hpp.",
                {
                    { "tests_arena_concepts_payload_and_policy",
                      "The payload and link-policy concepts.",
                      &tests_arena_concepts_payload_and_policy },
                    { "tests_arena_concepts_node_and_arena",
                      "The node, arena and cross-arena concepts.",
                      &tests_arena_concepts_node_and_arena },
                }
            },
            dt::block_spec{
                "tree",
                "tree/arena_tree.hpp.",
                {
                    { "tests_arena_tree_root",
                      "Root tracking: create, query, set.",
                      &tests_arena_tree_root },
                    { "tests_arena_tree_add",
                      "add_child, add_child_first and add_sibling_after.",
                      &tests_arena_tree_add },
                    { "tests_arena_tree_remove",
                      "remove takes one node and orphans its children.",
                      &tests_arena_tree_remove },
                    { "tests_arena_tree_remove_subtree",
                      "remove_subtree frees the whole subtree, on any policy.",
                      &tests_arena_tree_remove_subtree },
                    { "tests_arena_tree_clear_swap_equality",
                      "clear, swap and equality include the root.",
                      &tests_arena_tree_clear_swap_equality },
                    { "tests_arena_tree_equivalence",
                      "equivalent() on a tree also compares the root's tree.",
                      &tests_arena_tree_equivalence },
                    { "tests_arena_tree_aliases",
                      "The four aliases name the four policies.",
                      &tests_arena_tree_aliases },
                }
            },
            dt::block_spec{
                "tree traits",
                "tree/arena_tree_traits.hpp and tree/arena_tree_concepts.hpp.",
                {
                    { "tests_arena_tree_traits_identity",
                      "An arena tree is an arena with root() and has_root().",
                      &tests_arena_tree_traits_identity },
                    { "tests_arena_tree_traits_methods",
                      "set_root and create_root are detected.",
                      &tests_arena_tree_traits_methods },
                    { "tests_arena_tree_traits_navigation",
                      "Navigation follows the policy, and a non-arena has none.",
                      &tests_arena_tree_traits_navigation },
                    { "tests_arena_tree_traits_class",
                      "arena_tree_class reports each alias's capabilities.",
                      &tests_arena_tree_traits_class },
                    { "tests_arena_tree_concepts",
                      "The arena tree concepts.",
                      &tests_arena_tree_concepts },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_ARENA_TESTS_
