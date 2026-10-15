/*******************************************************************************
* bitwisedom [tests]                                             model_tests.cpp
*
* The model's suite: bits, expressions, sheets, resolution, operations, the
* generator, files and history, and the C environment and importer.
*   A suite is a table of sections, as djinterp's are: each section a
* function that checks one thing and says which check failed, the runner a
* loop over the table that prints each verdict and the totals. The count is
* the same at every level, and a run that prints another count fails in
* CTest.
*
* path:      /tests/djinterp/tools/bitwisedom/model_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
// std
#include <cstdio>   // std::printf
#include <string>   // std::string
#include <vector>   // std::vector
// bitwisedom
#include <djinterp/tools/bitwisedom/bits.hpp>      // masks, encodings, parsing
#include <djinterp/tools/bitwisedom/cenv.hpp>      // c_env, run_directives
#include <djinterp/tools/bitwisedom/cimport.hpp>   // import_code, merge_rows
#include <djinterp/tools/bitwisedom/codegen.hpp>   // generate
#include <djinterp/tools/bitwisedom/examples.hpp>  // make_example
#include <djinterp/tools/bitwisedom/exchange.hpp>  // read_json_sheet, write_export
#include <djinterp/tools/bitwisedom/expr.hpp>      // parse_expression, evaluate
#include <djinterp/tools/bitwisedom/history.hpp>   // history
#include <djinterp/tools/bitwisedom/json.hpp>      // parse_json
#include <djinterp/tools/bitwisedom/ops.hpp>       // apply_op, preview_op
#include <djinterp/tools/bitwisedom/persist.hpp>   // save_sheet, load_sheet
#include <djinterp/tools/bitwisedom/rename.hpp>    // rename_row
#include <djinterp/tools/bitwisedom/resolve.hpp>   // resolve


namespace
{

using namespace ::bitwisedom;

// the code attached to the request of 2026.10.09
#include "./attachment.inc"

// failure
//   the first check that failed in the running section, or nullptr.
const char* failure = nullptr;

// expect
//   records the first failed check of a section.
void
expect(
    bool        _ok,
    const char* _what
)
{
    if ( (!_ok) &&
         (failure == nullptr) )
    {
        failure = _what;
    }

    return;
}

// symbol
//   a symbol row, for building sheets in the sections below.
row
symbol(
    const std::string& _name,
    def_kind           _def
)
{
    row made;
    made.name = _name;
    made.def  = _def;

    return made;
}

// group
//   a group row.
row
group(
    const std::string& _name,
    const std::string& _prefix
)
{
    row made;
    made.kind   = row_kind::group;
    made.name   = _name;
    made.prefix = _prefix;

    return made;
}

// value_of
//   a resolved row's value by macro name, or a sentinel.
u64
value_of(
    const resolution&  _resolution,
    const std::string& _full
)
{
    const auto found = _resolution.by_full.find(_full);

    if (found == _resolution.by_full.end())
    {
        return 0xDEADBEEFu;
    }

    return _resolution.find(found->second)->value;
}

// has_issue
//   whether any issue's message contains a phrase.
bool
has_issue(
    const resolution&  _resolution,
    severity           _level,
    const std::string& _phrase
)
{
    for (const issue& each : _resolution.issues)
    {
        if ( (each.level == _level) &&
             (each.message.find(_phrase) != std::string::npos) )
        {
            return true;
        }
    }

    return false;
}

// contains
//   whether text contains a phrase.
bool
contains(
    const std::string& _text,
    const std::string& _phrase
)
{
    return (_text.find(_phrase) != std::string::npos);
}

// longest_line
//   the length of a text's longest line.
std::size_t
longest_line(
    const std::string& _text
)
{
    std::size_t start   = 0;
    std::size_t longest = 0;

    while (start < _text.size())
    {
        std::size_t end = _text.find('\n', start);

        if (end == std::string::npos)
        {
            end = _text.size();
        }

        longest = (end - start > longest) ? (end - start) : longest;
        start   = end + 1;
    }

    return longest;
}

// c_eval
//   a text evaluated in a fresh environment, with some definitions first.
c_result
c_eval(
    const std::string& _text,
    standard           _level   = standard::c99,
    data_model         _model   = data_model::llp64,
    const std::string& _defines = std::string()
)
{
    c_settings settings;
    settings.level = _level;
    settings.model = _model;

    c_env                    env(settings);
    std::vector<std::string> notes;

    (void)run_directives(env, _defines, notes);

    return env.evaluate(_text, c_scope{});
}

// said
//   whether an evaluation said something of a level, with a phrase in it.
bool
said(
    const c_result&    _result,
    c_level            _level,
    const std::string& _phrase
)
{
    for (const c_diagnostic& each : _result.diagnostics)
    {
        if ( (each.level == _level) &&
             (contains(each.text(), _phrase)) )
        {
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------- sections

void
bits_masks_and_runs()
{
    int lsb   = 0;
    int width = 0;

    expect(width_mask(0) == 0, "width_mask(0)");
    expect(width_mask(5) == 0x1Fu, "width_mask(5)");
    expect(width_mask(64) == ~u64(0), "width_mask(64)");
    expect(range_mask(4, 3) == 0x70u, "range_mask(4, 3)");
    expect(range_mask(64, 1) == 0, "range_mask past the top");
    expect(is_run(0x70u, lsb, width) && (lsb == 4) && (width == 3),
           "0x70 is a run of 3 at 4");
    expect(!is_run(0x50u, lsb, width), "0x50 is not a run");
    expect(is_run(~u64(0) << 60, lsb, width) && (width == 4),
           "a run to the top");
    expect(bit_count(0xF0F0u) == 8, "bit_count");
    expect((lowest_set(0x80u) == 7) && (highest_set(0x80u) == 7),
           "lowest and highest set");

    return;
}

void
bits_movement()
{
    expect(rotate_left(0x8u, 1, 4) == 0x1u, "rotate_left wraps in 4 bits");
    expect(rotate_right(0x1u, 1, 4) == 0x8u, "rotate_right wraps in 4");
    expect(rotate_left(0x1u, 9, 8) == 0x2u, "rotate by more than width");
    expect(reverse_bits(0x1u, 4) == 0x8u, "reverse_bits in 4");
    expect(reverse_bytes(0x1234u, 16) == 0x3412u, "reverse_bytes in 16");
    expect(reverse_bytes(0x123u, 12) == 0x301u,
           "reverse_bytes of a part-byte width");
    expect(sign_extend(0xFu, 4) == -1, "sign_extend -1");
    expect(sign_extend(0x7u, 4) == 7, "sign_extend +7");
    expect(sign_extend(u64(1) << 63, 64) == (-9223372036854775807LL - 1),
           "sign_extend the most negative");

    return;
}

void
bits_encodings()
{
    for (int e = 0; e < ENCODING_COUNT; ++e)
    {
        const encoding which = static_cast<encoding>(e);

        for (int width = 1; width <= 8; ++width)
        {
            const i64         bias  = default_bias(width);
            const value_range range = range_of(which, width, bias);

            for (i64 v = range.low; v <= range.high; ++v)
            {
                u64 bits = 0;

                expect(encode(which, width, bias, v, bits),
                       "every number in range fits");
                expect(decode(which, width, bias, bits) == v,
                       "decode undoes encode");
            }

            u64 bits = 0;
            expect(!encode(which, width, bias, range.high + 1, bits),
                   "one past the top does not fit");
        }
    }

    u64 bits = 0;
    (void)encode(encoding::twos_complement, 4, 0, -1, bits);
    expect(bits == 0xFu, "two's complement -1 in 4 bits");
    (void)encode(encoding::ones_complement, 4, 0, -3, bits);
    expect(bits == 0xCu, "ones' complement -3 in 4 bits");
    (void)encode(encoding::sign_magnitude, 4, 0, -3, bits);
    expect(bits == 0xBu, "sign-magnitude -3 in 4 bits");
    (void)encode(encoding::offset_binary, 4, 8, -3, bits);
    expect(bits == 0x5u, "offset binary -3 in 4 bits, bias 8");

    return;
}

void
bits_parsing()
{
    expect(parse_integer("0x1F").value == 31, "hex");
    expect(parse_integer("0b101").value == 5, "binary");
    expect(parse_integer("017").value == 15, "a leading zero is octal");
    expect(parse_integer("0o17").value == 15, "0o is octal");
    expect(parse_integer(" 1'000 ").value == 1000, "' separators");
    expect(parse_integer("1_000u").value == 1000, "_ and a suffix");
    expect(parse_integer("0xFFull").value == 255, "ull");
    expect(parse_integer("-1").negative &&
               (parse_integer("-1").value == ~u64(0)),
           "negative is two's complement");
    expect(parse_integer("0xFFFFFFFFFFFFFFFF").value == ~u64(0),
           "64 bits");
    expect(!parse_integer("0x10000000000000000").ok, "65 bits overflow");
    expect(!parse_integer("08").ok, "8 is not octal");
    expect(!parse_integer("").ok, "no digits");
    expect(!parse_integer("12abc").ok, "a stray suffix");
    expect(format_bin(0x5u, 8, 4, '_') == "0000_0101", "format_bin");
    expect(format_hex(0xABu, 4, true) == "00AB", "format_hex");
    expect(group_digits("", 2).empty(), "group empty");
    expect(group_digits("A", 2) == "A", "group single digit");
    expect(group_digits("ABC", 2) == "A BC", "hex uneven pair");
    expect(group_digits("0000ABCD", 2) == "00 00 AB CD",
           "hex padded pairs");
    expect(group_digits(format_oct(012345670u), 3) == "12 345 670",
           "octal three-digit groups");
    expect(group_digits("1234567", 3) == "1 234 567",
           "octal uneven left group");
    expect(format_bin(0x1234u, 16, 8, ' ') == "00010010 00110100",
           "binary eight-digit groups");
    expect(format_bin(0x1234u, 13, 8, ' ') == "10010 00110100",
           "binary partial leading group");
    expect(group_digits("12345678", 0) == "12345678",
           "group disabled");
    expect(format_signed(-42) == "-42", "format_signed");

    return;
}

/*
bits_wide
  Wide values at the seams between their words, and against the u64
functions at every width they share: whatever moves or adds bits in 64 or
fewer must do the same to a wide value. Then what only a wide value can
do: rotate and shift past 64 bits, and read numbers of any width.
*/
void
bits_wide()
{
    // the seams
    wide v = wide::bit(63) | wide::bit(64) | wide::bit(130);

    expect( (v.words() == 3) &&
            (v.count() == 3) &&
            (v.lowest() == 63) &&
            (v.highest() == 130) &&
            (v.next(64) == 64) &&
            (v.next(65) == 130) &&
            (v.next(131) == -1),
           "bits at the seams");
    expect(wide::run(60, 8).field(58, 12) == 0x3FCu,
           "a field read across a seam");

    wide w;
    w.put(60, 10, 0x3FFu);
    expect(w == wide::run(60, 10), "a field written across a seam");
    w.put(60, 10, 0);
    expect( (!w.any()) && (w.words() == 0),
           "and cleared, with no word left over");
    expect(wide::run(0, 64) == wide(~u64(0)), "a whole word");
    expect(wide::run(0, 200).count() == 200, "a long run");

    wide t = wide::run(0, 200);
    t.trim(130);
    expect( (t.count() == 130) && (t.highest() == 129), "trim");
    expect(wide::run(5, 1).slice(4, 3) == wide(0x2u), "slice");

    // against the u64 functions, at every width they share
    const u64 samples[] = { 0x0u, 0x1u, 0x8000000000000000u,
                            0x0123456789ABCDEFu, 0xFFFFFFFFFFFFFFFFu,
                            0x00F0F0F0F0F0F0F0u };
    bool same = true;

    for (int width = 1; width <= 64; ++width)
    {
        for (u64 raw : samples)
        {
            const u64  value = raw & width_mask(width);
            const wide big(value);

            for (int n : { 0, 1, 3, 8, 63, 64, 65 })
            {
                same = ( (same) &&
                         (rotate_left(big, n, width) ==
                          wide(rotate_left(value, n, width))) &&
                         (rotate_right(big, n, width) ==
                          wide(rotate_right(value, n, width))) );
            }

            same = ( (same) &&
                     (reverse_bits(big, width) ==
                      wide(reverse_bits(value, width))) &&
                     (reverse_bytes(big, width) ==
                      wide(reverse_bytes(value, width))) &&
                     (complement(big, width) ==
                      wide(~value & width_mask(width))) &&
                     (negate(big, width) ==
                      wide((~value + 1u) & width_mask(width))) &&
                     (add(big, wide(0x1234u), width) ==
                      wide((value + 0x1234u) & width_mask(width))) &&
                     (subtract(big, wide(7u), width) ==
                      wide((value - 7u) & width_mask(width))) &&
                     (multiply(big, 10u, width) ==
                      wide((value * 10u) & width_mask(width))) &&
                     (shift_left(big, 3, width) ==
                      wide((value << 3) & width_mask(width))) &&
                     (shift_right(big, 3) == wide(value >> 3)) );
        }
    }

    expect(same, "as the u64 functions do, at every width to 64");

    // and past 64
    expect(rotate_left(wide::bit(199), 1, 200) == wide::bit(0),
           "a rotation round 200 bits");
    expect(rotate_right(wide::bit(0), 70, 200) == wide::bit(130),
           "a rotation past a word");
    expect(shift_left(wide::bit(10), 120, 200) == wide::bit(130),
           "a shift past two words");
    expect(shift_left(wide::bit(10), 190, 200) == wide(),
           "and out of the width");
    expect(shift_right(wide::bit(130), 67) == wide::bit(63),
           "a shift down across a seam");
    expect(add(wide(~u64(0)), wide(1u), 200) == wide::bit(64),
           "a carry into the next word");
    expect(negate(wide(1u), 128) == wide::run(0, 128), "-1 in 128 bits");
    expect(multiply(wide::bit(63), 4u, 200) == wide::bit(65),
           "a product past a word");
    expect(widen_signed(-2, 100) == (wide::run(1, 99)),
           "a negative number's 100 bits");
    expect(reverse_bytes(wide(0x1122u), 96) ==
               (wide::bit(80) | wide::bit(84) | wide::bit(89) |
                wide::bit(93)),
           "bytes reversed in 96 bits");

    int lsb   = 0;
    int width = 0;

    expect( (is_run(wide::run(60, 70), lsb, width)) &&
            (lsb == 60) &&
            (width == 70),
           "a run across words");
    expect(!is_run(wide::bit(3) | wide::bit(100), lsb, width), "not a run");

    // text
    expect(format_hex(wide::bit(68), 0, true) == "100000000000000000",
           "hex past 64 bits");
    expect(runs_text(wide::run(4, 4) | wide::bit(12) | wide::run(30, 2), 8) ==
               "bits 4..7, 12 and 30..31",
           "runs, as a person reads them");
    expect(runs_text(wide::bit(5), 8) == "bit 5", "one bit");
    expect(runs_text(wide(), 8) == "no bits", "no bits");
    expect(runs_text(wide(0x55u), 2) == "bits 0, 2 and 2 more runs",
           "runs past the most named");

    // reading numbers of any width
    expect(parse_wide("0x1" + std::string(32, '0')).value == wide::bit(128),
           "hex of 129 bits");
    expect(parse_wide("0b1" + std::string(99, '0')).value == wide::bit(99),
           "binary of 100 bits");
    expect(parse_wide("340282366920938463463374607431768211456").value ==
               wide::bit(128),
           "decimal of 129 bits");
    expect(parse_wide("0x7").value == wide(7u), "a small one");
    expect( (parse_wide("-1").negative) &&
            (parse_wide("-1").value == wide(~u64(0))),
           "a negative one, in 64 bits");
    expect(!parse_wide("-0x1" + std::string(20, '0')).ok,
           "a negative one past 64 bits is refused");
    expect(!parse_wide("0x1" + std::string(16384, '0')).ok,
           "past the widest sheet");
    expect(!parse_wide("0x").ok, "no digits");

    return;
}

// lookup
//   two names, A = 1 and B = 6, for the expression sections.
bool
lookup(
    const std::string& _name,
    u64&               _value,
    std::string&       _error
)
{
    if (_name == "A")
    {
        _value = 1;

        return true;
    }

    if (_name == "B")
    {
        _value = 6;

        return true;
    }

    _error = "unknown name '" + _name + "'";

    return false;
}

u64
eval(
    const std::string& _text
)
{
    const evaluation result = evaluate(parse_expression(_text), lookup);

    return result.ok ? result.value : 0xBADu;
}

void
expressions_evaluate()
{
    expect(eval("1 | 2 & 3") == 3, "& binds tighter than |");
    expect(eval("1 << 2 + 1") == 8, "+ binds tighter than <<");
    expect(eval("~0 & 0xF") == 15, "complement");
    expect(eval("-1") == ~u64(0), "negation wraps");
    expect(eval("(A | B) ^ 3") == 4, "names and parentheses");
    expect(eval("BIT(3)") == 8, "BIT");
    expect(eval("MASK(4)") == 15, "MASK");
    expect(eval("GENMASK(7, 4)") == 0xF0u, "GENMASK");
    expect(eval("10 / 3 * 3 + 10 % 3") == 10, "division and modulo");
    expect(eval("!0 + !5") == 1, "logical not");

    expect(!parse_expression("").ok(), "empty is an error");
    expect(!parse_expression("(1 | 2").ok(), "unclosed parenthesis");
    expect(!parse_expression("FOO(1)").ok(), "unknown function");
    expect(!parse_expression("BIT(1, 2)").ok(), "wrong argument count");
    expect(!evaluate(parse_expression("1 / 0"), lookup).ok, "x / 0");
    expect(!evaluate(parse_expression("C"), lookup).ok, "unknown name");
    expect(evaluate(parse_expression("1 << 64"), lookup).overshift,
           "an overshift is reported");

    return;
}

void
expressions_print()
{
    const expression parsed = parse_expression("A | B << 2 | GENMASK(3, 0)");
    const std::string text  = print_c(
        parsed,
        [](const std::string& _name) { return "D_" + _name; },
        [](u64 _value) { return "0x" + format_hex(_value, 0, true) + "u"; },
        lookup);

    expect(text == "(D_A | (D_B << 2) | 0xFu)",
           "names renamed, counts plain, calls folded, one chain");

    const expression minus = parse_expression("A - B - 1");
    const std::string left = print_c(
        minus,
        [](const std::string& _name) { return _name; },
        [](u64 _value) { return format_dec(_value); },
        lookup);

    expect(left == "(A - B - 1)", "a left-grouped subtraction flattens");

    const expression right = parse_expression("A - (B - 1)");
    const std::string kept = print_c(
        right,
        [](const std::string& _name) { return _name; },
        [](u64 _value) { return format_dec(_value); },
        lookup);

    expect(kept == "(A - (B - 1))", "a right-grouped one does not");

    return;
}

// small
//   a sheet exercising names, kinds and checks.
sheet
small()
{
    sheet s = make_sheet("small", 16);
    s.prefix = "D_";

    const int flags = insert_row(s, 0, -1, group("Flags", "F_"));

    row a = symbol("A", def_kind::bit);
    a.bit = 0;
    row b = symbol("B", def_kind::bit);
    b.bit = 0;   // overlaps A
    row mode = symbol("MODE", def_kind::range);
    mode.lsb   = 4;
    mode.width = 3;

    (void)insert_row(s, flags, -1, a);
    (void)insert_row(s, flags, -1, b);

    const int mode_id = insert_row(s, flags, -1, mode);

    row neg = symbol("NEG", def_kind::field_value);
    neg.field  = mode_id;
    neg.enc    = encoding::twos_complement;
    neg.number = -2;
    (void)insert_row(s, flags, -1, neg);

    row wide = symbol("WIDE", def_kind::field_value);
    wide.field  = mode_id;
    wide.number = 9;   // does not fit 3 bits
    (void)insert_row(s, flags, -1, wide);

    row lonely = group("Isolated", "I_");
    lonely.isolate = true;

    const int isolated = insert_row(s, flags, -1, lonely);

    row inner = symbol("IN", def_kind::literal);
    inner.literal = 0x1234u;
    (void)insert_row(s, isolated, -1, inner);

    row sum = symbol("SUM", def_kind::expression);
    sum.expr = "A | MODE";
    (void)insert_row(s, 0, -1, sum);

    row loop_a = symbol("LA", def_kind::expression);
    loop_a.expr = "LB + 1";
    row loop_b = symbol("LB", def_kind::expression);
    loop_b.expr = "LA + 1";
    (void)insert_row(s, 0, -1, loop_a);
    (void)insert_row(s, 0, -1, loop_b);

    row far = symbol("FAR", def_kind::bit);
    far.bit = 20;
    (void)insert_row(s, 0, -1, far);

    row bad = symbol("__BAD", def_kind::bit);
    bad.bit = 9;
    (void)insert_row(s, 0, -1, bad);

    return s;
}

void
resolve_names_and_values()
{
    const resolution r = resolve(small());

    expect(value_of(r, "D_F_A") == 0x1u, "the cascade names A D_F_A");
    expect(value_of(r, "D_F_MODE") == 0x70u, "MODE is bits 4..6");
    expect(value_of(r, "D_F_NEG") == (0x6u << 4),
           "-2 in three bits of two's complement is 110");
    expect(r.find(r.by_full.at("D_F_NEG"))->number == -2,
           "NEG decodes to -2");
    expect(value_of(r, "D_I_IN") == 0x1234u,
           "an isolated group drops the groups around it");
    expect(value_of(r, "D_SUM") == 0x71u, "SUM is A | MODE");
    expect(r.usage[0] == 2, "two allocations claim bit 0");
    expect(r.collisions == 0x1u, "bit 0 collides");

    // Group names can join the prefix cascade without changing sheets
    // that used groups only as organizational labels.
    sheet names = make_sheet("inheritance", 32);
    row foo = group("D_FOO_", "");
    foo.name_children = true;
    const int foo_id = insert_row(names, 0, -1, foo);
    row bar = group("BAR_", "");
    bar.name_children = true;
    const int bar_id = insert_row(names, foo_id, -1, bar);
    (void)insert_row(names, bar_id, -1,
                     symbol("EXAMPLE", def_kind::bit));
    (void)insert_row(names, foo_id, -1, symbol("BAZ", def_kind::bit));
    const resolution cascade = resolve(names);
    expect(cascade.by_full.count("D_FOO_BAR_EXAMPLE") == 1,
           "group names prefix grandchildren in ancestor order");
    expect(cascade.by_full.count("D_FOO_BAZ") == 1,
           "group names prefix direct children");
    expect(cascade.find(cascade.by_full.at("D_FOO_BAR_EXAMPLE"))->depth == 2,
           "nested descendants retain their tree depth for indentation");

    // Exact token lists: repeated boundary matches, both ends and
    // direction-specific trimming. Affixes are never trimmed.
    names.trim_both = "__";
    names.trim_left = "PRE_";
    names.trim_right = "_";
    find_row(names, foo_id)->name = "__D_FOO_";
    find_row(names, bar_id)->name = "PRE_BAR_";
    const int example_id = names.root.children[0].children[0].children[0].id;
    find_row(names, example_id)->name = "__EXAMPLE__";
    const resolution trimmed = resolve(names);
    expect(trimmed.by_full.count("D_FOOBAREXAMPLE") == 1,
           "the trim lists apply to each component before joining");

    sheet restored;
    std::string error;
    expect(load_sheet(save_sheet(names), restored, error),
           "naming settings survive a saved sheet");
    expect(restored.trim_both == names.trim_both &&
           restored.trim_left == names.trim_left &&
           restored.trim_right == names.trim_right &&
           find_row(restored, foo_id)->name_children &&
           find_row(restored, bar_id)->name_children,
           "trim lists and per-group inheritance survive roundtrip");
    expect(resolve(restored).by_full.count("D_FOOBAREXAMPLE") == 1,
           "roundtrip preserves effective descendant names");

    const json_read concise = read_json_sheet(write_bitfield(names));
    expect(concise.ok &&
           resolve(concise.result).by_full.count("D_FOOBAREXAMPLE") == 1,
           "bitfield export/import keeps inheritance and trimming");

    find_row(names, foo_id)->name_children = false;
    expect(resolve(names).by_full.count("BAREXAMPLE") == 1,
           "disabling ancestor inheritance leaves nested group optional");

    return;
}

void
nested_ranges_and_symbolic_defines()
{
    sheet s = make_document("nested", 16);
    expect(s.root.children.size() == 1, "new document contains root");
    expect(s.root.children.front().name == "root" &&
           s.root.children.front().def == def_kind::range &&
           s.root.children.front().lsb == 0 &&
           s.root.children.front().width == 16,
           "root spans every bit");
    const int root_id = s.root.children.front().id;
    row* root = find_row(s, root_id);
    root->name = "D_ERR_";
    root->name_children = true;
    root->transparent = true;

    row array = symbol("ARRAY_", def_kind::range);
    array.lsb = 4;
    array.width = 8;
    array.name_children = true;
    const int array_id = insert_row(s, root_id, -1, array);
    row index = symbol("INDEX_", def_kind::range);
    index.lsb = 5;
    index.width = 2;
    index.name_children = true;
    const int index_id = insert_row(s, array_id, -1, index);

    row code = group("D_CODE_", "");
    code.name_children = true;
    const int code_id = insert_row(s, 0, -1, code);
    row code_array = symbol("ARRAY_", def_kind::range);
    code_array.lsb = 4;
    code_array.width = 8;
    code_array.name_children = true;
    const int code_array_id = insert_row(s, code_id, -1, code_array);
    row field = symbol("FIELD", def_kind::range);
    field.lsb = 4;
    field.width = 8;
    (void)insert_row(s, code_array_id, -1, field);
    row low = symbol("IDX_LO", def_kind::bit);
    low.bit = 5;
    (void)insert_row(s, code_array_id, -1, low);
    row high = symbol("IDX_HI", def_kind::bit);
    high.bit = 6;
    (void)insert_row(s, code_array_id, -1, high);

    row oob = symbol("OOB", def_kind::expression);
    oob.expr = "D_CODE_ARRAY_FIELD | (D_CODE_ARRAY_IDX_LO | \\\n"
               "                             D_CODE_ARRAY_IDX_HI)";
    const int oob_id = insert_row(s, index_id, -1, oob);
    const resolution r = resolve(s);
    expect(r.errors == 0, "nested ranges and references resolve");
    expect(r.collisions == wide(), "contained ranges do not collide");
    expect(r.find(oob_id) != nullptr &&
           r.find(oob_id)->full == "D_ERR_ARRAY_INDEX_OOB" &&
           r.find(oob_id)->depth == 3,
           "three inherited range names produce full prefix and indent");
    expect(value_of(r, "D_ERR_ARRAY_INDEX_OOB") == 0xFF0u,
           "C preprocessor-style expression evaluates referenced symbols");
    expect(open_groups(s, index_id, false) > 0 &&
           find_row(s, index_id)->collapsed &&
           open_groups(s, index_id, true) > 0 &&
           !find_row(s, index_id)->collapsed,
           "range nodes with children can collapse and expand");
    expect(r.find(root_id)->colour == 0,
           "transparent overrides automatic palette");
    const generated g = generate(s, r, "2026.10.14");
    expect(contains(g.text, "#define D_ERR_ARRAY_INDEX_OOB"),
           "the nested expression is emitted as a preprocessor define");
    expect(contains(g.text, "D_CODE_ARRAY_IDX_HI"),
           "its named dependencies are present in generated C");

    sheet reloaded;
    std::string error;
    expect(load_sheet(save_sheet(s), reloaded, error),
           "native JSON roundtrips nested ranges");
    expect(find_row(reloaded, root_id)->transparent &&
           find_row(reloaded, index_id)->name_children &&
           find_row(reloaded, oob_id)->expr == oob.expr,
           "transparent, nested naming and expressions are persisted");
    const json_read concise = read_json_sheet(write_bitfield(s));
    expect(concise.ok &&
           resolve(concise.result).by_full.count("D_ERR_ARRAY_INDEX_OOB") == 1,
           "concise JSON roundtrips nested range names");
    row intermediary = group("HOLDER", "");
    const int intermediary_id = insert_row(s, index_id, -1, intermediary);
    expect(next_free_bit(s, intermediary_id) == 5,
           "new allocations respect range ancestors through groups");
    row outside = symbol("OUTSIDE", def_kind::bit);
    outside.bit = 15;
    (void)insert_row(s, intermediary_id, -1, outside);
    expect(has_issue(resolve(s), severity::error,
                     "allocation extends outside parent range"),
           "a child allocation cannot escape an ancestor range via groups");
}

void
resolve_issues()
{
    const resolution r = resolve(small());

    expect(has_issue(r, severity::warning, "shares bits 0x1 with B"),
           "the overlap is reported");
    expect(has_issue(r, severity::warning, "does not fit"),
           "a number too big for its field is reported");
    expect(has_issue(r, severity::error, "cycle: LA -> LB -> LA"),
           "the cycle is reported as its chain");
    expect(has_issue(r, severity::error, "outside the 16-bit width"),
           "a bit past the width is an error");
    expect(has_issue(r, severity::warning, "reserved"),
           "a reserved name is reported");
    expect(r.find(r.by_full.at("D_LA"))->error.size() > 0,
           "a row in a cycle has no value");

    sheet twice = small();
    row   again = symbol("SUM", def_kind::bit);
    (void)insert_row(twice, 0, -1, again);
    expect(has_issue(resolve(twice), severity::error, "defined twice"),
           "a name defined twice is an error");

    return;
}

void
tree_operations()
{
    sheet s = small();

    const int  flags = s.root.children[0].id;
    const int  sum   = s.root.children[1].id;
    const int  first = s.root.children[0].children[0].id;

    expect(move_row(s, sum, flags, 0), "a symbol moves into a group");
    expect(s.root.children[0].children[0].id == sum, "and lands first");
    expect(!move_row(s, flags, flags, 0), "a group cannot hold itself");
    expect(remove_row(s, first), "a row is removed");
    expect(find_row(s, first) == nullptr, "and is gone");
    expect(next_free_bit(s, flags) == 1,
           "B still holds bit 0, so bit 1 is the next free");

    // an index finds every row find_row does, and nothing else
    const row_index index(s);
    bool            agrees = (index.find(s.root.id) == &s.root);

    walk(s,
         [&index, &s, &agrees](const row& _row, const row&, int)
         {
             agrees = ( (agrees) &&
                        (index.find(_row.id) == find_row(s, _row.id)) &&
                        (index.find(_row.id) == &_row) );

             return;
         });

    expect(agrees, "an index finds each row, the root too");
    expect( (index.find(first) == nullptr) &&
            (index.find(-1) == nullptr) &&
            (index.find(s.next_id + 10) == nullptr),
           "and no row for a removed id, a negative one or one past them");

    return;
}

/*
groups_opened_and_closed
  Every group closed, from the root; one group opened with those within it,
and only those; a symbol and an unknown id changing nothing; and a second
call finding nothing left to change.
*/
void
groups_opened_and_closed()
{
    sheet s = small();

    const int flags    = s.root.children[0].id;
    const int isolated = s.root.children[0].children.back().id;
    const int sum      = s.root.children[1].id;

    // groups
    //   how many groups are closed, and how many open.
    auto groups = [&s](int& _closed, int& _open)
    {
        _closed = 0;
        _open   = 0;

        walk(s,
             [&_closed, &_open](const row& _row, const row&, int)
             {
                 if (_row.kind == row_kind::group)
                 {
                     _closed += _row.collapsed ? 1 : 0;
                     _open   += _row.collapsed ? 0 : 1;
                 }

                 return;
             });

        return;
    };

    int closed = 0;
    int open   = 0;

    expect(find_row(s, isolated)->kind == row_kind::group,
           "the sheet has a group within a group");
    expect(open_groups(s, s.root.id, false) == 2,
           "closing every group closes both");
    groups(closed, open);
    expect( (closed == 2) &&
            (open == 0) &&
            (!s.root.collapsed),
           "and leaves none open, the root as it was");
    expect(open_groups(s, s.root.id, false) == 0,
           "a second time, nothing is left to close");

    expect(open_groups(s, isolated, true) == 1,
           "a group opened alone");
    expect( (!find_row(s, isolated)->collapsed) &&
            (find_row(s, flags)->collapsed),
           "and not the group around it");
    expect(open_groups(s, flags, true) == 1,
           "the outer group, with its inner one already open");

    expect(open_groups(s, sum, false) == 0, "a symbol has no groups");
    expect(open_groups(s, 9999, false) == 0, "nor has an id no row has");

    expect(open_groups(s, s.root.id, false) == 2,
           "closing all again closes both");
    expect(open_groups(s, s.root.id, true) == 2, "and opening all, both");
    groups(closed, open);
    expect( (closed == 0) &&
            (open == 2),
           "and leaves every group open");

    return;
}

void
operations_values()
{
    expect(apply_op(op_kind::shift_left, 0x81u, 1, 8) == 0x02u,
           "shift left drops the top bit");
    expect(apply_op(op_kind::shift_arith, 0x80u, 2, 8) == 0xE0u,
           "an arithmetic shift copies the sign");
    expect(apply_op(op_kind::rotate_left, 0x81u, 1, 8) == 0x03u,
           "rotate left wraps");
    expect(apply_op(op_kind::negate, 0x01u, 0, 8) == 0xFFu, "negate");
    expect(apply_op(op_kind::swap_bytes, 0x1234u, 0, 16) == 0x3412u,
           "swap bytes");
    expect(apply_op(op_kind::clear_bit, 0xFFu, 3, 8) == 0xF7u, "clear bit");

    const std::vector<int> flow = bit_flow(op_kind::rotate_left, 1, 4);

    expect( (flow.size() == 4) && (flow[0] == 3) && (flow[1] == 0),
           "rotate left: bit 0 comes from bit 3");
    expect(bit_flow(op_kind::add, 1, 4).empty(),
           "add moves nothing, so has no flow");

    return;
}

void
operations_preview_and_apply()
{
    sheet            s = small();
    const resolution r = resolve(s);
    const int        a    = r.by_full.at("D_F_A");
    const int        mode = r.by_full.at("D_F_MODE");
    const int        sum  = r.by_full.at("D_SUM");

    const op_preview preview = preview_op(s,
                                          r,
                                          { a, mode, sum },
                                          op_kind::shift_left,
                                          1,
                                          true);

    expect(preview.rows.size() == 3, "three targets");
    expect(preview.rows[0].rewrite == "bit 0 -> bit 1", "a bit stays a bit");
    expect(preview.rows[1].rewrite == "bits 4..6 -> bits 5..7",
           "a range stays a range");
    expect(preview.rows[2].rewrite == "follows its operands",
           "an expression whose operands move moves with them");
    expect(preview.usage_after[1] == 1, "bit 1 is claimed after");
    expect(preview.collisions_after == 0, "and the collision is gone");

    const int changed = apply_op_to(s,
                                    r,
                                    { a, mode, sum },
                                    op_kind::shift_left,
                                    1,
                                    false);
    const resolution after = resolve(s);

    expect(changed == 3, "three rows changed");
    expect(value_of(after, "D_F_A") == 0x2u, "A is now bit 1");
    expect(value_of(after, "D_F_MODE") == 0xE0u, "MODE is now bits 5..7");
    expect(value_of(after, "D_SUM") == 0xE2u, "SUM follows its operands");
    expect(find_row(s, sum)->expr == "A | MODE", "and is written as before");

    // an expression shifted alone is wrapped in the shift
    sheet            alone  = small();
    const resolution aloner = resolve(alone);
    const int        lone   = aloner.by_full.at("D_SUM");
    (void)apply_op_to(alone, aloner, { lone }, op_kind::shift_left, 1, false);
    expect(find_row(alone, lone)->expr == "(A | MODE) << 1",
           "an expression alone is wrapped");
    expect(value_of(resolve(alone), "D_SUM") == 0xE2u,
           "and has the value its preview showed");

    // a range rotated across the top is no longer one run
    sheet            wrap  = small();
    const resolution wrapr = resolve(wrap);
    (void)apply_op_to(wrap,
                      wrapr,
                      { wrapr.by_full.at("D_F_MODE") },
                      op_kind::rotate_left,
                      11,
                      false);
    expect(find_row(wrap, wrapr.by_full.at("D_F_MODE"))->def ==
               def_kind::literal,
           "a range split by a rotation becomes a literal");

    return;
}

void
generator_macros()
{
    sheet            s = make_example("uart_cr");
    const resolution r = resolve(s);
    const generated  g = generate(s, r, "2026.10.04");

    expect(r.errors == 0, "the example has no errors");
    expect(contains(g.text, "#ifndef UART_CR_H\n#define UART_CR_H 1"),
           "the guard");
    expect(contains(g.text, "#define UART_CR_EN"), "a bit");
    expect(contains(g.text, "0x00000001u"), "a padded hex literal");
    expect(contains(g.text, "#define UART_CR_PARITY_GET(x)"),
           "a range's getter template");
    expect(contains(g.text, "(UART_CR_EN | UART_CR_TXEN | UART_CR_RXEN"),
           "an expression with macro names, as one list");
    expect(contains(g.text,
                    "#define UART_CR_IS_ENABLED(x) "),
           "a hand-written macro");
    expect(contains(g.text, "(((x) & UART_CR_EN) != 0)"),
           "its body's names replaced");
    expect(contains(g.text, "TABLE OF CONTENTS"), "a table of contents");
    expect(contains(g.text, "// 1.  ENABLES"), "a numbered section");
    expect(contains(g.text, "//   constant: enables the UART."),
           "a brief");
    expect(contains(g.text, "#endif  // UART_CR_H"), "the guard's end");

    // the generator keeps to the guide's 80 characters where it can
    std::size_t start   = 0;
    std::size_t longest = 0;

    while (start < g.text.size())
    {
        std::size_t end = g.text.find('\n', start);

        if (end == std::string::npos)
        {
            end = g.text.size();
        }

        longest = (end - start > longest) ? (end - start) : longest;
        start   = end + 1;
    }

    expect(longest <= 80, "no line past 80 characters");

    return;
}

void
generator_levels()
{
    sheet s = make_example("posix_mode");
    s.gen.base  = radix::bin;
    s.gen.level = standard::c99;

    generated g = generate(s, resolve(s), "2026.10.04");

    expect(contains(g.text, "0x"), "C99 has no binary literals");
    expect( (!g.notes.empty()) && contains(g.notes[0], "binary literals"),
           "and a note says so");

    s.gen.level = standard::c23;
    g = generate(s, resolve(s), "2026.10.04");
    expect(contains(g.text, "0b"), "C23 has them");

    s.gen.lang  = language::cpp;
    s.gen.level = standard::cpp17;
    s.gen.form  = gen_form::constants;
    s.gen.name_space = "mode";
    g = generate(s, resolve(s), "2026.10.04");
    expect(contains(g.text, "inline constexpr std::uint16_t MODE_OWNER_R"),
           "C++17 constants");
    expect(contains(g.text, "namespace mode"), "in a namespace");

    s.gen.base  = radix::shift;
    s.gen.form  = gen_form::macros;
    s.gen.style = gen_style::plain;
    g = generate(s, resolve(s), "2026.10.04");
    expect(contains(g.text, "(1u << 8)"), "the shift radix");
    expect(!contains(g.text, "TABLE OF CONTENTS"), "the plain style");

    return;
}

void
files_round_trip()
{
    for (const std::string& name : example_names())
    {
        const sheet       original = make_example(name);
        const std::string first    = save_sheet(original);
        sheet             loaded;
        std::string       error;

        expect(load_sheet(first, loaded, error), "an example loads");
        expect(save_sheet(loaded) == first, "and saves the same");
        expect(loaded.next_id == original.next_id, "ids continue");
    }

    sheet       loaded;
    std::string error;

    expect(!load_sheet("{ \"rows\": [", loaded, error), "not JSON");
    expect(contains(error, "line 1"), "with a position");
    expect(!load_sheet("{ \"format\": \"x\" }", loaded, error),
           "not a sheet");
    expect(!load_sheet("{ \"format\": \"bitwisedom.sheet\", \"version\": 9 }",
                       loaded,
                       error),
           "a newer version");

    return;
}

void
history_steps()
{
    history steps;
    sheet   s = make_sheet("h", 8);

    steps.record(s, "");
    s.title = "one";
    steps.record(s, "drag");
    s.title = "two";
    steps.record(s, "drag");   // merges with the step before
    s.title = "three";

    expect(steps.depth() == 2, "a run with one key is one step");
    expect(steps.undo(s) && (s.title == "one"), "undo the run");
    expect(steps.undo(s) && (s.title == "h"), "undo to the start");
    expect(!steps.undo(s), "nothing left to undo");
    expect(steps.redo(s) && (s.title == "one"), "redo");
    expect(steps.redo(s) && (s.title == "three"), "redo to the end");
    expect(!steps.can_redo(), "nothing left to redo");

    return;
}

void
examples_clean()
{
    for (const std::string& name : example_names())
    {
        const resolution r = resolve(make_example(name));

        expect(r.errors == 0, "no example has an error");
        expect(r.warnings == 0, "nor a warning");
    }

    return;
}

void
cenv_expansion()
{
    c_settings settings;
    settings.level = standard::c23;
    settings.model = data_model::lp64;

    c_env                    env(settings);
    std::vector<std::string> notes;
    const std::string        defines =
        "#define STR(x) #x\n"
        "#define CAT(a, b) a ## b\n"
        "#define SELF SELF + 1\n"
        "#define F(x) (x + 1)\n"
        "#define G F\n"
        "#define V(fmt, ...) fmt __VA_OPT__(+) __VA_ARGS__\n"
        "#define W(fmt, ...) fmt , ## __VA_ARGS__\n"
        "#define EMPTY\n"
        "#define TWO(a, b) ((a) * 10 + (b))\n";

    expect(run_directives(env, defines, notes) == 9, "nine definitions");
    expect(notes.empty(), "and nothing to say of them");
    expect(env.expand("STR(a  +  \"b\")") == "\"a + \\\"b\\\"\"",
           "# spells its argument, its quotes escaped");
    expect(env.expand("CAT(x, y)") == "xy", "## pastes");
    expect(env.expand("SELF") == "SELF + 1",
           "a macro is not replaced within its own replacement");
    expect(env.expand("G(2)") == "(2 + 1)",
           "a replacement is rescanned with what follows it");
    expect(env.expand("V(1)") == "1", "__VA_OPT__ without variable arguments");
    expect(env.expand("V(1, 2)") == "1 + 2", "and with them");
    expect(env.expand("W(1)") == "1", ", ## __VA_ARGS__ drops the comma");
    expect(env.expand("TWO(EMPTY 3, 4)") == "((3) * 10 + (4))",
           "arguments are expanded before substitution");

    c_result result = env.evaluate("CAT(1, 2) + TWO(1, 2)", c_scope{});
    expect( (result.ok) && (result.value.bits == 24u), "12 + 12");

    result = env.evaluate("TWO(1)", c_scope{});
    expect( (!result.ok) &&
            (said(result, c_level::error,
                  "takes 2 arguments (a, b), and is given 1")),
           "the wrong number of arguments");

    result = env.evaluate("CAT(+, -)", c_scope{});
    expect( (!result.ok) &&
            (said(result, c_level::error, "does not give one token")),
           "a paste that makes no token");

    result = env.evaluate("SELF", c_scope{});
    expect( (!result.ok) &&
            (said(result, c_level::error, "defined in terms of itself")),
           "a macro that names itself");

    c_macro     bad;
    std::string error;
    bad.name          = "BAD";
    bad.function_like = true;
    bad.params        = { "x" };
    bad.body          = "#y";
    expect( (!env.define(bad, error)) &&
            (contains(error, "not followed by a parameter")),
           "# before what is not a parameter");

    return;
}

void
cenv_evaluation()
{
    c_result r = c_eval("0x0A00 | (1u << 3)");
    expect( (r.ok) && (r.value.bits == 0xA08u) &&
            (r.value.type.name == "unsigned int"),
           "an int and an unsigned int");

    r = c_eval("1 << 31");
    expect( (r.undefined_behaviour) &&
            (said(r, c_level::warning, "1 << 31 overflows int")),
           "C: a 1 shifted into the sign bit is undefined");

    r = c_eval("1 << 31", standard::cpp17);
    expect( (r.ok) && (r.value.as_signed() == -2147483647 - 1),
           "C++17: it is not");

    r = c_eval("-1 << 1", standard::cpp17);
    expect(r.undefined_behaviour, "C++17: a negative shifted left is");

    r = c_eval("-1 << 1", standard::cpp20);
    expect( (r.ok) && (r.value.as_signed() == -2), "C++20: it is not");

    r = c_eval("(uint64_t)1 << -4");
    expect( (r.undefined_behaviour) &&
            (said(r, c_level::warning, "shift count -4 is negative")),
           "a negative shift count");

    r = c_eval("1L << 40", standard::c99, data_model::llp64);
    expect(r.undefined_behaviour, "LLP64's long is 32 bits");

    r = c_eval("1L << 40", standard::c99, data_model::lp64);
    expect( (r.ok) && (r.value.bits == (u64(1) << 40)), "LP64's is 64");

    r = c_eval("INT_MAX + 1");
    expect( (r.undefined_behaviour) &&
            (said(r, c_level::warning, "overflows int")),
           "signed overflow");

    r = c_eval("UINT32_MAX + 1u");
    expect( (r.ok) && (r.value.bits == 0u), "unsigned arithmetic wraps");

    r = c_eval("1 / 0");
    expect(r.undefined_behaviour, "division by zero");

    r = c_eval("1 ? 2 : 1 / 0");
    expect( (r.ok) && (r.value.bits == 2u) && (r.diagnostics.empty()),
           "the arm of ?: not taken is not evaluated");

    r = c_eval("0 && (1 / 0)");
    expect( (r.ok) && (r.value.bits == 0u), "nor the right of a decided &&");

    r = c_eval("(int8_t)200");
    expect( (r.ok) && (r.value.as_signed() == -56) &&
            (said(r, c_level::note, "implementation-defined")),
           "a narrowing conversion, noted");

    r = c_eval("-1 < 0u");
    expect( (r.ok) && (r.value.bits == 0u) &&
            (said(r, c_level::note, "becomes 4294967295")),
           "a negative meeting an unsigned type");

    r = c_eval("sizeof(uint64_t) + sizeof(long)",
               standard::c99,
               data_model::llp64);
    expect( (r.ok) && (r.value.bits == 12u), "sizeof, by the data model");

    r = c_eval("UINT64_C(1) << 63");
    expect( (r.ok) && (r.value.bits == (u64(1) << 63)) &&
            (r.value.type.name == "unsigned long long"),
           "UINT64_C pastes its suffix on");

    r = c_eval("static_cast<std::uint8_t>(0x1FF) + std::uint16_t(1)",
               standard::cpp17);
    expect( (r.ok) && (r.value.bits == 256u), "C++'s casts");

    r = c_eval("'a' + true", standard::cpp17);
    expect( (r.ok) && (r.value.bits == 98u), "C++'s char and bool");

    r = c_eval("\"text\"");
    expect( (!r.ok) &&
            (said(r, c_level::error, "a string is not an integer")),
           "a string");

    r = c_eval("4294967296", standard::c99, data_model::llp64);
    expect( (r.ok) && (r.value.type.name == "long long"),
           "a decimal literal's type");

    r = c_eval("0xFFFFFFFF");
    expect( (r.ok) && (r.value.type.name == "unsigned int"),
           "a hexadecimal literal's");

    r = c_eval("B_FLAG * 2", standard::c99, data_model::llp64,
               "#define A_FLAG 1\n#define B_FLAG A_FLAG << 2\n");
    expect( (r.ok) && (r.value.bits == 16u),
           "an unparenthesized body binds to its surroundings");

    r = c_eval("0xA00 | 0x800");
    expect( (r.ok) && (said(r, c_level::note, "'|' adds nothing")),
           "an | that sets no bit");

    return;
}

void
cenv_directives_and_names()
{
    c_env                    env(c_settings{});
    std::vector<std::string> notes;
    const int                count = run_directives(
        env,
        "#ifndef GUARD\n"
        "#define GUARD 1\n"
        "#if defined(__cplusplus)\n"
        "#define LANG 2\n"
        "#else\n"
        "#define LANG 1\n"
        "#endif\n"
        "#if USER_FEATURE\n"
        "#define FEATURE 1\n"
        "#endif\n"
        "#if _MSC_VER\n"
        "#define MSVC 1\n"
        "#endif\n"
        "#ifdef GUARD\n"
        "#undef GUARD\n"
        "#endif\n"
        "#endif\n",
        notes);

    expect(count == 3, "GUARD, LANG and FEATURE are defined");
    expect( (env.find("LANG") != nullptr) &&
            (env.find("LANG")->body == "1"),
           "as C, not C++");
    expect(env.find("FEATURE") != nullptr,
           "a condition on a name nothing defines is taken as true");
    expect( (notes.size() == 1) && (contains(notes[0], "USER_FEATURE")),
           "and said to be");
    expect(env.find("MSVC") == nullptr,
           "a compiler's own name is not assumed");
    expect(env.find("GUARD") == nullptr, "#undef");

    const std::vector<std::string> names = { "D_CODE_ARRAY_INDEX_VALID",
                                             "D_CODE_ARRAY_INDEX_HI",
                                             "D_CODE_ARRAY_INDEX_POS",
                                             "D_ERROR_ARRAY_INDEX_LO" };

    expect(suggest("D_CODE_ARRAY_IDX_IS_VALID", names) ==
               std::vector<std::string>{ "D_CODE_ARRAY_INDEX_VALID" },
           "an abbreviation, and a word too many");
    expect(suggest("D_CODE_ARRAY_IDX_IS_HI", names) ==
               std::vector<std::string>{ "D_CODE_ARRAY_INDEX_HI" },
           "the same, for HI");
    expect(suggest("D_CODE_ARRAY_IDX_LO", names).empty(), "LO is not HI");
    expect(suggest("D_CODE_ARAY_INDEX_POS", names) ==
               std::vector<std::string>{ "D_CODE_ARRAY_INDEX_POS" },
           "a letter left out");

    return;
}

void
resolve_macros_as_c()
{
    sheet s = make_sheet("m", 32);
    s.gen.model    = data_model::llp64;
    s.environment  = "#define HIDDEN 7\n";

    const int regs = insert_row(s, 0, -1, group("regs", "R_"));

    row flag = symbol("EN", def_kind::bit);
    flag.bit = 3;
    (void)insert_row(s, regs, -1, flag);

    // add
    //   a macro row at the top of the sheet
    auto add = [&s](const char* _name,
                    const char* _params,
                    const char* _body)
    {
        row made = symbol(_name, def_kind::macro);
        made.params = _params;
        made.body   = _body;
        (void)insert_row(s, 0, -1, made);
    };

    add("CODE", "", "({EN} | 0x100u)");
    add("SHIFTED", "n", "(1u << (n))");
    add("BAD", "", "SHIFTED(32)");
    add("LOCAL", "", "(EN | 1)");
    add("TWIN", "", "(0x108u)");
    add("BLOCK", "x", "do { (x) = 0; } while (0)");
    add("USES_ENV", "", "(HIDDEN + 1)");

    const resolution r = resolve(s);

    expect(value_of(r, "CODE") == 0x108u,
           "a macro's value, a symbol's macro name in it");
    expect(r.find(r.by_full.at("CODE"))->ctype == "unsigned int",
           "and its type");
    expect( (!r.find(r.by_full.at("BAD"))->has_value) &&
            (has_issue(r, severity::warning,
                       "undefined behaviour: SHIFTED(32): shift count 32 "
                       "is not less than the 32-bit width of unsigned int")),
           "undefined behaviour, where it happened");
    expect( (has_issue(r, severity::warning, "EN is not defined")) &&
            (has_issue(r, severity::warning, "write {EN}")),
           "a symbol's own name where its macro name belongs");
    expect(has_issue(r, severity::note, "has the value of CODE"), "a twin");
    expect(r.find(r.by_full.at("BLOCK"))->error.empty(),
           "C's own braces are not placeholders");
    expect(value_of(r, "USES_ENV") == 8u, "the environment's macros");
    expect(r.errors == 0, "nothing in error");

    const generated g = generate(s, r, "2026.10.09");

    expect( (contains(g.text, "#define BLOCK(x)")) &&
            (contains(g.text, "do { (x) = 0; } while (0)")),
           "a statement macro, as written");
    expect(contains(g.text, "(R_EN | 0x100u)"), "a placeholder, filled in");

    return;
}

void
import_the_attachment()
{
    import_options options;
    options.title = "attachment";
    options.level = standard::c99;
    options.model = data_model::llp64;

    const imported in = import_code(ATTACHMENT, options);

    expect( (in.ok) && (in.definitions == 27), "27 definitions");
    expect( (in.literals == 2) && (in.macros == 25),
           "two literals, and the rest macros");
    expect(in.groups == 3, "a group for each rule");
    expect(in.result.width == 32, "32 bits wide");

    const resolution r = resolve(in.result);

    expect(value_of(r, "D_CODE_ARRAY_INDEX_HI") == 0xA08u,
           "D_CODE_ARRAY_INDEX_HI is 0xA08");
    expect(r.find(r.by_full.at("D_CODE_ARRAY_INDEX_HI"))->ctype == "uint64_t",
           "a uint64_t");
    expect(value_of(r, "D_CODE_ARRAY_INDEX_POS") == 0xA00u,
           "D_CODE_ARRAY_INDEX_POS is 0xA00");
    expect(has_issue(r, severity::note, "has the value of D_CODE_BLOCK_ARRAY"),
           "as D_CODE_BLOCK_ARRAY is");
    expect(has_issue(r, severity::warning,
                     "D_BYTE_BIT_R(3, 0): shift count -4 is negative"),
           "D_CODE_ARRAY_IS_BOUNDED shifts by -4");
    expect(has_issue(r, severity::warning,
                     "D_BYTE_BIT_R(4, 0): shift count -5 is negative"),
           "D_CODE_ARRAY_INDEX_VALID by -5");
    expect(has_issue(r, severity::warning,
                     "D_UINT_CAST takes 2 arguments (_bits, _value); this "
                     "body gives it 1"),
           "D_BIT_L calls D_UINT_CAST with one argument");
    expect(has_issue(r, severity::warning,
                     "D_CODE_ARRAY_IDX_IS_VALID is not defined; did you "
                     "mean D_CODE_ARRAY_INDEX_VALID?"),
           "a misspelt name, and the name it meant");
    expect(has_issue(r, severity::warning, "D_CODE_ARRAY_FIELD is not defined"),
           "a name nothing is near");
    expect(r.errors == 0, "nothing in error: a macro is written as it is");

    const generated g = generate(in.result, r, "2026.10.09");

    expect(contains(g.text, "#define D_UINT_CAST(_bits, _value)"),
           "a head on one line");
    expect(contains(g.text, "\n      (_bits) <= 16 ? (uint16_t)(_value) :"),
           "a body's lines as they were laid out");
    expect(contains(g.text, "0x0A00\n"), "a literal's digits");
    expect(longest_line(g.text) <= 80, "no line past 80 characters");

    // read back, it is the same
    const imported   again = import_code(g.text, options);
    const resolution back  = resolve(again.result);
    bool             same  = (again.definitions == 27);

    for (const resolved_row& each : r.rows)
    {
        if (each.kind != row_kind::symbol)
        {
            continue;
        }

        const auto found = back.by_full.find(each.full);

        if (found == back.by_full.end())
        {
            same = false;

            continue;
        }

        const resolved_row* other = back.find(found->second);
        const row*          first = find_row(in.result, each.id);
        const row*          then  = find_row(again.result, other->id);

        same = ( (same) &&
                 (other->has_value == each.has_value) &&
                 (other->value == each.value) &&
                 (first->def == then->def) &&
                 (first->params == then->params) &&
                 (first->body == then->body) &&
                 (first->digits == then->digits) );
    }

    expect(same, "the header read back is the same sheet");

    return;
}

void
import_and_merge()
{
    import_options options;
    const imported in = import_code(
        "// UART_CR_NEW\n"
        "//   constant: the new bit.\n"
        "#define UART_CR_NEW (1u << 30)\n"
        "#define UART_CR_EN (1u << 5)  // moved\n"
        "#define UART_CR_WIDE (0xFu << 12)\n"
        "#define UART_CR_TEXT \"text\"\n"
        "typedef int unused;\n",
        options);

    expect(in.allocations == 3, "two bits and a range");
    expect(in.skipped == 1, "a line of code left out");
    expect(find_row(in.result, 1)->note == "the new bit", "a brief, read");

    sheet         s = make_example("uart_cr");
    const merged  m = merge_rows(s, 0, in.result);
    const resolution r = resolve(s);

    expect( (m.added == 3) && (m.updated == 1), "three new, one replaced");
    expect(value_of(r, "UART_CR_NEW") == (u64(1) << 30),
           "a new one keeps its macro name");
    expect(value_of(r, "UART_CR_EN") == (u64(1) << 5),
           "an old one takes the new definition");
    expect(value_of(r, "UART_CR_WIDE") == (u64(0xF) << 12), "a range");
    expect(looks_like_sheet(save_sheet(s)), "a sheet is told from code");
    expect(!looks_like_sheet(ATTACHMENT), "and code from a sheet");

    return;
}

// defined_as
//   what a header's #define of _name says, its spaces trimmed; empty when
// it has none on one line.
std::string
defined_as(
    const std::string& _text,
    const std::string& _name
)
{
    const std::string head = "\n#define " + _name + " ";
    const std::size_t at   = _text.find(head);

    if (at == std::string::npos)
    {
        return std::string();
    }

    const std::size_t end   = _text.find('\n', at + 1);
    std::string       value = _text.substr(at + head.size(),
                                           end - at - head.size());
    const std::size_t first = value.find_first_not_of(' ');

    return (first == std::string::npos) ? std::string()
                                        : value.substr(first);
}

/*
generator_wide
  A sheet wider than an integer, written as arrays: the array macros, a
place for every bit and range, a value as its field holds it, a set of bits
as an initializer, a field across words read and written word by word, a
range no integer carries without its _GET and _SET, the templates of a wide
sheet and not a narrow one's, and the words' width.
*/
void
generator_wide()
{
    sheet s = make_sheet("wide", 200);
    s.prefix = "W_";

    row across = symbol("ACROSS", def_kind::range);
    across.lsb   = 28;
    across.width = 8;

    const int field = insert_row(s, 0, -1, across);

    row minus = symbol("MINUS_ONE", def_kind::field_value);
    minus.field  = field;
    minus.number = -1;
    minus.enc    = encoding::twos_complement;
    insert_row(s, 0, -1, minus);

    row huge = symbol("HUGE", def_kind::range);
    huge.lsb   = 100;
    huge.width = 80;
    insert_row(s, 0, -1, huge);

    row top = symbol("TOP", def_kind::bit);
    top.bit = 199;
    insert_row(s, 0, -1, top);

    row both = symbol("BOTH", def_kind::expression);
    both.expr = "W_TOP | W_MINUS_ONE";
    insert_row(s, 0, -1, both);

    const resolution r = resolve(s);
    const generated  g = generate(s, r, "2026.10.11");

    expect(r.errors == 0, "the wide sheet has no errors");
    expect(defined_as(g.text, "W_WORD_BITS") == "32", "its words' width");
    expect(defined_as(g.text, "W_WORDS") == "7", "how many words hold it");
    expect(contains(g.text, "#define W_TEST(a, n)"), "a bit's test");
    expect(defined_as(g.text, "W_TOP") == "199", "a bit is its place");
    expect(defined_as(g.text, "W_ACROSS") == "28", "and a range its lowest");
    expect(defined_as(g.text, "W_MINUS_ONE") == "0xFFu",
           "a value is the bits its field holds");
    expect(contains(defined_as(g.text, "W_HUGE_SHIFT"), "4"),
           "the wide templates: where in its word a range begins");
    expect( (contains(g.text, "((unsigned long long)(a)[0]) >> 28)")) &&
            (contains(g.text, "((unsigned long long)(a)[1]) << 4)")),
           "a field across words is read from both");
    expect(contains(g.text, "#define W_BOTH"), "an expression");
    expect(contains(g.text, "{ 0xF0000000u, 0x0000000Fu,"),
           "is an array's initializer, lowest word first");
    expect(!contains(g.text, "#define W_HUGE_GET"),
           "a range no integer carries has no _GET");

    bool told = false;

    for (const std::string& each : g.notes)
    {
        told = ( (told) || (contains(each, "W_HUGE_GET is left out")) );
    }

    expect(told, "and the notes say why");
    expect(longest_line(g.text) <= 80, "no line past 80 characters");

    // C++'s constants, and bytes for words
    s.gen.level = standard::cpp17;
    s.gen.lang  = language::cpp;
    s.gen.form  = gen_form::constants;
    s.gen.word  = 8;

    const generated cpp = generate(s, resolve(s), "2026.10.11");

    expect(defined_as(cpp.text, "W_WORDS") == "25", "25 bytes hold it");
    expect(contains(cpp.text, "inline constexpr int W_TOP = 199;"),
           "a place is an int");
    expect(contains(cpp.text,
                    "inline constexpr std::uint8_t W_BOTH[W_WORDS] ="),
           "a set is an array of words");

    // a narrow sheet writes none of a wide one's templates
    sheet            narrow = make_example("uart_cr");
    const generated  plain  = generate(narrow, resolve(narrow), "2026.10.11");

    expect(!contains(plain.text, "UART_CR_PARITY_WORD"),
           "a narrow sheet has none of the wide templates");
    expect(bitset_prefix(make_sheet("x86 features", 128)) ==
               "X86_FEATURES_",
           "a sheet with no prefix names its arrays for its title");

    return;
}

// BITFIELD
//   constant: a bitfield as a person would write one, a key misspelt.
const char* const BITFIELD = R"json({
  "title": "dev", "width": 16, "prefix": "D_",
  "rows": [
    { "group": "Control", "prefix": "CTL_", "rows": [
      { "name": "EN", "bit": 0, "note": "on" },
      { "name": "MODE", "bits": "4..6",
        "values": { "SLOW": 1, "FAST": { "number": 2, "note": "fast" } } },
      { "name": "GAIN", "lsb": 8, "width": 4 },
      { "name": "GAIN_DOWN", "field": "D_CTL_GAIN", "number": -1,
        "encoding": "twos_complement" } ] },
    { "name": "ALL", "expr": "D_CTL_EN | D_CTL_MODE" },
    { "name": "MAGIC", "value": "0x0F00" },
    { "name": "MINUS", "value": -3 },
    { "name": "IS_ON", "params": "x", "body": "(((x) & {D_CTL_EN}) != 0)" },
    { "name": "TOP", "bit": 15, "nmae": "misspelt" } ],
  "generator": { "file_name": "dev.h" } })json";

/*
exchange_json
  A bitfield as a person writes one, read as a sheet; what one may not
say, refused with where it was said; and every example through a bitfield
and through an export, and back, writing the same header and, from the
export, the same sheet file.
*/
void
exchange_json()
{
    const json_read read = read_json_sheet(BITFIELD);

    expect( (read.ok) &&
            (read.shape == json_shape::bitfield),
           "a bitfield written by hand reads");

    const resolution r = resolve(read.result);

    expect(r.errors == 0, "and resolves without an error");
    expect(value_of(r, "D_CTL_FAST") == 0x20u,
           "a range's values are placed in it, by their own names");
    expect(value_of(r, "D_CTL_GAIN_DOWN") == 0xF00u,
           "a value names its range, and has its encoding");
    expect(value_of(r, "D_ALL") == 0x71u, "an expression");
    expect(value_of(r, "D_MAGIC") == 0xF00u, "a literal");
    expect( (r.find(r.by_full.at("D_MINUS"))->negative), "a negative one");
    expect(read.result.gen.file_name == "dev.h",
           "the generator's settings it gives");
    expect( (read.notes.size() == 1) &&
            (contains(read.notes.front(), "rows[5] (TOP)")) &&
            (contains(read.notes.front(), "\"nmae\"")),
           "a misspelt key is passed over, and noted with where");

    // what a bitfield may not say, refused with where
    expect(!read_json_sheet(R"json({ "rows": [] })json").ok, "it has a width");
    expect(contains(read_json_sheet(
                        R"json({ "width": 8, "rows": [ { "group": "G",
                             "rows": [ { "name": "BAD", "bit": 1,
                                         "expr": "X" } ] } ] })json").error,
                    "rows[0].rows[0] (BAD): two definitions"),
           "two definitions are refused, with the row's path");
    expect(contains(read_json_sheet(
                        R"json({ "width": 8, "rows": [ { "name": "V",
                             "field": "NOPE", "number": 1 } ] })json").error,
                    "names no row"),
           "a value's range must be there");
    expect(!read_json_sheet(R"json({ "format": "elsewhere.thing" })json").ok,
           "and a format it does not know is refused");

    // a row to a line, where it fits; one too long, a member to a line
    const std::string written = write_bitfield(read.result);

    expect(contains(written,
                    "\n                { \"name\": \"EN\", \"bit\": 0, "
                    "\"note\": \"on\" },\n"),
           "a short row is written on one line");
    expect(contains(written,
                    "\"values\": { \"SLOW\": 1, \"FAST\": "
                    "{ \"number\": 2, \"note\": \"fast\" } }\n"),
           "and values that fit, objects and all");

    sheet long_note = make_sheet("long", 8);
    row   noted     = symbol("A", def_kind::bit);
    noted.bit  = 0;
    noted.note = "a note long enough that its row will not fit on one line "
                 "of a hundred columns, however it is indented";
    (void)insert_row(long_note, 0, -1, noted);

    expect(contains(write_bitfield(long_note),
                    "{\n            \"name\": \"A\",\n"),
           "a row too long for its line, a member to a line");

    // every example, there and back
    bool same_header = true;
    bool same_sheet  = true;
    bool every_row   = true;

    for (const std::string& name : example_names())
    {
        const sheet      original = make_example(name);
        const resolution resolved = resolve(original);
        const generated  header   = generate(original, resolved,
                                             "2026.10.11");
        const json_read  through  = read_json_sheet(
                                        write_bitfield(original));
        const std::string all     = write_export(original, resolved, header);
        const json_read  whole    = read_json_sheet(all);

        same_header = ( (same_header) &&
                        (through.ok) &&
                        (generate(through.result,
                                  resolve(through.result),
                                  "2026.10.11").text == header.text) );
        same_sheet  = ( (same_sheet) &&
                        (whole.ok) &&
                        (whole.shape == json_shape::everything) &&
                        (save_sheet(whole.result) == save_sheet(original)) );

        // an export lists every row
        json_value  parsed;
        std::string error;
        int         rows = 0;

        walk(original,
             [&rows](const row&, const row&, int)
             {
                 ++rows;
             });

        every_row = ( (every_row) &&
                      (parse_json(all, parsed, error)) &&
                      (parsed.find("symbols") != nullptr) &&
                      (static_cast<int>(
                           parsed.find("symbols")->items.size()) == rows) );
    }

    expect(same_header, "a bitfield writes the same header as its sheet");
    expect(same_sheet, "an export holds its sheet whole");
    expect(every_row, "and lists every row");

    return;
}

void
generator_macro_layout()
{
    sheet s = make_sheet("layout", 32);
    s.gen.banner = false;
    s.gen.guard  = false;
    s.gen.pad    = false;

    row wide = symbol("D_A_MACRO_WITH_A_LONG_NAME_AND_PARAMETERS",
                      def_kind::macro);
    wide.params = "_first_parameter, _second_parameter, _third";
    wide.body   = "((_first_parameter) + (_second_parameter) + (_third))";
    (void)insert_row(s, 0, -1, wide);

    row now = symbol("D_NOW", def_kind::macro);
    now.callable = true;
    now.body     = "(42)";
    (void)insert_row(s, 0, -1, now);

    row lines = symbol("D_LINES", def_kind::macro);
    lines.body = "( (1) |\n  (2) )";
    (void)insert_row(s, 0, -1, lines);

    row literal = symbol("D_LIT", def_kind::literal);
    literal.literal = 0xA00u;
    literal.fmt     = radix::hex;
    literal.suf     = int_suffix::none;
    literal.digits  = 4;
    (void)insert_row(s, 0, -1, literal);

    const resolution r = resolve(s);
    const generated  g = generate(s, r, "2026.10.09");

    expect(contains(g.text, "#define D_A_MACRO_WITH_A_LONG_NAME_AND_PARAMETERS("
                            "_first_parameter,"),
           "parameters spread, one to a line");
    expect(contains(g.text, "\n" + std::string(50, ' ') + "_second_parameter,"),
           "aligned after the parenthesis");
    expect(contains(g.text, "#define D_NOW()"),
           "a function-like macro of no parameters");
    const std::string first_line = "    ( (1) |";

    expect(contains(g.text, "\n" + first_line +
                                std::string(79 - first_line.size(), ' ') +
                                "\\\n      (2) )"),
           "a body's own lines, the backslash at the 80th column");
    expect(contains(g.text, "0x0A00\n"), "a literal's least digits");
    expect(value_of(r, "D_LINES") == 3u, "a body over lines, evaluated");
    expect(!r.find(r.by_full.at("D_NOW"))->has_value,
           "a function-like macro has no value");
    expect(longest_line(g.text) <= 80, "no line past 80 characters");

    return;
}

void
files_new_fields()
{
    sheet s = make_sheet("f", 32);
    s.environment = "#define X 1\n";
    s.gen.model   = data_model::ilp32;

    row literal = symbol("L", def_kind::literal);
    literal.digits = 6;
    (void)insert_row(s, 0, -1, literal);

    row macro = symbol("M", def_kind::macro);
    macro.callable = true;
    macro.body     = "(1)";
    (void)insert_row(s, 0, -1, macro);

    sheet       loaded;
    std::string error;

    expect(load_sheet(save_sheet(s), loaded, error), "it loads");
    expect(loaded.environment == s.environment, "its environment");
    expect(loaded.gen.model == data_model::ilp32, "its data model");
    expect(loaded.root.children[0].digits == 6, "a literal's digits");
    expect(loaded.root.children[1].callable, "a callable macro");

    return;
}

void
rename_and_references()
{
    sheet s = make_example("uart_cr");

    row check = symbol("CHECK", def_kind::macro);
    check.params = "EN";
    check.body   = "((EN) | UART_CR_EN | sizeof(\"UART_CR_EN\"))";
    (void)insert_row(s, 0, -1, check);

    const resolution before   = resolve(s);
    const int        en       = before.by_full.at("UART_CR_EN");
    const int        preset   = before.by_full.at("UART_CR_DEFAULT");
    const int        enabled  = before.by_full.at("UART_CR_IS_ENABLED");
    const int        checking = before.by_full.at("UART_CR_CHECK");
    const u64        value    = value_of(before, "UART_CR_DEFAULT");
    const renamed    done     = rename_row(s, en, "ENABLE");
    const resolution after    = resolve(s);

    expect(done.ok, "the rename is made");
    expect( (after.by_full.count("UART_CR_ENABLE") == 1) &&
            (after.by_full.count("UART_CR_EN") == 0),
           "the macro name follows the name");
    expect(done.references == 3, "three references rewritten");
    expect(contains(find_row(s, preset)->expr, "ENABLE |"),
           "in an expression, by its own name");
    expect(find_row(s, enabled)->body == "(((x) & {ENABLE}) != 0)",
           "in a placeholder");
    expect(find_row(s, checking)->body ==
               "((EN) | UART_CR_ENABLE | sizeof(\"UART_CR_EN\"))",
           "by its macro name, but not a parameter or a string");
    expect(value_of(after, "UART_CR_DEFAULT") == value,
           "and the sheet means what it meant");
    expect(after.errors == 0, "nothing in error");
    expect(!rename_row(s, en, "TXEN").ok, "not to another symbol's name");
    expect(!rename_row(s, en, "A-B").ok, "nor to what is no C name");
    expect(rename_row(s, find_row(s, 1)->id, "Switches").ok, "a group");

    // the group as code to paste: its symbols, under its name, alone
    const resolution again = resolve(s);
    const generated  code  = generate_code(s, again, { 1 });

    expect( (contains(code.text, "// Switches")) &&
            (contains(code.text, "#define UART_CR_ENABLE")),
           "a group as code");
    expect( (!contains(code.text, "UART_CR_PARITY")) &&
            (!contains(code.text, "#ifndef")) &&
            (!contains(code.text, "TABLE OF CONTENTS")),
           "and nothing else");

    return;
}

/*
colours_and_schemes
  A new sheet's colours are all automatic, and the theme and scheme it is
shown with fill them in; what a sheet sets, it keeps in every theme, and a
file writes only that. A file from before themes were the window's had
every colour written out: its defaults are automatic again when read, and
a colour that was chosen stays chosen.
*/
void
colours_and_schemes()
{
    sheet s = make_example("uart_cr");

    expect( (s.colours.palette == -1) &&
            (s.colours.accent == 0) &&
            (s.grid.minor.colour == 0),
           "a new sheet's colours are automatic");

    const look dark  = make_look(s, 0, 0);
    const look light = make_look(s, 1, 0);
    const look earth = make_look(s, 1, 2);

    expect( (dark.colours.theme == 0) &&
            (light.colours.theme == 1) &&
            (dark.colours.empty_cell != light.colours.empty_cell) &&
            (dark.grid.minor.colour != light.grid.minor.colour),
           "a look is the theme's");
    expect( (earth.colours.palette == 2) &&
            (earth.colours.accent == scheme_for(1, 2).colours.accent) &&
            (earth.colours.accent != light.colours.accent),
           "and the scheme's");
    expect( (dark.colours.accent != 0) &&
            (dark.colours.selection != 0) &&
            (dark.grid.segments.colour != 0),
           "with nothing left automatic");

    // the automatic row colours follow the scheme's palette
    const resolution vivid  = resolve(s, make_look(s, 0, 0).colours);
    const resolution blind  = resolve(s, make_look(s, 0, 4).colours);
    const int        en     = vivid.by_full.at("UART_CR_EN");

    expect(vivid.find(en)->colour != blind.find(en)->colour,
           "rows take their colours from the scheme's palette");

    // what the sheet sets, it keeps
    s.colours.accent  = 0x123456FFu;
    s.colours.palette = 3;

    expect( (make_look(s, 1, 2).colours.accent == 0x123456FFu) &&
            (make_look(s, 0, 0).colours.palette == 3),
           "a sheet's own colour stays in every theme and scheme");

    // and a file writes only that
    const std::string text = save_sheet(s);
    sheet             back;
    std::string       error;

    expect(load_sheet(text, back, error), "the sheet reads back");
    expect( (back.colours.accent == 0x123456FFu) &&
            (back.colours.palette == 3) &&
            (back.colours.empty_cell == 0) &&
            (back.grid.minor.colour == 0),
           "with what it set, and the rest automatic");
    expect( (!contains(text, "\"theme\"")) &&
            (!contains(text, "\"empty_cell\"")),
           "and the file names no theme and no automatic colour");

    automatic_colours(back);
    expect( (back.colours.accent == 0) &&
            (back.colours.palette == -1),
           "a sheet's colours given back to the scheme");

    // a file from before: its defaults automatic, its choices kept
    const char* const old =
        "{\"format\":\"bitwisedom.sheet\",\"version\":1,\"title\":\"old\","
        "\"width\":8,\"colours\":{\"theme\":3,\"palette\":0,"
        "\"saturation\":0.7,\"value\":0.82,\"accent\":\"#2F6FE0FF\","
        "\"empty_cell\":\"#EEF0F4FF\",\"collision\":\"#AA0000FF\","
        "\"probe\":\"#FFC247FF\"},"
        "\"grid\":{\"minor\":{\"colour\":\"#C9CED8FF\"},"
        "\"major\":{\"colour\":\"#00FF00FF\"}},"
        "\"rows\":[{\"id\":1,\"kind\":\"symbol\",\"name\":\"A\","
        "\"def\":\"bit\",\"bit\":3}]}";
    sheet older;

    expect(load_sheet(old, older, error), "an old file reads");
    expect( (older.colours.accent == 0) &&
            (older.colours.empty_cell == 0) &&
            (older.colours.probe == 0) &&
            (older.colours.palette == -1) &&
            (older.colours.saturation < 0.0f) &&
            (older.grid.minor.colour == 0),
           "its defaults are automatic");
    expect( (older.colours.collision == 0xAA0000FFu) &&
            (older.grid.major.colour == 0x00FF00FFu),
           "and its own colours are kept");

    return;
}

// section
//   struct: one entry of the table.
struct section
{
    const char* name;
    void      (*run)();
};

const section SECTIONS[] = {
    { "bits: masks and runs",         bits_masks_and_runs },
    { "bits: movement",               bits_movement },
    { "bits: encodings",              bits_encodings },
    { "bits: parsing and formatting", bits_parsing },
    { "bits: wide values",            bits_wide },
    { "expressions: evaluate",        expressions_evaluate },
    { "expressions: print as C",      expressions_print },
    { "resolve: names and values",    resolve_names_and_values },
    { "ranges: nesting and defines",  nested_ranges_and_symbolic_defines },
    { "resolve: issues",              resolve_issues },
    { "sheet: tree operations",       tree_operations },
    { "sheet: groups opened, closed", groups_opened_and_closed },
    { "ops: values and flow",         operations_values },
    { "ops: preview and apply",       operations_preview_and_apply },
    { "codegen: macros",              generator_macros },
    { "codegen: levels and forms",    generator_levels },
    { "files: round trip",            files_round_trip },
    { "history: steps",               history_steps },
    { "examples: clean",              examples_clean },
    { "cenv: expansion",              cenv_expansion },
    { "cenv: evaluation",             cenv_evaluation },
    { "cenv: directives and names",   cenv_directives_and_names },
    { "resolve: macros as C",         resolve_macros_as_c },
    { "import: the attachment",       import_the_attachment },
    { "import: and merge",            import_and_merge },
    { "codegen: macro layout",        generator_macro_layout },
    { "codegen: wide sheets",         generator_wide },
    { "files: bitfields and exports", exchange_json },
    { "files: new fields",            files_new_fields },
    { "rename: and its references",   rename_and_references },
    { "colours: themes and schemes",  colours_and_schemes }
};

}  // anonymous namespace


int
main()
{
    int passed = 0;
    int failed = 0;

    for (const section& each : SECTIONS)
    {
        failure = nullptr;
        each.run();

        if (failure == nullptr)
        {
            ++passed;
            std::printf("  %-32s PASS\n", each.name);
        }
        else
        {
            ++failed;
            std::printf("  %-32s FAIL: %s\n", each.name, failure);
        }
    }

    std::printf("passed: %d   failed: %d\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}
