/*******************************************************************************
* djinterp [test]                                             observer_tests.hpp
*
*   Unit-test suite header for inc/djinterp/paradigm/pattern/observer.hpp - the
* three-tier observer pattern (delegate / event / observer) plus its connection
* handles and compile-time capability traits. Two faces, selected by
* DTEST_SPEC_MODE:
*
*     - default face: the flat declarations of every tests_* predicate. Each
*       observer_tests_*.cpp includes the header this way and DEFINES the
*       predicates it owns. Section-local fixtures (free functions and counters
*       for the fn_ptr tiers) live in anonymous namespaces in the owning TU, so
*       the header carries no shared mutable state.
*
*     - spec-provider face (DTEST_SPEC_MODE defined): the above PLUS
*       observer_spec(), which the runner hands to run_module. Only the runner
*       defines DTEST_SPEC_MODE, so the spec is compiled exactly once.
*
*   LANGUAGE FLOOR: observer.hpp uses if constexpr, std::void_t, std::conjunction,
* inline constexpr variable templates, and [[nodiscard]] unconditionally, so
* C++17 is the floor. The suite targets C++17 and up; there is no dialect-
* conditional surface to guard.
*
*   FINDINGS (see the accompanying report): (A) observer::compact uses
* std::remove_if without including <algorithm> - a latent portability issue that
* compiles only because libstdc++ pulls the header in transitively. (B)
* destroying an observer does not invalidate outstanding connection handles -
* connected() still reports true afterwards, contrary to the header comment,
* though the handle stays memory-safe. The suite is written to pass against the
* header as shipped: it verifies the safe, observable behaviour and does not pin
* the post-destruction connected() value.
*
*
* path:      /tests/djinterp/core/paradigm/observer/observer_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.10
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_OBSERVER_TESTS_
#define DJINTERP_TEST_OBSERVER_TESTS_ 1

// djinterp
#include "../../../../../inc/djinterp/core/paradigm/observer/observer.hpp"                       // the header under test (bare form)
#include "djinterp/test/test_defaults.hpp"    // module_spec, run_module


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                TEST DECLARATIONS                                         ///
///////////////////////////////////////////////////////////////////////////////

// -- section: signature decomposition (observer_tests_sig_decompose.cpp)
bool tests_sig_decompose_return_type();
bool tests_sig_decompose_arity();
bool tests_sig_decompose_fn_ptr_type();
bool tests_sig_decompose_void_and_multi();

// -- section: connection handles (observer_tests_connection.cpp)
bool tests_connection_default();
bool tests_connection_disconnect();
bool tests_connection_reset();
bool tests_scoped_connection_raii();
bool tests_scoped_connection_move_ctor();
bool tests_scoped_connection_move_assign();
bool tests_scoped_connection_release();
bool tests_scoped_connection_default();
bool tests_scoped_connections_add();
bool tests_scoped_connections_plus_equals();
bool tests_scoped_connections_raii();

// -- section: tier 0 delegate (observer_tests_delegate.cpp)
bool tests_delegate_default_state();
bool tests_delegate_connect_replace();
bool tests_delegate_disconnect();
bool tests_delegate_disconnect_all();
bool tests_delegate_notify_void();
bool tests_delegate_notify_nonvoid();
bool tests_delegate_operator_call();
bool tests_delegate_get();
bool tests_delegate_implicit_ctor();
bool tests_delegate_capacity();
bool tests_delegate_stdfunction_captures();
bool tests_delegate_multi_arg();
bool tests_observer_ptr_alias();

// -- section: tier 1 event (observer_tests_event.cpp)
bool tests_event_default_state();
bool tests_event_connect_slot_ids();
bool tests_event_connect_full_no_slot();
bool tests_event_disconnect_and_reuse();
bool tests_event_disconnect_all();
bool tests_event_slot_connected();
bool tests_event_full_empty();
bool tests_event_notify_order();
bool tests_event_at();
bool tests_event_compact();
bool tests_event_operator_call();
bool tests_event_stdfunction();
bool tests_event_multi_arg();

// -- section: tier 2 observer (observer_tests_observer.cpp)
bool tests_observer_default_state();
bool tests_observer_connect_notify();
bool tests_observer_operator_plus_equals();
bool tests_observer_disconnect_via_connection();
bool tests_observer_disconnect_all();
bool tests_observer_size_including_dead();
bool tests_observer_notify_skips_dead();
bool tests_observer_compact_keeps_live();
bool tests_observer_reserve();
bool tests_observer_scoped_raii();
bool tests_observer_connection_outlives_safe();
bool tests_observer_fn_ptr_override();
bool tests_observer_multi_arg();

// -- section: capability traits (observer_tests_traits.cpp)
bool tests_trait_has_connect();
bool tests_trait_has_notify();
bool tests_trait_has_disconnect_all();
bool tests_trait_has_count();
bool tests_trait_has_compact();
bool tests_trait_type_alias_detectors();
bool tests_trait_has_static_capacity();
bool tests_trait_disconnect_detectors();
bool tests_trait_connect_return_detectors();
bool tests_trait_is_observable();
bool tests_trait_is_delegate();
bool tests_trait_is_event();
bool tests_trait_is_observer();
bool tests_trait_is_bounded();
bool tests_trait_has_connection_tracking();
bool tests_trait_observable_arity();


///////////////////////////////////////////////////////////////////////////////
///                SPEC-PROVIDER FACE  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// observer_spec
//   function: assembles the module_spec - one block per semantic section of
// observer.hpp, one test per tests_* predicate. Compiled only in the runner TU.
inline ::djinterp::test::module_spec
observer_spec()
{
    namespace dt = ::djinterp::test;

    dt::module_spec m;

    m.name       = "observer";
    m.descriptor =
        "Three-tier observer pattern: signature decomposition, connection "
        "handles, the delegate / event / observer tiers, and the compile-time "
        "capability traits.";

    m.blocks = std::vector<dt::block_spec>
    {
        dt::block_spec
        {
            "signature decomposition",
            "sig_decompose extracts return type, fn-ptr type, and arity.",
            std::vector<dt::test_spec>
            {
                { "return_type", "return type is extracted.",        &tests_sig_decompose_return_type   },
                { "arity",       "argument count is extracted.",      &tests_sig_decompose_arity         },
                { "fn_ptr_type", "function-pointer type is formed.",  &tests_sig_decompose_fn_ptr_type   },
                { "void / multi","zero-arg and multi-arg signatures.", &tests_sig_decompose_void_and_multi }
            }
        },
        dt::block_spec
        {
            "connection handles",
            "connection, scoped_connection, scoped_connections.",
            std::vector<dt::test_spec>
            {
                { "connection default",       "default handle is disconnected.",   &tests_connection_default            },
                { "connection disconnect",    "disconnect flips the live flag.",   &tests_connection_disconnect         },
                { "connection reset",         "reset releases without disconnect.", &tests_connection_reset             },
                { "scoped_connection RAII",   "disconnects on destruction.",       &tests_scoped_connection_raii        },
                { "scoped_connection move",   "move transfers the guard.",         &tests_scoped_connection_move_ctor   },
                { "scoped_connection move=",  "move-assign disconnects the old.",  &tests_scoped_connection_move_assign },
                { "scoped_connection release","release surrenders the guard.",     &tests_scoped_connection_release     },
                { "scoped_connection default","default guard is inert.",           &tests_scoped_connection_default     },
                { "scoped_connections add",   "add tracks a connection.",          &tests_scoped_connections_add        },
                { "scoped_connections +=",    "operator+= tracks a connection.",   &tests_scoped_connections_plus_equals },
                { "scoped_connections RAII",  "disconnects all on destruction.",   &tests_scoped_connections_raii       }
            }
        },
        dt::block_spec
        {
            "tier 0: delegate",
            "single-slot observable, raw fn-ptr default.",
            std::vector<dt::test_spec>
            {
                { "default state",     "unconnected on construction.",        &tests_delegate_default_state       },
                { "connect replaces",  "connect swaps the stored callable.",  &tests_delegate_connect_replace     },
                { "disconnect",        "disconnect clears the callable.",     &tests_delegate_disconnect          },
                { "disconnect_all",    "alias for disconnect.",               &tests_delegate_disconnect_all      },
                { "notify (void)",     "invokes when connected.",             &tests_delegate_notify_void         },
                { "notify (non-void)", "returns R{} when unconnected.",       &tests_delegate_notify_nonvoid      },
                { "operator()",        "call operator aliases notify.",       &tests_delegate_operator_call       },
                { "get",               "direct access to the callable.",      &tests_delegate_get                 },
                { "implicit ctor",     "constructs from a callable.",         &tests_delegate_implicit_ctor       },
                { "capacity",          "capacity is one.",                    &tests_delegate_capacity            },
                { "std::function",     "captures via a function override.",   &tests_delegate_stdfunction_captures },
                { "multi-arg",         "forwards multiple arguments.",        &tests_delegate_multi_arg           },
                { "observer_ptr alias","observer_ptr is a delegate.",         &tests_observer_ptr_alias           }
            }
        },
        dt::block_spec
        {
            "tier 1: event",
            "fixed-capacity inline array, slot ids.",
            std::vector<dt::test_spec>
            {
                { "default state",       "empty on construction.",             &tests_event_default_state         },
                { "connect slot ids",    "returns ascending slot ids.",        &tests_event_connect_slot_ids      },
                { "connect when full",   "returns no_slot when full.",         &tests_event_connect_full_no_slot  },
                { "disconnect + reuse",  "freed slot is reused first.",        &tests_event_disconnect_and_reuse  },
                { "disconnect_all",      "nulls every slot.",                  &tests_event_disconnect_all        },
                { "slot_connected",      "per-slot liveness query.",           &tests_event_slot_connected        },
                { "full / empty",        "occupancy predicates.",              &tests_event_full_empty            },
                { "notify order",        "invokes slots front to back.",       &tests_event_notify_order          },
                { "at",                  "direct slot access.",                &tests_event_at                    },
                { "compact",             "defragments live slots forward.",    &tests_event_compact               },
                { "operator()",          "call operator aliases notify.",      &tests_event_operator_call         },
                { "std::function",       "captures via a function override.",  &tests_event_stdfunction           },
                { "multi-arg",           "forwards multiple arguments.",       &tests_event_multi_arg             }
            }
        },
        dt::block_spec
        {
            "tier 2: observer",
            "dynamic, connection-tracked, unbounded.",
            std::vector<dt::test_spec>
            {
                { "default state",         "empty on construction.",              &tests_observer_default_state          },
                { "connect + notify",      "connect returns a handle; notify runs.", &tests_observer_connect_notify      },
                { "operator+=",            "shorthand for connect.",              &tests_observer_operator_plus_equals   },
                { "disconnect via handle", "connection.disconnect drops a slot.",  &tests_observer_disconnect_via_connection },
                { "disconnect_all",        "detaches every slot.",                &tests_observer_disconnect_all         },
                { "size_including_dead",   "dead slots linger until compact.",    &tests_observer_size_including_dead    },
                { "notify skips dead",     "dead slots are not invoked.",         &tests_observer_notify_skips_dead      },
                { "compact keeps live",    "compact frees dead, spares live handles.", &tests_observer_compact_keeps_live },
                { "reserve",               "pre-allocation is a no-op to callers.", &tests_observer_reserve              },
                { "scoped RAII",           "scoped_connection detaches on scope exit.", &tests_observer_scoped_raii      },
                { "handle outlives obs",   "a handle stays safe after destruction.", &tests_observer_connection_outlives_safe },
                { "fn_ptr override",       "observer over a raw fn-ptr callable.", &tests_observer_fn_ptr_override        },
                { "multi-arg",             "forwards multiple arguments.",        &tests_observer_multi_arg              }
            }
        },
        dt::block_spec
        {
            "capability traits",
            "SFINAE detectors and composite identity traits.",
            std::vector<dt::test_spec>
            {
                { "has_connect",          "detects .connect(callable).",          &tests_trait_has_connect             },
                { "has_notify",           "detects .notify.",                     &tests_trait_has_notify              },
                { "has_disconnect_all",   "detects .disconnect_all().",           &tests_trait_has_disconnect_all      },
                { "has_count",            "detects .count().",                    &tests_trait_has_count               },
                { "has_compact",          "detects .compact().",                  &tests_trait_has_compact             },
                { "type-alias detectors", "signature/callable/return_type.",      &tests_trait_type_alias_detectors    },
                { "has_static_capacity",  "detects ::capacity.",                  &tests_trait_has_static_capacity     },
                { "disconnect detectors", "void vs by-id disconnect shape.",      &tests_trait_disconnect_detectors    },
                { "connect-return detect","void / slot_id / connection return.",  &tests_trait_connect_return_detectors },
                { "is_observable",        "connect + notify + count.",            &tests_trait_is_observable           },
                { "is_delegate",          "single-slot classification.",          &tests_trait_is_delegate             },
                { "is_event",             "fixed-capacity classification.",       &tests_trait_is_event                },
                { "is_observer",          "dynamic classification.",              &tests_trait_is_observer             },
                { "is_bounded",           "non-zero static capacity.",            &tests_trait_is_bounded              },
                { "has_connection_tracking","connection-returning connect.",      &tests_trait_has_connection_tracking },
                { "observable_arity",     "signature arity extraction.",          &tests_trait_observable_arity        }
            }
        }
    };

    return m;
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_OBSERVER_TESTS_
