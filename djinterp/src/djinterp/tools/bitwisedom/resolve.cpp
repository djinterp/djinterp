/*******************************************************************************
* bitwisedom [bitwisedom]                                            resolve.cpp
*
* Resolving a sheet: three passes over its rows.
*   The first walks the tree and lays out what the tree alone decides --
* each row's names, depth, visibility, format and colour. The second
* computes values, recursing into what an expression or a field_value
* depends on, with a stack of rows in progress so that a cycle is reported
* as the chain it is. The third, in reverse order, gathers each group's
* union and then checks the sheet as a whole: duplicate names, overlapping
* allocations, and the usage of every bit.
*
* path:      /src/djinterp/tools/bitwisedom/resolve.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
#include <djinterp/tools/bitwisedom/resolve.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::find
#include <cctype>     // std::isalnum, std::isalpha, std::isupper
#include <memory>     // std::unique_ptr
#include <utility>    // std::move
#include <vector>     // std::vector
// bitwisedom
#include <djinterp/tools/bitwisedom/cenv.hpp>  // c_env, run_directives
#include <djinterp/tools/bitwisedom/expr.hpp>  // parse_expression, evaluate


namespace bitwisedom
{

namespace
{

// cascade
//   struct: what a row inherits from the groups around it.
struct cascade
{
    std::string prefix;
    std::string suffix;
    radix       fmt     = radix::hex;
    int_suffix  suf     = int_suffix::u;
    bool        visible = true;
    int         depth   = 0;
    int         parent  = 0;
};

// trim_tokens
//   Comma-separated exact tokens; ignore whitespace surrounding a token.
// Match longest first, so an overlapping shorter token cannot hide a longer
// one. Empty entries are ignored. A match removes the complete token.
std::vector<std::string>
trim_tokens(const std::string& _list)
{
    std::vector<std::string> tokens;
    std::size_t start = 0;

    while (start <= _list.size())
    {
        const std::size_t comma = _list.find(',', start);
        const std::size_t end = (comma == std::string::npos)
                                    ? _list.size() : comma;
        const std::size_t first = _list.find_first_not_of(" \t\r\n", start);

        if ( (first != std::string::npos) && (first < end) )
        {
            const std::size_t last = _list.find_last_not_of(" \t\r\n", end - 1);
            tokens.push_back(_list.substr(first, last - first + 1));
        }

        if (comma == std::string::npos)
        {
            break;
        }

        start = comma + 1;
    }

    std::stable_sort(tokens.begin(), tokens.end(),
                     [](const std::string& _a, const std::string& _b)
                     {
                         return _a.size() > _b.size();
                     });
    return tokens;
}

// trim_end
//   Strip boundary tokens repeatedly, but never characters inside a name.
void
trim_end(std::string& _name, const std::vector<std::string>& _tokens,
         bool _left)
{
    bool again = true;

    while (again)
    {
        again = false;

        for (const std::string& token : _tokens)
        {
            if (token.size() > _name.size())
            {
                continue;
            }

            const bool matches = _left
                ? _name.compare(0, token.size(), token) == 0
                : _name.compare(_name.size() - token.size(),
                                token.size(), token) == 0;

            if (matches)
            {
                _name.erase(_left ? 0 : _name.size() - token.size(),
                            token.size());
                again = true;
                break;
            }
        }
    }
}

// trimmed_local
//   The rules operate on each node's own name before affixes/ancestor names
// are concatenated. This preserves separators that belong to those affixes.
std::string
trimmed_local(const sheet& _sheet, std::string _name)
{
    const auto both = trim_tokens(_sheet.trim_both);
    const auto left = trim_tokens(_sheet.trim_left);
    const auto right = trim_tokens(_sheet.trim_right);

    trim_end(_name, both, true);
    trim_end(_name, left, true);
    trim_end(_name, both, false);
    trim_end(_name, right, false);

    return _name;
}

// eval_state
//   enum: where a row's value computation stands.
enum class eval_state
{
    pending,
    running,
    done
};

// span_text
//   "bit n" or "bits a..b", for messages.
std::string
span_text(
    int _lsb,
    int _width
)
{
    if (_width == 1)
    {
        return "bit " + format_dec(static_cast<u64>(_lsb));
    }

    return "bits " + format_dec(static_cast<u64>(_lsb)) + ".." +
           format_dec(static_cast<u64>(_lsb + _width - 1));
}

// bits_words
//   a set of bits for a message: as hex in a sheet an integer holds, and
// as its runs in a wider one.
std::string
bits_words(
    const wide& _bits,
    int         _width
)
{
    if (!is_wide(_width))
    {
        return "0x" + format_hex(_bits.low(), 0, true);
    }

    return runs_text(_bits, 4);
}

// resolver
//   class: one resolution in progress.
class resolver
{
public:
    resolver(
        const sheet&           _sheet,
        const colour_settings& _look,
        resolution&            _out
    )
        : m_sheet(_sheet),
          m_out(_out),
          m_palette(_look.palette),
          m_saturation(_look.saturation),
          m_value(_look.value)
    {}

    void run()
    {
        m_out.width = m_sheet.width;
        m_out.usage.assign(static_cast<std::size_t>(m_sheet.width), 0);

        cascade top;
        top.prefix = m_sheet.prefix;
        top.suffix = m_sheet.suffix;
        top.fmt    = (m_sheet.gen.base == radix::inherit) ? radix::hex
                                                          : m_sheet.gen.base;
        top.suf    = (m_sheet.gen.suffix == int_suffix::inherit)
                         ? int_suffix::u
                         : m_sheet.gen.suffix;

        lay_out(m_sheet.root, top, top);
        name_check();
        build_environment();

        m_state.assign(m_out.rows.size(), eval_state::pending);

        for (std::size_t i = 0; i < m_out.rows.size(); ++i)
        {
            evaluate(i);
        }

        gather_groups();
        check_overlaps();
        check_twins();
        count_usage();
        count_issues();

        return;
    }

private:
    const sheet&             m_sheet;
    resolution&              m_out;
    std::vector<const row*>  m_source;
    std::vector<eval_state>  m_state;
    std::vector<std::size_t> m_stack;
    int                      m_symbols = 0;
    int                      m_groups  = 0;
    c_settings               m_settings;
    std::unique_ptr<c_env>   m_env;
    std::vector<std::string> m_names;     // every symbol's macro name
    int                      m_palette    = 0;   // automatic colours'
    float                    m_saturation = 0.62f;
    float                    m_value      = 0.92f;

    void report(
        severity           _level,
        std::size_t        _index,
        const std::string& _message
    )
    {
        resolved_row& target = m_out.rows[_index];

        m_out.issues.push_back(issue{ _level, target.id, _message });

        // an error is the row's, and the first one is the one it shows
        if ( (_level == severity::error) &&
             (target.error.empty()) )
        {
            target.error     = _message;
            target.has_value = false;
        }

        return;
    }

    // lay_out
    //   the first pass: names, depth, visibility, format and colour.
    void lay_out(
        const row&     _group,
        const cascade& _inherited,
        const cascade& _outer
    )
    {
        for (const row& child : _group.children)
        {
            resolved_row made;
            made.id       = child.id;
            made.parent   = _group.id;
            made.depth    = _inherited.depth;
            made.kind     = child.kind;
            made.def      = child.def;
            made.local    = child.name;
            made.visible  = _inherited.visible;
            made.children = static_cast<int>(child.children.size());
            made.fmt      = (child.fmt == radix::inherit) ? _inherited.fmt
                                                          : child.fmt;
            made.suf      = (child.suf == int_suffix::inherit)
                                ? _inherited.suf
                                : child.suf;
            made.prefix   = _inherited.prefix + child.prefix;
            made.suffix   = child.suffix + _inherited.suffix;
            const std::string local_name = trimmed_local(m_sheet, child.name);
            made.full     = made.prefix + local_name + made.suffix;

            const bool group = (child.kind == row_kind::group);

            // an automatic colour comes from the palette, groups and symbols
            // counted apart so a new group does not shift every symbol's
            if (child.transparent)
            {
                made.colour = 0;
            }
            else if (child.colour != 0)
            {
                made.colour = child.colour;
            }
            else
            {
                made.colour = palette_colour(
                    m_palette,
                    group ? (m_groups * 3 + 1) : m_symbols,
                    m_saturation,
                    m_value);
            }

            if (group)
            {
                ++m_groups;
            }
            else
            {
                ++m_symbols;
            }

            m_out.index[made.id] = m_out.rows.size();
            m_out.rows.push_back(std::move(made));
            m_source.push_back(&child);

            if (!can_contain(child))
            {
                continue;
            }

            // an isolated group starts again from the sheet's own affixes
            const cascade& base = child.isolate ? _outer : _inherited;
            cascade        inner;
            inner.prefix  = base.prefix + child.prefix +
                (child.name_children ? local_name : std::string());
            inner.suffix  = child.suffix + base.suffix;
            inner.fmt     = (child.fmt == radix::inherit) ? _inherited.fmt
                                                          : child.fmt;
            inner.suf     = (child.suf == int_suffix::inherit)
                                ? _inherited.suf
                                : child.suf;
            inner.visible = ( (_inherited.visible) &&
                              (!child.collapsed) );
            inner.depth   = _inherited.depth + 1;
            inner.parent  = child.id;

            lay_out(child, inner, _outer);
        }

        return;
    }

    // name_check
    //   a symbol needs a C identifier for a name, not a reserved one, and
    // not one another symbol already has.
    void name_check()
    {
        for (std::size_t i = 0; i < m_out.rows.size(); ++i)
        {
            const resolved_row& each = m_out.rows[i];

            if (each.kind == row_kind::group)
            {
                continue;
            }

            if (trimmed_local(m_sheet, each.local).empty())
            {
                report(severity::error, i, "the symbol has no name");

                continue;
            }

            if (!is_identifier(each.full))
            {
                report(severity::error,
                       i,
                       "'" + each.full + "' is not a C identifier");

                continue;
            }

            if (is_reserved(each.full))
            {
                report(severity::warning,
                       i,
                       "'" + each.full + "' is reserved to the "
                       "implementation (a leading underscore and a capital, "
                       "or a double underscore)");
            }

            const auto found = m_out.by_full.find(each.full);

            if (found != m_out.by_full.end())
            {
                const std::string message =
                    each.full + " is defined twice";

                report(severity::error, i, message);
                report(severity::error,
                       m_out.index[found->second],
                       message);

                continue;
            }

            m_out.by_full[each.full] = each.id;
        }

        return;
    }

    // evaluate
    //   the second pass, for one row: its value, after whatever it uses.
    void evaluate(
        std::size_t _index
    )
    {
        if (m_state[_index] == eval_state::done)
        {
            return;
        }

        m_state[_index] = eval_state::running;
        m_stack.push_back(_index);

        resolved_row& target = m_out.rows[_index];
        const row&    source = *m_source[_index];

        // a symbol already in error is not evaluated over it
        if ( (target.kind == row_kind::symbol) &&
             (target.error.empty()) )
        {
            switch (source.def)
            {
                case def_kind::bit:         evaluate_bit(_index);    break;
                case def_kind::range:       evaluate_range(_index);  break;
                case def_kind::field_value: evaluate_value(_index);  break;
                case def_kind::literal:     evaluate_literal(_index); break;
                case def_kind::expression:  evaluate_expr(_index);   break;
                case def_kind::macro:       evaluate_macro(_index);  break;
            }
        }

        m_stack.pop_back();
        m_state[_index] = eval_state::done;

        return;
    }

    void evaluate_bit(
        std::size_t _index
    )
    {
        const row&    source = *m_source[_index];
        resolved_row& target = m_out.rows[_index];

        if ( (source.bit < 0) ||
             (source.bit >= m_sheet.width) )
        {
            report(severity::error,
                   _index,
                   "bit " + format_signed(source.bit) + " is outside the " +
                       format_dec(static_cast<u64>(m_sheet.width)) +
                       "-bit width");

            return;
        }

        settle(target, wide::bit(source.bit));
        target.span_lsb   = source.bit;
        target.span_width = 1;

        return;
    }

    // settle
    //   a row's bits, within the sheet's width, and its value as an
    // integer from them.
    void settle(
        resolved_row& _target,
        wide          _bits
    ) const
    {
        _bits.trim(m_sheet.width);

        _target.has_value = true;
        _target.value     = _bits.low();
        _target.bits      = std::move(_bits);

        return;
    }

    // span_fits
    //   whether a run of bits lies within the sheet; reports it if not.
    bool span_fits(
        std::size_t _index,
        int         _lsb,
        int         _width
    )
    {
        if ( (_lsb < 0)                       ||
             (_width < 1)                     ||
             (_lsb + _width > m_sheet.width) )
        {
            report(severity::error,
                   _index,
                   ( (_width < 1) || (_lsb < 0) )
                       ? std::string("a range needs a width of at least 1 "
                                     "and a lowest bit of at least 0")
                       : span_text(_lsb, _width) + " run past the " +
                             format_dec(static_cast<u64>(m_sheet.width)) +
                             "-bit width");

            return false;
        }

        return true;
    }

    void evaluate_range(
        std::size_t _index
    )
    {
        const row&    source = *m_source[_index];
        resolved_row& target = m_out.rows[_index];

        if (!span_fits(_index, source.lsb, source.width))
        {
            return;
        }

        settle(target, wide::run(source.lsb, source.width));
        target.span_lsb   = source.lsb;
        target.span_width = source.width;

        return;
    }

    /*
    evaluate_value
      A field_value placed in a range row takes that row's bits; one with no
    field takes its own. A number the encoding cannot hold is a warning, not
    an error: it still has bits -- the ones it wraps to -- and they are shown.
    */
    void evaluate_value(
        std::size_t _index
    )
    {
        const row& source = *m_source[_index];
        int        lsb    = source.lsb;
        int        width  = source.width;

        if (source.field != 0)
        {
            const auto found = m_out.index.find(source.field);

            if ( (found == m_out.index.end()) ||
                 (m_out.rows[found->second].kind != row_kind::symbol) ||
                 (m_source[found->second]->def != def_kind::range) )
            {
                report(severity::error,
                       _index,
                       "its field is not a range symbol");

                return;
            }

            evaluate(found->second);

            const resolved_row& field = m_out.rows[found->second];

            if (!field.has_value)
            {
                report(severity::error,
                       _index,
                       "its field, " + field.local + ", has no value");

                return;
            }

            lsb   = field.span_lsb;
            width = field.span_width;
        }
        else if (!span_fits(_index, lsb, width))
        {
            return;
        }

        // a number is an integer, and its field no wider than one
        if (width > INTEGER_WIDTH)
        {
            report(severity::error,
                   _index,
                   "a value's field is at most 64 bits wide; this one "
                   "spans " + format_dec(static_cast<u64>(width)));

            return;
        }

        u64        bits = 0;
        const bool fits = encode(source.enc,
                                 width,
                                 source.bias,
                                 source.number,
                                 bits);

        resolved_row& target = m_out.rows[_index];
        settle(target, shift_left(wide(bits), lsb, m_sheet.width));
        target.number     = decode(source.enc, width, source.bias, bits);
        target.negative   = (source.number < 0);
        target.span_lsb   = lsb;
        target.span_width = width;
        target.field      = source.field;

        if (!fits)
        {
            const value_range range = range_of(source.enc,
                                               width,
                                               source.bias);

            report(severity::warning,
                   _index,
                   format_signed(source.number) + " does not fit " +
                       encoding_name(source.enc) + " in " +
                       format_dec(static_cast<u64>(width)) + " bits (" +
                       format_signed(range.low) + ".." +
                       format_signed(range.high) + "); it wraps to " +
                       format_signed(target.number));
        }

        return;
    }

    void evaluate_literal(
        std::size_t _index
    )
    {
        const row&    source = *m_source[_index];
        resolved_row& target = m_out.rows[_index];

        // a negative number is its two's complement in the sheet's width:
        // within 64 bits, the low bits of its 64-bit pattern; past them,
        // that pattern with its sign carried up
        if ( (source.negative) &&
             (is_wide(m_sheet.width)) )
        {
            settle(target,
                   widen_signed(static_cast<i64>(source.literal.low()),
                                m_sheet.width));
        }
        else
        {
            settle(target, source.literal);
        }

        target.negative = source.negative;

        // a positive number wider than the sheet loses its top bits
        if ( (!source.negative) &&
             (!source.literal.fits(m_sheet.width)) )
        {
            report(severity::warning,
                   _index,
                   "0x" + format_hex(source.literal, 0, true) +
                       " has bits above the " +
                       format_dec(static_cast<u64>(m_sheet.width)) +
                       "-bit width; they are dropped");
        }

        return;
    }

    void evaluate_expr(
        std::size_t _index
    )
    {
        const row&       source = *m_source[_index];
        const expression parsed = parse_expression(source.expr);

        if (!parsed.ok())
        {
            report(severity::error,
                   _index,
                   parsed.error + " (at " +
                       format_dec(static_cast<u64>(parsed.error_at)) + ")");

            return;
        }

        const int  self   = m_out.rows[_index].id;
        bool       cycled = false;

        // a wide sheet's expressions are over sets of bits
        if (is_wide(m_sheet.width))
        {
            const wide_evaluation result = bitwisedom::evaluate_wide(
                parsed,
                [this, self, &cycled](const std::string& _name,
                                      wide&              _value,
                                      std::string&       _error)
                {
                    u64 low = 0;

                    if (!look_up(self, _name, low, _error, cycled))
                    {
                        return false;
                    }

                    _value = m_out.rows[m_out.index[find_name(
                        m_out, _name, self, _error)]].bits;

                    return true;
                },
                m_sheet.width);

            if (!result.ok)
            {
                report(severity::error, _index, result.error);

                return;
            }

            settle(m_out.rows[_index], result.value);

            if (result.overshift)
            {
                report(severity::warning,
                       _index,
                       "a shift past the " +
                           format_dec(static_cast<u64>(m_sheet.width)) +
                           "-bit width gives 0");
            }

            return;
        }

        const evaluation result = bitwisedom::evaluate(
            parsed,
            [this, self, &cycled](const std::string& _name,
                                  u64&               _value,
                                  std::string&       _error)
            {
                return look_up(self, _name, _value, _error, cycled);
            });

        if (!result.ok)
        {
            report(severity::error, _index, result.error);

            return;
        }

        const u64     mask   = width_mask(m_sheet.width);
        resolved_row& target = m_out.rows[_index];

        settle(target, wide(result.value & mask));
        target.negative  = ((result.value >> 63) != 0);

        if (result.overshift)
        {
            report(severity::warning,
                   _index,
                   "a shift by 64 or more gives 0 here, and is undefined "
                   "in C");
        }

        if ( (!target.negative)               &&
             ((result.value & ~mask) != 0) )
        {
            report(severity::warning,
                   _index,
                   "the value has bits above the " +
                       format_dec(static_cast<u64>(m_sheet.width)) +
                       "-bit width; they are dropped");
        }

        return;
    }

    // look_up
    //   an expression's name, resolved and evaluated; a row still running
    // is a cycle, reported as the chain from it to here.
    bool look_up(
        int                _self,
        const std::string& _name,
        u64&               _value,
        std::string&       _error,
        bool&              _cycled
    )
    {
        const int id = find_name(m_out, _name, _self, _error);

        if (id == 0)
        {
            return false;
        }

        const std::size_t index = m_out.index[id];

        if (m_state[index] == eval_state::running)
        {
            std::string chain;
            bool        inside = false;

            for (std::size_t each : m_stack)
            {
                inside = ( (inside) ||
                           (each == index) );

                if (inside)
                {
                    chain += m_out.rows[each].local + " -> ";
                }
            }

            _error  = "cycle: " + chain + m_out.rows[index].local;
            _cycled = true;

            return false;
        }

        evaluate(index);

        const resolved_row& found = m_out.rows[index];

        // a macro has a value when it is object-like and evaluates
        if ( (found.kind != row_kind::symbol) ||
             ( (found.def == def_kind::macro) &&
               (function_like(*m_source[index])) ) )
        {
            _error = "'" + _name + "' is not a symbol with a value";

            return false;
        }

        if (!found.has_value)
        {
            _error = "'" + _name + "' has no value: " +
                     (found.error.empty() ? std::string("see its row")
                                          : found.error);

            return false;
        }

        _value = found.value;

        return true;
    }

    // ------------------------------------------------------ the C side

    /*
    build_environment
      The standard headers' macros, then the sheet's environment text,
    then every macro row in error-free standing, defined under its macro
    name. A macro row C would refuse -- `#` before a non-parameter, `##` at
    an end -- is in error, and stays out of the header.
    */
    void build_environment()
    {
        m_settings.level = m_sheet.gen.level;
        m_settings.model = m_sheet.gen.model;
        m_env            = std::make_unique<c_env>(m_settings);

        if (!m_sheet.environment.empty())
        {
            std::vector<std::string> notes;

            (void)run_directives(*m_env, m_sheet.environment, notes);

            for (const std::string& each : notes)
            {
                m_out.issues.push_back(issue{ severity::warning, 0,
                                              "environment, " + each });
            }
        }

        for (std::size_t i = 0; i < m_out.rows.size(); ++i)
        {
            const resolved_row& each = m_out.rows[i];

            if (each.kind == row_kind::symbol)
            {
                m_names.push_back(each.full);
            }
        }

        for (std::size_t i = 0; i < m_out.rows.size(); ++i)
        {
            const resolved_row& each   = m_out.rows[i];
            const row&          source = *m_source[i];

            if ( (each.kind != row_kind::symbol)  ||
                 (source.def != def_kind::macro)  ||
                 (!each.error.empty()) )
            {
                continue;
            }

            c_macro     made;
            std::string error;

            made.name   = each.full;
            made.origin = each.local;

            if ( (function_like(source)) &&
                 (!read_params(source.params, made, error)) )
            {
                report(severity::error, i, "its parameters: " + error);

                continue;
            }

            made.body = macro_body(m_out, source, nullptr);

            if (!m_env->define(made, error))
            {
                report(severity::error, i, error);
            }
        }

        return;
    }

    // scope_for
    //   what a macro's evaluation may look up: the sheet's other symbols,
    // by macro name, each evaluated as it is asked for.
    c_scope scope_for(
        int _self
    )
    {
        c_scope scope;
        scope.names  = m_names;
        scope.lookup = [this, _self](const std::string& _name,
                                     c_value&           _value,
                                     std::string&       _error)
        {
            const auto found = m_out.by_full.find(_name);

            if (found == m_out.by_full.end())
            {
                return false;
            }

            const std::size_t   index  = m_out.index[found->second];
            const resolved_row& target = m_out.rows[index];
            const row&          source = *m_source[index];

            // a macro is expanded, not looked up: one that reaches here is
            // function-like, or blocked by its own expansion
            if ( (target.kind != row_kind::symbol) ||
                 (source.def == def_kind::macro) )
            {
                return false;
            }

            bool        cycled = false;
            u64         value  = 0;
            std::string error;

            if (!look_up(_self, _name, value, error, cycled))
            {
                _error = error;

                return false;
            }

            // in a wide sheet a symbol is in C what its header makes it:
            // a place in the arrays, or a number
            if (is_wide(m_sheet.width))
            {
                const c_constant known = wide_constant(m_out.rows[index]);

                if (!known.ok)
                {
                    _error = _name + " is " + known.error;

                    return false;
                }

                _value = constant_value(known.value,
                                        known.negative,
                                        true,
                                        target.suf,
                                        m_settings);

                return true;
            }

            const bool negative = ( (source.def == def_kind::literal) &&
                                    (source.negative) &&
                                    (target.negative) );
            const u64  bits     = negative
                                      ? static_cast<u64>(
                                            sign_extend(value, m_sheet.width))
                                      : value;

            _value = constant_value(bits,
                                    negative,
                                    (target.fmt == radix::dec) || negative,
                                    target.suf,
                                    m_settings);

            return true;
        };

        return scope;
    }

    // speaks_c
    //   whether a body is an expression at all, rather than statements or
    // declarations, which a macro may well be and which have no value.
    static bool speaks_c(
        const std::vector<c_token>& _tokens
    )
    {
        static const char* const words[] = {
            "do", "while", "for", "if", "else", "return", "switch", "case",
            "goto", "break", "continue", "typedef", "struct", "union",
            "enum", "static", "extern", "inline", "void" };

        for (const c_token& each : _tokens)
        {
            if ( (each.is(";")) ||
                 (each.is("{")) ||
                 (each.is("}")) )
            {
                return false;
            }

            if (each.kind != c_class::identifier)
            {
                continue;
            }

            for (const char* word : words)
            {
                if (each.text == word)
                {
                    return false;
                }
            }
        }

        return true;
    }

    // severity_of
    //   what an evaluation's diagnostic is on a row: a value-less macro is
    // not an error -- it is written as it is either way -- so the worst is a
    // warning, and a body that is simply not an integer is a note.
    static severity severity_of(
        const c_diagnostic& _diagnostic
    )
    {
        if (_diagnostic.problem == c_problem::not_integer)
        {
            return severity::note;
        }

        return (_diagnostic.level == c_level::note) ? severity::note
                                                    : severity::warning;
    }

    // tell
    //   one of an evaluation's diagnostics, on the row it belongs to: a
    // remark about a definition the evaluation went through goes on that
    // definition's row, once; the rest go on the row evaluated, without
    // the row's own name at the head of the chain.
    void tell(
        const c_diagnostic& _diagnostic,
        std::size_t         _index
    )
    {
        c_diagnostic  shown  = _diagnostic;
        const std::string& self = m_out.rows[_index].full;

        if ( (!shown.chain.empty()) &&
             (shown.chain.back() == self) )
        {
            shown.chain.pop_back();
        }

        std::size_t owner = _index;

        if ( (shown.problem == c_problem::style) &&
             (!shown.chain.empty()) )
        {
            const std::string& inner = shown.chain.front();
            const std::string  name  = inner.substr(0, inner.find('('));
            const auto         found = m_out.by_full.find(name);

            if (found != m_out.by_full.end())
            {
                owner = m_out.index[found->second];
                shown.chain.clear();
            }
        }

        std::string text = shown.text();

        // a local name where its macro name belongs
        if (shown.problem == c_problem::undefined_name)
        {
            const std::string name = shown.message.substr(
                0, shown.message.find(' '));

            for (const resolved_row& each : m_out.rows)
            {
                if ( (each.kind == row_kind::symbol) &&
                     (each.local == name) &&
                     (each.full != name) )
                {
                    text += " (" + name + " is " + each.full + "'s own name; "
                            "write {" + name + "} for its macro name)";

                    break;
                }
            }
        }

        if (_diagnostic.problem == c_problem::undefined)
        {
            text = "undefined behaviour: " + text;
        }

        // each once on its row
        for (const issue& each : m_out.issues)
        {
            if ( (each.row == m_out.rows[owner].id) &&
                 (each.message == text) )
            {
                return;
            }
        }

        report(severity_of(_diagnostic), owner, text);

        return;
    }

    /*
    evaluate_macro
      A function-like macro has no value; its body is checked, as every
    macro's is. An object-like one is evaluated by name, and keeps its
    value, its type and its expansion.
    */
    void evaluate_macro(
        std::size_t _index
    )
    {
        const std::string name  = m_out.rows[_index].full;
        const c_macro*    made  = m_env ? m_env->find(name) : nullptr;
        const int         self  = m_out.rows[_index].id;

        if (made == nullptr)
        {
            return;
        }

        const c_scope scope = scope_for(self);

        for (const c_diagnostic& each : m_env->lint(*made, scope))
        {
            // an object-like macro's names are checked by evaluating it
            if ( (!made->function_like) &&
                 (each.problem == c_problem::undefined_name) )
            {
                continue;
            }

            tell(each, _index);
        }

        if ( (made->function_like) ||
             (made->tokens.empty()) ||
             (!speaks_c(made->tokens)) )
        {
            return;
        }

        const c_result result = m_env->evaluate(name, scope);

        m_out.rows[_index].expansion = result.expansion;

        for (const c_diagnostic& each : result.diagnostics)
        {
            tell(each, _index);
        }

        if (!result.ok)
        {
            return;
        }

        const u64     mask   = width_mask(m_sheet.width);
        resolved_row& target = m_out.rows[_index];

        settle(target, wide(result.value.bits & mask));
        target.value     = result.value.bits & mask;
        target.negative  = result.value.negative();
        target.ctype     = result.value.type.name;

        if ( (!target.negative) &&
             ((result.value.bits & ~mask) != 0) )
        {
            report(severity::note,
                   _index,
                   "its value, 0x" + format_hex(result.value.bits, 0, true) +
                       ", has bits above the " +
                       format_dec(static_cast<u64>(m_sheet.width)) +
                       "-bit width; the grid shows the bits within it");
        }

        return;
    }

    /*
    check_twins
      Two literals or valued macros in one group with the same value are
    noted -- codes that should differ often do not, when a flag is ORed
    into bits the code already has. One that is the other's alias, its body
    the other's name alone, is not.
    */
    void check_twins()
    {
        for (std::size_t i = 0; i < m_out.rows.size(); ++i)
        {
            if (!is_constant(i))
            {
                continue;
            }

            for (std::size_t j = 0; j < i; ++j)
            {
                if ( (!is_constant(j)) ||
                     (m_out.rows[j].parent != m_out.rows[i].parent) ||
                     (m_out.rows[j].bits != m_out.rows[i].bits) ||
                     (is_alias(i, j)) ||
                     (is_alias(j, i)) )
                {
                    continue;
                }

                report(severity::note,
                       i,
                       "has the value of " + m_out.rows[j].full + ", " +
                           bits_words(m_out.rows[i].bits, m_sheet.width));

                break;
            }
        }

        return;
    }

    bool is_constant(
        std::size_t _index
    ) const
    {
        const resolved_row& each = m_out.rows[_index];

        return ( (each.kind == row_kind::symbol) &&
                 (each.has_value)                &&
                 ( (each.def == def_kind::literal) ||
                   (each.def == def_kind::macro) ) );
    }

    // is_alias
    //   whether row _a's body is row _b's name, and nothing else.
    bool is_alias(
        std::size_t _a,
        std::size_t _b
    ) const
    {
        const row& source = *m_source[_a];

        if (source.def != def_kind::macro)
        {
            return false;
        }

        std::string body = source.body;

        while ( (!body.empty()) &&
                ( (body.front() == ' ') || (body.front() == '(') ) )
        {
            body.erase(0, 1);
        }

        while ( (!body.empty()) &&
                ( (body.back() == ' ') || (body.back() == ')') ) )
        {
            body.pop_back();
        }

        return ( (body == m_out.rows[_b].full) ||
                 (body == "{" + m_out.rows[_b].local + "}") );
    }

    // gather_groups
    //   a group's value is the union of everything in it.
    void gather_groups()
    {
        for (std::size_t i = m_out.rows.size(); i-- > 0; )
        {
            const resolved_row& each = m_out.rows[i];

            if ( (!each.has_value) ||
                 (each.parent == 0) )
            {
                continue;
            }

            resolved_row& parent = m_out.rows[m_out.index[each.parent]];
            // A range is its declared span, not the union of descendants.
            if (parent.kind != row_kind::group)
            {
                continue;
            }
            parent.has_value = true;
            parent.bits     |= each.bits;
            parent.value     = parent.bits.low();
        }

        return;
    }

    // check_overlaps
    //   Nested bit and range allocations must fit their enclosing range.
    //   Fully contained ranges (including identical endpoints) are legal;
    //   partial intersections and overlapping independent bits are not.
    //   Within an exclusive group, no two bit or range symbols may share a
    // bit. A shared bit is a warning on both, and a collision in the grid.
    void check_overlaps()
    {
        for (std::size_t i = 0; i < m_out.rows.size(); ++i)
        {
            if (!is_allocation(i))
            {
                continue;
            }
            const resolved_row& child = m_out.rows[i];
            const resolved_row* parent = m_out.find(child.parent);
            // A group between two ranges does not remove the enclosing
            // range's bounds; validate against every range ancestor.
            while (parent != nullptr)
            {
                if ((parent->kind == row_kind::symbol) &&
                    (parent->def == def_kind::range) &&
                    (parent->has_value) &&
                    ((child.bits & parent->bits) != child.bits))
                {
                    report(severity::error, i,
                           "allocation extends outside parent range " +
                           parent->full);
                }
                parent = m_out.find(parent->parent);
            }
        }
        for (std::size_t i = 0; i < m_out.rows.size(); ++i)
        {
            if (!is_allocation(i))
            {
                continue;
            }

            const resolved_row& first = m_out.rows[i];
            const row*          group = find_row(m_sheet, first.parent);

            if ( (group != nullptr) &&
                 (!group->exclusive) )
            {
                continue;
            }

            for (std::size_t j = i + 1; j < m_out.rows.size(); ++j)
            {
                if ( (!is_allocation(j))                       ||
                     (m_out.rows[j].parent != first.parent) )
                {
                    continue;
                }

                const wide shared = first.bits & m_out.rows[j].bits;

                if (!shared.any())
                {
                    continue;
                }
                // A range wholly enclosed by another range is an
                // intentional subset, not a conflicting allocation.
                if (((first.def == def_kind::range) ||
                     (m_out.rows[j].def == def_kind::range)) &&
                    ((shared == first.bits) ||
                     (shared == m_out.rows[j].bits)))
                {
                    continue;
                }

                m_out.rows[i].overlap |= shared;
                m_out.rows[j].overlap |= shared;
                m_out.collisions      |= shared;

                const std::string bits = bits_words(shared, m_sheet.width);

                report(severity::warning,
                       i,
                       "shares bits " + bits + " with " +
                           m_out.rows[j].local);
                report(severity::warning,
                       j,
                       "shares bits " + bits + " with " + first.local);
            }
        }

        return;
    }

    bool is_allocation(
        std::size_t _index
    ) const
    {
        const resolved_row& each = m_out.rows[_index];

        return ( (each.kind == row_kind::symbol) &&
                 (each.has_value)                &&
                 ( (each.def == def_kind::bit) ||
                   (each.def == def_kind::range) ) );
    }

    void count_usage()
    {
        for (std::size_t i = 0; i < m_out.rows.size(); ++i)
        {
            if (!is_allocation(i))
            {
                continue;
            }

            const wide& bits = m_out.rows[i].bits;
            m_out.allocated |= bits;

            for (int b = bits.next(0);
                 (b >= 0) && (b < m_sheet.width);
                 b = bits.next(b + 1))
            {
                ++m_out.usage[static_cast<std::size_t>(b)];
            }
        }

        return;
    }

    void count_issues()
    {
        for (const issue& each : m_out.issues)
        {
            if (each.level == severity::error)
            {
                ++m_out.errors;
            }
            else if (each.level == severity::warning)
            {
                ++m_out.warnings;
            }
        }

        return;
    }
};

}  // anonymous namespace


resolution
resolve(
    const sheet& _sheet
)
{
    // the dark theme's first scheme, as a window opens with
    return resolve(_sheet, make_look(_sheet, 0, 0).colours);
}

resolution
resolve(
    const sheet&           _sheet,
    const colour_settings& _look
)
{
    resolution out;
    resolver   pass(_sheet, _look, out);

    pass.run();

    return out;
}

c_constant
wide_constant(
    const resolved_row& _row
)
{
    c_constant out;

    if ( (_row.kind != row_kind::symbol) ||
         (!_row.has_value) )
    {
        out.error = "not a symbol with a value";

        return out;
    }

    switch (_row.def)
    {
        case def_kind::bit:
        case def_kind::range:
            out.ok    = true;
            out.value = static_cast<u64>(_row.span_lsb);
            break;

        // the bits its field holds, as the field's _GET reads them: a
        // negative number in two's complement, say, is its field's pattern
        case def_kind::field_value:
            out.ok    = true;
            out.value = _row.bits.field(_row.span_lsb, _row.span_width);
            break;

        case def_kind::literal:
            if ( (!_row.negative) &&
                 (_row.bits.highest() >= INTEGER_WIDTH) )
            {
                out.error = "wider than any integer";
            }
            else
            {
                out.ok       = true;
                out.value    = _row.bits.low();
                out.negative = _row.negative;
            }

            break;

        case def_kind::macro:
            out.ok       = true;
            out.value    = _row.value;
            out.negative = _row.negative;
            break;

        case def_kind::expression:
            out.error = "a set of bits, which no C integer holds";
            break;
    }

    return out;
}

int
find_name(
    const resolution&  _resolution,
    const std::string& _name,
    int                _from,
    std::string&       _error
)
{
    const auto exact = _resolution.by_full.find(_name);

    if (exact != _resolution.by_full.end())
    {
        return exact->second;
    }

    const resolved_row* from    = _resolution.find(_from);
    const int           parent  = (from != nullptr) ? from->parent : 0;
    int                 single  = 0;
    int                 sibling = 0;
    int                 count   = 0;

    for (const resolved_row& each : _resolution.rows)
    {
        if ( (each.kind != row_kind::symbol) ||
             (each.local != _name) )
        {
            continue;
        }

        ++count;
        single = each.id;

        if (each.parent == parent)
        {
            sibling = each.id;
        }
    }

    if (count == 1)
    {
        return single;
    }

    if (count > 1)
    {
        if (sibling != 0)
        {
            return sibling;
        }

        _error = "'" + _name + "' is ambiguous: " +
                 format_dec(static_cast<u64>(count)) +
                 " symbols have that name; use the macro name";

        return 0;
    }

    _error = "unknown name '" + _name + "'";

    return 0;
}

std::string
macro_body(
    const resolution&         _resolution,
    const row&                _macro,
    std::vector<std::string>* _unknown
)
{
    const std::string& body = _macro.body;
    std::string        out;
    std::size_t        at   = 0;
    c_macro            shape;
    std::string        ignored;

    (void)read_params(_macro.params, shape, ignored);

    while (at < body.size())
    {
        const std::size_t open  = body.find('{', at);
        const std::size_t close = (open == std::string::npos)
                                      ? std::string::npos
                                      : body.find('}', open);

        if (close == std::string::npos)
        {
            out += body.substr(at);

            break;
        }

        const std::string inside = body.substr(open + 1, close - open - 1);

        out += body.substr(at, open - at);
        at   = close + 1;

        // {NAME} around a name that is not a parameter
        const bool param = (std::find(shape.params.begin(),
                                      shape.params.end(),
                                      inside) != shape.params.end());
        std::string error;
        const int   id = ( (!param) &&
                           (is_identifier(inside)) )
                             ? find_name(_resolution, inside, _macro.id, error)
                             : 0;

        if (id != 0)
        {
            out += _resolution.find(id)->full;

            continue;
        }

        if ( (_unknown != nullptr) &&
             (!param) &&
             (is_identifier(inside)) )
        {
            _unknown->push_back(inside);
        }

        // C's own braces, or a name that is no symbol: as written
        out += body.substr(open, close - open + 1);
    }

    return out;
}

bool
is_identifier(
    const std::string& _name
)
{
    if ( (_name.empty()) ||
         ( (!std::isalpha(static_cast<unsigned char>(_name[0]))) &&
           (_name[0] != '_') ) )
    {
        return false;
    }

    for (char each : _name)
    {
        if ( (!std::isalnum(static_cast<unsigned char>(each))) &&
             (each != '_') )
        {
            return false;
        }
    }

    return true;
}

bool
is_reserved(
    const std::string& _name
)
{
    const bool capital =
        ( (_name.size() >= 2) &&
          (_name[0] == '_')   &&
          (std::isupper(static_cast<unsigned char>(_name[1]))) );

    return ( (capital) ||
             (_name.find("__") != std::string::npos) );
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
