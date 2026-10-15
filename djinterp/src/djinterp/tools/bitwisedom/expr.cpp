/*******************************************************************************
* bitwisedom [bitwisedom]                                               expr.cpp
*
* Integer expressions: tokenizer, recursive-descent parser, evaluator and
* C printer.
*   The parser is one function per precedence level, from | down to the
* unary operators, each building nodes in the expression's flat list and
* returning the index of the node it built -- so the tree is a value, copied
* with the sheet that holds it, and nothing points into it.
*
* path:      /src/djinterp/tools/bitwisedom/expr.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
#include <djinterp/tools/bitwisedom/expr.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::min
#include <cctype>     // std::isalnum, std::isalpha, std::isdigit, std::isspace
#include <utility>    // std::move


namespace bitwisedom
{

namespace
{

// token_kind
//   enum: what one token is.
enum class token_kind
{
    number,
    name,
    symbol,
    end
};

// token
//   struct: one token, with its text and where it began.
struct token
{
    token_kind  kind = token_kind::end;
    std::string text;
    u64         value = 0;
    bool        big   = false;   // wider than 64 bits
    std::size_t at    = 0;
};

// word_character
//   whether a character may continue a name or a number: a letter, a digit
// or an underscore.
bool
word_character(
    char _character
)
{
    return ( (std::isalnum(static_cast<unsigned char>(_character))) ||
             (_character == '_') );
}

// MAX_DEPTH
//   constant: how deeply expressions may nest before the parser stops; a
// typed expression never comes close, and a pathological one cannot exhaust
// the stack.
const int MAX_DEPTH = 200;

// parser
//   class: the recursive descent over one text.
class parser
{
public:
    explicit parser(
        std::string_view _text
    )
        : m_text(_text)
    {}

    expression run()
    {
        m_expression = expression{};

        if (!advance())
        {
            return fail_result();
        }

        // empty text is not an expression
        if (m_current.kind == token_kind::end)
        {
            m_error    = "the expression is empty";
            m_error_at = 0;

            return fail_result();
        }

        const int root = parse_or(0);

        if (root < 0)
        {
            return fail_result();
        }

        if (m_current.kind != token_kind::end)
        {
            m_error    = "unexpected '" + m_current.text + "'";
            m_error_at = m_current.at;

            return fail_result();
        }

        m_expression.root = root;

        return m_expression;
    }

private:
    std::string_view m_text;
    std::size_t      m_at = 0;
    token            m_current;
    expression       m_expression;
    std::string      m_error;
    std::size_t      m_error_at = 0;

    expression fail_result()
    {
        expression failed;
        failed.error    = m_error;
        failed.error_at = m_error_at;

        return failed;
    }

    bool fail(
        const std::string& _message,
        std::size_t        _at
    )
    {
        // the first error is the one reported
        if (m_error.empty())
        {
            m_error    = _message;
            m_error_at = _at;
        }

        return false;
    }

    // advance
    //   reads the next token into m_current.
    bool advance()
    {
        while ( (m_at < m_text.size()) &&
                (std::isspace(static_cast<unsigned char>(m_text[m_at]))) )
        {
            ++m_at;
        }

        m_current      = token{};
        m_current.at   = m_at;

        if (m_at >= m_text.size())
        {
            m_current.kind = token_kind::end;

            return true;
        }

        const char first = m_text[m_at];

        // a number runs over every character a C literal may contain
        if (std::isdigit(static_cast<unsigned char>(first)))
        {
            const std::size_t start = m_at;

            while ( (m_at < m_text.size()) &&
                    ( (word_character(m_text[m_at])) ||
                      (m_text[m_at] == '\'') ) )
            {
                ++m_at;
            }

            m_current.kind = token_kind::number;
            m_current.text = std::string(m_text.substr(start, m_at - start));

            // any width a sheet can be: the evaluation decides whether it
            // is too wide for the sheet
            const parsed_wide parsed = parse_wide(m_current.text);

            if (!parsed.ok)
            {
                return fail("bad number '" + m_current.text + "': " +
                                parsed.error,
                            start);
            }

            m_current.value = parsed.value.low();
            m_current.big   = (parsed.value.highest() >= INTEGER_WIDTH);

            return true;
        }

        if ( (std::isalpha(static_cast<unsigned char>(first))) ||
             (first == '_') )
        {
            const std::size_t start = m_at;

            while ( (m_at < m_text.size()) &&
                    (word_character(m_text[m_at])) )
            {
                ++m_at;
            }

            m_current.kind = token_kind::name;
            m_current.text = std::string(m_text.substr(start, m_at - start));

            return true;
        }

        // the two-character operators first
        if ( (m_at + 1 < m_text.size())    &&
             ( ( (first == '<') && (m_text[m_at + 1] == '<') ) ||
               ( (first == '>') && (m_text[m_at + 1] == '>') ) ) )
        {
            m_current.kind = token_kind::symbol;
            m_current.text = std::string(m_text.substr(m_at, 2));
            m_at          += 2;

            return true;
        }

        const std::string singles = "~-+!*/%&^|(),";

        if (singles.find(first) != std::string::npos)
        {
            m_current.kind = token_kind::symbol;
            m_current.text = std::string(1, first);
            ++m_at;

            return true;
        }

        return fail(std::string("unexpected character '") + first + "'",
                    m_at);
    }

    bool is_symbol(
        const char* _text
    ) const
    {
        return ( (m_current.kind == token_kind::symbol) &&
                 (m_current.text == _text) );
    }

    int add_node(
        expr_op          _op,
        std::vector<int> _args
    )
    {
        expr_node node;
        node.op   = _op;
        node.args = std::move(_args);
        m_expression.nodes.push_back(std::move(node));

        return static_cast<int>(m_expression.nodes.size()) - 1;
    }

    // binary_level
    //   one left-associative level: operands from _next, joined by any of
    // the level's operators.
    template<typename Next>
    int binary_level(
        int                                                 _depth,
        const std::vector<std::pair<const char*, expr_op>>& _operators,
        Next                                                _next
    )
    {
        int left = _next(_depth);

        while (left >= 0)
        {
            const std::pair<const char*, expr_op>* matched = nullptr;

            for (const std::pair<const char*, expr_op>& each : _operators)
            {
                if (is_symbol(each.first))
                {
                    matched = &each;
                }
            }

            // no operator of this level: the operand stands alone
            if (matched == nullptr)
            {
                break;
            }

            if (!advance())
            {
                return -1;
            }

            const int right = _next(_depth);

            if (right < 0)
            {
                return -1;
            }

            left = add_node(matched->second, { left, right });
        }

        return left;
    }

    int parse_or(int _depth)
    {
        return binary_level(_depth,
                            { { "|", expr_op::bit_or } },
                            [this](int _d) { return parse_xor(_d); });
    }

    int parse_xor(int _depth)
    {
        return binary_level(_depth,
                            { { "^", expr_op::bit_xor } },
                            [this](int _d) { return parse_and(_d); });
    }

    int parse_and(int _depth)
    {
        return binary_level(_depth,
                            { { "&", expr_op::bit_and } },
                            [this](int _d) { return parse_shift(_d); });
    }

    int parse_shift(int _depth)
    {
        return binary_level(_depth,
                            { { "<<", expr_op::shift_left },
                              { ">>", expr_op::shift_right } },
                            [this](int _d) { return parse_sum(_d); });
    }

    int parse_sum(int _depth)
    {
        return binary_level(_depth,
                            { { "+", expr_op::add },
                              { "-", expr_op::subtract } },
                            [this](int _d) { return parse_product(_d); });
    }

    int parse_product(int _depth)
    {
        return binary_level(_depth,
                            { { "*", expr_op::multiply },
                              { "/", expr_op::divide },
                              { "%", expr_op::modulo } },
                            [this](int _d) { return parse_unary(_d); });
    }

    int parse_unary(
        int _depth
    )
    {
        if (_depth > MAX_DEPTH)
        {
            fail("the expression nests too deeply", m_current.at);

            return -1;
        }

        expr_op op       = expr_op::literal;
        bool    is_unary = true;

        if (is_symbol("~"))
        {
            op = expr_op::complement;
        }
        else if (is_symbol("-"))
        {
            op = expr_op::negate;
        }
        else if (is_symbol("!"))
        {
            op = expr_op::logical_not;
        }
        else if (is_symbol("+"))
        {
            // unary plus changes nothing; it is read and dropped
            if (!advance())
            {
                return -1;
            }

            return parse_unary(_depth + 1);
        }
        else
        {
            is_unary = false;
        }

        if (!is_unary)
        {
            return parse_primary(_depth);
        }

        if (!advance())
        {
            return -1;
        }

        const int operand = parse_unary(_depth + 1);

        return (operand < 0) ? -1
                             : add_node(op, { operand });
    }

    int parse_primary(
        int _depth
    )
    {
        if (m_current.kind == token_kind::number)
        {
            const int node = add_node(expr_op::literal, {});
            m_expression.nodes[static_cast<std::size_t>(node)].value =
                m_current.value;
            m_expression.nodes[static_cast<std::size_t>(node)].big =
                m_current.big;
            m_expression.nodes[static_cast<std::size_t>(node)].text =
                m_current.text;

            return advance() ? node : -1;
        }

        if (m_current.kind == token_kind::name)
        {
            const std::string name = m_current.text;
            const std::size_t at   = m_current.at;

            if (!advance())
            {
                return -1;
            }

            // a name followed by '(' is a call
            if (is_symbol("("))
            {
                return parse_call(name, at, _depth);
            }

            const int node = add_node(expr_op::name, {});
            m_expression.nodes[static_cast<std::size_t>(node)].text = name;

            return node;
        }

        if (is_symbol("("))
        {
            if (!advance())
            {
                return -1;
            }

            const int inner = parse_or(_depth + 1);

            if (inner < 0)
            {
                return -1;
            }

            if (!is_symbol(")"))
            {
                fail("expected ')'", m_current.at);

                return -1;
            }

            return advance() ? inner : -1;
        }

        fail( (m_current.kind == token_kind::end)
                  ? std::string("the expression ends too soon")
                  : "unexpected '" + m_current.text + "'",
              m_current.at);

        return -1;
    }

    int parse_call(
        const std::string& _name,
        std::size_t        _at,
        int                _depth
    )
    {
        // only the three functions the language knows
        std::size_t expected = 0;

        if ( (_name == "BIT") ||
             (_name == "MASK") )
        {
            expected = 1;
        }
        else if (_name == "GENMASK")
        {
            expected = 2;
        }
        else
        {
            fail("unknown function '" + _name + "'", _at);

            return -1;
        }

        if (!advance())
        {
            return -1;
        }

        std::vector<int> args;

        while (!is_symbol(")"))
        {
            const int argument = parse_or(_depth + 1);

            if (argument < 0)
            {
                return -1;
            }

            args.push_back(argument);

            if (is_symbol(","))
            {
                if (!advance())
                {
                    return -1;
                }
            }
            else if (!is_symbol(")"))
            {
                fail("expected ',' or ')'", m_current.at);

                return -1;
            }
        }

        if (args.size() != expected)
        {
            fail(_name + " takes " + format_dec(expected) + " argument" +
                     ((expected == 1) ? "" : "s"),
                 _at);

            return -1;
        }

        const int node = add_node(expr_op::call, std::move(args));
        m_expression.nodes[static_cast<std::size_t>(node)].text = _name;

        return advance() ? node : -1;
    }
};

// shift
//   a left or right shift that gives zero, and says so, past 63.
u64
shift(
    u64   _value,
    u64   _count,
    bool  _left,
    bool& _overshift
)
{
    if (_count >= 64)
    {
        _overshift = true;

        return 0;
    }

    const int count = static_cast<int>(_count);

    return _left ? (_value << count)
                 : (_value >> count);
}

// evaluate_node
//   evaluates one node and its operands.
bool
evaluate_node(
    const expression&  _expression,
    int                _index,
    const name_lookup& _lookup,
    evaluation&        _result,
    u64&               _value
)
{
    const expr_node& node = _expression.nodes[static_cast<std::size_t>(_index)];
    u64              a    = 0;
    u64              b    = 0;

    // operands first; a failure anywhere fails the whole
    if ( (!node.args.empty()) &&
         (!evaluate_node(_expression, node.args[0], _lookup, _result, a)) )
    {
        return false;
    }

    if ( (node.args.size() > 1) &&
         (!evaluate_node(_expression, node.args[1], _lookup, _result, b)) )
    {
        return false;
    }

    switch (node.op)
    {
        case expr_op::literal:
            if (node.big)
            {
                _result.error = node.text + " does not fit in 64 bits";

                return false;
            }

            _value = node.value;
            return true;

        case expr_op::name:
        {
            std::string error;

            if (!_lookup(node.text, _value, error))
            {
                _result.error = error.empty()
                                    ? "unknown name '" + node.text + "'"
                                    : error;

                return false;
            }

            return true;
        }

        case expr_op::call:
            if (node.text == "BIT")
            {
                _value = shift(1u, a, true, _result.overshift);
            }
            else if (node.text == "MASK")
            {
                _value = (a >= 64) ? ~u64(0)
                                   : ((u64(1) << a) - 1u);
            }
            else
            {
                // GENMASK(high, low): bits low through high
                if ( (a >= 64) ||
                     (b > a) )
                {
                    _result.error = "GENMASK wants high >= low, both below 64";

                    return false;
                }

                _value = range_mask(static_cast<int>(b),
                                    static_cast<int>(a - b + 1));
            }

            return true;

        case expr_op::negate:       _value = ~a + 1u;               return true;
        case expr_op::complement:   _value = ~a;                    return true;
        case expr_op::logical_not:  _value = (a == 0) ? 1u : 0u;    return true;
        case expr_op::multiply:     _value = a * b;                 return true;
        case expr_op::add:          _value = a + b;                 return true;
        case expr_op::subtract:     _value = a - b;                 return true;
        case expr_op::bit_and:      _value = a & b;                 return true;
        case expr_op::bit_xor:      _value = a ^ b;                 return true;
        case expr_op::bit_or:       _value = a | b;                 return true;

        case expr_op::shift_left:
            _value = shift(a, b, true, _result.overshift);
            return true;

        case expr_op::shift_right:
            _value = shift(a, b, false, _result.overshift);
            return true;

        case expr_op::divide:
        case expr_op::modulo:
            if (b == 0)
            {
                _result.error = "division by zero";

                return false;
            }

            _value = (node.op == expr_op::divide) ? (a / b)
                                                  : (a % b);
            return true;
    }

    return false;
}

/*
evaluate_wide_node
  One node over sets of bits. A count, an index or a divisor must be an
integer -- 64 bits or fewer -- and so must one side of a product.
*/
bool
evaluate_wide_node(
    const expression&  _expression,
    int                _index,
    const wide_lookup& _lookup,
    int                _width,
    wide_evaluation&   _result,
    wide&              _value
)
{
    const expr_node& node = _expression.nodes[static_cast<std::size_t>(_index)];
    wide             a;
    wide             b;

    if ( (!node.args.empty()) &&
         (!evaluate_wide_node(_expression, node.args[0], _lookup, _width,
                              _result, a)) )
    {
        return false;
    }

    if ( (node.args.size() > 1) &&
         (!evaluate_wide_node(_expression, node.args[1], _lookup, _width,
                              _result, b)) )
    {
        return false;
    }

    // integer
    //   an operand that must be an integer: a count, an index, a divisor.
    auto integer = [&_result](const wide&       _operand,
                              const char*       _what,
                              u64&              _out)
    {
        if (_operand.highest() >= INTEGER_WIDTH)
        {
            _result.error = std::string(_what) + " must be an integer, not a "
                            "set of bits past bit 63";

            return false;
        }

        _out = _operand.low();

        return true;
    };

    const u64 width = static_cast<u64>(_width);
    u64       n     = 0;
    u64       m     = 0;

    switch (node.op)
    {
        case expr_op::literal:
            _value = parse_wide(node.text).value;
            return true;

        case expr_op::name:
        {
            std::string error;

            if (!_lookup(node.text, _value, error))
            {
                _result.error = error.empty()
                                    ? "unknown name '" + node.text + "'"
                                    : error;

                return false;
            }

            return true;
        }

        case expr_op::call:
            if (!integer(a, "an argument", n))
            {
                return false;
            }

            if (node.text == "BIT")
            {
                if (n >= width)
                {
                    _result.error = "BIT(" + format_dec(n) + ") is past the " +
                                    format_dec(width) + "-bit width";

                    return false;
                }

                _value = wide::bit(static_cast<int>(n));
            }
            else if (node.text == "MASK")
            {
                _value = wide::run(0, static_cast<int>(std::min(n, width)));
            }
            else
            {
                if ( (!integer(b, "an argument", m)) ||
                     (n >= width) ||
                     (m > n) )
                {
                    if (_result.error.empty())
                    {
                        _result.error = "GENMASK wants high >= low, both "
                                        "below " + format_dec(width);
                    }

                    return false;
                }

                _value = wide::run(static_cast<int>(m),
                                   static_cast<int>(n - m + 1));
            }

            return true;

        case expr_op::negate:
            _value = negate(a, _width);
            return true;

        case expr_op::complement:
            _value = complement(a, _width);
            return true;

        case expr_op::logical_not:
            _value = a.any() ? wide() : wide(1u);
            return true;

        case expr_op::add:
            _value = add(a, b, _width);
            return true;

        case expr_op::subtract:
            _value = subtract(a, b, _width);
            return true;

        case expr_op::bit_and: _value = a & b; return true;
        case expr_op::bit_xor: _value = a ^ b; return true;
        case expr_op::bit_or:  _value = a | b; return true;

        case expr_op::multiply:
            // one side an integer: the other multiplied by it
            if (b.highest() < INTEGER_WIDTH)
            {
                _value = multiply(a, b.low(), _width);
            }
            else if (a.highest() < INTEGER_WIDTH)
            {
                _value = multiply(b, a.low(), _width);
            }
            else
            {
                _result.error = "a product needs one side that is an "
                                "integer";

                return false;
            }

            return true;

        case expr_op::shift_left:
        case expr_op::shift_right:
            if (!integer(b, "a shift's count", n))
            {
                return false;
            }

            if (n >= width)
            {
                _result.overshift = true;
                _value            = wide();

                return true;
            }

            _value = (node.op == expr_op::shift_left)
                         ? shift_left(a, static_cast<int>(n), _width)
                         : shift_right(a, static_cast<int>(n));
            return true;

        case expr_op::divide:
        case expr_op::modulo:
            if ( (!integer(a, "a dividend", n)) ||
                 (!integer(b, "a divisor", m)) )
            {
                return false;
            }

            if (m == 0)
            {
                _result.error = "division by zero";

                return false;
            }

            _value = wide((node.op == expr_op::divide) ? (n / m)
                                                       : (n % m));
            return true;
    }

    return false;
}

// binary_text
//   the C spelling of a binary operator.
const char*
binary_text(
    expr_op _op
)
{
    switch (_op)
    {
        case expr_op::multiply:    return "*";
        case expr_op::divide:      return "/";
        case expr_op::modulo:      return "%";
        case expr_op::add:         return "+";
        case expr_op::subtract:    return "-";
        case expr_op::shift_left:  return "<<";
        case expr_op::shift_right: return ">>";
        case expr_op::bit_and:     return "&";
        case expr_op::bit_xor:     return "^";
        case expr_op::bit_or:      return "|";
        default:                   return "?";
    }
}

// associative
//   whether a chain of one operator may drop its inner parentheses on both
// sides; subtraction and division only on the left, as C groups them.
bool
associative(
    expr_op _op
)
{
    return ( (_op == expr_op::bit_or)   ||
             (_op == expr_op::bit_and)  ||
             (_op == expr_op::bit_xor)  ||
             (_op == expr_op::add)      ||
             (_op == expr_op::multiply) );
}

// chain
//   the operands of a run of one operator, left to right, each printed by
// _print; operands of another operator are printed whole.
template<typename Print>
void
chain(
    const expression&         _expression,
    int                       _index,
    expr_op                   _op,
    const Print&              _print,
    std::vector<std::string>& _parts
)
{
    const expr_node& node = _expression.nodes[static_cast<std::size_t>(_index)];

    for (std::size_t i = 0; i < node.args.size(); ++i)
    {
        const int        operand = node.args[i];
        const expr_node& inner   =
            _expression.nodes[static_cast<std::size_t>(operand)];
        const bool       flatten = ( (inner.op == _op) &&
                                     ( (i == 0) ||
                                       (associative(_op)) ) );

        if (flatten)
        {
            chain(_expression, operand, _op, _print, _parts);
        }
        else
        {
            _parts.push_back(_print(operand));
        }
    }

    return;
}

// print_node
//   prints one node; _count marks a shift's right operand, which keeps its
// spelling and takes no suffix.
std::string
print_node(
    const expression&      _expression,
    int                    _index,
    const name_printer&    _names,
    const literal_printer& _literals,
    const name_lookup&     _lookup,
    bool                   _count
)
{
    const expr_node& node = _expression.nodes[static_cast<std::size_t>(_index)];

    // print_operand
    //   prints one operand of this node.
    auto operand = [&](std::size_t _which,
                       bool        _is_count)
    {
        return print_node(_expression,
                          node.args[_which],
                          _names,
                          _literals,
                          _lookup,
                          _is_count);
    };

    switch (node.op)
    {
        case expr_op::literal:
            return _count ? format_dec(node.value)
                          : _literals(node.value);

        case expr_op::name:
            return _names(node.text);

        case expr_op::call:
        {
            evaluation scratch;
            u64        value = 0;

            // a call folds to the literal it makes; a failure keeps the call
            if (evaluate_node(_expression, _index, _lookup, scratch, value))
            {
                return _literals(value);
            }

            std::string text = node.text + "(";

            for (std::size_t i = 0; i < node.args.size(); ++i)
            {
                text += ((i == 0) ? "" : ", ") + operand(i, true);
            }

            return text + ")";
        }

        case expr_op::negate:
            return "(-" + operand(0, false) + ")";

        case expr_op::complement:
            return "(~" + operand(0, false) + ")";

        case expr_op::logical_not:
            return "(!" + operand(0, false) + ")";

        default:
            break;
    }

    const bool shifting = ( (node.op == expr_op::shift_left) ||
                            (node.op == expr_op::shift_right) );

    // a chain of one operator reads as one parenthesized list
    if (!shifting)
    {
        std::vector<std::string> parts;

        chain(_expression,
              _index,
              node.op,
              [&](int _operand)
              {
                  return print_node(_expression,
                                    _operand,
                                    _names,
                                    _literals,
                                    _lookup,
                                    false);
              },
              parts);

        std::string text = "(";

        for (std::size_t i = 0; i < parts.size(); ++i)
        {
            text += (i == 0) ? "" : std::string(" ") +
                                        binary_text(node.op) + " ";
            text += parts[i];
        }

        return text + ")";
    }

    return "(" + operand(0, false) + " " + binary_text(node.op) + " " +
           operand(1, shifting) + ")";
}

// collect_names
//   gathers names below one node, in order.
void
collect_names(
    const expression&         _expression,
    int                       _index,
    std::vector<std::string>& _names
)
{
    const expr_node& node = _expression.nodes[static_cast<std::size_t>(_index)];

    if (node.op == expr_op::name)
    {
        _names.push_back(node.text);
    }

    for (int argument : node.args)
    {
        collect_names(_expression, argument, _names);
    }

    return;
}

}  // anonymous namespace


expression
parse_expression(
    std::string_view _text
)
{
    // The editor accepts an actual C preprocessor continuation (a
    // backslash followed by CRLF or LF), in addition to ordinary spaces
    // and newlines. It is the logical expression that is parsed.
    std::string logical;
    logical.reserve(_text.size());
    for (std::size_t i = 0; i < _text.size(); ++i)
    {
        if (_text[i] == '\\')
        {
            std::size_t next = i + 1;
            if ((next < _text.size()) && (_text[next] == '\r'))
            {
                ++next;
            }
            if ((next < _text.size()) && (_text[next] == '\n'))
            {
                logical.push_back(' ');
                i = next;
                continue;
            }
        }
        logical.push_back(_text[i]);
    }
    parser reader(logical);
    return reader.run();
}

std::vector<std::string>
referenced_names(
    const expression& _expression
)
{
    std::vector<std::string> names;

    if (_expression.ok())
    {
        collect_names(_expression, _expression.root, names);
    }

    return names;
}

evaluation
evaluate(
    const expression&  _expression,
    const name_lookup& _lookup
)
{
    evaluation result;

    if (!_expression.ok())
    {
        result.error = _expression.error;

        return result;
    }

    u64 value = 0;

    result.ok    = evaluate_node(_expression,
                                 _expression.root,
                                 _lookup,
                                 result,
                                 value);
    result.value = value;

    return result;
}

wide_evaluation
evaluate_wide(
    const expression&  _expression,
    const wide_lookup& _lookup,
    int                _width
)
{
    wide_evaluation result;

    if (!_expression.ok())
    {
        result.error = _expression.error;

        return result;
    }

    wide value;

    result.ok = evaluate_wide_node(_expression,
                                   _expression.root,
                                   _lookup,
                                   _width,
                                   result,
                                   value);
    value.trim(_width);
    result.value = value;

    return result;
}

std::string
print_c(
    const expression&      _expression,
    const name_printer&    _names,
    const literal_printer& _literals,
    const name_lookup&     _lookup
)
{
    if (!_expression.ok())
    {
        return std::string();
    }

    return print_node(_expression,
                      _expression.root,
                      _names,
                      _literals,
                      _lookup,
                      false);
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
