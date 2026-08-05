/******************************************************************************
* djinterp [net]                                                    client.hpp
*
*   Foundational client-side module for the net subframework. A client is, at
* its core, a factory that produces a live connection to an endpoint. This
* header defines that contract as the `connector` concept, together with the
* connect conveniences and a robust retry-with-backoff driver, and a thin
* `client` facade that pairs a connector with a default retry policy.
*
* CONTENTS (all in namespace djinterp::net):
*   I.   connector concept
*   II.  connect() conveniences
*   III. is_retryable + retry_policy + connect_with_retry()
*   IV.  client<Connector> facade
*
*   A concrete backend (e.g. a POSIX TCP connector) supplies connect(endpoint)
* -> open_result and thereby models `connector`; everything here is generic
* over that. The foundation is synchronous/blocking -- an async connector is a
* later variant, not a foundation-level fork.
*
*   Requires:  net.hpp (C++20).
*
* path:      /inc/djinterp/net/client.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.17
******************************************************************************/

#ifndef DJINTERP_NET_CLIENT_
#define DJINTERP_NET_CLIENT_ 1

// std
#include <chrono>
#include <string>
#include <thread>
#include <utility>
// djinterp
#include "net.hpp"


NS_DJINTERP
NS_NET


///////////////////////////////////////////////////////////////////////////////
///                     I.   CONNECTOR CONCEPT                             ///
///////////////////////////////////////////////////////////////////////////////

// connector
//   concept: anything that can produce a connection to an endpoint -- a client
// transport backend. connect() yields an open_result (an owned connection or
// the error that prevented it). A backend's concrete tcp_connector /
// tls_connector / in-process connector models this.
template<typename _Type>
concept connector = requires(_Type& _connector, const endpoint& _endpoint)
{
    { _connector.connect(_endpoint) } -> std::same_as<open_result>;
};


///////////////////////////////////////////////////////////////////////////////
///                    II.   CONNECT CONVENIENCES                          ///
///////////////////////////////////////////////////////////////////////////////

// connect
//   function: connects to a host and port without building an endpoint by hand.
template<connector _Connector>
D_NODISCARD open_result
connect(
    _Connector&        _connector,
    const std::string& _host,
    port_type          _port,
    protocol           _proto = protocol::tcp
)
{
    return _connector.connect(endpoint(_host, _port, _proto));
}


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
    io_error _error
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

// connect_with_retry
//   function: connects to `_endpoint` using `_connector`, retrying transient
// failures per `_policy` with exponential backoff. Returns the first successful
// open_result, or the last failure once attempts are exhausted or a
// non-retryable error occurs.
template<connector _Connector>
D_NODISCARD open_result
connect_with_retry(
    _Connector&         _connector,
    const endpoint&     _endpoint,
    const retry_policy& _policy = retry_policy()
)
{
    const unsigned attempts = (_policy.max_attempts == 0) ? 1u
                                                          : _policy.max_attempts;
    long        delay = _policy.initial_delay_ms;
    open_result result;

    // try, backing off between transient failures
    for (unsigned i = 0; i < attempts; ++i)
    {
        result = _connector.connect(_endpoint);

        if (result.ok())
        {
            return result;
        }

        // permanent failures are not worth retrying
        if (!is_retryable(result.error))
        {
            return result;
        }

        // sleep and grow the delay before the next attempt (if any)
        if ((i + 1) < attempts)
        {
            if (delay > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }

            const double next = static_cast<double>(delay) *
                                _policy.backoff_multiplier;

            long grown = (next >= 2147483647.0) ? 2147483647L
                                                : static_cast<long>(next);

            // apply the optional ceiling
            if ( (_policy.max_delay_ms > 0) &&
                 (grown > _policy.max_delay_ms) )
            {
                grown = _policy.max_delay_ms;
            }

            delay = grown;
        }
    }

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///                        IV.   CLIENT FACADE                             ///
///////////////////////////////////////////////////////////////////////////////

// client
//   class: a thin facade pairing a connector backend with a default retry
// policy, so callers can just say connect(host, port). Owns its connector by
// value; the backend type is fixed at compile time (static dispatch). For
// runtime transport selection, instantiate with a connector that itself
// type-erases, or hold connections through the abstract connection interface.
template<connector _Connector>
class client
{
public:
    explicit client(
        _Connector _connector
    )
        : m_connector(std::move(_connector)),
          m_retry()
    {
    }

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
    D_NODISCARD _Connector&
    backend()
    {
        return m_connector;
    }

    // connect (endpoint)
    //   function: connects to `_endpoint` using the default retry policy.
    D_NODISCARD open_result
    connect(
        const endpoint& _endpoint
    )
    {
        return connect_with_retry(m_connector, _endpoint, m_retry);
    }

    // connect (host / port)
    //   function: connects to a host and port using the default retry policy.
    D_NODISCARD open_result
    connect(
        const std::string& _host,
        port_type          _port,
        protocol           _proto = protocol::tcp
    )
    {
        return connect(endpoint(_host, _port, _proto));
    }

private:
    _Connector   m_connector;
    retry_policy m_retry;
};


NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_CLIENT_
