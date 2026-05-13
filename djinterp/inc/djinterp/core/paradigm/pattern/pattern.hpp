/******************************************************************************
* djinterp [paradigm]                                              pattern.hpp
*
*   Type-agnostic pattern primitive.  A pattern is a four-faced view
* of a structural specification:
*
*     1. predicate face  - operator()(input)             -> bool
*     2. extractor face  - extract(input)                -> match_result
*     3. renderer  face  - render(captures)              -> input
*     4. rewrite   face  - rewrite(input, key, value)    -> input
*
*   The four faces are derived from the same compiled spec.  Because
* the predicate face has the shape of a callable returning bool, a
* conforming pattern composes directly with the predicate combinators
* in functional/predicate.hpp (predicate_and, predicate_or,
* predicate_not) and with any callable-consuming primitive in
* functional/compose.hpp or functional/fn_builder.hpp.
*
*   This header defines the CRTP base from which all concrete patterns
* derive, along with the capture map, the match result, and the three
* generic combinators (pattern_and, pattern_or, pattern_not).
*   A conforming derived pattern must provide:
*     - `input_type`      typedef
*     - `key_type`        typedef
*     - `value_type`      typedef
*     - `bool          do_match(const input_type&) const;`
*     - `match_result  do_extract(const input_type&) const;`
*     - `input_type    do_render(const capture_map_type&) const;`
*     - `input_type    do_rewrite(const input_type&,
*                                 const key_type&,
*                                 const value_type&) const;`
*   The base enforces its contract through SFINAE traits and
* deferred static_asserts - no virtual dispatch, no tag types.
*
*
* path:      /inc/djinterp/core/paradigm/pattern/pattern.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    STATUS CODES
II.   CAPTURE MAP
III.  MATCH RESULT
IV.   PATTERN (CRTP BASE)
V.    PATTERN TRAITS
VI.   COMBINATORS
      a. pattern_and
      b. pattern_or
      c. pattern_not
VII.  C++20 CONCEPTS
*/

#ifndef DJINTERP_PATTERN_
#define DJINTERP_PATTERN_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   STATUS CODES                                        ///
///////////////////////////////////////////////////////////////////////////////

// pattern_status
//   typedef: classifies the outcome of a pattern operation.
typedef std::int32_t pattern_status;

// DPatternStatus*
//   constants: standard pattern status codes.  Derived patterns
// may define additional codes above DPatternStatusUserBase.
constexpr pattern_status DPatternStatusOk           =  0;
constexpr pattern_status DPatternStatusNoMatch      =  1;
constexpr pattern_status DPatternStatusAmbiguous    =  2;
constexpr pattern_status DPatternStatusKeyNotFound  =  3;
constexpr pattern_status DPatternStatusMalformed    =  4;
constexpr pattern_status DPatternStatusUserBase     = 64;


///////////////////////////////////////////////////////////////////////////////
///                II.  CAPTURE MAP                                         ///
///////////////////////////////////////////////////////////////////////////////

// pattern_capture
//   struct: a single key -> value binding produced by extraction.
template<typename _Key,
         typename _Value>
struct pattern_capture
{
    _Key    key;
    _Value  value;

    pattern_capture()
        : key  (),
          value()
    {}

    pattern_capture(
        const _Key&   _key,
        const _Value& _value
    )
        : key  (_key),
          value(_value)
    {}

    pattern_capture(
        _Key&&   _key,
        _Value&& _value
    )
        : key  (std::move(_key)),
          value(std::move(_value))
    {}
};


// pattern_capture_map
//   class: ordered list of pattern_capture entries with by-key
// lookup.  Preserves insertion order (matching text_template's
// binding semantics) and supports replace-on-set.
template<typename _Key,
         typename _Value>
class pattern_capture_map
{
public:
    using capture_type   = pattern_capture<_Key, _Value>;
    using storage_type   = std::vector<capture_type>;
    using iterator       = typename storage_type::iterator;
    using const_iterator = typename storage_type::const_iterator;
    using size_type      = std::size_t;
    using key_type       = _Key;
    using value_type     = _Value;

    pattern_capture_map()
        : m_entries()
    {}

    // -----------------------------------------------------------------
    //  capacity
    // -----------------------------------------------------------------

    D_NODISCARD
    bool         empty() const { return m_entries.empty(); }

    D_NODISCARD
    size_type    size()  const { return m_entries.size();  }

    void         clear()       { m_entries.clear(); return; }

    // -----------------------------------------------------------------
    //  lookup
    // -----------------------------------------------------------------

    // find
    //   method: returns a pointer to the value bound to _key, or
    // nullptr if the key is absent.
    D_NODISCARD
    const _Value*
    find
    (
        const _Key& _key
    ) const
    {
        for (const auto& e : m_entries)
        {
            if (e.key == _key)
            {
                return &e.value;
            }
        }

        return nullptr;
    }

    // find (mutable)
    //   method: mutable overload of find().
    D_NODISCARD
    _Value*
    find
    (
        const _Key& _key
    )
    {
        for (auto& e : m_entries)
        {
            if (e.key == _key)
            {
                return &e.value;
            }
        }

        return nullptr;
    }

    // has
    //   method: returns true if _key is bound.
    D_NODISCARD
    bool
    has
    (
        const _Key& _key
    ) const
    {
        return (find(_key) != nullptr);
    }

    // -----------------------------------------------------------------
    //  mutation
    // -----------------------------------------------------------------

    // set
    //   method: binds _key to _value, replacing any existing
    // binding.  Returns a reference to *this for chaining.
    pattern_capture_map&
    set
    (
        const _Key&   _key,
        const _Value& _value
    )
    {
        _Value* existing = find(_key);

        if (existing)
        {
            *existing = _value;
        }
        else
        {
            m_entries.emplace_back(_key, _value);
        }

        return *this;
    }

    // set (move overload)
    pattern_capture_map&
    set
    (
        _Key&&   _key,
        _Value&& _value
    )
    {
        _Value* existing = find(_key);

        if (existing)
        {
            *existing = std::move(_value);
        }
        else
        {
            m_entries.emplace_back(std::move(_key),
                                   std::move(_value));
        }

        return *this;
    }

    // erase
    //   method: removes the binding for _key, returning true if
    // a binding was removed.
    bool
    erase
    (
        const _Key& _key
    )
    {
        for (auto it = m_entries.begin();
             it != m_entries.end();
             ++it)
        {
            if (it->key == _key)
            {
                m_entries.erase(it);

                return true;
            }
        }

        return false;
    }

    // merge
    //   method: copies bindings from _other into this map.  Keys
    // already present in this map are overwritten when
    // _overwrite is true; otherwise they are preserved.
    void
    merge
    (
        const pattern_capture_map& _other,
        bool                       _overwrite = true
    )
    {
        for (const auto& e : _other.m_entries)
        {
            if (_overwrite || !has(e.key))
            {
                set(e.key, e.value);
            }
        }

        return;
    }

    // -----------------------------------------------------------------
    //  iteration
    // -----------------------------------------------------------------

    iterator       begin()       { return m_entries.begin(); }
    iterator       end()         { return m_entries.end();   }
    const_iterator begin() const { return m_entries.begin(); }
    const_iterator end()   const { return m_entries.end();   }

    // -----------------------------------------------------------------
    //  storage access
    // -----------------------------------------------------------------

    D_NODISCARD
    const storage_type& entries() const { return m_entries; }

private:
    storage_type m_entries;
};


///////////////////////////////////////////////////////////////////////////////
///                III. MATCH RESULT                                        ///
///////////////////////////////////////////////////////////////////////////////

// pattern_match_result
//   struct: result of applying a pattern's extractor face.
// Carries the matched flag, a status code, and the populated
// capture map.  Default-constructed = no match.
template<typename _Key,
         typename _Value>
struct pattern_match_result
{
    using capture_map_type = pattern_capture_map<_Key, _Value>;

    bool                matched;
    pattern_status      status;
    capture_map_type    captures;

    pattern_match_result()
        : matched (false),
          status  (DPatternStatusNoMatch),
          captures()
    {}

    // success constructor
    explicit pattern_match_result(
        capture_map_type _captures
    )
        : matched (true),
          status  (DPatternStatusOk),
          captures(std::move(_captures))
    {}

    // failure constructor
    explicit pattern_match_result(
        pattern_status _status
    )
        : matched (false),
          status  (_status),
          captures()
    {}

    D_NODISCARD
    explicit operator bool() const { return matched; }
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  PATTERN (CRTP BASE)                                 ///
///////////////////////////////////////////////////////////////////////////////

// forward declaration of traits for static_assert in pattern<>
NS_TRAITS

    template<typename _Type, typename = void>
    struct pattern_has_input_type;

    template<typename _Type, typename = void>
    struct pattern_has_key_type;

    template<typename _Type, typename = void>
    struct pattern_has_value_type;

    template<typename _Type, typename = void>
    struct pattern_has_do_match;

    template<typename _Type, typename = void>
    struct pattern_has_do_extract;

    template<typename _Type, typename = void>
    struct pattern_has_do_render;

    template<typename _Type, typename = void>
    struct pattern_has_do_rewrite;

NS_END  // traits


// pattern
//   class: generic CRTP base for all concrete pattern types.
// Provides the four public faces (operator(), match, extract,
// render, rewrite) by forwarding to the derived implementation,
// and exposes a uniform composition surface.
//
//   Conformance of the derived type is checked via deferred
// static_asserts in the public methods, mirroring the
// approach used in parse/scanner.hpp.
template<typename _Derived>
class pattern
{
public:
    using derived_type = _Derived;

    // -----------------------------------------------------------------
    //  predicate face (operator())
    // -----------------------------------------------------------------

    // operator()
    //   method: predicate face.  Returns true iff the input
    // conforms to the pattern.  Allows a conforming pattern to
    // act as a predicate in any predicate-consuming combinator.
    D_NODISCARD
    bool
    operator()
    (
        const typename derived_type::input_type& _in
    ) const
    {
        check_conformance();

        return self().do_match(_in);
    }

    // -----------------------------------------------------------------
    //  match face
    // -----------------------------------------------------------------

    // match
    //   method: explicit alias for operator() - preferred when
    // operator() would be ambiguous with another face.
    D_NODISCARD
    bool
    match
    (
        const typename derived_type::input_type& _in
    ) const
    {
        check_conformance();

        return self().do_match(_in);
    }

    // -----------------------------------------------------------------
    //  extractor face
    // -----------------------------------------------------------------

    // extract
    //   method: returns a match result populated with the
    // captures bound by the input.  An unmatched input yields
    // a result with matched == false.
    D_NODISCARD
    pattern_match_result<
        typename derived_type::key_type,
        typename derived_type::value_type>
    extract
    (
        const typename derived_type::input_type& _in
    ) const
    {
        check_conformance();

        return self().do_extract(_in);
    }

    // -----------------------------------------------------------------
    //  renderer face
    // -----------------------------------------------------------------

    // render
    //   method: produces an input value from a capture map,
    // substituting bound values at each capture point.
    D_NODISCARD
    typename derived_type::input_type
    render
    (
        const pattern_capture_map<
            typename derived_type::key_type,
            typename derived_type::value_type>& _captures
    ) const
    {
        check_conformance();

        return self().do_render(_captures);
    }

    // -----------------------------------------------------------------
    //  rewrite face
    // -----------------------------------------------------------------

    // rewrite
    //   method: parses _in, replaces the capture for _key with
    // _value, and re-renders.  Captures other than _key are
    // preserved.  If _in does not match the pattern, the input
    // is returned unchanged.
    D_NODISCARD
    typename derived_type::input_type
    rewrite
    (
        const typename derived_type::input_type& _in,
        const typename derived_type::key_type&   _key,
        const typename derived_type::value_type& _value
    ) const
    {
        check_conformance();

        return self().do_rewrite(_in, _key, _value);
    }

protected:
    pattern()  = default;
    ~pattern() = default;

private:
    // self
    //   method: CRTP cast helper.
    D_NODISCARD
    derived_type&       self()       { return *static_cast<derived_type*>(this); }

    D_NODISCARD
    const derived_type& self() const { return *static_cast<const derived_type*>(this); }

    // check_conformance
    //   method: deferred static_assert checks that fire only on
    // first use of the public faces.  Mirrors the approach used
    // by parse/scanner<>.
    static void
    check_conformance()
    {
        static_assert(
            traits::pattern_has_input_type<derived_type>::value,
            "Pattern must define a public `input_type` typedef.");

        static_assert(
            traits::pattern_has_key_type<derived_type>::value,
            "Pattern must define a public `key_type` typedef.");

        static_assert(
            traits::pattern_has_value_type<derived_type>::value,
            "Pattern must define a public `value_type` typedef.");

        static_assert(
            traits::pattern_has_do_match<derived_type>::value,
            "Pattern must define a public `do_match` member "
            "function returning a bool-convertible value.");

        static_assert(
            traits::pattern_has_do_extract<derived_type>::value,
            "Pattern must define a public `do_extract` member "
            "function returning a pattern_match_result.");

        static_assert(
            traits::pattern_has_do_render<derived_type>::value,
            "Pattern must define a public `do_render` member "
            "function returning an input_type.");

        static_assert(
            traits::pattern_has_do_rewrite<derived_type>::value,
            "Pattern must define a public `do_rewrite` member "
            "function returning an input_type.");
    }
};


///////////////////////////////////////////////////////////////////////////////
///                V.   PATTERN TRAITS                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_TRAITS

    // pattern_has_input_type
    //   trait: detects a public `input_type` typedef.
    template<typename _Type, typename>
    struct pattern_has_input_type : std::false_type
    {};

    template<typename _Type>
    struct pattern_has_input_type<_Type,
        std::void_t<typename _Type::input_type>> : std::true_type
    {};

    // pattern_has_key_type
    //   trait: detects a public `key_type` typedef.
    template<typename _Type, typename>
    struct pattern_has_key_type : std::false_type
    {};

    template<typename _Type>
    struct pattern_has_key_type<_Type,
        std::void_t<typename _Type::key_type>> : std::true_type
    {};

    // pattern_has_value_type
    //   trait: detects a public `value_type` typedef.
    template<typename _Type, typename>
    struct pattern_has_value_type : std::false_type
    {};

    template<typename _Type>
    struct pattern_has_value_type<_Type,
        std::void_t<typename _Type::value_type>> : std::true_type
    {};

    // pattern_has_do_match
    //   trait: detects a callable `do_match(input_type) -> bool`.
    template<typename _Type, typename>
    struct pattern_has_do_match : std::false_type
    {};

    template<typename _Type>
    struct pattern_has_do_match<_Type, std::void_t<
        decltype(
            static_cast<bool>(
                std::declval<const _Type&>().do_match(
                    std::declval<
                        const typename _Type::input_type&>()))
        )>> : std::true_type
    {};

    // pattern_has_do_extract
    //   trait: detects a callable `do_extract(input_type)`.
    template<typename _Type, typename>
    struct pattern_has_do_extract : std::false_type
    {};

    template<typename _Type>
    struct pattern_has_do_extract<_Type, std::void_t<
        decltype(
            std::declval<const _Type&>().do_extract(
                std::declval<
                    const typename _Type::input_type&>())
        )>> : std::true_type
    {};

    // pattern_has_do_render
    //   trait: detects a callable
    // `do_render(capture_map_type) -> input_type`.
    template<typename _Type, typename>
    struct pattern_has_do_render : std::false_type
    {};

    template<typename _Type>
    struct pattern_has_do_render<_Type, std::void_t<
        decltype(
            std::declval<const _Type&>().do_render(
                std::declval<
                    const pattern_capture_map<
                        typename _Type::key_type,
                        typename _Type::value_type>&>())
        )>> : std::true_type
    {};

    // pattern_has_do_rewrite
    //   trait: detects a callable
    // `do_rewrite(input_type, key_type, value_type) -> input_type`.
    template<typename _Type, typename>
    struct pattern_has_do_rewrite : std::false_type
    {};

    template<typename _Type>
    struct pattern_has_do_rewrite<_Type, std::void_t<
        decltype(
            std::declval<const _Type&>().do_rewrite(
                std::declval<const typename _Type::input_type&>(),
                std::declval<const typename _Type::key_type&>(),
                std::declval<const typename _Type::value_type&>())
        )>> : std::true_type
    {};

    // is_pattern
    //   trait: composite trait - true iff _Type satisfies the
    // full pattern protocol.
    template<typename _Type>
    struct is_pattern
    {
        static constexpr bool value =
            ( pattern_has_input_type <_Type>::value &&
              pattern_has_key_type   <_Type>::value &&
              pattern_has_value_type <_Type>::value &&
              pattern_has_do_match   <_Type>::value &&
              pattern_has_do_extract <_Type>::value &&
              pattern_has_do_render  <_Type>::value &&
              pattern_has_do_rewrite <_Type>::value );
    };

    // is_pattern_v
    //   value: convenience alias for is_pattern<_Type>::value.
    template<typename _Type>
    constexpr bool is_pattern_v = is_pattern<_Type>::value;

NS_END  // traits


///////////////////////////////////////////////////////////////////////////////
///                VI.  COMBINATORS                                         ///
///////////////////////////////////////////////////////////////////////////////

// =================================================================
//  a. pattern_and
// =================================================================

// pattern_and_combinator
//   class: combinator that matches iff both child patterns match.
// Extraction merges both capture maps (right-hand keys win on
// collision).  Render delegates to the right-hand pattern.
// Rewrite delegates to the right-hand pattern.
template<typename _PatternA,
         typename _PatternB>
class pattern_and_combinator
    : public pattern<pattern_and_combinator<_PatternA, _PatternB>>
{
public:
    using input_type       = typename _PatternA::input_type;
    using key_type         = typename _PatternA::key_type;
    using value_type       = typename _PatternA::value_type;
    using capture_map_type = pattern_capture_map<key_type, value_type>;
    using match_result_type =
        pattern_match_result<key_type, value_type>;

    static_assert(
        std::is_same<typename _PatternA::input_type,
                     typename _PatternB::input_type>::value,
        "pattern_and requires matching input_type on both sides.");

    static_assert(
        std::is_same<typename _PatternA::key_type,
                     typename _PatternB::key_type>::value,
        "pattern_and requires matching key_type on both sides.");

    static_assert(
        std::is_same<typename _PatternA::value_type,
                     typename _PatternB::value_type>::value,
        "pattern_and requires matching value_type on both sides.");

    template<typename _AFwd,
             typename _BFwd>
    pattern_and_combinator(
        _AFwd&& _a,
        _BFwd&& _b
    )
        : m_a(std::forward<_AFwd>(_a)),
          m_b(std::forward<_BFwd>(_b))
    {}

    // CRTP-required interface
    D_NODISCARD
    bool
    do_match
    (
        const input_type& _in
    ) const
    {
        return ( m_a.do_match(_in) && m_b.do_match(_in) );
    }

    D_NODISCARD
    match_result_type
    do_extract
    (
        const input_type& _in
    ) const
    {
        auto a = m_a.do_extract(_in);

        if (!a.matched)
        {
            return match_result_type(DPatternStatusNoMatch);
        }

        auto b = m_b.do_extract(_in);

        if (!b.matched)
        {
            return match_result_type(DPatternStatusNoMatch);
        }

        // merge: right-hand keys overwrite left on collision
        a.captures.merge(b.captures, true);

        return match_result_type(std::move(a.captures));
    }

    D_NODISCARD
    input_type
    do_render
    (
        const capture_map_type& _c
    ) const
    {
        return m_b.do_render(_c);
    }

    D_NODISCARD
    input_type
    do_rewrite
    (
        const input_type& _in,
        const key_type&   _k,
        const value_type& _v
    ) const
    {
        return m_b.do_rewrite(_in, _k, _v);
    }

    // introspection
    D_NODISCARD const _PatternA& first()  const { return m_a; }
    D_NODISCARD const _PatternB& second() const { return m_b; }

private:
    _PatternA m_a;
    _PatternB m_b;
};


// pattern_and
//   function: constructs a pattern_and_combinator from two
// patterns of matching type.
template<typename _PatternA,
         typename _PatternB>
D_NODISCARD
pattern_and_combinator<typename std::decay<_PatternA>::type,
                       typename std::decay<_PatternB>::type>
pattern_and
(
    _PatternA&& _a,
    _PatternB&& _b
)
{
    return pattern_and_combinator<
        typename std::decay<_PatternA>::type,
        typename std::decay<_PatternB>::type>(
            std::forward<_PatternA>(_a),
            std::forward<_PatternB>(_b));
}


// =================================================================
//  b. pattern_or
// =================================================================

// pattern_or_combinator
//   class: combinator that matches iff either child pattern
// matches.  Extraction returns the left-hand result if it
// matches, otherwise the right-hand result.  Render and rewrite
// delegate to the left-hand pattern.
template<typename _PatternA,
         typename _PatternB>
class pattern_or_combinator
    : public pattern<pattern_or_combinator<_PatternA, _PatternB>>
{
public:
    using input_type       = typename _PatternA::input_type;
    using key_type         = typename _PatternA::key_type;
    using value_type       = typename _PatternA::value_type;
    using capture_map_type = pattern_capture_map<key_type, value_type>;
    using match_result_type =
        pattern_match_result<key_type, value_type>;

    static_assert(
        std::is_same<typename _PatternA::input_type,
                     typename _PatternB::input_type>::value,
        "pattern_or requires matching input_type on both sides.");

    static_assert(
        std::is_same<typename _PatternA::key_type,
                     typename _PatternB::key_type>::value,
        "pattern_or requires matching key_type on both sides.");

    static_assert(
        std::is_same<typename _PatternA::value_type,
                     typename _PatternB::value_type>::value,
        "pattern_or requires matching value_type on both sides.");

    template<typename _AFwd,
             typename _BFwd>
    pattern_or_combinator(
        _AFwd&& _a,
        _BFwd&& _b
    )
        : m_a(std::forward<_AFwd>(_a)),
          m_b(std::forward<_BFwd>(_b))
    {}

    D_NODISCARD
    bool
    do_match
    (
        const input_type& _in
    ) const
    {
        return ( m_a.do_match(_in) || m_b.do_match(_in) );
    }

    D_NODISCARD
    match_result_type
    do_extract
    (
        const input_type& _in
    ) const
    {
        auto a = m_a.do_extract(_in);

        if (a.matched)
        {
            return a;
        }

        return m_b.do_extract(_in);
    }

    D_NODISCARD
    input_type
    do_render
    (
        const capture_map_type& _c
    ) const
    {
        return m_a.do_render(_c);
    }

    D_NODISCARD
    input_type
    do_rewrite
    (
        const input_type& _in,
        const key_type&   _k,
        const value_type& _v
    ) const
    {
        // delegate to whichever side actually matches
        if (m_a.do_match(_in))
        {
            return m_a.do_rewrite(_in, _k, _v);
        }

        return m_b.do_rewrite(_in, _k, _v);
    }

    D_NODISCARD const _PatternA& first()  const { return m_a; }
    D_NODISCARD const _PatternB& second() const { return m_b; }

private:
    _PatternA m_a;
    _PatternB m_b;
};


// pattern_or
//   function: constructs a pattern_or_combinator.
template<typename _PatternA,
         typename _PatternB>
D_NODISCARD
pattern_or_combinator<typename std::decay<_PatternA>::type,
                      typename std::decay<_PatternB>::type>
pattern_or
(
    _PatternA&& _a,
    _PatternB&& _b
)
{
    return pattern_or_combinator<
        typename std::decay<_PatternA>::type,
        typename std::decay<_PatternB>::type>(
            std::forward<_PatternA>(_a),
            std::forward<_PatternB>(_b));
}


// =================================================================
//  c. pattern_not
// =================================================================

// pattern_not_combinator
//   class: combinator that matches iff the wrapped pattern does
// NOT match.  Extraction yields an empty capture map on success
// (i.e. when the input does not match the wrapped pattern).
// Render returns a default-constructed input_type.  Rewrite
// returns the input unchanged.
template<typename _Pattern>
class pattern_not_combinator
    : public pattern<pattern_not_combinator<_Pattern>>
{
public:
    using input_type       = typename _Pattern::input_type;
    using key_type         = typename _Pattern::key_type;
    using value_type       = typename _Pattern::value_type;
    using capture_map_type = pattern_capture_map<key_type, value_type>;
    using match_result_type =
        pattern_match_result<key_type, value_type>;

    template<typename _PFwd>
    explicit pattern_not_combinator(
        _PFwd&& _p
    )
        : m_p(std::forward<_PFwd>(_p))
    {}

    D_NODISCARD
    bool
    do_match
    (
        const input_type& _in
    ) const
    {
        return ( !m_p.do_match(_in) );
    }

    D_NODISCARD
    match_result_type
    do_extract
    (
        const input_type& _in
    ) const
    {
        if (m_p.do_match(_in))
        {
            return match_result_type(DPatternStatusNoMatch);
        }

        return match_result_type(capture_map_type());
    }

    D_NODISCARD
    input_type
    do_render
    (
        const capture_map_type& /*_c*/
    ) const
    {
        return input_type();
    }

    D_NODISCARD
    input_type
    do_rewrite
    (
        const input_type& _in,
        const key_type&   /*_k*/,
        const value_type& /*_v*/
    ) const
    {
        return _in;
    }

    D_NODISCARD const _Pattern& inner() const { return m_p; }

private:
    _Pattern m_p;
};


// pattern_not
//   function: constructs a pattern_not_combinator.
template<typename _Pattern>
D_NODISCARD
pattern_not_combinator<typename std::decay<_Pattern>::type>
pattern_not
(
    _Pattern&& _p
)
{
    return pattern_not_combinator<
        typename std::decay<_Pattern>::type>(
            std::forward<_Pattern>(_p));
}


///////////////////////////////////////////////////////////////////////////////
///                VII. C++20 CONCEPTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#if defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                               \
    (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1)

// pattern_type
//   concept: constrains types that satisfy the full pattern
// protocol - input/key/value typedefs plus the four do_*
// member functions.
template<typename _Type>
concept pattern_type = traits::is_pattern<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_PATTERN_
