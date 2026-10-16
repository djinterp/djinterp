/*******************************************************************************
* djinterp [test]                                            publisher_tests.hpp
*
*   Unit-test suite header for the publish-subscribe paradigm foundation -
* status codes, subscription tokens, the three subscription handle types, the
* message envelope, the subscriber registry, the CRTP broker base and its
* customization points, the concrete bus, the conformance traits, the C++20
* concepts, and the factories.  Two faces, selected by DTEST_SPEC_MODE:
*
*     - default face: the flat declarations of every tests_* predicate plus the
*       shared fixtures the section TUs build against.  Each
*       publisher_tests_*.cpp includes the header this way and DEFINES the
*       predicates it owns.  Section-local fixtures (derived brokers, free
*       function handlers) live in anonymous namespaces in the owning TU.
*
*     - spec-provider face (DTEST_SPEC_MODE defined): the above PLUS
*       publisher_spec(), which the runner hands to run_module.  Only the runner
*       defines DTEST_SPEC_MODE, so the spec is compiled exactly once.
*
*   LANGUAGE FLOOR: the module uses std::void_t and [[nodiscard]]
* unconditionally, so C++17 is the floor.  The suite targets C++17 and up; the
* only dialect-conditional surface is the C++20 broker_type / handler_for
* concepts, guarded by D_ENV_CPP_FEATURE_LANG_CONCEPTS.
*
*   REVOCATION MODEL (the crux of most tests below): a subscription owns a
* shared "alive" flag that the matching registry entry also holds.
*     - subscription::unsubscribe() is a LAZY revoke - it flips the flag, so the
*       broker skips the entry on the next publish, but the entry's storage
*       stays until compact() reclaims it.  Live-subscriber counts therefore
*       drop immediately even though the entry is still physically present, and
*       broker::unsubscribe(token) can still find it.
*     - broker::unsubscribe(token) / unsubscribe_topic() / unsubscribe_all() are
*       EAGER - they flip the flag AND erase the entry (and its topic bucket,
*       once empty).
*     - subscription::release() detaches the handle WITHOUT revoking: the
*       registration stays live and keeps receiving publishes.
*   Because the flag is shared rather than a back-pointer, a handle never
* dangles; it stays safe to query and revoke even after its broker is gone.
*
*
* path:      /tests/djinterp/core/paradigm/publisher/publisher_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.11
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_PUBLISHER_TESTS_
#define DJINTERP_TEST_PUBLISHER_TESTS_ 1

// std
#include <cstddef>
#include <functional>
#include <string>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/core/paradigm/publisher/publisher.hpp"                      // the header under test (bare form)
#include "djinterp/test/test_defaults.hpp"    // module_spec, run_module


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                SHARED FIXTURES                                          ///
///////////////////////////////////////////////////////////////////////////////
//   Class definitions and aliases only - no shared mutable state - so the
// header is safe to include from every section TU.

// string_bus
//   alias: the canonical bus under test - std::string topics, int payloads, the
// default std::function handler.
using string_bus = bus<std::string, int>;

// string_registry
//   alias: a registry matching string_bus, for testing the storage core on its
// own (without a broker wrapped around it).
using string_registry =
    subscriber_registry<std::string, std::function<void(const std::string&, const int&)> >;


// delivery
//   struct: one recorded delivery - the topic and payload a handler received.
struct delivery
{
    std::string topic;
    int         payload;

    delivery()
        : topic  (),
          payload(0)
    {}

    delivery(const std::string& _topic,
             int                _payload)
        : topic  (_topic),
          payload(_payload)
    {}
};


// recorder
//   type: a handler-shaped functor that appends every delivery to a
// caller-owned log.  It holds only a pointer, so it stays copyable and can be
// stored in a std::function slot, in a raw-functor handler slot, or captured by
// value - which lets one fixture serve every handler-type variation the module
// supports.
struct recorder
{
    std::vector<delivery>* log;

    explicit recorder(std::vector<delivery>* _log = nullptr)
        : log(_log)
    {}

    void
    operator()(const std::string& _topic,
               const int&         _payload) const
    {
        if (log != nullptr)
        {
            log->push_back(delivery(_topic, _payload));
        }

        return;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                TEST DECLARATIONS                                         ///
///////////////////////////////////////////////////////////////////////////////

// -- section: status codes & tokens (publisher_tests_status_token.cpp)
bool tests_status_codes();
bool tests_status_default_publish_codes();
bool tests_token_default();
bool tests_token_explicit();
bool tests_token_valid();
bool tests_token_equality();
bool tests_token_id_type();

// -- section: subscription handles (publisher_tests_subscription.cpp)
bool tests_subscription_default();
bool tests_subscription_active();
bool tests_subscription_token_accessor();
bool tests_subscription_unsubscribe();
bool tests_subscription_unsubscribe_idempotent();
bool tests_subscription_default_unsubscribe_safe();
bool tests_subscription_release();
bool tests_subscription_copies_share_flag();
bool tests_subscription_outlives_broker();
bool tests_scoped_subscription_default();
bool tests_scoped_subscription_raii();
bool tests_scoped_subscription_move_ctor();
bool tests_scoped_subscription_move_assign();
bool tests_scoped_subscription_self_move();
bool tests_scoped_subscription_release();
bool tests_scoped_subscription_unsubscribe();
bool tests_scoped_subscription_move_only();
bool tests_scoped_subscriptions_default();
bool tests_scoped_subscriptions_add();
bool tests_scoped_subscriptions_plus_equals();
bool tests_scoped_subscriptions_raii();
bool tests_scoped_subscriptions_unsubscribe_all();

// -- section: message envelope (publisher_tests_message.cpp)
bool tests_message_default();
bool tests_message_copy_ctor();
bool tests_message_move_ctor();
bool tests_message_typedefs();

// -- section: subscriber registry (publisher_tests_registry.cpp)
bool tests_registry_default();
bool tests_registry_add();
bool tests_registry_dispatch();
bool tests_registry_dispatch_unknown_topic();
bool tests_registry_dispatch_skips_dead();
bool tests_registry_remove();
bool tests_registry_remove_absent();
bool tests_registry_remove_drops_empty_bucket();
bool tests_registry_remove_topic();
bool tests_registry_remove_topic_absent();
bool tests_registry_clear();
bool tests_registry_compact();
bool tests_registry_compact_drops_dead_bucket();
bool tests_registry_subscriber_count();
bool tests_registry_has_subscribers();
bool tests_registry_topic_count();
bool tests_registry_empty();
bool tests_registry_reserve_topics();

// -- section: broker surface (publisher_tests_broker.cpp)
bool tests_broker_default_state();
bool tests_broker_subscribe_tokens();
bool tests_broker_publish_delivers();
bool tests_broker_publish_order();
bool tests_broker_publish_no_subscribers();
bool tests_broker_publish_topic_isolation();
bool tests_broker_publish_message();
bool tests_broker_operator_call();
bool tests_broker_unsubscribe_eager();
bool tests_broker_unsubscribe_absent();
bool tests_broker_unsubscribe_lazily_revoked_token();
bool tests_broker_unsubscribe_topic();
bool tests_broker_unsubscribe_topic_absent();
bool tests_broker_unsubscribe_all();
bool tests_broker_lazy_revoke_skipped();
bool tests_broker_lazy_revoke_all_dead();
bool tests_broker_compact();
bool tests_broker_subscriber_count();
bool tests_broker_has_subscribers();
bool tests_broker_topic_count();
bool tests_broker_empty();
bool tests_broker_reserve_topics();
bool tests_broker_not_directly_constructible();

// -- section: customization points & bus (publisher_tests_customization.cpp)
bool tests_bus_default_hooks();
bool tests_bus_typedefs();
bool tests_bus_fn_ptr_handler();
bool tests_bus_functor_handler();
bool tests_bus_nonstring_topic();
bool tests_hook_should_publish_veto();
bool tests_hook_should_publish_state();
bool tests_hook_do_deliver_override();
bool tests_hook_do_publish_override();

// -- section: traits & concepts (publisher_tests_traits.cpp)
bool tests_trait_has_subscribe();
bool tests_trait_has_publish();
bool tests_trait_has_unsubscribe();
bool tests_trait_is_broker();
bool tests_trait_is_broker_v();
bool tests_trait_is_subscription();
bool tests_trait_is_subscription_v();
bool tests_concept_broker_type();
bool tests_concept_handler_for();

// -- section: factories (publisher_tests_factories.cpp)
bool tests_make_bus();
bool tests_make_bus_custom_handler();
bool tests_make_message();


///////////////////////////////////////////////////////////////////////////////
///                SPEC-PROVIDER FACE  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// publisher_spec
//   function: assembles the module_spec - one block per semantic section of the
// module, one test per tests_* predicate.  Compiled only in the runner TU.
inline ::djinterp::test::module_spec
publisher_spec()
{
    namespace dt = ::djinterp::test;

    dt::module_spec m;

    m.name       = "pubsub";
    m.descriptor =
        "Publish-subscribe paradigm foundation: status codes, subscription "
        "tokens and handles, the message envelope, the subscriber registry, the "
        "CRTP broker base and its customization points, the synchronous bus, the "
        "conformance traits, the C++20 concepts, and the factories.";

    m.blocks = std::vector<dt::block_spec>
    {
        dt::block_spec
        {
            "status codes & subscription tokens",
            "the publish status constants and the opaque registration id.",
            std::vector<dt::test_spec>
            {
                { "status codes",        "the standard status constants.",        &tests_status_codes              },
                { "default publish codes","which codes the default routing emits.", &tests_status_default_publish_codes },
                { "token default",       "the null token has id zero.",           &tests_token_default             },
                { "token explicit",      "an explicit id is stored.",             &tests_token_explicit            },
                { "token valid",         "validity tracks a non-zero id.",        &tests_token_valid               },
                { "token equality",      "== and != compare ids.",                &tests_token_equality            },
                { "token id_type",       "the id type is a 64-bit unsigned.",     &tests_token_id_type             }
            }
        },
        dt::block_spec
        {
            "subscription handles",
            "subscription, scoped_subscription, and scoped_subscriptions.",
            std::vector<dt::test_spec>
            {
                { "sub default",          "a default handle is inactive.",          &tests_subscription_default              },
                { "sub active",           "a fresh registration is live.",          &tests_subscription_active               },
                { "sub token",            "the handle carries its token.",          &tests_subscription_token_accessor       },
                { "sub unsubscribe",      "lazy revoke deactivates the handle.",    &tests_subscription_unsubscribe          },
                { "sub unsub idempotent", "revoking twice is safe.",                &tests_subscription_unsubscribe_idempotent },
                { "sub default unsub",    "revoking a null handle is a no-op.",     &tests_subscription_default_unsubscribe_safe },
                { "sub release",          "release detaches but keeps it live.",    &tests_subscription_release              },
                { "sub copies",           "copies share one liveness flag.",        &tests_subscription_copies_share_flag    },
                { "sub outlives broker",  "a handle never dangles.",                &tests_subscription_outlives_broker      },
                { "scoped default",       "a default guard is inert.",              &tests_scoped_subscription_default       },
                { "scoped RAII",          "revokes on destruction.",                &tests_scoped_subscription_raii          },
                { "scoped move ctor",     "move transfers the obligation.",         &tests_scoped_subscription_move_ctor     },
                { "scoped move assign",   "move-assign revokes the prior.",         &tests_scoped_subscription_move_assign   },
                { "scoped self-move",     "self-move-assign is guarded.",           &tests_scoped_subscription_self_move     },
                { "scoped release",       "release surrenders the obligation.",     &tests_scoped_subscription_release       },
                { "scoped unsubscribe",   "early revoke through the guard.",        &tests_scoped_subscription_unsubscribe   },
                { "scoped move-only",     "the guard is not copyable.",             &tests_scoped_subscription_move_only     },
                { "scoped_subs default",  "an empty collection.",                   &tests_scoped_subscriptions_default      },
                { "scoped_subs add",      "add tracks a subscription.",             &tests_scoped_subscriptions_add          },
                { "scoped_subs +=",       "operator+= tracks a subscription.",      &tests_scoped_subscriptions_plus_equals  },
                { "scoped_subs RAII",     "revokes every held subscription.",       &tests_scoped_subscriptions_raii         },
                { "scoped_subs unsub_all","explicit bulk revoke empties it.",       &tests_scoped_subscriptions_unsubscribe_all }
            }
        },
        dt::block_spec
        {
            "message envelope",
            "pubsub_message pairs a topic with its payload.",
            std::vector<dt::test_spec>
            {
                { "default",   "value-initialized topic and payload.", &tests_message_default   },
                { "copy ctor", "constructs from const lvalues.",       &tests_message_copy_ctor },
                { "move ctor", "constructs by moving both fields.",    &tests_message_move_ctor },
                { "typedefs",  "topic_type / payload_type aliases.",   &tests_message_typedefs  }
            }
        },
        dt::block_spec
        {
            "subscriber registry",
            "the shared topic -> subscribers store and dispatch loop.",
            std::vector<dt::test_spec>
            {
                { "default",             "a fresh registry is empty.",             &tests_registry_default                },
                { "add",                 "registration creates topic buckets.",    &tests_registry_add                    },
                { "dispatch",            "visits live handlers in order.",         &tests_registry_dispatch               },
                { "dispatch (unknown)",  "an unknown topic visits nobody.",        &tests_registry_dispatch_unknown_topic  },
                { "dispatch skips dead", "deactivated entries are skipped.",       &tests_registry_dispatch_skips_dead    },
                { "remove",              "eager removal deactivates and erases.",  &tests_registry_remove                 },
                { "remove (absent)",     "an unknown token reports false.",        &tests_registry_remove_absent          },
                { "remove drops bucket", "the topic goes with its last entry.",    &tests_registry_remove_drops_empty_bucket },
                { "remove_topic",        "drops every entry for one topic.",       &tests_registry_remove_topic           },
                { "remove_topic absent", "an unknown topic removes nothing.",      &tests_registry_remove_topic_absent    },
                { "clear",               "drops everything, deactivating flags.",  &tests_registry_clear                  },
                { "compact",             "reclaims lazily revoked entries.",       &tests_registry_compact                },
                { "compact drops bucket","an all-dead topic is dropped.",          &tests_registry_compact_drops_dead_bucket },
                { "subscriber_count",    "live tallies, overall and per topic.",   &tests_registry_subscriber_count       },
                { "has_subscribers",     "per-topic liveness query.",              &tests_registry_has_subscribers        },
                { "topic_count",         "distinct topics with a live entry.",     &tests_registry_topic_count            },
                { "empty",               "true when no entry is live.",            &tests_registry_empty                  },
                { "reserve_topics",      "pre-allocation is transparent.",         &tests_registry_reserve_topics         }
            }
        },
        dt::block_spec
        {
            "broker surface",
            "subscribe / publish / unsubscribe and bookkeeping, via bus.",
            std::vector<dt::test_spec>
            {
                { "default state",        "a fresh broker is empty.",              &tests_broker_default_state          },
                { "subscribe tokens",     "tokens are unique, issued from one.",   &tests_broker_subscribe_tokens       },
                { "publish delivers",     "every live subscriber receives it.",    &tests_broker_publish_delivers       },
                { "publish order",        "delivery follows registration order.",  &tests_broker_publish_order          },
                { "publish (no subs)",    "an unsubscribed topic reports so.",     &tests_broker_publish_no_subscribers },
                { "topic isolation",      "only the named topic receives.",        &tests_broker_publish_topic_isolation },
                { "publish (message)",    "the envelope overload routes.",         &tests_broker_publish_message        },
                { "operator()",           "the call operator aliases publish.",    &tests_broker_operator_call          },
                { "unsubscribe",          "eager revoke by token.",                &tests_broker_unsubscribe_eager      },
                { "unsubscribe (absent)", "an unknown token reports false.",       &tests_broker_unsubscribe_absent     },
                { "unsubscribe (lazy)",   "a lazily revoked entry is still found.", &tests_broker_unsubscribe_lazily_revoked_token },
                { "unsubscribe_topic",    "drops every subscriber of a topic.",    &tests_broker_unsubscribe_topic      },
                { "unsub_topic (absent)", "an unknown topic removes nothing.",     &tests_broker_unsubscribe_topic_absent },
                { "unsubscribe_all",      "drops every subscriber everywhere.",    &tests_broker_unsubscribe_all        },
                { "lazy revoke skipped",  "a revoked handler is not invoked.",     &tests_broker_lazy_revoke_skipped    },
                { "lazy revoke all dead", "an all-dead topic reports no subs.",    &tests_broker_lazy_revoke_all_dead   },
                { "compact",              "reclaims lazily revoked storage.",      &tests_broker_compact                },
                { "subscriber_count",     "live tallies, overall and per topic.",  &tests_broker_subscriber_count       },
                { "has_subscribers",      "per-topic liveness query.",             &tests_broker_has_subscribers        },
                { "topic_count",          "distinct topics with a live entry.",    &tests_broker_topic_count            },
                { "empty",                "true when no subscriber is live.",      &tests_broker_empty                  },
                { "reserve_topics",       "pre-allocation is transparent.",        &tests_broker_reserve_topics         },
                { "base not constructible","the CRTP base ctor is protected.",     &tests_broker_not_directly_constructible }
            }
        },
        dt::block_spec
        {
            "customization points & bus",
            "should_publish / do_deliver / do_publish overrides; bus defaults.",
            std::vector<dt::test_spec>
            {
                { "bus default hooks",   "bus overrides nothing and just works.",  &tests_bus_default_hooks        },
                { "bus typedefs",        "the inherited type aliases.",            &tests_bus_typedefs            },
                { "fn-ptr handler",      "a raw function pointer handler.",        &tests_bus_fn_ptr_handler      },
                { "functor handler",     "a custom functor handler.",              &tests_bus_functor_handler     },
                { "non-string topic",    "topics need only ==.",                   &tests_bus_nonstring_topic     },
                { "should_publish veto", "a payload veto yields Rejected.",        &tests_hook_should_publish_veto },
                { "should_publish state","a broker-state veto yields Rejected.",   &tests_hook_should_publish_state },
                { "do_deliver override", "per-handler delivery is reshaped.",      &tests_hook_do_deliver_override },
                { "do_publish override", "routing is replaced wholesale.",         &tests_hook_do_publish_override }
            }
        },
        dt::block_spec
        {
            "conformance traits & concepts",
            "pubsub_has_*, is_broker, is_subscription, and the C++20 concepts.",
            std::vector<dt::test_spec>
            {
                { "has_subscribe",     "detects subscribe(topic, handler).",   &tests_trait_has_subscribe    },
                { "has_publish",       "detects publish(topic, payload).",     &tests_trait_has_publish      },
                { "has_unsubscribe",   "detects unsubscribe(token).",          &tests_trait_has_unsubscribe  },
                { "is_broker",         "the composite broker protocol.",       &tests_trait_is_broker        },
                { "is_broker_v",       "the _v alias mirrors ::value.",        &tests_trait_is_broker_v      },
                { "is_subscription",   "the handle protocol (active+unsub).",  &tests_trait_is_subscription  },
                { "is_subscription_v", "the _v alias mirrors ::value.",        &tests_trait_is_subscription_v },
                { "broker_type",       "the C++20 broker concept (C++20+).",   &tests_concept_broker_type    },
                { "handler_for",       "the C++20 handler concept (C++20+).",  &tests_concept_handler_for    }
            }
        },
        dt::block_spec
        {
            "factories",
            "make_bus and make_message.",
            std::vector<dt::test_spec>
            {
                { "make_bus",          "yields a usable synchronous bus.",   &tests_make_bus               },
                { "make_bus (handler)","honours an explicit handler type.",  &tests_make_bus_custom_handler },
                { "make_message",      "builds a topic/payload envelope.",   &tests_make_message           }
            }
        }
    };

    return m;
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_PUBLISHER_TESTS_
