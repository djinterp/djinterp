/*******************************************************************************
* djinterp [parsegen]                                               analysis.hpp
*
*   The C++ face of grammar analysis declared in analysis.h.
*   `facts` derives from d_parsegen_facts, adds no data member, and is asserted
* layout-identical, so a C stage reads C++-computed facts by taking their
* address.  `fixed_facts<Nodes, Rules>` carries its own storage and its own
* scratch, for a build with no allocator.
*
*   The named queries are the point.  `disjoint()` answers the one question two
* separate transforms depend on -- whether a choice can be told apart by a
* single symbol -- and it answers it INCLUDING the nullability check that a
* caller working from first sets alone would forget.
*
*
* path:      /inc/djinterp/parsegen/analysis.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_ANALYSIS_HPP
#define DJINTERP_PARSEGEN_ANALYSIS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>           // std::is_standard_layout
// djinterp
#include "../parse/charset.hpp"  // parse::charset
#include "./c/analysis.h"        // the C analysis this layer faces
#include "./grammar.hpp"         // parsegen::grammar, NS_PARSEGEN
// re_std
#include "../../re_std/cstdint/cstdint.hpp"  // re_std::int32_t, uint32_t,
                                             // uint8_t


NS_DJINTERP
NS_PARSEGEN


// node_facts
//   type: what was derived about one expression, unchanged.  An alias,
// because the facts hold an array of them.
using node_facts = ::d_parsegen_node_facts;

// rule_facts
//   type: what was derived about one rule, unchanged, for the same reason.
using rule_facts = ::d_parsegen_rule_facts;


// facts
//   class: the derived properties of one grammar, with lifetime.  Move-only,
// as every owning container here is, and indexed exactly as the grammar is so
// a node index means the same thing on both sides.
class facts : public d_parsegen_facts
{
public:
    // facts
    //   constructor: empty facts owning nothing.
    facts() noexcept
    {
        d_parsegen_facts_init(this, nullptr, 0u, nullptr, 0u,
                              nullptr, nullptr, nullptr);
    }

    facts(const facts&)            = delete;
    facts& operator=(const facts&) = delete;

    // facts
    //   constructor: takes over another's storage and ownership.
    facts(
        facts&& _other
    ) noexcept
        : d_parsegen_facts(_other)
    {
        d_parsegen_facts_init(&_other, nullptr, 0u, nullptr, 0u,
                              nullptr, nullptr, nullptr);
    }

    // operator=
    //   function: releases these facts, then takes over another's.
    facts&
    operator=(
        facts&& _other
    ) noexcept
    {
        // guard against self-move, which would release the storage being taken
        if (this != &_other)
        {
            d_parsegen_facts_release(this);

            static_cast<d_parsegen_facts&>(*this) = _other;

            d_parsegen_facts_init(&_other, nullptr, 0u, nullptr, 0u,
                                  nullptr, nullptr, nullptr);
        }

        return *this;
    }

    // ~facts
    //   destructor: releases any storage these facts own.
    ~facts() noexcept
    {
        d_parsegen_facts_release(this);
    }

#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)
    // size_for
    //   function: replaces this collection's storage with storage it allocates
    // and owns, sized for one grammar.
    D_NODISCARD bool
    size_for(
        const d_parsegen_grammar& _grammar
    ) noexcept
    {
        d_parsegen_facts_release(this);

        return (d_parsegen_facts_init_heap(this, &_grammar) == 0);
    }
#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP

    // analyze
    //   function: derives what the grammar means and folds it into what the
    // grammar reports.  The grammar is not const because of that second half:
    // an unanalysed grammar under-reports its capabilities, and a family
    // selected against an under-report is the wrong family.
    D_NODISCARD bool
    analyze(
        d_parsegen_grammar& _grammar,
        d_parse_diag_sink*  _diag = nullptr
    ) noexcept
    {
        return (d_parsegen_analyze(&_grammar, this, _diag) == 0);
    }

    // complete
    //   accessor: whether analysis settled and found nothing fatal.
    D_CONSTEXPR_CPP14 bool
    complete() const noexcept
    {
        return ((flags & D_PARSEGEN_FACTS_COMPLETE) != 0u);
    }

    // derived
    //   accessor: the capabilities analysis added that the syntax alone did
    // not show -- LEFT_RECURSION being the one that matters.
    feature_set
    derived() const noexcept
    {
        return feature_set(implied);
    }

    // nullable
    //   accessor: whether an expression can match the empty input.
    bool
    nullable(
        re_std::int32_t _node
    ) const noexcept
    {
        return (d_parsegen_nullable(this, _node) != 0);
    }

    // first
    //   accessor: the symbols an expression can begin with -- a sound
    // over-approximation, so it can rule an alternative out but never in.
    const d_parse_charset*
    first(
        re_std::int32_t _node
    ) const noexcept
    {
        return d_parsegen_first(this, _node);
    }

    // of_node
    //   accessor: everything derived about one expression, or null.
    const node_facts*
    of_node(
        re_std::int32_t _node
    ) const noexcept
    {
        return d_parsegen_node_facts_at(this, _node);
    }

    // of_rule
    //   accessor: everything derived about one rule, or null.
    const rule_facts*
    of_rule(
        re_std::int32_t _rule
    ) const noexcept
    {
        return d_parsegen_rule_facts_at(this, _rule);
    }

    // left_recursive
    //   accessor: whether a rule can reach itself without consuming anything.
    // A fact, not a verdict: which families mind is a registry question.
    bool
    left_recursive(
        re_std::int32_t _rule
    ) const noexcept
    {
        return (d_parsegen_left_recursive(this, _rule) != 0);
    }

    // reachable
    //   accessor: whether some chain of references leads to a rule from the
    // start symbol.
    bool
    reachable(
        re_std::int32_t _rule
    ) const noexcept
    {
        const rule_facts* const found = of_rule(_rule);

        return ( (found != nullptr) &&
                 ((found->flags & D_PARSEGEN_FACT_REACHABLE) != 0u) );
    }

    // productive
    //   accessor: whether a rule can match anything at all.
    bool
    productive(
        re_std::int32_t _rule
    ) const noexcept
    {
        const rule_facts* const found = of_rule(_rule);

        return ( (found != nullptr) &&
                 ((found->flags & D_PARSEGEN_FACT_PRODUCTIVE) != 0u) );
    }

    // disjoint
    //   accessor: whether a choice's alternatives can be told apart by one
    // symbol -- the precondition both for lowering an unordered choice to an
    // ordered one and for compiling either as a jump table.  Includes the
    // nullability check, which is the half that gets forgotten.
    bool
    disjoint(
        const d_parsegen_grammar& _grammar,
        re_std::int32_t              _choice,
        re_std::int32_t*             _conflict = nullptr
    ) const noexcept
    {
        return (d_parsegen_choice_disjoint(&_grammar,
                                           this,
                                           _choice,
                                           _conflict) != 0);
    }

    // head_set
    //   accessor: the symbols any alternative of a choice can begin with --
    // what a head-fail test compares against.
    D_NODISCARD bool
    head_set(
        const d_parsegen_grammar& _grammar,
        re_std::int32_t              _choice,
        d_parse_charset&          _out
    ) const noexcept
    {
        return (d_parsegen_choice_head_set(&_grammar,
                                           this,
                                           _choice,
                                           &_out) == 0);
    }
};


// fixed_facts
//   class: a fact collection carrying its own storage and its own scratch, for
// a build with no allocator.  The scratch sizes are the ones analysis needs:
// two words and a byte per rule, and one index per node.
template<re_std::uint32_t Nodes,
         re_std::uint32_t Rules>
class fixed_facts : public facts
{
public:
    // fixed_facts
    //   constructor: binds the embedded storage as this collection's.
    fixed_facts() noexcept
    {
        d_parsegen_facts_init(this,
                              m_nodes,
                              Nodes,
                              m_rules,
                              Rules,
                              m_work,
                              m_mark,
                              m_stack);
    }

private:
    node_facts    m_nodes[Nodes];
    rule_facts    m_rules[Rules];
    re_std::uint32_t m_work[Rules * 2u];
    re_std::uint8_t  m_mark[Rules];
    re_std::int32_t  m_stack[Nodes];
};


//   The claim this type costs nothing over the C one, checked.
static_assert(sizeof(facts) == sizeof(d_parsegen_facts),
              "parsegen::facts must add no data member");
static_assert(alignof(facts) == alignof(d_parsegen_facts),
              "parsegen::facts must add no data member");
static_assert(std::is_standard_layout<facts>::value,
              "parsegen::facts must remain standard-layout");
static_assert(std::is_trivial<node_facts>::value,
              "per-node facts must remain trivial");


NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSEGEN_ANALYSIS_HPP
