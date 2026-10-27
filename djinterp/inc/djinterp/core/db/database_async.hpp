/*******************************************************************************
* djinterp [core]                                             database_async.hpp
*
* Operations that do not block, over the C layers' calls that do not wait.
*   async_operation<Result> is one operation of a connection or a prepared
* statement -- connect_async, execute_query_async, execute_update_async,
* execute_async -- made without waiting on the network: start() begins it,
* pending() says what it waits for, as the C layers' d_db_pending does, and
* resume() goes on with what came, as d_db_wait, until done(). result() then
* gives what the blocking operation returns, or throws what it throws. The
* program waits however it likes -- poll(2), its own event loop -- on the
* socket and the deadline pending() names; the operation never waits.
*   SQLite has no socket and nothing to wait for: its operations are done as
* soon as they start. With libmysqlclient, which has no statement calls that
* do not block, a prepared statement's operation fails, D_DB_UNSUPPORTED.
*   While an operation is under way, from its start until it is done, it
* holds its connection as a stream's run does: every other operation of the
* connection, and of its statements, throws query_exception with the
* layer's busy code. An operation destroyed while under way abandons it,
* which leaves the connection lost: reconnect() makes another. cancel() asks
* the server to stop what the operation runs, from a connection of its own,
* and the operation goes on to its end -- the server's refusal, or its
* result where it finished first.
*   From C++20, an operation is awaited in a coroutine with on(loop), the
* loop being the program's own (concepts::db_event_loop): told what to wait
* for and whom to wake (db_wakeup), it calls the wakeup with what came. No
* loop is imposed.
*   An operation belongs to the connection or the statement that made it,
* which must outlive it and stay where it is while it is under way. Not
* copyable; movable. Not thread-safe, as nothing of core/db is.
*
*
* path:      /inc/djinterp/core/db/database_async.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.24
*                                                            revised: 2026.10.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPERATIONS
    ----------
    1.  Their steps
         1.  internal::async_steps
         2.  internal::async_value
         3.  internal::at_once_steps
    2.  The operation
         1.  async_operation
2.  AWAITING
    --------
    1.  Levels
         1.  D_INTERNAL_DB_ASYNC_AWAIT
    2.  The loop
         1.  db_wakeup
         2.  concepts::db_event_loop
    3.  The awaitable
         1.  async_awaitable
3.  DEFINITIONS
    -----------
    1.  The operation
    2.  The awaitable
*/

#ifndef DJINTERP_DB_DATABASE_ASYNC_HPP
#define DJINTERP_DB_DATABASE_ASYNC_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <exception>    // std::exception_ptr, std::current_exception
#include <functional>   // std::function
#include <memory>       // std::unique_ptr
#include <optional>     // std::optional
#include <stdexcept>    // std::logic_error
#include <type_traits>  // std::is_void_v
#include <utility>      // std::move
// djinterp
#include "../../djinterp.hpp"      // framework root
#include "../../c/db/db_common.h"  // d_db_pending, d_db_wait, D_DB_*

// 2.1.1
// D_INTERNAL_DB_ASYNC_AWAIT
//   constant: 1 from C++20 where the language and the library have
// coroutines and concepts: async_operation's on(loop), awaited. The
// language's coroutines are asked of the compiler by their own name too,
// __cpp_impl_coroutine, which env's D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE
// misspells (__cpp_IMPL_coroutine) and so never reports.
#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER)              &&                         \
      ( (defined(__cpp_impl_coroutine))  ||                                   \
        (D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE) )  &&                         \
      (D_ENV_CPP_FEATURE_STL_COROUTINE)            &&                         \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS)            &&                         \
      (D_ENV_CPP_FEATURE_STL_CONCEPTS) )
    #define D_INTERNAL_DB_ASYNC_AWAIT 1
#else
    #define D_INTERNAL_DB_ASYNC_AWAIT 0
#endif

#if D_INTERNAL_DB_ASYNC_AWAIT
    #include <concepts>   // the concept's requirements
    #include <coroutine>  // std::coroutine_handle
#endif  // D_INTERNAL_DB_ASYNC_AWAIT


NS_DJINTERP


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Their steps
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.1.1
    // async_steps
    //   class template: what one kind of operation does, behind
    // async_operation: begun once, advanced with what came until it is done,
    // its result then taken once. begin and advance answer true once done,
    // false while waiting -- `_pending` then saying for what -- and throw
    // what the blocking operation throws for a failure, which the operation
    // keeps for result(); ended lets the connection go, done or failed;
    // cancel asks the server to stop it; abandon gives it up while it
    // waits, as the operation is destroyed.
    template<typename Result>
    class async_steps
    {
    public:
        virtual ~async_steps() = default;

        virtual bool   begin(d_db_pending& _pending)    = 0;
        virtual bool   advance(unsigned      _events,
                               d_db_pending& _pending)  = 0;
        virtual Result take()                           = 0;
        virtual void   ended() noexcept                 = 0;
        virtual void   cancel()                         = 0;
        virtual void   abandon() noexcept               = 0;
    };

    // 1.1.2
    // async_value
    //   class template: the result an operation keeps from its end until
    // result() hands it over; nothing for void.
    template<typename Result>
    class async_value
    {
    public:
        void
        put(
            Result&& _value
        )
        {
            m_value.emplace(std::move(_value));

            return;
        }

        Result
        take()
        {
            Result taken = std::move(*m_value);

            m_value.reset();

            return taken;
        }

    private:
        std::optional<Result> m_value;
    };

    template<>
    class async_value<void>
    {
    public:
        void
        take() noexcept
        {
            return;
        }
    };

    // 1.1.3
    // at_once_steps
    //   class template: an operation a vendor makes without waiting on
    // anything -- SQLite's, which has no socket -- done as it begins: the
    // blocking operation run, its result kept.
    template<typename Result>
    class at_once_steps final : public async_steps<Result>
    {
    public:
        explicit at_once_steps(
            std::function<Result()> _run
        )
            : m_run(std::move(_run)),
              m_value()
        {}

        bool
        begin(
            d_db_pending& _pending
        ) override
        {
            static_cast<void>(_pending);

            // the blocking operation, its result kept for take
            if constexpr (std::is_void_v<Result>)
            {
                m_run();
            }
            else
            {
                m_value.put(m_run());
            }

            return true;
        }

        bool
        advance(
            unsigned      _events,
            d_db_pending& _pending
        ) override
        {
            static_cast<void>(_events);
            static_cast<void>(_pending);

            return true;
        }

        Result
        take() override
        {
            return m_value.take();
        }

        void
        ended() noexcept override
        {
            return;
        }

        void
        cancel() override
        {
            return;
        }

        void
        abandon() noexcept override
        {
            return;
        }

    private:
        std::function<Result()> m_run;
        async_value<Result>     m_value;
    };

NS_END  // internal

// 1.2    The operation
//------------------------------------------------------------------------------
// 1.2.1
// async_operation
//   class template: one operation that does not block, as the banner
// describes. Before start() it has done nothing; from then until done() it
// is under way, holding its connection; once done, result() hands over its
// result, or throws its failure, once. An operation made empty -- default
// constructed, or moved from -- has nothing to start. Calling a member out
// of that order -- resume() before start() or after done(), result()
// before done() or twice -- throws std::logic_error. Not copyable; movable.
template<typename Result>
class async_operation
{
private:
    // phase
    //   enum: where the operation is.
    enum class phase
    {
        idle,      // not started
        waiting,   // under way
        finished,  // done, its result or failure kept
        handed     // done, its result handed over
    };

public:
    using result_type = Result;

    async_operation() noexcept;
    explicit async_operation(
        std::unique_ptr<internal::async_steps<Result>> _steps) noexcept;

    async_operation(const async_operation&)            = delete;
    async_operation& operator=(const async_operation&) = delete;

    async_operation(async_operation&& _other) noexcept;
    async_operation& operator=(async_operation&& _other) noexcept;

    ~async_operation();

    /**
     * @brief Begins the operation.
     *
     * @note A failure is the operation's, kept for result(), which throws
     *       it: start() does not throw it itself.
     *
     * @throws std::logic_error for an empty operation, or one started
     *         already.
     * @post The operation is under way, or done -- SQLite's always, and any
     *       that needed nothing it had to wait for, or that failed.
     */
    void start();

    /**
     * @brief Goes on with the operation, given what came.
     *
     * @param[in] _events  what came, as d_db_wait: the socket readable,
     *                     writable, or the deadline passed.
     * @throws std::logic_error unless the operation is under way.
     * @post The operation is under way still, pending() saying for what, or
     *       done.
     */
    void resume(unsigned _events);

    // state -- whether the operation was started, and whether it is done;
    // what it waits for while under way, nothing pending otherwise
    bool                started() const noexcept;
    bool                done() const noexcept;
    const d_db_pending& pending() const noexcept;

    /**
     * @brief Hands over the operation's result, once it is done.
     *
     * @return what the blocking operation returns.
     * @throws what the blocking operation throws for its failure;
     *         std::logic_error unless the operation is done and its result
     *         not yet handed over.
     */
    Result result();

    /**
     * @brief Asks the server to stop what the operation runs.
     *
     * @note The request goes from a connection of its own, which it makes
     *       and closes, waiting for both; the operation goes on, and ends
     *       with the server's refusal -- 1317 on MariaDB and MySQL, 57014 on
     *       PostgreSQL -- or with its result, where it finished first. A
     *       request the server receives before it has begun the statement
     *       may be lost -- MariaDB discards a KILL QUERY between statements
     *       -- so a caller that must stop the statement asks once the
     *       server runs it. A connect has nothing to stop; destroying it
     *       abandons it.
     *
     * @throws query_exception when the request cannot be made; nothing for
     *         an operation not under way.
     */
    void cancel();

#if D_INTERNAL_DB_ASYNC_AWAIT
    /**
     * @brief Awaits the operation in a coroutine, waiting through the
     *        program's own loop.
     *
     * @param[in] _loop  the loop, which outlives the awaiting.
     * @return the awaitable: co_await starts the operation if it has not
     *         been, waits through `_loop` until it is done, and gives its
     *         result, or throws its failure.
     */
    template<typename Loop>
    auto on(Loop& _loop);
#endif  // D_INTERNAL_DB_ASYNC_AWAIT

private:
    // m_run -- one step of the steps, its failure kept: done, the result
    // taken and the connection let go
    template<typename Step>
    void m_run(Step&& _step) noexcept;

    // m_abandon -- the steps given up while under way
    void m_abandon() noexcept;

    // m_unpend -- nothing pending
    void m_unpend() noexcept;

    std::unique_ptr<internal::async_steps<Result>> m_steps;
    d_db_pending                                   m_pending;
    phase                                          m_phase;
    std::exception_ptr                             m_failure;
    internal::async_value<Result>                  m_value;
};


//==============================================================================
// 2.  AWAITING
//==============================================================================


#if D_INTERNAL_DB_ASYNC_AWAIT

// 2.2    The loop
//------------------------------------------------------------------------------
// 2.2.1
// db_wakeup
//   class: what a loop calls, once, when what an operation waits for has
// come, with what came, as d_db_wait. Small and copyable, for a loop to
// keep.
class db_wakeup
{
public:
    using function_type = void (*)(void*    _context,
                                   unsigned _events) noexcept;

    db_wakeup(
        function_type _function,
        void*         _context
    ) noexcept
        : m_function(_function),
          m_context(_context)
    {}

    void
    operator()(
        unsigned _events
    ) const noexcept
    {
        m_function(m_context, _events);

        return;
    }

private:
    function_type m_function;
    void*         m_context;
};

NS_CONCEPTS

    // 2.2.2
    // db_event_loop
    //   concept: a program's loop, which an operation awaited waits through:
    // watch(pending, wakeup) arranges for the wakeup to be called once what
    // `pending` names has come -- its socket readable or writable, or its
    // deadline passed -- and returns at once.
    template<typename Loop>
    concept db_event_loop =
        requires(Loop&               _loop,
                 const d_db_pending& _pending,
                 db_wakeup           _wakeup)
        {
            _loop.watch(_pending, _wakeup);
        };

NS_END  // concepts

// 2.3    The awaitable
//------------------------------------------------------------------------------
// 2.3.1
// async_awaitable
//   class template: an operation awaited through a loop. Ready at once for
// an operation done as it starts; otherwise the coroutine sleeps while the
// loop waits, each wakeup going on with the operation until it is done.
template<typename Result,
         typename Loop>
class async_awaitable
{
public:
    async_awaitable(
        async_operation<Result>& _operation,
        Loop&                    _loop
    ) noexcept
        : m_operation(&_operation),
          m_loop(&_loop),
          m_handle(),
          m_failure()
    {}

    bool                    await_ready();
    void                    await_suspend(std::coroutine_handle<> _handle);
    Result                  await_resume();

private:
    // m_woken -- the loop's wakeup: on with the operation, then the
    // coroutine woken once it is done, or the loop told to wait again
    static void m_woken(void*    _self,
                        unsigned _events) noexcept;

    async_operation<Result>* m_operation;
    Loop*                    m_loop;
    std::coroutine_handle<>  m_handle;
    std::exception_ptr       m_failure;
};

#endif  // D_INTERNAL_DB_ASYNC_AWAIT


//==============================================================================
// 3.  DEFINITIONS
//==============================================================================


// 3.1    The operation
//------------------------------------------------------------------------------
template<typename Result>
async_operation<Result>::async_operation() noexcept
    : m_steps(),
      m_pending(),
      m_phase(phase::idle),
      m_failure(),
      m_value()
{
    m_unpend();
}

template<typename Result>
async_operation<Result>::async_operation(
    std::unique_ptr<internal::async_steps<Result>> _steps
) noexcept
    : m_steps(std::move(_steps)),
      m_pending(),
      m_phase(phase::idle),
      m_failure(),
      m_value()
{
    m_unpend();
}

template<typename Result>
async_operation<Result>::async_operation(
    async_operation&& _other
) noexcept
    : m_steps(std::move(_other.m_steps)),
      m_pending(_other.m_pending),
      m_phase(_other.m_phase),
      m_failure(std::move(_other.m_failure)),
      m_value(std::move(_other.m_value))
{
    _other.m_phase = phase::idle;
    _other.m_unpend();
}

template<typename Result>
async_operation<Result>&
async_operation<Result>::operator=(
    async_operation&& _other
) noexcept
{
    // an operation assigned to itself keeps what it has
    if (this != &_other)
    {
        m_abandon();

        m_steps        = std::move(_other.m_steps);
        m_pending      = _other.m_pending;
        m_phase        = _other.m_phase;
        m_failure      = std::move(_other.m_failure);
        m_value        = std::move(_other.m_value);
        _other.m_phase = phase::idle;
        _other.m_unpend();
    }

    return *this;
}

template<typename Result>
async_operation<Result>::~async_operation()
{
    m_abandon();
}

template<typename Result>
void
async_operation<Result>::start()
{
    // something to start, once
    if (!m_steps)
    {
        throw std::logic_error("async_operation start: no operation");
    }

    if (m_phase != phase::idle)
    {
        throw std::logic_error("async_operation start: started already");
    }

    m_phase = phase::waiting;
    m_run(
        [this]() -> bool
        {
            return m_steps->begin(m_pending);
        });

    return;
}

template<typename Result>
void
async_operation<Result>::resume(
    unsigned _events
)
{
    // only an operation under way goes on
    if (m_phase != phase::waiting)
    {
        throw std::logic_error("async_operation resume: not under way");
    }

    m_run(
        [this, _events]() -> bool
        {
            return m_steps->advance(_events, m_pending);
        });

    return;
}

template<typename Result>
bool
async_operation<Result>::started() const noexcept
{
    return (m_phase != phase::idle);
}

template<typename Result>
bool
async_operation<Result>::done() const noexcept
{
    return ( (m_phase == phase::finished) ||
             (m_phase == phase::handed) );
}

template<typename Result>
const d_db_pending&
async_operation<Result>::pending() const noexcept
{
    return m_pending;
}

template<typename Result>
Result
async_operation<Result>::result()
{
    // a result kept, not yet handed over
    if (m_phase != phase::finished)
    {
        throw std::logic_error((m_phase == phase::handed)
                                   ? "async_operation result: handed over "
                                     "already"
                                   : "async_operation result: not done");
    }

    m_phase = phase::handed;

    // the failure, or the result
    if (m_failure)
    {
        std::rethrow_exception(m_failure);
    }

    return m_value.take();
}

template<typename Result>
void
async_operation<Result>::cancel()
{
    // only what is under way can be stopped
    if (m_phase == phase::waiting)
    {
        m_steps->cancel();
    }

    return;
}

#if D_INTERNAL_DB_ASYNC_AWAIT

template<typename Result>
template<typename Loop>
auto
async_operation<Result>::on(
    Loop& _loop
)
{
    static_assert(concepts::db_event_loop<Loop>,
                  "a loop has watch(const d_db_pending&, db_wakeup)");

    return async_awaitable<Result, Loop>(*this, _loop);
}

#endif  // D_INTERNAL_DB_ASYNC_AWAIT

/*
async_operation::m_run
  A failure is kept whatever it is, and ends the operation as success does;
taking the result can fail too -- for want of memory -- and is kept the same
way. The connection is let go either way.
*/
template<typename Result>
template<typename Step>
void
async_operation<Result>::m_run(
    Step&& _step
) noexcept
{
    try
    {
        // waiting still: nothing more to do now
        if (!_step())
        {
            return;
        }

        if constexpr (std::is_void_v<Result>)
        {
            m_steps->take();
        }
        else
        {
            m_value.put(m_steps->take());
        }
    }
    catch (...)
    {
        m_failure = std::current_exception();
    }

    m_steps->ended();
    m_phase = phase::finished;
    m_unpend();

    return;
}

template<typename Result>
void
async_operation<Result>::m_abandon() noexcept
{
    // only an operation under way has anything to give up
    if (m_phase == phase::waiting)
    {
        m_steps->abandon();
        m_phase = phase::finished;
        m_unpend();
    }

    return;
}

template<typename Result>
void
async_operation<Result>::m_unpend() noexcept
{
    m_pending.socket     = D_DB_SOCKET_NONE;
    m_pending.events     = 0u;
    m_pending.timeout_ms = 0u;
    m_pending.deadline   = 0.0;
    m_pending.state      = 0u;

    return;
}

#if D_INTERNAL_DB_ASYNC_AWAIT

// 3.2    The awaitable
//------------------------------------------------------------------------------
/*
async_awaitable::await_ready
  The operation starts here when it has not; one done as it starts -- every
SQLite operation -- never sleeps.
*/
template<typename Result,
         typename Loop>
bool
async_awaitable<Result, Loop>::await_ready()
{
    // started here, unless the program started it
    if (!m_operation->started())
    {
        m_operation->start();
    }

    return m_operation->done();
}

/*
async_awaitable::await_suspend
  The loop may wake the coroutine before watch returns, which may then end
it: nothing of the awaitable is touched after watch.
*/
template<typename Result,
         typename Loop>
void
async_awaitable<Result, Loop>::await_suspend(
    std::coroutine_handle<> _handle
)
{
    m_handle = _handle;
    m_loop->watch(m_operation->pending(), db_wakeup(&m_woken, this));

    return;
}

template<typename Result,
         typename Loop>
Result
async_awaitable<Result, Loop>::await_resume()
{
    // a loop that could not wait again
    if (m_failure)
    {
        std::rethrow_exception(m_failure);
    }

    return m_operation->result();
}

/*
async_awaitable::m_woken
  resume keeps any failure of the operation's for result(); a loop that
cannot wait again ends the awaiting with its own failure, which await_resume
throws.
*/
template<typename Result,
         typename Loop>
void
async_awaitable<Result, Loop>::m_woken(
    void*    _self,
    unsigned _events
) noexcept
{
    async_awaitable* const self = static_cast<async_awaitable*>(_self);

    try
    {
        self->m_operation->resume(_events);

        // under way still: the loop waits again
        if (!self->m_operation->done())
        {
            self->m_loop->watch(self->m_operation->pending(),
                                db_wakeup(&m_woken, self));

            return;
        }
    }
    catch (...)
    {
        self->m_failure = std::current_exception();
    }

    self->m_handle.resume();

    return;
}

#endif  // D_INTERNAL_DB_ASYNC_AWAIT


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_DATABASE_ASYNC_HPP
