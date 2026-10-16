/******************************************************************************
* djinterp [test]                       concurrency_strategy_traits_tests.hpp
*
*   DTest suite for /core/container/sync/concurrency_strategy_traits.hpp - the
* compile-time classification header that places a container on the monograph's
* concurrency axis (strategy, progress/arity/iteration/reclamation signature,
* and the concurrent / synchronized / sequential aggregate).
*
*   The suite is two-faced through DTEST_SPEC_MODE (see the DTest authoring
* guide, section 3).  In the normal face it is a plain header pulled into each
* section .cpp: it carries the fixtures and the D_CS_CHECK macro and includes
* the header under test.  In the spec face (defined by the runner) it drops the
* fixtures, pulls in test_defaults.hpp, and exposes the provider
* concurrency_strategy_traits_spec().
*
*   Because the classification is entirely compile-time, every check is a
* constexpr comparison of a trait value against its expected value.  Each check
* is therefore a de-facto static assertion; it is routed through D_CS_CHECK
* so the runner reports a pass/fail row per test rather than failing the build.
* The C++98 fallback arms of the header are gated out under the C++20 build the
* runner uses; this suite exercises the C++11-and-up path (the shipped path).
*
* -----------------------------------------------------------------------------
* BUILD PREREQUISITE
* -----------------------------------------------------------------------------
*   The header under test consumes three symbols the module set here does NOT
* itself ship; the suite compiles once these are present in the tree (all three
* are ordinary framework headers, merely outside this component):
*
*     - trait_detect.hpp must define D_TYPE_TRAIT_DETECTED.  The header under
*       test uses it at nine sites; earlier trait_detect.hpp left it undefined.
*       The updated trait_detect.hpp defines it as an alias of
*       D_TYPE_TRAIT_TRUE - that definition is the change that lets this header
*       compile at all.  (Reached transitively via
*       threadsafe_container_traits.hpp.)
*     - container_traits.hpp must provide is_immutable_container / _v (used by
*       is_vacuously_concurrent).
*     - threadsafe_container_traits.hpp must provide has_lock_policy_type / _v,
*       thread_safety_level, container_thread_safety_level / _v, and
*       supports_concurrent_reads / _v.
*
*   No edit is made to the module under test. The vacuous-concurrency tests are
* written as forwarding invariants (is_vacuously_concurrent_v<T> ==
* is_immutable_container_v<T>) so they hold regardless of how the real
* is_immutable_container decides immutability; the true branch is additionally
* exercised by the immutable_probe fixture, opting in through the marker the
* reference container_traits recognises.
*
* path:      /tests/djinterp/core/container/sync/
*                                   concurrency_strategy_traits_tests.hpp
* author(s): djinterp test generator
******************************************************************************/

#ifndef DJINTERP_TESTS_CONCURRENCY_STRATEGY_TRAITS_TESTS_
#define DJINTERP_TESTS_CONCURRENCY_STRATEGY_TRAITS_TESTS_ 1

// std
#include <cstdio>
#include <cstddef>
#include <atomic>

// -- (part 1) mode-gated includes ------------------------------------------
// djinterp core is ALWAYS first and unconditional (NS_*, D_* qualifiers,
// language gates), so both faces have it.
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
// normal face: the header under test, plus the real /core/sync/ primitives the
// real_primitives block classifies (full-from-inc; the sync slug is reached
// through the inc/ root, not an extra INCLUDES entry).
#include "concurrency_strategy_traits.hpp"
#include <djinterp/core/sync/atomic.hpp>
#include <djinterp/core/sync/cow.hpp>
#include <djinterp/core/sync/rcu.hpp>
#include <djinterp/core/sync/hazard_pointer.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"     // module_spec + run_module
#endif


// ensure the ::djinterp::test namespace name exists for the unconditional dt
// alias below, even in the normal face where test_defaults.hpp is not pulled
// in (re-opening an existing namespace is a no-op in the spec face).
namespace djinterp { namespace test {} }


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test). Declared UNCONDITIONALLY,
// because the spec provider (spec mode) needs dt::module_spec too.
namespace dt = ::djinterp::test;


// D_CS_CHECK
//   check: evaluate the (possibly comma-bearing) expression once; on failure
// print it with its location and early-return false.  Variadic so a top-level
// comma in a trait expression passes through whole.  Unique two-letter suffix
// (CS) so co-compiled suites never collide. Left OUTSIDE the fixture guard: it
// depends on nothing from the header under test.
#define D_CS_CHECK(...)                                                   \
    do                                                                    \
    {                                                                     \
        if (!(__VA_ARGS__))                                               \
        {                                                                 \
            std::printf("    check failed: %s\n    at: %s:%d\n",          \
                        #__VA_ARGS__, __FILE__, __LINE__);                \
            return false;                                                 \
        }                                                                 \
    }                                                                     \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures - normal mode only

// -- tag fixtures : one per strategy tag (drive the tag fast-path) ----------
struct tag_none   { using concurrency_strategy_tag = none_strategy_tag;   };
struct tag_locked { using concurrency_strategy_tag = locked_strategy_tag; };
struct tag_cow    { using concurrency_strategy_tag = cow_strategy_tag;    };
struct tag_rcu    { using concurrency_strategy_tag = rcu_strategy_tag;    };
struct tag_atomic { using concurrency_strategy_tag = atomic_strategy_tag; };
struct tag_hazard { using concurrency_strategy_tag = hazard_strategy_tag; };
struct tag_hybrid { using concurrency_strategy_tag = hybrid_strategy_tag; };

// -- stand-in nested types the structural detectors look for ----------------
struct fixture_lock_policy {};
struct fixture_cow_body    {};
struct fixture_rcu_body    {};
struct fixture_hazard_body {};

// -- structural LOCKED fixtures --------------------------------------------
// locked-by-structure = has_lock_policy_type && (read_lock() || write_lock()).
// read_lock() is probed on a const ref, so it must be const-qualified.
struct st_locked_rw
{
    using lock_policy_type = fixture_lock_policy;
    void read_lock() const;
    void write_lock();
};

struct st_locked_read_only          // policy + read_lock() only (OR, left arm)
{
    using lock_policy_type = fixture_lock_policy;
    void read_lock() const;
};

struct st_locked_write_only         // policy + write_lock() only (right arm)
{
    using lock_policy_type = fixture_lock_policy;
    void write_lock();
};

struct st_policy_no_method          // policy but NO method -> NOT locked (AND)
{
    using lock_policy_type = fixture_lock_policy;
};

struct st_method_no_policy          // read_lock() but NO policy -> NOT locked
{
    void read_lock() const;
};

// -- structural COW fixtures : cow_state alias OR snapshot() ----------------
struct st_cow_state    { using cow_state_type = fixture_cow_body; };
struct st_cow_snapshot { void snapshot() const;                   };

// -- structural RCU fixtures : rcu_protected alias OR epoch counter ---------
struct st_rcu_protected { using rcu_protected_type = fixture_rcu_body; };
struct st_rcu_epoch     { using epoch_counter_type = int;             };

// -- structural ATOMIC fixtures : value_type is atomic, OR load(index) ------
struct st_atomic_value  { using value_type = std::atomic<int>; };
struct st_atomic_loadat { int load(std::size_t) const;         };
struct st_value_plain   { using value_type = int; };  // value_type, NOT atomic

// -- structural HAZARD fixture : hazard_domain alias ------------------------
struct st_hazard { using hazard_domain_type = fixture_hazard_body; };

// -- the empty container : no tag, no structure -> strategy none ------------
struct st_plain {};

// -- immutability probe : opts in through the marker the reference
//    container_traits recognises (drives is_vacuously_concurrent's true arm).
struct immutable_probe { using immutable_container_marker = void; };

// -- tag-over-structure precedence fixtures ---------------------------------
// carries the cow tag AND full locked structure: the tag must win (cow, and
// specifically NOT locked and NOT hybrid).
struct pr_cowtag_lockstruct
{
    using concurrency_strategy_tag = cow_strategy_tag;
    using lock_policy_type         = fixture_lock_policy;
    void read_lock() const;
    void write_lock();
};

// carries the locked tag AND cow structure: the tag must win (locked).
struct pr_locktag_cowstruct
{
    using concurrency_strategy_tag = locked_strategy_tag;
    using cow_state_type           = fixture_cow_body;
};

// carries an explicit none tag AND cow structure: the tag suppresses the
// structural read (strategy none; NOT cow).
struct pr_nonetag_cowstruct
{
    using concurrency_strategy_tag = none_strategy_tag;
    using cow_state_type           = fixture_cow_body;
};

// -- structural HYBRID fixtures : two or more strategies, no tag ------------
struct hy_locked_cow                // locked + cow -> hybrid
{
    using lock_policy_type = fixture_lock_policy;
    void read_lock() const;
    using cow_state_type   = fixture_cow_body;
};

struct hy_atomic_hazard             // atomic + hazard -> hybrid
{
    using value_type         = std::atomic<int>;
    using hazard_domain_type = fixture_hazard_body;
};

struct hy_triple                    // locked + cow + rcu -> hybrid
{
    using lock_policy_type = fixture_lock_policy;
    void write_lock();
    using cow_state_type      = fixture_cow_body;
    using rcu_protected_type  = fixture_rcu_body;
};

// -- real /core/sync/ primitives under test (integration) -------------------
using real_cow_t    = cow_state<int>;            // cow_strategy_tag
using real_rcu_t    = rcu_protected<int>;        // rcu_strategy_tag
using real_asize_t  = atomic_size;               // atomic_strategy_tag
using real_aver_t   = atomic_version;            // atomic_strategy_tag
using real_haz_t    = hazard_domain;             // hazard_strategy_tag
using real_mhaz_t   = multi_hazard_domain<2>;    // hazard_strategy_tag

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------
// One flat list, grouped by section; each banner names the .cpp that defines
// the group.

// I/II.  tags            (concurrency_strategy_traits_tests_tags.cpp)
bool tests_tags_enum_values();
bool tests_tags_has_strategy_tag();

// III.   structural      (concurrency_strategy_traits_tests_structural.cpp)
bool tests_structural_locked_members();
bool tests_structural_cow_members();
bool tests_structural_rcu_members();
bool tests_structural_atomic_members();
bool tests_structural_hazard_member();

// IV.    predicates      (concurrency_strategy_traits_tests_predicates.cpp)
bool tests_predicates_locked();
bool tests_predicates_cow();
bool tests_predicates_rcu();
bool tests_predicates_atomic();
bool tests_predicates_hazard();
bool tests_predicates_tag_precedence();

// IV.b   aggregate       (concurrency_strategy_traits_tests_aggregate.cpp)
bool tests_aggregate_synchronized();
bool tests_aggregate_vacuous();
bool tests_aggregate_concurrent();
bool tests_aggregate_sequential();

// V.     deduction       (concurrency_strategy_traits_tests_deduction.cpp)
bool tests_deduction_tagged();
bool tests_deduction_structural();
bool tests_deduction_hybrid();
bool tests_deduction_tag_over_structure();
bool tests_deduction_cvref();

// VI.    signature       (concurrency_strategy_traits_tests_signature.cpp)
bool tests_signature_none_locked();
bool tests_signature_cow_rcu();
bool tests_signature_atomic_hazard();
bool tests_signature_hybrid();
bool tests_signature_projections();
bool tests_signature_shorthands();

// VII. classification (concurrency_strategy_traits_tests_classification.cpp)
bool tests_classification_strategy_fields();
bool tests_classification_aggregate_fields();
bool tests_classification_signature_fields();
bool tests_classification_level_forwarding();

// integration  (concurrency_strategy_traits_tests_real_primitives.cpp)
bool tests_real_cow_state();
bool tests_real_rcu_protected();
bool tests_real_atomic_types();
bool tests_real_hazard_domains();
bool tests_real_tag_over_structure();


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
concurrency_strategy_traits_spec()
{
    return dt::module_spec{
        "concurrency_strategy_traits",
        "compile-time concurrency classification: strategy, monograph "
        "signature, and the concurrent / synchronized / sequential aggregate",
        {
            dt::block_spec{
                "tags",
                "the strategy enum values and the concurrency_strategy_tag "
                "alias detector",
                {
                    { "tests_tags_enum_values",
                      "concurrency_strategy enumerators carry their "
                      "documented integer values (none=0 .. hybrid=6)",
                      &tests_tags_enum_values },
                    { "tests_tags_has_strategy_tag",
                      "has_concurrency_strategy_tag_v is true for tagged "
                      "types and false for tag-less ones",
                      &tests_tags_has_strategy_tag },
                }
            },
            dt::block_spec{
                "structural",
                "the section-III structural member detectors, each on a type "
                "that has the member and one that does not",
                {
                    { "tests_structural_locked_members",
                      "has_read_lock_method_v / has_write_lock_method_v track "
                      "the const read_lock() / write_lock() members",
                      &tests_structural_locked_members },
                    { "tests_structural_cow_members",
                      "has_cow_state_type_v / has_snapshot_method_v track the "
                      "cow_state_type alias and snapshot() method",
                      &tests_structural_cow_members },
                    { "tests_structural_rcu_members",
                      "has_rcu_protected_type_v / has_epoch_type_v track the "
                      "rcu_protected_type and epoch_counter_type aliases",
                      &tests_structural_rcu_members },
                    { "tests_structural_atomic_members",
                      "the value_type-is-atomic probe and "
                      "has_atomic_load_at_v track an atomic value_type and a "
                      "load(size_t) member",
                      &tests_structural_atomic_members },
                    { "tests_structural_hazard_member",
                      "has_hazard_domain_type_v tracks the hazard_domain_type "
                      "alias",
                      &tests_structural_hazard_member },
                }
            },
            dt::block_spec{
                "predicates",
                "the five per-strategy predicates: each detection path (tag "
                "and structural) and the tag-wins precedence",
                {
                    { "tests_predicates_locked",
                      "is_locked_container_v (and the is_mutex_container_v "
                      "synonym): tag, both structural arms, and the AND",
                      &tests_predicates_locked },
                    { "tests_predicates_cow",
                      "is_cow_container_v: tag path and both structural arms "
                      "(cow_state alias, snapshot method)",
                      &tests_predicates_cow },
                    { "tests_predicates_rcu",
                      "is_rcu_container_v: tag path and both structural arms "
                      "(rcu_protected alias, epoch counter)",
                      &tests_predicates_rcu },
                    { "tests_predicates_atomic",
                      "is_atomic_container_v: tag path and both structural "
                      "arms (atomic value_type, indexed load)",
                      &tests_predicates_atomic },
                    { "tests_predicates_hazard",
                      "is_hazard_container_v: tag path and the hazard_domain "
                      "structural arm",
                      &tests_predicates_hazard },
                    { "tests_predicates_tag_precedence",
                      "a declared tag overrides incidental structure, and an "
                      "explicit none tag suppresses structural detection",
                      &tests_predicates_tag_precedence },
                }
            },
            dt::block_spec{
                "aggregate",
                "the vacuous / concurrent / synchronized / sequential axis, "
                "tested as the header's logical invariants",
                {
                    { "tests_aggregate_synchronized",
                      "is_synchronized_container_v equals the OR of the five "
                      "strategy predicates",
                      &tests_aggregate_synchronized },
                    { "tests_aggregate_vacuous",
                      "is_vacuously_concurrent_v forwards "
                      "is_immutable_container and lights up for the immutable "
                      "probe",
                      &tests_aggregate_vacuous },
                    { "tests_aggregate_concurrent",
                      "is_concurrent_container_v equals synchronized OR "
                      "vacuously-concurrent",
                      &tests_aggregate_concurrent },
                    { "tests_aggregate_sequential",
                      "is_sequential_container_v is the exact complement of "
                      "is_concurrent_container_v",
                      &tests_aggregate_sequential },
                }
            },
            dt::block_spec{
                "deduction",
                "concurrency_strategy_of over the tagged, structural, hybrid, "
                "precedence, and cv/ref inputs",
                {
                    { "tests_deduction_tagged",
                      "each of the seven tags deduces its strategy (exercises "
                      "every tag_to_strategy specialization)",
                      &tests_deduction_tagged },
                    { "tests_deduction_structural",
                      "each single-strategy structure, and the empty type, "
                      "deduces the matching strategy or none",
                      &tests_deduction_structural },
                    { "tests_deduction_hybrid",
                      "two or more structural strategies deduce hybrid",
                      &tests_deduction_hybrid },
                    { "tests_deduction_tag_over_structure",
                      "a tag on a multi-strategy structure deduces by the "
                      "tag, never hybrid",
                      &tests_deduction_tag_over_structure },
                    { "tests_deduction_cvref",
                      "deduction strips const / volatile / reference before "
                      "classifying",
                      &tests_deduction_cvref },
                }
            },
            dt::block_spec{
                "signature",
                "the strategy -> signature table, its projections, and the "
                "progress-grade shorthands",
                {
                    { "tests_signature_none_locked",
                      "the none and locked rows of the signature table",
                      &tests_signature_none_locked },
                    { "tests_signature_cow_rcu",
                      "the cow and rcu rows of the signature table",
                      &tests_signature_cow_rcu },
                    { "tests_signature_atomic_hazard",
                      "the atomic and hazard rows of the signature table",
                      &tests_signature_atomic_hazard },
                    { "tests_signature_hybrid",
                      "the hybrid row (the conservative meet)",
                      &tests_signature_hybrid },
                    { "tests_signature_projections",
                      "read/write_progress_of, arity_of, iteration_of and "
                      "reclamation_of agree with the signature fields",
                      &tests_signature_projections },
                    { "tests_signature_shorthands",
                      "is_lock_free_container, is_wait_free_reader_container "
                      "and offers_snapshot_iteration for each strategy",
                      &tests_signature_shorthands },
                }
            },
            dt::block_spec{
                "classification",
                "the container_concurrency_class aggregate forwards every "
                "member from its component traits",
                {
                    { "tests_classification_strategy_fields",
                      "is_locked / is_cow / is_rcu / is_atomic / is_hazard / "
                      "strategy match the standalone predicates",
                      &tests_classification_strategy_fields },
                    { "tests_classification_aggregate_fields",
                      "is_concurrent / is_synchronized / "
                      "is_vacuously_concurrent / is_sequential match the "
                      "standalone traits",
                      &tests_classification_aggregate_fields },
                    { "tests_classification_signature_fields",
                      "signature and its five projections match "
                      "concurrency_signature_of",
                      &tests_classification_signature_fields },
                    { "tests_classification_level_forwarding",
                      "level and concurrent_reads forward the threadsafe "
                      "container traits",
                      &tests_classification_level_forwarding },
                }
            },
            dt::block_spec{
                "real_primitives",
                "the updated /core/sync/ primitives classify through their "
                "tags and are never misread by incidental structure",
                {
                    { "tests_real_cow_state",
                      "cow_state<int> is cow (not locked, not hybrid) with "
                      "the cow signature",
                      &tests_real_cow_state },
                    { "tests_real_rcu_protected",
                      "rcu_protected<int> is rcu (not locked) with the rcu "
                      "signature",
                      &tests_real_rcu_protected },
                    { "tests_real_atomic_types",
                      "atomic_size and atomic_version are atomic with the "
                      "atomic signature",
                      &tests_real_atomic_types },
                    { "tests_real_hazard_domains",
                      "hazard_domain and multi_hazard_domain are hazard with "
                      "the hazard signature",
                      &tests_real_hazard_domains },
                    { "tests_real_tag_over_structure",
                      "cow_state's lock_policy_type and rcu_protected's "
                      "read_lock() do not make either one locked",
                      &tests_real_tag_over_structure },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_CONCURRENCY_STRATEGY_TRAITS_TESTS_
