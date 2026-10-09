/*******************************************************************************
* djinterp [parsegen]                                 parsegen_grammar_tests.cpp
*
* The neutral-grammar suite's sections: the tree a frontend builds, repetition
* as bounds, the two kinds of choice, capability accumulation, and canonical
* rendering.
*   Every section builds into `scratch`, which is a grammar that owns its
* storage where the build has an allocator and one that carries its own where
* it has none. The type is selected once, at namespace scope, and every
* assertion below holds either way -- which is itself a thing the suite checks,
* by passing under both.
*
*
* path:      /tests/djinterp/parsegen/parsegen_grammar_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./parsegen_grammar_tests.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <cstddef>  // std::size_t
#include <cstring>  // std::strcmp, std::strstr
// djinterp
#include "../../../inc/djinterp/parse/charset.hpp"      // charset
#include "../../../inc/djinterp/parse/diagnostic.hpp"   // fixed_diagnostics
#include "../../../inc/djinterp/parsegen/grammar.hpp"   // the grammar's face
#include "../../../inc/djinterp/parsegen/registry.hpp"  // fixed_registry
// re_std
#include "../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int32_t,
                                                    // uint16_t, uint32_t


namespace
{

using djinterp::parse::charset;
using djinterp::parse::fixed_diagnostics;
using namespace djinterp::parsegen;

#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)
// scratch
//   type: the grammar these sections build into. With an allocator it grows
// on demand; without one it carries its own storage.
using scratch = grammar;

// ready
//   function: gives a scratch grammar its storage.
bool
ready(
    scratch& _grammar
)
{
    return _grammar.reserve();
}
#else
using scratch = fixed_grammar<256u, 16u, 2048u, 64u>;

bool
ready(
    scratch& _grammar
)
{
    (void)_grammar;

    return true;
}
#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP

// kind_of
//   function: a node's kind, or the kind no node has where there is no node.
re_std::uint16_t
kind_of(
    const node* _node
)
{
    return _node ? _node->kind : static_cast<re_std::uint16_t>(0xFFFFu);
}

// is_kind
//   function: whether a node exists and is of a given kind.
bool
is_kind(
    const node* _node,
    node_kind   _kind
)
{
    return ( (_node != nullptr)                                    &&
             (_node->kind == static_cast<re_std::uint16_t>(_kind)) );
}

// any_at
//   function: builds an any-symbol node under a source cursor whose scope is
// this call, so the cursor is restored by the time the call returns.
re_std::int32_t
any_at(
    scratch&         _grammar,
    re_std::uint32_t _offset,
    re_std::uint32_t _length
)
{
    const auto scope = _grammar.at(_offset, _length);

    return _grammar.any();
}

// define_sum
//   function: defines `Number = [0-9]+ ;` and `Sum = Number ( '+' Number )* ;`
// in a grammar, spelling the digit class as its caller asks.
bool
define_sum(
    scratch&    _grammar,
    const char* _digits
)
{
    const re_std::int32_t number =
        _grammar.plus(_grammar.klass(charset::of(_digits)));

    if (!_grammar.define("Number", number))
    {
        return false;
    }

    const re_std::int32_t tail = _grammar.star(
        _grammar.sequence({ _grammar.literal("+"), _grammar.ref("Number") }));

    return _grammar.define("Sum",
                           _grammar.sequence({ _grammar.ref("Number"), tail }));
}

/*
d_tests_parsegen_grammar_nesting
  The tree the old two-level model could not express.
  Tests the following:
  - a choice nested inside a sequence, which `vector<vector<term>>` could not
    represent at all
  - a repetition over that group, so grouping and repetition compose
  - the three levels are there to be walked: sequence, repeat, ordered choice
  - the result resolves and verifies with nothing reported
*/
bool
d_tests_parsegen_grammar_nesting(void)
{
    fixed_diagnostics<16u, 1024u> diag;
    scratch                       g;

    if (!ready(g))
    {
        return false;
    }

    g.from("test");

    // a (b / c)* d -- the shape that motivated the model
    const re_std::int32_t a     = g.literal("a");
    const re_std::int32_t inner = g.ordered({ g.literal("b"), g.literal("c") });
    const re_std::int32_t many  = g.star(inner);
    const re_std::int32_t body  = g.sequence({ a, many, g.literal("d") });

    if ( (body == no_node)       ||
         (!g.define("S", body))  )
    {
        return false;
    }

    const node* const root   = g.node_at(body);
    const node* const first  = root ? g.node_at(root->child) : nullptr;
    const node* const second = first ? g.node_at(first->next) : nullptr;
    const node* const nested = second ? g.node_at(second->child) : nullptr;

    return ( (is_kind(root, node_kind::sequence))             &&
             (is_kind(second, node_kind::repeat))             &&
             (is_kind(nested, node_kind::ordered_choice))     &&
             (g.resolve(&diag))                               &&
             (g.verify(&diag))                                &&
             (!diag.failed())                                 );
}

/*
d_tests_parsegen_grammar_unwrap
  A list of one.
  Tests the following:
  - a sequence of one member is that member, not a wrapper around it
  - the same holds for each kind of choice
*/
bool
d_tests_parsegen_grammar_unwrap(void)
{
    scratch g;

    if (!ready(g))
    {
        return false;
    }

    const re_std::int32_t a = g.literal("a");

    return ( (g.sequence({ a }) == a)    &&
             (g.ordered({ a }) == a)     &&
             (g.unordered({ a }) == a)   );
}

/*
d_tests_parsegen_grammar_attribution
  Where in the source a node came from.
  Tests the following:
  - the cursor applies to what is built under it
  - it is restored when its scope ends, so what is built after is not
    attributed to it
*/
bool
d_tests_parsegen_grammar_attribution(void)
{
    scratch g;

    if (!ready(g))
    {
        return false;
    }

    const node* const under = g.node_at(any_at(g, 40u, 7u));
    const node* const after = g.node_at(g.any());

    return ( (under != nullptr)       &&
             (under->offset == 40u)   &&
             (under->length == 7u)    &&
             (after != nullptr)       &&
             (after->offset != 40u)   );
}

/*
d_tests_parsegen_grammar_bounds
  Repetition as bounds rather than a star flag.
  Tests the following:
  - the three classic forms are bounds, and do not set BOUNDED_REPEAT
  - an explicit range does set it, so a family can ask whether it was handed
    a count -- ABNF's 3*5DIGIT, which the old model could not express
  - a reversed range is refused rather than normalised
*/
bool
d_tests_parsegen_grammar_bounds(void)
{
    scratch g;

    if (!ready(g))
    {
        return false;
    }

    const re_std::int32_t digit = g.klass(charset::of("0-9"));

    (void)g.optional(digit);
    (void)g.star(digit);
    (void)g.plus(digit);

    // `?`, `*` and `+` are bounds, and none of them is a counted repetition
    if (g.uses().any(feature::bounded_repeat))
    {
        return false;
    }

    return ( (g.repeat(digit, 3u, 5u) != no_node)         &&
             (g.uses().any(feature::bounded_repeat))      &&
             (g.repeat(digit, 5u, 3u) == no_node)         );
}

/*
d_tests_parsegen_grammar_bounds_render
  Each form of repetition, back as the notation it means.
  Tests the following:
  - a classic form renders as its suffix
  - a counted range renders as its bounds
  - an open range renders with no upper bound
*/
bool
d_tests_parsegen_grammar_bounds_render(void)
{
    scratch g;
    char    text[128];

    if (!ready(g))
    {
        return false;
    }

    const re_std::int32_t digit = g.klass(charset::of("0-9"));

    (void)g.render_expr(g.plus(digit), text, sizeof(text));

    if (std::strcmp(text, "[0-9]+") != 0)
    {
        return false;
    }

    (void)g.render_expr(g.repeat(digit, 3u, 5u), text, sizeof(text));

    if (std::strcmp(text, "[0-9]{3,5}") != 0)
    {
        return false;
    }

    (void)g.render_expr(g.repeat(digit, 2u, unbounded), text, sizeof(text));

    return (std::strcmp(text, "[0-9]{2,}") == 0);
}

/*
d_tests_parsegen_grammar_choice
  The distinction the whole model exists to protect.
  Tests the following:
  - ordered and unordered choice are different node kinds, not one with a flag
  - each sets its own capability and neither sets the other's
  - a grammar mixing both reports both, which is what a family must see
*/
bool
d_tests_parsegen_grammar_choice(void)
{
    scratch peg;
    scratch bnf;

    if ( (!ready(peg))  ||
         (!ready(bnf))  )
    {
        return false;
    }

    const re_std::int32_t pa = peg.literal("a");
    const re_std::int32_t pb = peg.literal("b");
    const re_std::int32_t po = peg.ordered({ pa, pb });
    const re_std::int32_t bu = bnf.unordered({ bnf.literal("a"),
                                               bnf.literal("b") });

    // different kinds, each declaring itself and not the other
    if ( (kind_of(peg.node_at(po)) == kind_of(bnf.node_at(bu)))  ||
         (!peg.uses().any(feature::ordered_choice))              ||
         (peg.uses().any(feature::unordered_choice))             ||
         (!bnf.uses().any(feature::unordered_choice))            ||
         (bnf.uses().any(feature::ordered_choice))               )
    {
        return false;
    }

    // a grammar holding both reports both
    return ( (peg.unordered({ pa, pb }) != no_node)           &&
             (peg.uses().any(feature::ordered_choice))        &&
             (peg.uses().any(feature::unordered_choice))      );
}

/*
d_tests_parsegen_grammar_choice_render
  The two kinds of choice, in the canonical text.
  Tests the following:
  - an ordered choice renders with `/`
  - an unordered choice renders with `|`, so the distinction survives into
    text and back
*/
bool
d_tests_parsegen_grammar_choice_render(void)
{
    scratch g;
    char    text[64];

    if (!ready(g))
    {
        return false;
    }

    const re_std::int32_t a = g.literal("a");
    const re_std::int32_t b = g.literal("b");

    (void)g.render_expr(g.ordered({ a, b }), text, sizeof(text));

    if (std::strcmp(text, "'a' / 'b'") != 0)
    {
        return false;
    }

    (void)g.render_expr(g.unordered({ a, b }), text, sizeof(text));

    return (std::strcmp(text, "'a' | 'b'") == 0);
}

/*
d_tests_parsegen_grammar_features
  Capability accumulation as a property of building.
  Tests the following:
  - a fresh grammar uses nothing
  - each construct declares itself without the frontend saying so
  - a single-symbol literal is not a string literal, since a family may handle
    one and not the other
*/
bool
d_tests_parsegen_grammar_features(void)
{
    scratch g;

    if ( (!ready(g))          ||
         (!g.uses().empty())  )
    {
        return false;
    }

    const re_std::int32_t digit = g.klass(charset::of("0-9"));

    (void)g.literal("x");

    // a class declared itself; one symbol is not a string
    if ( (!g.uses().any(feature::character_class))  ||
         (g.uses().any(feature::literal_string))    )
    {
        return false;
    }

    (void)g.literal("while");

    return ( (g.uses().any(feature::literal_string))                &&
             (g.capture(g.plus(digit), 1, "value") != no_node)      &&
             (g.forbid(g.literal("_")) != no_node)                  &&
             (g.uses().any(feature::capture))                       &&
             (g.uses().any(feature::syntactic_predicate))           );
}

/*
d_tests_parsegen_grammar_derived
  What building does not declare.
  Tests the following:
  - a rule that references itself leftmost is left-recursive, but that is a
    derived property: building it declares nothing, and neither does
    resolution -- analysis is what fills it in
  - resolution does bind the reference to the rule it names
*/
bool
d_tests_parsegen_grammar_derived(void)
{
    fixed_diagnostics<16u, 1024u> diag;
    scratch                       g;

    if (!ready(g))
    {
        return false;
    }

    const re_std::int32_t self = g.ref("R");

    if (!g.define("R", g.sequence({ self, g.literal("+") })))
    {
        return false;
    }

    return ( (g.resolve(&diag))                             &&
             (!g.uses().any(feature::left_recursion))       &&
             (g.node_at(self)->b == g.index_of("R"))        );
}

/*
d_tests_parsegen_grammar_selection
  The accumulated set is what a family is matched against.
  Tests the following:
  - a grammar a family does not reject selects it
  - add a host action, and the same family stops accepting the same grammar,
    with the capability named in the note
*/
bool
d_tests_parsegen_grammar_selection(void)
{
    fixed_diagnostics<16u, 1024u> diag;
    fixed_registry<4u>            reg;
    scratch                       g;
    stage                         peg = {};

    peg.name    = "peg";
    peg.kind    = static_cast<re_std::uint32_t>(stage_kind::family);
    peg.rejects = feature::unordered_choice | feature::host_action;

    if ( (!ready(g))                                          ||
         (!reg.add(peg))                                      ||
         (g.klass(charset::of("0-9")) == no_node)             ||
         (!reg.select(stage_kind::family, g.uses(), &diag))   )
    {
        return false;
    }

    (void)g.action(g.literal("z"), "emit();");

    diag.clear();

    return ( (!reg.select(stage_kind::family, g.uses(), &diag))         &&
             (diag[1].is(D_PARSEGEN_DIAG_DOMAIN_REGISTRY,
                         D_PARSEGEN_DIAG_REGISTRY_REJECTED))            &&
             (std::strstr(diag[1].text(), "HOST_ACTION") != nullptr)    );
}

/*
d_tests_parsegen_grammar_render
  A whole grammar as canonical text.
  Tests the following:
  - a grammar renders as its rules, one per line
  - parenthesisation follows binding strength, and only where it is needed:
    the group inside the repetition takes parentheses, the top-level sequence
    does not
*/
bool
d_tests_parsegen_grammar_render(void)
{
    scratch g;
    char    text[512];

    if ( (!ready(g))                                ||
         (!define_sum(g, "0-9"))                    ||
         (g.render(text, sizeof(text)) >= sizeof(text)) )
    {
        return false;
    }

    return ( (std::strstr(text, "Number = [0-9]+ ;"))                  &&
             (std::strstr(text, "Sum = Number ( '+' Number )* ;"))     );
}

/*
d_tests_parsegen_grammar_digest
  Two grammars that mean the same thing.
  Tests the following:
  - they render the same and digest the same, however their classes were
    spelled
  - the digest ignores source positions, so a reformatted grammar is the same
    cache key
*/
bool
d_tests_parsegen_grammar_digest(void)
{
    scratch first;
    scratch second;
    char    text[512];
    char    other[512];

    if ( (!ready(first))             ||
         (!ready(second))            ||
         (!define_sum(first, "0-9")) )
    {
        return false;
    }

    // the same grammar, from another place in another file, spelled out
    const auto scope = second.at(999u, 3u);

    if (!define_sum(second, "0123456789"))
    {
        return false;
    }

    (void)first.render(text, sizeof(text));
    (void)second.render(other, sizeof(other));

    return ( (std::strcmp(text, other) == 0)        &&
             (first.digest() == second.digest())    );
}

/*
d_tests_parsegen_grammar_unresolved
  A reference to a rule nobody defined.
  Tests the following:
  - resolution fails, and the grammar is left unresolved
  - the failure is identified by the grammar's domain and its own code
  - the missing rule is named, not merely counted
*/
bool
d_tests_parsegen_grammar_unresolved(void)
{
    fixed_diagnostics<16u, 1024u> diag;
    scratch                       broken;

    if ( (!ready(broken))                                ||
         (!broken.define("S", broken.ref("Missing")))    )
    {
        return false;
    }

    return ( (!broken.resolve(&diag))                                   &&
             (!broken.resolved())                                       &&
             (diag[0].is(D_PARSEGEN_DIAG_DOMAIN_GRAMMAR,
                         D_PARSEGEN_DIAG_GRAMMAR_UNRESOLVED_REF))       &&
             (std::strstr(diag[0].text(), "Missing") != nullptr)        );
}

}  // namespace

// d_tests_parsegen_grammar
//   constant: the suite -- every section this build has, the tree's shape
// before what is derived from it.
const d_tests_section d_tests_parsegen_grammar[] =
{
    { "parsegen_grammar_nesting",
      &d_tests_parsegen_grammar_nesting },
    { "parsegen_grammar_unwrap",
      &d_tests_parsegen_grammar_unwrap },
    { "parsegen_grammar_attribution",
      &d_tests_parsegen_grammar_attribution },
    { "parsegen_grammar_bounds",
      &d_tests_parsegen_grammar_bounds },
    { "parsegen_grammar_bounds_render",
      &d_tests_parsegen_grammar_bounds_render },
    { "parsegen_grammar_choice",
      &d_tests_parsegen_grammar_choice },
    { "parsegen_grammar_choice_render",
      &d_tests_parsegen_grammar_choice_render },
    { "parsegen_grammar_features",
      &d_tests_parsegen_grammar_features },
    { "parsegen_grammar_derived",
      &d_tests_parsegen_grammar_derived },
    { "parsegen_grammar_selection",
      &d_tests_parsegen_grammar_selection },
    { "parsegen_grammar_render",
      &d_tests_parsegen_grammar_render },
    { "parsegen_grammar_digest",
      &d_tests_parsegen_grammar_digest },
    { "parsegen_grammar_unresolved",
      &d_tests_parsegen_grammar_unresolved }
};

// d_tests_parsegen_grammar_count
//   constant: how many sections the table above holds in this build.
const std::size_t d_tests_parsegen_grammar_count =
    sizeof(d_tests_parsegen_grammar) / sizeof(d_tests_parsegen_grammar[0]);

#endif  // floor, for now
