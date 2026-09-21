/*******************************************************************************
* djinterp [parse]                                               diagnostic.hpp
*
*   The C++ face of the diagnostic channel declared in diagnostic.h.
*   Every type here IS its C counterpart: `span` derives from d_parse_span and
* `diagnostics` from d_parse_diag_sink, neither adds a data member, and both
* are asserted layout-identical to their base. A C++ sink is therefore passed
* to a C stage by taking its address, with no conversion, no wrapper object,
* and no copy -- which is what makes this a face rather than a binding.
*
*   What the C++ side adds is lifetime and iteration: `diagnostics` releases
* what it owns, `fixed_diagnostics<N, M>` carries its own storage so a stage
* that must not allocate simply declares one, and a sink iterates as a range of
* views. A view pairs a record with the sink that holds its text, because a
* diagnostic's message is an arena offset and means nothing without it.
*
* path:      /inc/djinterp/parse/diagnostic.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VALUE TYPES
    -----------
    1.  Severity
         1.  severity
         2.  name
    2.  Location
         1.  span
    3.  Records
         1.  diagnostic
         2.  diagnostic_view
2.  THE SINK
    --------
    1.  Iteration
         1.  diagnostic_iterator
    2.  The sink
         1.  diagnostics
    3.  Self-contained storage
         1.  fixed_diagnostics
    4.  Layout guarantees
*/

#ifndef DJINTERP_PARSE_DIAGNOSTIC_HPP_
#define DJINTERP_PARSE_DIAGNOSTIC_HPP_ 1

// std
#include <cstddef>              // std::size_t, std::ptrdiff_t
#include <cstdint>              // std::uint8_t, std::uint16_t, std::uint32_t
#include <iterator>             // std::forward_iterator_tag
#include <type_traits>          // std::is_standard_layout
// djinterp
#include "../djinterp.hpp"      // framework root
#include "./diagnostic.h"       // the C facility this layer faces


// D_KEYWORD_PARSE
//   keyword: resolves to `parse`.  Guarded rather than owned -- parse.hpp is
// its canonical home, and this spelling only fires when the substrate is built
// without it.
#ifndef D_KEYWORD_PARSE
    #define D_KEYWORD_PARSE             parse
#endif

// NS_PARSE
//   namespace: the parse subsystem namespace.  Guarded for the same reason.
#ifndef NS_PARSE
    #define NS_PARSE                    D_NAMESPACE(D_KEYWORD_PARSE)
#endif


NS_DJINTERP
NS_PARSE


//==============================================================================
// 1.  VALUE TYPES
//==============================================================================


// 1.1    Severity
//------------------------------------------------------------------------------
// 1.1.1
// severity
//   enum: how badly a diagnostic bears on the run.  A scoped enum over the C
// codes, with the same underlying width, so it converts both ways for free.
enum class severity : std::uint8_t
{
    note    = D_PARSE_SEVERITY_NOTE,
    warning = D_PARSE_SEVERITY_WARNING,
    error   = D_PARSE_SEVERITY_ERROR,
    fatal   = D_PARSE_SEVERITY_FATAL
};

// 1.1.2
// name
//   function: the lowercase display name of a severity.
inline const char*
name(
    severity _severity
)
{
    return d_parse_severity_name(static_cast<int>(_severity));
}


// 1.2    Location
//------------------------------------------------------------------------------
// 1.2.1
// span
//   struct: a half-open byte range within one source.  Adds constructors to
// d_parse_span and nothing else, so it remains trivially copyable and is
// passed to the C emitters by slicing to its base at no cost.
struct span : d_parse_span
{
    // span
    //   constructor: a span that points nowhere.
    constexpr span() noexcept
        : d_parse_span{ 0u, 0u, 0u, 0u, 0u }
    {}

    // span
    //   constructor: adopts a C span unchanged.
    constexpr span(
        const d_parse_span& _span
    ) noexcept
        : d_parse_span(_span)
    {}

    // span
    //   constructor: a byte range of the primary source, with no line or
    // column resolved.
    constexpr span(
        std::uint32_t _offset,
        std::uint32_t _length
    ) noexcept
        : d_parse_span{ 0u, _offset, _length, 0u, 0u }
    {}

    // span
    //   constructor: a fully described range.
    constexpr span(
        std::uint32_t _source,
        std::uint32_t _offset,
        std::uint32_t _length,
        std::uint32_t _line,
        std::uint32_t _column
    ) noexcept
        : d_parse_span{ _source, _offset, _length, _line, _column }
    {}

    // at
    //   function: a zero-length span at one offset.
    static constexpr span
    at(
        std::uint32_t _offset
    ) noexcept
    {
        return span(_offset, 0u);
    }

    // located
    //   accessor: whether a line was resolved for this span.
    constexpr bool
    located() const noexcept
    {
        return (line > 0u);
    }
};


// 1.3    Records
//------------------------------------------------------------------------------
// 1.3.1
// diagnostic
//   type: the stored record, unchanged.  It is a position-independent POD by
// design and gains nothing from a wrapper; what a reader wants is the view
// below, which can resolve the message.
using diagnostic = ::d_parse_diagnostic;

// 1.3.2
// diagnostic_view
//   class: a record paired with the sink that holds its text.  Non-owning and
// pointer-sized twice over; every accessor inlines to the field read it wraps.
class diagnostic_view
{
public:
    // diagnostic_view
    //   constructor: binds a record to its sink.  Either may be null, in which
    // case every accessor returns its documented empty answer.
    constexpr diagnostic_view(
        const d_parse_diag_sink*  _sink,
        const d_parse_diagnostic* _record
    ) noexcept
        : m_sink(_sink),
          m_record(_record)
    {}

    // valid
    //   accessor: whether this view refers to a record at all.
    constexpr bool
    valid() const noexcept
    {
        return (m_record != nullptr);
    }

    // level
    //   accessor: the severity of the record.
    constexpr severity
    level() const noexcept
    {
        return (m_record != nullptr)
               ? static_cast<severity>(m_record->severity)
               : severity::note;
    }

    // domain
    //   accessor: the domain of the stage that emitted the record.
    constexpr std::uint16_t
    domain() const noexcept
    {
        return (m_record != nullptr) ? m_record->domain : std::uint16_t(0);
    }

    // code
    //   accessor: the condition, in that domain's private code space.
    constexpr std::uint16_t
    code() const noexcept
    {
        return (m_record != nullptr) ? m_record->code : std::uint16_t(0);
    }

    // where
    //   accessor: the source range the record points at.
    constexpr span
    where() const noexcept
    {
        return (m_record != nullptr) ? span(m_record->span) : span();
    }

    // continuation
    //   accessor: whether this record elaborates the one before it.
    constexpr bool
    continuation() const noexcept
    {
        return ( (m_record != nullptr) &&
                 ((m_record->flags & D_PARSE_DIAG_FLAG_CONTINUATION) != 0u) );
    }

    // text
    //   accessor: the message, resolved against the owning sink's arena.
    // Never null; the empty string when no text was stored.
    const char*
    text() const noexcept
    {
        return d_parse_diag_message(m_sink, m_record);
    }

    // render
    //   function: writes the default one-line rendering into a caller buffer.
    // Returns the length the full line would occupy, as snprintf reports it.
    std::size_t
    render(
        char*       _out,
        std::size_t _size
    ) const noexcept
    {
        return d_parse_diag_format(m_sink, m_record, _out, _size);
    }

    // record
    //   accessor: the underlying POD.
    constexpr const d_parse_diagnostic*
    record() const noexcept
    {
        return m_record;
    }

private:
    const d_parse_diag_sink*  m_sink;
    const d_parse_diagnostic* m_record;
};


//==============================================================================
// 2.  THE SINK
//==============================================================================


// 2.1    Iteration
//------------------------------------------------------------------------------
// 2.1.1
// diagnostic_iterator
//   class: a forward iterator over a sink's stored records, yielding views.
// Holds an index rather than a pointer, so it never forms a pointer to a
// derived type over an array of the base.
class diagnostic_iterator
{
public:
    using value_type        = diagnostic_view;
    using difference_type   = std::ptrdiff_t;
    using reference         = diagnostic_view;
    using iterator_category = std::forward_iterator_tag;

    // diagnostic_iterator
    //   constructor: positions the iterator at one index of a sink.
    constexpr diagnostic_iterator(
        const d_parse_diag_sink* _sink,
        std::uint32_t            _index
    ) noexcept
        : m_sink(_sink),
          m_index(_index)
    {}

    // operator*
    //   function: the view at the current position.
    diagnostic_view
    operator*() const noexcept
    {
        return diagnostic_view(m_sink, d_parse_diag_at(m_sink, m_index));
    }

    // operator++
    //   function: advances to the next stored record.
    constexpr diagnostic_iterator&
    operator++() noexcept
    {
        ++m_index;

        return *this;
    }

    // operator++
    //   function: advances, yielding the previous position.
    constexpr diagnostic_iterator
    operator++(int) noexcept
    {
        const diagnostic_iterator previous = *this;

        ++m_index;

        return previous;
    }

    // operator==
    //   function: whether two iterators sit at the same position.
    constexpr bool
    operator==(
        const diagnostic_iterator& _other
    ) const noexcept
    {
        return ( (m_sink == _other.m_sink) &&
                 (m_index == _other.m_index) );
    }

    // operator!=
    //   function: the negation of operator==.
    constexpr bool
    operator!=(
        const diagnostic_iterator& _other
    ) const noexcept
    {
        return !(*this == _other);
    }

private:
    const d_parse_diag_sink* m_sink;
    std::uint32_t            m_index;
};


// 2.2    The sink
//------------------------------------------------------------------------------
// 2.2.1
// diagnostics
//   class: the collection point, with lifetime.  Derives from the C sink and
// adds no data member, so `&sink` is a d_parse_diag_sink* the moment a C stage
// asks for one.  Move-only: two owners of one arena is not a thing worth
// supporting, and a copy would silently double the cost of the cheap case.
class diagnostics : public d_parse_diag_sink
{
public:
    using iterator       = diagnostic_iterator;
    using const_iterator = diagnostic_iterator;

    // diagnostics
    //   constructor: a sink with no storage.  It still tallies, so it answers
    // failed() correctly while allocating and storing nothing.
    diagnostics() noexcept
    {
        d_parse_diag_sink_init(this, nullptr, 0u, nullptr, 0u);
    }

    // diagnostics
    //   constructor: a sink over caller-supplied storage.
    diagnostics(
        d_parse_diagnostic* _items,
        std::uint32_t       _item_count,
        char*               _text,
        std::uint32_t       _text_bytes
    ) noexcept
    {
        d_parse_diag_sink_init(this,
                               _items,
                               _item_count,
                               _text,
                               _text_bytes);
    }

    diagnostics(const diagnostics&)            = delete;
    diagnostics& operator=(const diagnostics&) = delete;

    // diagnostics
    //   constructor: takes over another sink's storage and ownership.
    diagnostics(
        diagnostics&& _other
    ) noexcept
        : d_parse_diag_sink(_other)
    {
        d_parse_diag_sink_init(&_other, nullptr, 0u, nullptr, 0u);
    }

    // operator=
    //   function: releases this sink, then takes over another's.
    diagnostics&
    operator=(
        diagnostics&& _other
    ) noexcept
    {
        // guard against self-move, which would release the storage being taken
        if (this != &_other)
        {
            d_parse_diag_sink_release(this);

            static_cast<d_parse_diag_sink&>(*this) = _other;

            d_parse_diag_sink_init(&_other, nullptr, 0u, nullptr, 0u);
        }

        return *this;
    }

    // ~diagnostics
    //   destructor: releases any storage this sink owns.
    ~diagnostics() noexcept
    {
        d_parse_diag_sink_release(this);
    }

#if (D_INTERNAL_PARSE_DIAG_HEAP == 1)
    // reserve
    //   function: replaces this sink's storage with storage it allocates and
    // owns.  Returns false if the allocation was refused, leaving the sink
    // valid and empty.
    D_NODISCARD bool
    reserve(
        std::uint32_t _item_count = 0u,
        std::uint32_t _text_bytes = 0u
    ) noexcept
    {
        d_parse_diag_sink_release(this);

        return (d_parse_diag_sink_init_heap(this,
                                            _item_count,
                                            _text_bytes) == 0);
    }
#endif  // D_INTERNAL_PARSE_DIAG_HEAP

    // emit
    //   function: accepts a diagnostic.  Returns false only when the sink
    // rejected it outright -- a stored-or-not distinction is truncated().
    bool
    emit(
        severity      _severity,
        std::uint16_t _domain,
        std::uint16_t _code,
        const span&   _span,
        const char*   _message
    ) noexcept
    {
        return (d_parse_diag_emit(this,
                                  static_cast<int>(_severity),
                                  _domain,
                                  _code,
                                  _span,
                                  _message) == 0);
    }

    // note
    //   function: accepts a note that elaborates the preceding diagnostic.
    bool
    note(
        std::uint16_t _domain,
        std::uint16_t _code,
        const span&   _span,
        const char*   _message
    ) noexcept
    {
        return (d_parse_diag_emit_flagged(this,
                                          D_PARSE_SEVERITY_NOTE,
                                          _domain,
                                          _code,
                                          D_PARSE_DIAG_FLAG_CONTINUATION,
                                          _span,
                                          _message) == 0);
    }

    // warning
    //   function: accepts a warning.
    bool
    warning(
        std::uint16_t _domain,
        std::uint16_t _code,
        const span&   _span,
        const char*   _message
    ) noexcept
    {
        return emit(severity::warning, _domain, _code, _span, _message);
    }

    // error
    //   function: accepts an error.
    bool
    error(
        std::uint16_t _domain,
        std::uint16_t _code,
        const span&   _span,
        const char*   _message
    ) noexcept
    {
        return emit(severity::error, _domain, _code, _span, _message);
    }

    // fatal
    //   function: accepts a fatal diagnostic.
    bool
    fatal(
        std::uint16_t _domain,
        std::uint16_t _code,
        const span&   _span,
        const char*   _message
    ) noexcept
    {
        return emit(severity::fatal, _domain, _code, _span, _message);
    }

#if (D_INTERNAL_PARSE_DIAG_FORMAT == 1)
    // emitf
    //   function: accepts a diagnostic whose message is formatted printf-style.
    template<typename... _Args>
    bool
    emitf(
        severity      _severity,
        std::uint16_t _domain,
        std::uint16_t _code,
        const span&   _span,
        const char*   _format,
        _Args...      _args
    ) noexcept
    {
        return (d_parse_diag_emitf(this,
                                   static_cast<int>(_severity),
                                   _domain,
                                   _code,
                                   _span,
                                   _format,
                                   _args...) == 0);
    }
#endif  // D_INTERNAL_PARSE_DIAG_FORMAT

    // filter
    //   function: sets the lowest severity this sink accepts, clamped so that
    // errors and fatals are always counted.
    void
    filter(
        severity _minimum
    ) noexcept
    {
        d_parse_diag_sink_filter(this, static_cast<int>(_minimum));
    }

    // on_emit
    //   function: installs the callback invoked for each accepted diagnostic.
    void
    on_emit(
        d_parse_diag_hook _hook,
        void*             _ctx = nullptr
    ) noexcept
    {
        d_parse_diag_sink_hook(this, _hook, _ctx);
    }

    // clear
    //   function: empties the sink for reuse, keeping its storage.
    void
    clear() noexcept
    {
        d_parse_diag_sink_reset(this);
    }

    // failed
    //   accessor: whether anything was reported that makes the run's output
    // unusable.
    bool
    failed() const noexcept
    {
        return (d_parse_diag_failed(this) != 0);
    }

    // truncated
    //   accessor: whether the sink dropped a record or a message it accepted.
    bool
    truncated() const noexcept
    {
        return (d_parse_diag_truncated(this) != 0);
    }

    // tally
    //   accessor: how many diagnostics of one severity were accepted, stored
    // or not.
    std::uint32_t
    tally_of(
        severity _severity
    ) const noexcept
    {
        return d_parse_diag_tally(this, static_cast<int>(_severity));
    }

    // size
    //   accessor: how many diagnostics are stored.
    constexpr std::uint32_t
    size() const noexcept
    {
        return count;
    }

    // empty
    //   accessor: whether any diagnostic is stored.
    constexpr bool
    empty() const noexcept
    {
        return (count == 0u);
    }

    // operator[]
    //   accessor: a view of the stored diagnostic at an index.
    diagnostic_view
    operator[](
        std::uint32_t _index
    ) const noexcept
    {
        return diagnostic_view(this, d_parse_diag_at(this, _index));
    }

    // begin
    //   accessor: an iterator to the first stored diagnostic.
    constexpr iterator
    begin() const noexcept
    {
        return iterator(this, 0u);
    }

    // end
    //   accessor: an iterator one past the last stored diagnostic.
    constexpr iterator
    end() const noexcept
    {
        return iterator(this, count);
    }

    // print
    //   function: writes every stored diagnostic to stdout.  Diagnostics and
    // build checks only.
    void
    print() const noexcept
    {
        d_parse_diag_print(this);
    }
};


// 2.3    Self-contained storage
//------------------------------------------------------------------------------
// 2.3.1
// fixed_diagnostics
//   class: a sink carrying its own storage, for a stage that must not
// allocate.  Declaring one is the whole of the setup.
//   Unlike `diagnostics` this is NOT layout-identical to the C sink, because
// it adds the arrays as members; it converts to d_parse_diag_sink* through its
// base as any derived class does, which is all the C side ever needs.
template<std::uint32_t _Items,
         std::uint32_t _Text>
class fixed_diagnostics : public diagnostics
{
public:
    // fixed_diagnostics
    //   constructor: binds the embedded arrays as this sink's storage.
    fixed_diagnostics() noexcept
    {
        d_parse_diag_sink_init(this, m_items, _Items, m_text, _Text);
    }

private:
    d_parse_diagnostic m_items[_Items];
    char               m_text[_Text];
};


// 2.4    Layout guarantees
//------------------------------------------------------------------------------
//   The claim this header makes is that its types cost nothing over the C
// ones.  These assertions are that claim, checked.
static_assert(sizeof(span) == sizeof(d_parse_span),
              "parse::span must be layout-identical to d_parse_span");
static_assert(alignof(span) == alignof(d_parse_span),
              "parse::span must be layout-identical to d_parse_span");
static_assert(std::is_standard_layout<span>::value,
              "parse::span must remain standard-layout");
static_assert(std::is_trivially_copyable<span>::value,
              "parse::span must remain trivially copyable");
static_assert(sizeof(diagnostics) == sizeof(d_parse_diag_sink),
              "parse::diagnostics must add no data member");
static_assert(alignof(diagnostics) == alignof(d_parse_diag_sink),
              "parse::diagnostics must add no data member");
static_assert(std::is_standard_layout<diagnostics>::value,
              "parse::diagnostics must remain standard-layout");


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_DIAGNOSTIC_HPP_
