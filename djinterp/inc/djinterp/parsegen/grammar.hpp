/*******************************************************************************
* djinterp [parsegen]                                               grammar.hpp
*
*   The C++ face of the neutral grammar declared in grammar.h.
*   `node` and `rule` are ALIASES rather than derived types, because a grammar
* is an array of each.  `grammar` derives, adds no data member, and is asserted
* layout-identical, so a C stage reads a C++-built grammar by taking its
* address.
*
*   The builder is where the C++ side earns its place.  Sequences and choices
* take an initializer list rather than a pointer and a count, `at()` returns a
* scope guard that restores the previous origin, and the three classic
* repetition forms have names -- so a frontend reads as the notation it is
* reading rather than as arena bookkeeping.
*
*   ONE OMISSION IS DELIBERATE: there is no `choice()`.  `ordered()` and
* `unordered()` are the only spellings, because the difference is the language
* the grammar denotes and a default would let a frontend get it wrong silently.
*
* path:      /inc/djinterp/parsegen/grammar.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

#ifndef DJINTERP_PARSEGEN_GRAMMAR_HPP_
#define DJINTERP_PARSEGEN_GRAMMAR_HPP_ 1

// std
#include <cstddef>                      // std::size_t
#include <cstdint>                      // std::int32_t, std::uint32_t
#include <initializer_list>             // std::initializer_list
#include <type_traits>                  // std::is_standard_layout
// djinterp
#include "../parse/charset.hpp"         // parse::charset
#include "../parse/diagnostic.hpp"      // parse::diagnostics
#include "./grammar.h"                  // the C grammar this layer faces
#include "./registry.hpp"               // parsegen::feature_set, NS_PARSEGEN


NS_DJINTERP
NS_PARSEGEN


// node
//   type: one expression node, unchanged.  An alias, because a grammar is an
// array of them and forming a pointer to a derived type over an array of its
// base is the one thing this layer must not do.
using node = ::d_parsegen_node;

// rule
//   type: one named production, unchanged, for the same reason.
using rule = ::d_parsegen_rule;

// node_kind
//   enum: what an expression node is.  Ordered and unordered choice are
// separate kinds, so there is no neutral spelling to reach for by accident.
enum class node_kind : std::uint16_t
{
    empty            = D_PARSEGEN_NODE_EMPTY,
    any              = D_PARSEGEN_NODE_ANY,
    klass            = D_PARSEGEN_NODE_CLASS,
    literal          = D_PARSEGEN_NODE_LITERAL,
    ref              = D_PARSEGEN_NODE_REF,
    sequence         = D_PARSEGEN_NODE_SEQUENCE,
    ordered_choice   = D_PARSEGEN_NODE_ORDERED_CHOICE,
    unordered_choice = D_PARSEGEN_NODE_UNORDERED_CHOICE,
    repeat           = D_PARSEGEN_NODE_REPEAT,
    capture          = D_PARSEGEN_NODE_CAPTURE,
    and_predicate    = D_PARSEGEN_NODE_AND_PREDICATE,
    not_predicate    = D_PARSEGEN_NODE_NOT_PREDICATE,
    action           = D_PARSEGEN_NODE_ACTION
};

// unbounded
//   constant: the repetition maximum meaning no limit.
constexpr std::uint32_t unbounded = D_PARSEGEN_UNBOUNDED;

// no_node
//   constant: the node index meaning none.
constexpr std::int32_t no_node = D_PARSEGEN_NO_NODE;


// grammar
//   class: the nodes, the rules, their pool, and the capability set the whole
// thing uses -- with lifetime.  Move-only, as every owning container here is.
class grammar : public d_parsegen_grammar
{
public:
    using const_iterator = const rule*;

    // origin_scope
    //   class: restores the grammar's attribution cursor when it goes out of
    // scope, so a frontend can attribute a subtree without having to remember
    // what the cursor was before it.
    class origin_scope
    {
    public:
        // origin_scope
        //   constructor: aims the cursor at a source range, remembering the
        // previous one.
        origin_scope(
            d_parsegen_grammar& _grammar,
            std::uint32_t       _offset,
            std::uint32_t       _length
        ) noexcept
            : m_grammar(&_grammar),
              m_offset(_grammar.origin_offset),
              m_length(_grammar.origin_length)
        {
            d_parsegen_grammar_at(m_grammar, _offset, _length);
        }

        origin_scope(const origin_scope&)            = delete;
        origin_scope& operator=(const origin_scope&) = delete;

        // ~origin_scope
        //   destructor: restores the previous cursor.
        ~origin_scope() noexcept
        {
            d_parsegen_grammar_at(m_grammar, m_offset, m_length);
        }

    private:
        d_parsegen_grammar* m_grammar;
        std::uint32_t       m_offset;
        std::uint32_t       m_length;
    };

    // grammar
    //   constructor: an empty grammar owning nothing.
    grammar() noexcept
    {
        d_parsegen_grammar_init(this);
    }

    grammar(const grammar&)            = delete;
    grammar& operator=(const grammar&) = delete;

    // grammar
    //   constructor: takes over another grammar's storage and ownership.
    grammar(
        grammar&& _other
    ) noexcept
        : d_parsegen_grammar(_other)
    {
        d_parsegen_grammar_init(&_other);
    }

    // operator=
    //   function: releases this grammar, then takes over another's.
    grammar&
    operator=(
        grammar&& _other
    ) noexcept
    {
        // guard against self-move, which would release the storage being taken
        if (this != &_other)
        {
            d_parsegen_grammar_release(this);

            static_cast<d_parsegen_grammar&>(*this) = _other;

            d_parsegen_grammar_init(&_other);
        }

        return *this;
    }

    // ~grammar
    //   destructor: releases any storage this grammar owns, pool included.
    ~grammar() noexcept
    {
        d_parsegen_grammar_release(this);
    }

#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)
    // reserve
    //   function: replaces this grammar's storage with storage it allocates
    // and owns, which then grows on demand.
    D_NODISCARD bool
    reserve(
        std::uint32_t _nodes = 0u,
        std::uint32_t _rules = 0u
    ) noexcept
    {
        d_parsegen_grammar_release(this);

        return (d_parsegen_grammar_init_heap(this, _nodes, _rules) == 0);
    }
#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP

    // at
    //   function: aims the attribution cursor at a source range for as long as
    // the returned guard lives.
    D_NODISCARD origin_scope
    at(
        std::uint32_t _offset,
        std::uint32_t _length
    ) noexcept
    {
        return origin_scope(*this, _offset, _length);
    }

    // from
    //   function: records which notation this grammar was read from.
    void
    from(
        const char* _notation
    ) noexcept
    {
        d_parsegen_grammar_notation(this, _notation);
    }

    // empty_expr
    //   function: the expression matching nothing at all.
    std::int32_t
    empty_expr() noexcept
    {
        return d_parsegen_empty(this);
    }

    // any
    //   function: the expression matching any one symbol.
    std::int32_t
    any() noexcept
    {
        return d_parsegen_any(this);
    }

    // klass
    //   function: the expression matching one symbol from a set.
    std::int32_t
    klass(
        const d_parse_charset& _set
    ) noexcept
    {
        return d_parsegen_class(this, &_set);
    }

    // literal
    //   function: the expression matching an exact sequence of symbols.
    std::int32_t
    literal(
        const char* _text
    ) noexcept
    {
        return d_parsegen_literal(this, _text);
    }

    // ref
    //   function: a reference to a rule by name, bound later by resolve().
    std::int32_t
    ref(
        const char* _name
    ) noexcept
    {
        return d_parsegen_ref(this, _name);
    }

    // sequence
    //   function: the expression matching each child in order.
    std::int32_t
    sequence(
        std::initializer_list<std::int32_t> _children
    ) noexcept
    {
        return d_parsegen_sequence(this,
                                   _children.begin(),
                                   static_cast<std::uint32_t>(
                                       _children.size()));
    }

    // ordered
    //   function: PEG's `/` -- the FIRST child that matches wins and commits.
    std::int32_t
    ordered(
        std::initializer_list<std::int32_t> _children
    ) noexcept
    {
        return d_parsegen_ordered(this,
                                  _children.begin(),
                                  static_cast<std::uint32_t>(
                                      _children.size()));
    }

    // unordered
    //   function: BNF's `|` -- ANY child may match and the order carries no
    // meaning.  A grammar using this may be ambiguous.
    std::int32_t
    unordered(
        std::initializer_list<std::int32_t> _children
    ) noexcept
    {
        return d_parsegen_unordered(this,
                                    _children.begin(),
                                    static_cast<std::uint32_t>(
                                        _children.size()));
    }

    // repeat
    //   function: the expression matching its child between two bounds.
    std::int32_t
    repeat(
        std::int32_t  _child,
        std::uint32_t _minimum,
        std::uint32_t _maximum
    ) noexcept
    {
        return d_parsegen_repeat(this, _child, _minimum, _maximum);
    }

    // optional
    //   function: `?` -- {0, 1}.
    std::int32_t
    optional(
        std::int32_t _child
    ) noexcept
    {
        return repeat(_child, 0u, 1u);
    }

    // star
    //   function: `*` -- {0, unbounded}.
    std::int32_t
    star(
        std::int32_t _child
    ) noexcept
    {
        return repeat(_child, 0u, unbounded);
    }

    // plus
    //   function: `+` -- {1, unbounded}.
    std::int32_t
    plus(
        std::int32_t _child
    ) noexcept
    {
        return repeat(_child, 1u, unbounded);
    }

    // capture
    //   function: tags a subexpression so what it matched can be recovered.
    std::int32_t
    capture(
        std::int32_t _child,
        std::int32_t _tag,
        const char*  _name = nullptr
    ) noexcept
    {
        return d_parsegen_capture(this, _child, _tag, _name);
    }

    // require
    //   function: `&` -- asserts the child matches here, consuming nothing.
    std::int32_t
    require(
        std::int32_t _child
    ) noexcept
    {
        return d_parsegen_predicate(this, _child, 0);
    }

    // forbid
    //   function: `!` -- asserts the child does NOT match here, consuming
    // nothing.
    std::int32_t
    forbid(
        std::int32_t _child
    ) noexcept
    {
        return d_parsegen_predicate(this, _child, 1);
    }

    // action
    //   function: attaches host-language code to a subexpression.
    std::int32_t
    action(
        std::int32_t _child,
        const char*  _code
    ) noexcept
    {
        return d_parsegen_action(this, _child, _code);
    }

    // define
    //   function: declares a named production.  The first one declared is the
    // start symbol unless the grammar says otherwise.
    D_NODISCARD bool
    define(
        const char*   _name,
        std::int32_t  _body,
        std::uint16_t _flags = 0u
    ) noexcept
    {
        return (d_parsegen_rule_add(this, _name, _body, _flags) == 0);
    }

    // node_at
    //   accessor: the node at an index, or null.
    const node*
    node_at(
        std::int32_t _index
    ) const noexcept
    {
        return d_parsegen_node_at(this, _index);
    }

    // rule_at
    //   accessor: the rule at an index, or null.
    const rule*
    rule_at(
        std::uint32_t _index
    ) const noexcept
    {
        return d_parsegen_rule_at(this, _index);
    }

    // name_of
    //   accessor: the name of a rule, or "".  Never null.
    const char*
    name_of(
        std::uint32_t _index
    ) const noexcept
    {
        return d_parsegen_rule_name(this, _index);
    }

    // index_of
    //   accessor: the index of a rule by name, or -1.
    std::int32_t
    index_of(
        const char* _name
    ) const noexcept
    {
        return d_parsegen_rule_index(this, _name);
    }

    // resolve
    //   function: binds every reference to the rule it names, reporting every
    // unbound one.  Does not compute LEFT_RECURSION -- that is analysis's.
    D_NODISCARD bool
    resolve(
        d_parse_diag_sink* _diag = nullptr
    ) noexcept
    {
        return (d_parsegen_grammar_resolve(this, _diag) == 0);
    }

    // verify
    //   function: checks that the arena is well formed, reporting every
    // problem rather than the first.
    D_NODISCARD bool
    verify(
        d_parse_diag_sink* _diag = nullptr
    ) noexcept
    {
        return (d_parsegen_grammar_verify(this, _diag) == 0);
    }

    // resolved
    //   accessor: whether every reference has been bound since the last edit.
    constexpr bool
    resolved() const noexcept
    {
        return ((flags & D_PARSEGEN_GRAMMAR_RESOLVED) != 0u);
    }

    // verified
    //   accessor: whether structure has been checked since the last edit.
    constexpr bool
    verified() const noexcept
    {
        return ((flags & D_PARSEGEN_GRAMMAR_VERIFIED) != 0u);
    }

    // uses
    //   accessor: the capabilities this grammar actually uses, accumulated as
    // it was built.  What a registry query matches a family against.
    feature_set
    uses() const noexcept
    {
        return feature_set(features);
    }

    // digest
    //   accessor: a 64-bit key over the structure, the rules, and the pool --
    // not over source positions, so two spellings of one grammar agree.
    std::uint64_t
    digest() const noexcept
    {
        return d_parsegen_grammar_hash(this);
    }

    // render
    //   function: writes the whole grammar in the canonical notation, `/` for
    // ordered choice and `|` for unordered.
    std::size_t
    render(
        char*       _out,
        std::size_t _size
    ) const noexcept
    {
        return d_parsegen_grammar_render(this, _out, _size);
    }

    // render_expr
    //   function: writes one expression in the canonical notation.
    std::size_t
    render_expr(
        std::int32_t _node,
        char*        _out,
        std::size_t  _size
    ) const noexcept
    {
        return d_parsegen_expr_render(this, _node, _out, _size);
    }

    // size
    //   accessor: how many rules the grammar declares.
    constexpr std::uint32_t
    size() const noexcept
    {
        return rule_count;
    }

    // nodes_used
    //   accessor: how many expression nodes the grammar holds.
    constexpr std::uint32_t
    nodes_used() const noexcept
    {
        return node_count;
    }

    // begin
    //   accessor: a pointer to the first rule.
    constexpr const_iterator
    begin() const noexcept
    {
        return rules;
    }

    // end
    //   accessor: a pointer one past the last rule.
    constexpr const_iterator
    end() const noexcept
    {
        return (rules != nullptr) ? (rules + rule_count) : nullptr;
    }
};


// fixed_grammar
//   class: a grammar carrying its own node arena, rule table, and pool, for a
// frontend that must not allocate -- a build with no allocator, or one that
// caps how large a grammar it will accept.  The same shape every other
// container here offers.
template<std::uint32_t _Nodes,
         std::uint32_t _Rules,
         std::uint32_t _PoolBytes   = 1024u,
         std::uint32_t _PoolEntries = 64u>
class fixed_grammar : public grammar
{
public:
    // fixed_grammar
    //   constructor: binds the embedded storage as this grammar's.
    fixed_grammar() noexcept
    {
        d_parsegen_grammar_init(this);

        this->nodes         = m_nodes;
        this->node_capacity = _Nodes;
        this->rules         = m_rules;
        this->rule_capacity = _Rules;

        d_parse_pool_init(&this->pool,
                          m_bytes,
                          _PoolBytes,
                          m_entries,
                          _PoolEntries);
    }

private:
    node               m_nodes[_Nodes];
    rule               m_rules[_Rules];
    char               m_bytes[_PoolBytes];
    d_parse_pool_entry m_entries[_PoolEntries];
};


//   The claim this type costs nothing over the C one, checked.
static_assert(sizeof(node) == 28u,
              "an expression node is twenty-eight bytes: kind, flags, two "
              "links, two operands, and a source range");
static_assert(std::is_trivial<node>::value,
              "an expression node must remain trivial, or a grammar stops "
              "being memcpy-able");
static_assert(std::is_standard_layout<node>::value,
              "an expression node must remain standard-layout");
static_assert(sizeof(grammar) == sizeof(d_parsegen_grammar),
              "parsegen::grammar must add no data member");
static_assert(alignof(grammar) == alignof(d_parsegen_grammar),
              "parsegen::grammar must add no data member");
static_assert(std::is_standard_layout<grammar>::value,
              "parsegen::grammar must remain standard-layout");


NS_END  // parsegen
NS_END  // djinterp


#endif  // DJINTERP_PARSEGEN_GRAMMAR_HPP_
