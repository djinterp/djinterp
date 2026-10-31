/*******************************************************************************
* djinterp [cpp/parsegen]                                             corpus.cpp
*
* Definitions for `corpus.hpp`.
*   A walk's shortest way out is measured in rules, not characters: a rule's
* height is the fewest rules, one inside the next, that any derivation of it
* must expand, found by a fixpoint that starts every rule at "no way out" and
* lowers each to what its body gives until none moves. Past the depth or the
* length a walk takes, at each choice, the alternative of least height, and
* no repetition and no option: each rule it then enters is lower than the one
* it left, so the walk ends however the rules recurse.
*   The generator's draws are reduced by remainder. Its bias, for the small
* ranges drawn here, is below one part in 2^50, and the remainder is the same
* on every platform, as std::uniform_int_distribution's results are not.
*
*
* path:      /src/djinterp/cpp/modules/parsegen/vparse/corpus.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.17
*******************************************************************************/
#include "djinterp/cpp/modules/parsegen/vparse/corpus.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <algorithm>  // std::max, std::min, std::sort, std::unique
#include <cstddef>    // std::size_t
#include <map>        // std::map
#include <memory>     // std::make_shared
#include <set>        // std::set
#include <string>     // std::string
#include <utility>    // std::make_pair
#include <vector>     // std::vector
// djinterp
#include "djinterp/cpp/modules/parse/parse.hpp"  // parse_state


NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

namespace corpus
{

namespace
{

// NONE
//   constant: the height of a rule, or a node, with no way out.
const std::size_t NONE = static_cast<std::size_t>(-1);

// LETTERS
//   constant: what a walk draws `.` from in a grammar that names no
// character at all.
const char* const LETTERS = "abcdefghijklmnopqrstuvwxyz0123456789";

// walker
//   struct: one corpus being made: the rules and their places by name, each
// rule's height, the characters the grammar names, how to walk, and the
// generator's state.
struct walker
{
    const ebnf::rules*                 rules = nullptr;
    std::map<std::string, std::size_t> index;
    std::vector<std::size_t>           heights;
    std::string                        alphabet;
    const settings*                    how   = nullptr;
    re_std::uint64_t                   state = 0u;
};

/**
 * @brief Draws the generator's next value: splitmix64 (Steele, Lea and
 *        Flood, 2014).
 *
 * @param[in,out] _walker  the corpus being made; its state advances.
 * @return 64 bits.
 */
re_std::uint64_t next(
    walker& _walker
)
{
    re_std::uint64_t mixed = (_walker.state += 0x9E3779B97F4A7C15ull);

    mixed = (mixed ^ (mixed >> 30)) * 0xBF58476D1CE4E5B9ull;
    mixed = (mixed ^ (mixed >> 27)) * 0x94D049BB133111EBull;

    return mixed ^ (mixed >> 31);
}

/**
 * @brief Draws a number below a bound.
 *
 * @param[in,out] _walker  the corpus being made; its state advances.
 * @param[in]     _bound   the bound; at least 1.
 * @return a number from 0 to `_bound - 1`.
 */
std::size_t below(
    walker&     _walker,
    std::size_t _bound
)
{
    return static_cast<std::size_t>(next(_walker) %
                                    static_cast<re_std::uint64_t>(_bound));
}

// raised
//   function: a height one rule higher, where it has one.
std::size_t raised(
    std::size_t _height
)
{
    return (_height == NONE) ? NONE
                             : _height + 1u;
}

// height_of
//   function: a node's height, by the rules' heights as they stand.
std::size_t height_of(
    const ebnf::node& _node,
    const walker&     _walker
)
{
    std::size_t out = 0u;

    switch (_node.kind)
    {
        // text, or nothing at all
        case ebnf::N_LIT:
        case ebnf::N_ANY:
        case ebnf::N_OPT:
        case ebnf::N_STAR:
        case ebnf::N_NOT:
            break;

        // a class with no member matches nothing
        case ebnf::N_SET:
            out = _node.text.empty() ? NONE
                                     : 0u;
            break;

        // a rule, one higher; a rule nothing defines has no way out
        case ebnf::N_REF:
        {
            const auto found = _walker.index.find(_node.text);

            out = (found == _walker.index.end())
                      ? NONE
                      : raised(_walker.heights[found->second]);
            break;
        }

        // every part, so the highest of them
        case ebnf::N_SEQ:
            // part by part
            for (const ebnf::node_ptr& kid : _node.kids)
            {
                out = std::max(out,
                               height_of(*kid,
                                         _walker));
            }
            break;

        // any one of them, so the lowest
        case ebnf::N_CHOICE:
            out = NONE;

            // alternative by alternative
            for (const ebnf::node_ptr& kid : _node.kids)
            {
                out = std::min(out,
                               height_of(*kid,
                                         _walker));
            }
            break;

        // at least once
        case ebnf::N_PLUS:
            out = _node.kids.empty() ? 0u
                                     : height_of(*_node.kids[0],
                                                 _walker);
            break;

        default:
            out = NONE;
            break;
    }

    return out;
}

/**
 * @brief Walks one node at random, appending the text it derives.
 *
 * @param[in,out] _walker  the corpus being made.
 * @param[in]     _node    the node; its height is not NONE, so it ends.
 * @param[in]     _depth   how many rules the walk is inside.
 * @param[in,out] _out     the text so far.
 */
void walk(
    walker&           _walker,
    const ebnf::node& _node,
    std::size_t       _depth,
    std::string&      _out
)
{
    const bool cheap = ( (_depth >= _walker.how->depth) ||
                         (_out.size() >= _walker.how->length) );

    switch (_node.kind)
    {
        case ebnf::N_LIT:
            _out += _node.text;
            break;

        case ebnf::N_ANY:
            _out += _walker.alphabet[below(_walker,
                                           _walker.alphabet.size())];
            break;

        case ebnf::N_SET:
            _out += _node.text[below(_walker,
                                     _node.text.size())];
            break;

        // into the rule, one deeper
        case ebnf::N_REF:
        {
            const std::size_t at = _walker.index.at(_node.text);

            walk(_walker,
                 *(*_walker.rules)[at].second,
                 _depth + 1u,
                 _out);
            break;
        }

        // each part in order
        case ebnf::N_SEQ:
            // part by part
            for (const ebnf::node_ptr& kid : _node.kids)
            {
                walk(_walker,
                     *kid,
                     _depth,
                     _out);
            }
            break;

        // the lowest way out, or any way out at all
        case ebnf::N_CHOICE:
        {
            std::vector<std::size_t> ways;
            std::size_t              lowest = NONE;

            // the alternatives that end, and the height of the lowest
            for (std::size_t at = 0u; at < _node.kids.size(); at++)
            {
                const std::size_t height = height_of(*_node.kids[at],
                                                     _walker);

                // an alternative with no way out is never taken
                if (height != NONE)
                {
                    ways.push_back(at);
                    lowest = std::min(lowest,
                                      height);
                }
            }

            std::size_t chosen = ways[0];

            // past the depth or the length, the first of the lowest
            if (cheap)
            {
                // the alternatives that end, in order
                for (const std::size_t way : ways)
                {
                    // the first that low
                    if (height_of(*_node.kids[way],
                                  _walker) == lowest)
                    {
                        chosen = way;

                        break;
                    }
                }
            }
            else
            {
                chosen = ways[below(_walker,
                                    ways.size())];
            }

            walk(_walker,
                 *_node.kids[chosen],
                 _depth,
                 _out);
            break;
        }

        // taken half the time, until the walk is past its depth or length,
        // where what it holds can end
        case ebnf::N_OPT:
        {
            const bool ends = (height_of(*_node.kids[0],
                                         _walker) != NONE);

            // within the limits, an option that ends, and the draw for it
            if ( (!cheap) &&
                 (ends)   &&
                 (below(_walker,
                        2u) == 1u) )
            {
                walk(_walker,
                     *_node.kids[0],
                     _depth,
                     _out);
            }

            break;
        }

        // up to the repeats, or once for `+`, and none past the limits
        case ebnf::N_STAR:
        case ebnf::N_PLUS:
        {
            const bool        ends  = (height_of(*_node.kids[0],
                                                 _walker) != NONE);
            const std::size_t least = (_node.kind == ebnf::N_PLUS) ? 1u
                                                                   : 0u;
            const std::size_t most  = std::max(least,
                                               _walker.how->repeats);
            const std::size_t times = (!ends) ? 0u
                                    : cheap   ? least
                                              : least +
                                                below(_walker,
                                                      most - least + 1u);

            // each round, while the text is short enough for another
            for (std::size_t round = 0u; round < times; round++)
            {
                // the length reached: no more rounds than must be
                if ( (round >= least) &&
                     (_out.size() >= _walker.how->length) )
                {
                    break;
                }

                walk(_walker,
                     *_node.kids[0],
                     _depth,
                     _out);
            }
            break;
        }

        // a predicate derives no text
        default:
            break;
    }

    return;
}

/**
 * @brief Copies a text with one character deleted, inserted or replaced.
 *
 * @param[in,out] _walker  the corpus being made.
 * @param[in]     _text    the text.
 * @return the copy; an empty text can only gain a character.
 */
std::string near_miss(
    walker&            _walker,
    const std::string& _text
)
{
    std::string       out  = _text;
    const std::size_t kind = _text.empty() ? 1u
                                           : below(_walker,
                                                   3u);
    const char        with = _walker.alphabet[below(_walker,
                                                    _walker.alphabet.size())];

    // delete, insert, or replace
    if (kind == 0u)
    {
        out.erase(below(_walker,
                        out.size()),
                  1u);
    }
    else if (kind == 1u)
    {
        out.insert(below(_walker,
                         out.size() + 1u),
                   1u,
                   with);
    }
    else
    {
        const std::size_t at = below(_walker,
                                     out.size());

        // a replacement that changes the character, where one can
        if ( (out[at] == with) &&
             (_walker.alphabet.size() > 1u) )
        {
            const std::size_t was = _walker.alphabet.find(with);

            out[at] = _walker.alphabet[(was + 1u) % _walker.alphabet.size()];
        }
        else
        {
            out[at] = with;
        }
    }

    return out;
}

/**
 * @brief Gathers every character a grammar names, in literals and classes.
 *
 * @param[in]     _node  a node of it.
 * @param[in,out] _seen  the characters so far.
 */
void gather(
    const ebnf::node& _node,
    std::set<char>&   _seen
)
{
    // a literal's characters, or a class's members
    if ( (_node.kind == ebnf::N_LIT) ||
         (_node.kind == ebnf::N_SET) )
    {
        _seen.insert(_node.text.begin(),
                     _node.text.end());
    }

    // and the nodes below
    for (const ebnf::node_ptr& kid : _node.kids)
    {
        gather(*kid,
               _seen);
    }

    return;
}

/**
 * @brief Makes a node of a kind.
 *
 * @param[in] _kind  the kind.
 * @param[in] _text  its text.
 * @return the node, with no children.
 */
ebnf::node_ptr node_of(
    int                _kind,
    const std::string& _text = std::string()
)
{
    ebnf::node_ptr out = std::make_shared<ebnf::node>();

    out->kind = _kind;
    out->text = _text;

    return out;
}

}  // anonymous namespace


/*
generate (EBNF)
  The heights first, by fixpoint; then the walks, each a fresh text from the
start rule, and its near-misses after it, a text kept only the first time it
is made.
*/
std::vector<sample> generate(
    const ebnf::rules& _rules,
    const settings&    _settings
)
{
    std::vector<sample>   out;
    walker                w;
    std::set<char>        seen;
    std::set<std::string> kept;

    w.rules = &_rules;
    w.how   = &_settings;
    w.state = _settings.seed;
    w.heights.assign(_rules.size(),
                     NONE);

    // each rule by name, the first of a name where two share it, and every
    // character the grammar names
    for (std::size_t at = 0u; at < _rules.size(); at++)
    {
        (void)w.index.insert(std::make_pair(_rules[at].first,
                                            at));

        // a rule with a body
        if (_rules[at].second)
        {
            gather(*_rules[at].second,
                   seen);
        }
    }

    w.alphabet = seen.empty() ? std::string(LETTERS)
                              : std::string(seen.begin(),
                                            seen.end());

    bool moved = true;

    // the heights, lowered until none moves
    while (moved)
    {
        moved = false;

        // each rule, by its body as the others now stand
        for (std::size_t at = 0u; at < _rules.size(); at++)
        {
            const std::size_t height = _rules[at].second
                                           ? height_of(*_rules[at].second,
                                                       w)
                                           : NONE;

            // lower than it was
            if (height < w.heights[at])
            {
                w.heights[at] = height;
                moved         = true;
            }
        }
    }

    // nothing to walk from
    if ( (_rules.empty())          ||
         (w.heights[0] == NONE) )
    {
        return out;
    }

    // a text, kept the first time it is made
    const auto keep = [&out, &kept](const std::string& _text,
                                    bool               _derived)
    {
        // a text made before
        if (!kept.insert(_text).second)
        {
            return;
        }

        sample one;

        one.text    = _text;
        one.derived = _derived;
        out.push_back(one);

        return;
    };

    // each walk, and its near-misses
    for (std::size_t round = 0u; round < _settings.walks; round++)
    {
        std::string text;

        walk(w,
             *_rules[0].second,
             0u,
             text);
        keep(text,
             true);

        // each near-miss of it
        for (std::size_t miss = 0u; miss < _settings.misses; miss++)
        {
            keep(near_miss(w,
                           text),
                 false);
        }
    }

    return out;
}

/*
generate (notation)
  The notation's rules as EBNF's tree spells them: a rule a choice of its
alternatives, an alternative a sequence of its terms, and a starred term a
repetition of it.
*/
std::vector<sample> generate(
    const ruleset&  _grammar,
    const settings& _settings
)
{
    ebnf::rules tree;

    // each rule
    for (const rule& each : _grammar.rules)
    {
        ebnf::node_ptr choice = node_of(ebnf::N_CHOICE);

        // each alternative
        for (const std::vector<term>& alt : each.alts)
        {
            ebnf::node_ptr sequence = node_of(ebnf::N_SEQ);

            // each term, as the node it is
            for (const term& part : alt)
            {
                ebnf::node_ptr one;

                // the kind of term
                if (part.kind == T_CHAR)
                {
                    one = node_of(ebnf::N_LIT,
                                  std::string(1u,
                                              part.ch));
                }
                else if (part.kind == T_SET)
                {
                    one = node_of(ebnf::N_SET,
                                  part.set);
                }
                else if (part.kind == T_REF)
                {
                    one = node_of(ebnf::N_REF,
                                  part.ref);
                }
                else
                {
                    one = node_of(ebnf::N_ANY);
                }

                // a starred term is a repetition of it
                if (part.star)
                {
                    ebnf::node_ptr repeated = node_of(ebnf::N_STAR);

                    repeated->kids.push_back(one);
                    one = repeated;
                }

                sequence->kids.push_back(one);
            }

            choice->kids.push_back(sequence);
        }

        tree.push_back(std::make_pair(each.name,
                                      choice));
    }

    return generate(tree,
                    _settings);
}

/*
judge
  One run, on a machine of its own with the given limit, untraced. A run that
finished is one the machine halted -- a MATCH, or a failure with nowhere to
go back to -- rather than one the step limit stopped.
*/
verdict judge(
    const peg::program& _program,
    const std::string&  _input,
    long                _step_limit
)
{
    static const op_set ops = peg::make_ops();
    verdict             out;
    parse_state<char>   state(_input.data(),
                              _input.size(),
                              0);
    machine             m;

    m.step_limit = _step_limit;

    out.ok       = peg::run(m,
                            state,
                            _program,
                            ops);
    out.consumed = out.ok ? state.offset
                          : 0u;
    out.steps    = m.steps;
    out.finished = m.halted;

    return out;
}

/*
compare
  Both programs over every input, in corpus order; an input is a difference
when either the verdict, the length read, or having finished differs.
*/
comparison compare(
    const peg::program&             _first,
    const peg::program&             _second,
    const std::vector<std::string>& _inputs,
    long                            _step_limit
)
{
    comparison out;

    out.inputs = _inputs.size();

    // input by input
    for (std::size_t at = 0u; at < _inputs.size(); at++)
    {
        difference one;

        one.index  = at;
        one.first  = judge(_first,
                           _inputs[at],
                           _step_limit);
        one.second = judge(_second,
                           _inputs[at],
                           _step_limit);

        out.first_steps  += one.first.steps;
        out.second_steps += one.second.steps;
        out.matched      += ( (one.first.ok) &&
                              (one.second.ok) ) ? 1u : 0u;

        // the two said different things
        if ( (one.first.ok != one.second.ok)              ||
             (one.first.consumed != one.second.consumed)  ||
             (one.first.finished != one.second.finished) )
        {
            out.differences.push_back(one);
        }
    }

    return out;
}

}  // namespace corpus

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now
