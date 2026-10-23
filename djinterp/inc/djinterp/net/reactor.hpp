/*******************************************************************************
* djinterp [net]                                                     reactor.hpp
*
*   Non-blocking, event-driven backend for the net subframework, built on the
* readiness mechanism env_net.h selects (epoll on Linux). Where the blocking
* foundation gives each connection a thread, the reactor multiplexes many
* connections on one thread via readiness notification -- the C10k model the
* would_block vocabulary and the readiness-backend detection were laid down for.
*
* CONTENTS (namespace djinterp::net):
*   internal:         non-blocking fd helper, epoll<->event_mask conversion
*   event_mask        readiness/condition flags (readable/writable/error/hangup)
*   reactor           an epoll event loop: register fds with interest +
*                     callback, wait, dispatch; thread-safe stop() via eventfd.
*                     Reentrancy-safe -- a callback may add/modify/remove fds,
*                     including removing its own (erase is deferred past
*                     dispatch)
*   event_connection  a non-blocking connection with an outbound buffer:
*                     send() flushes what it can and queues the rest; the
*                     server drains it on write-readiness (backpressure without
*                     blocking)
*   event_server      accepts connections and dispatches on_connect / on_data /
*                     on_close callbacks from a single-threaded reactor loop
*
*   The blocking backends remain the default; this is a distinct concurrency
* model (non-blocking, callback-driven) rather than an execution_policy, since a
* reactor cannot run to-completion blocking handlers. It reuses
* socket_connection from tcp.hpp -- already non-blocking-correct (EAGAIN ->
* would_block) -- and only needs the descriptor made non-blocking and
* registered.
*
*   Requires:  net.hpp, tcp.hpp, env_net.h, and epoll (D_ENV_NET_HAS_EPOLL). A
*              kqueue backend would be a sibling of the same shape; without
*              epoll the header is empty. C++98 and later: callbacks are
*              re_std::function, the run flags sit under a lock of dmutex.h's
*              (link dmutex.c), the registrations are in a std::map, and the
*              outbound queue is a string with a read offset. event_mask is
*              scoped in a struct (decision 3.4): event_mask::value. The
*              owner's decision 4 (2026.09.28) rebuilds the poller as a
*              portable one in C, with this reactor as its C++ layer.
*
*   Where D_ENV_NET_HAS_EPOLL is 0, with no epoll, it compiles to nothing,
* unless D_CFG_NET_REQUIRE_BACKENDS asks for an #error (decision 101 of the
* register).
*
*
* path:      /inc/djinterp/net/reactor.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.10.16
*******************************************************************************/

#ifndef DJINTERP_NET_REACTOR_HPP
#define DJINTERP_NET_REACTOR_HPP 1

// djinterp
#include "../env/net/env_net.h"  // D_ENV_NET_HAS_EPOLL
#include "./net_common.hpp"      // connection, io_error, endpoint
#include "./tcp/tcp.hpp"         // socket_connection, tcp_acceptor
#include "../config/net/cfg_net.h"  // D_INTERNAL_NET_REQUIRE_BACKENDS


// epoll is the readiness mechanism; without it there is nothing
#if D_ENV_NET_HAS_EPOLL

// std
#include <cerrno>   // errno, EINTR, EAGAIN, EWOULDBLOCK, ECONNABORTED
#include <cstddef>  // std::size_t
#include <cstring>  // std::memset
#include <map>      // std::map
#include <new>      // std::nothrow
#include <string>   // std::string
#include <vector>   // std::vector
// posix
#include <fcntl.h>        // fcntl, O_NONBLOCK
#include <sys/epoll.h>    // epoll_create1, epoll_ctl, epoll_wait
#include <sys/eventfd.h>  // eventfd
#include <sys/socket.h>   // accept
#include <unistd.h>       // close, read, write
// djinterp
#include "../c/sync/dmutex.h"  // d_mutex_t, for the run flags
#if (D_MOVE_ENABLED == 1)
    // std
    #include <utility>  // std::move
#endif  // D_MOVE_ENABLED
// re_std
#include "../../re_std/cstdint/cstdint.hpp"       // uint32_t, uint64_t
#include "../../re_std/functional/function.hpp"  // re_std::function


NS_DJINTERP
NS_NET


///////////////////////////////////////////////////////////////////////////////
///                            EVENT MASK                                  ///
///////////////////////////////////////////////////////////////////////////////

// event_mask
//   enum: the readiness conditions a reactor reports and a caller registers
// interest in. readable/writable are requested and reported; error/hangup are
// always reported by the poller and need not be requested. Scoped in a struct
// (decision 3.4): the type is event_mask::value.
struct event_mask
{
    enum value
    {
        none     = 0u,
        readable = 1u << 0,
        writable = 1u << 1,
        error    = 1u << 2,
        hangup   = 1u << 3
    };
};

D_NODISCARD inline event_mask::value
operator|(
    event_mask::value _a,
    event_mask::value _b
)
{
    return static_cast<event_mask::value>(static_cast<unsigned int>(_a) |
                                          static_cast<unsigned int>(_b));
}

D_NODISCARD inline event_mask::value
operator&(
    event_mask::value _a,
    event_mask::value _b
)
{
    return static_cast<event_mask::value>(static_cast<unsigned int>(_a) &
                                          static_cast<unsigned int>(_b));
}

inline event_mask::value&
operator|=(
    event_mask::value& _a,
    event_mask::value  _b
)
{
    _a = _a | _b;

    return _a;
}

// has_event
//   function: whether `_flag` is set in `_mask`.
D_NODISCARD inline bool
has_event(
    event_mask::value _mask,
    event_mask::value _flag
)
{
    return (static_cast<unsigned int>(_mask & _flag) != 0u);
}

// any_event
//   function: whether any condition is set.
D_NODISCARD inline bool
any_event(
    event_mask::value _mask
)
{
    return (static_cast<unsigned int>(_mask) != 0u);
}


///////////////////////////////////////////////////////////////////////////////
///                        INTERNAL HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // set_nonblocking
    //   function: sets O_NONBLOCK on a descriptor.
    D_NODISCARD inline io_error::value
    set_nonblocking(
        int _fd
    )
    {
        const int flags = ::fcntl(_fd,
                                  F_GETFL,
                                  0);

        // the flags cannot be read
        if (flags < 0)
        {
            return from_errno(errno);
        }

        // the flags cannot be written
        if (::fcntl(_fd,
                    F_SETFL,
                    flags | O_NONBLOCK) < 0)
        {
            return from_errno(errno);
        }

        return io_error::none;
    }

    // epoll_interest
    //   function: the epoll event bits to request for an interest set. Peer
    // hangup (EPOLLRDHUP) is requested when available so half-close is seen
    // promptly.
    D_NODISCARD inline uint32_t
    epoll_interest(
        event_mask::value _mask
    )
    {
        uint32_t bits = 0;

        // reading
        if (has_event(_mask,
                      event_mask::readable))
        {
            bits |= EPOLLIN;
        }

        // writing
        if (has_event(_mask,
                      event_mask::writable))
        {
            bits |= EPOLLOUT;
        }

    #ifdef EPOLLRDHUP
        bits |= EPOLLRDHUP;
    #endif  // EPOLLRDHUP

        return bits;
    }

    // epoll_to_event
    //   function: converts reported epoll bits to an event_mask.
    D_NODISCARD inline event_mask::value
    epoll_to_event(
        uint32_t _bits
    )
    {
        event_mask::value mask = event_mask::none;

        // readable
        if (_bits & EPOLLIN)
        {
            mask |= event_mask::readable;
        }

        // writable
        if (_bits & EPOLLOUT)
        {
            mask |= event_mask::writable;
        }

        // an error on the descriptor
        if (_bits & EPOLLERR)
        {
            mask |= event_mask::error;
        }

    #ifdef EPOLLRDHUP
        const uint32_t hung = EPOLLHUP | EPOLLRDHUP;
    #else
        const uint32_t hung = EPOLLHUP;
    #endif  // EPOLLRDHUP

        // the peer hung up, or half-closed
        if (_bits & hung)
        {
            mask |= event_mask::hangup;
        }

        return mask;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                             REACTOR                                    ///
///////////////////////////////////////////////////////////////////////////////

// reactor
//   class: an epoll-based event loop. Descriptors are registered with an
// interest set and a callback; run()/run_once() wait and dispatch the callbacks
// for ready descriptors. A single reactor instance is driven from one thread;
// stop() (and the internal wake) are thread-safe. Callbacks may add, modify, or
// remove registrations during dispatch -- including removing their own fd --
// because entry erasure is deferred until the current dispatch completes, so a
// callback is never destroyed while it runs. Non-copyable, non-movable.
class reactor
{
public:
    typedef re_std::function<void(int,
                                  event_mask::value)> callback;

    reactor()
        : m_epoll(-1),
          m_wake(-1),
          m_entries(),
          m_events(),
          m_deferred(),
          m_stop(false),
          m_running(false),
          m_dispatching(false)
    {
        (void)::d_mutex_init(&m_flags);
        m_epoll = ::epoll_create1(EPOLL_CLOEXEC);

        // no epoll instance, no reactor
        if (m_epoll < 0)
        {
            return;
        }

        m_wake = ::eventfd(0,
                           EFD_CLOEXEC | EFD_NONBLOCK);

        // no wake descriptor: the reactor gives up its epoll instance
        if (m_wake < 0)
        {
            (void)::close(m_epoll);
            m_epoll = -1;

            return;
        }

        // register the wake eventfd directly (kept out of the entry map)
        struct epoll_event ev;

        std::memset(&ev,
                    0,
                    sizeof(ev));
        ev.events  = EPOLLIN;
        ev.data.fd = m_wake;

        // a wake descriptor epoll will not take is no wake at all
        if (::epoll_ctl(m_epoll,
                        EPOLL_CTL_ADD,
                        m_wake,
                        &ev) < 0)
        {
            (void)::close(m_wake);
            (void)::close(m_epoll);
            m_wake  = -1;
            m_epoll = -1;

            return;
        }
    }

    ~reactor()
    {
        // the wake descriptor, if made
        if (m_wake >= 0)
        {
            (void)::close(m_wake);
            m_wake = -1;
        }

        // the epoll instance, if made
        if (m_epoll >= 0)
        {
            (void)::close(m_epoll);
            m_epoll = -1;
        }

        (void)::d_mutex_destroy(&m_flags);
    }

    // is_valid
    //   function: whether the epoll instance was created.
    D_NODISCARD bool
    is_valid() const
    {
        return (m_epoll >= 0);
    }

    // add
    //   function: registers a descriptor with an interest set and callback.
    D_NODISCARD io_error::value
    add(
        int               _fd,
        event_mask::value _interest,
        const callback&   _cb
    )
    {
        // a reactor without epoll, or no descriptor
        if ( (m_epoll < 0) ||
             (_fd < 0) )
        {
            return io_error::invalid_argument;
        }

        struct epoll_event ev;

        std::memset(&ev,
                    0,
                    sizeof(ev));
        ev.events  = internal::epoll_interest(_interest);
        ev.data.fd = _fd;

        // epoll refuses the descriptor
        if (::epoll_ctl(m_epoll,
                        EPOLL_CTL_ADD,
                        _fd,
                        &ev) < 0)
        {
            return internal::from_errno(errno);
        }

        entry& e = m_entries[_fd];

        e.cb       = _cb;
        e.interest = _interest;
        e.dead     = false;

        return io_error::none;
    }

    // modify
    //   function: changes the interest set for a registered descriptor.
    D_NODISCARD io_error::value
    modify(
        int               _fd,
        event_mask::value _interest
    )
    {
        const std::map<int, entry>::iterator it = m_entries.find(_fd);

        // not registered
        if (it == m_entries.end())
        {
            return io_error::invalid_argument;
        }

        // nothing to change
        if (it->second.interest == _interest)
        {
            return io_error::none;
        }

        struct epoll_event ev;

        std::memset(&ev,
                    0,
                    sizeof(ev));
        ev.events  = internal::epoll_interest(_interest);
        ev.data.fd = _fd;

        // epoll refuses the change
        if (::epoll_ctl(m_epoll,
                        EPOLL_CTL_MOD,
                        _fd,
                        &ev) < 0)
        {
            return internal::from_errno(errno);
        }

        it->second.interest = _interest;

        return io_error::none;
    }

    // remove
    //   function: unregisters a descriptor. The epoll deletion happens at once;
    // the entry (and its callback) is erased after the current dispatch when
    // called from within a callback, so a callback may safely remove its own
    // fd.
    io_error::value
    remove(
        int _fd
    )
    {
        const std::map<int, entry>::iterator it = m_entries.find(_fd);

        // not registered
        if (it == m_entries.end())
        {
            return io_error::invalid_argument;
        }

        (void)::epoll_ctl(m_epoll,
                          EPOLL_CTL_DEL,
                          _fd,
                          D_NULLPTR);

        // within a dispatch the entry waits for it to end
        if (m_dispatching)
        {
            it->second.dead = true;
            m_deferred.push_back(_fd);
        }
        else
        {
            m_entries.erase(it);
        }

        return io_error::none;
    }

    // run_once
    //   function: waits up to `_timeout_ms` (-1 blocks) and dispatches ready
    // callbacks once.
    D_NODISCARD io_error::value
    run_once(
        int _timeout_ms
    )
    {
        return dispatch(_timeout_ms);
    }

    // run
    //   function: dispatches until stop(). Returns none on a clean stop, or the
    // first fatal poller error.
    D_NODISCARD io_error::value
    run()
    {
        // no epoll instance to wait on
        if (m_epoll < 0)
        {
            return io_error::invalid_argument;
        }

        set_flags(false,
                  true);

        io_error::value result = io_error::none;

        // until stopped, or a fatal error
        while (!stopping())
        {
            result = dispatch(-1);

            // an interrupted wait is waited again
            if ( (result != io_error::none) &&
                 (result != io_error::interrupted) )
            {
                break;
            }
        }

        set_flags(stopping(),
                  false);

        return (result == io_error::interrupted) ? io_error::none
                                                 : result;
    }

    // stop
    //   function: requests the run loop to exit; wakes a blocked wait. Safe to
    // call from another thread.
    void
    stop()
    {
        (void)::d_mutex_lock(&m_flags);
        m_stop = true;
        (void)::d_mutex_unlock(&m_flags);
        wake();

        return;
    }

    // is_running
    //   function: whether a run loop is active.
    D_NODISCARD bool
    is_running() const
    {
        (void)::d_mutex_lock(&m_flags);

        const bool running = m_running;

        (void)::d_mutex_unlock(&m_flags);

        return running;
    }

    // handle_count
    //   function: the number of registered descriptors (excluding the wake fd).
    D_NODISCARD std::size_t
    handle_count() const
    {
        return m_entries.size();
    }

private:
    D_DELETED_FN(reactor(const reactor&))
    D_DELETED_FN(reactor& operator=(const reactor&))

    struct entry
    {
        callback          cb;
        event_mask::value interest;
        bool              dead;

        entry()
            : cb(),
              interest(event_mask::none),
              dead(false)
        {
        }
    };

    // stopping, set_flags
    //   function: the run flags, read and written under their lock.
    bool
    stopping() const
    {
        (void)::d_mutex_lock(&m_flags);

        const bool stop = m_stop;

        (void)::d_mutex_unlock(&m_flags);

        return stop;
    }

    void
    set_flags(
        bool _stop,
        bool _running
    )
    {
        (void)::d_mutex_lock(&m_flags);
        m_stop    = _stop;
        m_running = _running;
        (void)::d_mutex_unlock(&m_flags);

        return;
    }

    // wake
    //   function: nudges the eventfd so a blocked epoll_wait returns.
    void
    wake()
    {
        // no wake descriptor to nudge
        if (m_wake < 0)
        {
            return;
        }

        const uint64_t one = 1;
        const ssize_t  n   = ::write(m_wake,
                                     &one,
                                     sizeof(one));

        (void)n;

        return;
    }

    // deliver
    //   function: one ready descriptor: the wake descriptor's counter is
    // drained, and any other descriptor still registered gets its callback.
    void
    deliver(
        int      _fd,
        uint32_t _bits
    )
    {
        // the wake descriptor: drain the counter and carry on
        if (_fd == m_wake)
        {
            uint64_t      sink = 0;
            const ssize_t r    = ::read(m_wake,
                                        &sink,
                                        sizeof(sink));

            (void)r;

            return;
        }

        const std::map<int, entry>::iterator it = m_entries.find(_fd);

        // removed by an earlier callback in this same batch
        if ( (it == m_entries.end()) ||
             (it->second.dead) )
        {
            return;
        }

        // the callback may add / modify / remove registrations; a
        // std::map's entries stay where they are while it does
        it->second.cb(_fd,
                      internal::epoll_to_event(_bits));

        return;
    }

    // erase_deferred
    //   function: once a dispatch is done, erases the entries removed during
    // it, unless they were added again.
    void
    erase_deferred()
    {
        for (std::size_t k = 0; k < m_deferred.size(); ++k)
        {
            const std::map<int, entry>::iterator dit =
                m_entries.find(m_deferred[k]);

            // still dead: not re-added during the dispatch
            if ( (dit != m_entries.end()) &&
                 (dit->second.dead) )
            {
                m_entries.erase(dit);
            }
        }

        m_deferred.clear();

        return;
    }

    // dispatch
    //   function: one epoll_wait + callback dispatch, then process deferred
    // removals.
    D_NODISCARD io_error::value
    dispatch(
        int _timeout_ms
    )
    {
        // no epoll instance to wait on
        if (m_epoll < 0)
        {
            return io_error::invalid_argument;
        }

        // size the result buffer to current registrations (plus the wake fd)
        std::size_t want = m_entries.size() + 1;

        // never fewer than sixteen at a time
        if (want < 16)
        {
            want = 16;
        }

        m_events.resize(want);

        const int n = ::epoll_wait(m_epoll,
                                   &m_events[0],
                                   static_cast<int>(m_events.size()),
                                   _timeout_ms);

        // the wait failed: a signal, or worse
        if (n < 0)
        {
            return (errno == EINTR) ? io_error::interrupted
                                    : internal::from_errno(errno);
        }

        m_dispatching = true;

        // each ready descriptor's callback
        for (int i = 0; i < n; ++i)
        {
            const struct epoll_event& event =
                m_events[static_cast<std::size_t>(i)];

            deliver(event.data.fd,
                    event.events);
        }

        m_dispatching = false;

        // now that dispatch is done, erase entries removed during it
        erase_deferred();

        return io_error::none;
    }

    int                             m_epoll;
    int                             m_wake;
    std::map<int, entry>            m_entries;
    std::vector<struct epoll_event> m_events;
    std::vector<int>                m_deferred;
    bool                            m_stop;     // under m_flags
    bool                            m_running;  // under m_flags
    bool                            m_dispatching;
    mutable d_mutex_t               m_flags;
};


///////////////////////////////////////////////////////////////////////////////
///                        EVENT CONNECTION                                ///
///////////////////////////////////////////////////////////////////////////////

// event_connection
//   class: a non-blocking connection managed by an event_server. Wraps a
// socket_connection and an outbound buffer. send() writes as much as the socket
// accepts immediately and queues the remainder; the server flushes the queue on
// write-readiness. close() requests a graceful close once the queue drains. A
// user_data slot carries per-connection state: a pointer the callbacks own
// what it points to (C++98 has no shared_ptr to own it here). Non-copyable;
// held by pointer.
class event_connection
{
public:
    // event_connection -- over a connected descriptor, which it adopts
    explicit event_connection(
        socket_connection::native_handle_type _fd
    )
        : user_data(D_NULLPTR),
          m_socket(_fd,
                   protocol::tcp),
          m_outbound(),
          m_sent(0u),
          m_closing(false)
    {
    }

#if (D_MOVE_ENABLED == 1)
    // event_connection -- over a socket_connection, moved in (C++11)
    explicit event_connection(
        socket_connection&& _socket
    )
        : user_data(D_NULLPTR),
          m_socket(std::move(_socket)),
          m_outbound(),
          m_sent(0u),
          m_closing(false)
    {
    }
#endif  // D_MOVE_ENABLED

    // native_handle
    //   function: the underlying descriptor.
    D_NODISCARD int
    native_handle() const
    {
        return m_socket.native_handle();
    }

    // remote_endpoint / local_endpoint
    D_NODISCARD endpoint
    remote_endpoint() const
    {
        return m_socket.remote_endpoint();
    }

    D_NODISCARD endpoint
    local_endpoint() const
    {
        return m_socket.local_endpoint();
    }

    // send
    //   function: sends `_length` bytes, flushing immediately where possible
    // and queuing the rest. Returns a transport error only on a hard failure;
    // would_block is handled by queuing, not returned.
    D_NODISCARD io_error::value
    send(
        const void* _data,
        std::size_t _length
    )
    {
        // a closing connection takes nothing more
        if (m_closing)
        {
            return io_error::closed;
        }

        // nothing to send
        if (_length == 0)
        {
            return io_error::none;
        }

        const char* const p = static_cast<const char*>(_data);

        // already queuing: preserve order, append behind the pending bytes
        if (wants_write())
        {
            m_outbound.append(p,
                              _length);

            return io_error::none;
        }

        std::size_t off = 0;

        // nothing queued: try to write straight through first
        while (off < _length)
        {
            const io_result r = m_socket.write(p + off,
                                               _length - off);

            // the socket took some, or none: carry on, or stop
            if (r.error == io_error::none)
            {
                // nothing taken and no error: queue the rest
                if (r.count == 0)
                {
                    break;
                }

                off += r.count;
            }
            else if (r.error == io_error::would_block)
            {
                break;
            }
            else
            {
                return r.error;
            }
        }

        // what the socket did not take waits for write-readiness
        if (off < _length)
        {
            m_outbound.assign(p + off,
                              _length - off);
            m_sent = 0u;
        }

        return io_error::none;
    }

    D_NODISCARD io_error::value
    send(
        const std::string& _text
    )
    {
        return send(_text.data(),
                    _text.size());
    }

    // flush_outbound
    //   function: writes as much queued data as the socket accepts. Called by
    // the server on write-readiness.
    D_NODISCARD io_error::value
    flush_outbound()
    {
        // until drained, or the socket is full
        while (wants_write())
        {
            const io_result r = m_socket.write(m_outbound.data() + m_sent,
                                               m_outbound.size() - m_sent);

            // the socket took some, or none: carry on, or stop
            if (r.error == io_error::none)
            {
                // nothing taken and no error: try again on readiness
                if (r.count == 0)
                {
                    break;
                }

                m_sent += r.count;
            }
            else if (r.error == io_error::would_block)
            {
                break;
            }
            else
            {
                return r.error;
            }
        }

        // fully drained: the storage is reclaimed
        if (!wants_write())
        {
            m_outbound.clear();
            m_sent = 0u;
        }
        else if (m_sent > (m_outbound.size() / 2u))
        {
            // mostly sent: the sent prefix goes, so the queue cannot grow
            // without bound behind a slow reader
            m_outbound.erase(0u,
                             m_sent);
            m_sent = 0u;
        }

        return io_error::none;
    }

    // close
    //   function: requests a graceful close (flush the queue, then close).
    void
    close()
    {
        m_closing = true;

        return;
    }

    // is_closing
    //   function: whether a graceful close has been requested.
    D_NODISCARD bool
    is_closing() const
    {
        return m_closing;
    }

    // wants_write
    //   function: whether there is queued data still to send.
    D_NODISCARD bool
    wants_write() const
    {
        return (m_sent < m_outbound.size());
    }

    // socket
    //   function: access to the underlying connection (for reads).
    D_NODISCARD socket_connection&
    socket()
    {
        return m_socket;
    }

    // per-connection user state (set/read by callbacks), borrowed
    void* user_data;

private:
    D_DELETED_FN(event_connection(const event_connection&))
    D_DELETED_FN(event_connection& operator=(const event_connection&))

    socket_connection m_socket;
    std::string       m_outbound;  // queued bytes, from m_sent on
    std::size_t       m_sent;      // how much of m_outbound has gone
    bool              m_closing;
};


///////////////////////////////////////////////////////////////////////////////
///                          EVENT SERVER                                  ///
///////////////////////////////////////////////////////////////////////////////

// event_server
//   class: a single-threaded, event-driven TCP server. It binds a listener,
// registers it with a reactor, accepts connections non-blocking, and drives
// three callbacks: on_connect when a connection is accepted, on_data for each
// chunk read (its bytes are valid only for the call), and on_close when a
// connection ends. From a callback, event_connection::send() queues a reply and
// close() ends the connection gracefully. run() blocks until stop(); stop() is
// thread-safe. Non-copyable, non-movable.
class event_server
{
public:
    struct callbacks
    {
        re_std::function<void(event_connection&)> on_connect;
        re_std::function<void(event_connection&,
                              const byte*,
                              std::size_t)>        on_data;
        re_std::function<void(event_connection&)> on_close;
    };

    event_server()
        : m_reactor(),
          m_acceptor(),
          m_conns(),
          m_callbacks(),
          m_listen_fd(-1)
    {
    }

    ~event_server()
    {
        // the connections still open go with the server
        for (std::map<int, event_connection*>::iterator it = m_conns.begin();
             it != m_conns.end();
             ++it)
        {
            delete it->second;
        }
    }

    // is_valid
    //   function: whether the underlying reactor was created.
    D_NODISCARD bool
    is_valid() const
    {
        return m_reactor.is_valid();
    }

    // bind
    //   function: binds and listens (via the TCP acceptor) and makes the
    // listener non-blocking for the reactor.
    D_NODISCARD io_error::value
    bind(
        const endpoint& _endpoint
    )
    {
        const io_error::value e = m_acceptor.bind(_endpoint);

        // the acceptor would not bind
        if (e != io_error::none)
        {
            return e;
        }

        m_listen_fd = m_acceptor.native_handle();

        // no descriptor to listen on
        if (m_listen_fd < 0)
        {
            return io_error::unknown;
        }

        return internal::set_nonblocking(m_listen_fd);
    }

    // local_endpoint
    //   function: the bound address (resolves an ephemeral port).
    D_NODISCARD endpoint
    local_endpoint() const
    {
        return m_acceptor.local_endpoint();
    }

    // run
    //   function: registers the listener and runs the reactor loop until
    // stop().
    D_NODISCARD io_error::value
    run(
        const callbacks& _callbacks
    )
    {
        // no reactor, or nothing bound
        if ( (!m_reactor.is_valid()) ||
             (m_listen_fd < 0) )
        {
            return io_error::invalid_argument;
        }

        m_callbacks = _callbacks;

        const io_error::value e =
            m_reactor.add(m_listen_fd,
                          event_mask::readable,
                          reactor::callback(listener_ready(this)));

        // the listener would not register
        if (e != io_error::none)
        {
            return e;
        }

        return m_reactor.run();
    }

    // stop
    //   function: stops the reactor loop (thread-safe).
    void
    stop()
    {
        m_reactor.stop();

        return;
    }

    // connection_count
    //   function: the number of live connections.
    D_NODISCARD std::size_t
    connection_count() const
    {
        return m_conns.size();
    }

private:
    D_DELETED_FN(event_server(const event_server&))
    D_DELETED_FN(event_server& operator=(const event_server&))

    // listener_ready, connection_ready
    //   struct: the reactor callbacks, as function objects over the server.
    struct listener_ready
    {
        explicit listener_ready(
            event_server* _server
        )
            : server(_server)
        {}

        void
        operator()(
            int               _fd,
            event_mask::value _ready
        ) const
        {
            (void)_fd;
            server->on_listener(_ready);

            return;
        }

        event_server* server;
    };

    struct connection_ready
    {
        explicit connection_ready(
            event_server* _server
        )
            : server(_server)
        {}

        void
        operator()(
            int               _fd,
            event_mask::value _ready
        ) const
        {
            server->on_connection(_fd,
                                  _ready);

            return;
        }

        event_server* server;
    };

    // on_listener
    //   function: accepts all pending connections (non-blocking), wraps and
    // registers each, and fires on_connect.
    void
    on_listener(
        event_mask::value _ready
    )
    {
        // an error on the listener accepts nothing
        if (has_event(_ready,
                      event_mask::error))
        {
            return;
        }

        // every pending connection
        for (;;)
        {
            const int cfd = ::accept(m_listen_fd,
                                     D_NULLPTR,
                                     D_NULLPTR);

            // no connection: a signal, an empty queue, or one aborted
            if (cfd < 0)
            {
                // a signal, or a connection aborted before it was taken
                if ( (errno == EINTR) ||
                     (errno == ECONNABORTED) )
                {
                    continue;
                }

                break;
            }

            internal::set_cloexec(cfd);
            internal::suppress_sigpipe(cfd);

            // a descriptor that cannot be made non-blocking is dropped
            if (internal::set_nonblocking(cfd) != io_error::none)
            {
                (void)::close(cfd);

                continue;
            }

            accept_one(cfd);
        }

        return;
    }

    // accept_one
    //   function: wraps and registers one accepted descriptor and fires
    // on_connect.
    void
    accept_one(
        int _fd
    )
    {
        event_connection* const conn =
            new (std::nothrow) event_connection(_fd);

        // no memory for the connection: the descriptor is dropped
        if (!conn)
        {
            (void)::close(_fd);

            return;
        }

        m_conns[_fd] = conn;

        const io_error::value e =
            m_reactor.add(_fd,
                          event_mask::readable,
                          reactor::callback(connection_ready(this)));

        // a connection the reactor will not watch is dropped
        if (e != io_error::none)
        {
            m_conns.erase(_fd);
            delete conn;

            return;
        }

        // the caller hears of it
        if (!m_callbacks.on_connect.empty())
        {
            m_callbacks.on_connect(*conn);
        }

        settle(_fd,
               conn);

        return;
    }

    // on_connection
    //   function: services a ready connection -- flush on writable, drain and
    // deliver on readable, and close on EOF or error.
    void
    on_connection(
        int               _fd,
        event_mask::value _ready
    )
    {
        const std::map<int, event_connection*>::iterator it = m_conns.find(_fd);

        // already closed
        if (it == m_conns.end())
        {
            return;
        }

        event_connection* const c     = it->second;
        bool                    fatal = has_event(_ready,
                                                  event_mask::error);

        // writable: drain queued output
        if ( (!fatal) &&
             (has_event(_ready,
                        event_mask::writable)) &&
             (c->flush_outbound() != io_error::none) )
        {
            fatal = true;
        }

        // readable (or peer hangup): read all available and deliver
        if ( (!fatal) &&
             ( (has_event(_ready,
                          event_mask::readable)) ||
               (has_event(_ready,
                          event_mask::hangup)) ) )
        {
            fatal = drain(*c);
        }

        // EOF or an error ends the connection
        if (fatal)
        {
            close_connection(_fd);

            return;
        }

        settle(_fd,
               c);

        return;
    }

    // drain
    //   function: reads all a connection has, delivering each chunk; returns
    // whether the connection has ended (EOF or a hard error).
    bool
    drain(
        event_connection& _c
    )
    {
        // until the socket is empty, the stream ends, or a close is asked
        for (;;)
        {
            byte            buffer[16384];
            const io_result r = _c.socket().read(buffer,
                                                 sizeof(buffer));

            // a would-block read leaves the connection open
            if (r.error == io_error::would_block)
            {
                return false;
            }

            // an orderly EOF, or a hard error, ends it
            if ( (r.error != io_error::none) ||
                 (r.count == 0) )
            {
                return true;
            }

            // the caller hears each chunk
            if (!m_callbacks.on_data.empty())
            {
                m_callbacks.on_data(_c,
                                    buffer,
                                    r.count);
            }

            // a callback may have asked to close; stop reading if so
            if (_c.is_closing())
            {
                return false;
            }
        }
    }

    // settle
    //   function: after servicing/callbacks, either close a drained closing
    // connection or update its readiness interest (add writable iff queued).
    void
    settle(
        int               _fd,
        event_connection* _c
    )
    {
        // closed by a callback already
        if (m_conns.find(_fd) == m_conns.end())
        {
            return;
        }

        // a closing connection with nothing left to send goes now
        if ( (_c->is_closing()) &&
             (!_c->wants_write()) )
        {
            close_connection(_fd);

            return;
        }

        event_mask::value interest = event_mask::readable;

        // queued bytes wait for write-readiness
        if (_c->wants_write())
        {
            interest |= event_mask::writable;
        }

        (void)m_reactor.modify(_fd,
                               interest);

        return;
    }

    // close_connection
    //   function: fires on_close, unregisters from the reactor, and destroys
    // the connection (closing its descriptor). Order matters: deregister
    // before the descriptor is closed.
    void
    close_connection(
        int _fd
    )
    {
        const std::map<int, event_connection*>::iterator it = m_conns.find(_fd);

        // already closed
        if (it == m_conns.end())
        {
            return;
        }

        event_connection* const conn = it->second;

        // the caller hears of it
        if (!m_callbacks.on_close.empty())
        {
            m_callbacks.on_close(*conn);
        }

        (void)m_reactor.remove(_fd);
        m_conns.erase(it);
        delete conn;

        return;
    }

    reactor                           m_reactor;
    tcp_acceptor                      m_acceptor;
    std::map<int, event_connection*>  m_conns;  // each owned
    callbacks                         m_callbacks;
    int                               m_listen_fd;
};


NS_END  // net
NS_END  // djinterp


#elif (D_INTERNAL_NET_REQUIRE_BACKENDS == 1)
    #error "reactor.hpp: no epoll (D_ENV_NET_HAS_EPOLL)"
#endif  // D_ENV_NET_HAS_EPOLL


#endif  // DJINTERP_NET_REACTOR_HPP
