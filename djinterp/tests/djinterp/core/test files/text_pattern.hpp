/******************************************************************************
* djinterp [text]                                             text_pattern.hpp
*
*   Marker-aware text pattern engine.  Given a template literal in the
* same dialect as text_template (default markers: %key%), text_pattern
* compiles the literal into an alternating sequence of literal and
* capture segments and exposes the four pattern faces:
*
*     1. predicate  - does input conform to the pattern?
*     2. extractor  - return the bindings the input represents.
*     3. renderer   - produce text from a binding map.
*     4. rewrite    - splice a new value for one key into existing text,
*                     preserving every other capture.
*
*   text_pattern is the inverse of text_template: a single template
* literal is the canonical specification, and the two engines consume
* it in opposite directions.  This lets the same literal drive both
* generation (writing new files) and parsing (verifying existing ones).
*
*   MARKER SYNTAX:
*   Markers default to "%" / "%" matching text_template's defaults.
* They are configurable via set_markers().  An escape character
* (default: '\') preceding a prefix marker suppresses capture and
* matches the prefix literally.
*
*   CAPTURE BOUNDARIES:
*   Captures are lazy - a capture extends from its start position
* up to the next literal segment.  Two adjacent captures (with no
* literal between them) are malformed and rejected at compile time.
*
*   USAGE:
*     text_pattern p(
*         "path:      %relative_path%/%filename%.%fileext%");
*
*     // predicate face
*     if (p("path:      /inc/text/text_pattern.hpp")) { ... }
*
*     // extractor face
*     auto r = p.extract("path:      /inc/text/text_pattern.hpp");
*     if (r.matched)
*     {
*         std::string fname = *r.captures.find("filename");
*         // ...
*     }
*
*     // renderer face
*     pattern_capture_map<std::string, std::string> b;
*     b.set("relative_path", "/inc/text");
*     b.set("filename",      "text_pattern");
*     b.set("fileext",       "hpp");
*     std::string s = p.render(b);  // produces the path line
*
*     // rewrite face
*     std::string fixed = p.rewrite(
*         "path:      /inc/wrong/text_pattern.hpp",
*         "relative_path",
*         "/inc/text");
*     // fixed == "path:      /inc/text/text_pattern.hpp"
*
*
* TABLE OF CONTENTS
* =================
* I.    DEFAULTS
* II.   COMPILED SEGMENT
* III.  TEXT PATTERN
*       a. construction
*       b. marker configuration
*       c. escape configuration
*       d. CRTP face: do_match
*       e. CRTP face: do_extract
*       f. CRTP face: do_render
*       g. CRTP face: do_rewrite
*       h. find (partial match)
*       i. accessors
* IV.   CONCAT
* V.    CONVENIENCE FACTORIES
*
*
* path:      /inc/djinterp/core/text/text_pattern.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_TEXT_PATTERN_
#define DJINTERP_TEXT_PATTERN_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "../paradigm/pattern/pattern.hpp"

NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///                II.  COMPILED SEGMENT                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // text_pattern_segment_kind
    //   enum: kind of a compiled segment within a text_pattern.
    enum text_pattern_segment_kind : std::uint8_t
    {
        text_pattern_segment_literal = 0,
        text_pattern_segment_capture = 1
    };

    // text_pattern_segment
    //   struct: a single segment of a compiled text_pattern.
    // For literal segments, `text` is the literal to match.  For
    // capture segments, `text` is the key name.
    struct text_pattern_segment
    {
        text_pattern_segment_kind   kind;
        std::string                 text;

        text_pattern_segment()
            : kind(text_pattern_segment_literal),
              text()
        {}

        text_pattern_segment(
            text_pattern_segment_kind _kind,
            std::string               _text
        )
            : kind(_kind),
              text(std::move(_text))
        {}
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                III. TEXT PATTERN                                        ///
///////////////////////////////////////////////////////////////////////////////

// text_pattern
//   class: marker-aware text pattern engine deriving from the
// generic pattern<> CRTP base.  Compiles a template literal once
// at construction; subsequent match / extract / render / rewrite
// operations are O(n * s) where n is the input size and s is the
// segment count.
class text_pattern
    : public pattern<text_pattern, std::string>
{
public:
    using input_type        = std::string;
    using key_type          = std::string;
    using value_type        = std::string;
    using size_type         = std::size_t;
    using capture_map_type  = pattern_capture_map<key_type, value_type>;
    using match_result_type = pattern_match_result<key_type, value_type>;

    // =================================================================
    //  a. construction
    // =================================================================

    // default constructor - empty pattern
    text_pattern()
        : m_literal (),
          m_prefix  (D_TEXT_PAT_DEFAULT_PREFIX),
          m_suffix  (D_TEXT_PAT_DEFAULT_SUFFIX),
          m_escape  (D_TEXT_PAT_DEFAULT_ESCAPE),
          m_segments(),
          m_compile_status(DPatternStatusOk)
    {}

    // from literal (default markers)
    explicit text_pattern(
        std::string _literal
    )
        : m_literal (std::move(_literal)),
          m_prefix  (D_TEXT_PAT_DEFAULT_PREFIX),
          m_suffix  (D_TEXT_PAT_DEFAULT_SUFFIX),
          m_escape  (D_TEXT_PAT_DEFAULT_ESCAPE),
          m_segments(),
          m_compile_status(DPatternStatusOk)
    {
        compile();
    }

    // from literal + custom markers
    text_pattern(
        std::string _literal,
        std::string _prefix,
        std::string _suffix
    )
        : m_literal (std::move(_literal)),
          m_prefix  (std::move(_prefix)),
          m_suffix  (std::move(_suffix)),
          m_escape  (D_TEXT_PAT_DEFAULT_ESCAPE),
          m_segments(),
          m_compile_status(DPatternStatusOk)
    {
        compile();
    }


    // =================================================================
    //  b. marker configuration
    // =================================================================

    D_NODISCARD
    const std::string& prefix() const { return m_prefix; }

    D_NODISCARD
    const std::string& suffix() const { return m_suffix; }

    // set_markers
    //   method: changes the marker prefix and suffix and
    // recompiles the pattern.
    void
    set_markers
    (
        const std::string& _prefix,
        const std::string& _suffix
    )
    {
        m_prefix = _prefix;
        m_suffix = _suffix;

        compile();

        return;
    }


    // =================================================================
    //  c. escape configuration
    // =================================================================

    D_NODISCARD
    char escape() const { return m_escape; }

    // set_escape
    //   method: changes the escape character and recompiles.
    // Pass '\0' to disable escape handling entirely.
    void
    set_escape
    (
        char _ch
    )
    {
        m_escape = _ch;

        compile();

        return;
    }


    // =================================================================
    //  d. CRTP face: do_match
    // =================================================================

    // do_match
    //   method: returns true iff _in conforms to the full
    // pattern from start to end.  For partial / substring
    // matching, use find().
    D_NODISCARD
    bool
    do_match
    (
        const std::string& _in
    ) const
    {
        if (m_compile_status != DPatternStatusOk)
        {
            return false;
        }

        capture_map_type discard;
        size_type        end_pos = 0;

        if (!run_segments(_in, 0, end_pos, discard))
        {
            return false;
        }

        // require full consumption of the input
        return (end_pos == _in.size());
    }


    // =================================================================
    //  e. CRTP face: do_extract
    // =================================================================

    // do_extract
    //   method: matches _in and returns the populated capture
    // map.  On failure, returns a match_result with matched
    // false and an explanatory status code.
    D_NODISCARD
    match_result_type
    do_extract
    (
        const std::string& _in
    ) const
    {
        if (m_compile_status != DPatternStatusOk)
        {
            return match_result_type(m_compile_status);
        }

        capture_map_type captures;
        size_type        end_pos = 0;

        if (!run_segments(_in, 0, end_pos, captures))
        {
            return match_result_type(DPatternStatusNoMatch);
        }

        if (end_pos != _in.size())
        {
            return match_result_type(DPatternStatusNoMatch);
        }

        return match_result_type(std::move(captures));
    }


    // =================================================================
    //  f. CRTP face: do_render
    // =================================================================

    // do_render
    //   method: produces text by walking the compiled segments,
    // emitting literals verbatim and substituting bound values
    // at each capture point.  Unbound captures emit the empty
    // string.
    D_NODISCARD
    std::string
    do_render
    (
        const capture_map_type& _captures
    ) const
    {
        std::string out;

        out.reserve(m_literal.size());

        for (const auto& seg : m_segments)
        {
            if (seg.kind == internal::text_pattern_segment_literal)
            {
                out.append(seg.text);
            }
            else
            {
                const std::string* v = _captures.find(seg.text);

                if (v != nullptr)
                {
                    out.append(*v);
                }
            }
        }

        return out;
    }


    // =================================================================
    //  g. CRTP face: do_rewrite
    // =================================================================

    // do_rewrite
    //   method: parses _in, replaces the binding for _key with
    // _value, and re-renders.  Returns _in unchanged if the
    // input does not match the pattern.
    D_NODISCARD
    std::string
    do_rewrite
    (
        const std::string& _in,
        const std::string& _key,
        const std::string& _value
    ) const
    {
        match_result_type r = do_extract(_in);

        if (!r.matched)
        {
            return _in;
        }

        r.captures.set(_key, _value);

        return do_render(r.captures);
    }


    // =================================================================
    //  h. find (partial match)
    // =================================================================

    // find
    //   method: scans _in for the first occurrence of the
    // pattern.  Returns true on success, with _pos set to the
    // start position and _out populated.  Returns false if no
    // occurrence is found.
    bool
    find
    (
        const std::string& _in,
        size_type&         _pos,
        match_result_type& _out
    ) const
    {
        if (m_compile_status != DPatternStatusOk)
        {
            _out = match_result_type(m_compile_status);
            return false;
        }

        // determine the longest leading literal to anchor the search
        std::string anchor;

        if ( !m_segments.empty() &&
             (m_segments.front().kind ==
              internal::text_pattern_segment_literal) )
        {
            anchor = m_segments.front().text;
        }

        size_type start = 0;

        while (start <= _in.size())
        {
            size_type candidate = (anchor.empty())
                                ? start
                                : _in.find(anchor, start);

            if (candidate == std::string::npos)
            {
                _out = match_result_type(DPatternStatusNoMatch);
                return false;
            }

            capture_map_type captures;
            size_type        end_pos = 0;

            if (run_segments(_in, candidate, end_pos, captures))
            {
                _pos = candidate;
                _out = match_result_type(std::move(captures));
                return true;
            }

            // advance past this attempt
            start = candidate + 1;
        }

        _out = match_result_type(DPatternStatusNoMatch);
        return false;
    }


    // =================================================================
    //  i. accessors
    // =================================================================

    D_NODISCARD
    const std::string& literal() const { return m_literal; }

    D_NODISCARD
    pattern_status compile_status() const { return m_compile_status; }

    D_NODISCARD
    bool is_compiled_ok() const
    {
        return (m_compile_status == DPatternStatusOk);
    }

    // capture_keys
    //   method: returns the list of capture keys in the order
    // they appear in the literal.  Useful for diagnostics and
    // for verifying that a binding map covers every capture.
    D_NODISCARD
    std::vector<std::string>
    capture_keys() const
    {
        std::vector<std::string> keys;

        for (const auto& seg : m_segments)
        {
            if (seg.kind == internal::text_pattern_segment_capture)
            {
                keys.push_back(seg.text);
            }
        }

        return keys;
    }

    // segment_count
    //   method: returns the number of compiled segments.
    D_NODISCARD
    size_type segment_count() const { return m_segments.size(); }


private:

    // =================================================================
    //  internal: compilation
    // =================================================================

    // compile
    //   method: scans the literal and produces the segment
    // list.  Adjacent captures (with no literal between) cause
    // the pattern to be marked malformed.
    void
    compile()
    {
        m_segments.clear();
        m_compile_status = DPatternStatusOk;

        if (m_prefix.empty() || m_suffix.empty())
        {
            m_compile_status = DPatternStatusMalformed;
            return;
        }

        const size_type pfx_len = m_prefix.size();
        const size_type sfx_len = m_suffix.size();
        std::string     buffer;
        size_type       i = 0;

        // tracks whether the last segment emitted was a capture;
        // used to detect adjacent-capture malformation.
        bool last_was_capture = false;

        while (i < m_literal.size())
        {
            // escape: emit the prefix as a literal char and skip
            if ( (m_escape != '\0')                       &&
                 (m_literal[i] ==
                    static_cast<char>(m_escape))          &&
                 ((i + 1 + pfx_len) <= m_literal.size())  &&
                 (m_literal.compare(i + 1, pfx_len,
                                    m_prefix) == 0) )
            {
                buffer.append(m_prefix);
                i += 1 + pfx_len;

                continue;
            }

            // prefix: open a capture
            if ( ((i + pfx_len) <= m_literal.size()) &&
                 (m_literal.compare(i, pfx_len,
                                    m_prefix) == 0) )
            {
                size_type key_start = i + pfx_len;
                size_type sfx_pos   = m_literal.find(
                    m_suffix, key_start);

                if (sfx_pos != std::string::npos)
                {
                    // flush pending literal buffer
                    if (!buffer.empty())
                    {
                        m_segments.emplace_back(
                            internal::text_pattern_segment_literal,
                            std::move(buffer));
                        buffer.clear();
                        last_was_capture = false;
                    }

                    // adjacent-capture check
                    if (last_was_capture)
                    {
                        m_compile_status = DPatternStatusMalformed;
                        return;
                    }

                    std::string key = m_literal.substr(
                        key_start, sfx_pos - key_start);

                    m_segments.emplace_back(
                        internal::text_pattern_segment_capture,
                        std::move(key));

                    last_was_capture = true;
                    i = sfx_pos + sfx_len;

                    continue;
                }
            }

            // ordinary character
            buffer.push_back(m_literal[i]);
            ++i;
        }

        // flush trailing literal buffer
        if (!buffer.empty())
        {
            m_segments.emplace_back(
                internal::text_pattern_segment_literal,
                std::move(buffer));
        }

        return;
    }


    // =================================================================
    //  internal: matching engine
    // =================================================================

    // run_segments
    //   method: attempts to match the compiled segments against
    // _in starting at _start.  On success, _end_pos is set to
    // the first input position past the matched region and
    // captures are populated.  Captures are lazy - each capture
    // extends to the next occurrence of the following literal.
    bool
    run_segments
    (
        const std::string& _in,
        size_type          _start,
        size_type&         _end_pos,
        capture_map_type&  _captures
    ) const
    {
        size_type pos = _start;

        for (size_type i = 0; i < m_segments.size(); ++i)
        {
            const auto& seg = m_segments[i];

            if (seg.kind == internal::text_pattern_segment_literal)
            {
                if ( (pos + seg.text.size()) > _in.size() )
                {
                    return false;
                }

                if (_in.compare(pos, seg.text.size(),
                                seg.text) != 0)
                {
                    return false;
                }

                pos += seg.text.size();
            }
            else
            {
                // capture: extends to the next literal segment,
                // or to end-of-input for a trailing capture.
                if ( (i + 1) < m_segments.size() )
                {
                    const auto& next = m_segments[i + 1];

                    // adjacency was rejected at compile time; this
                    // is a defensive check.
                    if (next.kind !=
                        internal::text_pattern_segment_literal)
                    {
                        return false;
                    }

                    size_type lit_pos = _in.find(next.text, pos);

                    if (lit_pos == std::string::npos)
                    {
                        return false;
                    }

                    _captures.set(seg.text,
                                  _in.substr(pos,
                                             lit_pos - pos));
                    pos = lit_pos;
                }
                else
                {
                    // trailing capture - takes the remainder
                    _captures.set(seg.text,
                                  _in.substr(pos));
                    pos = _in.size();
                }
            }
        }

        _end_pos = pos;

        return true;
    }


    // =================================================================
    //  storage
    // =================================================================

    std::string                                 m_literal;
    std::string                                 m_prefix;
    std::string                                 m_suffix;
    char                                        m_escape;
    std::vector<internal::text_pattern_segment> m_segments;
    pattern_status                              m_compile_status;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  CONCAT                                              ///
///////////////////////////////////////////////////////////////////////////////

// concat
//   function: produces a new text_pattern whose literal is the
// concatenation of _left's and _right's literals.  Both
// patterns must share the same marker configuration; if they
// disagree, _left's markers are used.  This is the text-specific
// composition primitive - sub-patterns combine into full-file
// patterns the same way functions compose into pipelines.
D_NODISCARD
inline text_pattern
concat
(
    const text_pattern& _left,
    const text_pattern& _right
)
{
    return text_pattern(_left.literal() + _right.literal(),
                        _left.prefix(),
                        _left.suffix());
}


///////////////////////////////////////////////////////////////////////////////
///                V.   CONVENIENCE FACTORIES                               ///
///////////////////////////////////////////////////////////////////////////////

// make_pattern
//   function: builds a text_pattern with default markers ("%").
D_NODISCARD
inline text_pattern
make_pattern
(
    const std::string& _literal
)
{
    return text_pattern(_literal);
}

// make_curly_pattern
//   function: builds a text_pattern using "{" / "}" as markers.
D_NODISCARD
inline text_pattern
make_curly_pattern
(
    const std::string& _literal
)
{
    return text_pattern(_literal, "{", "}");
}

// make_dollar_pattern
//   function: builds a text_pattern using "${" / "}" as markers.
D_NODISCARD
inline text_pattern
make_dollar_pattern
(
    const std::string& _literal
)
{
    return text_pattern(_literal, "${", "}");
}

// make_mustache_pattern
//   function: builds a text_pattern using "{{" / "}}" as markers,
// matching Mustache / Handlebars convention and pairing with
// make_mustache_template() in text_template.hpp.
D_NODISCARD
inline text_pattern
make_mustache_pattern
(
    const std::string& _literal
)
{
    return text_pattern(_literal, "{{", "}}");
}


NS_END  // djinterp


#endif  // DJINTERP_TEXT_PATTERN_