/*******************************************************************************
* djinterp [parsegen]                                                   ebnf.cpp
*
*   Definitions for ebnf.hpp: the tokenizer, the recursive-descent parser, and
* the compiler from syntax tree to PEG program.
*   Consolidated from the standalone prototype. The parser still reports errors
* by throwing internally -- it is the natural shape for recursive descent -- but
* both public functions catch at the boundary and report through `_ok`, so no
* exception escapes. Two latent faults were fixed on the way: a class range
* ending at the top of `char` looped forever, and a duplicated rule name was
* silently overwritten instead of reported.
*
*
* path:      /src/djinterp/parsegen/vparse/ebnf.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.01
*******************************************************************************/
#include "djinterp/parsegen/vparse/ebnf.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has
// std
#include <cctype>                             // std::isalpha, std::isalnum
#include <cstddef>                            // std::size_t
#include <exception>                          // std::exception
#include <map>                                // std::map
#include <stdexcept>                          // std::runtime_error
#include <string>                             // std::string
#include <utility>                            // std::move, std::pair
#include <vector>                             // std::vector


NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

namespace ebnf
{

namespace
{

// make
//   function: builds one syntax-tree node.
node_ptr
make(
    int                   _kind,
    std::string           _text = {},
    std::vector<node_ptr> _kids = {}
)
{
    node_ptr built = std::make_shared<node>();

    built->kind = _kind;
    built->text = std::move(_text);
    built->kids = std::move(_kids);

    return built;
}

node_ptr lit(std::string _s)               { return make(N_LIT, std::move(_s)); }
node_ptr any()                             { return make(N_ANY); }
node_ptr set(std::string _s)               { return make(N_SET, std::move(_s)); }
node_ptr ref(std::string _s)               { return make(N_REF, std::move(_s)); }
node_ptr seq(std::vector<node_ptr> _k)     { return make(N_SEQ, {}, std::move(_k)); }
node_ptr choice(std::vector<node_ptr> _k)  { return make(N_CHOICE, {}, std::move(_k)); }
node_ptr opt(node_ptr _n)                  { return make(N_OPT, {}, { std::move(_n) }); }
node_ptr star(node_ptr _n)                 { return make(N_STAR, {}, { std::move(_n) }); }
node_ptr plus(node_ptr _n)                 { return make(N_PLUS, {}, { std::move(_n) }); }
node_ptr negate(node_ptr _n)               { return make(N_NOT, {}, { std::move(_n) }); }


//------------------------------------------------------------------------------
// compiler: syntax tree -> PEG program
//------------------------------------------------------------------------------

// item
//   struct: one instruction or label in the pre-assembly stream. Branch targets
// are symbolic labels until assemble() resolves them to addresses.
struct item
{
    int         op;
    bool        is_label;
    int         label;
    char        ch;
    std::string members;

    // item
    //   constructor: every field named, as each emission site gives them.
    // (Default member initializers would make item a non-aggregate at C++11,
    // where brace-initializing it needs this constructor.)
    item(
        int         _op,
        bool        _is_label,
        int         _label,
        char        _ch,
        std::string _members
    )
        : op(_op),
          is_label(_is_label),
          label(_label),
          ch(_ch),
          members(std::move(_members))
    {}
};

// labels
//   struct: a label counter.
struct labels
{
    int n = 0;

    int
    next()
    {
        return ++n;
    }
};

// emit
//   function: appends the instructions for one subtree. The choice encodings are
// the standard PEG ones -- CHOICE/COMMIT cascades, with `!p` as CHOICE past p,
// then a COMMIT into an unconditional FAIL.
void
emit(
    const node_ptr&                   _n,
    std::vector<item>&                _out,
    labels&                           _lab,
    const std::map<std::string, int>& _rule
)
{
    // (no default arguments: a lambda's are C++14)
    auto instr_ = [&](int _op, int _label, char _ch)
    {
        _out.push_back(item{ _op, false, _label, _ch, {} });
    };
    auto label_ = [&](int _id)
    {
        _out.push_back(item{ peg::MATCH, true, _id, 0, {} });
    };

    switch (_n->kind)
    {
        case N_LIT:
            for (const char c : _n->text)
            {
                instr_(peg::CHAR, -1, c);
            }
            break;

        case N_ANY:
            instr_(peg::ANY, -1, 0);
            break;

        case N_SET:
            _out.push_back(item{ peg::SET, false, -1, 0, _n->text });
            break;

        case N_REF:
        {
            const auto found = _rule.find(_n->text);

            if (found == _rule.end())
            {
                throw std::runtime_error("rule not defined: " + _n->text);
            }

            instr_(peg::CALL, found->second, 0);
            break;
        }

        case N_SEQ:
            for (const node_ptr& kid : _n->kids)
            {
                emit(kid, _out, _lab, _rule);
            }
            break;

        case N_CHOICE:
        {
            const int end = _lab.next();

            for (std::size_t i = 0; (i + 1) < _n->kids.size(); ++i)
            {
                const int after = _lab.next();

                instr_(peg::CHOICE, after, 0);
                emit(_n->kids[i], _out, _lab, _rule);
                instr_(peg::COMMIT, end, 0);
                label_(after);
            }

            emit(_n->kids.back(), _out, _lab, _rule);
            label_(end);
            break;
        }

        case N_OPT:
        {
            const int after = _lab.next();

            instr_(peg::CHOICE, after, 0);
            emit(_n->kids[0], _out, _lab, _rule);
            instr_(peg::COMMIT, after, 0);
            label_(after);
            break;
        }

        case N_STAR:
        {
            const int top = _lab.next();
            const int end = _lab.next();

            label_(top);
            instr_(peg::CHOICE, end, 0);
            emit(_n->kids[0], _out, _lab, _rule);
            instr_(peg::COMMIT, top, 0);
            label_(end);
            break;
        }

        case N_PLUS:
        {
            emit(_n->kids[0], _out, _lab, _rule);

            const int top = _lab.next();
            const int end = _lab.next();

            label_(top);
            instr_(peg::CHOICE, end, 0);
            emit(_n->kids[0], _out, _lab, _rule);
            instr_(peg::COMMIT, top, 0);
            label_(end);
            break;
        }

        case N_NOT:
        {
            const int matched = _lab.next();
            const int fail    = _lab.next();

            instr_(peg::CHOICE, matched, 0);
            emit(_n->kids[0], _out, _lab, _rule);
            instr_(peg::COMMIT, fail, 0);
            label_(fail);
            instr_(peg::FAIL, -1, 0);
            label_(matched);
            break;
        }

        default:
            throw std::runtime_error("unknown syntax-tree node");
    }

    return;
}

// assemble
//   function: resolves labels to addresses and produces the program.
peg::program
assemble(
    const std::vector<item>& _items
)
{
    std::map<int, int> label_at;
    int                address = 0;

    for (const item& it : _items)
    {
        if (it.is_label)
        {
            label_at[it.label] = address;
        }
        else
        {
            ++address;
        }
    }

    peg::program prog;

    for (const item& it : _items)
    {
        if (it.is_label)
        {
            continue;
        }

        peg::instr ins{};

        ins.op = it.op;

        const bool branches = ( (it.op == peg::CHOICE) ||
                                (it.op == peg::JUMP)   ||
                                (it.op == peg::CALL)   ||
                                (it.op == peg::COMMIT) );

        if (branches)
        {
            ins.arg = label_at.at(it.label);
        }
        else if (it.op == peg::CHAR)
        {
            ins.ch = it.ch;
        }
        else if (it.op == peg::SET)
        {
            ins.set = it.members;
        }

        prog.push_back(ins);
    }

    return prog;
}


//------------------------------------------------------------------------------
// tokenizer and recursive-descent parser
//------------------------------------------------------------------------------

// token kinds
enum class tk
{
    id,
    str,
    set,
    pun,
    end
};

// token
//   struct: one lexical token.
struct token
{
    tk          kind;
    std::string text;
};

// tokenize
//   function: splits grammar text into tokens. Class ranges are expanded here to
// an explicit membership string, which is the form a SET instruction carries.
std::vector<token>
tokenize(
    const std::string& _src
)
{
    std::vector<token> out;
    std::size_t        i = 0;
    const std::size_t  n = _src.size();

    auto id_start = [](char _c)
    {
        return (std::isalpha(static_cast<unsigned char>(_c)) || (_c == '_'));
    };
    auto id_cont = [](char _c)
    {
        return (std::isalnum(static_cast<unsigned char>(_c)) || (_c == '_'));
    };

    while (i < n)
    {
        const char c = _src[i];

        if ( (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n') )
        {
            i++;
        }
        else if (c == '#')
        {
            // a comment runs to the end of the line
            while ( (i < n) && (_src[i] != '\n') )
            {
                i++;
            }
        }
        else if ( (c == '"') || (c == '\'') )
        {
            const char  quote = c;
            std::string body;

            i++;

            while ( (i < n) && (_src[i] != quote) )
            {
                if ( (_src[i] == '\\') && ((i + 1) < n) )
                {
                    body += _src[i + 1];
                    i    += 2;
                }
                else
                {
                    body += _src[i];
                    i++;
                }
            }

            if (i >= n)
            {
                throw std::runtime_error("unterminated string literal");
            }

            i++;
            out.push_back(token{ tk::str, body });
        }
        else if (c == '[')
        {
            std::string members;

            i++;

            while ( (i < n) && (_src[i] != ']') )
            {
                if ( ((i + 2) < n) && (_src[i + 1] == '-') && (_src[i + 2] != ']') )
                {
                    // an int counter: a range ending at the top of char must
                    // terminate, which a char counter cannot guarantee
                    const int low  = static_cast<unsigned char>(_src[i]);
                    const int high = static_cast<unsigned char>(_src[i + 2]);

                    for (int x = low; x <= high; ++x)
                    {
                        members += static_cast<char>(x);
                    }

                    i += 3;
                }
                else
                {
                    members += _src[i];
                    i++;
                }
            }

            if (i >= n)
            {
                throw std::runtime_error("unterminated character class");
            }

            i++;
            out.push_back(token{ tk::set, members });
        }
        else if (id_start(c))
        {
            const std::size_t j = i;

            while ( (i < n) && id_cont(_src[i]) )
            {
                i++;
            }

            out.push_back(token{ tk::id, _src.substr(j, i - j) });
        }
        else if (std::string("=|()?*+;.").find(c) != std::string::npos)
        {
            out.push_back(token{ tk::pun, std::string(1, c) });
            i++;
        }
        else
        {
            throw std::runtime_error(std::string("unexpected character: ") + c);
        }
    }

    out.push_back(token{ tk::end, "" });

    return out;
}

// parser
//   struct: the recursive-descent parser over a token stream.
//     grammar := rule*
//     rule    := ID '=' expr ';'
//     expr    := seq ('|' seq)*
//     seq     := postfix*
//     postfix := primary ('?' | '*' | '+')*
//     primary := STR | SET | '.' | ID | '(' expr ')'
struct parser
{
    std::vector<token> t;
    std::size_t        i;

    // parser
    //   constructor: a parser at the start of _tokens.
    explicit parser(
        std::vector<token> _tokens
    )
        : t(std::move(_tokens)),
          i(0)
    {}

    const token& peek() const { return t[i]; }
    token        next()       { return t[i++]; }

    bool
    is_pun(
        const char* _p
    ) const
    {
        return ( (peek().kind == tk::pun) && (peek().text == _p) );
    }

    void
    expect_pun(
        const char* _p
    )
    {
        if (!is_pun(_p))
        {
            throw std::runtime_error(std::string("expected '") + _p + "'");
        }

        next();

        return;
    }

    std::pair<rules, std::string>
    grammar()
    {
        rules       parsed;
        std::string start;

        while (peek().kind != tk::end)
        {
            std::pair<std::string, node_ptr> r = rule();

            // the first rule declared is the start symbol
            if (start.empty())
            {
                start = r.first;
            }

            parsed.push_back(std::move(r));
        }

        return { parsed, start };
    }

    std::pair<std::string, node_ptr>
    rule()
    {
        if (peek().kind != tk::id)
        {
            throw std::runtime_error("expected a rule name");
        }

        std::string name = next().text;

        expect_pun("=");

        node_ptr body = expr();

        expect_pun(";");

        return { name, body };
    }

    node_ptr
    expr()
    {
        std::vector<node_ptr> alts{ sequence() };

        while (is_pun("|"))
        {
            next();
            alts.push_back(sequence());
        }

        return (alts.size() == 1) ? alts[0] : choice(alts);
    }

    node_ptr
    sequence()
    {
        std::vector<node_ptr> items;

        while (!( (peek().kind == tk::end) ||
                  is_pun("|")              ||
                  is_pun(";")              ||
                  is_pun(")")              ))
        {
            items.push_back(postfix());
        }

        // an empty alternative matches nothing, which the empty literal is
        if (items.empty())
        {
            return lit("");
        }

        return (items.size() == 1) ? items[0] : seq(items);
    }

    node_ptr
    postfix()
    {
        node_ptr n = primary();

        while (is_pun("?") || is_pun("*") || is_pun("+"))
        {
            const std::string p = next().text;

            n = (p == "?") ? opt(n)
              : (p == "*") ? star(n)
              :              plus(n);
        }

        return n;
    }

    node_ptr
    primary()
    {
        const token tok = peek();

        if (tok.kind == tk::str) { next(); return lit(tok.text); }
        if (tok.kind == tk::set) { next(); return set(tok.text); }
        if (is_pun("."))         { next(); return any(); }
        if (tok.kind == tk::id)  { next(); return ref(tok.text); }

        if (is_pun("("))
        {
            next();

            node_ptr inner = expr();

            expect_pun(")");

            return inner;
        }

        throw std::runtime_error("unexpected token where an expression "
                                 "was expected");
    }
};

// report
//   function: records a failure through the optional out-parameters.
void
report(
    bool*              _ok,
    std::string*       _error,
    const std::string& _why
)
{
    if (_ok)
    {
        *_ok = false;
    }

    if (_error)
    {
        *_error = _why;
    }

    return;
}

}  // namespace


/*
parse
  Reads EBNF grammar text into rules.

Parameter(s):
  _source: the grammar text.
  _ok:     receives whether it parsed; optional.
  _error:  receives the reason it did not; optional.
Return:
  The rules in declaration order and the start symbol (the first rule), or an
empty pair on failure. Never throws.
*/
std::pair<rules, std::string>
parse(
    const std::string& _source,
    bool*              _ok,
    std::string*       _error
)
{
    if (_ok)
    {
        *_ok = true;
    }

    // recursive descent reports by throwing; nothing escapes this boundary
    try
    {
        parser p(tokenize(_source));

        return p.grammar();
    }
    catch (const std::exception& e)
    {
        report(_ok, _error, e.what());
    }

    return {};
}


/*
compile
  Compiles rules to a whole-input PEG program: CALL start, then `!.`, then
MATCH, followed by each rule's body and a RETURN -- the same shape
gen::compile produces, so the result runs under the same driver.

Parameter(s):
  _rules:  the rules, as parse() returns them.
  _start:  the rule to begin at.
  _ok:     receives whether it compiled; optional.
  _error:  receives the reason it did not; optional.
Return:
  The program, or an empty one on failure. Never throws.
*/
peg::program
compile(
    const rules&       _rules,
    const std::string& _start,
    bool*              _ok,
    std::string*       _error
)
{
    if (_ok)
    {
        *_ok = true;
    }

    try
    {
        labels                     lab;
        std::map<std::string, int> rule;

        // a repeated name would make every reference to it ambiguous
        for (const auto& r : _rules)
        {
            if (!rule.emplace(r.first, lab.next()).second)
            {
                throw std::runtime_error("rule defined twice: " + r.first);
            }
        }

        if (rule.find(_start) == rule.end())
        {
            throw std::runtime_error("start rule not defined: " + _start);
        }

        std::vector<item> items;

        items.push_back(item{ peg::CALL, false, rule.at(_start), 0, {} });
        emit(negate(any()), items, lab, rule);
        items.push_back(item{ peg::MATCH, false, -1, 0, {} });

        for (const auto& r : _rules)
        {
            items.push_back(item{ peg::MATCH, true, rule.at(r.first), 0, {} });
            emit(r.second, items, lab, rule);
            items.push_back(item{ peg::RETURN, false, -1, 0, {} });
        }

        return assemble(items);
    }
    catch (const std::exception& e)
    {
        report(_ok, _error, e.what());
    }

    return {};
}

}  // namespace ebnf

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now
