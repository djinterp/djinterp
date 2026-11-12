/*******************************************************************************
* djinterp [cpp][test]                                    test_counter_event.hpp
*
* The event tie-in for DTest's counters on the C++ face: handlers that record
* an occurrence into a tally key, or move a test counter by its step, each time
* they are called. It couples an event handler to test_counter.hpp and does
* nothing else: the caller binds the handlers wherever handlers are bound, so
* this file includes no event header. Neither test_counter.hpp nor DTest's
* runners include it.
*   The tally key and the event are separate choices; a suite that likes may
* use the event's tag as the key.
*   C's counterpart is c/modules/test/test_counter_event.h.
*
* ONE TEST COUNTER
* ----------------
*   A test counter is a counter, so its handler is
* cpp/util/counter/counter_event.hpp's counter_handler, made by
* make_test_counter_handler with the counter's step.
*
* THE CALLER OWNS THE TALLY AND THE COUNTER
* -----------------------------------------
*   A handler keeps a pointer to what it moves, so that must outlive every
* handler bound to move it, or they are unbound first.
*
* PORTABILITY
* -----------
*   Requires C++17 or later, as test_counter.hpp does; below that this file
* is empty.
*
* HOW TO USE
* ----------
*   Bind a tally key, or a test counter, to an event:
*
*     D_EVENT(on_retry, int);
*     struct retried;
*     DEventDispatcher d;
*     test_tally<>     tally;
*
*     d.bind<on_retry>(make_tally_handler<retried>(tally));
*     d.fire<on_retry>(3);                // tally.count<retried>() == 1
*
* OPTIONS
* -------
*   Which tally and key a handler records; a test counter's handler takes the
* counter's step, or the operation given.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/cpp/modules/test/test_counter_event.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  A TALLY KEY
    -----------
    1.  tally_handler
    2.  make_tally_handler
2.  A TEST COUNTER
    --------------
    1.  make_test_counter_handler
*/

#ifndef DJINTERP_TEST_COUNTER_EVENT_HPP
#define DJINTERP_TEST_COUNTER_EVENT_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, as test_counter.hpp is.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../djinterp.hpp"                    // framework root
#include "../../util/counter/counter_event.hpp"  // counter_handler
#include "./test_counter.hpp"                    // test_tally, test_counter


NS_DJINTERP
NS_TEST


//==============================================================================
// 1.  A TALLY KEY
//==============================================================================


// 1.1    tally_handler
//------------------------------------------------------------------------------
// tally_handler
//   class: an event handler that records one occurrence of `Key` in a tally
// per call. It ignores the occurrence's payload and returns no verdict, so it
// never stops an event reaching the handlers bound after it.
//
// Example:
//   d.bind<DOnTestFailed>(tally_handler<failed, test_tally<>>(tally));
template<typename Key,
         typename Tally>
class tally_handler
{
public:
    using key_type   = Key;
    using tally_type = Tally;

    /**
     * @brief Aims a handler at one key of a tally.
     *
     * @param[in,out] _tally  the tally; must outlive the handler.
     */
    explicit tally_handler(
        Tally& _tally
    ) noexcept
        : m_tally(&_tally)
    {
    }

    /**
     * @brief Records one occurrence of `Key`.
     *
     * @tparam Payload  the occurrence's payload types; ignored.
     */
    template<typename... Payload>
    void operator()(
        const Payload&...
    ) const
    {
        (void)m_tally->template record<Key>();

        return;
    }

    /**
     * @brief Returns the tally the handler records into.
     *
     * @return the tally.
     */
    Tally& target() const noexcept
    {
        return *m_tally;
    }

private:
    Tally* m_tally;  // the caller's
};


// 1.2    make_tally_handler
//------------------------------------------------------------------------------
/**
 * @brief Makes the handler that records one occurrence of `Key` in `_tally`
 *        per call, to bind to an event.
 *
 * @tparam Key    the key type.
 * @tparam Tally  the tally's type, a test_tally; deduced.
 * @param[in,out] _tally  the tally; must outlive the handler.
 * @return the handler.
 */
template<typename Key,
         typename Tally>
tally_handler<Key, Tally> make_tally_handler(
    Tally& _tally
) noexcept
{
    return tally_handler<Key, Tally>(_tally);
}


//==============================================================================
// 2.  A TEST COUNTER
//==============================================================================


// 2.1    make_test_counter_handler
//------------------------------------------------------------------------------
/**
 * @brief Makes counter_event.hpp's handler for a test counter: each call
 *        applies `_op`, by the counter's step.
 *
 * @note The step is read now; a counter given another step later needs a new
 *       handler.
 *
 * @tparam ValueType  the counter's value type; deduced.
 * @param[in,out] _counter  the counter; must outlive the handler.
 * @param[in]     _op       what each call does; increment by default.
 * @return the handler.
 */
template<typename ValueType>
counter_handler<test_counter<ValueType>> make_test_counter_handler(
    test_counter<ValueType>& _counter,
    DCounterEventOp          _op = DCounterEventOp::increment
) noexcept
{
    return counter_handler<test_counter<ValueType>>(_counter,
                                                    _op,
                                                    _counter.step());
}


NS_END  // test
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_TEST_COUNTER_EVENT_HPP
