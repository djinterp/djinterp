/*******************************************************************************
* djinterp [testing]                             node_container_traits_tests.hpp
*
* Unit tests for node_container_traits.hpp:
*   Declarations, fixtures and assertion plumbing for the node container trait
* suite.  Every test function is declared here and defined in the section .cpp
* that matches the section of the header under test.
*
*   THE SUITE IS DELIBERATELY STANDALONE.  It includes node_container_traits.hpp
* and nothing else from djinterp, exactly as that header advertises itself.  In
* particular node_container.hpp is NOT included, so djinterp's three ownership
* policies are visible here only through the forward declarations the header
* under test provides.  That is a load-bearing property, not an oversight:
* is_owning_container reads `ownership_policy::owns`, which needs a COMPLETE
* policy, while ownership_model_of only compares type identity, which does not.
* The two traits therefore answer differently depending on whether
* node_container.hpp was included - see tests_is_owning_container_completeness.
* Every section TU must agree on this, or the same trait would be instantiated
* two ways in one program.
*
*   FIXTURE NAMING.  Fixtures are prefixed nct_ and are free of commas, so they
* can be passed through the cv/ref macros below without parenthesising.
*
*
* path:      /tests/djinterp/cpp/container/traits/node_container_traits_tests.hpp
*                                              node_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Assertion plumbing
      ------------------

II.   Shape fixtures          (the realistic node container silhouettes)
      ------------------------------------------------------------------

III.  Entry marker isolators  (one entry signal each, and the near-misses)
      --------------------------------------------------------------------

IV.   Method shape probes     (what counts as "has a method")
      -------------------------------------------------------

V.    Ownership fixtures      (policies, entry_owns spellings)
      --------------------------------------------------------

VI.   Shape precedence probes (types that answer to several shapes at once)
      ---------------------------------------------------------------------

VII.  Test declarations
      -----------------
*/

#ifndef DJINTERP_TESTS_NODE_CONTAINER_TRAITS_
#define DJINTERP_TESTS_NODE_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <type_traits>
// djinterp
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/container/traits/node_container_traits.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


// ===========================================================================
// I.   ASSERTION PLUMBING
// ===========================================================================

// nc_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = nc_check_failed(...)` on the failing path only.
inline
bool nc_check_failed(
    const char* _expr,
    const char* _file,
    int         _line
)
{
    std::printf("      ! %s:%d\n        %s\n", _file, _line, _expr);

    return false;
}

// D_NC_CHECK
//   macro: run-time assertion.  Variadic so a template argument list's commas
// survive the preprocessor.  The enclosing test function must declare a local
// `bool ok = true;` - this macro clears it on failure and leaves it untouched
// otherwise, so one function reports every failure it finds, not just the
// first.
#define D_NC_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            ok = ::djinterp::testing::nc_check_failed(#__VA_ARGS__,           \
                                                      __FILE__,               \
                                                      __LINE__);              \
        }                                                                     \
    } while (0)

// D_NC_STATIC_CHECK
//   macro: compile-time assertion, for properties a run-time read cannot
// witness - constant-expression usability and type identity.
#define D_NC_STATIC_CHECK(...)                                                \
    static_assert((__VA_ARGS__), #__VA_ARGS__)

//   THE THREE CV/REF CONTRACTS
//     The header under test does not answer cv- and reference-qualified types
// uniformly, because its sections ask their questions differently:
//
//     sections I and III  write clean_t<_Type>, which strips everything, so
//                         all seven spellings agree            -> AGREE
//     section II (most)   writes `const _Type&`, so the two volatile
//                         spellings go dark                    -> CONST_PROBE
//     section II (release) writes `_Type&`, so every const or volatile
//                         spelling goes dark                   -> MUTABLE_PROBE
//
//     One macro per contract, so the divergence is visible in the test source
// rather than buried in a comment.  None of the three may be used on void or a
// function type, since `void&` cannot be formed.

// D_NC_CV_REF_AGREE
//   macro: the clean_t contract.  All seven cv- and reference-qualified
// spellings answer EXPECT.
#define D_NC_CV_REF_AGREE(TRAIT, TYPE, EXPECT)                                \
    do                                                                        \
    {                                                                         \
        D_NC_CHECK(TRAIT<TYPE>::value                == (EXPECT));            \
        D_NC_CHECK(TRAIT<const TYPE>::value          == (EXPECT));            \
        D_NC_CHECK(TRAIT<volatile TYPE>::value       == (EXPECT));            \
        D_NC_CHECK(TRAIT<const volatile TYPE>::value == (EXPECT));            \
        D_NC_CHECK(TRAIT<TYPE&>::value               == (EXPECT));            \
        D_NC_CHECK(TRAIT<const TYPE&>::value         == (EXPECT));            \
        D_NC_CHECK(TRAIT<TYPE&&>::value              == (EXPECT));            \
    } while (0)

// D_NC_CONST_PROBE_AGREE
//   macro: the `const _Type&` contract, for a CONST-QUALIFIED accessor.  The
// five non-volatile spellings answer EXPECT; both volatile spellings are false,
// because no fixture declares a `const volatile` member.
#define D_NC_CONST_PROBE_AGREE(TRAIT, TYPE, EXPECT)                           \
    do                                                                        \
    {                                                                         \
        D_NC_CHECK(TRAIT<TYPE>::value                == (EXPECT));            \
        D_NC_CHECK(TRAIT<const TYPE>::value          == (EXPECT));            \
        D_NC_CHECK(TRAIT<TYPE&>::value               == (EXPECT));            \
        D_NC_CHECK(TRAIT<const TYPE&>::value         == (EXPECT));            \
        D_NC_CHECK(TRAIT<TYPE&&>::value              == (EXPECT));            \
        D_NC_CHECK(TRAIT<volatile TYPE>::value       == false);               \
        D_NC_CHECK(TRAIT<const volatile TYPE>::value == false);               \
    } while (0)

// D_NC_MUTABLE_PROBE_AGREE
//   macro: the `_Type&` contract, for a NON-const accessor.  Only the three
// spellings that yield a modifiable lvalue answer EXPECT; every const or
// volatile spelling is false.
#define D_NC_MUTABLE_PROBE_AGREE(TRAIT, TYPE, EXPECT)                         \
    do                                                                        \
    {                                                                         \
        D_NC_CHECK(TRAIT<TYPE>::value                == (EXPECT));            \
        D_NC_CHECK(TRAIT<TYPE&>::value               == (EXPECT));            \
        D_NC_CHECK(TRAIT<TYPE&&>::value              == (EXPECT));            \
        D_NC_CHECK(TRAIT<const TYPE>::value          == false);               \
        D_NC_CHECK(TRAIT<volatile TYPE>::value       == false);               \
        D_NC_CHECK(TRAIT<const volatile TYPE>::value == false);               \
        D_NC_CHECK(TRAIT<const TYPE&>::value         == false);               \
    } while (0)

// nc_shape_predicate_count
//   function: how many of the four shape predicates hold for _Type.  The
// predicates are NOT a partition - a type may answer to several at once - so
// this counts rather than asserts uniqueness, and the classification tests pin
// which count each fixture is expected to produce.
template<typename _Type>
inline
int nc_shape_predicate_count()
{
    return ( ( is_tree_shaped_container<_Type>::value    ? 1 : 0 )
           + ( is_list_shaped_container<_Type>::value    ? 1 : 0 )
           + ( is_doubly_linked_container<_Type>::value  ? 1 : 0 )
           + ( is_graph_shaped_container<_Type>::value   ? 1 : 0 ) );
}


// ===========================================================================
// II.  SHAPE FIXTURES
// ===========================================================================
//   The realistic silhouettes: what a tree, a list, a graph and the bare
// DNodeContainer base actually expose.  These mirror the production types
// without depending on them.

// DNctNode
//   struct: the payload node the fixtures below hand out.
struct DNctNode
{
    int value;
};

// DNctTree
//   struct: a fully furnished tree-shaped container - a single root entry,
// the generic entry_point() it aliases, an ownership policy, and the release
// hatch.  The djinterp policy named here is visible only as a forward
// declaration, which is exactly the production situation.
struct DNctTree
{
    using node_type        = DNctNode;
    using ownership_policy = unique_owning_policy;
    using entry_storage    = DNctNode*;

    static constexpr bool entry_owns = true;

    const DNctNode* root() const;
    bool has_root() const;
    const DNctNode* entry_point() const;
    bool has_entry() const;
    DNctNode* release_entry();
};

// DNctList
//   struct: a doubly linked list-shaped container - head() and tail().
struct DNctList
{
    using node_type = DNctNode;

    const DNctNode* head() const;
    const DNctNode* tail() const;
    const DNctNode* entry_point() const;
};

// DNctSlist
//   struct: a singly linked list-shaped container - head() but no tail().
struct DNctSlist
{
    using node_type = DNctNode;

    const DNctNode* head() const;
};

// DNctGraph
//   struct: a graph-shaped container - the entry SET rho and its cardinality.
struct DNctGraph
{
    using node_type = DNctNode;

    const DNctNode* entries() const;
    std::size_t entry_count() const;
};

// DNctGeneric
//   struct: the topology-neutral base silhouette - entry_point() only, no
// domain-specific alias.  This is the shape the real DNodeContainer presents.
struct DNctGeneric
{
    using node_type = DNctNode;

    const DNctNode* entry_point() const;
    bool has_entry() const;
    DNctNode* release_entry();
};

// DNctBare
//   struct: no aliases, no methods.  The universal negative.
struct DNctBare
{
    int unrelated;
};


// ===========================================================================
// III. ENTRY MARKER ISOLATORS
// ===========================================================================
//   One entry signal each, so a regression names the detector at fault, plus
// the near-misses that must NOT promote a type to a shape.

// DNctNodeOnly
//   struct: the node_type half of the identity requirement, alone.
struct DNctNodeOnly
{
    using node_type = DNctNode;
};

// DNctRootOnly
//   struct: the entry half, alone - no node_type, so not a node container.
struct DNctRootOnly
{
    const DNctNode* root() const;
};

// DNctHeadOnlyNoNode
//   struct: as above, through head().
struct DNctHeadOnlyNoNode
{
    const DNctNode* head() const;
};

// DNctEntriesOnlyNoNode
//   struct: as above, through entries().
struct DNctEntriesOnlyNoNode
{
    const DNctNode* entries() const;
};

// DNctEntryPointOnlyNoNode
//   struct: as above, through entry_point().
struct DNctEntryPointOnlyNoNode
{
    const DNctNode* entry_point() const;
};

// DNctTailNoHead
//   struct: tail() without head().  tail() is not itself an entry reference,
// so this is a node_type-only type: neither a node container nor list-shaped.
struct DNctTailNoHead
{
    using node_type = DNctNode;

    const DNctNode* tail() const;
};

// DNctRootedTailNoHead
//   struct: a genuine node container - node_type plus root() - that also
// carries tail() but no head().  This is the only witness that
// is_doubly_linked_container is layered on is_list_shaped_container rather
// than on is_node_container: it satisfies identity and has a tail, so a
// version of the trait built on identity would call it doubly linked, while
// the real one correctly does not.  A tree with a back pointer is not a list.
struct DNctRootedTailNoHead
{
    using node_type = DNctNode;

    const DNctNode* root() const;
    const DNctNode* tail() const;
};

// DNctEntryCountNoEntries
//   struct: entry_count() without entries().  The cardinality of rho is not
// an entry reference, so this must not read as graph-shaped.
struct DNctEntryCountNoEntries
{
    using node_type = DNctNode;

    std::size_t entry_count() const;
};

// nct_has_root_no_root
//   struct: has_root() without root().  The interrogative is not an entry
// reference either.
struct nct_has_root_no_root
{
    using node_type = DNctNode;

    bool has_root() const;
};

// nct_has_entry_no_entry_point
//   struct: has_entry() without entry_point().
struct nct_has_entry_no_entry_point
{
    using node_type = DNctNode;

    bool has_entry() const;
};

// DNctReleaseNoEntry
//   struct: release_entry() without any entry reference.
struct DNctReleaseNoEntry
{
    using node_type = DNctNode;

    DNctNode* release_entry();
};

// nct_void_node_type
//   struct: node_type is declared, and is void.  The alias EXISTS, so the
// type is a node container; the extracted node is void rather than the
// nonesuch that stands for "no alias".
struct nct_void_node_type
{
    using node_type = void;

    const DNctNode* root() const;
};

// nct_const_node_type
//   struct: node_type is cv-qualified.  clean_t is applied to the CONTAINER
// before the alias is read, never to the alias's own result, so the
// qualification survives extraction.
struct nct_const_node_type
{
    using node_type = const DNctNode;

    const DNctNode* root() const;
};

// nct_nonesuch_node_type
//   struct: node_type is declared, and is nonesuch - the very type the
// extractor uses as its "absent" answer.  Only has_node_type can tell this
// apart from a type with no alias at all.
struct nct_nonesuch_node_type
{
    using node_type = nonesuch;

    const DNctNode* root() const;
};


// ===========================================================================
// IV.  METHOD SHAPE PROBES
// ===========================================================================
//   The detectors ask whether `x.root()` compiles on a suitably qualified x.
// These fixtures pin what that admits and what it rejects - the answer is
// narrower than "declares a member named root" in some directions and wider in
// others.

// DNctVoidRoot
//   struct: root() returning void.  The probe names the expression's type and
// never constrains it, so void counts as a detection.
struct DNctVoidRoot
{
    using node_type = DNctNode;

    void root() const;
};

// DNctPrivateRoot
//   struct: root() is private.  Access control participates in the probe, so
// this is not a detection.
struct DNctPrivateRoot
{
    using node_type = DNctNode;

private:
    const DNctNode* root() const;
};

// DNctArgRoot
//   struct: root() requires an argument, so the nullary call is ill-formed.
struct DNctArgRoot
{
    using node_type = DNctNode;

    const DNctNode* root(
        int _index
    ) const;
};

// DNctDefaultArgRoot
//   struct: root() has a defaulted parameter, so the nullary call succeeds.
struct DNctDefaultArgRoot
{
    using node_type = DNctNode;

    const DNctNode* root(
        int _index = 0
    ) const;
};

// DNctDataMemberRoot
//   struct: root is a data member of pointer type.  `x.root()` cannot call a
// pointer to node, so this is not a detection.
struct DNctDataMemberRoot
{
    using node_type = DNctNode;

    const DNctNode* root;
};

// DNctFnptrRoot
//   struct: root is a data member of POINTER-TO-FUNCTION type.  `x.root()`
// calls through it, so this IS a detection - the probe tests callability, not
// member-function-ness.
struct DNctFnptrRoot
{
    using node_type = DNctNode;

    const DNctNode* (*root)();
};

// DNctStaticRoot
//   struct: root() is a static member.  It is callable through an object
// expression, so it is a detection.
struct DNctStaticRoot
{
    using node_type = DNctNode;

    static
    const DNctNode* root();
};

// DNctRootBase
//   struct: supplies root() to DNctInheritedRoot below.
struct DNctRootBase
{
    const DNctNode* root() const;
};

// DNctInheritedRoot
//   struct: root() arrives by inheritance.  Name lookup finds it, so it is a
// detection - the traits are structural, not declaration-site.
struct DNctInheritedRoot : DNctRootBase
{
    using node_type = DNctNode;
};

// DNctOverloadedRoot
//   struct: const and non-const root() overloads.  Overload resolution on a
// const object picks the const one.
struct DNctOverloadedRoot
{
    using node_type = DNctNode;

    const DNctNode* root() const;
    DNctNode* root();
};

// DNctTemplateRoot
//   struct: root() is a member template with a defaulted parameter, so the
// nullary call deduces and succeeds.
struct DNctTemplateRoot
{
    using node_type = DNctNode;

    template<typename _Dummy = int>
    const DNctNode* root() const;
};

// DNctNonconstRoot
//   struct: root() is NOT const-qualified.  The section II probe forms a
// `const _Type&`, so the bare type reads dark while the reference spellings
// read lit - the divergence tests_node_container_cv_ref_divergence pins.
struct DNctNonconstRoot
{
    using node_type = DNctNode;

    DNctNode* root();
};

// DNctConstRelease
//   struct: release_entry() IS const-qualified, unlike the production one.
// The section II mutable probe accepts it either way, so this fixture shows
// the mutable probe is a lower bound, not a requirement of non-constness.
struct DNctConstRelease
{
    using node_type = DNctNode;

    const DNctNode* entry_point() const;
    DNctNode* release_entry() const;
};


// ===========================================================================
// V.   OWNERSHIP FIXTURES
// ===========================================================================

// DNctPolicyOwning
//   struct: a COMPLETE policy that owns.  Complete is the operative word:
// is_owning_container reads the member, which an incomplete type cannot
// supply.
struct DNctPolicyOwning
{
    static constexpr bool owns = true;
};

// DNctPolicyNonOwning
//   struct: a complete policy that does not own.
struct DNctPolicyNonOwning
{
    static constexpr bool owns = false;
};

// DNctPolicyIntOwns
//   struct: `owns` is an int.  is_owning_container converts contextually to
// bool rather than demanding bool exactly, so a non-zero int owns.
struct DNctPolicyIntOwns
{
    static constexpr int owns = 2;
};

// DNctPolicyNoOwns
//   struct: a policy with no `owns` member at all.
struct DNctPolicyNoOwns
{
    int unrelated;
};

// DNctPolicyNonstaticOwns
//   struct: `owns` is a non-static data member, so it is not nameable as a
// constant.
struct DNctPolicyNonstaticOwns
{
    bool owns;
};

// nct_policy_incomplete
//   struct: declared and never defined - the local stand-in for djinterp's
// three forward-declared policies.
struct nct_policy_incomplete;

// DNctWithOwningPolicy
//   struct: names the complete owning policy.
struct DNctWithOwningPolicy
{
    using ownership_policy = DNctPolicyOwning;
};

// DNctWithNonOwningPolicy
//   struct: names the complete non-owning policy.
struct DNctWithNonOwningPolicy
{
    using ownership_policy = DNctPolicyNonOwning;
};

// DNctWithIntOwnsPolicy
//   struct: names the int-valued policy.
struct DNctWithIntOwnsPolicy
{
    using ownership_policy = DNctPolicyIntOwns;
};

// DNctWithNoOwnsPolicy
//   struct: names the policy that has no owns member.
struct DNctWithNoOwnsPolicy
{
    using ownership_policy = DNctPolicyNoOwns;
};

// DNctWithNonstaticOwnsPolicy
//   struct: names the policy whose owns is non-static.
struct DNctWithNonstaticOwnsPolicy
{
    using ownership_policy = DNctPolicyNonstaticOwns;
};

// DNctWithIncompletePolicy
//   struct: names the incomplete policy.  ownership_model_of can still
// classify it - by identity - while is_owning_container cannot read it.
struct DNctWithIncompletePolicy
{
    using ownership_policy = nct_policy_incomplete;
};

// DNctUniqueContainer
//   struct: a node container naming djinterp's unique policy.
struct DNctUniqueContainer
{
    using node_type        = DNctNode;
    using ownership_policy = unique_owning_policy;

    const DNctNode* root() const;
};

// DNctSharedContainer
//   struct: a node container naming djinterp's shared policy - the sharing
// that admits a DAG or general graph.
struct DNctSharedContainer
{
    using node_type        = DNctNode;
    using ownership_policy = shared_owning_policy;

    const DNctNode* entries() const;
};

// DNctNonOwningContainer
//   struct: a node container naming djinterp's non-owning policy.
struct DNctNonOwningContainer
{
    using node_type        = DNctNode;
    using ownership_policy = non_owning_policy;

    const DNctNode* head() const;
};

// DNctUnknownPolicyContainer
//   struct: a node container whose policy is none of the three djinterp
// names, so the model is unknown even though the policy is complete and owns.
struct DNctUnknownPolicyContainer
{
    using node_type        = DNctNode;
    using ownership_policy = DNctPolicyOwning;

    const DNctNode* entry_point() const;
};

// DNctEntryStorageOnly
//   struct: the entry_storage alias in isolation.
struct DNctEntryStorageOnly
{
    using entry_storage = DNctNode*;
};

// --- entry_owns spellings ---------------------------------------------------
//   has_entry_owns_constant demands `decltype(T::entry_owns)` be exactly
// `const bool`, which is a stricter test than "is convertible to bool".

// DNctEntryOwnsConstexpr
//   struct: `static constexpr bool` - constexpr implies const, so this is the
// spelling the production containers use and the one that is detected.
struct DNctEntryOwnsConstexpr
{
    static constexpr bool entry_owns = true;
};

// DNctEntryOwnsConst
//   struct: `static const bool` - the same declared type, also detected.
struct DNctEntryOwnsConst
{
    static const bool entry_owns = false;
};

// DNctEntryOwnsPlain
//   struct: `static bool` - not const, so not detected.
struct DNctEntryOwnsPlain
{
    static bool entry_owns;
};

// DNctEntryOwnsInt
//   struct: `static constexpr int` - const, but not bool, so not detected.
struct DNctEntryOwnsInt
{
    static constexpr int entry_owns = 1;
};

// DNctEntryOwnsNonstatic
//   struct: a non-static data member.  decltype names its declared type,
// which is a bare bool, so it is not detected.
struct DNctEntryOwnsNonstatic
{
    bool entry_owns;
};

// DNctEntryOwnsMethod
//   struct: entry_owns is a function, not a constant.
struct DNctEntryOwnsMethod
{
    static
    bool entry_owns();
};


// ===========================================================================
// VI.  SHAPE PRECEDENCE PROBES
// ===========================================================================
//   The shape predicates overlap by construction; node_container_shape_of
// resolves the overlap by a fixed order.  These fixtures occupy the
// overlapping cells so the order is pinned rather than assumed.

// DNctTreeAndList
//   struct: root() AND head().  Both predicates hold; tree wins.
struct DNctTreeAndList
{
    using node_type = DNctNode;

    const DNctNode* root() const;
    const DNctNode* head() const;
};

// DNctListAndGraph
//   struct: head() AND entries().  Both predicates hold; list wins.
struct DNctListAndGraph
{
    using node_type = DNctNode;

    const DNctNode* head() const;
    const DNctNode* entries() const;
};

// DNctTreeAndGraph
//   struct: root() AND entries().  Both predicates hold; tree wins.
struct DNctTreeAndGraph
{
    using node_type = DNctNode;

    const DNctNode* root() const;
    const DNctNode* entries() const;
};

// DNctAllShapes
//   struct: every entry reference at once.  All four predicates hold; tree
// still wins.
struct DNctAllShapes
{
    using node_type = DNctNode;

    const DNctNode* root() const;
    const DNctNode* head() const;
    const DNctNode* tail() const;
    const DNctNode* entries() const;
    const DNctNode* entry_point() const;
};
#endif  // !DTEST_SPEC_MODE  (fixtures)



// ===========================================================================
// VII. TEST DECLARATIONS
// ===========================================================================

// I.   Ownership policy detection     (..._tests_ownership_detect.cpp)
bool tests_has_ownership_policy();
bool tests_has_entry_storage_type();
bool tests_has_entry_owns_constant();
bool tests_ownership_detect_cv_ref();
bool tests_ownership_detect_companions();

// II.  Entry point detection          (..._tests_entry_point.cpp)
bool tests_has_entry_point_method();
bool tests_has_root_method();
bool tests_has_root_member_method();
bool tests_has_head_method();
bool tests_has_tail_method();
bool tests_has_entries_method();
bool tests_has_entry_count_method();
bool tests_has_has_entry_method();
bool tests_has_release_entry_method();
bool tests_entry_point_method_shapes();
bool tests_has_any_entry_point();
bool tests_entry_point_probe_qualification();
bool tests_entry_point_companions();

// III. Node type extraction           (..._tests_node_type.cpp)
bool tests_has_node_type();
bool tests_node_type_of_t();
bool tests_node_type_of_t_fallback();
bool tests_node_type_cv_ref();

// IV.  Ownership model classification (..._tests_ownership_model.cpp)
bool tests_ownership_model_enum();
bool tests_ownership_model_of();
bool tests_ownership_model_absent_policy();
bool tests_is_owning_container();
bool tests_is_owning_container_completeness();
bool tests_is_unique_owning_container();
bool tests_is_shared_owning_container();
bool tests_ownership_model_orthogonality();
bool tests_ownership_model_cv_ref();
bool tests_ownership_model_companions();

// V.   Node container identity        (..._tests_identity.cpp)
bool tests_is_node_container();
bool tests_is_tree_shaped_container();
bool tests_is_list_shaped_container();
bool tests_is_doubly_linked_container();
bool tests_is_graph_shaped_container();
bool tests_identity_composition_shape();
bool tests_identity_overlap();
bool tests_node_container_cv_ref_divergence();
bool tests_identity_companions();

// VI.  Combined classification        (..._tests_classification.cpp)
bool tests_node_container_shape_enum();
bool tests_node_container_shape_of();
bool tests_node_container_shape_precedence();
bool tests_node_container_shape_companions();
bool tests_node_container_class_identity();
bool tests_node_container_class_ownership();
bool tests_node_container_class_entry();
bool tests_node_container_class_node();
bool tests_node_container_class_agreement();
bool tests_node_container_class_edge_types();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// node_container_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
node_container_traits_spec()
{
    return dt::module_spec{
        "node_container_traits",
        "compile-time SFINAE-based structural detection for node-based "
        "containers",
        {
            dt::block_spec{
                "ownership_detect",
                "Ownership policy detection",
                {
                    { "tests_has_ownership_policy",
                      "Verifies the probe for a nested ownership_policy alias",
                      &tests_has_ownership_policy },
                    { "tests_has_entry_storage_type",
                      "Verifies the probe for a nested entry_storage alias - "
                      "the type in which a container actually holds its entry "
                      "reference",
                      &tests_has_entry_storage_type },
                    { "tests_has_entry_owns_constant",
                      "Verifies the probe for a static entry_owns constant",
                      &tests_has_entry_owns_constant },
                    { "tests_ownership_detect_cv_ref",
                      "Verifies that all three section I probes honour the "
                      "clean_t contract: each writes clean_t<_Type> before "
                      "asking its question, so cv-qualification and "
                      "reference-ness are stripped and every spelling of a "
                      "type answers alike",
                      &tests_ownership_detect_cv_ref },
                    { "tests_ownership_detect_companions",
                      "Verifies the `_v` companions of the section I probes",
                      &tests_ownership_detect_companions },
                }
            },
            dt::block_spec{
                "entry_point",
                "Entry point detection",
                {
                    { "tests_has_entry_point_method",
                      "Verifies the probe for the topology-neutral "
                      "entry_point() accessor",
                      &tests_has_entry_point_method },
                    { "tests_has_root_method",
                      "Verifies the probe for root() - the single "
                      "distinguished entry that marks a tree-shaped container",
                      &tests_has_root_method },
                    { "tests_has_root_member_method",
                      "Verifies the probe for has_root() - the interrogative "
                      "that reports whether a tree's root is populated",
                      &tests_has_root_member_method },
                    { "tests_has_head_method",
                      "Verifies the probe for head() - the entry that marks a "
                      "list-shaped container",
                      &tests_has_head_method },
                    { "tests_has_tail_method",
                      "Verifies the probe for tail() - the second end of a "
                      "doubly linked list",
                      &tests_has_tail_method },
                    { "tests_has_entries_method",
                      "Verifies the probe for entries() - the entry SET rho "
                      "that marks a graph- shaped container, where a node may "
                      "be reached by several references",
                      &tests_has_entries_method },
                    { "tests_has_entry_count_method",
                      "Verifies the probe for entry_count() - the cardinality "
                      "of the entry set",
                      &tests_has_entry_count_method },
                    { "tests_has_has_entry_method",
                      "Verifies the probe for has_entry() - the interrogative "
                      "for the generic entry point",
                      &tests_has_has_entry_method },
                    { "tests_has_release_entry_method",
                      "Verifies the probe for release_entry() - the hatch "
                      "that relinquishes ownership of the entry",
                      &tests_has_release_entry_method },
                    { "tests_entry_point_method_shapes",
                      "Verifies WHAT the method probes admit, using root() as "
                      "the representative",
                      &tests_entry_point_method_shapes },
                    { "tests_has_any_entry_point",
                      "Verifies the disjunction that decides whether a type "
                      "holds a root at all",
                      &tests_has_any_entry_point },
                    { "tests_entry_point_probe_qualification",
                      "Verifies the qualification contract of section II, "
                      "which differs from the clean_t contract the rest of "
                      "the header keeps",
                      &tests_entry_point_probe_qualification },
                    { "tests_entry_point_companions",
                      "Verifies the `_v` companions of the section II probes",
                      &tests_entry_point_companions },
                }
            },
            dt::block_spec{
                "node_type",
                "Node type extraction",
                {
                    { "tests_has_node_type",
                      "Verifies the probe for a nested node_type alias - one "
                      "half of the identity requirement",
                      &tests_has_node_type },
                    { "tests_node_type_of_t",
                      "Verifies the extractor on types that HAVE the alias",
                      &tests_node_type_of_t },
                    { "tests_node_type_of_t_fallback",
                      "Verifies the extractor on types that do NOT have the "
                      "alias, where the internal helper's primary template "
                      "supplies nonesuch",
                      &tests_node_type_of_t_fallback },
                    { "tests_node_type_cv_ref",
                      "Verifies that the section III probe honours the "
                      "clean_t contract",
                      &tests_node_type_cv_ref },
                }
            },
            dt::block_spec{
                "ownership_model",
                "Ownership model classification",
                {
                    { "tests_ownership_model_enum",
                      "Verifies the ownership enumeration itself",
                      &tests_ownership_model_enum },
                    { "tests_ownership_model_of",
                      "Verifies the classification of a named policy by type "
                      "identity",
                      &tests_ownership_model_of },
                    { "tests_ownership_model_absent_policy",
                      "Verifies what the classifier answers for a type that "
                      "names NO policy - the primary template's branch",
                      &tests_ownership_model_absent_policy },
                    { "tests_is_owning_container",
                      "Verifies the predicate that reads the policy's own "
                      "`owns` member",
                      &tests_is_owning_container },
                    { "tests_is_owning_container_completeness",
                      "Verifies that is_owning_container requires a COMPLETE "
                      "policy, and pins the consequence for this suite",
                      &tests_is_owning_container_completeness },
                    { "tests_is_unique_owning_container",
                      "Verifies the predicate for the exclusive-ownership "
                      "model",
                      &tests_is_unique_owning_container },
                    { "tests_is_shared_owning_container",
                      "Verifies the predicate for the reference-counted model "
                      "- the sharing that, in the monograph's terms, admits a "
                      "node reached by several references and so a DAG or "
                      "general graph rather than a tree",
                      &tests_is_shared_owning_container },
                    { "tests_ownership_model_orthogonality",
                      "Verifies that the identity question and the ownership "
                      "question are genuinely independent, by exhibiting a "
                      "fixture for each combination that can exist",
                      &tests_ownership_model_orthogonality },
                    { "tests_ownership_model_cv_ref",
                      "Verifies that section IV honours the clean_t contract "
                      "throughout - both the identity classifier and the "
                      "member read strip qualification before asking",
                      &tests_ownership_model_cv_ref },
                    { "tests_ownership_model_companions",
                      "Verifies the `_v` companions of the section IV traits",
                      &tests_ownership_model_companions },
                }
            },
            dt::block_spec{
                "identity",
                "Node container identity",
                {
                    { "tests_is_node_container",
                      "Verifies the structural minimum",
                      &tests_is_node_container },
                    { "tests_is_tree_shaped_container",
                      "Verifies the tree predicate - a node container with a "
                      "single root entry",
                      &tests_is_tree_shaped_container },
                    { "tests_is_list_shaped_container",
                      "Verifies the list predicate - a node container with a "
                      "head entry",
                      &tests_is_list_shaped_container },
                    { "tests_is_doubly_linked_container",
                      "Verifies the doubly linked predicate - list-shaped AND "
                      "carrying tail()",
                      &tests_is_doubly_linked_container },
                    { "tests_is_graph_shaped_container",
                      "Verifies the graph predicate - a node container "
                      "exposing the entry SET rho",
                      &tests_is_graph_shaped_container },
                    { "tests_identity_composition_shape",
                      "Verifies the SHAPE of the composition, not merely its "
                      "per-type answers: each predicate is its base conjoined "
                      "with exactly one marker",
                      &tests_identity_composition_shape },
                    { "tests_identity_overlap",
                      "Verifies that the shape predicates OVERLAP rather than "
                      "partition",
                      &tests_identity_overlap },
                    { "tests_node_container_cv_ref_divergence",
                      "Verifies - and pins - the fact that this header does "
                      "NOT answer cv- and reference-qualified types "
                      "uniformly, because its sections phrase their questions "
                      "differently",
                      &tests_node_container_cv_ref_divergence },
                    { "tests_identity_companions",
                      "Verifies the `_v` companions of the section V traits",
                      &tests_identity_companions },
                }
            },
            dt::block_spec{
                "classification",
                "Combined classification",
                {
                    { "tests_node_container_shape_enum",
                      "Verifies the shape enumeration itself",
                      &tests_node_container_shape_enum },
                    { "tests_node_container_shape_of",
                      "Verifies the shape each unambiguous fixture resolves to",
                      &tests_node_container_shape_of },
                    { "tests_node_container_shape_precedence",
                      "Verifies the ORDER in which the overlapping predicates "
                      "are consulted",
                      &tests_node_container_shape_precedence },
                    { "tests_node_container_shape_companions",
                      "Verifies the `_v` companion of node_container_shape_of",
                      &tests_node_container_shape_companions },
                    { "tests_node_container_class_identity",
                      "Verifies the identity and shape members of the snapshot",
                      &tests_node_container_class_identity },
                    { "tests_node_container_class_ownership",
                      "Verifies the ownership members of the snapshot",
                      &tests_node_container_class_ownership },
                    { "tests_node_container_class_entry",
                      "Verifies the entry point members of the snapshot",
                      &tests_node_container_class_entry },
                    { "tests_node_container_class_node",
                      "Verifies the node type members of the snapshot",
                      &tests_node_container_class_node },
                    { "tests_node_container_class_agreement",
                      "Verifies that the snapshot is a faithful mirror: every "
                      "member equals the standalone trait it republishes, "
                      "checked across carriers that occupy different cells of "
                      "the classification",
                      &tests_node_container_class_agreement },
                    { "tests_node_container_class_edge_types",
                      "Verifies that the snapshot instantiates and answers "
                      "sensibly for types that are not containers at all - "
                      "the case an aggregate is most likely to break on, "
                      "since it touches every trait in the header at once",
                      &tests_node_container_class_edge_types },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_NODE_CONTAINER_TRAITS_
