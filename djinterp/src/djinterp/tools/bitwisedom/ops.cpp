/*******************************************************************************
* bitwisedom [bitwisedom]                                                ops.cpp
*
* Operations on symbols: the arithmetic, the bit flow, and the rewrites.
*   preview_op and apply_op_to share one view of a target: its value before,
* its value after, and what applying makes of its definition. The preview
* describes the rewrite in words; applying performs it. Both read values
* from the resolution taken before the operation, so a target's result never
* depends on another target already having been rewritten.
*
* path:      /src/djinterp/tools/bitwisedom/ops.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
#include <djinterp/tools/bitwisedom/ops.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::find, std::min
#include <cstddef>    // std::size_t
#include <functional> // std::function
#include <utility>    // std::move, std::pair


namespace bitwisedom
{

namespace
{

// collect_symbols
//   the symbols a set of rows stands for: a symbol itself, a group every
// symbol in it; each once, in the order first met.
std::vector<int>
collect_symbols(
    const sheet&            _sheet,
    const std::vector<int>& _targets
)
{
    std::vector<int> ids;

    // add
    //   appends one row's symbols.
    std::function<void(const row&)> add = [&ids, &add](const row& _row)
    {
        if (_row.kind == row_kind::symbol)
        {
            if (std::find(ids.begin(), ids.end(), _row.id) == ids.end())
            {
                ids.push_back(_row.id);
            }

            return;
        }

        for (const row& child : _row.children)
        {
            add(child);
        }

        return;
    };

    for (int id : _targets)
    {
        const row* found = find_row(_sheet, id);

        if (found != nullptr)
        {
            add(*found);
        }
    }

    return ids;
}

// operable
//   whether an operation can act on a resolved row: a symbol with a value.
bool
operable(
    const resolved_row* _row
)
{
    return ( (_row != nullptr)                  &&
             (_row->kind == row_kind::symbol)   &&
             (_row->def != def_kind::macro)     &&
             (_row->has_value) );
}

// hex
//   a value as 0x-prefixed hexadecimal, for messages and expressions.
std::string
hex(
    const wide& _value
)
{
    return "0x" + format_hex(_value, 0, true);
}

// count_of
//   an operand as a count, held to the width: a count past it -- or past
// any integer -- is the width.
int
count_of(
    const wide& _operand,
    int         _width
)
{
    if (_operand.highest() >= INTEGER_WIDTH)
    {
        return _width;
    }

    return (_operand.low() >= static_cast<u64>(_width))
               ? _width
               : static_cast<int>(_operand.low());
}

// turn_of
//   an operand as a rotation: its integer, round the width.
int
turn_of(
    const wide& _operand,
    int         _width
)
{
    return (_width <= 0) ? 0
                         : static_cast<int>(_operand.low() %
                                            static_cast<u64>(_width));
}

// span
//   "bit n" or "bits a..b".
std::string
span(
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

// wrapped
//   an expression with the operation applied in C, or the empty string
// for an operation C has no operator for.
std::string
wrapped(
    const std::string& _expr,
    op_kind            _op,
    const wide&        _operand
)
{
    const std::string inner = "(" + _expr + ")";
    const std::string count = format_dec(_operand.low());

    // a count or a sum past an integer is nothing C's operators write
    if ( (_operand.highest() >= INTEGER_WIDTH) &&
         (_op != op_kind::bit_and)             &&
         (_op != op_kind::bit_or)              &&
         (_op != op_kind::bit_xor)             &&
         (_op != op_kind::and_not) )
    {
        return std::string();
    }

    switch (_op)
    {
        case op_kind::shift_left:  return inner + " << " + count;
        case op_kind::shift_right: return inner + " >> " + count;
        case op_kind::complement:  return "~" + inner;
        case op_kind::bit_and:     return inner + " & " + hex(_operand);
        case op_kind::bit_or:      return inner + " | " + hex(_operand);
        case op_kind::bit_xor:     return inner + " ^ " + hex(_operand);
        case op_kind::and_not:     return inner + " & ~" + hex(_operand);
        case op_kind::add:         return inner + " + " + count;
        case op_kind::subtract:    return inner + " - " + count;
        case op_kind::multiply:    return inner + " * " + count;
        case op_kind::negate:      return "-" + inner;
        case op_kind::set_bit:
            return inner + " | BIT(" + count + ")";
        case op_kind::clear_bit:
            return inner + " & ~BIT(" + count + ")";
        case op_kind::toggle_bit:
            return inner + " ^ BIT(" + count + ")";
        default:
            return std::string();
    }
}

// field_mask
//   the bits a resolved field_value's field covers.
wide
field_mask(
    const resolved_row& _row
)
{
    return wide::run(_row.span_lsb, _row.span_width);
}

// describe_change
//   in words, what applying the operation made of one definition: _before
// is the row as it was, _after as the operation left it.
std::string
describe_change(
    const row&          _before,
    const row&          _after,
    const resolved_row& _resolved,
    const wide&         _old,
    const wide&         _new
)
{
    if (_old == _new)
    {
        return "unchanged";
    }

    // a definition that could not keep its kind became a literal
    if (_after.def != _before.def)
    {
        return "becomes the literal " + hex(_after.literal);
    }

    switch (_after.def)
    {
        case def_kind::bit:
            return span(_before.bit, 1) + " -> " + span(_after.bit, 1);

        case def_kind::range:
            return span(_before.lsb, _before.width) + " -> " +
                   span(_after.lsb, _after.width);

        case def_kind::field_value:
            return format_signed(_resolved.number) + " -> " +
                   format_signed(_after.number);

        case def_kind::literal:
            return hex(_old) + " -> " + hex(_new);

        case def_kind::expression:
            return (_after.expr == _before.expr)
                       ? std::string("follows its operands")
                       : "becomes " + _after.expr;

        case def_kind::macro:
            break;
    }

    return "unchanged";
}

// to_literal
//   turns a symbol into a literal of the given bits.
void
to_literal(
    row&        _row,
    const wide& _bits
)
{
    _row.def      = def_kind::literal;
    _row.literal  = _bits;
    _row.negative = false;

    return;
}

// rewrite
//   applies the operation to one definition, keeping its kind where the
// result still fits it.
void
rewrite(
    row&                _row,
    const resolved_row& _resolved,
    op_kind             _op,
    const wide&         _operand,
    const wide&         _after
)
{
    switch (_row.def)
    {
        case def_kind::bit:
            if (_after.count() == 1)
            {
                _row.bit = _after.lowest();
            }
            else
            {
                to_literal(_row, _after);
            }

            break;

        case def_kind::range:
        {
            int lsb   = 0;
            int width = 0;

            if (is_run(_after, lsb, width))
            {
                _row.lsb   = lsb;
                _row.width = width;
            }
            else
            {
                to_literal(_row, _after);
            }

            break;
        }

        case def_kind::field_value:
            if ((_after & complement(field_mask(_resolved),
                                     MAX_WIDTH)) == wide())
            {
                const u64 bits = _after.field(_resolved.span_lsb,
                                              _resolved.span_width);

                _row.number = decode(_row.enc,
                                     _resolved.span_width,
                                     _row.bias,
                                     bits);
            }
            else
            {
                to_literal(_row, _after);
            }

            break;

        case def_kind::literal:
            _row.literal  = _after;
            _row.negative = false;
            break;

        case def_kind::expression:
        {
            const std::string text = wrapped(_row.expr, _op, _operand);

            if (text.empty())
            {
                to_literal(_row, _after);
            }
            else
            {
                _row.expr = text;
            }

            break;
        }

        case def_kind::macro:
            break;
    }

    return;
}

// count_usage
//   how many of the sheet's allocations claim each bit, with some values
// replaced; and which bits collide within an exclusive group.
void
count_usage(
    const sheet&             _sheet,
    const resolution&        _resolution,
    const std::vector<int>&  _ids,
    const std::vector<wide>& _values,
    std::vector<int>&        _usage,
    wide&                    _collisions
)
{
    _usage.assign(static_cast<std::size_t>(_sheet.width), 0);
    _collisions = wide();

    std::vector<const resolved_row*> allocations;
    std::vector<wide>                values;

    for (const resolved_row& each : _resolution.rows)
    {
        if ( (each.kind != row_kind::symbol) ||
             (!each.has_value)               ||
             ( (each.def != def_kind::bit) &&
               (each.def != def_kind::range) ) )
        {
            continue;
        }

        wide value = each.bits;

        // a target counts with its value after the operation
        for (std::size_t i = 0; i < _ids.size(); ++i)
        {
            if (_ids[i] == each.id)
            {
                value = _values[i];
            }
        }

        for (int b = value.next(0);
             (b >= 0) && (b < _sheet.width);
             b = value.next(b + 1))
        {
            ++_usage[static_cast<std::size_t>(b)];
        }

        allocations.push_back(&each);
        values.push_back(std::move(value));
    }

    for (std::size_t i = 0; i < allocations.size(); ++i)
    {
        const row* group = find_row(_sheet, allocations[i]->parent);

        if ( (group != nullptr) &&
             (!group->exclusive) )
        {
            continue;
        }

        for (std::size_t j = i + 1; j < allocations.size(); ++j)
        {
            if (allocations[j]->parent == allocations[i]->parent)
            {
                const wide shared = values[i] & values[j];
                // A wholly enclosed range is a legal allocation, even
                // while previewing an edit to the enclosing bit span.
                if (((allocations[i]->def == def_kind::range) ||
                     (allocations[j]->def == def_kind::range)) &&
                    ((shared == values[i]) || (shared == values[j])))
                {
                    continue;
                }
                _collisions |= shared;
            }
        }
    }

    return;
}

}  // anonymous namespace


op_info
describe(
    op_kind _op
)
{
    switch (_op)
    {
        case op_kind::shift_left:   return { "shift left", "<<", "count",
                                             true };
        case op_kind::shift_right:  return { "shift right", ">>", "count",
                                             true };
        case op_kind::shift_arith:  return { "arithmetic shift right", "",
                                             "count", true };
        case op_kind::rotate_left:  return { "rotate left", "", "count", true };
        case op_kind::rotate_right: return { "rotate right", "", "count",
                                             true };
        case op_kind::complement:   return { "complement", "~", "", false };
        case op_kind::bit_and:      return { "and", "&", "value", false };
        case op_kind::bit_or:       return { "or", "|", "value", false };
        case op_kind::bit_xor:      return { "xor", "^", "value", false };
        case op_kind::and_not:      return { "and not", "& ~", "value", false };
        case op_kind::add:          return { "add", "+", "value", false };
        case op_kind::subtract:     return { "subtract", "-", "value", false };
        case op_kind::multiply:     return { "multiply", "*", "value", false };
        case op_kind::negate:       return { "negate", "-", "", false };
        case op_kind::swap_bytes:   return { "swap bytes", "", "", true };
        case op_kind::reverse:      return { "reverse bits", "", "", true };
        case op_kind::set_bit:      return { "set bit", "| BIT", "bit", false };
        case op_kind::clear_bit:    return { "clear bit", "& ~BIT", "bit",
                                             false };
        case op_kind::toggle_bit:   return { "toggle bit", "^ BIT", "bit",
                                             false };
    }

    return {};
}

/*
apply_op
  One value, within the width. Over 64 bits or fewer, it does exactly what
C's unsigned arithmetic does masked to the width; the u64 form below is
this one, so the two cannot disagree.
*/
wide
apply_op(
    op_kind     _op,
    const wide& _value,
    const wide& _operand,
    int         _width
)
{
    wide value = _value;

    value.trim(_width);

    const int  count = count_of(_operand, _width);
    const int  turn  = turn_of(_operand, _width);
    wide       out;

    switch (_op)
    {
        case op_kind::shift_left:
            out = (count >= _width) ? wide()
                                    : shift_left(value, count, _width);
            break;

        case op_kind::shift_right:
            out = (count >= _width) ? wide()
                                    : shift_right(value, count);
            break;

        case op_kind::shift_arith:
        {
            const bool sign = ( (_width > 0) &&
                                (value.test(_width - 1)) );

            if (count >= _width)
            {
                out = sign ? wide::run(0, _width) : wide();
            }
            else
            {
                out = shift_right(value, count);

                if (sign)
                {
                    out |= wide::run(_width - count, count);
                }
            }

            break;
        }

        case op_kind::rotate_left:
            out = rotate_left(value, turn, _width);
            break;

        case op_kind::rotate_right:
            out = rotate_right(value, turn, _width);
            break;

        case op_kind::complement: out = complement(value, _width);  break;
        case op_kind::bit_and:    out = value & _operand;           break;
        case op_kind::bit_or:     out = value | _operand;           break;
        case op_kind::bit_xor:    out = value ^ _operand;           break;

        case op_kind::and_not:
            out = value & complement(_operand, _width);
            break;

        case op_kind::add:
            out = add(value, _operand, _width);
            break;

        case op_kind::subtract:
            out = subtract(value, _operand, _width);
            break;

        case op_kind::multiply:
            out = multiply(value, _operand.low(), _width);
            break;

        case op_kind::negate:     out = negate(value, _width);        break;
        case op_kind::swap_bytes: out = reverse_bytes(value, _width); break;
        case op_kind::reverse:    out = reverse_bits(value, _width);  break;

        case op_kind::set_bit:
            out = value;

            if (count < _width)
            {
                out.set(count, true);
            }

            break;

        case op_kind::clear_bit:
            out = value;

            if (count < _width)
            {
                out.set(count, false);
            }

            break;

        case op_kind::toggle_bit:
            out = value;

            if (count < _width)
            {
                out.set(count, !value.test(count));
            }

            break;
    }

    out.trim(_width);

    return out;
}

u64
apply_op(
    op_kind _op,
    u64     _value,
    u64     _operand,
    int     _width
)
{
    const int width = std::min(_width, INTEGER_WIDTH);

    return apply_op(_op, wide(_value), wide(_operand), width).low();
}

std::vector<int>
bit_flow(
    op_kind _op,
    u64     _operand,
    int     _width
)
{
    std::vector<int> flow;

    if ( (!describe(_op).permutes) ||
         (_width <= 0) )
    {
        return flow;
    }

    const int width = _width;
    const int count = (_operand >= static_cast<u64>(width))
                          ? width
                          : static_cast<int>(_operand);
    const int turn  = static_cast<int>(_operand % static_cast<u64>(width));
    const int bytes = (width + 7) / 8;

    flow.assign(static_cast<std::size_t>(width), -1);

    for (int i = 0; i < width; ++i)
    {
        int source = -1;

        switch (_op)
        {
            case op_kind::shift_left:
                source = (i >= count) ? (i - count) : -1;
                break;

            case op_kind::shift_right:
                source = (i + count < width) ? (i + count) : -1;
                break;

            case op_kind::shift_arith:
                source = (i + count < width) ? (i + count) : (width - 1);
                break;

            case op_kind::rotate_left:
                source = (i - turn + width) % width;
                break;

            case op_kind::rotate_right:
                source = (i + turn) % width;
                break;

            case op_kind::swap_bytes:
                source = (bytes - 1 - i / 8) * 8 + i % 8;
                source = (source < width) ? source : -1;
                break;

            case op_kind::reverse:
                source = width - 1 - i;
                break;

            default:
                break;
        }

        flow[static_cast<std::size_t>(i)] = source;
    }

    return flow;
}

op_preview
preview_op(
    const sheet&            _sheet,
    const resolution&       _resolution,
    const std::vector<int>& _targets,
    op_kind                 _op,
    const wide&             _operand,
    bool                    _probe
)
{
    op_preview preview;
    preview.op      = _op;
    preview.operand = _operand;
    preview.probe   = _probe;
    preview.flow    = bit_flow(_op,
                               static_cast<u64>(count_of(_operand,
                                                         _sheet.width)),
                               _sheet.width);

    // which source bits the flow carries anywhere; the rest fall off
    wide carried;

    for (int source : preview.flow)
    {
        if (source >= 0)
        {
            carried.set(source, true);
        }
    }

    const bool        moves = describe(_op).permutes;
    std::vector<int>  ids;
    std::vector<wide> afters;

    for (int id : collect_symbols(_sheet, _targets))
    {
        const resolved_row* resolved = _resolution.find(id);
        const row*          source   = find_row(_sheet, id);

        if ( (!operable(resolved)) ||
             (source == nullptr) )
        {
            continue;
        }

        op_row made;
        made.id     = id;
        made.before = resolved->bits;
        made.after  = apply_op(_op, made.before, _operand, _sheet.width);

        if (moves)
        {
            made.lost   = (made.before &
                           complement(carried, _sheet.width)).count();
            made.gained = made.after.count() -
                          (made.before.count() - made.lost);
            made.gained = (made.gained > 0) ? made.gained : 0;
        }
        else
        {
            made.lost   = (made.before &
                           complement(made.after, _sheet.width)).count();
            made.gained = (made.after &
                           complement(made.before, _sheet.width)).count();
        }

        ids.push_back(id);
        afters.push_back(made.after);
        preview.rows.push_back(made);
    }

    if (_probe)
    {
        preview.probe_before = _sheet.probe;
        preview.probe_before.trim(_sheet.width);
        preview.probe_after  = apply_op(_op,
                                        preview.probe_before,
                                        _operand,
                                        _sheet.width);
    }

    // the rewrites are described from a rehearsal on a copy, so the words
    // say what applying will actually do
    sheet rehearsal = _sheet;

    (void)apply_op_to(rehearsal,
                      _resolution,
                      _targets,
                      _op,
                      _operand,
                      false);

    for (op_row& made : preview.rows)
    {
        const row*          before   = find_row(_sheet, made.id);
        const row*          after    = find_row(rehearsal, made.id);
        const resolved_row* resolved = _resolution.find(made.id);

        made.rewrite = describe_change(*before,
                                       *after,
                                       *resolved,
                                       made.before,
                                       made.after);
    }

    preview.usage_before      = _resolution.usage;
    preview.collisions_before = _resolution.collisions;

    count_usage(_sheet,
                _resolution,
                ids,
                afters,
                preview.usage_after,
                preview.collisions_after);

    return preview;
}

/*
apply_op_to
  Expressions are rewritten last, after everything they might refer to. An
expression whose operands were moved too may already have the value the
operation was to give it, and is left as written; otherwise it is wrapped in
the operation, if C has an operator for it and the wrapped form evaluates to
that value; failing both, it becomes the literal. Either way every target
ends with exactly the value its preview showed.
*/
int
apply_op_to(
    sheet&                  _sheet,
    const resolution&       _resolution,
    const std::vector<int>& _targets,
    op_kind                 _op,
    const wide&             _operand,
    bool                    _probe
)
{
    std::vector<std::pair<int, wide>> deferred;
    int                               changed = 0;

    for (int id : collect_symbols(_sheet, _targets))
    {
        const resolved_row* resolved = _resolution.find(id);
        row*                target   = find_row(_sheet, id);

        if ( (!operable(resolved)) ||
             (target == nullptr) )
        {
            continue;
        }

        const wide after = apply_op(_op,
                                    resolved->bits,
                                    _operand,
                                    _sheet.width);

        // a symbol the operation leaves alone is not rewritten
        if (after == resolved->bits)
        {
            continue;
        }

        if (target->def == def_kind::expression)
        {
            deferred.emplace_back(id, after);

            continue;
        }

        rewrite(*target, *resolved, _op, _operand, after);
        ++changed;
    }

    if (!deferred.empty())
    {
        const resolution moved = resolve(_sheet);

        for (const std::pair<int, wide>& each : deferred)
        {
            row*                target = find_row(_sheet, each.first);
            const resolved_row* now    = moved.find(each.first);

            ++changed;

            // its operands moved, and it moved with them
            if ( (now != nullptr)     &&
                 (now->has_value)     &&
                 (now->bits == each.second) )
            {
                continue;
            }

            const std::string original = target->expr;
            const std::string text     = wrapped(original, _op, _operand);

            if (!text.empty())
            {
                target->expr = text;

                const resolution  check = resolve(_sheet);
                const resolved_row* after = check.find(each.first);

                if ( (after != nullptr)    &&
                     (after->has_value)    &&
                     (after->bits == each.second) )
                {
                    continue;
                }

                target->expr = original;
            }

            to_literal(*target, each.second);
        }
    }

    if (_probe)
    {
        wide before = _sheet.probe;

        before.trim(_sheet.width);

        const wide after = apply_op(_op, before, _operand, _sheet.width);

        if (after != before)
        {
            ++changed;
        }

        _sheet.probe = after;
    }

    return changed;
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
