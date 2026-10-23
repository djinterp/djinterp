/*******************************************************************************
* djinterp [net]                                                      client.hpp
*
*   Foundational client-side module for the net subframework. A client is, at
* its core, a factory that produces a live connection to an endpoint. This
* header defines that contract as the `connector` concept, together with the
* connect conveniences and a robust retry-with-backoff driver, and a thin
* `client` facade that pairs a connector with a default retry policy.
*
* CONTENTS (all in namespace djinterp::net):
*   I.   connector concept (C++20)
*   II.  connect() conveniences
*   III. is_retryable + retry_policy + connect_with_retry()
*   IV.  client<Connector> facade
*
*   A concrete backend (e.g. a POSIX TCP connector) supplies
* connect(endpoint, open_result&), which fills the result in place at every
* level, and from C++11 connect(endpoint) -> open_result as well; everything
* here is generic over the first, and each convenience has both forms too.
* From C++20 the `connector` concept states the contract and constrains the
* templates. The foundation is synchronous/blocking -- an async connector is
* a later variant, not a foundation-level fork.
*
*   Requires:  net.hpp; C++98 and later. The retry driver sleeps through
*              dmutex.h's d_thread_sleep: link dmutex.c.
*
*
* path:      /inc/djinterp/net/client.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.10.08
*******************************************************************************/

#ifndef DJINTERP_NET_CLIENT_HPP
#define DJINTERP_NET_CLIENT_HPP 1

// std
#include <ctime>   // timespec
#include <string>  // std::string
// djinterp
#include "../c/sync/dmutex.h"  // d_thread_sleep
#include "./net_common.hpp"    // connection, open_result, endpoint
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // std
    #include <concepts>  // std::same_as
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_DJINTERP
NS_NET


///////////////////////////////////////////////////////////////////////////////
///                     I.   CONNECTOR CONCEPT                             ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// connector
//   concept: anything that can produce a connection to an endpoint -- a client
// transport backend. connect(endpoint, open_result&) fills the result in
// place (an owned connection, or the error that prevented it) and returns
// the error. tcp_connector, tls_connector and an in-process connector model
// it. C++20 and later; below, the templates take any type with that member.
template<typename Type>
concept connector = requires(Type&           _connector,
                             const endpoint& _endpoint,
                             open_result&    _out)
{
    { _connector.connect(_endpoint,
                         _out) } -> std::same_as<io_error::value>;
};

// D_INTERNAL_NET_REQUIRES_CONNECTOR
//   macro: constrains a template parameter to `connector` from C++20, and
// leaves it unconstrained below, where concepts cannot be spelled.
#define D_INTERNAL_NET_REQUIRES_CONNECTOR(Type) requires connector<Type>
#else
#define D_INTERNAL_NET_REQUIRES_CONNECTOR(Type)
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///                    II.   CONNECT CONVENIENCES                          ///
///////////////////////////////////////////////////////////////////////////////

// connect
//   function: connects to a host and port without building an endpoint by
// hand, filling `_out` in place; returns the error it records.
template<typename Connector>
D_INTERNAL_NET_REQUIRES_CONNECTOR(Connector)
io_error::value
connect(
    Connector&         _connector,
    const std::string& _host,
    port_type          _port,
    open_result&       _out,
    protocol::value    _proto = protocol::tcp
)
{
    return _connector.connect(endpoint(_host,
                                       _port,
                                       _proto),
                              _out);
}

#if (D_MOVE_ENABLED == 1)
// connect
//   function: the same, returning the result, which C++11 can move.
template<typename Connector>
D_INTERNAL_NET_REQUIRES_CONNECTOR(Connector)
D_NODISCARD open_result
connect(
    Connector&         _connector,
    const std::string& _host,
    port_type          _port,
    protocol::value    _proto = protocol::tcp
)
{
    open_result result;

    (void)connect(_connector,
                  _host,
                  _port,
                  result,
                  _proto);

    return result;
}
#endif  // D_MOVE_ENABLED


///////////////////////////////////////////////////////////////////////////////
///            III.   RETRY POLICY + CONNECT-WITH-RETRY                    ///
///////////////////////////////////////////////////////////////////////////////

// is_retryable
//   function: whether a connect failure is the kind worth retrying (a transient
// transport condition) as opposed to a permanent one (bad argument, permission,
// already closed). Used by connect_with_retry to avoid hammering on hopeless
// errors.
D_NODISCARD inline bool
is_retryable(
    io_error::value _error
)
{
    switch (_error)
    {
        case io_error::timed_out:
        case io_error::connection_refused:
        case io_error::connection_reset:
        case io_error::connection_aborted:
        case io_error::host_unreachable:
        case io_error::network_down:
        case io_error::interrupted:
            return true;

        default:
            return false;
    }
}

// retry_policy
//   struct: controls connect_with_retry. `max_attempts` is the total number of
// tries (>= 1; 0 is treated as 1). Between failed attempts the driver sleeps
// `initial_delay_ms`, growing it by `backoff_multiplier` each round and capping
// it at `max_delay_ms` (0 = uncapped). The default performs a SINGLE attempt --
// opt into retries explicitly.
struct retry_policy
{
    unsigned max_attempts;
    long     initial_delay_ms;
    double   backoff_multiplier;
    long     max_delay_ms;

    retry_policy()
        : max_attempts(1),
          initial_delay_ms(100),
          backoff_multiplier(2.0),
          max_delay_ms(5000)
    {
    }

    retry_policy(
        unsigned _max_attempts,
        long     _initial_delay_ms,
        double   _backoff_multiplier,
        long     _max_delay_ms
    )
        : max_attempts(_max_attempts),
          initial_delay_ms(_initial_delay_ms),
          backoff_multiplier(_backoff_multiplier),
          max_delay_ms(_max_delay_ms)
    {
    }
};

NS_INTERNAL

    // pause_ms
    //   function: sleeps the calling thread, through dmutex.h, at every
    // level (std::this_thread is C++11's).
    inline void
    pause_ms(
        long _milliseconds
    )
    {
        struct timespec span;

        span.tv_sec  = static_cast<time_t>(_milliseconds / 1000L);
        span.tv_nsec = (_milliseconds % 1000L) * 1000000L;
        (void)::d_thread_sleep(&span,
                               D_NULLPTR);

        return;
    }

    // next_delay
    //   function: the delay after `_delay`, grown by the policy's multiplier
    // and capped by its ceiling.
    inline long
    next_delay(
        long                _delay,
        const retry_policy& _policy
    )
    {
        const double next  = static_cast<double>(_delay) *
                             _policy.backoff_multiplier;
        long         grown = (next >= 2147483647.0) ? 2147483647L
                                                    : static_cast<long>(next);

        // apply the optional ceiling
        if ( (_policy.max_delay_ms > 0) &&
             (grown > _policy.max_delay_ms) )
        {
            grown = _policy.max_delay_ms;
        }

        return grown;
    }

NS_END  // internal

// connect_with_retry
//   function: connects to `_endpoint` using `_connector`, retrying transient
// failures per `_policy` with exponential backoff, into `_out`: the first
// success, or the last failure once attempts are exhausted or a
// non-retryable error occurs. Returns the error `_out` records.
template<typename Connector>
D_INTERNAL_NET_REQUIRES_CONNECTOR(Connector)
io_error::value
connect_with_retry(
    Connector&          _connector,
    const endpoint&     _endpoint,
    open_result&        _out,
    const retry_policy& _policy = retry_policy()
)
{
    const unsigned attempts =
        (_policy.max_attempts == 0) ? 1u
                                    : _policy.max_attempts;
    long           delay    = _policy.initial_delay_ms;

    // try, backing off between transient failures
    for (unsigned i = 0; i < attempts; ++i)
    {
        const io_error::value tried = _connector.connect(_endpoint,
                                                         _out);

        // a connection, or a failure not worth retrying, ends it
        if ( (tried == io_error::none) ||
             (!is_retryable(tried)) )
        {
            return tried;
        }

        // sleep and grow the delay before the next attempt (if any)
        if ((i + 1) < attempts)
        {
            // a positive delay is slept
            if (delay > 0)
            {
                internal::pause_ms(delay);
            }

            delay = internal::next_delay(delay,
                                         _policy);
        }
    }

    return _out.error;
}

#if (D_MOVE_ENABLED == 1)
// connect_with_retry
//   function: the same, returning the result, which C++11 can move.
template<typename Connector>
D_INTERNAL_NET_REQUIRES_CONNECTOR(Connector)
D_NODISCARD open_result
connect_with_retry(
    Connector&          _connector,
    const endpoint&     _endpoint,
    const retry_policy& _policy = retry_policy()
)
{
    open_result result;

    (void)connect_with_retry(_connector,
                             _endpoint,
                             result,
                             _policy);

    return result;
}
#endif  // D_MOVE_ENABLED


///////////////////////////////////////////////////////////////////////////////
///                        IV.   CLIENT FACADE                             ///
///////////////////////////////////////////////////////////////////////////////

// client
//   class: a thin facade pairing a connector backend with a default retry
// policy, so callers can just say connect(host, port). Owns its connector by
// value; the backend type is fixed at compile time (static dispatch). For
// runtime transport selection, instantiate with a connector that itself
// type-erases, or hold connections through the abstract connection interface.
// A connector that cannot be copied (tls_connector) is default-constructed
// and configured through backend(), or from C++11 moved in.
template<typename Connector>
D_INTERNAL_NET_REQUIRES_CONNECTOR(Connector)
class client
{
public:
    client()
        : m_connector(),
          m_retry()
    {
    }

    explicit client(
        const Connector& _connector
    )
        : m_connector(_connector),
          m_retry()
    {
    }

#if (D_MOVE_ENABLED == 1)
    explicit client(
        Connector&& _connector
    )
        : m_connector(static_cast<Connector&&>(_connector)),
          m_retry()
    {
    }
#endif  // D_MOVE_ENABLED

    // set_retry
    //   function: sets the default retry policy. Returns *this for chaining.
    client&
    set_retry(
        const retry_policy& _policy
    )
    {
        m_retry = _policy;

        return *this;
    }

    // retry
    //   function: the current default retry policy.
    D_NODISCARD const retry_policy&
    retry() const
    {
        return m_retry;
    }

    // backend
    //   function: access to the underlying connector.
    D_NODISCARD Connector&
    backend()
    {
        return m_connector;
    }

    // connect (endpoint)
    //   function: connects to `_endpoint` using the default retry policy,
    // into `_out`.
    io_error::value
    connect(
        const endpoint& _endpoint,
        open_result&    _out
    )
    {
        return connect_with_retry(m_connector,
                                  _endpoint,
                                  _out,
                                  m_retry);
    }

    // connect (host / port)
    //   function: connects to a host and port using the default retry policy,
    // into `_out`.
    io_error::value
    connect(
        const std::string& _host,
        port_type          _port,
        open_result&       _out,
        protocol::value    _proto = protocol::tcp
    )
    {
        return connect(endpoint(_host,
                                _port,
                                _proto),
                       _out);
    }

#if (D_MOVE_ENABLED == 1)
    // connect -- the same two, returning the result, which C++11 can move
    D_NODISCARD open_result
    connect(
        const endpoint& _endpoint
    )
    {
        open_result result;

        (void)connect(_endpoint,
                      result);

        return result;
    }

    D_NODISCARD open_result
    connect(
        const std::string& _host,
        port_type          _port,
        protocol::value    _proto = protocol::tcp
    )
    {
        return connect(endpoint(_host,
                                _port,
                                _proto));
    }
#endif  // D_MOVE_ENABLED

private:
    Connector    m_connector;
    retry_policy m_retry;
};


NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_CLIENT_HPP
