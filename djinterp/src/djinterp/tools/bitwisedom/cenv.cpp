/*******************************************************************************
* bitwisedom [bitwisedom]                                               cenv.cpp
*
* The define environment: the standard headers' macros, the expander, the
* evaluator, the directive walker and the suggestions.
*   The expander keeps its work as a stack, the next token on top, so that
* a replacement goes back on top to be rescanned together with the tokens
* after it, as the standard says; arguments are expanded on their own, as
* though they were the rest of the file. Hide sets are sorted vectors of
* macro ids, and invocations are records the tokens point into.
*   The evaluator is a recursive descent over the expanded tokens, one
* function per precedence level. It never stops at a problem it can carry:
* undefined behaviour marks the operand and is reported once, where it
* arose; only an error -- a name nothing defines, a string, a syntax
* error -- ends the evaluation.
*
* path:      /src/djinterp/tools/bitwisedom/cenv.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.09
*                                                            revised: 2026.10.10
*******************************************************************************/
#include <djinterp/tools/bitwisedom/cenv.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::sort, std::find, std::min, std::max
#include <cctype>     // std::isupper, std::isdigit, std::isspace
#include <iterator>   // std::back_inserter
#include <set>        // std::set
#include <utility>    // std::move


namespace bitwisedom
{

namespace
{

//==============================================================================
// 1.  TYPES
//==============================================================================

// RANK_*
//   constants: the standard integer types' ranks.
const int RANK_BOOL  = 0;
const int RANK_CHAR  = 1;
const int RANK_SHORT = 2;
const int RANK_INT   = 3;
const int RANK_LONG  = 4;
const int RANK_LLONG = 5;

// STEP_LIMIT / DEPTH_LIMIT
//   constants: how many tokens an expansion may handle, and how deeply an
// expression may nest, before the text is refused as too large.
const long STEP_LIMIT  = 2000000;
const int  DEPTH_LIMIT = 400;

int
long_bits(
    data_model _model
)
{
    return (_model == data_model::lp64) ? 64 : 32;
}

int
rank_bits(
    int        _rank,
    data_model _model
)
{
    switch (_rank)
    {
        case RANK_BOOL:  return 8;
        case RANK_CHAR:  return 8;
        case RANK_SHORT: return 16;
        case RANK_INT:   return 32;
        case RANK_LONG:  return long_bits(_model);
        default:         break;
    }

    return 64;
}

std::string
rank_name(
    int      _rank,
    bool     _signed,
    standard _level
)
{
    switch (_rank)
    {
        case RANK_BOOL:
            return ( (cpp_level(_level)) ||
                     (_level == standard::c23) ) ? "bool" : "_Bool";

        case RANK_CHAR:
            return _signed ? "signed char" : "unsigned char";

        case RANK_SHORT:
            return _signed ? "short" : "unsigned short";

        case RANK_INT:
            return _signed ? "int" : "unsigned int";

        case RANK_LONG:
            return _signed ? "long" : "unsigned long";

        default:
            break;
    }

    return _signed ? "long long" : "unsigned long long";
}

c_type
make_type(
    int                _rank,
    bool               _signed,
    const c_settings&  _settings,
    const std::string& _name = std::string()
)
{
    c_type made;
    made.rank      = _rank;
    made.is_signed = ( (_rank != RANK_BOOL) &&
                       (_signed) );
    made.bits      = rank_bits(_rank, _settings.model);
    made.name      = _name.empty() ? rank_name(_rank,
                                               made.is_signed,
                                               _settings.level)
                                   : _name;

    return made;
}

// mask_of
//   the low _bits bits.
u64
mask_of(
    int _bits
)
{
    return (_bits >= 64) ? ~u64(0)
                         : ((u64(1) << _bits) - 1u);
}

u64
type_max(
    const c_type& _type
)
{
    if (_type.rank == RANK_BOOL)
    {
        return 1;
    }

    return _type.is_signed ? (mask_of(_type.bits) >> 1)
                           : mask_of(_type.bits);
}

i64
type_min(
    const c_type& _type
)
{
    if (!_type.is_signed)
    {
        return 0;
    }

    // -(2^(bits-1)), without overflowing on the way
    return -static_cast<i64>(mask_of(_type.bits) >> 1) - 1;
}

// make_value
//   a value of a type from its bits: cut to the width, and sign-extended
// when the type is signed; a bool is 0 or 1.
c_value
make_value(
    const c_type& _type,
    u64           _raw
)
{
    c_value made;
    made.type = _type;

    if (_type.rank == RANK_BOOL)
    {
        made.bits = (_raw != 0) ? 1u : 0u;

        return made;
    }

    const u64 mask = mask_of(_type.bits);
    u64       bits = _raw & mask;

    if ( (_type.is_signed) &&
         (_type.bits < 64) &&
         (((bits >> (_type.bits - 1)) & 1u) != 0) )
    {
        bits |= ~mask;
    }

    made.bits = bits;

    return made;
}

// value_text
//   a value as a number: signed or unsigned as its type is.
std::string
value_text(
    const c_value& _value
)
{
    return _value.type.is_signed ? format_signed(_value.as_signed())
                                 : format_dec(_value.bits);
}

// fits
//   whether a value is one a type can hold.
bool
fits(
    const c_value& _value,
    const c_type&  _type
)
{
    if (_value.type.is_signed)
    {
        const i64 number = _value.as_signed();

        if (number < 0)
        {
            return ( (_type.is_signed) &&
                     (number >= type_min(_type)) );
        }

        return (static_cast<u64>(number) <= type_max(_type));
    }

    return (_value.bits <= type_max(_type));
}

// typedef_type
//   the type one of the standard typedef names stands for, by the data
// model: <stdint.h>'s, <stddef.h>'s, and the character types C makes
// typedefs of and C++ keywords.
bool
typedef_type(
    const std::string& _name,
    const c_settings&  _settings,
    c_type&            _out
)
{
    struct entry
    {
        const char* name;
        int         rank;
        bool        is_signed;
    };

    const bool lp64  = (_settings.model == data_model::lp64);
    const bool llp64 = (_settings.model == data_model::llp64);
    const bool ilp32 = (_settings.model == data_model::ilp32);
    const int  wide  = lp64 ? RANK_LONG : RANK_LLONG;
    const int  ptr   = lp64 ? RANK_LONG : (ilp32 ? RANK_INT : RANK_LLONG);
    const int  fast  = lp64 ? RANK_LONG : RANK_INT;

    const entry table[] = {
        { "int8_t",         RANK_CHAR,  true  },
        { "uint8_t",        RANK_CHAR,  false },
        { "int16_t",        RANK_SHORT, true  },
        { "uint16_t",       RANK_SHORT, false },
        { "int32_t",        RANK_INT,   true  },
        { "uint32_t",       RANK_INT,   false },
        { "int64_t",        wide,       true  },
        { "uint64_t",       wide,       false },
        { "int_least8_t",   RANK_CHAR,  true  },
        { "uint_least8_t",  RANK_CHAR,  false },
        { "int_least16_t",  RANK_SHORT, true  },
        { "uint_least16_t", RANK_SHORT, false },
        { "int_least32_t",  RANK_INT,   true  },
        { "uint_least32_t", RANK_INT,   false },
        { "int_least64_t",  wide,       true  },
        { "uint_least64_t", wide,       false },
        { "int_fast8_t",    RANK_CHAR,  true  },
        { "uint_fast8_t",   RANK_CHAR,  false },
        { "int_fast16_t",   fast,       true  },
        { "uint_fast16_t",  fast,       false },
        { "int_fast32_t",   fast,       true  },
        { "uint_fast32_t",  fast,       false },
        { "int_fast64_t",   wide,       true  },
        { "uint_fast64_t",  wide,       false },
        { "intmax_t",       wide,       true  },
        { "uintmax_t",      wide,       false },
        { "intptr_t",       ptr,        true  },
        { "uintptr_t",      ptr,        false },
        { "ptrdiff_t",      ptr,        true  },
        { "size_t",         ptr,        false },
        { "ssize_t",        ptr,        true  },
        { "wchar_t",        llp64 ? RANK_SHORT : RANK_INT, !llp64 },
        { "char8_t",        RANK_CHAR,  false },
        { "char16_t",       RANK_SHORT, false },
        { "char32_t",       RANK_INT,   false },
        { "_Bool",          RANK_BOOL,  false },
        { "bool",           RANK_BOOL,  false } };

    for (const entry& each : table)
    {
        if (_name == each.name)
        {
            _out = make_type(each.rank, each.is_signed, _settings, _name);

            return true;
        }
    }

    return false;
}

// usual
//   the usual arithmetic conversions' common type, of operands already
// promoted. A result the same as an operand keeps that operand's name.
c_type
usual(
    const c_type&     _left,
    const c_type&     _right,
    const c_settings& _settings
)
{
    if (_left.same(_right))
    {
        return _left;
    }

    if (_left.is_signed == _right.is_signed)
    {
        return (_left.rank >= _right.rank) ? _left : _right;
    }

    const c_type& unsigned_one = _left.is_signed ? _right : _left;
    const c_type& signed_one   = _left.is_signed ? _left : _right;

    if (unsigned_one.rank >= signed_one.rank)
    {
        return unsigned_one;
    }

    // the signed type holds every value of the unsigned one
    if (signed_one.bits > unsigned_one.bits)
    {
        return signed_one;
    }

    return make_type(signed_one.rank, false, _settings);
}

// promoted
//   the integer promotions: anything of lower rank than int becomes int,
// which holds every value of every narrower type here.
c_type
promoted(
    const c_type&     _type,
    const c_settings& _settings
)
{
    if (_type.rank < RANK_INT)
    {
        return make_type(RANK_INT, true, _settings);
    }

    return _type;
}

/*
literal_type
  The type C gives an integer literal: the first of its list that holds its
value. The list depends on the suffix and on whether it is written in
decimal, which only ever looks for signed types.
*/
bool
literal_type(
    const c_literal&  _literal,
    const c_settings& _settings,
    c_type&           _out,
    std::string&      _problem
)
{
    std::vector<std::pair<int, bool>> list;
    const bool decimal = (_literal.base == 10);

    if (_literal.bit_precise)
    {
        _problem = "a _BitInt literal (wb) is not evaluated here";

        return false;
    }

    if (_literal.size)
    {
        const int ptr = (_settings.model == data_model::lp64)  ? RANK_LONG
                      : (_settings.model == data_model::ilp32) ? RANK_INT
                                                               : RANK_LLONG;

        list.emplace_back(ptr, !_literal.is_unsigned);
    }
    else if (_literal.is_unsigned)
    {
        for (int rank = RANK_INT + _literal.longs; rank <= RANK_LLONG; ++rank)
        {
            list.emplace_back(rank, false);
        }
    }
    else
    {
        for (int rank = RANK_INT + _literal.longs; rank <= RANK_LLONG; ++rank)
        {
            list.emplace_back(rank, true);

            if (!decimal)
            {
                list.emplace_back(rank, false);
            }
        }
    }

    for (const std::pair<int, bool>& each : list)
    {
        const c_type candidate = make_type(each.first, each.second, _settings);

        if (_literal.value <= type_max(candidate))
        {
            _out = candidate;

            return true;
        }
    }

    // a decimal too large for every signed type: the compilers make it
    // unsigned, and say so
    _out     = make_type(RANK_LLONG, false, _settings);
    _problem = "is too large for every signed type; it is taken as "
               "unsigned long long";

    return true;
}

// is_reserved_name
//   a name the implementation keeps: an underscore and a capital, or two
// underscores at the start.
bool
is_reserved_name(
    const std::string& _name
)
{
    return ( (_name.size() >= 2) &&
             (_name[0] == '_')   &&
             ( (_name[1] == '_') ||
               (std::isupper(static_cast<unsigned char>(_name[1]))) ) );
}

// looks_like_macro
//   a name written as macros are: capitals, digits and underscores, with
// at least two letters.
bool
looks_like_macro(
    const std::string& _name
)
{
    int letters = 0;

    for (char each : _name)
    {
        const unsigned char c = static_cast<unsigned char>(each);

        if (std::isupper(c))
        {
            ++letters;
        }
        else if ( (!std::isdigit(c)) &&
                  (each != '_') )
        {
            return false;
        }
    }

    return (letters >= 2);
}


//==============================================================================
// 2.  THE STANDARD HEADERS
//==============================================================================

/*
standard_text
  What the standard headers and the compiler define, as #define lines: the
data model decides the 64-bit types' suffixes and the pointer-sized types,
and the language and level decide <stdbool.h> and the version macros.
*/
std::string
standard_text(
    const c_settings& _settings
)
{
    const bool lp64   = (_settings.model == data_model::lp64);
    const bool llp64  = (_settings.model == data_model::llp64);
    const bool ilp32  = (_settings.model == data_model::ilp32);
    const bool cpp    = cpp_level(_settings.level);
    const char* l64   = lp64 ? "L" : "LL";
    const std::string s64 = l64;
    const std::string ptr_max = lp64  ? "9223372036854775807L"
                              : ilp32 ? "2147483647"
                                      : "9223372036854775807LL";
    const std::string ptr_umax = lp64  ? "0xFFFFFFFFFFFFFFFFUL"
                               : ilp32 ? "0xFFFFFFFFU"
                                       : "0xFFFFFFFFFFFFFFFFULL";
    std::string out;

    // define
    //   one line of the text
    auto define = [&out](const std::string& _line)
    {
        out += "#define " + _line + "\n";
    };

    // <stdint.h>
    define("INT8_MIN (-127 - 1)");
    define("INT8_MAX 127");
    define("UINT8_MAX 255");
    define("INT16_MIN (-32767 - 1)");
    define("INT16_MAX 32767");
    define("UINT16_MAX 65535");
    define("INT32_MIN (-2147483647 - 1)");
    define("INT32_MAX 2147483647");
    define("UINT32_MAX 0xFFFFFFFFU");
    define("INT64_MIN (-9223372036854775807" + s64 + " - 1)");
    define("INT64_MAX 9223372036854775807" + s64);
    define("UINT64_MAX 0xFFFFFFFFFFFFFFFFU" + s64);

    for (const char* bits : { "8", "16", "32", "64" })
    {
        const std::string n = bits;

        define("INT_LEAST" + n + "_MIN INT" + n + "_MIN");
        define("INT_LEAST" + n + "_MAX INT" + n + "_MAX");
        define("UINT_LEAST" + n + "_MAX UINT" + n + "_MAX");
    }

    define("INT8_C(c) c");
    define("INT16_C(c) c");
    define("INT32_C(c) c");
    define("INT64_C(c) c ## " + s64);
    define("UINT8_C(c) c");
    define("UINT16_C(c) c");
    define("UINT32_C(c) c ## U");
    define("UINT64_C(c) c ## U" + s64);
    define("INTMAX_C(c) c ## " + s64);
    define("UINTMAX_C(c) c ## U" + s64);
    define("INTMAX_MIN INT64_MIN");
    define("INTMAX_MAX INT64_MAX");
    define("UINTMAX_MAX UINT64_MAX");
    define("INTPTR_MIN (-" + ptr_max + " - 1)");
    define("INTPTR_MAX " + ptr_max);
    define("UINTPTR_MAX " + ptr_umax);
    define("PTRDIFF_MIN INTPTR_MIN");
    define("PTRDIFF_MAX INTPTR_MAX");
    define("SIZE_MAX UINTPTR_MAX");
    define(llp64 ? "WCHAR_MIN 0x0000" : "WCHAR_MIN (-2147483647 - 1)");
    define(llp64 ? "WCHAR_MAX 0xFFFF" : "WCHAR_MAX 2147483647");

    // <limits.h>
    define("CHAR_BIT 8");
    define("SCHAR_MIN (-128)");
    define("SCHAR_MAX 127");
    define("UCHAR_MAX 255");
    define("CHAR_MIN SCHAR_MIN");
    define("CHAR_MAX SCHAR_MAX");
    define("SHRT_MIN (-32768)");
    define("SHRT_MAX 32767");
    define("USHRT_MAX 65535");
    define("INT_MIN (-2147483647 - 1)");
    define("INT_MAX 2147483647");
    define("UINT_MAX 0xFFFFFFFFU");
    define(lp64 ? "LONG_MAX 9223372036854775807L" : "LONG_MAX 2147483647L");
    define("LONG_MIN (-LONG_MAX - 1L)");
    define(lp64 ? "ULONG_MAX 0xFFFFFFFFFFFFFFFFUL" : "ULONG_MAX 0xFFFFFFFFUL");
    define("LLONG_MAX 9223372036854775807LL");
    define("LLONG_MIN (-LLONG_MAX - 1LL)");
    define("ULLONG_MAX 0xFFFFFFFFFFFFFFFFULL");

    // <stddef.h>, <stdbool.h> and the version, by language
    if (cpp)
    {
        const char* version =
            (_settings.level == standard::cpp98) ? "199711L"
          : (_settings.level == standard::cpp11) ? "201103L"
          : (_settings.level == standard::cpp14) ? "201402L"
          : (_settings.level == standard::cpp17) ? "201703L"
          : (_settings.level == standard::cpp20) ? "202002L"
                                                 : "202302L";

        define("NULL 0");
        define(std::string("__cplusplus ") + version);
    }
    else
    {
        const char* version =
            (_settings.level == standard::c99) ? "199901L"
          : (_settings.level == standard::c11) ? "201112L"
          : (_settings.level == standard::c17) ? "201710L"
                                               : "202311L";

        define("NULL ((void*)0)");
        define("__STDC__ 1");
        define(std::string("__STDC_VERSION__ ") + version);

        if (_settings.level != standard::c23)
        {
            define("bool _Bool");
            define("true 1");
            define("false 0");
            define("__bool_true_false_are_defined 1");
        }
    }

    define("__STDC_HOSTED__ 1");

    // the data model's own names
    if (lp64)
    {
        define("__LP64__ 1");
        define("_LP64 1");
    }
    else if (llp64)
    {
        define("_WIN32 1");
        define("_WIN64 1");
    }
    else
    {
        define("__ILP32__ 1");
        define("_ILP32 1");
    }

    return out;
}


//==============================================================================
// 3.  EXPANSION
//==============================================================================

// hideset
//   type: the macros a token may no longer be replaced by, as sorted ids.
using hideset = std::vector<int>;

hideset
hs_union(
    const hideset& _a,
    const hideset& _b
)
{
    hideset out;

    std::set_union(_a.begin(), _a.end(),
                   _b.begin(), _b.end(),
                   std::back_inserter(out));

    return out;
}

hideset
hs_intersect(
    const hideset& _a,
    const hideset& _b
)
{
    hideset out;

    std::set_intersection(_a.begin(), _a.end(),
                          _b.begin(), _b.end(),
                          std::back_inserter(out));

    return out;
}

bool
hs_has(
    const hideset& _set,
    int            _id
)
{
    return std::binary_search(_set.begin(), _set.end(), _id);
}

// xtok
//   struct: a token in expansion: the token, its hide set, and the
// invocation it came out of (-1: the text being evaluated).
struct xtok
{
    c_token tok;
    hideset hide;
    int     origin = -1;
};

// invocation
//   struct: one macro replaced, as it was written. `parent` is the
// invocation its name came out of; `line` and `column` where it stands in
// the evaluated text, or where its outermost ancestor does.
struct invocation
{
    std::string name;
    std::string text;
    int         parent = -1;
    int         line   = 0;
    int         column = 0;
};

// trimmed
//   tokens spelled, without the space the first one may lead with.
std::string
trimmed(
    const std::vector<xtok>& _tokens
)
{
    std::string out;

    for (const xtok& each : _tokens)
    {
        if (each.tok.kind == c_class::placemarker)
        {
            continue;
        }

        if ( (!out.empty()) &&
             (each.tok.has(C_LEADING_SPACE)) )
        {
            out += ' ';
        }

        out += each.tok.text;
    }

    return out;
}

// set_leading
//   gives a token, or takes from it, the space before it.
void
set_leading(
    c_token& _token,
    bool     _space
)
{
    if (_space)
    {
        _token.flags |= C_LEADING_SPACE;
    }
    else
    {
        _token.flags &= ~C_LEADING_SPACE;
    }

    return;
}

// expander
//   class: one expansion: the records of its invocations, and its
// diagnostics.
class expander
{
public:
    expander(
        const c_env&               _env,
        bool                       _if_mode,
        std::vector<c_diagnostic>& _diagnostics
    )
        : m_env(_env),
          m_if_mode(_if_mode),
          m_diagnostics(_diagnostics)
    {}

    std::vector<invocation> records;
    std::vector<std::string> tested;   // names `defined` asked about

    // run
    //   a token list, expanded through: every macro replaced, and the
    // replacements rescanned with what follows them.
    std::vector<xtok> run(
        std::vector<xtok> _tokens
    )
    {
        std::vector<xtok> work(_tokens.rbegin(), _tokens.rend());
        std::vector<xtok> out;

        while (!work.empty())
        {
            if (++m_steps > STEP_LIMIT)
            {
                if (!m_overrun)
                {
                    m_overrun = true;
                    report(c_problem::expansion,
                           "the expansion is too large to follow (a macro "
                           "that grows without end?)",
                           work.back());
                }

                while (!work.empty())
                {
                    out.push_back(std::move(work.back()));
                    work.pop_back();
                }

                break;
            }

            xtok current = std::move(work.back());
            work.pop_back();

            if (current.tok.kind != c_class::identifier)
            {
                out.push_back(std::move(current));

                continue;
            }

            if ( (m_if_mode) &&
                 (if_operator(current, work, out)) )
            {
                continue;
            }

            if (current.tok.text == "__LINE__")
            {
                current.tok.kind = c_class::number;
                current.tok.text =
                    std::to_string(std::max(1, current.tok.line));
                out.push_back(std::move(current));

                continue;
            }

            if (current.tok.text == "__COUNTER__")
            {
                current.tok.kind = c_class::number;
                current.tok.text = std::to_string(m_counter++);
                out.push_back(std::move(current));

                continue;
            }

            const c_macro* macro = m_env.find(current.tok.text);

            if (macro == nullptr)
            {
                out.push_back(std::move(current));

                continue;
            }

            const int id = id_of(macro->name);

            if (hs_has(current.hide, id))
            {
                out.push_back(std::move(current));

                continue;
            }

            if (!macro->function_like)
            {
                const int record = record_of(*macro,
                                             current,
                                             std::vector<std::vector<xtok>>());
                const hideset hide = hs_union(current.hide, hideset{ id });

                push_front(work, substitute(*macro,
                                            std::vector<std::vector<xtok>>(),
                                            hide,
                                            record,
                                            current.tok.flags));

                continue;
            }

            // a function-like macro's name is replaced only before a '('
            if ( (work.empty()) ||
                 (!work.back().tok.is("(")) )
            {
                out.push_back(std::move(current));

                continue;
            }

            std::vector<std::vector<xtok>> args;
            std::vector<xtok>              consumed;
            xtok                           close;

            if (!collect(*macro, work, args, consumed, close))
            {
                report(c_problem::arguments,
                       "the arguments to " + macro->name + " never close",
                       current);
                out.push_back(std::move(current));
                out.insert(out.end(), consumed.begin(), consumed.end());

                continue;
            }

            std::string problem;

            if (!arity(*macro, args, problem))
            {
                report(c_problem::arguments, problem, current);
                out.push_back(std::move(current));
                out.insert(out.end(), consumed.begin(), consumed.end());

                continue;
            }

            const int     record = record_of(*macro, current, args);
            const hideset hide   = hs_union(hs_intersect(current.hide,
                                                         close.hide),
                                            hideset{ id });

            push_front(work, substitute(*macro,
                                        args,
                                        hide,
                                        record,
                                        current.tok.flags));
        }

        return out;
    }

    // chain_of
    //   the invocations a token came out of, innermost first.
    std::vector<std::string> chain_of(
        int _origin
    ) const
    {
        std::vector<std::string> chain;

        for (int at = _origin;
             at >= 0;
             at = records[static_cast<std::size_t>(at)].parent)
        {
            chain.push_back(records[static_cast<std::size_t>(at)].text);
        }

        return chain;
    }

    // place_of
    //   where a token stands in the evaluated text: its own place, or its
    // outermost invocation's.
    void place_of(
        const xtok& _token,
        int&        _line,
        int&        _column
    ) const
    {
        if (_token.origin < 0)
        {
            _line   = _token.tok.line;
            _column = _token.tok.column;

            return;
        }

        const invocation& at = records[static_cast<std::size_t>(_token.origin)];
        _line   = at.line;
        _column = at.column;

        return;
    }

    void report(
        c_problem          _problem,
        const std::string& _message,
        const xtok&        _where
    )
    {
        c_diagnostic made;
        made.level   = c_level::error;
        made.problem = _problem;
        made.message = _message;
        made.chain   = chain_of(_where.origin);
        place_of(_where, made.line, made.column);
        m_diagnostics.push_back(std::move(made));

        return;
    }

private:
    const c_env&               m_env;
    bool                       m_if_mode;
    std::vector<c_diagnostic>& m_diagnostics;
    std::map<std::string, int> m_ids;
    long                       m_steps   = 0;
    bool                       m_overrun = false;
    int                        m_counter = 0;

    int id_of(
        const std::string& _name
    )
    {
        const auto found = m_ids.find(_name);

        if (found != m_ids.end())
        {
            return found->second;
        }

        const int id = static_cast<int>(m_ids.size());
        m_ids[_name] = id;

        return id;
    }

    // push_front
    //   tokens put back on top of the work, first on top.
    static void push_front(
        std::vector<xtok>&  _work,
        std::vector<xtok>&& _tokens
    )
    {
        for (std::size_t i = _tokens.size(); i-- > 0; )
        {
            _work.push_back(std::move(_tokens[i]));
        }

        return;
    }

    /*
    if_operator
      In an #if, `defined` answers from the table without expanding its
    name, and the __has_ operators, which ask about things this
    environment has not seen, answer 0.
    */
    bool if_operator(
        const xtok&        _current,
        std::vector<xtok>& _work,
        std::vector<xtok>& _out
    )
    {
        const std::string& name = _current.tok.text;

        if (name == "defined")
        {
            const bool paren = ( (!_work.empty()) &&
                                 (_work.back().tok.is("(")) );

            if (paren)
            {
                _work.pop_back();
            }

            if ( (_work.empty()) ||
                 (_work.back().tok.kind != c_class::identifier) )
            {
                report(c_problem::syntax, "'defined' needs a name", _current);

                return true;
            }

            const std::string tested_name = _work.back().tok.text;
            _work.pop_back();

            if (paren)
            {
                if ( (!_work.empty()) &&
                     (_work.back().tok.is(")")) )
                {
                    _work.pop_back();
                }
                else
                {
                    report(c_problem::syntax,
                           "'defined(" + tested_name + "' is missing its ')'",
                           _current);
                }
            }

            tested.push_back(tested_name);

            xtok answer = _current;
            answer.tok.kind = c_class::number;
            answer.tok.text = (m_env.find(tested_name) != nullptr) ? "1" : "0";
            _out.push_back(std::move(answer));

            return true;
        }

        if ( (name.compare(0, 6, "__has_") != 0) ||
             (_work.empty())                     ||
             (!_work.back().tok.is("(")) )
        {
            return false;
        }

        // __has_include(...) and its kin: the parenthesized operand, gone
        int depth = 0;

        while (!_work.empty())
        {
            const xtok next = std::move(_work.back());
            _work.pop_back();

            depth += next.tok.is("(") ? 1 : 0;
            depth -= next.tok.is(")") ? 1 : 0;

            if (depth == 0)
            {
                break;
            }
        }

        tested.push_back(name);

        xtok answer = _current;
        answer.tok.kind = c_class::number;
        answer.tok.text = "0";
        _out.push_back(std::move(answer));

        return true;
    }

    /*
    collect
      A function-like macro's arguments, from the '(' on top of the work
    to its matching ')': split at the commas outside nested parentheses,
    except that a variadic macro's last argument takes the rest, commas and
    all.
    */
    static bool collect(
        const c_macro&                  _macro,
        std::vector<xtok>&              _work,
        std::vector<std::vector<xtok>>& _args,
        std::vector<xtok>&              _consumed,
        xtok&                           _close
    )
    {
        _consumed.push_back(std::move(_work.back()));
        _work.pop_back();

        std::vector<xtok> current;
        int               depth = 0;

        while (!_work.empty())
        {
            xtok next = std::move(_work.back());
            _work.pop_back();
            _consumed.push_back(next);

            if (next.tok.is("("))
            {
                ++depth;
            }
            else if (next.tok.is(")"))
            {
                if (depth == 0)
                {
                    _args.push_back(std::move(current));
                    _close = std::move(next);

                    return true;
                }

                --depth;
            }
            else if ( (next.tok.is(",")) &&
                      (depth == 0) &&
                      ( (!_macro.variadic) ||
                        (_args.size() < _macro.params.size()) ) )
            {
                _args.push_back(std::move(current));
                current.clear();

                continue;
            }

            current.push_back(std::move(next));
        }

        return false;
    }

    // arity
    //   whether the arguments are as many as the macro takes.
    static bool arity(
        const c_macro&                        _macro,
        std::vector<std::vector<xtok>>&       _args,
        std::string&                          _problem
    )
    {
        const std::size_t named = _macro.params.size();

        // F() is no arguments to a macro of none, and one empty argument
        // to a macro of one
        if ( (named == 0)          &&
             (!_macro.variadic)    &&
             (_args.size() == 1)   &&
             (_args[0].empty()) )
        {
            _args.clear();

            return true;
        }

        if (_macro.variadic)
        {
            // the variadic part may be left out altogether
            if (_args.size() == named)
            {
                _args.emplace_back();
            }

            if (_args.size() == named + 1)
            {
                return true;
            }
        }
        else if (_args.size() == named)
        {
            return true;
        }

        const std::string count = format_dec(static_cast<u64>(named));
        const std::string given = format_dec(static_cast<u64>(_args.size()));

        _problem = "macro " + _macro.name + " takes " +
                   (_macro.variadic ? "at least " : "") + count +
                   ((named == 1) ? " argument" : " arguments") +
                   (named > 0 ? " (" + params_text(_macro) + ")" : "") +
                   ", and is given " + given;

        return false;
    }

    int record_of(
        const c_macro&                        _macro,
        const xtok&                           _name,
        const std::vector<std::vector<xtok>>& _args
    )
    {
        invocation made;
        made.name   = _macro.name;
        made.text   = _macro.name;
        made.parent = _name.origin;

        if (_macro.function_like)
        {
            made.text += "(";

            for (std::size_t i = 0; i < _args.size(); ++i)
            {
                made.text += ((i == 0) ? "" : ", ") + trimmed(_args[i]);
            }

            made.text += ")";
        }

        if (_name.origin < 0)
        {
            made.line   = _name.tok.line;
            made.column = _name.tok.column;
        }
        else
        {
            const invocation& parent =
                records[static_cast<std::size_t>(_name.origin)];

            made.line   = parent.line;
            made.column = parent.column;
        }

        records.push_back(std::move(made));

        return static_cast<int>(records.size()) - 1;
    }

    // param_index
    //   which parameter a body token names, or -1; the variadic part is
    // the one past the named ones.
    static int param_index(
        const c_macro& _macro,
        const c_token& _token
    )
    {
        if ( (!_macro.function_like) ||
             (_token.kind != c_class::identifier) )
        {
            return -1;
        }

        for (std::size_t i = 0; i < _macro.params.size(); ++i)
        {
            if (_macro.params[i] == _token.text)
            {
                return static_cast<int>(i);
            }
        }

        if ( (_macro.variadic) &&
             (_token.text == _macro.variadic_name) )
        {
            return static_cast<int>(_macro.params.size());
        }

        return -1;
    }

    // stringized
    //   `#` applied to an argument: its spelling as a string literal, the
    // quotes and backslashes of its literals escaped.
    static xtok stringized(
        const std::vector<xtok>& _arg,
        const c_token&           _hash,
        int                      _record
    )
    {
        std::string text = "\"";
        bool        first = true;

        for (const xtok& each : _arg)
        {
            if (each.tok.kind == c_class::placemarker)
            {
                continue;
            }

            if ( (!first) &&
                 (each.tok.has(C_LEADING_SPACE)) )
            {
                text += ' ';
            }

            first = false;

            const bool quoted = ( (each.tok.kind == c_class::string) ||
                                  (each.tok.kind == c_class::character) );

            for (char c : each.tok.text)
            {
                if ( (quoted) &&
                     ( (c == '"') || (c == '\\') ) )
                {
                    text += '\\';
                }

                text += c;
            }
        }

        text += "\"";

        xtok made;
        made.tok        = _hash;
        made.tok.kind   = c_class::string;
        made.tok.text   = std::move(text);
        made.origin     = _record;

        return made;
    }

    /*
    paste
      `##`: the token before it and _right become one, scanned again. A
    placemarker on the left gives way to the right; a result that is not
    exactly one token is reported, and both are kept.
    */
    void paste(
        std::vector<xtok>& _out,
        const xtok&        _right,
        int                _record
    )
    {
        xtok& left = _out.back();

        if (left.tok.kind == c_class::placemarker)
        {
            const unsigned flags = left.tok.flags;
            left = _right;
            set_leading(left.tok, (flags & C_LEADING_SPACE) != 0);

            return;
        }

        if (_right.tok.kind == c_class::placemarker)
        {
            return;
        }

        const std::string joined = left.tok.text + _right.tok.text;
        const c_lexed     again  = lex_c(joined, m_env.settings().level, true);

        if ( (again.tokens.size() != 1)                      ||
             (!again.errors.empty())                         ||
             (again.tokens[0].kind == c_class::comment)      ||
             (again.tokens[0].text.size() != joined.size()) )
        {
            report(c_problem::expansion,
                   "pasting \"" + left.tok.text + "\" and \"" +
                       _right.tok.text + "\" does not give one token",
                   _right.origin >= 0 ? _right : left);
            _out.push_back(_right);

            return;
        }

        left.tok.kind   = again.tokens[0].kind;
        left.tok.text   = joined;
        left.hide       = hs_intersect(left.hide, _right.hide);
        left.origin     = _record;

        return;
    }

    std::vector<xtok>& expansion_of(
        int                                   _param,
        const std::vector<std::vector<xtok>>& _args,
        std::vector<std::vector<xtok>>&       _expanded,
        std::vector<bool>&                    _done
    )
    {
        const std::size_t at = static_cast<std::size_t>(_param);

        if (!_done[at])
        {
            _expanded[at] = run(_args[at]);
            _done[at]     = true;
        }

        return _expanded[at];
    }

    /*
    substitute
      A macro's replacement for one invocation: parameters replaced by
    their arguments -- expanded, except beside # and ## -- # and ## and
    __VA_OPT__ applied, then every token given the invocation's hide set,
    and the first the spacing the macro's name had.
    */
    std::vector<xtok> substitute(
        const c_macro&                        _macro,
        const std::vector<std::vector<xtok>>& _args,
        const hideset&                        _hide,
        int                                   _record,
        unsigned                              _flags
    )
    {
        std::vector<std::vector<xtok>> expanded(_args.size());
        std::vector<bool>              done(_args.size(), false);
        std::vector<xtok>              out;

        replace_range(_macro, _args, expanded, done,
                      0, _macro.tokens.size(), _record, out);

        out.erase(std::remove_if(out.begin(),
                                 out.end(),
                                 [](const xtok& _each)
                                 {
                                     return (_each.tok.kind ==
                                             c_class::placemarker);
                                 }),
                  out.end());

        for (xtok& each : out)
        {
            each.hide = hs_union(each.hide, _hide);
        }

        if (!out.empty())
        {
            set_leading(out.front().tok, (_flags & C_LEADING_SPACE) != 0);
        }

        return out;
    }

    void replace_range(
        const c_macro&                        _macro,
        const std::vector<std::vector<xtok>>& _args,
        std::vector<std::vector<xtok>>&       _expanded,
        std::vector<bool>&                    _done,
        std::size_t                           _begin,
        std::size_t                           _end,
        int                                   _record,
        std::vector<xtok>&                    _out
    )
    {
        const std::vector<c_token>& body     = _macro.tokens;
        const int                   variadic =
            _macro.variadic ? static_cast<int>(_macro.params.size()) : -1;

        for (std::size_t i = _begin; i < _end; ++i)
        {
            const c_token& each = body[i];

            // # parameter
            if ( (_macro.function_like) &&
                 (each.is("#"))         &&
                 (i + 1 < _end) )
            {
                const int param = param_index(_macro, body[i + 1]);

                if (param >= 0)
                {
                    _out.push_back(stringized(
                        _args[static_cast<std::size_t>(param)], each,
                        _record));
                    ++i;

                    continue;
                }
            }

            // ## operand
            if ( (each.is("##")) &&
                 (!_out.empty()) &&
                 (i + 1 < _end) )
            {
                const c_token& next  = body[i + 1];
                const int      param = param_index(_macro, next);

                ++i;

                if (param >= 0)
                {
                    const std::vector<xtok>& arg =
                        _args[static_cast<std::size_t>(param)];

                    if (arg.empty())
                    {
                        // `, ## __VA_ARGS__` with nothing drops the comma
                        if ( (param == variadic) &&
                             (_out.back().tok.is(",")) )
                        {
                            _out.pop_back();
                        }

                        continue;
                    }

                    paste(_out, arg.front(), _record);
                    _out.insert(_out.end(), arg.begin() + 1, arg.end());

                    continue;
                }

                xtok right;
                right.tok    = next;
                right.origin = _record;
                paste(_out, right, _record);

                continue;
            }

            // __VA_OPT__ ( content )
            if ( (variadic >= 0)                 &&
                 (each.named("__VA_OPT__"))      &&
                 (i + 1 < _end)                  &&
                 (body[i + 1].is("(")) )
            {
                std::size_t close = i + 2;
                int         depth = 0;

                for (; close < _end; ++close)
                {
                    depth += body[close].is("(") ? 1 : 0;

                    if (body[close].is(")"))
                    {
                        if (depth == 0)
                        {
                            break;
                        }

                        --depth;
                    }
                }

                const bool present =
                    !expansion_of(variadic, _args, _expanded, _done).empty();

                if (present)
                {
                    const std::size_t first = _out.size();

                    replace_range(_macro, _args, _expanded, _done,
                                  i + 2, close, _record, _out);

                    // the content takes __VA_OPT__'s spacing
                    if (first < _out.size())
                    {
                        set_leading(_out[first].tok,
                                    each.has(C_LEADING_SPACE));
                    }
                }
                else
                {
                    xtok mark;
                    mark.tok.kind = c_class::placemarker;
                    mark.origin   = _record;
                    _out.push_back(std::move(mark));
                }

                i = close;

                continue;
            }

            const int param = param_index(_macro, each);

            if (param < 0)
            {
                xtok made;
                made.tok    = each;
                made.origin = _record;
                _out.push_back(std::move(made));

                continue;
            }

            // a parameter: its argument, unexpanded before ##
            const bool pasted = ( (i + 1 < _end) &&
                                  (body[i + 1].is("##")) );
            const std::vector<xtok>& source =
                pasted ? _args[static_cast<std::size_t>(param)]
                       : expansion_of(param, _args, _expanded, _done);

            if (source.empty())
            {
                if (pasted)
                {
                    xtok mark;
                    mark.tok.kind = c_class::placemarker;
                    mark.origin   = _record;
                    _out.push_back(std::move(mark));
                }

                continue;
            }

            const std::size_t first = _out.size();

            _out.insert(_out.end(), source.begin(), source.end());
            set_leading(_out[first].tok, each.has(C_LEADING_SPACE));
        }

        return;
    }
};


//==============================================================================
// 4.  EVALUATION
//==============================================================================

// shift_rule
//   enum: what the language says of a signed left shift.
enum class shift_rule
{
    strict,     // C, C++98: undefined unless the result is representable
    unsigned_,  // C++11 to C++17: representable as the unsigned type
    modular     // C++20 on: always defined, modulo 2^N
};

// operand
//   struct: a value being computed. `ub` means it met undefined behaviour
// and has no value; `cast` that its type was written in a cast.
struct operand
{
    c_value value;
    bool    ub   = false;
    bool    cast = false;
};

// evaluator
//   class: one evaluation of an expanded token list.
class evaluator
{
public:
    evaluator(
        const c_settings&       _settings,
        const c_scope&          _scope,
        const c_env&            _env,
        const expander&         _expander,
        bool                    _if_mode,
        c_result&               _result
    )
        : m_settings(_settings),
          m_scope(_scope),
          m_env(_env),
          m_expander(_expander),
          m_if_mode(_if_mode),
          m_result(_result)
    {
        m_cpp  = cpp_level(_settings.level);
        m_rule = (!m_cpp) || (_settings.level == standard::cpp98)
                     ? shift_rule::strict
               : (_settings.level >= standard::cpp20) ? shift_rule::modular
                                                      : shift_rule::unsigned_;
    }

    void run(
        const std::vector<xtok>& _tokens
    )
    {
        m_tokens = &_tokens;

        if (_tokens.empty())
        {
            fail(c_problem::syntax, "there is nothing to evaluate", nullptr);

            return;
        }

        if (!names_known())
        {
            return;
        }

        const operand result = expression();

        if (m_failed)
        {
            return;
        }

        if (m_at < _tokens.size())
        {
            fail(c_problem::syntax,
                 "'" + _tokens[m_at].tok.text + "' after the expression",
                 &_tokens[m_at]);

            return;
        }

        m_result.value = result.value;

        if (result.ub)
        {
            m_result.undefined_behaviour = true;

            return;
        }

        m_result.ok = true;

        return;
    }

private:
    const c_settings&         m_settings;
    const c_scope&            m_scope;
    const c_env&              m_env;
    const expander&           m_expander;
    bool                      m_if_mode;
    c_result&                 m_result;
    bool                      m_cpp     = false;
    shift_rule                m_rule    = shift_rule::strict;
    const std::vector<xtok>*  m_tokens  = nullptr;
    std::size_t               m_at      = 0;
    bool                      m_failed  = false;
    int                       m_quiet   = 0;
    int                       m_depth   = 0;
    std::map<std::string, c_value> m_known;
    std::set<std::string>     m_said;

    // --------------------------------------------------------- reporting

    void say(
        c_level            _level,
        c_problem          _problem,
        const std::string& _message,
        const xtok*        _where
    )
    {
        c_diagnostic made;
        made.level   = _level;
        made.problem = _problem;
        made.message = _message;

        if (_where != nullptr)
        {
            made.chain = m_expander.chain_of(_where->origin);
            m_expander.place_of(*_where, made.line, made.column);
        }

        // each once
        const std::string key = made.text();

        if (!m_said.insert(key).second)
        {
            return;
        }

        m_result.diagnostics.push_back(std::move(made));

        return;
    }

    void fail(
        c_problem          _problem,
        const std::string& _message,
        const xtok*        _where
    )
    {
        if (m_failed)
        {
            return;
        }

        m_failed = true;
        say(c_level::error, _problem, _message, _where);

        return;
    }

    // undefined
    //   undefined behaviour met, where the operand is evaluated.
    void undefined(
        operand&           _operand,
        const std::string& _message,
        const xtok&        _where
    )
    {
        _operand.ub = true;

        if (m_quiet == 0)
        {
            say(c_level::warning, c_problem::undefined, _message, &_where);
        }

        return;
    }

    void noted(
        c_problem          _problem,
        const std::string& _message,
        const xtok&        _where
    )
    {
        if (m_quiet == 0)
        {
            say(c_level::note, _problem, _message, &_where);
        }

        return;
    }

    // ------------------------------------------------------------ tokens

    const xtok* peek_token(
        std::size_t _ahead = 0
    ) const
    {
        const std::size_t at = m_at + _ahead;

        return (at < m_tokens->size()) ? &(*m_tokens)[at] : nullptr;
    }

    bool peek(
        const char* _spelling
    ) const
    {
        const xtok* next = peek_token();

        return ( (next != nullptr) &&
                 (punct(*next, _spelling)) );
    }

    // punct
    //   whether a token is an operator, C++'s alternative spellings
    // included.
    bool punct(
        const xtok& _token,
        const char* _spelling
    ) const
    {
        if (_token.tok.is(_spelling))
        {
            return true;
        }

        if ( (!m_cpp) ||
             (_token.tok.kind != c_class::identifier) )
        {
            return false;
        }

        const std::string& word = _token.tok.text;
        const std::string  want = _spelling;

        return ( ( (word == "and")    && (want == "&&") ) ||
                 ( (word == "or")     && (want == "||") ) ||
                 ( (word == "not")    && (want == "!") )  ||
                 ( (word == "bitand") && (want == "&") )  ||
                 ( (word == "bitor")  && (want == "|") )  ||
                 ( (word == "xor")    && (want == "^") )  ||
                 ( (word == "compl")  && (want == "~") )  ||
                 ( (word == "not_eq") && (want == "!=") ) );
    }

    const xtok* next()
    {
        const xtok* at = peek_token();

        if (at != nullptr)
        {
            ++m_at;
        }

        return at;
    }

    bool expect(
        const char* _spelling
    )
    {
        if (peek(_spelling))
        {
            ++m_at;

            return true;
        }

        const xtok* found = peek_token();

        fail(c_problem::syntax,
             (found == nullptr)
                 ? std::string("the expression ends where '") + _spelling +
                       "' should be"
                 : std::string("'") + _spelling + "' expected, not '" +
                       found->tok.text + "'",
             (found != nullptr) ? found : last_token());

        return false;
    }

    const xtok* last_token() const
    {
        return m_tokens->empty() ? nullptr : &m_tokens->back();
    }

    // ------------------------------------------------------------- names

    // word
    //   whether a name is part of the language rather than something a
    // definition must supply.
    bool word(
        const std::string& _name
    ) const
    {
        static const char* const words[] = {
            "sizeof", "signed", "unsigned", "char", "short", "int", "long",
            "const", "volatile", "restrict", "void", "struct", "union",
            "enum", "_Alignof", "alignof", "_Bool", "__int64", "float",
            "double" };
        static const char* const cpp_words[] = {
            "static_cast", "reinterpret_cast", "const_cast", "dynamic_cast",
            "true", "false", "nullptr", "this", "std", "re_std", "and", "or",
            "not", "bitand", "bitor", "xor", "compl", "not_eq",
            "operator", "typename", "decltype" };

        c_type ignored;

        for (const char* each : words)
        {
            if (_name == each)
            {
                return true;
            }
        }

        if (m_cpp)
        {
            for (const char* each : cpp_words)
            {
                if (_name == each)
                {
                    return true;
                }
            }
        }

        if ( (c23()) &&
             ( (_name == "true") || (_name == "false") ||
               (_name == "nullptr") ) )
        {
            return true;
        }

        return typedef_type(_name, m_settings, ignored);
    }

    bool c23() const
    {
        return (m_settings.level == standard::c23);
    }

    /*
    names_known
      Before anything is computed, every name the expansion left is
    looked up -- so that all of the names nothing defines are reported
    together, each with what it might have meant, rather than the first
    alone. In an #if, a name nothing defines is 0.
    */
    bool names_known()
    {
        std::vector<std::string> unknown;
        std::vector<const xtok*> where;

        for (const xtok& each : *m_tokens)
        {
            if ( (each.tok.kind != c_class::identifier) ||
                 (word(each.tok.text)) ||
                 (m_known.count(each.tok.text) != 0) )
            {
                continue;
            }

            const std::string& name = each.tok.text;
            c_value            value;
            std::string        error;

            if ( (!m_if_mode)            &&
                 (m_scope.lookup)        &&
                 (m_scope.lookup(name, value, error)) )
            {
                m_known[name] = value;

                continue;
            }

            if (!error.empty())
            {
                fail(c_problem::undefined_name, error, &each);

                return false;
            }

            if (std::find(unknown.begin(), unknown.end(), name) ==
                unknown.end())
            {
                unknown.push_back(name);
                where.push_back(&each);
            }
        }

        m_result.unknown = unknown;

        if (unknown.empty())
        {
            return true;
        }

        if (m_if_mode)
        {
            for (const std::string& name : unknown)
            {
                m_known[name] = make_value(int_type(), 0);
            }

            return true;
        }

        std::vector<std::string> candidates = m_env.names();

        candidates.insert(candidates.end(),
                          m_scope.names.begin(),
                          m_scope.names.end());

        for (std::size_t i = 0; i < unknown.size(); ++i)
        {
            const std::string& name    = unknown[i];
            std::string        message = name + " is not defined";

            if (m_env.find(name) != nullptr)
            {
                message = name + " is defined in terms of itself, and is "
                          "left as a name";
            }
            else
            {
                const std::vector<std::string> near = suggest(name,
                                                              candidates);

                if (near.size() == 1)
                {
                    message += "; did you mean " + near[0] + "?";
                }
                else if (near.size() > 1)
                {
                    message += "; did you mean " + near[0] + " or " +
                               near[1] + "?";
                }
            }

            say(c_level::error, c_problem::undefined_name, message, where[i]);
        }

        m_failed = true;

        return false;
    }

    // ------------------------------------------------------------- types

    c_type int_type() const
    {
        return (m_if_mode) ? make_type(RANK_LLONG, true, m_settings, "intmax_t")
                           : make_type(RANK_INT, true, m_settings);
    }

    c_type truth_type() const
    {
        return m_cpp ? make_type(RANK_BOOL, false, m_settings)
                     : int_type();
    }

    c_type size_type() const
    {
        c_type made;

        (void)typedef_type("size_t", m_settings, made);

        return made;
    }

    // widened
    //   in an #if, every integer is intmax_t or uintmax_t.
    c_type widened(
        const c_type& _type
    ) const
    {
        if (!m_if_mode)
        {
            return _type;
        }

        return make_type(RANK_LLONG, _type.is_signed, m_settings,
                         _type.is_signed ? "intmax_t" : "uintmax_t");
    }

    // type_start
    //   whether a token, and the ones after it, begin a type's name.
    bool type_start(
        std::size_t _ahead
    ) const
    {
        const xtok* first = peek_token(_ahead);

        if ( (first == nullptr) ||
             (first->tok.kind != c_class::identifier) )
        {
            // C++'s ::uint32_t
            return ( (first != nullptr) &&
                     (m_cpp)            &&
                     (first->tok.is("::")) &&
                     (type_start(_ahead + 1)) );
        }

        const std::string& name = first->tok.text;
        c_type             ignored;

        static const char* const keywords[] = {
            "signed", "unsigned", "char", "short", "int", "long", "const",
            "volatile", "void", "_Bool", "struct", "union", "enum",
            "__int64", "float", "double" };

        for (const char* each : keywords)
        {
            if (name == each)
            {
                return true;
            }
        }

        if ( (m_cpp) &&
             ( (name == "std") || (name == "re_std") ) )
        {
            const xtok* scope = peek_token(_ahead + 1);

            return ( (scope != nullptr) &&
                     (scope->tok.is("::")) &&
                     (type_start(_ahead + 2)) );
        }

        return ( (m_known.count(name) == 0) &&
                 (typedef_type(name, m_settings, ignored)) );
    }

    /*
    type_name
      A type's name, from the tokens on top: specifiers in any order
    (signed, unsigned, char, short, int, long twice, _Bool, the
    qualifiers, which change nothing here) or one typedef name, then any
    number of `*`, which make a pointer.
    */
    bool type_name(
        c_type& _out,
        bool&   _pointer
    )
    {
        int  longs    = 0;
        bool is_signed  = false;
        bool is_unsigned = false;
        int  base     = -1;       // a rank, or -1 for int's default
        bool any      = false;
        bool named    = false;
        bool other    = false;    // void, float, struct...

        _pointer = false;

        for (;;)
        {
            const xtok* at = peek_token();

            if (at == nullptr)
            {
                break;
            }

            const std::string& name = at->tok.text;

            if (at->tok.kind != c_class::identifier)
            {
                if ( (m_cpp) &&
                     (at->tok.is("::")) &&
                     (!any) )
                {
                    ++m_at;

                    continue;
                }

                break;
            }

            if ( (name == "const")    ||
                 (name == "volatile") ||
                 (name == "restrict") )
            {
                ++m_at;

                continue;
            }

            if ( (m_cpp) &&
                 ( (name == "std") || (name == "re_std") ) )
            {
                const xtok* scope = peek_token(1);

                if ( (scope != nullptr) &&
                     (scope->tok.is("::")) )
                {
                    m_at += 2;

                    continue;
                }
            }

            if (name == "signed")        { is_signed = true; }
            else if (name == "unsigned") { is_unsigned = true; }
            else if (name == "char")     { base = RANK_CHAR; }
            else if (name == "short")    { base = RANK_SHORT; }
            else if (name == "int")      { base = (base < 0) ? RANK_INT
                                                             : base; }
            else if (name == "long")     { ++longs; }
            else if (name == "__int64")  { longs = 2; }
            else if (name == "_Bool")    { base = RANK_BOOL; }
            else if ( (name == "void")   ||
                      (name == "float")  ||
                      (name == "double") ||
                      (name == "struct") ||
                      (name == "union")  ||
                      (name == "enum") )
            {
                other = true;
            }
            else if ( (!any) &&
                      (typedef_type(name, m_settings, _out)) )
            {
                named = true;
                ++m_at;
                any = true;

                break;
            }
            else if (other)
            {
                // a struct's tag
            }
            else
            {
                break;
            }

            ++m_at;
            any = true;
        }

        while (peek("*"))
        {
            ++m_at;
            _pointer = true;
        }

        if (!any)
        {
            const xtok* at = peek_token();

            fail(c_problem::syntax, "a type was expected", at);

            return false;
        }

        if (other)
        {
            _pointer = true;   // not an integer, which is all that matters

            return true;
        }

        if (named)
        {
            return true;
        }

        if (longs > 2)
        {
            fail(c_problem::syntax,
                 "'long long long' is too long",
                 peek_token());

            return false;
        }

        const int rank = (base == RANK_CHAR)  ? RANK_CHAR
                       : (base == RANK_SHORT) ? RANK_SHORT
                       : (base == RANK_BOOL)  ? RANK_BOOL
                       : (longs == 2)         ? RANK_LLONG
                       : (longs == 1)         ? RANK_LONG
                                              : RANK_INT;

        _out = make_type(rank, !is_unsigned, m_settings);

        // plain char is a type of its own, signed here
        if ( (rank == RANK_CHAR) &&
             (!is_signed)        &&
             (!is_unsigned) )
        {
            _out.name = "char";
        }

        return true;
    }

    // ------------------------------------------------------- conversions

    /*
    converted
      A value given a type. To a signed type that cannot hold it, the
    result is implementation-defined in C and before C++20; every compiler
    here wraps it, and that is noted.
    */
    operand converted(
        const operand& _from,
        const c_type&  _to,
        const xtok&    _where,
        bool           _cast
    )
    {
        operand out;
        out.ub   = _from.ub;
        out.cast = _cast;

        if ( (_to.is_signed)                       &&
             (!_from.ub)                           &&
             (!fits(_from.value, _to))             &&
             ( (!m_cpp) ||
               (m_settings.level < standard::cpp20) ) )
        {
            const c_value wrapped = make_value(_to, _from.value.bits);

            noted(c_problem::implementation,
                  value_text(_from.value) + " converted to " + _to.name +
                      " is implementation-defined; it wraps to " +
                      value_text(wrapped) + " here",
                  _where);
        }

        out.value = make_value(_to, _from.value.bits);

        return out;
    }

    operand promote(
        const operand& _from
    )
    {
        operand out = _from;
        out.value   = make_value(widened(promoted(_from.value.type,
                                                  m_settings)),
                                 _from.value.bits);

        return out;
    }

    // ------------------------------------------------------- the grammar

    operand expression()
    {
        operand left = conditional();

        while ( (!m_failed) &&
                (peek(",")) )
        {
            const xtok* comma = next();

            if (!m_cpp)
            {
                noted(c_problem::style,
                      "C does not allow the comma operator in a constant "
                      "expression",
                      *comma);
            }

            const operand right = conditional();
            const bool    ub    = left.ub;

            left    = right;
            left.ub = ( (ub) || (right.ub) );
        }

        return left;
    }

    operand conditional()
    {
        if (++m_depth > DEPTH_LIMIT)
        {
            fail(c_problem::syntax, "the expression nests too deeply",
                 peek_token());
            --m_depth;

            return operand{};
        }

        operand condition = logical_or();

        if ( (m_failed) ||
             (!peek("?")) )
        {
            --m_depth;

            return condition;
        }

        const xtok* question = next();
        const bool  truth    = (condition.value.bits != 0);

        // the arm not taken is typed, not evaluated
        if (!truth)
        {
            ++m_quiet;
        }

        operand yes = expression();

        if (!truth)
        {
            --m_quiet;
        }

        if (!expect(":"))
        {
            --m_depth;

            return operand{};
        }

        if (truth)
        {
            ++m_quiet;
        }

        operand no = conditional();

        if (truth)
        {
            --m_quiet;
        }

        --m_depth;

        if (m_failed)
        {
            return operand{};
        }

        const operand chosen = truth ? yes : no;
        const c_type  common = usual(promote(yes).value.type,
                                     promote(no).value.type,
                                     m_settings);

        // a cast's type does not survive ?: when the other arm's differs
        if ( (chosen.cast)                           &&
             (!chosen.value.type.same(common))       &&
             (m_quiet == 0) )
        {
            noted(c_problem::style,
                  "?: gives " + common.name + " whichever arm is taken: the "
                      "arms convert to their common type, so a cast in one "
                      "of them does not choose the result's type",
                  *question);
        }

        operand out = converted(chosen, common, *question, false);
        out.ub      = ( (condition.ub) || (chosen.ub) );

        return out;
    }

    operand logical_or()
    {
        operand left = logical_and();

        while ( (!m_failed) &&
                (peek("||")) )
        {
            ++m_at;

            const bool decided = (left.value.bits != 0);

            if (decided)
            {
                ++m_quiet;
            }

            const operand right = logical_and();

            if (decided)
            {
                --m_quiet;
            }

            operand out;
            out.value = make_value(truth_type(),
                                   ( (decided) || (right.value.bits != 0) )
                                       ? 1u : 0u);
            out.ub    = ( (left.ub) ||
                          ( (!decided) && (right.ub) ) );
            left      = out;
        }

        return left;
    }

    operand logical_and()
    {
        operand left = bit_or();

        while ( (!m_failed) &&
                (peek("&&")) )
        {
            ++m_at;

            const bool decided = (left.value.bits == 0);

            if (decided)
            {
                ++m_quiet;
            }

            const operand right = bit_or();

            if (decided)
            {
                --m_quiet;
            }

            operand out;
            out.value = make_value(truth_type(),
                                   ( (!decided) && (right.value.bits != 0) )
                                       ? 1u : 0u);
            out.ub    = ( (left.ub) ||
                          ( (!decided) && (right.ub) ) );
            left      = out;
        }

        return left;
    }

    operand bit_or()
    {
        operand left = bit_xor();

        while ( (!m_failed) &&
                (peek("|")) &&
                (!peek("||")) )
        {
            const xtok* op = next();
            left = arithmetic(*op, '|', left, bit_xor());
        }

        return left;
    }

    operand bit_xor()
    {
        operand left = bit_and();

        while ( (!m_failed) &&
                (peek("^")) )
        {
            const xtok* op = next();
            left = arithmetic(*op, '^', left, bit_and());
        }

        return left;
    }

    operand bit_and()
    {
        operand left = equality();

        while ( (!m_failed) &&
                (peek("&")) )
        {
            const xtok* op = next();
            left = arithmetic(*op, '&', left, equality());
        }

        return left;
    }

    operand equality()
    {
        operand left = relational();

        for (;;)
        {
            if (m_failed)
            {
                break;
            }

            const char* which = peek("==") ? "==" : peek("!=") ? "!=" : nullptr;

            if (which == nullptr)
            {
                break;
            }

            const xtok* op = next();
            left = compare(*op, which, left, relational());
        }

        return left;
    }

    operand relational()
    {
        operand left = shift();

        for (;;)
        {
            if (m_failed)
            {
                break;
            }

            const char* which = peek("<")  ? "<"
                              : peek(">")  ? ">"
                              : peek("<=") ? "<="
                              : peek(">=") ? ">="
                                           : nullptr;

            if (which == nullptr)
            {
                break;
            }

            const xtok* op = next();
            left = compare(*op, which, left, shift());
        }

        return left;
    }

    operand shift()
    {
        operand left = additive();

        for (;;)
        {
            if (m_failed)
            {
                break;
            }

            const bool to_left  = peek("<<");
            const bool to_right = peek(">>");

            if ( (!to_left) &&
                 (!to_right) )
            {
                break;
            }

            const xtok* op = next();
            left = shifted(*op, to_left, left, additive());
        }

        return left;
    }

    operand additive()
    {
        operand left = multiplicative();

        for (;;)
        {
            if (m_failed)
            {
                break;
            }

            const char which = peek("+") ? '+' : peek("-") ? '-' : '\0';

            if (which == '\0')
            {
                break;
            }

            const xtok* op = next();
            left = arithmetic(*op, which, left, multiplicative());
        }

        return left;
    }

    operand multiplicative()
    {
        operand left = cast();

        for (;;)
        {
            if (m_failed)
            {
                break;
            }

            const char which = peek("*") ? '*'
                             : peek("/") ? '/'
                             : peek("%") ? '%'
                                         : '\0';

            if (which == '\0')
            {
                break;
            }

            const xtok* op = next();
            left = arithmetic(*op, which, left, cast());
        }

        return left;
    }

    operand cast()
    {
        if ( (m_failed) ||
             (!peek("(")) ||
             (!type_start(1)) )
        {
            return unary();
        }

        const xtok* open = next();
        c_type      target;
        bool        pointer = false;

        if ( (!type_name(target, pointer)) ||
             (!expect(")")) )
        {
            return operand{};
        }

        if (peek("{"))
        {
            fail(c_problem::not_integer,
                 "a compound literal is not an integer constant", open);

            return operand{};
        }

        const operand inner = cast();

        if (m_failed)
        {
            return operand{};
        }

        if (pointer)
        {
            fail(c_problem::not_integer,
                 "a cast to a pointer, or to a type that is not an integer, "
                 "is not an integer constant",
                 open);

            return operand{};
        }

        return converted(inner, widened(target), *open, true);
    }

    operand unary()
    {
        const xtok* at = peek_token();

        if (at == nullptr)
        {
            fail(c_problem::syntax, "the expression ends early", last_token());

            return operand{};
        }

        if (punct(*at, "+") || punct(*at, "-") ||
            punct(*at, "~") || punct(*at, "!"))
        {
            const xtok&   op    = *next();
            const operand value = cast();

            if (m_failed)
            {
                return operand{};
            }

            return unary_op(op, value);
        }

        if ( (at->tok.is("&")) ||
             (at->tok.is("*")) )
        {
            fail(c_problem::not_integer,
                 "an address or a dereference is not an integer constant",
                 at);

            return operand{};
        }

        if ( (at->tok.is("++")) ||
             (at->tok.is("--")) ||
             (at->tok.is("=")) )
        {
            fail(c_problem::syntax,
                 "'" + at->tok.text + "' is not allowed in a constant "
                 "expression",
                 at);

            return operand{};
        }

        if ( (at->tok.named("sizeof"))   ||
             (at->tok.named("alignof"))  ||
             (at->tok.named("_Alignof")) )
        {
            return size_of();
        }

        return primary();
    }

    operand unary_op(
        const xtok&    _op,
        const operand& _value
    )
    {
        if (punct(_op, "!"))
        {
            operand out;
            out.value = make_value(truth_type(),
                                   (_value.value.bits == 0) ? 1u : 0u);
            out.ub    = _value.ub;

            return out;
        }

        operand out = promote(_value);

        if (punct(_op, "~"))
        {
            out.value = make_value(out.value.type, ~out.value.bits);

            return out;
        }

        if (punct(_op, "-"))
        {
            if ( (out.value.type.is_signed) &&
                 (out.value.as_signed() == type_min(out.value.type)) &&
                 (!out.ub) )
            {
                undefined(out,
                          "-(" + value_text(out.value) + ") overflows " +
                              out.value.type.name,
                          _op);

                return out;
            }

            out.value = make_value(out.value.type, u64(0) - out.value.bits);
        }

        return out;
    }

    operand size_of()
    {
        const xtok* word_token = next();
        const bool  of_type    = ( (peek("(")) &&
                                   (type_start(1)) );
        c_type      measured;

        if (of_type)
        {
            bool pointer = false;

            ++m_at;

            if ( (!type_name(measured, pointer)) ||
                 (!expect(")")) )
            {
                return operand{};
            }

            if (pointer)
            {
                measured.bits = (m_settings.model == data_model::ilp32) ? 32
                                                                         : 64;
            }
        }
        else if (!word_token->tok.named("sizeof"))
        {
            fail(c_problem::syntax,
                 word_token->tok.text + " takes a type", word_token);

            return operand{};
        }
        else
        {
            ++m_quiet;

            const operand inside = unary();

            --m_quiet;
            measured = inside.value.type;
        }

        if (m_failed)
        {
            return operand{};
        }

        operand out;
        out.value = make_value(
            widened(size_type()),
            static_cast<u64>(std::max(8, measured.bits) / 8));

        return out;
    }

    operand primary()
    {
        const xtok* at = next();

        if (at == nullptr)
        {
            fail(c_problem::syntax, "the expression ends early", last_token());

            return operand{};
        }

        switch (at->tok.kind)
        {
            case c_class::number:
            case c_class::character:
                return literal(*at);

            case c_class::string:
                fail(c_problem::not_integer,
                     "a string is not an integer: " + at->tok.text, at);

                return operand{};

            case c_class::identifier:
                return name(*at);

            case c_class::punctuator:
                break;

            default:
                fail(c_problem::syntax,
                     "'" + at->tok.text + "' cannot begin an operand", at);

                return operand{};
        }

        if (at->tok.is("("))
        {
            const operand inner = expression();

            if (!expect(")"))
            {
                return operand{};
            }

            return inner;
        }

        fail(c_problem::syntax,
             "'" + at->tok.text + "' cannot begin an operand", at);

        return operand{};
    }

    operand literal(
        const xtok& _token
    )
    {
        const c_literal decoded = decode_literal(_token.tok,
                                                 m_settings.level,
                                                 m_settings.model);
        operand         out;

        if (decoded.floating)
        {
            fail(c_problem::not_integer,
                 _token.tok.text + " is floating-point, not an integer",
                 &_token);

            return out;
        }

        if (!decoded.ok)
        {
            fail(c_problem::syntax, _token.tok.text + ": " + decoded.error,
                 &_token);

            return out;
        }

        if (decoded.user_suffix)
        {
            fail(c_problem::not_integer,
                 _token.tok.text + " is a user-defined literal", &_token);

            return out;
        }

        if (decoded.overflow)
        {
            fail(c_problem::syntax,
                 _token.tok.text + " does not fit in 64 bits", &_token);

            return out;
        }

        if (decoded.character)
        {
            out.value = character(decoded);

            return out;
        }

        c_type      type;
        std::string problem;

        if (!literal_type(decoded, m_settings, type, problem))
        {
            fail(c_problem::not_integer, problem, &_token);

            return out;
        }

        if (!problem.empty())
        {
            noted(c_problem::implementation, _token.tok.text + " " + problem,
                  _token);
        }

        out.value = make_value(widened(type), decoded.value);

        return out;
    }

    // character
    //   a character literal's value: in C an int, the char's value when it
    // is one character; in C++ a char. A wide literal is its type's.
    c_value character(
        const c_literal& _literal
    ) const
    {
        c_type type;

        switch (_literal.encoding)
        {
            case 1:  (void)typedef_type("wchar_t", m_settings, type);  break;
            case 3:  (void)typedef_type("char16_t", m_settings, type); break;
            case 4:  (void)typedef_type("char32_t", m_settings, type); break;
            case 2:
                type = (m_cpp && (m_settings.level >= standard::cpp20))
                           ? make_type(RANK_CHAR, false, m_settings, "char8_t")
                           : make_type(RANK_CHAR, false, m_settings);
                break;

            default:
                if ( (!m_cpp) &&
                     (_literal.multichar) )
                {
                    return make_value(widened(int_type()), _literal.value);
                }

                type      = make_type(RANK_CHAR, true, m_settings, "char");
                break;
        }

        const c_value made = make_value(type, _literal.value);

        // C's 'x' is an int holding the char's value
        if ( (!m_cpp) &&
             (_literal.encoding == 0) )
        {
            return make_value(widened(int_type()), made.bits);
        }

        return make_value(widened(type), made.bits);
    }

    operand name(
        const xtok& _token
    )
    {
        const std::string& text = _token.tok.text;

        if ( ( (m_cpp) || (m_settings.level == standard::c23) ) &&
             ( (text == "true") || (text == "false") ) )
        {
            operand out;
            out.value = make_value(widened(make_type(RANK_BOOL, false,
                                                     m_settings)),
                                   (text == "true") ? 1u : 0u);

            return out;
        }

        if ( (text == "nullptr") ||
             (text == "this") )
        {
            fail(c_problem::not_integer, text + " is not an integer",
                 &_token);

            return operand{};
        }

        if ( (m_cpp) &&
             ( (text == "static_cast")      ||
               (text == "reinterpret_cast") ||
               (text == "const_cast")       ||
               (text == "dynamic_cast") ) )
        {
            return named_cast(_token);
        }

        // C++'s T(x) and T{x}: the name begins a type
        --m_at;

        if ( (m_cpp) &&
             (type_start(0)) )
        {
            return functional_cast();
        }

        ++m_at;

        const auto known = m_known.find(text);

        if (known == m_known.end())
        {
            fail(c_problem::syntax,
                 "'" + text + "' cannot be used in an expression", &_token);

            return operand{};
        }

        if (peek("("))
        {
            fail(c_problem::syntax,
                 text + "(...) is a call, and " + text + " is not a "
                 "function-like macro",
                 &_token);

            return operand{};
        }

        operand out;
        out.value = known->second;
        out.value = make_value(widened(out.value.type), out.value.bits);

        return out;
    }

    operand named_cast(
        const xtok& _token
    )
    {
        if (!_token.tok.named("static_cast"))
        {
            fail(c_problem::not_integer,
                 _token.tok.text + " is not allowed in a constant expression",
                 &_token);

            return operand{};
        }

        c_type target;
        bool   pointer = false;

        if ( (!expect("<"))                   ||
             (!type_name(target, pointer))    ||
             (!expect(">"))                   ||
             (!expect("(")) )
        {
            return operand{};
        }

        const operand inner = expression();

        if (!expect(")"))
        {
            return operand{};
        }

        if (pointer)
        {
            fail(c_problem::not_integer,
                 "a cast to a pointer is not an integer constant", &_token);

            return operand{};
        }

        return converted(inner, widened(target), _token, true);
    }

    operand functional_cast()
    {
        const xtok* start   = peek_token();
        c_type      target;
        bool        pointer = false;

        if (!type_name(target, pointer))
        {
            return operand{};
        }

        const bool braces = peek("{");

        if ( (!braces) &&
             (!peek("(")) )
        {
            fail(c_problem::syntax,
                 "a type where a value should be", start);

            return operand{};
        }

        ++m_at;

        operand inner;

        // T() and T{} are zero
        if (peek(braces ? "}" : ")"))
        {
            inner.value = make_value(int_type(), 0);
        }
        else
        {
            inner = expression();
        }

        if ( (!expect(braces ? "}" : ")")) ||
             (pointer) )
        {
            if (pointer)
            {
                fail(c_problem::not_integer,
                     "a pointer is not an integer constant", start);
            }

            return operand{};
        }

        return converted(inner, widened(target), *start, true);
    }

    // ------------------------------------------------------- arithmetic

    operand arithmetic(
        const xtok&    _op,
        char           _which,
        const operand& _left,
        const operand& _right
    )
    {
        if (m_failed)
        {
            return operand{};
        }

        const operand a      = promote(_left);
        const operand b      = promote(_right);
        const c_type  common = usual(a.value.type, b.value.type, m_settings);
        const c_value x      = make_value(common, a.value.bits);
        const c_value y      = make_value(common, b.value.bits);

        operand out;
        out.ub    = ( (a.ub) || (b.ub) );
        out.value = make_value(common, 0);

        if (out.ub)
        {
            return out;
        }

        const std::string spelled_op(1, _which);
        const std::string shown = value_text(x) + " " + spelled_op + " " +
                                  value_text(y);

        // an | that adds no bit
        if ( (_which == '|') &&
             (x.bits != 0)   &&
             (y.bits != 0)   &&
             ( ((x.bits & y.bits) == y.bits) ||
               ((x.bits & y.bits) == x.bits) ) )
        {
            const bool    right = ((x.bits & y.bits) == y.bits);
            const c_value inner = right ? y : x;
            const c_value outer = right ? x : y;

            noted(c_problem::style,
                  "'|' adds nothing: 0x" +
                      format_hex(inner.bits & mask_of(common.bits), 0, true) +
                      " is already set in 0x" +
                      format_hex(outer.bits & mask_of(common.bits), 0, true),
                  _op);
        }

        switch (_which)
        {
            case '|':
                out.value = make_value(common, x.bits | y.bits);
                return out;

            case '^':
                out.value = make_value(common, x.bits ^ y.bits);
                return out;

            case '&':
                out.value = make_value(common, x.bits & y.bits);
                return out;

            default:
                break;
        }

        if ( ( (_which == '/') || (_which == '%') ) &&
             (y.bits == 0) )
        {
            undefined(out,
                      shown + ": " +
                          ((_which == '/') ? "division" : "remainder") +
                          " by zero",
                      _op);

            return out;
        }

        if (!common.is_signed)
        {
            const u64 mask = mask_of(common.bits);
            u64       raw  = 0;

            switch (_which)
            {
                case '+': raw = x.bits + y.bits; break;
                case '-': raw = x.bits - y.bits; break;
                case '*': raw = x.bits * y.bits; break;
                case '/': raw = (x.bits & mask) / (y.bits & mask); break;
                default:  raw = (x.bits & mask) % (y.bits & mask); break;
            }

            out.value = make_value(common, raw);

            return out;
        }

        i64 result = 0;

        if (!signed_op(_which, x.as_signed(), y.as_signed(), common, result))
        {
            undefined(out, shown + " overflows " + common.name, _op);

            return out;
        }

        out.value = make_value(common, static_cast<u64>(result));

        return out;
    }

    /*
    signed_op
      An operation on signed values, exactly: false when the result does
    not fit the type, which C calls undefined. 64-bit overflow is caught
    before it can happen; a narrower type's is caught in 64 bits.
    */
    static bool signed_op(
        char          _which,
        i64           _a,
        i64           _b,
        const c_type& _type,
        i64&          _out
    )
    {
        const i64 high = static_cast<i64>(mask_of(64) >> 1);
        const i64 low  = -high - 1;
        i64       r    = 0;

        switch (_which)
        {
            case '+':
                if ( ( (_b > 0) && (_a > high - _b) ) ||
                     ( (_b < 0) && (_a < low - _b) ) )
                {
                    return false;
                }

                r = _a + _b;
                break;

            case '-':
                if ( ( (_b < 0) && (_a > high + _b) ) ||
                     ( (_b > 0) && (_a < low + _b) ) )
                {
                    return false;
                }

                r = _a - _b;
                break;

            case '*':
                if ( (_a != 0) &&
                     (_b != 0) )
                {
                    const bool over =
                        (_a > 0) ? ( (_b > 0) ? (_a > high / _b)
                                              : (_b < low / _a) )
                                 : ( (_b > 0) ? (_a < low / _b)
                                              : (_b < high / _a) );

                    if (over)
                    {
                        return false;
                    }
                }

                r = _a * _b;
                break;

            case '/':
            case '%':
                if ( (_a == type_min(_type)) &&
                     (_b == -1) )
                {
                    return false;
                }

                r = (_which == '/') ? (_a / _b) : (_a % _b);
                break;

            default:
                break;
        }

        if ( (r < type_min(_type)) ||
             ( (r > 0) && (static_cast<u64>(r) > type_max(_type)) ) )
        {
            return false;
        }

        _out = r;

        return true;
    }

    /*
    shifted
      A shift: the left operand promoted gives the type, the right is
    promoted on its own. A negative count, or one not less than the
    width, is undefined everywhere; a signed left shift is undefined as
    the level says (see shift_rule); a negative value shifted right is
    implementation-defined before C++20, and arithmetic everywhere.
    */
    operand shifted(
        const xtok&    _op,
        bool           _left,
        const operand& _value,
        const operand& _count
    )
    {
        if (m_failed)
        {
            return operand{};
        }

        const operand a    = promote(_value);
        const operand b    = promote(_count);
        const c_type  type = a.value.type;

        operand out;
        out.ub    = ( (a.ub) || (b.ub) );
        out.value = make_value(type, 0);

        if (out.ub)
        {
            return out;
        }

        const bool        negative = b.value.negative();
        const std::string count    = value_text(b.value);

        if (negative)
        {
            undefined(out, "shift count " + count + " is negative", _op);

            return out;
        }

        if (b.value.bits >= static_cast<u64>(type.bits))
        {
            undefined(out,
                      "shift count " + count + " is not less than the " +
                          format_dec(static_cast<u64>(type.bits)) +
                          "-bit width of " + type.name,
                      _op);

            return out;
        }

        const int amount = static_cast<int>(b.value.bits);

        if (!_left)
        {
            if (a.value.negative())
            {
                if ( (!m_cpp) ||
                     (m_settings.level < standard::cpp20) )
                {
                    noted(c_problem::implementation,
                          "shifting the negative " + value_text(a.value) +
                              " right is implementation-defined; it is "
                              "arithmetic here",
                          _op);
                }

                // arithmetic, without relying on >> of a negative
                out.value = make_value(type,
                                       ~((~a.value.bits) >> amount));

                return out;
            }

            out.value = make_value(type, a.value.bits >> amount);

            return out;
        }

        const u64 raw = a.value.bits << amount;

        if ( (type.is_signed) &&
             (m_rule != shift_rule::modular) )
        {
            if (a.value.negative())
            {
                undefined(out,
                          "shifting the negative " + value_text(a.value) +
                              " left is undefined",
                          _op);

                return out;
            }

            // what may be shifted out: nothing at all, or the sign bit too
            const u64 room = (m_rule == shift_rule::strict)
                                 ? (type_max(type) >> amount)
                                 : (mask_of(type.bits) >> amount);

            if (a.value.bits > room)
            {
                undefined(out,
                          value_text(a.value) + " << " + count +
                              " overflows " + type.name,
                          _op);

                return out;
            }
        }

        out.value = make_value(type, raw);

        return out;
    }

    operand compare(
        const xtok&    _op,
        const char*    _which,
        const operand& _left,
        const operand& _right
    )
    {
        if (m_failed)
        {
            return operand{};
        }

        const operand a      = promote(_left);
        const operand b      = promote(_right);
        const c_type  common = usual(a.value.type, b.value.type, m_settings);
        const c_value x      = make_value(common, a.value.bits);
        const c_value y      = make_value(common, b.value.bits);
        const std::string which = _which;

        // a negative signed value meeting an unsigned type
        if (!common.is_signed)
        {
            const operand& turned = a.value.negative() ? a
                                  : b.value.negative() ? b
                                                       : a;

            if (turned.value.negative())
            {
                noted(c_problem::style,
                      value_text(turned.value) + " becomes " +
                          format_dec(make_value(common,
                                                turned.value.bits).bits) +
                          " when compared with " + common.name,
                      _op);
            }
        }

        bool truth = false;

        if (common.is_signed)
        {
            const i64 l = x.as_signed();
            const i64 r = y.as_signed();

            truth = (which == "==") ? (l == r)
                  : (which == "!=") ? (l != r)
                  : (which == "<")  ? (l < r)
                  : (which == ">")  ? (l > r)
                  : (which == "<=") ? (l <= r)
                                    : (l >= r);
        }
        else
        {
            const u64 l = x.bits;
            const u64 r = y.bits;

            truth = (which == "==") ? (l == r)
                  : (which == "!=") ? (l != r)
                  : (which == "<")  ? (l < r)
                  : (which == ">")  ? (l > r)
                  : (which == "<=") ? (l <= r)
                                    : (l >= r);
        }

        operand out;
        out.ub    = ( (a.ub) || (b.ub) );
        out.value = make_value(truth_type(), truth ? 1u : 0u);

        return out;
    }
};

// as_xtoks
//   scanned tokens as the expander's, comments left out.
std::vector<xtok>
as_xtoks(
    const std::vector<c_token>& _tokens
)
{
    std::vector<xtok> out;

    for (const c_token& each : _tokens)
    {
        if (each.kind == c_class::comment)
        {
            continue;
        }

        xtok made;
        made.tok = each;
        out.push_back(std::move(made));
    }

    return out;
}

std::string
spelled_x(
    const std::vector<xtok>& _tokens
)
{
    return trimmed(_tokens);
}


//==============================================================================
// 5.  SUGGESTIONS
//==============================================================================

std::vector<std::string>
words_of(
    const std::string& _name
)
{
    std::vector<std::string> out;
    std::string              current;

    for (char each : _name)
    {
        if (each == '_')
        {
            if (!current.empty())
            {
                out.push_back(current);
                current.clear();
            }

            continue;
        }

        current += static_cast<char>(
            std::toupper(static_cast<unsigned char>(each)));
    }

    if (!current.empty())
    {
        out.push_back(current);
    }

    return out;
}

// edit_distance
//   Levenshtein's, with a swap of two neighbours counted as one edit.
int
edit_distance(
    const std::string& _a,
    const std::string& _b
)
{
    const std::size_t n = _a.size();
    const std::size_t m = _b.size();
    std::vector<std::vector<int>> d(n + 1, std::vector<int>(m + 1, 0));

    for (std::size_t i = 0; i <= n; ++i)
    {
        d[i][0] = static_cast<int>(i);
    }

    for (std::size_t j = 0; j <= m; ++j)
    {
        d[0][j] = static_cast<int>(j);
    }

    for (std::size_t i = 1; i <= n; ++i)
    {
        for (std::size_t j = 1; j <= m; ++j)
        {
            const int cost = (_a[i - 1] == _b[j - 1]) ? 0 : 1;

            d[i][j] = std::min({ d[i - 1][j] + 1,
                                 d[i][j - 1] + 1,
                                 d[i - 1][j - 1] + cost });

            if ( (i > 1) && (j > 1) &&
                 (_a[i - 1] == _b[j - 2]) &&
                 (_a[i - 2] == _b[j - 1]) )
            {
                d[i][j] = std::min(d[i][j], d[i - 2][j - 2] + 1);
            }
        }
    }

    return d[n][m];
}

// abbreviates
//   whether _short is an abbreviation of _long: its first letter, and the
// rest of its letters in order.
bool
abbreviates(
    const std::string& _short,
    const std::string& _long
)
{
    if ( (_short.size() < 2)              ||
         (_short.size() >= _long.size())  ||
         (_short[0] != _long[0]) )
    {
        return false;
    }

    std::size_t at = 0;

    for (char each : _long)
    {
        if ( (at < _short.size()) &&
             (_short[at] == each) )
        {
            ++at;
        }
    }

    return (at == _short.size());
}

// related
//   what replacing one word by another costs: nothing for the same word,
// half for a mistyping or an abbreviation, and too much for any other.
double
related(
    const std::string& _a,
    const std::string& _b
)
{
    if (_a == _b)
    {
        return 0.0;
    }

    if ( (abbreviates(_a, _b)) ||
         (abbreviates(_b, _a)) )
    {
        return 0.5;
    }

    if ( (std::min(_a.size(), _b.size()) >= 3) &&
         (edit_distance(_a, _b) == 1) )
    {
        return 0.5;
    }

    return 1000.0;
}

// distance
//   two names compared a word at a time; a word added or left out costs
// 0.6.
double
distance(
    const std::vector<std::string>& _a,
    const std::vector<std::string>& _b
)
{
    const double gap = 0.6;
    const std::size_t n = _a.size();
    const std::size_t m = _b.size();
    std::vector<std::vector<double>> d(n + 1, std::vector<double>(m + 1, 0.0));

    for (std::size_t i = 0; i <= n; ++i)
    {
        d[i][0] = gap * static_cast<double>(i);
    }

    for (std::size_t j = 0; j <= m; ++j)
    {
        d[0][j] = gap * static_cast<double>(j);
    }

    for (std::size_t i = 1; i <= n; ++i)
    {
        for (std::size_t j = 1; j <= m; ++j)
        {
            d[i][j] = std::min({ d[i - 1][j] + gap,
                                 d[i][j - 1] + gap,
                                 d[i - 1][j - 1] + related(_a[i - 1],
                                                           _b[j - 1]) });
        }
    }

    return d[n][m];
}

}  // anonymous namespace


//==============================================================================
// THE PUBLIC FACE
//==============================================================================

std::string
c_diagnostic::text() const
{
    std::string out;

    for (std::size_t i = 0; i < chain.size(); ++i)
    {
        out += ((i == 0) ? "" : " in ") + chain[i];
    }

    return out.empty() ? message : out + ": " + message;
}

const c_diagnostic*
c_result::first(
    c_level _level
) const
{
    for (const c_diagnostic& each : diagnostics)
    {
        if (each.level >= _level)
        {
            return &each;
        }
    }

    return nullptr;
}

bool
read_params(
    const std::string& _text,
    c_macro&           _macro,
    std::string&       _error
)
{
    _macro.params.clear();
    _macro.variadic      = false;
    _macro.variadic_name = "__VA_ARGS__";

    std::size_t at = 0;

    while (at <= _text.size())
    {
        const std::size_t comma = _text.find(',', at);
        const std::size_t end   = (comma == std::string::npos) ? _text.size()
                                                               : comma;
        std::string       one   = _text.substr(at, end - at);

        // trimmed
        while ( (!one.empty()) &&
                (std::isspace(static_cast<unsigned char>(one.back()))) )
        {
            one.pop_back();
        }

        const std::size_t first = one.find_first_not_of(" \t\r\n");

        one = (first == std::string::npos) ? std::string() : one.substr(first);

        if (one.empty())
        {
            // "" is no parameters; an empty one between commas is an error
            if ( (_text.find_first_not_of(" \t\r\n") != std::string::npos) )
            {
                _error = "a parameter is missing between commas";

                return false;
            }

            break;
        }

        if (_macro.variadic)
        {
            _error = "'...' must be the last parameter";

            return false;
        }

        if (one == "...")
        {
            _macro.variadic = true;
        }
        else if ( (one.size() > 3) &&
                  (one.compare(one.size() - 3, 3, "...") == 0) )
        {
            // GNU's named variadic parameter, args...
            _macro.variadic      = true;
            _macro.variadic_name = one.substr(0, one.size() - 3);
        }
        else
        {
            for (std::size_t i = 0; i < one.size(); ++i)
            {
                const unsigned char c = static_cast<unsigned char>(one[i]);

                if ( (!std::isalnum(c) && one[i] != '_') ||
                     ( (i == 0) && (std::isdigit(c)) ) )
                {
                    _error = "'" + one + "' is not a parameter name";

                    return false;
                }
            }

            if (std::find(_macro.params.begin(),
                          _macro.params.end(),
                          one) != _macro.params.end())
            {
                _error = "the parameter '" + one + "' is named twice";

                return false;
            }

            _macro.params.push_back(one);
        }

        if (comma == std::string::npos)
        {
            break;
        }

        at = comma + 1;
    }

    _macro.function_like = true;

    return true;
}

std::string
params_text(
    const c_macro& _macro
)
{
    std::string out;

    for (const std::string& each : _macro.params)
    {
        out += (out.empty() ? "" : ", ") + each;
    }

    if (_macro.variadic)
    {
        out += (out.empty() ? "" : ", ") +
               ((_macro.variadic_name == "__VA_ARGS__")
                    ? std::string("...")
                    : _macro.variadic_name + "...");
    }

    return out;
}

std::string
type_text(
    const c_type& _type
)
{
    return _type.name;
}

c_value
constant_value(
    u64               _value,
    bool              _negative,
    bool              _decimal,
    int_suffix        _suffix,
    const c_settings& _settings
)
{
    c_literal written;
    written.integer     = true;
    written.value       = _negative ? (u64(0) - _value) : _value;
    written.base        = _decimal ? 10u : 16u;
    written.is_unsigned = ( (_suffix == int_suffix::u)  ||
                            (_suffix == int_suffix::ul) ||
                            (_suffix == int_suffix::ull) );
    written.longs       = (_suffix == int_suffix::ull) ? 2
                        : (_suffix == int_suffix::ul)  ? 1
                                                       : 0;

    c_type      type;
    std::string problem;

    (void)literal_type(written, _settings, type, problem);

    // (-N): N's type, negated
    return make_value(type, _negative ? (u64(0) - written.value)
                                      : written.value);
}

c_env::c_env(
    const c_settings& _settings,
    bool              _standard
)
    : m_settings(_settings)
{
    if (!_standard)
    {
        return;
    }

    std::vector<std::string> notes;

    (void)run_directives(*this, standard_text(_settings), notes);

    for (const auto& each : m_macros)
    {
        m_standard.push_back(each.first);
    }

    return;
}

const c_settings&
c_env::settings() const
{
    return m_settings;
}

/*
define
  The body is scanned at the environment's level, and the definition
checked as a compiler checks one: what # may stand before, where ## may
not stand, and where __VA_ARGS__ and __VA_OPT__ belong.
*/
bool
c_env::define(
    c_macro      _macro,
    std::string& _error
)
{
    if ( (_macro.name.empty()) ||
         (_macro.name == "defined") )
    {
        _error = "'" + _macro.name + "' cannot be defined";

        return false;
    }

    const c_lexed scanned = lex_c(_macro.body, m_settings.level, false);

    _macro.tokens.clear();

    for (const c_token& each : scanned.tokens)
    {
        _macro.tokens.push_back(each);
    }

    const std::vector<c_token>& body = _macro.tokens;

    if ( (!body.empty()) &&
         ( (body.front().is("##")) ||
           (body.back().is("##")) ) )
    {
        _error = "'##' cannot begin or end a macro's body";

        return false;
    }

    for (std::size_t i = 0; i < body.size(); ++i)
    {
        const c_token& each = body[i];

        if ( (_macro.function_like) &&
             (each.is("#")) )
        {
            const bool param =
                ( (i + 1 < body.size()) &&
                  (body[i + 1].kind == c_class::identifier) &&
                  ( (std::find(_macro.params.begin(),
                               _macro.params.end(),
                               body[i + 1].text) != _macro.params.end()) ||
                    ( (_macro.variadic) &&
                      ( (body[i + 1].text == _macro.variadic_name) ||
                        (body[i + 1].text == "__VA_OPT__") ) ) ) );

            if (!param)
            {
                _error = "'#' is not followed by a parameter of " +
                         _macro.name;

                return false;
            }
        }

        if ( ( (each.named("__VA_ARGS__")) ||
               (each.named("__VA_OPT__")) ) &&
             ( (!_macro.variadic) ||
               ( (each.text == "__VA_ARGS__") &&
                 (_macro.variadic_name != "__VA_ARGS__") ) ) )
        {
            _error = each.text + " belongs only in a variadic macro's body";

            return false;
        }
    }

    const std::string name = _macro.name;

    m_macros[name] = std::move(_macro);

    return true;
}

bool
c_env::undefine(
    const std::string& _name
)
{
    return (m_macros.erase(_name) > 0);
}

const c_macro*
c_env::find(
    const std::string& _name
) const
{
    const auto found = m_macros.find(_name);

    return (found == m_macros.end()) ? nullptr : &found->second;
}

bool
c_env::is_standard(
    const std::string& _name
) const
{
    return (std::find(m_standard.begin(), m_standard.end(), _name) !=
            m_standard.end());
}

std::vector<std::string>
c_env::names() const
{
    std::vector<std::string> out;

    for (const auto& each : m_macros)
    {
        out.push_back(each.first);
    }

    return out;
}

c_result
c_env::evaluate(
    std::string_view _text,
    const c_scope&   _scope
) const
{
    c_result      result;
    const c_lexed scanned = lex_c(_text, m_settings.level, false);

    for (const std::string& error : scanned.errors)
    {
        c_diagnostic made;
        made.level   = c_level::error;
        made.problem = c_problem::syntax;
        made.message = error;
        result.diagnostics.push_back(std::move(made));
    }

    if (!scanned.errors.empty())
    {
        return result;
    }

    expander                expand_pass(*this, false, result.diagnostics);
    const std::vector<xtok> expanded =
        expand_pass.run(as_xtoks(scanned.tokens));

    result.expansion = spelled_x(expanded);

    // an expansion that went wrong has said why
    if (result.first(c_level::error) != nullptr)
    {
        return result;
    }

    evaluator compute(m_settings, _scope, *this, expand_pass, false, result);
    compute.run(expanded);

    return result;
}

c_result
c_env::condition(
    std::string_view _text
) const
{
    c_result      result;
    const c_lexed scanned = lex_c(_text, m_settings.level, false);
    const c_scope nothing;

    if (!scanned.errors.empty())
    {
        c_diagnostic made;
        made.level   = c_level::error;
        made.problem = c_problem::syntax;
        made.message = scanned.errors.front();
        result.diagnostics.push_back(std::move(made));

        return result;
    }

    expander                expand_pass(*this, true, result.diagnostics);
    const std::vector<xtok> expanded =
        expand_pass.run(as_xtoks(scanned.tokens));

    result.expansion = spelled_x(expanded);

    if (result.first(c_level::error) != nullptr)
    {
        return result;
    }

    evaluator compute(m_settings, nothing, *this, expand_pass, true, result);
    compute.run(expanded);

    // the __has_ operators answered 0 without knowing
    for (const std::string& each : expand_pass.tested)
    {
        if (each.compare(0, 6, "__has_") == 0)
        {
            result.unknown.push_back(each);
        }
    }

    return result;
}

std::string
c_env::expand(
    std::string_view _text
) const
{
    std::vector<c_diagnostic> ignored;
    const c_lexed             scanned = lex_c(_text, m_settings.level, false);
    expander                  expand_pass(*this, false, ignored);

    return spelled_x(expand_pass.run(as_xtoks(scanned.tokens)));
}

/*
lint
  What every invocation of a definition would meet, found without one: the
calls in its body counted against the macros they call, the names in
capitals nothing defines, and the operators that would bind to the
surroundings of an unparenthesized body.
*/
std::vector<c_diagnostic>
c_env::lint(
    const c_macro& _macro,
    const c_scope& _scope
) const
{
    std::vector<c_diagnostic> out;
    const c_lexed             scanned = lex_c(_macro.body,
                                              m_settings.level,
                                              false);
    const std::vector<c_token>& body = scanned.tokens;
    std::set<std::string>     said;

    // is_param
    //   whether a name is one of the definition's own
    auto is_param = [&_macro](const std::string& _name)
    {
        return ( (std::find(_macro.params.begin(),
                            _macro.params.end(),
                            _name) != _macro.params.end()) ||
                 ( (_macro.variadic) &&
                   ( (_name == _macro.variadic_name) ||
                     (_name == "__VA_OPT__") ) ) );
    };

    // add
    //   a diagnostic, once
    auto add = [&out, &said](c_level            _level,
                             c_problem          _problem,
                             const std::string& _message)
    {
        if (!said.insert(_message).second)
        {
            return;
        }

        c_diagnostic made;
        made.level   = _level;
        made.problem = _problem;
        made.message = _message;
        out.push_back(std::move(made));
    };

    std::vector<std::string> candidates = names();

    candidates.insert(candidates.end(),
                      _scope.names.begin(),
                      _scope.names.end());

    for (std::size_t i = 0; i < body.size(); ++i)
    {
        const c_token& each = body[i];

        if ( (each.kind != c_class::identifier) ||
             (is_param(each.text)) )
        {
            continue;
        }

        const c_macro* called = find(each.text);

        // a call: its arguments counted
        if ( (called != nullptr)         &&
             (called->function_like)     &&
             (i + 1 < body.size())       &&
             (body[i + 1].is("(")) )
        {
            std::size_t count = 1;
            bool        empty = true;
            int         depth = 0;
            std::size_t j     = i + 2;

            for (; j < body.size(); ++j)
            {
                if (body[j].is("("))
                {
                    ++depth;
                }
                else if (body[j].is(")"))
                {
                    if (depth == 0)
                    {
                        break;
                    }

                    --depth;
                }
                else if ( (body[j].is(",")) &&
                          (depth == 0) )
                {
                    ++count;
                }

                empty = false;
            }

            if (j < body.size())
            {
                const std::size_t named = called->params.size();
                const std::size_t given = ( (empty) && (named == 0) ) ? 0
                                                                      : count;
                const bool        right = called->variadic ? (given >= named)
                                                           : (given == named);

                if (!right)
                {
                    add(c_level::warning,
                        c_problem::arguments,
                        called->name + " takes " +
                            (called->variadic ? "at least " : "") +
                            format_dec(static_cast<u64>(named)) +
                            ((named == 1) ? " argument" : " arguments") +
                            (named > 0 ? " (" + params_text(*called) + ")"
                                       : std::string()) +
                            "; this body gives it " +
                            format_dec(static_cast<u64>(given)));
                }
            }

            continue;
        }

        if ( (called != nullptr)                        ||
             (!looks_like_macro(each.text))             ||
             (std::find(_scope.names.begin(),
                        _scope.names.end(),
                        each.text) != _scope.names.end()) )
        {
            continue;
        }

        std::string message = each.text + " is not defined";
        const std::vector<std::string> near = suggest(each.text, candidates);

        if (near.size() == 1)
        {
            message += "; did you mean " + near[0] + "?";
        }
        else if (near.size() > 1)
        {
            message += "; did you mean " + near[0] + " or " + near[1] + "?";
        }

        add(c_level::warning, c_problem::undefined_name, message);
    }

    // an operator at the top of the body binds to the macro's surroundings
    bool whole = ( (body.size() >= 2)    &&
                   (body.front().is("(")) &&
                   (body.back().is(")")) );

    if (whole)
    {
        int depth = 0;

        for (std::size_t i = 0; i < body.size(); ++i)
        {
            depth += body[i].is("(") ? 1 : 0;
            depth -= body[i].is(")") ? 1 : 0;

            // the opening parenthesis closes before the end
            if ( (depth == 0) &&
                 (i + 1 < body.size()) )
            {
                whole = false;

                break;
            }
        }
    }

    if (!whole)
    {
        static const char* const binary[] = {
            "|", "&", "^", "+", "-", "*", "/", "%", "<<", ">>", "<", ">",
            "<=", ">=", "==", "!=", "&&", "||", "?" };
        int  depth   = 0;
        bool operand = false;

        for (const c_token& each : body)
        {
            if (each.is("("))
            {
                ++depth;
                operand = false;

                continue;
            }

            if (each.is(")"))
            {
                --depth;
                operand = true;

                continue;
            }

            bool is_binary = false;

            for (const char* op : binary)
            {
                is_binary = ( (is_binary) || (each.is(op)) );
            }

            if ( (depth == 0) &&
                 (operand)    &&
                 (is_binary) )
            {
                add(c_level::note,
                    c_problem::style,
                    "the body is not in parentheses, so its '" + each.text +
                        "' binds to whatever surrounds " + _macro.name +
                        " where it is used");

                break;
            }

            operand = ( (each.kind == c_class::identifier) ||
                        (each.kind == c_class::number) ||
                        (each.kind == c_class::character) );
        }
    }

    return out;
}


//==============================================================================
// DIRECTIVES
//==============================================================================

c_branches::c_branches(
    const c_env& _env
)
    : m_env(_env)
{}

bool
c_branches::live() const
{
    return ( (m_stack.empty()) ||
             ( (m_stack.back().outer) && (m_stack.back().taken) ) );
}

std::size_t
c_branches::depth() const
{
    return m_stack.size();
}

/*
c_branches::decide
  An #if's or #elif's condition: true, false, or -- resting on a name
nothing defines -- assumed true and noted.
*/
bool
c_branches::decide(
    const std::vector<c_token>& _line,
    int                         _number,
    std::vector<std::string>&   _notes
) const
{
    std::vector<c_token> rest(_line.begin() + 1, _line.end());
    const std::string    text   = spelled(rest);
    const c_result       result = m_env.condition(text);
    const std::string    where  = "line " + std::to_string(_number) + ": ";

    if (result.first(c_level::error) != nullptr)
    {
        _notes.push_back(where + "#" + _line[0].text + " " + text + ": " +
                         result.first(c_level::error)->text() +
                         "; the branch is skipped");

        return false;
    }

    for (const std::string& name : result.unknown)
    {
        if (!is_reserved_name(name))
        {
            _notes.push_back(where + "#" + _line[0].text + " " + text +
                             " rests on " + name + ", which nothing here "
                             "defines; the branch is read as taken");

            return true;
        }
    }

    return ( (result.ok) &&
             (result.value.bits != 0) );
}

bool
c_branches::feed(
    const std::vector<c_token>& _line,
    int                         _number,
    std::vector<std::string>&   _notes
)
{
    if (_line.empty())
    {
        return false;
    }

    const std::string& kind  = _line[0].text;
    const std::string  where = "line " + std::to_string(_number) + ": ";

    if ( (kind == "if")     ||
         (kind == "ifdef")  ||
         (kind == "ifndef") )
    {
        frame made;
        made.outer = live();

        if (made.outer)
        {
            if (kind == "if")
            {
                made.taken = decide(_line, _number, _notes);
            }
            else
            {
                const bool known = ( (_line.size() > 1) &&
                                     (m_env.find(_line[1].text) != nullptr) );

                made.taken = (kind == "ifdef") ? known : !known;
            }
        }

        made.done = made.taken;
        m_stack.push_back(made);

        return true;
    }

    if ( (kind == "elif")    ||
         (kind == "elifdef") ||
         (kind == "elifndef") )
    {
        if (m_stack.empty())
        {
            _notes.push_back(where + "#" + kind + " without #if");

            return true;
        }

        frame&     top   = m_stack.back();
        const bool ready = ( (top.outer) && (!top.done) );
        bool       taken = false;

        if (ready)
        {
            if (kind == "elif")
            {
                taken = decide(_line, _number, _notes);
            }
            else
            {
                const bool known = ( (_line.size() > 1) &&
                                     (m_env.find(_line[1].text) != nullptr) );

                taken = (kind == "elifdef") ? known : !known;
            }
        }

        top.taken = taken;
        top.done  = ( (top.done) || (taken) );

        return true;
    }

    if (kind == "else")
    {
        if (m_stack.empty())
        {
            _notes.push_back(where + "#else without #if");

            return true;
        }

        frame& top = m_stack.back();

        top.taken = ( (top.outer) && (!top.done) );
        top.done  = true;

        return true;
    }

    if (kind == "endif")
    {
        if (m_stack.empty())
        {
            _notes.push_back(where + "#endif without #if");
        }
        else
        {
            m_stack.pop_back();
        }

        return true;
    }

    return false;
}

/*
read_define
  A #define's tokens, after the `#`, as a macro: its name, then -- when a
'(' follows the name with no space between -- its parameters, then its
body.
*/
bool
read_define(
    const std::vector<c_token>& _line,
    c_macro&                    _macro,
    std::size_t&                _body,
    std::string&                _error
)
{
    if ( (_line.size() < 2) ||
         (_line[1].kind != c_class::identifier) )
    {
        _error = "#define needs a name";

        return false;
    }

    _macro.name = _line[1].text;
    _body       = 2;

    if ( (_line.size() > 2) &&
         (_line[2].is("(")) &&
         (!_line[2].has(C_LEADING_SPACE)) )
    {
        std::string params;
        std::size_t k = 3;

        for (; k < _line.size(); ++k)
        {
            if (_line[k].is(")"))
            {
                break;
            }

            if (_line[k].kind == c_class::comment)
            {
                continue;
            }

            params += _line[k].text;
        }

        if (k == _line.size())
        {
            _error = _macro.name + ": its parameters never close";

            return false;
        }

        if (!read_params(params, _macro, _error))
        {
            _error = _macro.name + ": " + _error;

            return false;
        }

        _body = k + 1;
    }

    // the body: its tokens' spelling, comments aside
    std::vector<c_token> rest;

    for (std::size_t k = _body; k < _line.size(); ++k)
    {
        if (_line[k].kind != c_class::comment)
        {
            rest.push_back(_line[k]);
        }
    }

    if (!rest.empty())
    {
        rest.front().flags &= ~C_LEADING_SPACE;
    }

    _macro.body = spelled(rest);

    return true;
}

/*
directive_lines
  A text's directives: for each `#` that starts a line, the tokens of the
same directive after it, and the line it is on.
*/
std::vector<std::pair<int, std::vector<c_token>>>
directive_lines(
    const std::vector<c_token>& _tokens
)
{
    std::vector<std::pair<int, std::vector<c_token>>> out;

    for (std::size_t i = 0; i < _tokens.size(); ++i)
    {
        const c_token& hash = _tokens[i];

        if ( (!hash.is("#"))             ||
             (!hash.has(C_LINE_START))   ||
             (!hash.has(C_IN_DIRECTIVE)) )
        {
            continue;
        }

        std::vector<c_token> line;
        std::size_t          j = i + 1;

        for (; j < _tokens.size(); ++j)
        {
            if ( (!_tokens[j].has(C_IN_DIRECTIVE)) ||
                 ( (_tokens[j].is("#")) && (_tokens[j].has(C_LINE_START)) ) )
            {
                break;
            }

            line.push_back(_tokens[j]);
        }

        out.emplace_back(hash.line, std::move(line));
        i = j - 1;
    }

    return out;
}

/*
run_directives
  Conditionals are followed whatever else is skipped, so their nesting is
right; #define and #undef act only in a branch taken. Everything else in
the text is passed over.
*/
int
run_directives(
    c_env&                    _env,
    std::string_view          _text,
    std::vector<std::string>& _notes
)
{
    const c_lexed scanned  = lex_c(_text, _env.settings().level, false);
    c_branches    branches(_env);
    int           defined  = 0;

    for (const std::string& error : scanned.errors)
    {
        _notes.push_back("line " + error);
    }

    for (const auto& entry : directive_lines(scanned.tokens))
    {
        const int                   number = entry.first;
        const std::vector<c_token>& line   = entry.second;

        if ( (line.empty()) ||
             (branches.feed(line, number, _notes)) ||
             (!branches.live()) )
        {
            continue;
        }

        if (line[0].text == "undef")
        {
            if (line.size() > 1)
            {
                (void)_env.undefine(line[1].text);
            }

            continue;
        }

        if (line[0].text != "define")
        {
            continue;
        }

        c_macro     made;
        std::size_t body = 0;
        std::string error;

        made.origin = "line " + std::to_string(number);

        if ( (!read_define(line, made, body, error)) ||
             (!_env.define(made, error)) )
        {
            _notes.push_back("line " + std::to_string(number) + ": " + error);

            continue;
        }

        ++defined;
    }

    if (branches.depth() > 0)
    {
        _notes.push_back(format_dec(static_cast<u64>(branches.depth())) +
                         " #if without #endif");
    }

    return defined;
}

std::vector<std::string>
suggest(
    const std::string&              _name,
    const std::vector<std::string>& _candidates
)
{
    const std::vector<std::string> words = words_of(_name);
    double                         best  = 1.15;
    std::vector<std::string>       out;

    if (words.empty())
    {
        return out;
    }

    for (const std::string& each : _candidates)
    {
        if (each == _name)
        {
            continue;
        }

        double cost = distance(words, words_of(each));

        // a name that differs only in case is nearer still
        if (cost == 0.0)
        {
            cost = 0.1;
        }

        if (cost < best - 1e-9)
        {
            best = cost;
            out.assign(1, each);
        }
        else if ( (cost <= best + 1e-9) &&
                  (!out.empty()) &&
                  (std::find(out.begin(), out.end(), each) == out.end()) )
        {
            out.push_back(each);
        }
    }

    if (out.size() > 2)
    {
        out.resize(2);
    }

    return out;
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
