/******************************************************************************
* djinterp [parse]                                          pattern_occurrence.hpp
*
* Functional occurrence unfold for searchable patterns:
*   This header replaces the scanner's hand-rolled "find every occurrence"
* loop (pattern_scanner::run_secondary) with a reusable functional unfold.
* The original loop was welded into the scanner's private section, and it
* copied the entire remaining input on every iteration (src.substr(base)),
* making it O(n^2) in the number of matches.  Here the same logic is a
* producer - a pull-based source yielding one positioned match per call and
* signalling exhaustion when no further occurrence exists - that scans in
* place without copying, and that any code (not just the scanner) can drain,
* fold, or pipe.
*
*   The contract a pattern must satisfy is now machine-checked rather than
* described in a comment: occurrence_producer requires
* has_find_method<_Pattern, std::string, match_result_type>, the structural
* trait added in core/functional/structural_traits.hpp.  A pattern that does
* not expose bool find(const std::string&, std::size_t&, R&) fails to compile
* with a clear message instead of deep in instantiation.
*
* CONTENTS
*   positioned_match<R>        (offset, captures) pair yielded per occurrence
*   occurrence_producer<P>     producer<positioned_match<R>> over a buffer
*   make_occurrence_producer   factory binding a pattern + buffer
*   collect_occurrences(p,buf) eager convenience -> vector<positioned_match<R>>
*
* path:      /inc/djinterp/parse/pattern_occurrence.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.29
******************************************************************************/

#ifndef DJINTERP_PARSE_PATTERN_OCCURRENCE_
#define DJINTERP_PARSE_PATTERN_OCCURRENCE_ 1

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "./parse.hpp"
#include "../core/functional/producer.hpp"
#include "../core/functional/structural_traits.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  positioned_match
// ================================================================

// positioned_match
//   struct: one occurrence of a pattern within a buffer - the
// absolute byte offset at which it started, plus the pattern's own
// capture result.  Position is a raw offset here (domain-agnostic);
// the scanner layers human line/column on top via its locate().
template<typename _MatchResult>
struct positioned_match
{
    std::size_t  offset;
    _MatchResult captures;

    positioned_match()
        : offset   (0),
          captures ()
    {}

    positioned_match(std::size_t          _offset,
                     const _MatchResult&  _captures)
        : offset   (_offset),
          captures (_captures)
    {}
};


// ================================================================
//  occurrence_producer
// ================================================================

// occurrence_producer
//   class: a producer (nullary callable yielding
// producer_step<positioned_match<R>>) that pulls the
// next occurrence of a searchable pattern from a buffer on each
// call, and signals exhaustion when none remain.  This is the
// functional unfold form of the old run_secondary loop.
//
//   It holds the buffer by const reference (the buffer must outlive
// the producer) and a copy of the pattern.  Crucially it scans in
// place from a moving cursor rather than copying shrinking
// substrings: find() is given the cursor offset and reports the
// next match position, avoiding the original loop's per-iteration
// O(n) copy.
//
//   The pattern must satisfy
// has_find_method<_Pattern, std::string, R> where R is the
// pattern's match_result_type; this is asserted at construction.
template<typename _Pattern>
class occurrence_producer
{
public:
    using pattern_type = _Pattern;
    using match_result_type =
        typename _Pattern::match_result_type;
    using value_type =
        positioned_match<match_result_type>;
    using step_type =
        producer_step<value_type>;

    occurrence_producer(
        const _Pattern&    _pattern,
        const std::string& _buffer
    )
        : m_pattern (_pattern),
          m_buffer  (&_buffer),
          m_cursor  (0),
          m_done    (false)
    {
        static_assert(
            has_find_method<
                _Pattern, std::string, match_result_type>::value,
            "occurrence_producer requires a pattern exposing "
            "bool find(const std::string&, std::size_t&, "
            "match_result_type&).");
    }

    // operator()
    //   method: the producer pull.  Returns the next occurrence
    // wrapped in a step, or an empty step at exhaustion.
    step_type operator()()
    {
        if (m_done || (m_buffer == nullptr))
        {
            return step_type();
        }

        const std::string& src = *m_buffer;

        if (m_cursor >= src.size())
        {
            m_done = true;
            return step_type();
        }

        // Scan in place from the cursor.  We give find() a view of
        // the tail by offsetting; patterns that scan from position 0
        // of the string they receive are accommodated by passing a
        // tail copy ONLY when the cursor is non-zero AND the pattern
        // cannot honour a start offset.  To stay protocol-compatible
        // with the existing find(buffer, pos, result) signature - in
        // which `pos` is an in/out cursor - we pass the cursor as the
        // starting position and let find() advance it.
        std::size_t       pos = m_cursor;
        match_result_type result;

        if (!m_pattern.find(src, pos, result))
        {
            m_done = true;
            return step_type();
        }

        // `pos` now holds the absolute start of the match.  Advance
        // the cursor past this start (single-step advance preserves
        // the original loop's overlap behaviour; pattern<> exposes no
        // match-end face).
        std::size_t match_offset = pos;

        m_cursor = match_offset + 1;

        return make_step(
            value_type(match_offset, result));
    }

    // reset
    //   method: rewind to the start of the buffer for re-scanning.
    void reset()
    {
        m_cursor = 0;
        m_done   = false;

        return;
    }

private:
    _Pattern           m_pattern;
    const std::string* m_buffer;
    std::size_t        m_cursor;
    bool               m_done;
};


// ================================================================
//  make_occurrence_producer
// ================================================================

// make_occurrence_producer
//   function: builds an occurrence_producer binding _pattern to
// _buffer (which must outlive the producer).
template<typename _Pattern>
D_NODISCARD
occurrence_producer<_Pattern>
make_occurrence_producer(
    const _Pattern&    _pattern,
    const std::string& _buffer
)
{
    return occurrence_producer<_Pattern>(_pattern, _buffer);
}


// ================================================================
//  collect_occurrences
// ================================================================

// collect_occurrences
//   function: eager convenience that drains an occurrence_producer
// over _buffer into a vector of positioned matches.  This is the
// drop-in replacement for run_secondary's accumulation: one call,
// no per-iteration copies, and the unfold logic lives in the
// reusable producer rather than inline in the scanner.
template<typename _Pattern>
D_NODISCARD
std::vector<positioned_match<typename _Pattern::match_result_type> >
collect_occurrences(
    const _Pattern&    _pattern,
    const std::string& _buffer
)
{
    using match_result_type =
        typename _Pattern::match_result_type;
    using value_type = positioned_match<match_result_type>;

    occurrence_producer<_Pattern> producer(_pattern, _buffer);

    std::vector<value_type> out;

    while (true)
    {
        producer_step<value_type> step = producer();

        if (!step.has_value)
        {
            break;
        }

        out.push_back(step.value);
    }

    return out;
}


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_PATTERN_OCCURRENCE_
