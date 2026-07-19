/******************************************************************************
* djinterp [container]                                              buffer.hpp
*
* Foundational buffer module for the djinterp container framework.
*   A buffer is temporary storage that is written in stages and consumed
* (read, flushed, or committed) as a unit or incrementally.  Unlike a
* general-purpose container, a buffer has directionality (write-then-
* read) and temporality (it gets drained).
*
*   This module is domain-agnostic: it has no knowledge of binary
* encodings, text encodings, or any higher-level format.  Domain-
* specific modules (binary_buffer.hpp, text_buffer.hpp, etc.) build
* on these primitives.
*
*   The design is policy-based along two orthogonal axes:
*
*     GROWTH POLICY - how the buffer acquires more storage when the
*       write cursor reaches capacity:
*         fixed_growth_policy       - compile-time capacity, no growth
*         linear_growth_policy      - grow by a constant increment
*         exponential_growth_policy - double capacity on exhaustion
*         page_growth_policy        - grow in OS page-aligned chunks
*
*     CURSOR POLICY - how the buffer tracks read/write positions:
*         write_only_cursor_policy  - single write cursor; consumer
*                                     manages read position externally
*         dual_cursor_policy        - paired write + read cursors for
*                                     producer/consumer patterns
*
*   buffer_base<Derived> is a CRTP base providing the storage-agnostic
* cursor arithmetic, state predicates, and growth dispatch.  The derived
* type owns the actual memory and element semantics.
*
* DEPENDENCIES:
*   djinterp.hpp    - namespace macros, clean_t
*   env.h           - C++ version and OS page-size detection
*
*
* path:      /inc/djinterp/core/container/buffer/buffer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.29
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.      growth strategy enum
II.     cursor model enum
III.    growth policies
IV.     cursor policies
V.      buffer_base (CRTP)
VI.     growth policy selection
VII.    cursor policy selection
VIII.   default policy aliases
*/

#ifndef DJINTERP_CONTAINER_BUFFER_
#define DJINTERP_CONTAINER_BUFFER_ 1

// std
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"


NS_DJINTERP


// =============================================================================
// I.   Growth Strategy Enum
// =============================================================================

// buffer_growth_strategy
//   enum: classifies the growth strategy a buffer uses when the write cursor 
// reaches capacity.
enum class buffer_growth_strategy
{
    // no growth - capacity is fixed at construction
    // or compile time; writes beyond capacity fail
    none,

    // grow by a constant byte increment each time
    // capacity is exhausted
    linear,

    // double capacity each time it is exhausted
    // (amortized O(1) append)
    exponential,

    // grow in multiples of the OS memory page size
    // (typically 4096 bytes)
    page_aligned
};


// =============================================================================
// II.  Cursor Model Enum
// =============================================================================

// buffer_cursor_model
//   enum: classifies how the buffer tracks read/write
// positions.
enum class buffer_cursor_model
{
    // single write cursor only - the consumer
    // manages read position externally (e.g. by
    // taking the whole buffer via data()/release())
    write_only,

    // paired write + read cursors - supports
    // incremental consumption via advance()/consume()
    dual
};


// =============================================================================
// III. Growth Policies
// =============================================================================
// Each growth policy is a plain struct exposing policy
// constants and a static compute method that returns
// the new capacity given the current capacity and the
// number of additional bytes requested.
//
// Mirrors the lock-policy pattern from threadsafe.hpp:
// no virtual functions, no inheritance, fully constexpr
// where possible.

// fixed_growth_policy
//   struct: no growth.  Capacity is set at construction
// and never changes.  Writes that exceed capacity fail.
struct fixed_growth_policy
{
    static constexpr buffer_growth_strategy strategy = buffer_growth_strategy::none;
    static constexpr bool can_grow = false;

    // compute
    //   returns the current capacity unchanged.
    // _required is ignored - growth is not permitted.
    static constexpr std::size_t
    compute(std::size_t _current,
            std::size_t /*_required*/) noexcept
    {
        return _current;
    }
};

// linear_growth_policy
//   struct: grow by a constant increment.  The increment
// defaults to 4096 bytes but can be overridden via the
// _Increment template parameter.
template<std::size_t _Increment = 4096>
struct linear_growth_policy
{
    static constexpr buffer_growth_strategy strategy = buffer_growth_strategy::linear;
    static constexpr bool        can_grow   = true;
    static constexpr std::size_t increment  = _Increment;

    static_assert(_Increment > 0,
        "linear_growth_policy: _Increment must be "
        "greater than zero.");

    // compute
    //   returns the smallest capacity >= _required that
    // is a multiple of _Increment above _current.
    static constexpr std::size_t
    compute(
        std::size_t _current,
        std::size_t _required
    ) noexcept
    {
        std::size_t cap = _current;

        while (cap < _required)
        {
            cap += _Increment;
        }

        return cap;
    }
};

// exponential_growth_policy
//   struct: double capacity on exhaustion.  Provides
// amortized O(1) appends.  The minimum initial capacity
// is 64 bytes.
struct exponential_growth_policy
{
    static constexpr buffer_growth_strategy strategy =
        buffer_growth_strategy::exponential;
    static constexpr bool        can_grow     = true;
    static constexpr std::size_t min_capacity = 64;

    // compute
    //   returns the smallest power-of-two-like capacity
    // >= _required, starting from _current doubled.
    static constexpr std::size_t
    compute(std::size_t _current,
            std::size_t _required) noexcept
    {
        std::size_t cap = (_current > 0)
            ? _current
            : min_capacity;

        while (cap < _required)
        {
            cap *= 2;
        }

        return cap;
    }
};

// page_growth_policy
//   struct: grow in multiples of the OS page size.
// The page size defaults to 4096 bytes but can be
// overridden at compile time.
template<std::size_t _PageSize = 4096>
struct page_growth_policy
{
    static constexpr buffer_growth_strategy strategy =
        buffer_growth_strategy::page_aligned;
    static constexpr bool        can_grow  = true;
    static constexpr std::size_t page_size = _PageSize;

    static_assert(_PageSize > 0 &&
                  (_PageSize & (_PageSize - 1)) == 0,
        "page_growth_policy: _PageSize must be a "
        "positive power of two.");

    // compute
    //   returns the smallest page-aligned capacity
    // >= _required.
    static constexpr std::size_t
    compute(std::size_t /*_current*/,
            std::size_t  _required) noexcept
    {
        // round up to next page boundary
        return (_required + _PageSize - 1)
            & ~(_PageSize - 1);
    }
};


// =============================================================================
// IV.  Cursor Policies
// =============================================================================
// Cursor policies govern how many position cursors a
// buffer maintains and what operations are available on
// them.  Each policy exposes a cursor_state type that
// the buffer_base stores, plus static helpers for cursor
// arithmetic.

// write_only_cursor_policy
//   struct: single write cursor.  The consumer retrieves
// the accumulated data via data()/size() and manages any
// read offset externally.
struct write_only_cursor_policy
{
    static constexpr buffer_cursor_model model = buffer_cursor_model::write_only;
    static constexpr bool has_read_cursor     = false;

    struct cursor_state
    {
        std::size_t write_pos;
    };

    // init
    //   initializes the cursor state to the beginning.
    static constexpr cursor_state
    init() noexcept
    {
        return { 0 };
    }

    // writable
    //   returns the number of bytes available for
    // writing between the write cursor and capacity.
    static constexpr std::size_t
    writable(
        const cursor_state& _state,
        std::size_t         _capacity
    ) noexcept
    {
        return _capacity - _state.write_pos;
    }

    // advance_write
    //   advances the write cursor by _n bytes.
    static constexpr void
    advance_write(cursor_state& _state,
                  std::size_t   _n) noexcept
    {
        _state.write_pos += _n;

        return;
    }

    // written
    //   returns the total number of bytes written.
    static constexpr std::size_t
    written(const cursor_state& _state) noexcept
    {
        return _state.write_pos;
    }

    // reset
    //   resets the write cursor to the beginning
    // without releasing storage.
    static constexpr void
    reset(cursor_state& _state) noexcept
    {
        _state.write_pos = 0;

        return;
    }
};

// dual_cursor_policy
//   struct: paired write + read cursors.  Supports
// incremental production and consumption.  The readable
// region is [read_pos, write_pos); the writable region
// is [write_pos, capacity).
struct dual_cursor_policy
{
    static constexpr buffer_cursor_model model =
        buffer_cursor_model::dual;
    static constexpr bool has_read_cursor = true;

    struct cursor_state
    {
        std::size_t write_pos;
        std::size_t read_pos;
    };

    // init
    //   initializes both cursors to the beginning.
    static constexpr cursor_state
    init() noexcept
    {
        return { 0, 0 };
    }

    // writable
    //   returns the number of bytes available for
    // writing between the write cursor and capacity.
    static constexpr std::size_t
    writable(const cursor_state& _state,
             std::size_t         _capacity) noexcept
    {
        return _capacity - _state.write_pos;
    }

    // readable
    //   returns the number of unconsumed bytes between
    // the read cursor and the write cursor.
    static constexpr std::size_t
    readable(const cursor_state& _state) noexcept
    {
        return _state.write_pos - _state.read_pos;
    }

    // advance_write
    //   advances the write cursor by _n bytes.
    static constexpr void
    advance_write(cursor_state& _state,
                  std::size_t   _n) noexcept
    {
        _state.write_pos += _n;

        return;
    }

    // advance_read
    //   advances the read cursor by _n bytes.
    static constexpr void
    advance_read(cursor_state& _state,
                 std::size_t   _n) noexcept
    {
        _state.read_pos += _n;

        return;
    }

    // written
    //   returns the total number of bytes written.
    static constexpr std::size_t
    written(const cursor_state& _state) noexcept
    {
        return _state.write_pos;
    }

    // consumed
    //   returns the total number of bytes consumed.
    static constexpr std::size_t
    consumed(const cursor_state& _state) noexcept
    {
        return _state.read_pos;
    }

    // reset
    //   resets both cursors to the beginning without
    // releasing storage.
    static constexpr void
    reset(cursor_state& _state) noexcept
    {
        _state.write_pos = 0;
        _state.read_pos  = 0;

        return;
    }

    // compact
    //   shifts unconsumed data to the front of the
    // buffer and resets both cursors accordingly.
    // Returns the number of bytes that were shifted.
    static std::size_t
    compact(cursor_state& _state,
            char*         _data) noexcept
    {
        std::size_t pending = readable(_state);

        if ( (pending > 0)       &&
             (_state.read_pos > 0) )
        {
            std::memmove(_data,
                         _data + _state.read_pos,
                         pending);
        }

        _state.read_pos  = 0;
        _state.write_pos = pending;

        return pending;
    }
};


// =============================================================================
// V.   buffer_base (CRTP)
// =============================================================================
// Provides storage-agnostic cursor arithmetic, state
// predicates, and growth dispatch for any buffer.
//
// The derived class must expose:
//   - char*       storage()           - writable pointer
//   - const char* storage() const     - readable pointer
//   - std::size_t capacity() const    - usable capacity
//   - bool        grow(std::size_t)   - grow usable
//                                       capacity to at
//                                       least N bytes
//
// "Usable capacity" is the number of bytes available for
// content.  The derived class may reserve additional raw
// bytes (e.g. a text buffer reserves +1 for a null
// terminator); buffer_base does not know or care about
// that distinction.
//
// The derived class owns the memory.  buffer_base never
// allocates or frees.

template<typename _Derived,
         typename _GrowthPolicy,
         typename _CursorPolicy>
class buffer_base
{
protected:
    buffer_base() noexcept
        : m_cursors(_CursorPolicy::init())
    {}

    ~buffer_base() = default;

    // non-copyable by default (derived may override)
    buffer_base(const buffer_base&)            = delete;
    buffer_base& operator=(const buffer_base&) = delete;

    // movable
    buffer_base(buffer_base&& _other) noexcept
        : m_cursors(_other.m_cursors)
    {
        _CursorPolicy::reset(_other.m_cursors);
    }

    buffer_base& operator=(buffer_base&& _other) noexcept
    {
        if (this != &_other)
        {
            m_cursors = _other.m_cursors;
            _CursorPolicy::reset(_other.m_cursors);
        }

        return *this;
    }

private:
    _Derived& self()
    {
        return static_cast<_Derived&>(*this);
    }

    const _Derived& self() const
    {
        return static_cast<const _Derived&>(*this);
    }

public:
    // --- policy types ---

    using growth_policy = _GrowthPolicy;
    using cursor_policy = _CursorPolicy;
    using cursor_state  =
        typename _CursorPolicy::cursor_state;

    // --- capacity and growth constants ---

    static constexpr buffer_growth_strategy growth_strategy =
        _GrowthPolicy::strategy;

    static constexpr buffer_cursor_model cursor_model =
        _CursorPolicy::model;

    static constexpr bool can_grow =
        _GrowthPolicy::can_grow;

    static constexpr bool has_read_cursor =
        _CursorPolicy::has_read_cursor;

    // --- state predicates ---

    // empty
    //   returns true if no data has been written, or all
    // written data has been consumed (dual-cursor mode).
    bool empty() const noexcept
    {
        if constexpr (_CursorPolicy::has_read_cursor)
        {
            return _CursorPolicy::readable(
                       m_cursors) == 0;
        }
        else
        {
            return _CursorPolicy::written(
                       m_cursors) == 0;
        }
    }

    // full
    //   returns true if the write cursor has reached
    // capacity and the growth policy does not allow
    // expansion.
    bool full() const noexcept
    {
        return _CursorPolicy::writable(
                   m_cursors,
                   self().capacity()) == 0
            && !_GrowthPolicy::can_grow;
    }

    // size
    //   returns the number of meaningful bytes.
    // Write-only: total bytes written.
    // Dual-cursor: unconsumed bytes (write - read).
    std::size_t size() const noexcept
    {
        if constexpr (_CursorPolicy::has_read_cursor)
        {
            return _CursorPolicy::readable(m_cursors);
        }
        else
        {
            return _CursorPolicy::written(m_cursors);
        }
    }

    // writable
    //   returns the number of bytes that can be written
    // without triggering growth.
    std::size_t writable() const noexcept
    {
        return _CursorPolicy::writable(
            m_cursors, self().capacity());
    }

    // write_position
    //   returns the current write cursor offset.
    std::size_t write_position() const noexcept
    {
        return _CursorPolicy::written(m_cursors);
    }

    // --- write operations ---

    // write
    //   appends _n bytes from _src to the buffer,
    // growing if necessary and permitted.  Returns the
    // number of bytes actually written (may be less than
    // _n if the buffer is fixed and full).
    std::size_t
    write(
        const void* _src,
        std::size_t _n
    ) noexcept
    {
        if ( (!_src) || 
             (_n == 0) )
        {
            return 0;
        }

        // ensure capacity
        if (!ensure_writable(_n))
        {
            // growth failed - write what fits
            _n = _CursorPolicy::writable(
                     m_cursors, self().capacity());

            if (_n == 0)
            {
                return 0;
            }
        }

        std::memcpy(
            self().storage()
                + _CursorPolicy::written(m_cursors),
            _src,
            _n);

        _CursorPolicy::advance_write(m_cursors, _n);

        return _n;
    }

    // write_byte
    //   appends a single byte to the buffer, growing if
    // necessary.  Returns true on success.
    bool write_byte(
        char _byte
    ) noexcept
    {
        return write(&_byte, 1) == 1;
    }

    // write_fill
    //   appends _n copies of _byte to the buffer.
    // Returns the number of bytes actually written.
    std::size_t
    write_fill(
        char        _byte,
        std::size_t _n
    ) noexcept
    {
        if (_n == 0)
        {
            return 0;
        }

        if (!ensure_writable(_n))
        {
            _n = _CursorPolicy::writable(
                     m_cursors, self().capacity());

            if (_n == 0)
            {
                return 0;
            }
        }

        std::memset(
            self().storage()
                + _CursorPolicy::written(m_cursors),
            static_cast<unsigned char>(_byte),
            _n);

        _CursorPolicy::advance_write(m_cursors, _n);

        return _n;
    }

    // --- read operations (dual-cursor only) ---

    // read
    //   copies up to _n bytes from the read cursor into
    // _dst and advances the read cursor.  Returns the
    // number of bytes actually read.
    // Only available when has_read_cursor is true.
    template<typename _CP = _CursorPolicy>
    std::enable_if_t<_CP::has_read_cursor, std::size_t>
    read(void*       _dst,
         std::size_t _n) noexcept
    {
        if ( (!_dst) || (_n == 0) )
        {
            return 0;
        }

        std::size_t avail =
            _CursorPolicy::readable(m_cursors);

        if (_n > avail)
        {
            _n = avail;
        }

        std::memcpy(
            _dst,
            self().storage() + m_cursors.read_pos,
            _n);

        _CursorPolicy::advance_read(m_cursors, _n);

        return _n;
    }

    // peek
    //   returns a const pointer to the unconsumed data
    // at the read cursor without advancing it.
    // Only available when has_read_cursor is true.
    template<typename _CP = _CursorPolicy>
    std::enable_if_t<_CP::has_read_cursor, const char*>
    peek() const noexcept
    {
        return self().storage() + m_cursors.read_pos;
    }

    // read_position
    //   returns the current read cursor offset.
    // Only available when has_read_cursor is true.
    template<typename _CP = _CursorPolicy>
    std::enable_if_t<_CP::has_read_cursor, std::size_t>
    read_position() const noexcept
    {
        return _CursorPolicy::consumed(m_cursors);
    }

    // readable
    //   returns the number of unconsumed bytes available
    // for reading.
    // Only available when has_read_cursor is true.
    template<typename _CP = _CursorPolicy>
    std::enable_if_t<_CP::has_read_cursor, std::size_t>
    readable() const noexcept
    {
        return _CursorPolicy::readable(m_cursors);
    }

    // advance
    //   advances the read cursor by _n bytes without
    // copying.  Returns the number of bytes actually
    // advanced (clamped to available).
    // Only available when has_read_cursor is true.
    template<typename _CP = _CursorPolicy>
    std::enable_if_t<_CP::has_read_cursor, std::size_t>
    advance(std::size_t _n) noexcept
    {
        std::size_t avail =
            _CursorPolicy::readable(m_cursors);

        if (_n > avail)
        {
            _n = avail;
        }

        _CursorPolicy::advance_read(m_cursors, _n);

        return _n;
    }

    // compact
    //   shifts unconsumed data to the front, freeing
    // space at the tail for more writes.  Returns the
    // number of unconsumed bytes that remain.
    // Only available when has_read_cursor is true.
    template<typename _CP = _CursorPolicy>
    std::enable_if_t<_CP::has_read_cursor, std::size_t>
    compact() noexcept
    {
        return _CursorPolicy::compact(
            m_cursors, self().storage());
    }

    // --- data access ---

    // write_head
    //   returns a mutable pointer to the current write
    // position.  The caller may write directly and then
    // call commit() to advance the cursor.
    char* write_head() noexcept
    {
        return self().storage()
            + _CursorPolicy::written(m_cursors);
    }

    // commit
    //   advances the write cursor by _n bytes after a
    // direct write to write_head().  The caller must
    // ensure _n <= writable().
    void commit(std::size_t _n) noexcept
    {
        _CursorPolicy::advance_write(m_cursors, _n);

        return;
    }

    // --- buffer lifecycle ---

    // reset
    //   resets all cursors to zero without releasing
    // storage.  The buffer can be reused immediately.
    void reset() noexcept
    {
        _CursorPolicy::reset(m_cursors);

        return;
    }

    // clear
    //   resets cursors and zeroes the storage region.
    void clear() noexcept
    {
        _CursorPolicy::reset(m_cursors);

        if (self().capacity() > 0)
        {
            std::memset(self().storage(),
                        0,
                        self().capacity());
        }

        return;
    }

    // --- capacity management ---

    // reserve
    //   ensures the buffer has at least _capacity bytes
    // of total storage.  Does nothing if capacity is
    // already sufficient.  Returns true on success,
    // false if growth is not permitted or fails.
    bool reserve(std::size_t _capacity) noexcept
    {
        if (_capacity <= self().capacity())
        {
            return true;
        }

        if constexpr (!_GrowthPolicy::can_grow)
        {
            return false;
        }
        else
        {
            std::size_t new_cap =
                _GrowthPolicy::compute(
                    self().capacity(), _capacity);

            return self().grow(new_cap);
        }
    }

protected:
    // ensure_writable
    //   ensures that at least _n bytes are available for
    // writing, growing the buffer if necessary and
    // permitted.  Returns true if the space is available.
    bool ensure_writable(std::size_t _n) noexcept
    {
        std::size_t avail =
            _CursorPolicy::writable(
                m_cursors, self().capacity());

        if (avail >= _n)
        {
            return true;
        }

        if constexpr (!_GrowthPolicy::can_grow)
        {
            return false;
        }
        else
        {
            std::size_t needed =
                _CursorPolicy::written(m_cursors) + _n;

            std::size_t new_cap =
                _GrowthPolicy::compute(
                    self().capacity(), needed);

            return self().grow(new_cap);
        }
    }

    // --- cursor state ---
    cursor_state m_cursors;
};


// =============================================================================
// VI.  Growth Policy Selection
// =============================================================================
// Compile-time selection of growth policy by enum value.

template<buffer_growth_strategy _Strategy>
struct select_growth_policy;

template<>
struct select_growth_policy<buffer_growth_strategy::none>
{
    using type = fixed_growth_policy;
};

template<>
struct select_growth_policy<buffer_growth_strategy::linear>
{
    using type = linear_growth_policy<>;
};

template<>
struct select_growth_policy<buffer_growth_strategy::exponential>
{
    using type = exponential_growth_policy;
};

template<>
struct select_growth_policy<buffer_growth_strategy::page_aligned>
{
    using type = page_growth_policy<>;
};

template<buffer_growth_strategy _Strategy>
using select_growth_policy_t =
    typename select_growth_policy<_Strategy>::type;


// =============================================================================
// VII. Cursor Policy Selection
// =============================================================================
// Compile-time selection of cursor policy by enum value.

template<buffer_cursor_model _Model>
struct select_cursor_policy;

template<>
struct select_cursor_policy<buffer_cursor_model::write_only>
{
    using type = write_only_cursor_policy;
};

template<>
struct select_cursor_policy<buffer_cursor_model::dual>
{
    using type = dual_cursor_policy;
};

template<buffer_cursor_model _Model>
using select_cursor_policy_t = typename select_cursor_policy<_Model>::type;


// =============================================================================
// VIII. Default Policy Aliases
// =============================================================================
// Sensible defaults: exponential growth with dual cursors.
// These can be used as default template arguments by
// concrete buffer implementations.

using default_growth_policy = exponential_growth_policy;
using default_cursor_policy = dual_cursor_policy;


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_BUFFER_