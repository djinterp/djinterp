/*******************************************************************************
* djinterp [cpp/parsegen]                                           notation.cpp
*
* Definitions for `notation.hpp`.
*
*
* path:      /src/djinterp/cpp/modules/parsegen/vparse/notation.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.16
*******************************************************************************/
// djinterp
#include "djinterp/cpp/modules/parsegen/vparse/notation.hpp"
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has
#include "djinterp/cpp/modules/parsegen/vparse/gen.hpp"      // gen::compile
#include "djinterp/cpp/modules/parsegen/vparse/machine.hpp"  // machine, the reader's limit

// std
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits

NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

namespace notation {

namespace {

const std::string WS      = " \t\n\r";
const std::string IDSTART = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
const std::string IDREST  = IDSTART + "0123456789";
const std::string CLASSCH = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-";

// expand_class
//   expand a notation class body into explicit membership: `0-9` -> the ten
// digits, ranges concatenate, a stray `-` is literal.
std::string expand_class(
    const std::string& _body
)
{
    std::string out;
    std::size_t i = 0;
    while (i < _body.size())
    {
        if ( ((i + 2) < _body.size()) && (_body[i + 1] == '-') )
        {
            for (char c = _body[i]; c <= _body[i + 2]; ++c) { out.push_back(c); }
            i += 3;
        }
        else { out.push_back(_body[i]); i += 1; }
    }
    return out;
}

// READ_STEPS_PER_BYTE
//   constant: the steps the reader is given for each byte of the text it
// reads, on top of a machine's default limit.
const long READ_STEPS_PER_BYTE = 128;

D_STATIC_ASSERT(READ_STEPS_PER_BYTE > 0,
                "read_limit() divides by READ_STEPS_PER_BYTE");

}  // anonymous namespace

// meta_grammar
//   function: the grammar of grammars -- the notation's syntax as a ruleset,
// so the generator compiles the parser that reads grammar text.  The captured
// spans (NAME / SETBODY / REF / STAR / PLUS / LITCH / BAR) drive the reader.
ruleset meta_grammar()
{
    ruleset g;

    // Grammar := WS* Rule*
    g.rules.push_back(rule{ "Grammar",
        { { cls(WS, true), ref("Rule", true) } } });

    // Rule := Ident{NAME} WS* '=' Alt ('|'{BAR} Alt)* WS* ';' WS*
    g.rules.push_back(rule{ "Rule",
        { { ref("Ident", false, NAME), cls(WS, true), lit('='),
            ref("Alt"), ref("BarAlt", true),
            cls(WS, true), lit(';'), cls(WS, true) } } });

    // BarAlt := WS* '|'{BAR} Alt
    g.rules.push_back(rule{ "BarAlt",
        { { cls(WS, true), lit('|', BAR), ref("Alt") } } });

    // Alt := TermWs*
    g.rules.push_back(rule{ "Alt",
        { { ref("TermWs", true) } } });

    // TermWs := WS* Term
    g.rules.push_back(rule{ "TermWs",
        { { cls(WS, true), ref("Term") } } });

    // Term := StarTerm | PlusTerm | PlainTerm
    g.rules.push_back(rule{ "Term",
        { { ref("StarTerm") }, { ref("PlusTerm") }, { ref("PlainTerm") } } });

    // StarTerm := Atom '*'{STAR}
    g.rules.push_back(rule{ "StarTerm",
        { { ref("Atom"), lit('*', STAR) } } });

    // PlusTerm := Atom '+'{PLUS}
    g.rules.push_back(rule{ "PlusTerm",
        { { ref("Atom"), lit('+', PLUS) } } });

    // PlainTerm := Atom
    g.rules.push_back(rule{ "PlainTerm",
        { { ref("Atom") } } });

    // Atom := SetTerm | LitTerm | RefTerm
    g.rules.push_back(rule{ "Atom",
        { { ref("SetTerm") }, { ref("LitTerm") }, { ref("RefTerm") } } });

    // SetTerm := '[' ClassBody{SETBODY} ']'
    g.rules.push_back(rule{ "SetTerm",
        { { lit('['), ref("ClassBody", false, SETBODY), lit(']') } } });

    // LitTerm := '\'' any{LITCH} '\''
    g.rules.push_back(rule{ "LitTerm",
        { { lit('\''), any_ch(LITCH), lit('\'') } } });

    // RefTerm := Ident{REF}
    g.rules.push_back(rule{ "RefTerm",
        { { ref("Ident", false, REF) } } });

    // Ident := [A-Za-z_] [A-Za-z0-9_]*
    g.rules.push_back(rule{ "Ident",
        { { cls(IDSTART), cls(IDREST, true) } } });

    // ClassBody := [0-9A-Za-z-] [0-9A-Za-z-]*
    g.rules.push_back(rule{ "ClassBody",
        { { cls(CLASSCH), cls(CLASSCH, true) } } });

    return g;
}

// read_ruleset
//   function: fold the ordered capture spans into a target ruleset.  NAME
// starts a rule (and resets to its first alternative); BAR opens a new
// alternative; REF / SETBODY / LITCH append a term; STAR marks the last term;
// PLUS expands `x+` into `x x*`.
ruleset read_ruleset(
    const std::vector<peg::capture>& _caps,
    const std::string&               _text
)
{
    ruleset out;
    int     curi   = -1;
    int     curalt = 0;

    for (const peg::capture& c : _caps)
    {
        const std::string txt = _text.substr(c.start, c.end - c.start);

        if (c.tag == NAME)
        {
            rule r; r.name = txt; r.alts.push_back({});
            out.rules.push_back(r);
            curi   = static_cast<int>(out.rules.size()) - 1;
            curalt = 0;
        }
        else if (c.tag == BAR)
        {
            out.rules[curi].alts.push_back({});
            curalt = static_cast<int>(out.rules[curi].alts.size()) - 1;
        }
        else if (c.tag == REF)
        {
            out.rules[curi].alts[curalt].push_back(ref(txt));
        }
        else if (c.tag == SETBODY)
        {
            out.rules[curi].alts[curalt].push_back(cls(expand_class(txt)));
        }
        else if (c.tag == LITCH)
        {
            out.rules[curi].alts[curalt].push_back(lit(txt.empty() ? '\0' : txt[0]));
        }
        else if (c.tag == STAR)
        {
            out.rules[curi].alts[curalt].back().star = true;
        }
        else if (c.tag == PLUS)
        {
            term dup = out.rules[curi].alts[curalt].back();
            dup.star = true;
            out.rules[curi].alts[curalt].push_back(dup);
        }
    }

    return out;
}

/*
read_limit
  Reading is linear in the text: the grammar of grammars backtracks over one
term at most, so no byte is read more than a few times over. The most it was
measured to take is 61.5 steps a byte, for alternatives of one reference
(`S=a|a|a...;`), the worst of every unit of up to six characters repeated;
READ_STEPS_PER_BYTE is about twice that. A limit is kept at all so that a
fault in meta_grammar() that made the reader loop still ends in a refusal.
Where a `long` is 32 bits, the budget stops growing at about 16 MB of text.
*/
long read_limit(
    std::size_t _length
)
{
    const long base = machine().step_limit;
    const long most = std::numeric_limits<long>::max();

    // a budget that would not fit in a long is the most a long holds
    if (_length > static_cast<std::size_t>((most - base) /
                                           READ_STEPS_PER_BYTE))
    {
        return most;
    }

    return base + (static_cast<long>(_length) * READ_STEPS_PER_BYTE);
}

/*
parse_grammar
  The whole text -> ruleset front-end: compile the grammar of grammars to a
parser, run it over the text, and fold the captures. The reader runs on a
machine of its own. A machine's default limit is set for a match over a
typed input, and a grammar of a few hundred rules takes the reader more steps
than that, so the reader is given steps in proportion to the text instead
(read_limit).
*/
ruleset parse_grammar(
    const std::string& _text,
    bool*              _ok
)
{
    const peg::program grammar_parser = gen::compile(meta_grammar());
    const op_set       ops            = peg::make_ops();

    machine                   reader;
    parse_state<char>         state(_text.data(),
                                    _text.size(),
                                    0);
    std::vector<peg::capture> caps;

    reader.step_limit = read_limit(_text.size());

    const bool good = peg::run(reader,
                               state,
                               grammar_parser,
                               ops,
                               &caps);

    // a caller that asked is told whether the text was read
    if (_ok != nullptr)
    {
        *_ok = good;
    }

    // a text that is not a grammar defines no rules
    if (!good)
    {
        return ruleset{};
    }

    return read_ruleset(caps,
                        _text);
}

}  // namespace notation

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now
