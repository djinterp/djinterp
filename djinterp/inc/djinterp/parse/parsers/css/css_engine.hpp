/******************************************************************************
* djinterp [css]                                                css_engine.hpp
*
* CSS engine -- top-level facade:
*   This header bundles parsing, rule indexing, and cascade resolution
* into a single class that hosts can drive without knowing the internal
* layout of the AST.
*
*   The engine compiles a parsed `css_stylesheet` into an indexed form:
* every (rule, selector) pair is filed into a bucket keyed on the most
* discriminating component of its rightmost compound.  At resolution
* time the engine evaluates only the buckets relevant to the target --
* its id bucket, its class buckets, its type bucket, and the universal
* bucket -- avoiding a linear scan of every rule per target.
*
*   The cascade is resolved by sorting matched declarations by:
*     1. !important flag (important wins),
*     2. specificity      (higher wins),
*     3. source order     (later wins),
* which corresponds to the standard CSS cascade for a single origin.
* Hosts that need the full multi-origin cascade (user-agent + user +
* author) can call `matched_rules` directly and run their own resolver.
*
* Contents:
*   - matched_declaration           one applied declaration with metadata
*   - resolved_property_map         property name -> winning value list
*   - css_engine                    the compiled-and-indexed stylesheet
*   - parse_stylesheet              free function: text -> css_stylesheet
*
*
* path:      /inc/cpp/css/css_engine.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_CSS_ENGINE_
#define DJINTERP_CSS_ENGINE_ 1

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "../core/djinterp.hpp"
#include "../parse/parse.hpp"
#include "../parse/text_parser.hpp"
#include "./css_ast.hpp"
#include "./css_match.hpp"
#include "./css_parser.hpp"


NS_DJINTERP
NS_CSS


// ================================================================
//  matched_declaration
// ================================================================

// matched_declaration
//   struct: an entry in a cascade-resolution candidate set --
// one declaration that was found to apply to a target, paired
// with the specificity of the matching selector and the
// declaration's global source-order rank.
//
//   `declaration` is borrowed from the engine's owned stylesheet
// and is valid for the lifetime of the engine.
struct matched_declaration
{
    const css_declaration*  declaration;
    std::uint32_t           specificity;
    std::uint32_t           source_order;

    matched_declaration()
        : declaration  (nullptr)
        , specificity  (0u)
        , source_order (0u)
    {}
};


// ================================================================
//  resolved_property_map
// ================================================================

// resolved_property_map
//   type: a property-name -> winning-value-list mapping.  The
// std::map is chosen for stable iteration order; hosts can
// substitute their own container by using the `matched_rules`
// API directly.
typedef std::map<std::string, css_value_list>      resolved_property_map;


// ================================================================
//  css_engine
// ================================================================

// css_engine
//   class: a compiled stylesheet ready for fast per-target style
// resolution.  Construction is cheap; the work happens in
// `compile`.  After compilation, `resolve` and `matched_rules`
// answer queries against host-supplied targets in time
// proportional to the size of the relevant rule buckets, not
// the full stylesheet.
class css_engine
{
private:
    // rule_record
    //   struct: indexes one (rule, selector) pair into the
    // engine's bucket maps.  Both indices reference the engine's
    // owned `m_sheet` instance.
    struct rule_record
    {
        std::size_t     rule_index;
        std::size_t     selector_index;
        std::uint32_t   source_order;

        rule_record()
            : rule_index     (0u)
            , selector_index (0u)
            , source_order   (0u)
        {}
    };

    typedef std::vector<rule_record>                       record_vector;
    typedef std::map<std::string, record_vector>           bucket_map;

public:
    css_engine();
    ~css_engine();

    // compile
    //   member: replaces the engine's compiled state with an
    // indexed view of _sheet.  _sheet is copied; the engine owns
    // the copy and all pointers handed out by `matched_rules`
    // and `resolve` reference the copy.
    void compile(const css_stylesheet& _sheet);

    // matched_rules
    //   member: appends one matched_declaration to _out for each
    // (rule, selector, declaration) triple that applies to the
    // target.  No cascade resolution is performed -- the caller
    // sees raw matches.
    void
    matched_rules(const style_target_view&            _target,
                  const style_match_options&          _opt,
                  std::vector<matched_declaration>&   _out) const;

    // resolve
    //   member: runs the standard CSS cascade over the matched
    // rules and writes the winning property/value bindings into
    // _out.  _out is cleared first.
    void
    resolve(const style_target_view&   _target,
            const style_match_options& _opt,
            resolved_property_map&     _out) const;

    // sheet
    //   member: returns the engine's owned stylesheet (e.g. for
    // walking @media at-rules, dumping for diagnostics, etc.).
    const css_stylesheet& sheet() const;

    // rule_count
    //   member: total number of style rules indexed.
    std::size_t rule_count() const;

private:
    // m_classify_and_index
    //   member: walks _cx's rightmost compound and adds a
    // rule_record to the most appropriate bucket.
    void m_classify_and_index(const css_complex_selector& _cx,
                              std::size_t                 _rule_index,
                              std::size_t                 _sel_index,
                              std::uint32_t               _order);

    // m_collect_from_bucket
    //   member: for one bucket, evaluates each rule_record
    // against the target and appends matches to _out.
    void
    m_collect_from_bucket(const record_vector&             _bucket,
                          const style_target_view&         _target,
                          const style_match_options&       _opt,
                          std::vector<matched_declaration>& _out) const;

    // m_collect_from_records
    //   member: iterates over a single record_vector found via
    // a bucket lookup; defers to m_collect_from_bucket.
    void
    m_evaluate_records(const record_vector&             _records,
                       const style_target_view&         _target,
                       const style_match_options&       _opt,
                       std::vector<matched_declaration>& _out) const;

    css_stylesheet      m_sheet;
    bucket_map          m_id_bucket;
    bucket_map          m_class_bucket;
    bucket_map          m_type_bucket;
    record_vector       m_universal_bucket;
};


// ================================================================
//  css_engine -- inline definitions
// ================================================================

inline
css_engine::css_engine()
    : m_sheet            ()
    , m_id_bucket        ()
    , m_class_bucket     ()
    , m_type_bucket      ()
    , m_universal_bucket ()
{}

inline
css_engine::~css_engine()
{}

inline const css_stylesheet&
css_engine::sheet() const
{
    return m_sheet;
}

inline std::size_t
css_engine::rule_count() const
{
    return m_sheet.rules.size();
}

inline void
css_engine::m_classify_and_index(const css_complex_selector& _cx,
                                 std::size_t                 _rule_index,
                                 std::size_t                 _sel_index,
                                 std::uint32_t               _order)
{
    rule_record                     rec;
    const css_compound_selector*    rightmost;

    rec.rule_index     = _rule_index;
    rec.selector_index = _sel_index;
    rec.source_order   = _order;

    rightmost = (_cx.tail.empty())
                    ? &_cx.head
                    : &_cx.tail.back().compound;

    // priority: id > class > type > universal
    std::size_t i;

    for (i = 0u; i < rightmost->simples.size(); ++i)
    {
        if (rightmost->simples[i].kind == DCssSimpleKindId)
        {
            m_id_bucket[rightmost->simples[i].name].push_back(rec);

            return;
        }
    }

    for (i = 0u; i < rightmost->simples.size(); ++i)
    {
        if (rightmost->simples[i].kind == DCssSimpleKindClass)
        {
            m_class_bucket[rightmost->simples[i].name].push_back(rec);

            return;
        }
    }

    for (i = 0u; i < rightmost->simples.size(); ++i)
    {
        if (rightmost->simples[i].kind == DCssSimpleKindType)
        {
            m_type_bucket[rightmost->simples[i].name].push_back(rec);

            return;
        }
    }

    // no discriminating component (universal-only, or attribute-
    // / pseudo-class-only) -- catch-all bucket.
    m_universal_bucket.push_back(rec);

    return;
}

inline void
css_engine::compile(const css_stylesheet& _sheet)
{
    std::size_t   r;
    std::uint32_t order;

    m_sheet            = _sheet;
    m_id_bucket        .clear();
    m_class_bucket     .clear();
    m_type_bucket      .clear();
    m_universal_bucket .clear();

    order = 0u;

    for (r = 0u; r < m_sheet.rules.size(); ++r)
    {
        const css_style_rule& rule = m_sheet.rules[r];
        std::size_t           s;

        for (s = 0u; s < rule.selectors.selectors.size(); ++s)
        {
            m_classify_and_index(rule.selectors.selectors[s],
                                 r,
                                 s,
                                 order);

            order += 1u;
        }
    }

    return;
}

inline void
css_engine::m_evaluate_records(
    const record_vector&             _records,
    const style_target_view&         _target,
    const style_match_options&       _opt,
    std::vector<matched_declaration>& _out
) const
{
    std::size_t i;

    for (i = 0u; i < _records.size(); ++i)
    {
        const rule_record&              rec = _records[i];
        const css_style_rule&           rule = m_sheet.rules[rec.rule_index];
        const css_complex_selector&     sel  =
            rule.selectors.selectors[rec.selector_index];

        if (!match_complex(sel, _target, _opt))
        {
            continue;
        }

        // selector matched -- emit one matched_declaration per
        // declaration in the rule.
        std::size_t d;

        for (d = 0u; d < rule.declarations.size(); ++d)
        {
            matched_declaration md;

            md.declaration  = &rule.declarations[d];
            md.specificity  = sel.specificity;
            md.source_order = rec.source_order;

            _out.push_back(md);
        }
    }

    return;
}

inline void
css_engine::matched_rules(
    const style_target_view&            _target,
    const style_match_options&          _opt,
    std::vector<matched_declaration>&   _out
) const
{
    // id bucket
    if (_target.id != nullptr)
    {
        bucket_map::const_iterator it = m_id_bucket.find(_target.id);

        if (it != m_id_bucket.end())
        {
            m_evaluate_records(it->second, _target, _opt, _out);
        }
    }

    // class buckets
    {
        std::size_t i;

        for (i = 0u; i < _target.classes_count; ++i)
        {
            if (_target.classes[i] == nullptr)
            {
                continue;
            }

            bucket_map::const_iterator it =
                m_class_bucket.find(_target.classes[i]);

            if (it != m_class_bucket.end())
            {
                m_evaluate_records(it->second, _target, _opt, _out);
            }
        }
    }

    // type bucket
    if (_target.type_name != nullptr)
    {
        bucket_map::const_iterator it =
            m_type_bucket.find(_target.type_name);

        if (it != m_type_bucket.end())
        {
            m_evaluate_records(it->second, _target, _opt, _out);
        }
    }

    // universal bucket
    m_evaluate_records(m_universal_bucket, _target, _opt, _out);

    return;
}


// ================================================================
//  cascade resolution
// ================================================================

NS_INTERNAL

    // cascade_less
    //   trait: comparator placing lower-priority declarations
    // first.  After std::sort with this comparator, walking the
    // sorted vector and writing each declaration's value into a
    // map yields a last-write-wins resolution that matches the
    // CSS cascade.
    struct cascade_less
    {
        bool
        operator()(const matched_declaration& _a,
                   const matched_declaration& _b) const
        {
            // !important beats not-important
            bool ai = (_a.declaration != nullptr) &&
                      (_a.declaration->important);
            bool bi = (_b.declaration != nullptr) &&
                      (_b.declaration->important);

            if (ai != bi)
            {
                // not-important goes first -> lower priority
                return (!ai && bi);
            }

            // higher specificity = higher priority -> placed later
            if (_a.specificity != _b.specificity)
            {
                return (_a.specificity < _b.specificity);
            }

            // later source order = higher priority -> placed later
            return (_a.source_order < _b.source_order);
        }
    };

NS_END  // internal

inline void
css_engine::resolve(const style_target_view&   _target,
                    const style_match_options& _opt,
                    resolved_property_map&     _out) const
{
    std::vector<matched_declaration> matches;

    _out.clear();

    matched_rules(_target, _opt, matches);

    if (matches.empty())
    {
        return;
    }

    // sort lowest priority first
    std::sort(matches.begin(),
              matches.end(),
              internal::cascade_less());

    // walk in priority order; later writes overwrite earlier
    std::size_t i;

    for (i = 0u; i < matches.size(); ++i)
    {
        const matched_declaration& md = matches[i];

        if (md.declaration == nullptr)
        {
            continue;
        }

        _out[md.declaration->property] = md.declaration->value;
    }

    return;
}


// ================================================================
//  parse_stylesheet  --  free-function convenience
// ================================================================

// parse_stylesheet
//   function: convenience wrapper that constructs a parser,
// hands it the source, and returns the resulting stylesheet.
// The returned parse_result is `ok` whenever the parser ran to
// completion, even if individual rules were recovered from --
// the parser is forgiving by design, matching real CSS.
inline parse::parse_result<css_stylesheet>
parse_stylesheet(const char* _source,
                 std::size_t _length)
{
    css_stylesheet_parser   p;
    parse::text_parse_state s(_source, _length);

    return p.do_parse(s);
}

// parse_stylesheet
//   function: overload taking a std::string.
inline parse::parse_result<css_stylesheet>
parse_stylesheet(const std::string& _source)
{
    return parse_stylesheet(_source.data(), _source.size());
}


NS_END  // css
NS_END  // djinterp


#endif  // DJINTERP_CSS_ENGINE_
