/******************************************************************************
* djinterp [net]                                                   reactor.hpp
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
*                     including removing its own (erase is deferred past dispatch)
*   event_connection  a non-blocking connection with an outbound buffer:
*                     send() flushes what it can and queues the rest; the server
*                     drains it on write-readiness (backpressure without blocking)
*   event_server      accepts connections and dispatches on_connect / on_data /
*                     on_close callbacks from a single-threaded reactor loop
*
*   The blocking backends remain the default; this is a distinct concurrency
* model (non-blocking, callback-driven) rather than an execution_policy, since a
* reactor cannot run to-completion blocking handlers. It reuses socket_connection
* from tcp.hpp -- already non-blocking-correct (EAGAIN -> would_block) -- and
* only needs the descriptor made non-blocking and registered.
*
*   Requires:  net.hpp, tcp.hpp, env_net.h, and epoll (D_ENV_NET_HAS_EPOLL). A
*              kqueue backend would be a sibling of the same shape.
*
* path:      /inc/djinterp/net/reactor.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.17
******************************************************************************/

#ifndef DJINTERP_NET_REACTOR_
#define DJINTERP_NET_REACTOR_ 1

// djinterp
#include "./net.hpp"
#include "./tcp.hpp"
#include "../core/container/buffer/byte_buffer.hpp"
#include "../core/env/net/env_net.h"


#if !D_ENV_NET_HAS_EPOLL
    #error "net/reactor.hpp requires epoll (see env_net.h); a kqueue backend is a sibling"
#endif


// std / POSIX
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>


NS_DJINTERP
NS_NET


///////////////////////////////////////////////////////////////////////////////
///                            EVENT MASK                                  ///
///////////////////////////////////////////////////////////////////////////////

// event_mask
//   enum: the readiness conditions a reactor reports and a caller registers
// interest in. readable/writable are requested and reported; error/hangup are
// always reported by the poller and need not be requested.
enum class event_mask : unsigned int
{
    none     = 0u,
    readable = 1u << 0,
    writable = 1u << 1,
    error    = 1u << 2,
    hangup   = 1u << 3
};

D_NODISCARD inline event_mask
operator|(
    event_mask _a,
    event_mask _b
)
{
    return static_cast<event_mask>(
        static_cast<unsigned int>(_a) | static_cast<unsigned int>(_b));
}

D_NODISCARD inline event_mask
operator&(
    event_mask _a,
    event_mask _b
)
{
    return static_cast<event_mask>(
        static_cast<unsigned int>(_a) & static_cast<unsigned int>(_b));
}

inline event_mask&
operator|=(
    event_mask& _a,
    event_mask  _b
)
{
    _a = _a | _b;
    return _a;
}

// has_event
//   function: whether `_flag` is set in `_mask`.
D_NODISCARD inline bool
has_event(
    event_mask _mask,
    event_mask _flag
)
{
    return (static_cast<unsigned int>(_mask & _flag) != 0u);
}

// any_event
//   function: whether any condition is set.
D_NODISCARD inline bool
any_event(
    event_mask _mask
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
    D_NODISCARD inline io_error
    set_nonblocking(
        int _fd
    )
    {
        const int flags = ::fcntl(_fd, F_GETFL, 0);

        if (flags < 0)
        {
            return from_errno(errno);
        }

        if (::fcntl(_fd, F_SETFL, flags | O_NONBLOCK) < 0)
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
        event_mask _mask
    )
    {
        uint32_t bits = 0;

        if (has_event(_mask, event_mask::readable))
        {
            bits |= EPOLLIN;
        }

        if (has_event(_mask, event_mask::writable))
        {
            bits |= EPOLLOUT;
        }

    #ifdef EPOLLRDHUP
        bits |= EPOLLRDHUP;
    #endif

        return bits;
    }

    // epoll_to_event
    //   function: converts reported epoll bits to an event_mask.
    D_NODISCARD inline event_mask
    epoll_to_event(
        uint32_t _bits
    )
    {
        event_mask mask = event_mask::none;

        if (_bits & EPOLLIN)
        {
            mask |= event_mask::readable;
        }

        if (_bits & EPOLLOUT)
        {
            mask |= event_mask::writable;
        }

        if (_bits & EPOLLERR)
        {
            mask |= event_mask::error;
        }

    #ifdef EPOLLRDHUP
        if (_bits & (EPOLLHUP | EPOLLRDHUP))
    #else
        if (_bits & EPOLLHUP)
    #endif
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
    using callback = std::function<void(int, event_mask)>;

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
        m_epoll = ::epoll_create1(EPOLL_CLOEXEC);

        if (m_epoll < 0)
        {
            return;
        }

        m_wake = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

        if (m_wake < 0)
        {
            (void)::close(m_epoll);
            m_epoll = -1;
            return;
        }

        // register the wake eventfd directly (kept out of the entry map)
        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events   = EPOLLIN;
        ev.data.fd  = m_wake;

        if (::epoll_ctl(m_epoll, EPOLL_CTL_ADD, m_wake, &ev) < 0)
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
        if (m_wake >= 0)
        {
            (void)::close(m_wake);
            m_wake = -1;
        }

        if (m_epoll >= 0)
        {
            (void)::close(m_epoll);
            m_epoll = -1;
        }
    }

    reactor(const reactor&)            D_DELETE;
    reactor& operator=(const reactor&) D_DELETE;
    reactor(reactor&&)                 D_DELETE;
    reactor& operator=(reactor&&)      D_DELETE;

    // is_valid
    //   function: whether the epoll instance was created.
    D_NODISCARD bool
    is_valid() const
    {
        return (m_epoll >= 0);
    }

    // add
    //   function: registers a descriptor with an interest set and callback.
    D_NODISCARD io_error
    add(
        int        _fd,
        event_mask _interest,
        callback   _cb
    )
    {
        if ( (m_epoll < 0) ||
             (_fd < 0) )
        {
            return io_error::invalid_argument;
        }

        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events  = internal::epoll_interest(_interest);
        ev.data.fd = _fd;

        if (::epoll_ctl(m_epoll, EPOLL_CTL_ADD, _fd, &ev) < 0)
        {
            return internal::from_errno(errno);
        }

        entry e;
        e.cb       = std::move(_cb);
        e.interest = _interest;
        e.dead     = false;

        m_entries[_fd] = std::move(e);

        return io_error::none;
    }

    // modify
    //   function: changes the interest set for a registered descriptor.
    D_NODISCARD io_error
    modify(
        int        _fd,
        event_mask _interest
    )
    {
        const std::unordered_map<int, entry>::iterator it = m_entries.find(_fd);

        if (it == m_entries.end())
        {
            return io_error::invalid_argument;
        }

        if (it->second.interest == _interest)
        {
            return io_error::none;
        }

        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events  = internal::epoll_interest(_interest);
        ev.data.fd = _fd;

        if (::epoll_ctl(m_epoll, EPOLL_CTL_MOD, _fd, &ev) < 0)
        {
            return internal::from_errno(errno);
        }

        it->second.interest = _interest;

        return io_error::none;
    }

    // remove
    //   function: unregisters a descriptor. The epoll deletion happens at once;
    // the entry (and its callback) is erased after the current dispatch when
    // called from within a callback, so a callback may safely remove its own fd.
    io_error
    remove(
        int _fd
    )
    {
        const std::unordered_map<int, entry>::iterator it = m_entries.find(_fd);

        if (it == m_entries.end())
        {
            return io_error::invalid_argument;
        }

        (void)::epoll_ctl(m_epoll, EPOLL_CTL_DEL, _fd, nullptr);

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
    D_NODISCARD io_error
    run_once(
        int _timeout_ms
    )
    {
        return dispatch(_timeout_ms);
    }

    // run
    //   function: dispatches until stop(). Returns none on a clean stop, or the
    // first fatal poller error.
    D_NODISCARD io_error
    run()
    {
        if (m_epoll < 0)
        {
            return io_error::invalid_argument;
        }

        m_stop.store(false);
        m_running.store(true);

        io_error result = io_error::none;

        while (!m_stop.load())
        {
            result = dispatch(-1);

            if ( (result != io_error::none) &&
                 (result != io_error::interrupted) )
            {
                break;
            }
        }

        m_running.store(false);

        return (result == io_error::interrupted) ? io_error::none : result;
    }

    // stop
    //   function: requests the run loop to exit; wakes a blocked wait. Safe to
    // call from another thread.
    void
    stop()
    {
        m_stop.store(true);
        wake();

        return;
    }

    // is_running
    //   function: whether a run loop is active.
    D_NODISCARD bool
    is_running() const
    {
        return m_running.load();
    }

    // handle_count
    //   function: the number of registered descriptors (excluding the wake fd).
    D_NODISCARD std::size_t
    handle_count() const
    {
        return m_entries.size();
    }

private:
    struct entry
    {
        callback   cb;
        event_mask interest;
        bool       dead;

        entry()
            : cb(),
              interest(event_mask::none),
              dead(false)
        {
        }
    };

    // wake
    //   function: nudges the eventfd so a blocked epoll_wait returns.
    void
    wake()
    {
        if (m_wake < 0)
        {
            return;
        }

        const uint64_t one = 1;
        const ssize_t  n   = ::write(m_wake, &one, sizeof(one));
        (void)n;

        return;
    }

    // dispatch
    //   function: one epoll_wait + callback dispatch, then process deferred
    // removals.
    D_NODISCARD io_error
    dispatch(
        int _timeout_ms
    )
    {
        if (m_epoll < 0)
        {
            return io_error::invalid_argument;
        }

        // size the result buffer to current registrations (plus the wake fd)
        std::size_t want = m_entries.size() + 1;

        if (want < 16)
        {
            want = 16;
        }

        m_events.resize(want);

        const int n = ::epoll_wait(m_epoll,
                                   m_events.data(),
                                   static_cast<int>(m_events.size()),
                                   _timeout_ms);

        if (n < 0)
        {
            if (errno == EINTR)
            {
                return io_error::interrupted;
            }

            return internal::from_errno(errno);
        }

        m_dispatching = true;

        for (int i = 0; i < n; ++i)
        {
            const int      fd   = m_events[i].data.fd;
            const uint32_t bits = m_events[i].events;

            if (fd == m_wake)
            {
                // drain the counter and carry on
                uint64_t sink = 0;
                const ssize_t r = ::read(m_wake, &sink, sizeof(sink));
                (void)r;
                continue;
            }

            const std::unordered_map<int, entry>::iterator it =
                m_entries.find(fd);

            if ( (it == m_entries.end()) ||
                 (it->second.dead) )
            {
                // removed by an earlier callback in this same batch
                continue;
            }

            const event_mask ready = internal::epoll_to_event(bits);

            // the callback may add / modify / remove registrations
            it->second.cb(fd, ready);
        }

        m_dispatching = false;

        // now that dispatch is done, erase entries removed during it
        if (!m_deferred.empty())
        {
            for (std::size_t k = 0; k < m_deferred.size(); ++k)
            {
                const std::unordered_map<int, entry>::iterator dit =
                    m_entries.find(m_deferred[k]);

                if ( (dit != m_entries.end()) &&
                     (dit->second.dead) )
                {
                    m_entries.erase(dit);
                }
            }

            m_deferred.clear();
        }

        return io_error::none;
    }

    int                             m_epoll;
    int                             m_wake;
    std::unordered_map<int, entry>  m_entries;
    std::vector<struct epoll_event> m_events;
    std::vector<int>                m_deferred;
    std::atomic<bool>               m_stop;
    std::atomic<bool>               m_running;
    bool                            m_dispatching;
};


///////////////////////////////////////////////////////////////////////////////
///                        EVENT CONNECTION                                ///
///////////////////////////////////////////////////////////////////////////////

// event_connection
//   class: a non-blocking connection managed by an event_server. Wraps a
// socket_connection and an outbound buffer. send() writes as much as the socket
// accepts immediately and queues the remainder; the server flushes the queue on
// write-readiness. close() requests a graceful close once the queue drains. A
// user_data slot carries per-connection state. Non-copyable; held by pointer.
class event_connection
{
public:
    explicit event_connection(
        socket_connection&& _socket
    )
        : user_data(),
          m_socket(std::move(_socket)),
          m_outbound(),
          m_closing(false)
    {
    }

    event_connection(const event_connection&)            D_DELETE;
    event_connection& operator=(const event_connection&) D_DELETE;

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
    //   function: sends `_length` bytes, flushing immediately where possible and
    // queuing the rest. Returns a transport error only on a hard failure;
    // would_block is handled by queuing, not returned.
    D_NODISCARD io_error
    send(
        const void* _data,
        std::size_t _length
    )
    {
        if (m_closing)
        {
            return io_error::closed;
        }

        if (_length == 0)
        {
            return io_error::none;
        }

        const byte* p = static_cast<const byte*>(_data);

        // nothing queued: try to write straight through first
        if (m_outbound.size() == 0)
        {
            // fully drained -- rewind the read/write cursors to reclaim storage
            m_outbound.reset();

            std::size_t off = 0;

            while (off < _length)
            {
                const io_result r = m_socket.write(p + off, _length - off);

                if (r.error == io_error::none)
                {
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

            if (off < _length)
            {
                m_outbound.append(p + off, _length - off);
            }

            return io_error::none;
        }

        // already queuing: preserve order, append behind the pending bytes
        m_outbound.append(p, _length);

        return io_error::none;
    }

    D_NODISCARD io_error
    send(
        const std::string& _text
    )
    {
        return send(_text.data(), _text.size());
    }

    // flush_outbound
    //   function: writes as much queued data as the socket accepts. Called by
    // the server on write-readiness.
    D_NODISCARD io_error
    flush_outbound()
    {
        while (m_outbound.size() > 0)
        {
            const io_result r = m_socket.write(m_outbound.data(),
                                               m_outbound.size());

            if (r.error == io_error::none)
            {
                if (r.count == 0)
                {
                    break;
                }

                m_outbound.advance(r.count);
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

        // fully drained -- rewind to reclaim the consumed prefix; while bytes
        // remain the read cursor tracks the flush point and the prefix is held
        if (m_outbound.size() == 0)
        {
            m_outbound.reset();
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
        return (m_outbound.size() > 0);
    }

    // socket
    //   function: access to the underlying connection (for reads).
    D_NODISCARD socket_connection&
    socket()
    {
        return m_socket;
    }

    // per-connection user state (set/read by callbacks)
    std::shared_ptr<void> user_data;

private:
    socket_connection       m_socket;
    ::djinterp::byte_stream m_outbound;
    bool                    m_closing;
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
        std::function<void(event_connection&)>                            on_connect;
        std::function<void(event_connection&, const byte*, std::size_t)>  on_data;
        std::function<void(event_connection&)>                            on_close;
    };

    event_server()
        : m_reactor(),
          m_acceptor(),
          m_conns(),
          m_callbacks(),
          m_listen_fd(-1)
    {
    }

    event_server(const event_server&)            D_DELETE;
    event_server& operator=(const event_server&) D_DELETE;
    event_server(event_server&&)                 D_DELETE;
    event_server& operator=(event_server&&)      D_DELETE;

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
    D_NODISCARD io_error
    bind(
        const endpoint& _endpoint
    )
    {
        const io_error e = m_acceptor.bind(_endpoint);

        if (e != io_error::none)
        {
            return e;
        }

        m_listen_fd = m_acceptor.native_handle();

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
    //   function: registers the listener and runs the reactor loop until stop().
    D_NODISCARD io_error
    run(
        const callbacks& _callbacks
    )
    {
        if (!m_reactor.is_valid())
        {
            return io_error::invalid_argument;
        }

        if (m_listen_fd < 0)
        {
            return io_error::invalid_argument;
        }

        m_callbacks = _callbacks;

        const io_error e = m_reactor.add(
            m_listen_fd,
            event_mask::readable,
            [this](int, event_mask _ready)
            {
                this->on_listener(_ready);
            });

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
    // on_listener
    //   function: accepts all pending connections (non-blocking), wraps and
    // registers each, and fires on_connect.
    void
    on_listener(
        event_mask _ready
    )
    {
        if (has_event(_ready, event_mask::error))
        {
            return;
        }

        for (;;)
        {
            const int cfd = ::accept(m_listen_fd, nullptr, nullptr);

            if (cfd < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                if ( (errno == EAGAIN) ||
                     (errno == EWOULDBLOCK) )
                {
                    break;
                }

                if (errno == ECONNABORTED)
                {
                    continue;
                }

                break;
            }

            internal::set_cloexec(cfd);
            internal::suppress_sigpipe(cfd);

            if (internal::set_nonblocking(cfd) != io_error::none)
            {
                (void)::close(cfd);
                continue;
            }

            std::unique_ptr<event_connection> conn(
                new event_connection(socket_connection(cfd, protocol::tcp)));

            event_connection* raw = conn.get();
            const int         fd  = raw->native_handle();

            m_conns[fd] = std::move(conn);

            const io_error e = m_reactor.add(
                fd,
                event_mask::readable,
                [this](int _fd, event_mask _evs)
                {
                    this->on_connection(_fd, _evs);
                });

            if (e != io_error::none)
            {
                m_conns.erase(fd);
                continue;
            }

            if (m_callbacks.on_connect)
            {
                m_callbacks.on_connect(*raw);
            }

            settle(fd, raw);
        }

        return;
    }

    // on_connection
    //   function: services a ready connection -- flush on writable, drain and
    // deliver on readable, and close on EOF or error.
    void
    on_connection(
        int        _fd,
        event_mask _ready
    )
    {
        const std::unordered_map<int, std::unique_ptr<event_connection>>::iterator
            it = m_conns.find(_fd);

        if (it == m_conns.end())
        {
            return;
        }

        event_connection* c     = it->second.get();
        bool              fatal = false;

        if (has_event(_ready, event_mask::error))
        {
            fatal = true;
        }

        // writable: drain queued output
        if ( (!fatal) &&
             has_event(_ready, event_mask::writable) )
        {
            if (c->flush_outbound() != io_error::none)
            {
                fatal = true;
            }
        }

        // readable (or peer hangup): read all available and deliver
        if ( (!fatal) &&
             ( has_event(_ready, event_mask::readable) ||
               has_event(_ready, event_mask::hangup) ) )
        {
            for (;;)
            {
                byte             buffer[16384];
                const io_result  r = c->socket().read(buffer, sizeof(buffer));

                if (r.error == io_error::none)
                {
                    if (r.count == 0)
                    {
                        fatal = true;   // orderly EOF
                        break;
                    }

                    if (m_callbacks.on_data)
                    {
                        m_callbacks.on_data(*c, buffer, r.count);
                    }

                    // a callback may have asked to close; stop reading if so
                    if (c->is_closing())
                    {
                        break;
                    }
                }
                else if (r.error == io_error::would_block)
                {
                    break;
                }
                else
                {
                    fatal = true;
                    break;
                }
            }
        }

        if (fatal)
        {
            close_connection(_fd);
            return;
        }

        settle(_fd, c);

        return;
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
        if (m_conns.find(_fd) == m_conns.end())
        {
            return;
        }

        if ( _c->is_closing() &&
             (!_c->wants_write()) )
        {
            close_connection(_fd);
            return;
        }

        event_mask interest = event_mask::readable;

        if (_c->wants_write())
        {
            interest |= event_mask::writable;
        }

        (void)m_reactor.modify(_fd, interest);

        return;
    }

    // close_connection
    //   function: fires on_close, unregisters from the reactor, and destroys the
    // connection (closing its descriptor). Order matters: deregister before the
    // descriptor is closed.
    void
    close_connection(
        int _fd
    )
    {
        const std::unordered_map<int, std::unique_ptr<event_connection>>::iterator
            it = m_conns.find(_fd);

        if (it == m_conns.end())
        {
            return;
        }

        if (m_callbacks.on_close)
        {
            m_callbacks.on_close(*it->second);
        }

        (void)m_reactor.remove(_fd);
        m_conns.erase(it);

        return;
    }

    reactor                                                        m_reactor;
    tcp_acceptor                                                   m_acceptor;
    std::unordered_map<int, std::unique_ptr<event_connection>>     m_conns;
    callbacks                                                      m_callbacks;
    int                                                            m_listen_fd;
};


NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_REACTOR_
