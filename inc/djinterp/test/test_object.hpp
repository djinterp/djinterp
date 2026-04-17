/******************************************************************************
* djinterp [test]                                             test_object.hpp
*
*   DTest framework test object header. Defines the unified test_object class
* template, parameterized on result type, status type, and a variadic option
* pack that configures capabilities at compile time.
*
*   TEMPLATE PARAMETERS:
*     _Type       — the evaluation result type. Must be convertible to bool.
*                   For simple assertions this is bool; for richer evaluations
*                   it may be any type with an operator bool() or explicit
*                   bool conversion.
*     _StatusType — the status classification enum. Must be an enumeration
*                   type. Defaults to DTestStatus. Users may substitute
*                   their own status enum to extend or redefine the set of
*                   test states.
*     _Options... — zero or more option tags from test_options.hpp:
*                     test_opt_named              — name() / set_name()
*                     test_opt_message             — message() / set_message()
*                     test_opt_skippable           — skip()
*                     test_opt_events<_Handler>    — event dispatch
*                     test_opt_children<_Container> — interior node
*
*   NODE CLASSIFICATION:
*   A test_object without test_opt_children is a leaf node. Its boolean
* value comes from evaluating its stored _Type result.
*   A test_object with test_opt_children<_Container> is an interior node.
* Its boolean value is the logical conjunction of its children: true if,
* and only if, every child converts to true. An empty group evaluates
* to true (vacuous truth).
*
*   CONTAINER FREEDOM:
*   The container holding child test_objects is up to the user. Any
* iterable type is accepted — arrays, vectors, trees, or any
* user-defined container.
*
*   EVENT DISPATCH:
*   Enabled only when test_opt_events<_Handler> is present. Event
* dispatch is runtime-only; constexpr evaluation paths do not fire
* events. Interior nodes fire events for themselves only — each child
* manages its own handler independently.
*
*   CONSTEXPR SUPPORT:
*   Leaf evaluation (operator bool, status) is constexpr when _Type
* supports constexpr construction and comparison. Interior node
* evaluation requires iteration and is constexpr under C++14+ relaxed
* constexpr rules. Event dispatch is never constexpr.
*
*   PORTABILITY:
*   Uses D_TEST_CONSTEXPR (C++14+ relaxed constexpr), D_CONSTEXPR
* (C++11 simple constexpr), and env.h for version detection.
* C++20 concepts are used when available; pre-C++20 falls back to
* static_assert validation.
*
*   STORAGE:
*   Disabled options consume zero space via the empty base
* optimization. Only enabled options contribute data members.
*
*
* path:      /inc/test/test_object.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.05
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    INTERNAL HELPERS
      -----------------
      i.    base type selection
      ii.   status derivation helpers

II.   TEST OBJECT
      -----------
      i.    type aliases
      ii.   construction
      iii.  evaluation
            a. operator bool
            b. status
            c. passed / failed
            d. evaluate
      iv.   naming        (test_opt_named)
      v.    messaging     (test_opt_message)
      vi.   skip          (test_opt_skippable)
      vii.  children      (test_opt_children)
      viii. event dispatch (test_opt_events)

III.  CONVENIENCE TYPE ALIASES

IV.   FACTORY FUNCTIONS
      -----------------
      a. make_test
      b. make_named_test
      c. make_test_group
*/

#ifndef DJINTERP_TEST_OBJECT_
#define DJINTERP_TEST_OBJECT_ 1

#include <cstddef>
#include <type_traits>
#include "../djinterp.hpp"
#include "./test_common.hpp"
#include "./test_options.hpp"
#include "./test_concepts.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   INTERNAL HELPERS                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // -----------------------------------------------------------------
    //  base type selection
    // -----------------------------------------------------------------
    // Each option maps to either its storage base or a tagged
    // empty base, selected via std::conditional. The empty
    // bases have distinct _Tag values to avoid duplicate base
    // class errors under the empty base optimization.

    // name_base_for
    //   type: resolves to test_name_base when test_opt_named
    // is present, else test_empty_base<0>.
    template<typename... _Options>
    using name_base_for = typename std::conditional<
        has_test_opt_named<_Options...>::value,
        test_name_base,
        test_empty_base<0>
    >::type;

    // message_base_for
    //   type: resolves to test_message_base when
    // test_opt_message is present, else test_empty_base<1>.
    template<typename... _Options>
    using message_base_for = typename std::conditional<
        has_test_opt_message<_Options...>::value,
        test_message_base,
        test_empty_base<1>
    >::type;

    // event_base_for
    //   type: resolves to test_event_base<_Handler> when
    // test_opt_events is present, else test_empty_base<2>.
    template<typename... _Options>
    using event_base_for = typename std::conditional<
        has_test_opt_events<_Options...>::value,
        test_event_base<extract_event_handler_t<_Options...>>,
        test_empty_base<2>
    >::type;

    // children_base_for
    //   type: resolves to test_children_base<_Container> when
    // test_opt_children is present, else test_empty_base<3>.
    template<typename... _Options>
    using children_base_for = typename std::conditional<
        has_test_opt_children<_Options...>::value,
        test_children_base<extract_children_container_t<_Options...>>,
        test_empty_base<3>
    >::type;

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST OBJECT                                         ///
///////////////////////////////////////////////////////////////////////////////

// test_object
//   class: unified test node parameterized on result type,
// status type, and compile-time capability options. Inherits
// from conditional storage bases to ensure disabled options
// consume zero space.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
template<typename    _Type       = bool,
         typename    _StatusType = DTestStatus,
         typename... _Options>
    requires ( test_result_type<_Type>       &&
               test_status_type<_StatusType> &&
               valid_test_options<_Options...> )
#else
template<typename    _Type       = bool,
         typename    _StatusType = DTestStatus,
         typename... _Options>
#endif
class test_object
    : private internal::name_base_for<_Options...>,
      private internal::message_base_for<_Options...>,
      private internal::event_base_for<_Options...>,
      private internal::children_base_for<_Options...>
{
#if (!D_ENV_LANG_IS_CPP20_OR_HIGHER)
    static_assert(is_test_result_type<_Type>::value,
                  "Type parameter `_Type` must be convertible "
                  "to bool.");
    static_assert(is_test_status_type<_StatusType>::value,
                  "Type parameter `_StatusType` must be an "
                  "enumeration type.");
    static_assert(are_valid_test_options<_Options...>::value,
                  "All option parameters must be recognized "
                  "DTest option tags.");
#endif

private:
    // option set aggregate for compile-time queries
    using opts = test_option_set<_Options...>;

    // base type aliases (for accessing inherited storage)
    using name_base     = internal::name_base_for<_Options...>;
    using message_base  = internal::message_base_for<_Options...>;
    using event_base    = internal::event_base_for<_Options...>;
    using children_base = internal::children_base_for<_Options...>;

public:
    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------
    using value_type              = _Type;
    using status_type             = _StatusType;
    using option_set_type         = opts;
    using size_type               = std::size_t;
    using event_handler_type      = typename opts::event_handler_type;
    using children_container_type = typename opts::children_container_type;

    // compile-time capability flags
    static constexpr bool is_leaf        = opts::is_leaf;
    static constexpr bool is_interior    = opts::is_interior;
    static constexpr bool is_named       = opts::has_named;
    static constexpr bool is_event_aware = opts::has_events;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // test_object()
    //   default constructor: creates a pending test with a
    // default-constructed result.
    D_CONSTEXPR test_object() noexcept
            : name_base(),
              message_base(),
              event_base(),
              children_base(),
              m_result(),
              m_status(static_cast<_StatusType>(3))
        {}

    // test_object(_Type)
    //   constructor: creates a test from a pre-evaluated result.
    // Status is derived from the boolean value of the result.
    D_CONSTEXPR explicit test_object(
            const _Type& _result
        ) noexcept
            : name_base(),
              message_base(),
              event_base(),
              children_base(),
              m_result(_result),
              m_status(static_cast<bool>(_result)
                       ? static_cast<_StatusType>(0)
                       : static_cast<_StatusType>(1))
        {}

    // -----------------------------------------------------------------
    //  evaluation
    // -----------------------------------------------------------------

    // operator bool
    //   evaluates this test object to its boolean result.
    // For leaf nodes, converts the stored _Type result to bool.
    // For interior nodes, returns the conjunction of all
    // children.
    D_TEST_CONSTEXPR operator bool() const noexcept
    {
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
        if constexpr (opts::is_interior)
        {
            return evaluate_conjunction();
        }
        else
#endif
        {
            return static_cast<bool>(m_result);
        }
    }

    // status
    //   returns the current status of this test object.
    // For leaf nodes, returns the stored status.
    // For interior nodes, derives the aggregate status from
    // children.
    D_TEST_CONSTEXPR status_type status() const noexcept
    {
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
        if constexpr (opts::is_interior)
        {
            return derive_group_status();
        }
        else
#endif
        {
            return m_status;
        }
    }

    // passed
    //   returns true if this test's status is the passed value
    // (enum value 0).
    D_TEST_CONSTEXPR bool passed() const noexcept
    {
        return (status() == static_cast<_StatusType>(0));
    }

    // failed
    //   returns true if this test's status is the failed value
    // (enum value 1).
    D_TEST_CONSTEXPR bool failed() const noexcept
    {
        return (status() == static_cast<_StatusType>(1));
    }

    // evaluate (leaf)
    //   sets the result and derives the status. Fires events
    // at runtime when event dispatch is enabled.
    D_TEST_CONSTEXPR void evaluate(
        const _Type& _result
    ) noexcept
    {
        m_result = _result;
        m_status = static_cast<bool>(_result)
                   ? static_cast<_StatusType>(0)
                   : static_cast<_StatusType>(1);

        fire_event(DTestEvent::on_evaluate);

        // fire outcome event
        if (static_cast<bool>(_result))
        {
            fire_event(DTestEvent::on_pass);
        }
        else
        {
            fire_event(DTestEvent::on_fail);
        }

        return;
    }

    // result
    //   returns a const reference to the stored result value.
    D_CONSTEXPR const _Type& result() const noexcept
    {
        return m_result;
    }

    // -----------------------------------------------------------------
    //  naming (enabled by test_opt_named)
    // -----------------------------------------------------------------

    // name
    //   returns the human-readable name, or nullptr if unnamed.
    template<typename _Dummy = void>
    D_CONSTEXPR auto name() const noexcept
        -> typename std::enable_if<
               opts::has_named && std::is_void<_Dummy>::value,
               const char*
           >::type
    {
        return name_base::m_name;
    }

    // set_name
    //   sets the human-readable name.
    template<typename _Dummy = void>
    D_TEST_CONSTEXPR auto set_name(
        const char* _name
    ) noexcept
        -> typename std::enable_if<
               opts::has_named && std::is_void<_Dummy>::value,
               void
           >::type
    {
        name_base::m_name = _name;

        return;
    }

    // -----------------------------------------------------------------
    //  messaging (enabled by test_opt_message)
    // -----------------------------------------------------------------

    // message
    //   returns the diagnostic message, or nullptr.
    template<typename _Dummy = void>
    D_CONSTEXPR auto message() const noexcept
        -> typename std::enable_if<
               opts::has_message && std::is_void<_Dummy>::value,
               const char*
           >::type
    {
        return message_base::m_message;
    }

    // set_message
    //   sets the diagnostic message.
    template<typename _Dummy = void>
    D_TEST_CONSTEXPR auto set_message(
            const char* _message
        ) noexcept
        -> typename std::enable_if<
               opts::has_message && std::is_void<_Dummy>::value,
               void
           >::type
    {
        message_base::m_message = _message;

        return;
    }

    // -----------------------------------------------------------------
    //  skip (enabled by test_opt_skippable)
    // -----------------------------------------------------------------

    // skip
    //   marks this test as intentionally skipped (status 2).
    template<typename _Dummy = void>
    D_TEST_CONSTEXPR auto skip() noexcept
        -> typename std::enable_if<
               opts::has_skippable && std::is_void<_Dummy>::value,
               void
           >::type
    {
        m_result = _Type{};
        m_status = static_cast<_StatusType>(2);

        fire_event(DTestEvent::on_skip);

        return;
    }

    // -----------------------------------------------------------------
    //  children (enabled by test_opt_children)
    // -----------------------------------------------------------------

    // children (const)
    //   returns a const reference to the child container.
    template<typename _Dummy = void>
    D_CONSTEXPR auto children() const noexcept
        -> typename std::enable_if<
               opts::has_children && std::is_void<_Dummy>::value,
               const children_container_type&
           >::type
    {
        return children_base::m_members;
    }

    // children (mutable)
    //   returns a mutable reference to the child container.
    template<typename _Dummy = void>
    D_TEST_CONSTEXPR auto children() noexcept
        -> typename std::enable_if<
               opts::has_children && std::is_void<_Dummy>::value,
               children_container_type&
           >::type
    {
        return children_base::m_members;
    }

    // size
    //   returns the number of direct children.
    template<typename _Dummy = void>
    D_CONSTEXPR auto size() const noexcept
    -> typename std::enable_if<
            opts::has_children && std::is_void<_Dummy>::value,
            size_type
        >::type
    {
        return children_base::m_members.size();
    }

    // begin (const)
    //   returns a const iterator to the first child.
    template<typename _Dummy = void>
    D_CONSTEXPR auto begin() const noexcept
    -> typename std::enable_if<
            opts::has_children && std::is_void<_Dummy>::value,
            decltype(std::declval<
                const children_container_type&>().begin())
        >::type
    {
        return children_base::m_members.begin();
    }

    // end (const)
    //   returns a const iterator past the last child.
    template<typename _Dummy = void>
    D_CONSTEXPR auto end() const noexcept
    -> typename std::enable_if<
            opts::has_children && std::is_void<_Dummy>::value,
            decltype(std::declval<
                const children_container_type&>().end())
        >::type
    {
        return children_base::m_members.end();
    }

    // begin (mutable)
    //   returns a mutable iterator to the first child.
    template<typename _Dummy = void>
    D_TEST_CONSTEXPR auto begin() noexcept
    -> typename std::enable_if<
            opts::has_children && std::is_void<_Dummy>::value,
            decltype(std::declval<
                children_container_type&>().begin())
        >::type
    {
        return children_base::m_members.begin();
    }

    // end (mutable)
    //   returns a mutable iterator past the last child.
    template<typename _Dummy = void>
    D_TEST_CONSTEXPR auto end() noexcept
    -> typename std::enable_if<
            opts::has_children && std::is_void<_Dummy>::value,
            decltype(std::declval<
                children_container_type&>().end())
        >::type
    {
        return children_base::m_members.end();
    }

    // -----------------------------------------------------------------
    //  event dispatch (enabled by test_opt_events)
    // -----------------------------------------------------------------

    // set_event_handler
    //   attaches a runtime event handler. Does NOT propagate
    // to children; each child manages its own handler.
    template<typename _Dummy = void>
    auto set_event_handler(
        event_handler_type _handler
    ) noexcept
    -> typename std::enable_if<
            opts::has_events && std::is_void<_Dummy>::value,
            void
        >::type
    {
        event_base::m_handler = _handler;

        return;
    }

    // event_handler
    //   returns the currently attached event handler.
    template<typename _Dummy = void>
    auto event_handler() const noexcept
    -> typename std::enable_if<
            opts::has_events && std::is_void<_Dummy>::value,
            event_handler_type
        >::type
    {
        return event_base::m_handler;
    }

private:
    // -----------------------------------------------------------------
    //  internal: event dispatch
    // -----------------------------------------------------------------

    // fire_event
    //   dispatches an event to the handler if events are
    // enabled and a handler is attached. No-op otherwise.
    void fire_event(
            DTestEvent _event
        ) const
    {
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
        if constexpr (opts::has_events)
        {
            if (event_base::m_handler)
            {
                const char* n = nullptr;

                if constexpr (opts::has_named)
                {
                    n = name_base::m_name;
                }

                event_base::m_handler(
                    test_event(_event,
                               static_cast<DTestStatus>(
                                   static_cast<int>(m_status)),
                               n));
            }
        }
#else
        fire_event_dispatch(
            _event,
            std::integral_constant<bool, opts::has_events>{});
#endif

        return;
    }

#if (!D_ENV_LANG_IS_CPP17_OR_HIGHER)
    // fire_event_dispatch (enabled)
    //   dispatches when events are present.
    void fire_event_dispatch(
            DTestEvent     _event,
            std::true_type
        ) const
    {
        if (event_base::m_handler)
        {
            event_base::m_handler(
                test_event(_event,
                           static_cast<DTestStatus>(
                               static_cast<int>(m_status)),
                           nullptr));
        }

        return;
    }

    // fire_event_dispatch (disabled)
    //   no-op when events are absent.
    void fire_event_dispatch(
            DTestEvent,
            std::false_type
        ) const
    {
        return;
    }
#endif

    // -----------------------------------------------------------------
    //  internal: conjunction evaluation (interior nodes)
    // -----------------------------------------------------------------

    // evaluate_conjunction
    //   iterates children and returns false on the first
    // child that converts to false. Used by operator bool()
    // for interior nodes.
    D_TEST_CONSTEXPR bool evaluate_conjunction() const noexcept
    {
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
        if constexpr (opts::has_children)
        {
            for (const auto& child : children_base::m_members)
            {
                if (!static_cast<bool>(child))
                {
                    return false;
                }
            }
        }
#endif

        return true;
    }

    // derive_group_status
    //   derives the aggregate status from children.
    // - all passed (0)                    => 0 (passed)
    // - any failed (1) or error (4)       => 1 (failed)
    // - any pending (3) and none failed   => 3 (pending)
    // - all skipped (2)                   => 2 (skipped)
    // - empty group                       => 0 (passed)
    D_TEST_CONSTEXPR status_type
    derive_group_status() const noexcept
    {
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
        if constexpr (opts::has_children)
        {
            bool any_pending = false;
            bool all_skipped = true;

            for (const auto& child : children_base::m_members)
            {
                auto s = static_cast<int>(child.status());

                // failed (1) or error (4)
                if ( (s == 1) ||
                     (s == 4) )
                {
                    return static_cast<_StatusType>(1);
                }

                if (s == 3)
                {
                    any_pending = true;
                    all_skipped = false;
                }
                else if (s == 0)
                {
                    all_skipped = false;
                }
            }

            if (any_pending)
            {
                return static_cast<_StatusType>(3);
            }

            // non-empty and all skipped
            if (all_skipped &&
                (children_base::m_members.begin() !=
                 children_base::m_members.end()))
            {
                return static_cast<_StatusType>(2);
            }
        }
#endif

        return static_cast<_StatusType>(0);
    }

    _Type       m_result;
    _StatusType m_status;
};


///////////////////////////////////////////////////////////////////////////////
///                III. CONVENIENCE TYPE ALIASES                            ///
///////////////////////////////////////////////////////////////////////////////

// basic_test
//   type: minimal leaf test — bool result, default status,
// no options.
using basic_test = test_object<>;

// named_test
//   type: leaf test with naming support.
using named_test = test_object<bool,
                               DTestStatus,
                               test_opt_named>;

// full_test
//   type: leaf test with naming, messaging, skip, and events.
using full_test = test_object<bool,
                              DTestStatus,
                              test_opt_named,
                              test_opt_message,
                              test_opt_skippable,
                              test_opt_events<>>;


///////////////////////////////////////////////////////////////////////////////
///                IV.  FACTORY FUNCTIONS                                   ///
///////////////////////////////////////////////////////////////////////////////

// make_test
//   function: constructs a basic_test from a boolean result.
D_CONSTEXPR_INLINE basic_test
make_test(
        bool _result
    ) noexcept
{
    return basic_test(_result);
}

// make_named_test
//   function: constructs a named_test from a boolean result
// and a name string.
D_CONSTEXPR_INLINE named_test
make_named_test(
        bool        _result,
        const char* _name
    ) noexcept
{
    named_test t(_result);
    t.set_name(_name);

    return t;
}

// make_test_group
//   function: constructs an interior test_object from a
// container of children. The _Container type is deduced.
template<typename _Container>
D_CONSTEXPR_INLINE auto
make_test_group(
        _Container&& _children
    )
    -> test_object<bool,
                   DTestStatus,
                   test_opt_named,
                   test_opt_children<
                       typename std::decay<_Container>::type>>
{
    using group_type = test_object<
        bool,
        DTestStatus,
        test_opt_named,
        test_opt_children<
            typename std::decay<_Container>::type>>;

    group_type g;
    g.children() = static_cast<_Container&&>(_children);

    return g;
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_OBJECT_
