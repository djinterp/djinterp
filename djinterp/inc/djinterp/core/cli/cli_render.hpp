/*******************************************************************************
* djinterp [core]                                                 cli_render.hpp
*
*   The interpreter for the CLI language (dj_cli.hpp), in the two shapes the
* framework already dispatches over - exactly the split option_diff.hpp draws:
*
*     PART A - COMPILE-TIME.  `ct_render` walks the type-level AST and folds it
*       to a `dji_cli_string` at compile time.  Node-kind dispatch is template
*       specialization on the `dji_cli` enum; the variable environment is an
*       ordinary option_set keyed by `dji_cli_string` names, and a `let`
*       extends it with option_set_override_t under override_replace - i.e.
*       lexical scoping IS the note's precedence union (+).  `dji_cli_format`
*       is the entry.  (The engine keeps the `ct_` tag - "compile-time" - so it
*       reads distinctly from the runtime `dji_cli_render` below.)
*
*     PART B - RUNTIME.  A `dji_cli_node` value tree (what a `.dj` parser would
*       yield, or what `dji_cli_lower` distills from the AST) rendered to a
*       std::string against a runtime environment.  Same node-kinds; the
*       rewrite engine here is full std::regex.  `dji_cli_run` is the entry.
*
*   Both realize the foundation's factorization law F(t, env) = ev(F-hat(t),
* env): rendering a template against an environment.  The two halves are
* independent but for ONE intentional bridge, `dji_cli_lower`, which distills
* the type-level AST into the runtime IR so a single template authored once as
* a type can be rendered either way (the cross-check in the tests relies on it).
*
*   PAYLOADS ARE CARRIERS.  Every payload is a `val_t<V>` (meta/carrier.hpp):
* text/name carry a `dji_cli_string` buffer, width carries a `std::size_t`.
* The render specializations therefore pattern-match `val_t<Buf>` /
* `val_t<Width>` directly, and because a name's carried value already IS the
* uniform buffer the option_set keys on, a lookup needs no conversion.  A
* let-bound result (also a `dji_cli_string`) re-carries into a lit node as
* `val_t<...>` with no special case.
*
*   Turing-completeness, both routes present in both halves:
*     - rewrite_ : a normal Markov algorithm to a fixpoint (literal patterns at
*                  compile time via dji_cli_markov; std::regex at runtime).
*     - fix_/if_ : structural recursion.  Termination is the program's job; the
*                  runtime engine additionally takes a step budget, and the
*                  compile-time engine is bounded by the instantiation depth.
*
*   Requires C++20; self-suppresses below it.  PART B's IR and renderer are
* otherwise standard-library-only and would compile far lower on their own, but
* the header is gated as one unit because PART A and `dji_cli_lower` need C++20.
*
*
* path:      /inc/djinterp/core/cli/cli_render.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.18
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================

      PART A - COMPILE-TIME (type-level AST -> dji_cli_string)

      A.I.    ct_render engine            (per-node-kind specializations)

      A.II.   dji_cli_format              (public entry + dji_cli_format_v)

      PART B - RUNTIME (dji_cli_node IR -> std::string)

      B.I.    dji_cli_rule / dji_cli_node (the runtime IR)

      B.II.   builders                    (dji_cli_lit, dji_cli_var, ... + rules)

      B.III.  dji_cli_env                 (runtime variable environment)

      B.IV.   dji_cli_render / dji_cli_run(the runtime interpreter)

      B.V.    dji_cli_lower               (AST -> IR bridge)
*/

#ifndef DJINTERP_CLI_CLI_RENDER_HPP
#define DJINTERP_CLI_CLI_RENDER_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <array>
#include <cstddef>
#include <map>
#include <regex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "../option/option.hpp"
#include "../option/option_set.hpp"      // option_set<>, find, contains
#include "../option/option_override.hpp" // option_set_override_t, override_replace
#include "./cli_string.hpp"
#include "./cli.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS


NS_DJINTERP


// ###########################################################################
// PART A - COMPILE-TIME
// ###########################################################################

// ===========================================================================
// A.I.  ct_render engine
// ===========================================================================
//   ct_render<Node, Env>::value is the rendered dji_cli_string of a node
// under an environment.  The environment is an option_set whose entries are
// option<name-buffer, content-node> bindings; a name flows into
// option_set_find_t because a name payload carries the same dji_cli_string the
// option key uses.

NS_INTERNAL

    // ct_render
    //   trait: primary template (specialized per node-kind below).
    template<typename Node,
             typename Env>
    struct ct_render;

    // ct_value
    //   function: ct_render<Node, Env>::value as a callable, so a rendered
    // string can be used directly as a non-type template argument (e.g. to
    // re-carry a let-bound value as a literal node).
    template<typename Node,
             typename Env>
    D_NODISCARD D_CONSTEXPR dji_cli_string<>
    ct_value()
    {
        return ct_render<Node, Env>::value;
    }

    // --- lit: emit literal text ---------------------------------------------
    //   The payload is a val_t carrying the buffer; match the buffer directly.
    template<dji_cli_string<> Buf,
             typename         Env>
    struct ct_render<option<dji_cli::lit, val_t<Buf>>, Env>
    {
        static D_CONSTEXPR dji_cli_string<> value = Buf;
    };

    // --- seq / group: concatenate the rendered children ---------------------
    template<typename    Env,
             typename... Children>
    D_NODISCARD D_CONSTEXPR dji_cli_string<>
    seq_concat()
    {
        dji_cli_string<> result;
        ( (result = dji_cli_concat(result, ct_render<Children, Env>::value)),
          ... );

        return result;
    }

    template<typename... Children,
             typename     Env>
    struct ct_render<option<dji_cli::seq, Children...>, Env>
    {
        static D_CONSTEXPR dji_cli_string<> value = seq_concat<Env, Children...>();
    };

    template<typename... Children,
             typename     Env>
    struct ct_render<option<dji_cli::group, Children...>, Env>
    {
        static D_CONSTEXPR dji_cli_string<> value = seq_concat<Env, Children...>();
    };

    // --- var: look up a binding (absent => empty) ---------------------------

    // binding_content
    //   trait: the content node carried by an environment binding (its first
    // arg).
    template<typename Binding>
    struct binding_content;

    template<auto        Key,
             typename    First,
             typename... Rest>
    struct binding_content<option<Key, First, Rest...>>
    {
        using type = First;
    };

    // var_lookup
    //   trait: render the binding for Key under Env, or empty when Present
    // is false (split on the bool so the absent case never names a binding).
    template<bool             Present,
             dji_cli_string<> Key,
             typename         Env>
    struct var_lookup
    {
        static D_CONSTEXPR dji_cli_string<> value = dji_cli_string<>{};
    };

    template<dji_cli_string<> Key,
             typename         Env>
    struct var_lookup<true, Key, Env>
    {
    private:
        using binding = option_set_find_t<Env, Key>;
        using content = typename binding_content<binding>::type;

    public:
        static D_CONSTEXPR dji_cli_string<> value = ct_render<content, Env>::value;
    };

    //   The var payload is a val_t carrying the name buffer; that buffer is
    // both the lookup key and the option_set key type, so it threads straight
    // through with no conversion.
    template<dji_cli_string<> Key,
             typename         Env>
    struct ct_render<option<dji_cli::var, val_t<Key>>, Env>
    {
        static D_CONSTEXPR dji_cli_string<> value =
            var_lookup<option_set_contains_v<Env, Key>, Key, Env>::value;
    };

    // --- formatting wrappers: bold / upper / lower / indent -----------------
    template<typename Child,
             typename Env>
    struct ct_render<option<dji_cli::bold, Child>, Env>
    {
        static D_CONSTEXPR dji_cli_string<> value =
            dji_cli_concat(
                dji_cli_concat(dji_cli_string<>("**"),
                               ct_render<Child, Env>::value),
                dji_cli_string<>("**"));
    };

    template<typename Child,
             typename Env>
    struct ct_render<option<dji_cli::upper, Child>, Env>
    {
        static D_CONSTEXPR dji_cli_string<> value =
            dji_cli_to_upper(ct_render<Child, Env>::value);
    };

    template<typename Child,
             typename Env>
    struct ct_render<option<dji_cli::lower, Child>, Env>
    {
        static D_CONSTEXPR dji_cli_string<> value =
            dji_cli_to_lower(ct_render<Child, Env>::value);
    };

    //   The width payload is a val_t carrying a std::size_t; match it directly.
    template<std::size_t Width,
             typename    Child,
             typename    Env>
    struct ct_render<option<dji_cli::indent, val_t<Width>, Child>, Env>
    {
        static D_CONSTEXPR dji_cli_string<> value =
            dji_cli_indent(ct_render<Child, Env>::value, Width);
    };

    // --- let_: call-by-value bind, then body (scope via override_replace) ----
    template<dji_cli_string<> Id,
             typename         Value,
             typename         Body,
             typename         Env>
    struct ct_render<option<dji_cli::let_, val_t<Id>, Value, Body>, Env>
    {
    private:
        // bind Id to the rendered value, re-carried as a literal node; the
        // rendered buffer rides straight back into val_t, no special case
        using bound_lit = option<dji_cli::lit, val_t< ct_value<Value, Env>() >>;
        using binding   = option<Id, bound_lit>;
        using env_next  =
            option_set_override_t<Env, option_set<binding>, override_replace>;

    public:
        static D_CONSTEXPR dji_cli_string<> value = ct_render<Body, env_next>::value;
    };

    // --- if_: render only the taken branch ----------------------------------
    template<bool     Cond,
             typename Then,
             typename Else,
             typename Env>
    struct if_branch
    {
        static D_CONSTEXPR dji_cli_string<> value = ct_render<Then, Env>::value;
    };

    template<typename Then,
             typename Else,
             typename Env>
    struct if_branch<false, Then, Else, Env>
    {
        static D_CONSTEXPR dji_cli_string<> value = ct_render<Else, Env>::value;
    };

    template<typename Cond,
             typename Then,
             typename Else,
             typename Env>
    struct ct_render<option<dji_cli::if_, Cond, Then, Else>, Env>
    {
        static D_CONSTEXPR dji_cli_string<> value =
            if_branch< dji_cli_truthy(ct_value<Cond, Env>()),
                       Then, Else, Env >::value;
    };

    // --- foreach_: fold the body over a bound list (a seq of items) ----------
    template<dji_cli_string<> Elem,
             typename         Body,
             typename         Env,
             typename...      Items>
    D_NODISCARD D_CONSTEXPR dji_cli_string<>
    foreach_fold()
    {
        dji_cli_string<> result;
        ( ( result = dji_cli_concat(
                result,
                ct_render<Body,
                    option_set_override_t<
                        Env,
                        option_set<option<Elem, Items>>,
                        override_replace>>::value) ), ... );

        return result;
    }

    template<dji_cli_string<> List,
             dji_cli_string<> Elem,
             typename         Body,
             typename         Env>
    struct ct_render<option<dji_cli::foreach_, val_t<List>, val_t<Elem>, Body>,
                     Env>
    {
    private:
        using list_binding = option_set_find_t<Env, List>;

        // peel binding -> bound list node -> the list node's item pack
        template<typename SeqNode>
        struct items;

        template<typename... Its>
        struct items<option<dji_cli::seq, Its...>>
        {
            static D_CONSTEXPR dji_cli_string<> run()
            {
                return foreach_fold<Elem, Body, Env, Its...>();
            }
        };

        template<typename Binding>
        struct peel;

        template<auto Key, typename SeqNode>
        struct peel<option<Key, SeqNode>>
        {
            static D_CONSTEXPR dji_cli_string<> run()
            {
                return items<SeqNode>::run();
            }
        };

    public:
        static D_CONSTEXPR dji_cli_string<> value = peel<list_binding>::run();
    };

    // --- rewrite_: drive the rules over the rendered child to a fixpoint -----
    template<typename... Rules>
    D_NODISCARD D_CONSTEXPR std::array<dji_cli_rewrite_rule<>, sizeof...(Rules)>
    rules_array()
    {
        return std::array<dji_cli_rewrite_rule<>, sizeof...(Rules)>{{
            dji_cli_rewrite_rule<>{ Rules::pattern,
                                    Rules::replacement,
                                    Rules::terminal }...
        }};
    }

    template<typename    Child,
             typename... Rules,
             typename     Env>
    struct ct_render<option<dji_cli::rewrite_, Child, Rules...>, Env>
    {
        // generous compile-time step budget; tune per program if needed
        static D_CONSTEXPR std::size_t k_budget = 100000;

        static D_CONSTEXPR dji_cli_string<> value =
            dji_cli_markov(ct_render<Child, Env>::value,
                           rules_array<Rules...>(),
                           k_budget);
    };

    // --- fix_: bind self to the body, then render the body -------------------
    //   `var<self>` inside re-renders the body under the current environment,
    // so recursion threads state through the lexical scope; terminate via if_.
    // Unbounded recursion is bounded here by the template instantiation depth.
    template<dji_cli_string<> Self,
             typename         Body,
             typename         Env>
    struct ct_render<option<dji_cli::fix_, val_t<Self>, Body>, Env>
    {
    private:
        using binding  = option<Self, Body>;
        using env_next =
            option_set_override_t<Env, option_set<binding>, override_replace>;

    public:
        static D_CONSTEXPR dji_cli_string<> value = ct_render<Body, env_next>::value;
    };

NS_END  // internal


// ===========================================================================
// A.II. dji_cli_format
// ===========================================================================

// dji_cli_format
//   trait: render a closed CLI template Template under environment Env
// (default empty) to a dji_cli_string, entirely at compile time.
template<typename Template,
         typename Env = option_set<>>
struct dji_cli_format
{
    static D_CONSTEXPR dji_cli_string<> value =
        internal::ct_render<Template, Env>::value;
};

// dji_cli_format_v
//   value: variable-template shorthand for dji_cli_format<Template,
// Env>::value.
template<typename Template,
         typename Env = option_set<>>
D_CONSTEXPR_INLINE dji_cli_string<> dji_cli_format_v =
    dji_cli_format<Template, Env>::value;


// ###########################################################################
// PART B - RUNTIME
// ###########################################################################
//
//   A value-level mirror of PART A.  The IR is a plain tagged tree; a template
// authored at runtime (by a parser, or by hand with the builders) renders to a
// std::string against a runtime environment.  Independent of PART A except for
// dji_cli_lower (B.V), which distills the AST into this IR.

// ===========================================================================
// B.I.  dji_cli_rule / dji_cli_node
// ===========================================================================

// dji_cli_rule
//   struct: one runtime rewrite production.  `regex` selects std::regex
// matching over literal substring matching; `terminal` halts the system.
struct dji_cli_rule
{
    std::string pattern;
    std::string replacement;
    bool        terminal = false;
    bool        regex    = false;
};

// dji_cli_node
//   struct: the runtime CLI IR.  `kind` selects the meaning of the payload
// fields: `text` is a lit's body or a var/let/foreach/fix name; `text2` is a
// foreach loop-variable name; `width` is an indent column; `kids` are the
// child nodes; `rules` are a rewrite_ node's productions.
struct dji_cli_node
{
    dji_cli::value            kind;
    std::string               text;
    std::string               text2;
    std::size_t               width = 0;
    std::vector<dji_cli_node> kids;
    std::vector<dji_cli_rule> rules;
};


// ===========================================================================
// B.II. builders
// ===========================================================================
//   Free constructors named with a `dji_cli_` prefix (the bare names
// lit/var/... are the type-level sugar in dj_cli.hpp).  Each yields a
// dji_cli_node.  Note that dji_cli_to_upper / dji_cli_to_lower / dji_cli_indent
// also name the compile-time string-algebra functions in dj_cli_string.hpp;
// these node builders are a disjoint overload (they take a dji_cli_node, the
// algebra takes a dji_cli_string), so the two coexist unambiguously.

// dji_cli_lit
//   function: a literal-text node.
D_NODISCARD inline dji_cli_node
dji_cli_lit(
    std::string _text
)
{
    return dji_cli_node{ dji_cli::lit, std::move(_text), "", 0, {}, {} };
}

// dji_cli_var
//   function: a variable-read node.
D_NODISCARD inline dji_cli_node
dji_cli_var(
    std::string _name
)
{
    return dji_cli_node{ dji_cli::var, std::move(_name), "", 0, {}, {} };
}

// dji_cli_seq
//   function: an ordered concatenation node.
D_NODISCARD inline dji_cli_node
dji_cli_seq(
    std::vector<dji_cli_node> _kids
)
{
    return dji_cli_node{ dji_cli::seq, "", "", 0, std::move(_kids), {} };
}

// dji_cli_group
//   function: a grouping node (semantically a seq).
D_NODISCARD inline dji_cli_node
dji_cli_group(
    std::vector<dji_cli_node> _kids
)
{
    return dji_cli_node{ dji_cli::group, "", "", 0, std::move(_kids), {} };
}

// dji_cli_bold
//   function: a flag-shaped wrapper node (renders **child**).
D_NODISCARD inline dji_cli_node
dji_cli_bold(
    dji_cli_node _child
)
{
    return dji_cli_node{ dji_cli::bold, "", "", 0, { std::move(_child) }, {} };
}

// dji_cli_to_upper
//   function: an upper-casing wrapper node.
D_NODISCARD inline dji_cli_node
dji_cli_to_upper(
    dji_cli_node _child
)
{
    return dji_cli_node{ dji_cli::upper, "", "", 0, { std::move(_child) }, {} };
}

// dji_cli_to_lower
//   function: a lower-casing wrapper node.  (Named to mirror dji_cli_to_lower
// the algebra and to leave the bare `dji_cli_lower` for the AST -> IR bridge
// in B.V.)
D_NODISCARD inline dji_cli_node
dji_cli_to_lower(
    dji_cli_node _child
)
{
    return dji_cli_node{ dji_cli::lower, "", "", 0, { std::move(_child) }, {} };
}

// dji_cli_indent
//   function: a key-value wrapper node indenting its child by _width columns.
D_NODISCARD inline dji_cli_node
dji_cli_indent(
    std::size_t  _width,
    dji_cli_node _child
)
{
    return dji_cli_node{ dji_cli::indent, "", "", _width,
                         { std::move(_child) }, {} };
}

// dji_cli_let
//   function: a binding node - render _value, bind it to _name, render _body.
D_NODISCARD inline dji_cli_node
dji_cli_let(
    std::string  _name,
    dji_cli_node _value,
    dji_cli_node _body
)
{
    return dji_cli_node{ dji_cli::let_, std::move(_name), "", 0,
                         { std::move(_value), std::move(_body) }, {} };
}

// dji_cli_if
//   function: a choice node - render _cond, then either _then or _else.
D_NODISCARD inline dji_cli_node
dji_cli_if(
    dji_cli_node _cond,
    dji_cli_node _then,
    dji_cli_node _else
)
{
    return dji_cli_node{ dji_cli::if_, "", "", 0,
                         { std::move(_cond), std::move(_then), std::move(_else) },
                         {} };
}

// dji_cli_foreach
//   function: a fold node over the list bound to _list, binding each item to
// _elem while rendering _body.
D_NODISCARD inline dji_cli_node
dji_cli_foreach(
    std::string  _list,
    std::string  _elem,
    dji_cli_node _body
)
{
    return dji_cli_node{ dji_cli::foreach_, std::move(_list), std::move(_elem), 0,
                         { std::move(_body) }, {} };
}

// dji_cli_rewrite
//   function: a Markov/regex node - render _child, drive _rules to a fixpoint.
D_NODISCARD inline dji_cli_node
dji_cli_rewrite(
    dji_cli_node              _child,
    std::vector<dji_cli_rule> _rules
)
{
    return dji_cli_node{ dji_cli::rewrite_, "", "", 0,
                         { std::move(_child) }, std::move(_rules) };
}

// dji_cli_fix
//   function: a recursion node - bind _self to _body, render _body.
D_NODISCARD inline dji_cli_node
dji_cli_fix(
    std::string  _self,
    dji_cli_node _body
)
{
    return dji_cli_node{ dji_cli::fix_, std::move(_self), "", 0,
                         { std::move(_body) }, {} };
}

// dji_cli_literal_rule
//   function: a literal-substring rewrite production.
D_NODISCARD inline dji_cli_rule
dji_cli_literal_rule(
    std::string _pattern,
    std::string _replacement,
    bool        _terminal = false
)
{
    return dji_cli_rule{ std::move(_pattern), std::move(_replacement),
                         _terminal, /*regex*/ false };
}

// dji_cli_regex_rule
//   function: an std::regex rewrite production.
D_NODISCARD inline dji_cli_rule
dji_cli_regex_rule(
    std::string _pattern,
    std::string _replacement,
    bool        _terminal = false
)
{
    return dji_cli_rule{ std::move(_pattern), std::move(_replacement),
                         _terminal, /*regex*/ true };
}


// ===========================================================================
// B.III. dji_cli_env
// ===========================================================================

// dji_cli_env
//   struct: the runtime variable environment - names to bound subtrees.
// `extend` is the value-level precedence union (+): a copy with the new
// binding overwriting, so a nested let shadows exactly as option_set_override
// does at the type level.
struct dji_cli_env
{
    std::map<std::string, dji_cli_node> vars;

    // extend
    //   function: a copy of this environment with _key bound to _value
    // (overwriting any prior binding - "later wins").
    D_NODISCARD dji_cli_env
    extend(
        const std::string& _key,
        dji_cli_node       _value
    ) const
    {
        dji_cli_env next = *this;
        next.vars[_key]  = std::move(_value);

        return next;
    }

    // find
    //   function: a pointer to the binding for _key, or nullptr if unbound.
    D_NODISCARD const dji_cli_node*
    find(
        const std::string& _key
    ) const
    {
        auto it = vars.find(_key);

        return (it == vars.end()) ? nullptr : &it->second;
    }
};


// ===========================================================================
// B.IV. dji_cli_render / dji_cli_run
// ===========================================================================

NS_INTERNAL

    // rt_truthy
    //   function: the runtime notion of truth for if_ (mirrors dji_cli_truthy).
    D_NODISCARD inline bool
    rt_truthy(
        const std::string& _text
    )
    {
        return !( _text.empty() || (_text == "false") || (_text == "0") );
    }

    // rt_indent
    //   function: prefix every line of _in with _width spaces.
    D_NODISCARD inline std::string
    rt_indent(
        const std::string& _in,
        std::size_t        _width
    )
    {
        std::string out;
        bool        line_top = true;

        for (char c : _in)
        {
            if (line_top)
            {
                out.append(_width, ' ');
                line_top = false;
            }

            out.push_back(c);

            if (c == '\n')
            {
                line_top = true;
            }
        }

        return out;
    }

    // rt_markov
    //   function: run an ordered rule system over _text to a fixpoint (literal
    // or std::regex per rule), decrementing _budget per step so the system
    // always terminates.
    D_NODISCARD inline std::string
    rt_markov(
        std::string                      _text,
        const std::vector<dji_cli_rule>& _rules,
        std::size_t&                     _budget
    )
    {
        while (_budget-- > 0)
        {
            bool fired = false;

            // first matching rule, top to bottom
            for (const dji_cli_rule& r : _rules)
            {
                if (r.regex)
                {
                    std::regex  re(r.pattern);
                    std::smatch m;

                    if (std::regex_search(_text, m, re))
                    {
                        _text = _text.substr(0, m.position(0))
                              + r.replacement
                              + _text.substr(m.position(0) + m.length(0));
                        fired = true;

                        if (r.terminal)
                        {
                            return _text;
                        }

                        break;
                    }
                }
                else
                {
                    auto at = _text.find(r.pattern);

                    if (at != std::string::npos && !r.pattern.empty())
                    {
                        _text = _text.substr(0, at)
                              + r.replacement
                              + _text.substr(at + r.pattern.size());
                        fired = true;

                        if (r.terminal)
                        {
                            return _text;
                        }

                        break;
                    }
                }
            }

            if (!fired)
            {
                break;
            }
        }

        return _text;
    }

NS_END  // internal


// dji_cli_render
//   function: render a runtime CLI node against an environment, threading a
// step budget (consumed by rewrite steps and fix entries so that rewrite_ and
// fix_ always terminate).
//
// Parameter(s):
//   _node:   the node to render.
//   _env:    the variable environment in scope.
//   _budget: remaining evaluation steps; decremented in place.
// Return:
//   the rendered text as a std::string.
D_NODISCARD inline std::string
dji_cli_render(
    const dji_cli_node& _node,
    const dji_cli_env&  _env,
    std::size_t&        _budget
)
{
    switch (_node.kind)
    {
        case dji_cli::lit:
        {
            return _node.text;
        }

        case dji_cli::var:
        {
            const dji_cli_node* bound = _env.find(_node.text);

            return bound ? dji_cli_render(*bound, _env, _budget) : std::string();
        }

        case dji_cli::seq:
        case dji_cli::group:
        {
            std::string out;

            // concatenate the rendered children
            for (const dji_cli_node& k : _node.kids)
            {
                out += dji_cli_render(k, _env, _budget);
            }

            return out;
        }

        case dji_cli::bold:
        {
            return "**" + dji_cli_render(_node.kids[0], _env, _budget) + "**";
        }

        case dji_cli::upper:
        {
            std::string s = dji_cli_render(_node.kids[0], _env, _budget);

            for (char& c : s)
            {
                if (c >= 'a' && c <= 'z') { c = static_cast<char>(c - 32); }
            }

            return s;
        }

        case dji_cli::lower:
        {
            std::string s = dji_cli_render(_node.kids[0], _env, _budget);

            for (char& c : s)
            {
                if (c >= 'A' && c <= 'Z') { c = static_cast<char>(c + 32); }
            }

            return s;
        }

        case dji_cli::indent:
        {
            return internal::rt_indent(
                dji_cli_render(_node.kids[0], _env, _budget), _node.width);
        }

        case dji_cli::let_:
        {
            // call-by-value: render the value, then bind it for the body
            std::string value = dji_cli_render(_node.kids[0], _env, _budget);
            dji_cli_env next  = _env.extend(_node.text, dji_cli_lit(value));

            return dji_cli_render(_node.kids[1], next, _budget);
        }

        case dji_cli::if_:
        {
            std::string cond = dji_cli_render(_node.kids[0], _env, _budget);

            return internal::rt_truthy(cond)
                 ? dji_cli_render(_node.kids[1], _env, _budget)
                 : dji_cli_render(_node.kids[2], _env, _budget);
        }

        case dji_cli::foreach_:
        {
            const dji_cli_node* list = _env.find(_node.text);
            std::string         out;

            // fold the body over each item, binding the loop variable
            if (list)
            {
                for (const dji_cli_node& item : list->kids)
                {
                    dji_cli_env next = _env.extend(_node.text2, item);
                    out += dji_cli_render(_node.kids[0], next, _budget);
                }
            }

            return out;
        }

        case dji_cli::rewrite_:
        {
            std::string body = dji_cli_render(_node.kids[0], _env, _budget);

            return internal::rt_markov(std::move(body), _node.rules, _budget);
        }

        case dji_cli::fix_:
        {
            // recursion guard: a spent budget stops the descent
            if (_budget == 0)
            {
                return std::string();
            }

            --_budget;
            dji_cli_env next = _env.extend(_node.text, _node.kids[0]);

            return dji_cli_render(_node.kids[0], next, _budget);
        }
    }

    return std::string();
}

// dji_cli_run
//   function: render a runtime CLI node to completion against an environment,
// with a default step budget.
//
// Parameter(s):
//   _node:   the node to render.
//   _env:    the environment (default empty).
//   _budget: the maximum number of evaluation steps (default 100000).
// Return:
//   the rendered text as a std::string.
D_NODISCARD inline std::string
dji_cli_run(
    const dji_cli_node& _node,
    dji_cli_env         _env    = dji_cli_env{},
    std::size_t         _budget = 100000
)
{
    return dji_cli_render(_node, _env, _budget);
}


// ===========================================================================
// B.V.  dji_cli_lower
// ===========================================================================
//   The one bridge between the halves: distill the type-level AST (dj_cli.hpp)
// into the runtime IR, so a template authored once as a type can be rendered
// at compile time (PART A) or lowered and rendered at runtime (PART B).

NS_INTERNAL

    // buffer_to_std
    //   function: a dji_cli_string buffer as a std::string.
    D_NODISCARD inline std::string
    buffer_to_std(
        dji_cli_string<> _s
    )
    {
        return std::string(_s.data, _s.size);
    }

    // lower_node
    //   trait: AST node -> dji_cli_node (specialized per node-kind).
    template<typename Node>
    struct lower_node;

    // lower_value
    //   function: lower_node<Node>::go() as a callable.
    template<typename Node>
    D_NODISCARD dji_cli_node
    lower_value()
    {
        return lower_node<Node>::go();
    }

    template<dji_cli_string<> Buf>
    struct lower_node<option<dji_cli::lit, val_t<Buf>>>
    {
        static dji_cli_node go() { return dji_cli_lit(buffer_to_std(Buf)); }
    };

    template<dji_cli_string<> Key>
    struct lower_node<option<dji_cli::var, val_t<Key>>>
    {
        static dji_cli_node go() { return dji_cli_var(buffer_to_std(Key)); }
    };

    template<typename... Children>
    struct lower_node<option<dji_cli::seq, Children...>>
    {
        static dji_cli_node go()
        {
            return dji_cli_seq({ lower_value<Children>()... });
        }
    };

    template<typename... Children>
    struct lower_node<option<dji_cli::group, Children...>>
    {
        static dji_cli_node go()
        {
            return dji_cli_group({ lower_value<Children>()... });
        }
    };

    template<typename Child>
    struct lower_node<option<dji_cli::bold, Child>>
    {
        static dji_cli_node go() { return dji_cli_bold(lower_value<Child>()); }
    };

    template<typename Child>
    struct lower_node<option<dji_cli::upper, Child>>
    {
        static dji_cli_node go() { return dji_cli_to_upper(lower_value<Child>()); }
    };

    template<typename Child>
    struct lower_node<option<dji_cli::lower, Child>>
    {
        static dji_cli_node go() { return dji_cli_to_lower(lower_value<Child>()); }
    };

    template<std::size_t Width,
             typename    Child>
    struct lower_node<option<dji_cli::indent, val_t<Width>, Child>>
    {
        static dji_cli_node go()
        {
            return dji_cli_indent(Width, lower_value<Child>());
        }
    };

    template<dji_cli_string<> Id,
             typename         Value,
             typename         Body>
    struct lower_node<option<dji_cli::let_, val_t<Id>, Value, Body>>
    {
        static dji_cli_node go()
        {
            return dji_cli_let(buffer_to_std(Id),
                               lower_value<Value>(),
                               lower_value<Body>());
        }
    };

    template<typename Cond,
             typename Then,
             typename Else>
    struct lower_node<option<dji_cli::if_, Cond, Then, Else>>
    {
        static dji_cli_node go()
        {
            return dji_cli_if(lower_value<Cond>(),
                              lower_value<Then>(),
                              lower_value<Else>());
        }
    };

    template<dji_cli_string<> List,
             dji_cli_string<> Elem,
             typename         Body>
    struct lower_node<option<dji_cli::foreach_, val_t<List>, val_t<Elem>, Body>>
    {
        static dji_cli_node go()
        {
            return dji_cli_foreach(buffer_to_std(List),
                                   buffer_to_std(Elem),
                                   lower_value<Body>());
        }
    };

    template<typename    Child,
             typename... Rules>
    struct lower_node<option<dji_cli::rewrite_, Child, Rules...>>
    {
        static dji_cli_node go()
        {
            return dji_cli_rewrite(
                lower_value<Child>(),
                std::vector<dji_cli_rule>{
                    dji_cli_rule{ buffer_to_std(Rules::pattern),
                                  buffer_to_std(Rules::replacement),
                                  Rules::terminal,
                                  /*regex*/ false }... });
        }
    };

    template<dji_cli_string<> Self,
             typename         Body>
    struct lower_node<option<dji_cli::fix_, val_t<Self>, Body>>
    {
        static dji_cli_node go()
        {
            return dji_cli_fix(buffer_to_std(Self), lower_value<Body>());
        }
    };

NS_END  // internal


// dji_cli_lower
//   trait: distill a type-level CLI template Template into its runtime
// dji_cli_node IR.  `go()` performs the construction.
template<typename Template>
struct dji_cli_lower
{
    D_NODISCARD static dji_cli_node
    go()
    {
        return internal::lower_value<Template>();
    }
};


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER ...

#endif  // floor, for now


#endif  // DJINTERP_CLI_CLI_RENDER_HPP
