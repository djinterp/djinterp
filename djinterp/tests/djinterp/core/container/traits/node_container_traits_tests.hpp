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
* path:      /tests/djinterp/core/container/traits/node_container_traits_tests.hpp
*                                              node_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
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
#include "../../../../../inc/djinterp/core/container/traits/node_container_traits.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   ASSERTION PLUMBING
// ===========================================================================

// nc_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = nc_check_failed(...)` on the failing path only.
inline bool
nc_check_failed(
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
inline int
nc_shape_predicate_count()
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
// node_container base actually expose.  These mirror the production types
// without depending on them.

// nct_node
//   struct: the payload node the fixtures below hand out.
struct nct_node
{
    int value;
};

// nct_tree
//   struct: a fully furnished tree-shaped container - a single root entry,
// the generic entry_point() it aliases, an ownership policy, and the release
// hatch.  The djinterp policy named here is visible only as a forward
// declaration, which is exactly the production situation.
struct nct_tree
{
    using node_type        = nct_node;
    using ownership_policy = unique_owning_policy;
    using entry_storage    = nct_node*;

    static constexpr bool entry_owns = true;

    const nct_node* root() const;
    bool            has_root() const;
    const nct_node* entry_point() const;
    bool            has_entry() const;
    nct_node*       release_entry();
};

// nct_list
//   struct: a doubly linked list-shaped container - head() and tail().
struct nct_list
{
    using node_type = nct_node;

    const nct_node* head() const;
    const nct_node* tail() const;
    const nct_node* entry_point() const;
};

// nct_slist
//   struct: a singly linked list-shaped container - head() but no tail().
struct nct_slist
{
    using node_type = nct_node;

    const nct_node* head() const;
};

// nct_graph
//   struct: a graph-shaped container - the entry SET rho and its cardinality.
struct nct_graph
{
    using node_type = nct_node;

    const nct_node* entries() const;
    std::size_t     entry_count() const;
};

// nct_generic
//   struct: the topology-neutral base silhouette - entry_point() only, no
// domain-specific alias.  This is the shape the real node_container presents.
struct nct_generic
{
    using node_type = nct_node;

    const nct_node* entry_point() const;
    bool            has_entry() const;
    nct_node*       release_entry();
};

// nct_bare
//   struct: no aliases, no methods.  The universal negative.
struct nct_bare
{
    int unrelated;
};


// ===========================================================================
// III. ENTRY MARKER ISOLATORS
// ===========================================================================
//   One entry signal each, so a regression names the detector at fault, plus
// the near-misses that must NOT promote a type to a shape.

// nct_node_only
//   struct: the node_type half of the identity requirement, alone.
struct nct_node_only
{
    using node_type = nct_node;
};

// nct_root_only
//   struct: the entry half, alone - no node_type, so not a node container.
struct nct_root_only
{
    const nct_node* root() const;
};

// nct_head_only_no_node
//   struct: as above, through head().
struct nct_head_only_no_node
{
    const nct_node* head() const;
};

// nct_entries_only_no_node
//   struct: as above, through entries().
struct nct_entries_only_no_node
{
    const nct_node* entries() const;
};

// nct_entry_point_only_no_node
//   struct: as above, through entry_point().
struct nct_entry_point_only_no_node
{
    const nct_node* entry_point() const;
};

// nct_tail_no_head
//   struct: tail() without head().  tail() is not itself an entry reference,
// so this is a node_type-only type: neither a node container nor list-shaped.
struct nct_tail_no_head
{
    using node_type = nct_node;

    const nct_node* tail() const;
};

// nct_rooted_tail_no_head
//   struct: a genuine node container - node_type plus root() - that also
// carries tail() but no head().  This is the only witness that
// is_doubly_linked_container is layered on is_list_shaped_container rather
// than on is_node_container: it satisfies identity and has a tail, so a
// version of the trait built on identity would call it doubly linked, while
// the real one correctly does not.  A tree with a back pointer is not a list.
struct nct_rooted_tail_no_head
{
    using node_type = nct_node;

    const nct_node* root() const;
    const nct_node* tail() const;
};

// nct_entry_count_no_entries
//   struct: entry_count() without entries().  The cardinality of rho is not
// an entry reference, so this must not read as graph-shaped.
struct nct_entry_count_no_entries
{
    using node_type = nct_node;

    std::size_t entry_count() const;
};

// nct_has_root_no_root
//   struct: has_root() without root().  The interrogative is not an entry
// reference either.
struct nct_has_root_no_root
{
    using node_type = nct_node;

    bool has_root() const;
};

// nct_has_entry_no_entry_point
//   struct: has_entry() without entry_point().
struct nct_has_entry_no_entry_point
{
    using node_type = nct_node;

    bool has_entry() const;
};

// nct_release_no_entry
//   struct: release_entry() without any entry reference.
struct nct_release_no_entry
{
    using node_type = nct_node;

    nct_node* release_entry();
};

// nct_void_node_type
//   struct: node_type is declared, and is void.  The alias EXISTS, so the
// type is a node container; the extracted node is void rather than the
// nonesuch that stands for "no alias".
struct nct_void_node_type
{
    using node_type = void;

    const nct_node* root() const;
};

// nct_const_node_type
//   struct: node_type is cv-qualified.  clean_t is applied to the CONTAINER
// before the alias is read, never to the alias's own result, so the
// qualification survives extraction.
struct nct_const_node_type
{
    using node_type = const nct_node;

    const nct_node* root() const;
};

// nct_nonesuch_node_type
//   struct: node_type is declared, and is nonesuch - the very type the
// extractor uses as its "absent" answer.  Only has_node_type can tell this
// apart from a type with no alias at all.
struct nct_nonesuch_node_type
{
    using node_type = nonesuch;

    const nct_node* root() const;
};


// ===========================================================================
// IV.  METHOD SHAPE PROBES
// ===========================================================================
//   The detectors ask whether `x.root()` compiles on a suitably qualified x.
// These fixtures pin what that admits and what it rejects - the answer is
// narrower than "declares a member named root" in some directions and wider in
// others.

// nct_void_root
//   struct: root() returning void.  The probe names the expression's type and
// never constrains it, so void counts as a detection.
struct nct_void_root
{
    using node_type = nct_node;

    void root() const;
};

// nct_private_root
//   struct: root() is private.  Access control participates in the probe, so
// this is not a detection.
struct nct_private_root
{
    using node_type = nct_node;

private:
    const nct_node* root() const;
};

// nct_arg_root
//   struct: root() requires an argument, so the nullary call is ill-formed.
struct nct_arg_root
{
    using node_type = nct_node;

    const nct_node* root(int _index) const;
};

// nct_default_arg_root
//   struct: root() has a defaulted parameter, so the nullary call succeeds.
struct nct_default_arg_root
{
    using node_type = nct_node;

    const nct_node* root(int _index = 0) const;
};

// nct_data_member_root
//   struct: root is a data member of pointer type.  `x.root()` cannot call a
// pointer to node, so this is not a detection.
struct nct_data_member_root
{
    using node_type = nct_node;

    const nct_node* root;
};

// nct_fnptr_root
//   struct: root is a data member of POINTER-TO-FUNCTION type.  `x.root()`
// calls through it, so this IS a detection - the probe tests callability, not
// member-function-ness.
struct nct_fnptr_root
{
    using node_type = nct_node;

    const nct_node* (*root)();
};

// nct_static_root
//   struct: root() is a static member.  It is callable through an object
// expression, so it is a detection.
struct nct_static_root
{
    using node_type = nct_node;

    static const nct_node* root();
};

// nct_root_base
//   struct: supplies root() to nct_inherited_root below.
struct nct_root_base
{
    const nct_node* root() const;
};

// nct_inherited_root
//   struct: root() arrives by inheritance.  Name lookup finds it, so it is a
// detection - the traits are structural, not declaration-site.
struct nct_inherited_root : nct_root_base
{
    using node_type = nct_node;
};

// nct_overloaded_root
//   struct: const and non-const root() overloads.  Overload resolution on a
// const object picks the const one.
struct nct_overloaded_root
{
    using node_type = nct_node;

    const nct_node* root() const;
    nct_node*       root();
};

// nct_template_root
//   struct: root() is a member template with a defaulted parameter, so the
// nullary call deduces and succeeds.
struct nct_template_root
{
    using node_type = nct_node;

    template<typename _Dummy = int>
    const nct_node* root() const;
};

// nct_nonconst_root
//   struct: root() is NOT const-qualified.  The section II probe forms a
// `const _Type&`, so the bare type reads dark while the reference spellings
// read lit - the divergence tests_node_container_cv_ref_divergence pins.
struct nct_nonconst_root
{
    using node_type = nct_node;

    nct_node* root();
};

// nct_const_release
//   struct: release_entry() IS const-qualified, unlike the production one.
// The section II mutable probe accepts it either way, so this fixture shows
// the mutable probe is a lower bound, not a requirement of non-constness.
struct nct_const_release
{
    using node_type = nct_node;

    const nct_node* entry_point() const;
    nct_node*       release_entry() const;
};


// ===========================================================================
// V.   OWNERSHIP FIXTURES
// ===========================================================================

// nct_policy_owning
//   struct: a COMPLETE policy that owns.  Complete is the operative word:
// is_owning_container reads the member, which an incomplete type cannot
// supply.
struct nct_policy_owning
{
    static constexpr bool owns = true;
};

// nct_policy_non_owning
//   struct: a complete policy that does not own.
struct nct_policy_non_owning
{
    static constexpr bool owns = false;
};

// nct_policy_int_owns
//   struct: `owns` is an int.  is_owning_container converts contextually to
// bool rather than demanding bool exactly, so a non-zero int owns.
struct nct_policy_int_owns
{
    static constexpr int owns = 2;
};

// nct_policy_no_owns
//   struct: a policy with no `owns` member at all.
struct nct_policy_no_owns
{
    int unrelated;
};

// nct_policy_nonstatic_owns
//   struct: `owns` is a non-static data member, so it is not nameable as a
// constant.
struct nct_policy_nonstatic_owns
{
    bool owns;
};

// nct_policy_incomplete
//   struct: declared and never defined - the local stand-in for djinterp's
// three forward-declared policies.
struct nct_policy_incomplete;

// nct_with_owning_policy
//   struct: names the complete owning policy.
struct nct_with_owning_policy
{
    using ownership_policy = nct_policy_owning;
};

// nct_with_non_owning_policy
//   struct: names the complete non-owning policy.
struct nct_with_non_owning_policy
{
    using ownership_policy = nct_policy_non_owning;
};

// nct_with_int_owns_policy
//   struct: names the int-valued policy.
struct nct_with_int_owns_policy
{
    using ownership_policy = nct_policy_int_owns;
};

// nct_with_no_owns_policy
//   struct: names the policy that has no owns member.
struct nct_with_no_owns_policy
{
    using ownership_policy = nct_policy_no_owns;
};

// nct_with_nonstatic_owns_policy
//   struct: names the policy whose owns is non-static.
struct nct_with_nonstatic_owns_policy
{
    using ownership_policy = nct_policy_nonstatic_owns;
};

// nct_with_incomplete_policy
//   struct: names the incomplete policy.  ownership_model_of can still
// classify it - by identity - while is_owning_container cannot read it.
struct nct_with_incomplete_policy
{
    using ownership_policy = nct_policy_incomplete;
};

// nct_unique_container
//   struct: a node container naming djinterp's unique policy.
struct nct_unique_container
{
    using node_type        = nct_node;
    using ownership_policy = unique_owning_policy;

    const nct_node* root() const;
};

// nct_shared_container
//   struct: a node container naming djinterp's shared policy - the sharing
// that admits a DAG or general graph.
struct nct_shared_container
{
    using node_type        = nct_node;
    using ownership_policy = shared_owning_policy;

    const nct_node* entries() const;
};

// nct_non_owning_container
//   struct: a node container naming djinterp's non-owning policy.
struct nct_non_owning_container
{
    using node_type        = nct_node;
    using ownership_policy = non_owning_policy;

    const nct_node* head() const;
};

// nct_unknown_policy_container
//   struct: a node container whose policy is none of the three djinterp
// names, so the model is unknown even though the policy is complete and owns.
struct nct_unknown_policy_container
{
    using node_type        = nct_node;
    using ownership_policy = nct_policy_owning;

    const nct_node* entry_point() const;
};

// nct_entry_storage_only
//   struct: the entry_storage alias in isolation.
struct nct_entry_storage_only
{
    using entry_storage = nct_node*;
};

// --- entry_owns spellings ---------------------------------------------------
//   has_entry_owns_constant demands `decltype(T::entry_owns)` be exactly
// `const bool`, which is a stricter test than "is convertible to bool".

// nct_entry_owns_constexpr
//   struct: `static constexpr bool` - constexpr implies const, so this is the
// spelling the production containers use and the one that is detected.
struct nct_entry_owns_constexpr
{
    static constexpr bool entry_owns = true;
};

// nct_entry_owns_const
//   struct: `static const bool` - the same declared type, also detected.
struct nct_entry_owns_const
{
    static const bool entry_owns = false;
};

// nct_entry_owns_plain
//   struct: `static bool` - not const, so not detected.
struct nct_entry_owns_plain
{
    static bool entry_owns;
};

// nct_entry_owns_int
//   struct: `static constexpr int` - const, but not bool, so not detected.
struct nct_entry_owns_int
{
    static constexpr int entry_owns = 1;
};

// nct_entry_owns_nonstatic
//   struct: a non-static data member.  decltype names its declared type,
// which is a bare bool, so it is not detected.
struct nct_entry_owns_nonstatic
{
    bool entry_owns;
};

// nct_entry_owns_method
//   struct: entry_owns is a function, not a constant.
struct nct_entry_owns_method
{
    static bool entry_owns();
};


// ===========================================================================
// VI.  SHAPE PRECEDENCE PROBES
// ===========================================================================
//   The shape predicates overlap by construction; node_container_shape_of
// resolves the overlap by a fixed order.  These fixtures occupy the
// overlapping cells so the order is pinned rather than assumed.

// nct_tree_and_list
//   struct: root() AND head().  Both predicates hold; tree wins.
struct nct_tree_and_list
{
    using node_type = nct_node;

    const nct_node* root() const;
    const nct_node* head() const;
};

// nct_list_and_graph
//   struct: head() AND entries().  Both predicates hold; list wins.
struct nct_list_and_graph
{
    using node_type = nct_node;

    const nct_node* head() const;
    const nct_node* entries() const;
};

// nct_tree_and_graph
//   struct: root() AND entries().  Both predicates hold; tree wins.
struct nct_tree_and_graph
{
    using node_type = nct_node;

    const nct_node* root() const;
    const nct_node* entries() const;
};

// nct_all_shapes
//   struct: every entry reference at once.  All four predicates hold; tree
// still wins.
struct nct_all_shapes
{
    using node_type = nct_node;

    const nct_node* root() const;
    const nct_node* head() const;
    const nct_node* tail() const;
    const nct_node* entries() const;
    const nct_node* entry_point() const;
};


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


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_NODE_CONTAINER_TRAITS_
