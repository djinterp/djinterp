/******************************************************************************
* djinterp [test]                                            mediator_tests.hpp
*
*   Unit-test suite header for inc/djinterp/patterns/mediator/mediator.hpp.
* Two faces, selected by DTEST_SPEC_MODE:
*
*     - default face: the flat declarations of every tests_* predicate (each a
*       nullary bool returning true iff all its checks passed) plus the shared
*       event payload types the section TUs build against.  Every
*       mediator_tests_*.cpp includes the header this way and DEFINES the
*       predicates it owns.
*
*     - spec-provider face (DTEST_SPEC_MODE defined): the above PLUS
*       mediator_spec(), which assembles the module_spec the runner hands to
*       run_module.  Only the runner defines DTEST_SPEC_MODE, so mediator_spec()
*       is compiled exactly once and the section TUs contribute only bodies.
*
*   BUILD PREREQUISITE (read BUG NOTES below):
*   mediator.hpp as-shipped does not compile on a conforming toolchain
* (GCC/Clang) for two reasons — a Windows-only include path and a trailing
* return type that names a member before it is declared.  Both are accepted by
* MSVC in its permissive mode but rejected by conforming compilers, and both
* block the ENTIRE header from parsing.  There is therefore no partial-coverage
* gate to hide behind: the two one-line fixes must be applied for this suite
* (or anything else) to build on GCC/Clang.  The fixes are in the accompanying
* patch; the suite targets the corrected header.
*
* path:      /tests/djinterp/patterns/mediator/mediator_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.09
******************************************************************************/

#ifndef DJINTERP_TEST_MEDIATOR_TESTS_
#define DJINTERP_TEST_MEDIATOR_TESTS_ 1

// std
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "mediator.hpp"                       // the header under test (bare form)
#include "djinterp/test/test_defaults.hpp"    // module_spec, run_module (full-from-inc)


///////////////////////////////////////////////////////////////////////////////
///                BUG NOTES (mediator.hpp, as shipped)                     ///
///////////////////////////////////////////////////////////////////////////////
//
//   Both defects below are accepted by MSVC's permissive front-end but are
//   hard errors on GCC/Clang, and each one stops the whole header from
//   compiling — so the suite is validated against the corrected header.
//
//   BUG 1 — non-portable include path.  Line 113 reads
//
//       #include ".\compat\std\any.hpp"
//
//   with backslash separators.  On POSIX toolchains the backslashes are literal
//   filename characters, so the file never resolves ("No such file or
//   directory") — even though the module advertises C++11-through-C++20
//   portability, and the sibling includes on lines 111-112 use forward slashes.
//   Fix: "compat/std/any.hpp".
//
//   BUG 2 — trailing return type names a member declared later.  In
//   static_mediator::get_colleague():
//
//       template<std::size_t _Index>
//       auto get_colleague() const
//           -> decltype(std::get<_Index>(m_colleagues))   // <- m_colleagues
//       { return std::get<_Index>(m_colleagues); }        //    declared below
//
//   m_colleagues is declared at the bottom of the class.  A trailing return
//   type is NOT a complete-class context, so the name is not yet visible and
//   GCC/Clang reject it ("'m_colleagues' was not declared in this scope").  The
//   function body is fine (a complete-class context).  Minimal fix that keeps
//   C++11 and the exact semantics: compute the element type without naming the
//   member —
//
//       -> typename std::tuple_element<_Index, std::tuple<_Colleagues*...>>::type
//
//   (or declare m_colleagues before the method, or use a plain auto return).
//
//   PORTABILITY NOTE (not a bug): mediator.hpp routes all type erasure through
//   djinterp::compat::any, which is external to this module.  The suite depends
//   only on its documented surface (construct-from-value, type(), get<T>(),
//   holds<T>(), any_type_id, any_type_id_of<T>::value).


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                SHARED EVENT PAYLOAD TYPES                                ///
///////////////////////////////////////////////////////////////////////////////
//   Trivial value types used as events across several sections.  Section-local
// colleague / mediator / handler fixtures are defined (in anonymous namespaces)
// in the owning TU.

// ping / pong / tick
//   types: distinct trivial event payloads.  Distinct types matter: the event
// bus and variant bus dispatch by type identity, so these let a test prove that
// a handler for one type does not fire for another.
struct ping
{
    int n;
};

struct pong
{
    int m;
};

struct tick
{
    int t;
};


///////////////////////////////////////////////////////////////////////////////
///                TEST DECLARATIONS                                         ///
///////////////////////////////////////////////////////////////////////////////

// -- section: configuration & mediator traits (mediator_tests_traits.cpp)
bool tests_feature_gates();
bool tests_has_notify_method();
bool tests_has_subscribe_method();
bool tests_has_set_mediator_method();
bool tests_has_on_event_method();
bool tests_is_mediator_like();
bool tests_is_colleague_like();
bool tests_mediator_capability();

// -- section: classic mediator (mediator_tests_classic.cpp)
bool tests_colleague_id_type();
bool tests_colleague_base_defaults();
bool tests_concrete_mediator_add();
bool tests_concrete_mediator_broadcast();
bool tests_concrete_mediator_remove();
bool tests_colleague_send_no_mediator();

// -- section: event bus (mediator_tests_event_bus.cpp)
bool tests_subscription_token_type();
bool tests_event_bus_subscribe_publish();
bool tests_event_bus_type_filtering();
bool tests_event_bus_multiple_handlers();
bool tests_event_bus_unsubscribe();
bool tests_event_bus_publish_move();
bool tests_event_bus_publish_lvalue();
bool tests_event_bus_notify();
bool tests_event_bus_notify_type_gated();
bool tests_event_bus_handler_counts();
bool tests_event_bus_clear();

// -- section: variant event bus (mediator_tests_variant_bus.cpp)
bool tests_variant_bus_subscribe_publish();
bool tests_variant_bus_multiple_types();
bool tests_variant_bus_subscribe_all();
bool tests_variant_bus_unsubscribe();
bool tests_variant_bus_handler_count();
bool tests_variant_bus_clear();

// -- section: signal hub (mediator_tests_signal_hub.cpp)
bool tests_signal_id_type();
bool tests_signal_hub_connect_emit();
bool tests_signal_hub_connect_literal();
bool tests_signal_hub_multicast();
bool tests_signal_hub_channels();
bool tests_signal_hub_disconnect();
bool tests_signal_hub_disconnect_all();
bool tests_signal_hub_connect_typed();
bool tests_signal_hub_emit_typed();
bool tests_signal_hub_string_view_connect();
bool tests_signal_hub_has_channel();
bool tests_signal_hub_clear();

// -- section: static mediator — CRTP (mediator_tests_static.cpp)
bool tests_static_colleague_defaults();
bool tests_static_mediator_register();
bool tests_static_mediator_get_colleague();
bool tests_static_mediator_broadcast();
bool tests_static_mediator_broadcast_reverse();
bool tests_static_colleague_send_no_mediator();

// -- section: dispatch policies (mediator_tests_dispatch.cpp)
bool tests_broadcast_policy();
bool tests_targeted_policy();
bool tests_filtered_policy();

// -- section: factories + concept-constrained interfaces (mediator_tests_factory_concepts.cpp)
bool tests_make_event_bus();
bool tests_make_signal_hub();
bool tests_subscribe_many();
bool tests_unsubscribe_all();
bool tests_mediator_for();
bool tests_colleague_of();
bool tests_handler_for();
bool tests_constrained_subscribe();
bool tests_constrained_connect();


///////////////////////////////////////////////////////////////////////////////
///                SPEC-PROVIDER FACE  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// mediator_spec
//   function: assembles the module_spec for the mediator suite — one block per
// semantic section of mediator.hpp, one test per tests_* predicate.  Compiled
// only in the runner TU (the sole definer of DTEST_SPEC_MODE); the section TUs
// supply the predicate bodies these entries point at.
inline ::djinterp::test::module_spec
mediator_spec()
{
    namespace dt = ::djinterp::test;

    dt::module_spec m;

    m.name       = "mediator";
    m.descriptor =
        "Mediator pattern module: conformance traits, the classic virtual "
        "mediator, the type-erased event bus, the variant event bus, the "
        "named-channel signal hub, the CRTP static mediator, the dispatch "
        "policies, the convenience factories, and the C++20 concept surface.";

    m.blocks = std::vector<dt::block_spec>
    {
        dt::block_spec
        {
            "configuration & mediator traits",
            "feature gates and the SFINAE protocol-detection traits.",
            std::vector<dt::test_spec>
            {
                { "feature gates",           "D_MEDIATOR_HAS_* track the language level.", &tests_feature_gates           },
                { "has_notify_method",       "detects notify(const any&).",                &tests_has_notify_method       },
                { "has_subscribe_method",    "detects subscribe(function<...>).",          &tests_has_subscribe_method    },
                { "has_set_mediator_method", "detects set_mediator(ptr).",                 &tests_has_set_mediator_method },
                { "has_on_event_method",     "detects on_event(const any&).",              &tests_has_on_event_method     },
                { "is_mediator_like",        "notify-capable classification.",             &tests_is_mediator_like        },
                { "is_colleague_like",       "participation classification (+ _v).",       &tests_is_colleague_like       },
                { "mediator_capability",     "aggregate conformance struct.",              &tests_mediator_capability     }
            }
        },
        dt::block_spec
        {
            "classic mediator",
            "virtual mediator_base / colleague_base / concrete_mediator.",
            std::vector<dt::test_spec>
            {
                { "colleague_id type",         "opaque id alias is size_t.",            &tests_colleague_id_type         },
                { "colleague_base defaults",   "default id / mediator binding.",        &tests_colleague_base_defaults   },
                { "concrete_mediator add",     "registration assigns ids + binds.",     &tests_concrete_mediator_add     },
                { "concrete_mediator receive", "broadcast to all but the sender.",      &tests_concrete_mediator_broadcast },
                { "concrete_mediator remove",  "unregistration stops delivery.",        &tests_concrete_mediator_remove  },
                { "colleague send unbound",    "send() with no mediator is a no-op.",   &tests_colleague_send_no_mediator }
            }
        },
        dt::block_spec
        {
            "event bus",
            "type-indexed publish/subscribe over compat::any.",
            std::vector<dt::test_spec>
            {
                { "subscription_token type", "opaque token alias is size_t.",        &tests_subscription_token_type    },
                { "subscribe + publish",     "handler fires for its event type.",    &tests_event_bus_subscribe_publish },
                { "type filtering",          "handlers fire only for their type.",   &tests_event_bus_type_filtering   },
                { "multiple handlers",       "all handlers for a type fire.",        &tests_event_bus_multiple_handlers },
                { "unsubscribe",             "token removal stops delivery.",        &tests_event_bus_unsubscribe      },
                { "publish (move)",          "rvalue publish overload.",             &tests_event_bus_publish_move     },
                { "publish (lvalue)",        "lvalue publish overload.",             &tests_event_bus_publish_lvalue   },
                { "notify",                  "pre-wrapped any dispatch by type id.", &tests_event_bus_notify           },
                { "notify type-gated",       "notify skips non-matching handlers.",  &tests_event_bus_notify_type_gated },
                { "handler counts",          "handler_count / handler_count_for.",   &tests_event_bus_handler_counts   },
                { "clear",                   "clear removes every handler.",         &tests_event_bus_clear            }
            }
        },
        dt::block_spec
        {
            "variant event bus",
            "closed-set std::variant bus (C++17+).",
            std::vector<dt::test_spec>
            {
                { "subscribe + publish", "typed handler fires for its alternative.", &tests_variant_bus_subscribe_publish },
                { "multiple types",      "each typed handler fires for its type.",   &tests_variant_bus_multiple_types    },
                { "subscribe_all",       "visitor handler sees every event.",        &tests_variant_bus_subscribe_all     },
                { "unsubscribe",         "token removal stops delivery.",            &tests_variant_bus_unsubscribe       },
                { "handler_count",       "handler_count reflects subscriptions.",    &tests_variant_bus_handler_count     },
                { "clear",               "clear removes every handler.",             &tests_variant_bus_clear             }
            }
        },
        dt::block_spec
        {
            "signal hub",
            "named-channel multicast over compat::any.",
            std::vector<dt::test_spec>
            {
                { "signal_id type",       "channel id alias is std::string.",        &tests_signal_id_type            },
                { "connect + emit",       "handler fires on its channel.",           &tests_signal_hub_connect_emit   },
                { "connect (literal)",    "string-literal channel is unambiguous.",  &tests_signal_hub_connect_literal },
                { "multicast",            "all handlers on a channel fire.",         &tests_signal_hub_multicast      },
                { "channel isolation",    "emit reaches only the named channel.",    &tests_signal_hub_channels       },
                { "disconnect",           "token removal stops delivery.",           &tests_signal_hub_disconnect     },
                { "disconnect_all",       "channel clear stops delivery.",           &tests_signal_hub_disconnect_all },
                { "connect_typed",        "typed extraction from the any wrapper.",  &tests_signal_hub_connect_typed  },
                { "emit_typed",           "typed wrap + emit.",                      &tests_signal_hub_emit_typed     },
                { "connect (string_view)","string_view channel overload (C++17+).",  &tests_signal_hub_string_view_connect },
                { "has_channel",          "channel occupancy query.",                &tests_signal_hub_has_channel    },
                { "clear",                "clear removes every slot.",               &tests_signal_hub_clear          }
            }
        },
        dt::block_spec
        {
            "static mediator (CRTP)",
            "compile-time colleague set; zero virtual dispatch.",
            std::vector<dt::test_spec>
            {
                { "static_colleague defaults", "default mediator pointer is null.",  &tests_static_colleague_defaults      },
                { "register_colleague",        "indexed registration binds mediator.",&tests_static_mediator_register       },
                { "get_colleague",             "indexed retrieval returns the ptr.",  &tests_static_mediator_get_colleague  },
                { "broadcast_except",          "sender excluded from broadcast.",     &tests_static_mediator_broadcast      },
                { "broadcast (reverse)",       "any colleague may originate.",        &tests_static_mediator_broadcast_reverse },
                { "send unbound",              "send() with no mediator is a no-op.", &tests_static_colleague_send_no_mediator }
            }
        },
        dt::block_spec
        {
            "dispatch policies",
            "broadcast / targeted / filtered routing strategies.",
            std::vector<dt::test_spec>
            {
                { "broadcast_policy", "delivers to every handler.",           &tests_broadcast_policy },
                { "targeted_policy",  "delivers to handlers matching a pred.", &tests_targeted_policy  },
                { "filtered_policy",  "delivers only if the event passes.",    &tests_filtered_policy  }
            }
        },
        dt::block_spec
        {
            "factories & concept-constrained interfaces",
            "make_* helpers (C++14+) and the C++20 concept surface.",
            std::vector<dt::test_spec>
            {
                { "make_event_bus",        "factory yields a usable event bus.",   &tests_make_event_bus       },
                { "make_signal_hub",       "factory yields a usable signal hub.",  &tests_make_signal_hub      },
                { "subscribe_many",        "batch subscription returns tokens.",   &tests_subscribe_many       },
                { "unsubscribe_all",       "batch unsubscription clears handlers.",&tests_unsubscribe_all      },
                { "mediator_for",          "notify-surface concept.",              &tests_mediator_for         },
                { "colleague_of",          "set_mediator + on_event concept.",     &tests_colleague_of         },
                { "handler_for",           "invocability concept.",                &tests_handler_for          },
                { "constrained_subscribe", "concept-guarded subscription.",        &tests_constrained_subscribe },
                { "constrained_connect",   "concept-guarded connection.",          &tests_constrained_connect  }
            }
        }
    };

    return m;
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_MEDIATOR_TESTS_
