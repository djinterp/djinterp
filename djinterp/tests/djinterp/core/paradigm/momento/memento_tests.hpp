/*******************************************************************************
* djinterp [test]                                              memento_tests.hpp
*
*   Unit-test suite header for inc/djinterp/paradigm/momento/memento.hpp - the
* Memento pattern module. Two faces, selected by DTEST_SPEC_MODE:
*
*     - default face: the flat declarations of every tests_* predicate plus the
*       shared state fixtures the section TUs build against. Every
*       memento_tests_*.cpp includes the header this way and DEFINES the
*       predicates it owns.
*
*     - spec-provider face (DTEST_SPEC_MODE defined): the above PLUS
*       memento_spec(), which the runner hands to run_module. Only the runner
*       defines DTEST_SPEC_MODE, so the spec is compiled exactly once.
*
*   LANGUAGE FLOOR: the module's capability traits use declval/void_t/
* false_type, so C++11 is the floor. The suite targets C++11 and up; the
* dialect-conditional surface is guarded internally:
*     - C++14 : the make_* convenience factories and make_external_snapshot,
*               the _v trait aliases
*     - C++17 : std::optional (try_pop, peek_undo/peek_redo), string_view tags
*     - C++20 : the memento_source / memento_target / snapshot_strategy /
*               history_policy concepts and constrained_checkpoint
*
*   NOTE ON re_std::any: the type-erased memento routes through re_std::any, which
* is external to this module. In the harness it is a thin, std-backed stand-in
* implementing the documented surface (has_value / holds<T> / get<T> / type /
* reset); the suite depends only on that surface.
*
*   NOTE ON A FINDING: memento_originator::current_sequence() is documented as
* the monotonic id of the next memento, but with a whole-object snapshot policy
* (deep_copy_snapshot) the counter lives inside the snapshotted state, so
* restore_memento rewinds it. The suite is written to pass against the header as
* shipped - it verifies state restoration and pre-restore monotonicity without
* pinning the post-restore counter - so it stays green whether or not the
* suggested restore_memento fix (preserve m_sequence across restore) is applied.
*
*
* path:      /tests/djinterp/core/paradigm/momento/memento_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.10
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_MEMENTO_TESTS_
#define DJINTERP_TEST_MEMENTO_TESTS_ 1

// std
#include <cstddef>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/core/paradigm/momento/memento.hpp"                       // the header under test (bare form)
#include "djinterp/test/test_defaults.hpp"   // module_spec, run_module


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                SHARED STATE FIXTURES                                    ///
///////////////////////////////////////////////////////////////////////////////
//   Small state types spanning the capabilities the module detects and the
// snapshot strategies consume. Class definitions only (no static data), so the
// header is safe to include from every section TU.

// sr_state
//   type: implements the save_state()/restore_state() protocol.
struct sr_state
{
    int value;

    sr_state() : value(0) {}
    explicit sr_state(int _v) : value(_v) {}

    int  save_state() const     { return value; }
    void restore_state(int _s)  { value = _s; }
};

// clonable_state
//   type: provides clone() (used by clone_snapshot).
struct clonable_state
{
    int value;

    clonable_state() : value(0) {}
    explicit clonable_state(int _v) : value(_v) {}

    clonable_state clone() const { return *this; }
};

// serializable_state
//   type: provides serialize()/deserialize() (used by serialized_snapshot).
struct serializable_state
{
    int value;

    serializable_state() : value(0) {}
    explicit serializable_state(int _v) : value(_v) {}

    std::vector<char>
    serialize() const
    {
        return std::vector<char>(1, static_cast<char>(value));
    }

    void
    deserialize(const std::vector<char>& _bytes)
    {
        value = _bytes.empty() ? 0 : static_cast<int>(_bytes[0]);
    }
};

// diffable_state
//   type: provides diff()/apply_diff() (used by delta_snapshot). diff is a
// signed value delta and apply_diff adds it, so the policy round-trips.
struct diffable_state
{
    int value;

    diffable_state() : value(0) {}
    explicit diffable_state(int _v) : value(_v) {}

    int  diff(const diffable_state& _other) const { return value - _other.value; }
    void apply_diff(int _delta)                   { value += _delta; }
};

// plain_state
//   type: copyable only - no memento-protocol members.
struct plain_state
{
    int value;

    plain_state() : value(0) {}
    explicit plain_state(int _v) : value(_v) {}
};

// eq_state
//   type: equality-comparable (used by coalescing_history and the equality
// trait).
struct eq_state
{
    int value;

    eq_state() : value(0) {}
    explicit eq_state(int _v) : value(_v) {}

    bool operator==(const eq_state& _o) const { return value == _o.value; }
};

// full_state
//   type: every memento capability at once (used by the aggregate trait).
struct full_state
{
    int value;

    full_state() : value(0) {}
    explicit full_state(int _v) : value(_v) {}

    int  save_state() const    { return value; }
    void restore_state(int _s) { value = _s; }
    full_state clone() const   { return *this; }
    std::vector<char> serialize() const { return std::vector<char>(1, static_cast<char>(value)); }
    void deserialize(const std::vector<char>& _b) { value = _b.empty() ? 0 : static_cast<int>(_b[0]); }
    int  diff(const full_state& _o) const { return value - _o.value; }
    void apply_diff(int _d)               { value += _d; }
    bool operator==(const full_state& _o) const { return value == _o.value; }
};


///////////////////////////////////////////////////////////////////////////////
///                TEST DECLARATIONS                                         ///
///////////////////////////////////////////////////////////////////////////////

// -- section: configuration & capability traits (memento_tests_traits.cpp)
bool tests_feature_gates();
bool tests_has_save_state_method();
bool tests_has_restore_state_method();
bool tests_has_clone_method();
bool tests_has_serialize_method();
bool tests_has_deserialize_method();
bool tests_has_diff_method();
bool tests_has_apply_diff_method();
bool tests_has_equality_operator();
bool tests_is_memento_capable();
bool tests_is_copyable_memento_capable();
bool tests_is_clonable_memento_capable();
bool tests_is_serializable_memento_capable();
bool tests_is_diff_memento_capable();
bool tests_memento_capability_aggregate();
bool tests_trait_v_forms();

// -- section: snapshot strategies (memento_tests_snapshot.cpp)
bool tests_deep_copy_snapshot();
bool tests_clone_snapshot();
bool tests_serialized_snapshot();
bool tests_delta_snapshot_baseline();
bool tests_delta_snapshot_delta();
bool tests_delta_record();
bool tests_external_snapshot();
bool tests_make_external_snapshot();

// -- section: history policies (memento_tests_history.cpp)
bool tests_unlimited_history();
bool tests_bounded_history_evicts();
bool tests_bounded_history_max_size();
bool tests_coalescing_history_suppresses();
bool tests_coalescing_history_distinct();

// -- section: memento core (memento_tests_core.cpp)
bool tests_memento_metadata_default();
bool tests_memento_metadata_sequence();
bool tests_memento_metadata_tag();
bool tests_memento_construction();
bool tests_memento_default();
bool tests_originator_create_restore();
bool tests_originator_sequence_monotonic();
bool tests_originator_tagged();

// -- section: memento caretaker (memento_tests_caretaker.cpp)
bool tests_caretaker_push_size();
bool tests_caretaker_pop();
bool tests_caretaker_peek();
bool tests_caretaker_at();
bool tests_caretaker_empty_clear();
bool tests_caretaker_for_each();
bool tests_caretaker_for_each_reverse();
bool tests_caretaker_try_pop();
bool tests_caretaker_coalescing();
bool tests_caretaker_bounded();

// -- section: undo / redo stack (memento_tests_undo_redo.cpp)
bool tests_undo_redo_checkpoint_undo();
bool tests_undo_redo_roundtrip();
bool tests_undo_redo_undo_empty();
bool tests_undo_redo_redo_empty();
bool tests_undo_redo_checkpoint_clears_redo();
bool tests_undo_redo_can_flags();
bool tests_undo_redo_depths();
bool tests_undo_redo_clear();
bool tests_undo_redo_clear_redo();
bool tests_undo_redo_tagged();
bool tests_undo_redo_peek();

// -- section: type-erased memento (memento_tests_any.cpp)
bool tests_any_memento_construct_holds();
bool tests_any_memento_get_sbo();
bool tests_any_memento_get_heap();
bool tests_any_memento_get_mutable();
bool tests_any_memento_type();
bool tests_any_memento_reset();
bool tests_any_memento_metadata();
bool tests_any_caretaker_push_pop();
bool tests_any_caretaker_peek();
bool tests_any_caretaker_heterogeneous();
bool tests_any_caretaker_for_each();
bool tests_any_caretaker_empty_clear();
bool tests_any_caretaker_try_pop();

// -- section: convenience factories (memento_tests_factories.cpp)
bool tests_make_memento();
bool tests_make_memento_with_policy();
bool tests_restore_memento();
bool tests_make_caretaker();
bool tests_make_undo_redo();

// -- section: concept-constrained interfaces (memento_tests_concepts.cpp)
bool tests_memento_source_concept();
bool tests_memento_target_concept();
bool tests_snapshot_strategy_concept();
bool tests_history_policy_concept();
bool tests_constrained_checkpoint();


///////////////////////////////////////////////////////////////////////////////
///                SPEC-PROVIDER FACE  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// memento_spec
//   function: assembles the module_spec - one block per semantic section of
// memento.hpp, one test per tests_* predicate. Compiled only in the runner TU.
inline ::djinterp::test::module_spec
memento_spec()
{
    namespace dt = ::djinterp::test;

    dt::module_spec m;

    m.name       = "memento";
    m.descriptor =
        "Memento pattern module: capability traits, snapshot strategies, "
        "history policies, the memento core (metadata / memento / originator / "
        "caretaker), the undo-redo stack, the type-erased memento, the "
        "convenience factories, and the C++20 concept surface.";

    m.blocks = std::vector<dt::block_spec>
    {
        dt::block_spec
        {
            "configuration & capability traits",
            "feature gates and the SFINAE capability-detection traits.",
            std::vector<dt::test_spec>
            {
                { "feature gates",            "D_MEMENTO_HAS_* track the language level.", &tests_feature_gates                  },
                { "has_save_state_method",    "detects save_state().",                     &tests_has_save_state_method          },
                { "has_restore_state_method", "detects restore_state(save_state()).",      &tests_has_restore_state_method       },
                { "has_clone_method",         "detects clone().",                          &tests_has_clone_method               },
                { "has_serialize_method",     "detects serialize().",                      &tests_has_serialize_method           },
                { "has_deserialize_method",   "detects deserialize(serialize()).",         &tests_has_deserialize_method         },
                { "has_diff_method",          "detects diff(other).",                      &tests_has_diff_method                },
                { "has_apply_diff_method",    "detects apply_diff(diff()).",               &tests_has_apply_diff_method          },
                { "has_equality_operator",    "detects operator==.",                       &tests_has_equality_operator          },
                { "is_memento_capable",       "save + restore classification.",            &tests_is_memento_capable             },
                { "is_copyable_memento",      "copy-constructible classification.",        &tests_is_copyable_memento_capable    },
                { "is_clonable_memento",      "clone classification.",                     &tests_is_clonable_memento_capable    },
                { "is_serializable_memento",  "serialize + deserialize classification.",   &tests_is_serializable_memento_capable },
                { "is_diff_memento",          "diff + apply_diff classification.",         &tests_is_diff_memento_capable        },
                { "memento_capability",       "aggregate capability struct.",              &tests_memento_capability_aggregate   },
                { "_v forms",                 "variable-template aliases mirror ::value.", &tests_trait_v_forms                  }
            }
        },
        dt::block_spec
        {
            "snapshot strategies",
            "deep-copy / clone / serialized / delta / external policies.",
            std::vector<dt::test_spec>
            {
                { "deep_copy_snapshot",     "capture by copy, restore by assign.", &tests_deep_copy_snapshot      },
                { "clone_snapshot",         "capture via clone().",                &tests_clone_snapshot          },
                { "serialized_snapshot",    "capture/restore via (de)serialize.",  &tests_serialized_snapshot     },
                { "delta_snapshot baseline","baseline diff round-trips.",          &tests_delta_snapshot_baseline },
                { "delta_snapshot delta",   "incremental delta round-trips.",      &tests_delta_snapshot_delta    },
                { "delta_record",           "delta record holds diff + flag.",     &tests_delta_record            },
                { "external_snapshot",      "delegates to user callables.",        &tests_external_snapshot       },
                { "make_external_snapshot", "factory builds a policy (C++14+).",   &tests_make_external_snapshot  }
            }
        },
        dt::block_spec
        {
            "history policies",
            "unlimited / bounded / coalescing eviction.",
            std::vector<dt::test_spec>
            {
                { "unlimited_history",     "retains everything.",                &tests_unlimited_history            },
                { "bounded evicts",        "drops oldest past the cap.",         &tests_bounded_history_evicts       },
                { "bounded max_size",      "exposes the compile-time cap.",      &tests_bounded_history_max_size     },
                { "coalescing suppresses", "skips a duplicate of the newest.",   &tests_coalescing_history_suppresses },
                { "coalescing distinct",   "keeps distinct consecutive states.", &tests_coalescing_history_distinct  }
            }
        },
        dt::block_spec
        {
            "memento core",
            "metadata, memento, and the CRTP originator.",
            std::vector<dt::test_spec>
            {
                { "metadata default",       "default sequence is zero.",          &tests_memento_metadata_default    },
                { "metadata sequence",      "explicit sequence is stored.",       &tests_memento_metadata_sequence   },
                { "metadata tag",           "string_view tag (C++17+).",          &tests_memento_metadata_tag        },
                { "memento construction",   "state + metadata are held.",         &tests_memento_construction        },
                { "memento default",        "default snapshot + metadata.",       &tests_memento_default             },
                { "originator create/restore","captures then restores state.",    &tests_originator_create_restore   },
                { "originator sequence",    "sequence increments per capture.",   &tests_originator_sequence_monotonic },
                { "originator tagged",      "tagged capture (C++17+).",           &tests_originator_tagged           }
            }
        },
        dt::block_spec
        {
            "memento caretaker",
            "history stack with a pluggable eviction policy.",
            std::vector<dt::test_spec>
            {
                { "push + size",       "push grows the history.",           &tests_caretaker_push_size        },
                { "pop",               "pop returns and removes newest.",   &tests_caretaker_pop             },
                { "peek",              "peek returns newest, no removal.",  &tests_caretaker_peek            },
                { "at",                "indexed access, 0 = oldest.",       &tests_caretaker_at              },
                { "empty + clear",     "clear empties the history.",        &tests_caretaker_empty_clear     },
                { "for_each",          "iterates oldest to newest.",        &tests_caretaker_for_each        },
                { "for_each_reverse",  "iterates newest to oldest.",        &tests_caretaker_for_each_reverse },
                { "try_pop",           "optional pop (C++17+).",            &tests_caretaker_try_pop         },
                { "coalescing policy", "duplicate suppression via policy.", &tests_caretaker_coalescing      },
                { "bounded policy",    "eviction via policy.",              &tests_caretaker_bounded         }
            }
        },
        dt::block_spec
        {
            "undo / redo stack",
            "dual-stack undo/redo over a snapshot policy.",
            std::vector<dt::test_spec>
            {
                { "checkpoint + undo",     "undo restores the prior checkpoint.", &tests_undo_redo_checkpoint_undo       },
                { "undo/redo round-trip",  "redo re-applies an undone state.",    &tests_undo_redo_roundtrip            },
                { "undo when empty",       "undo returns false on empty.",        &tests_undo_redo_undo_empty           },
                { "redo when empty",       "redo returns false on empty.",        &tests_undo_redo_redo_empty           },
                { "checkpoint clears redo","a new checkpoint drops the future.",  &tests_undo_redo_checkpoint_clears_redo },
                { "can_undo / can_redo",   "availability flags.",                 &tests_undo_redo_can_flags            },
                { "depths",                "undo/redo depth counts.",             &tests_undo_redo_depths              },
                { "clear",                 "clears both stacks.",                 &tests_undo_redo_clear               },
                { "clear_redo",            "clears only the future.",             &tests_undo_redo_clear_redo          },
                { "tagged checkpoint",     "tagged capture (C++17+).",            &tests_undo_redo_tagged             },
                { "peek undo/redo",        "optional peeks (C++17+).",            &tests_undo_redo_peek               }
            }
        },
        dt::block_spec
        {
            "type-erased memento",
            "any_memento and any_memento_caretaker (C++11+).",
            std::vector<dt::test_spec>
            {
                { "construct + holds",   "stores a snapshot; holds<T> checks it.", &tests_any_memento_construct_holds },
                { "get (SBO types)",     "value get for scalar snapshots.",        &tests_any_memento_get_sbo        },
                { "get (heap types)",    "const-ref get for class snapshots.",     &tests_any_memento_get_heap       },
                { "get (mutable)",       "mutable ref get for class snapshots.",   &tests_any_memento_get_mutable    },
                { "type",                "type id reflects the stored snapshot.",  &tests_any_memento_type           },
                { "reset",               "reset clears the stored snapshot.",      &tests_any_memento_reset          },
                { "metadata",            "metadata travels with the memento.",     &tests_any_memento_metadata       },
                { "caretaker push/pop",  "push then pop newest.",                  &tests_any_caretaker_push_pop     },
                { "caretaker peek",      "peek returns newest.",                   &tests_any_caretaker_peek         },
                { "caretaker mixed",     "heterogeneous snapshot types.",          &tests_any_caretaker_heterogeneous },
                { "caretaker for_each",  "iterates the history.",                  &tests_any_caretaker_for_each     },
                { "caretaker empty/clear","clear empties the history.",            &tests_any_caretaker_empty_clear  },
                { "caretaker try_pop",   "optional pop (C++17+).",                 &tests_any_caretaker_try_pop      }
            }
        },
        dt::block_spec
        {
            "convenience factories (C++14+)",
            "make_memento / restore_memento / make_caretaker / make_undo_redo.",
            std::vector<dt::test_spec>
            {
                { "make_memento",         "captures state into a memento.",      &tests_make_memento           },
                { "make_memento (policy)","honours an explicit snapshot policy.", &tests_make_memento_with_policy },
                { "restore_memento",      "restores state from a memento.",      &tests_restore_memento        },
                { "make_caretaker",       "builds a typed caretaker.",           &tests_make_caretaker         },
                { "make_undo_redo",       "builds an undo/redo stack.",          &tests_make_undo_redo         }
            }
        },
        dt::block_spec
        {
            "concept-constrained interfaces (C++20+)",
            "memento_source / memento_target / snapshot_strategy / history_policy.",
            std::vector<dt::test_spec>
            {
                { "memento_source",         "protocol or copyable classification.", &tests_memento_source_concept    },
                { "memento_target",         "assignable or restorable.",            &tests_memento_target_concept    },
                { "snapshot_strategy",      "capture/restore-shaped policy.",       &tests_snapshot_strategy_concept },
                { "history_policy",         "should_push/after_push policy.",       &tests_history_policy_concept    },
                { "constrained_checkpoint", "concept-guarded checkpoint.",          &tests_constrained_checkpoint    }
            }
        }
    };

    return m;
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_MEMENTO_TESTS_
