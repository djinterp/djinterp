/*******************************************************************************
* bitwisedom [bitwisedom]                                           selftest.cpp
*
* A header's self-test.
*   A narrow sheet's is assertions alone, each a typedef of an array whose
* size is -1 where the assertion fails, which every C and C++ level since
* C89 refuses. A wide sheet's adds a run: tables of the words each array
* should hold, and a main that fills, sets, clears and compares, naming the
* checks that fail by number. Moved here from bitwisedom-gen, which wrote
* them alone until `bitwisedom gen` wrote them too.
*
* path:      /src/djinterp/tools/bitwisedom/selftest.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.22
*******************************************************************************/
#include <djinterp/tools/bitwisedom/selftest.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::min
#include <string>     // std::string
// bitwisedom
#include <djinterp/tools/bitwisedom/bits.hpp>     // format_hex, wide
#include <djinterp/tools/bitwisedom/codegen.hpp>  // has_long_long, is_cpp,
                                                  // bitset_prefix


namespace bitwisedom
{

namespace
{

// literal_for
//   the literal a self-test compares a value with: wide enough for it. C++98
// has no long long, so there every value is an unsigned long, as the header's
// own are -- which holds 64 bits only where long does.
std::string
literal_for(
    u64      _value,
    standard _level
)
{
    const bool wide = ( (_value > 0xFFFFFFFFu) &&
                        (has_long_long(_level)) );

    return "0x" + format_hex(_value, 0, true) + ((wide) ? "ULL" : "UL");
}

// has_template
//   whether the header writes a template of _name beside a symbol of
// _kind: one that is on, for that kind, and for a sheet of _sheet's width.
bool
has_template(
    const sheet&       _sheet,
    const std::string& _name,
    def_kind           _kind
)
{
    for (const macro_template& each : _sheet.gen.templates)
    {
        if ( (each.enabled)                           &&
             (each.wide == is_wide(_sheet.width))     &&
             (each.name == _name)                     &&
             ((each.applies & applies_bit(_kind)) != 0) )
        {
            return true;
        }
    }

    return false;
}

// narrow_self_test
//   a translation unit that compiles only if the header is right.
std::string
narrow_self_test(
    const sheet&       _sheet,
    const resolution&  _resolution,
    const std::string& _header
)
{
    const standard level = _sheet.gen.level;
    std::string    out;
    int            count = 0;

    out += "/* the self-test of " + _header + ", written by bitwisedom:\n"
           "   it compiles only if every macro has the sheet's value. */\n";
    out += "#include \"" + _header + "\"\n\n";
    out += "#define BITWISEDOM_CHECK(name, holds) "
           "typedef char name[(holds) ? 1 : -1]\n\n";

    // check
    //   one assertion, under a name of its own.
    auto check = [&out, &count](const std::string& _condition)
    {
        out += "BITWISEDOM_CHECK(bitwisedom_check_" +
               format_dec(static_cast<u64>(count++)) + ", " + _condition +
               ");\n";

        return;
    };

    const bool getter = has_template(_sheet, "{NAME}_GET", def_kind::range);
    const bool setter = has_template(_sheet, "{NAME}_SET", def_kind::range);

    for (const resolved_row& each : _resolution.rows)
    {
        const row* source = find_row(_sheet, each.id);

        if ( (each.kind != row_kind::symbol) ||
             (each.def == def_kind::macro)   ||
             (!each.error.empty())           ||
             (source == nullptr)             ||
             (!source->emit) )
        {
            continue;
        }

        // a negative literal is written as a negative number
        if ( (each.def == def_kind::literal) &&
             (source->negative) )
        {
            check("(" + each.full + ") == (" +
                  format_signed(sign_extend(each.value, _sheet.width)) +
                  ")");

            continue;
        }

        check("(" + each.full + ") == " + literal_for(each.value, level));

        // a named value read back through its field's templates
        const resolved_row* field = _resolution.find(each.field);

        if ( (each.def != def_kind::field_value) ||
             (field == nullptr) )
        {
            continue;
        }

        const u64 raw = each.value >> each.span_lsb;

        if (getter)
        {
            check(field->full + "_GET(" + each.full + ") == " +
                  literal_for(raw, level));
        }

        if (setter)
        {
            check(field->full + "_SET(0, " + literal_for(raw, level) +
                  ") == " + literal_for(each.value, level));
        }
    }

    out += "\nint\nmain(void)\n{\n    return 0;\n}\n";

    return out;
}

// word_type
//   the type a wide sheet's self-test holds a word in: the header's own --
// the fixed-width type in the C++ constants form, which includes <cstdint>
// for it -- and elsewhere the C type as wide.
std::string
word_type(
    int      _word,
    standard _level,
    bool     _constants
)
{
    if ( (_constants) &&
         (_level >= standard::cpp11) )
    {
        return "std::uint" + format_dec(static_cast<u64>(_word)) + "_t";
    }

    switch (_word)
    {
        case 8:  return "unsigned char";
        case 16: return "unsigned short";
        case 64: return has_long_long(_level) ? "unsigned long long"
                                              : "unsigned long";
        default: return "unsigned int";
    }
}

/*
wide_self_test
  A wide sheet's self-test. What C knows as an integer -- a place, a value,
a literal an integer holds, and the templates' words, shifts and masks --
is checked as it is compiled, as a narrow sheet's values are. What lives in
arrays is checked as it runs: each array the header defines against the
words the sheet gives it, every bit set, tested and cleared through the
array macros, and every field written and read back through its _SET and
_GET, alone and among bits all set. It returns 0 only if each check holds,
and names the checks that do not.
*/
std::string
wide_self_test(
    const sheet&       _sheet,
    const resolution&  _resolution,
    const std::string& _header
)
{
    const standard    level     = _sheet.gen.level;
    const bool        constants = ( (_sheet.gen.form == gen_form::constants) &&
                                    (is_cpp(level)) );
    const int         word      = word_bits(_sheet);
    const int         words     = (_sheet.width + word - 1) / word;
    const std::string p         = bitset_prefix(_sheet);
    const std::string type      = word_type(word, level, constants);
    const wide        all       = wide::run(0, words * word);
    std::string       out;
    std::string       tables;   // the words the run compares arrays with
    std::string       body;     // main's checks
    int               count     = 0;
    int               runs      = 1;
    int               made      = 0;

    // check
    //   one assertion, as the header is compiled.
    auto check = [&out, &count](const std::string& _condition)
    {
        out += "BITWISEDOM_CHECK(bitwisedom_check_" +
               format_dec(static_cast<u64>(count++)) + ", " + _condition +
               ");\n";

        return;
    };

    // step
    //   one statement of the run.
    auto step = [&body](const std::string& _statement)
    {
        body += "    " + _statement + ";\n";

        return;
    };

    // expect
    //   one check, as the test runs.
    auto expect = [&body, &runs](const std::string& _condition)
    {
        body += "    bitwisedom_expect(" +
                format_dec(static_cast<u64>(runs++)) + ", " + _condition +
                ");\n";

        return;
    };

    // table
    //   an array of the words _bits makes, to compare with; its name.
    auto table = [&](const wide& _bits)
    {
        const std::string name = "bitwisedom_words_" +
                                 format_dec(static_cast<u64>(made++));

        tables += "static const " + type + " " + name + "[" + p +
                  "WORDS] = {";

        for (int k = 0; k < words; ++k)
        {
            const u64 each = _bits.field(k * word, word);

            tables += (k == 0) ? " " : ", ";
            tables += "0x" + format_hex(each, 0, true) +
                      ( (word <= 32)             ? "U"
                      : (has_long_long(level))   ? "ULL"
                                                 : "UL" );
        }

        tables += " };\n";

        return name;
    };

    out += "/* the self-test of " + _header + ", written by bitwisedom:\n"
           "   it compiles only if every macro C knows as an integer has the\n"
           "   sheet's value, and it returns 0 only if every array holds the\n"
           "   sheet's bits and the array macros read and write them as the\n"
           "   sheet does. */\n";
    out += "#include \"" + _header + "\"\n";
    out += "#include <stdio.h>\n\n";

    if ( (constants) &&
         (!_sheet.gen.name_space.empty()) )
    {
        out += "using namespace " + _sheet.gen.name_space + ";\n\n";
    }

    out += "#define BITWISEDOM_CHECK(name, holds) "
           "typedef char name[(holds) ? 1 : -1]\n\n";

    // the array macros
    check("(" + p + "WORD_BITS) == " + format_dec(static_cast<u64>(word)));
    check("(" + p + "WORDS) == " + format_dec(static_cast<u64>(words)));
    check("(" + p + "WORD(" + format_dec(static_cast<u64>(word + 1)) +
          ")) == 1");
    check("(" + p + "MASK(" + format_dec(static_cast<u64>(word + 3)) +
          ")) == " + literal_for(8u, level));

    for (const resolved_row& each : _resolution.rows)
    {
        const row* source = find_row(_sheet, each.id);

        if ( (each.kind != row_kind::symbol) ||
             (each.def == def_kind::macro)   ||
             (!each.error.empty())           ||
             (source == nullptr)             ||
             (!source->emit) )
        {
            continue;
        }

        const std::string name = each.full;
        const int         lsb  = each.span_lsb;

        switch (each.def)
        {
            case def_kind::bit:
            {
                const std::string at = format_dec(static_cast<u64>(lsb));

                check("(" + name + ") == " + at);

                if (has_template(_sheet, "{NAME}_WORD", def_kind::bit))
                {
                    check("(" + name + "_WORD) == " +
                          format_dec(static_cast<u64>(lsb / word)));
                }

                if (has_template(_sheet, "{NAME}_MASK", def_kind::bit))
                {
                    check("(" + name + "_MASK) == " +
                          literal_for(u64(1) << (lsb % word), level));
                }

                wide cleared = all;

                cleared.set(lsb, false);

                step("bitwisedom_fill(a, 0)");
                step(p + "SET(a, " + name + ")");
                expect(p + "TEST(a, " + name + ")");
                expect("bitwisedom_same(a, " + table(wide::bit(lsb)) + ")");

                if (has_template(_sheet, "{NAME}_IS_SET", def_kind::bit))
                {
                    expect(name + "_IS_SET(a)");
                }

                step("bitwisedom_fill(a, 1)");
                step(p + "CLEAR(a, " + name + ")");
                expect("!" + p + "TEST(a, " + name + ")");
                expect("bitwisedom_same(a, " + table(cleared) + ")");

                break;
            }

            case def_kind::range:
            {
                const int width = each.span_width;
                const int shift = lsb % word;
                const int here  = std::min(width, word - shift);

                check("(" + name + ") == " +
                      format_dec(static_cast<u64>(lsb)));

                if (has_template(_sheet, "{NAME}_WIDTH", def_kind::range))
                {
                    check("(" + name + "_WIDTH) == " +
                          format_dec(static_cast<u64>(width)));
                }

                if (has_template(_sheet, "{NAME}_WORD", def_kind::range))
                {
                    check("(" + name + "_WORD) == " +
                          format_dec(static_cast<u64>(lsb / word)));
                }

                if (has_template(_sheet, "{NAME}_SHIFT", def_kind::range))
                {
                    check("(" + name + "_SHIFT) == " +
                          format_dec(static_cast<u64>(shift)));
                }

                if (has_template(_sheet, "{NAME}_MASK", def_kind::range))
                {
                    check("(" + name + "_MASK) == " +
                          literal_for(range_mask(shift, here), level));
                }

                // a field no integer carries has no _GET or _SET
                if ( (width > INTEGER_WIDTH) ||
                     (!has_template(_sheet, "{NAME}_GET", def_kind::range)) ||
                     (!has_template(_sheet, "{NAME}_SET", def_kind::range)) )
                {
                    break;
                }

                // a pattern and its complement in an empty array, so every
                // bit of the field is written as 1 and as 0; and nothing in
                // a full one
                const u64 patterns[2] = {
                    0xA5A5A5A5A5A5A5A5ull & width_mask(width),
                    0x5A5A5A5A5A5A5A5Aull & width_mask(width) };
                wide      emptied = all;

                emptied.put(lsb, width, 0u);

                for (const u64 pattern : patterns)
                {
                    const std::string v = literal_for(pattern, level);
                    wide              placed;

                    placed.put(lsb, width, pattern);

                    step("bitwisedom_fill(a, 0)");
                    step("(void)" + name + "_SET(a, " + v + ")");
                    expect(name + "_GET(a) == " + v);
                    expect("bitwisedom_same(a, " + table(placed) + ")");
                }

                step("bitwisedom_fill(a, 1)");
                step("(void)" + name + "_SET(a, " + literal_for(0u, level) +
                     ")");
                expect(name + "_GET(a) == " + literal_for(0u, level));
                expect("bitwisedom_same(a, " + table(emptied) + ")");

                break;
            }

            case def_kind::field_value:
            {
                const u64           held  = each.bits.field(lsb,
                                                            each.span_width);
                const resolved_row* field = _resolution.find(each.field);

                check("(" + name + ") == " + literal_for(held, level));

                if ( (field == nullptr) ||
                     (field->span_width > INTEGER_WIDTH) ||
                     (!has_template(_sheet, "{NAME}_GET", def_kind::range)) ||
                     (!has_template(_sheet, "{NAME}_SET", def_kind::range)) )
                {
                    break;
                }

                step("bitwisedom_fill(a, 0)");
                step("(void)" + field->full + "_SET(a, " + name + ")");
                expect(field->full + "_GET(a) == " + name);
                expect("bitwisedom_same(a, " + table(each.bits) + ")");

                if (has_template(_sheet, "{NAME}_IS",
                                 def_kind::field_value))
                {
                    expect(name + "_IS(a)");
                }

                break;
            }

            case def_kind::literal:
            case def_kind::expression:
            {
                const bool array = ( (each.def == def_kind::expression) ||
                                     ( (!source->negative) &&
                                       (each.bits.highest() >=
                                        INTEGER_WIDTH) ) );

                if (!array)
                {
                    check("(" + name + ") == " +
                          (source->negative
                               ? "(" + format_signed(static_cast<i64>(
                                           each.value)) + ")"
                               : literal_for(each.value, level)));

                    break;
                }

                // an initializer is an array's once it is given one
                if (constants)
                {
                    expect("bitwisedom_same(" + name + ", " +
                           table(each.bits) + ")");
                }
                else
                {
                    const std::string got = "bitwisedom_got_" +
                                            format_dec(static_cast<u64>(
                                                runs));

                    body += "    {\n        const " + type + " " + got +
                            "[" + p + "WORDS] = " + name + ";\n\n    ";
                    body += "    bitwisedom_expect(" +
                            format_dec(static_cast<u64>(runs++)) +
                            ", bitwisedom_same(" + got + ", " +
                            table(each.bits) + "));\n    }\n";
                }

                break;
            }

            case def_kind::macro:
                break;
        }
    }

    out += "\n";
    out += "static int bitwisedom_failed = 0;\n\n";
    out += "static void\nbitwisedom_expect(\n    int _check,\n"
           "    int _holds\n)\n{\n    if (!_holds)\n    {\n"
           "        printf(\"run-time check %d failed\\n\", _check);\n"
           "        ++bitwisedom_failed;\n    }\n}\n\n";
    out += "/* every word all ones, or all zeros */\n";
    out += "static void\nbitwisedom_fill(\n    " + type + "* _a,\n"
           "    int _ones\n)\n{\n    int k;\n\n"
           "    for (k = 0; k < " + p + "WORDS; ++k)\n    {\n"
           "        _a[k] = (" + type + ")(_ones ? ~(" + type + ")0 : (" +
           type + ")0);\n    }\n}\n\n";
    out += "/* whether two arrays hold the same bits */\n";
    out += "static int\nbitwisedom_same(\n    const " + type + "* _a,\n"
           "    const " + type + "* _b\n)\n{\n    int k;\n\n"
           "    for (k = 0; k < " + p + "WORDS; ++k)\n    {\n"
           "        if (_a[k] != _b[k])\n        {\n"
           "            return 0;\n        }\n    }\n\n"
           "    return 1;\n}\n\n";
    out += tables;
    out += "\nint\nmain(void)\n{\n    " + type + " a[" + p + "WORDS];\n    " +
           type + " b[" + p + "WORDS];\n\n"
           "    /* the harness itself */\n"
           "    bitwisedom_fill(a, 1);\n"
           "    bitwisedom_fill(b, 1);\n"
           "    bitwisedom_expect(0, bitwisedom_same(a, b));\n\n";
    out += body;
    out += "\n    return (bitwisedom_failed == 0) ? 0 : 1;\n}\n";

    return out;
}

}  // anonymous namespace


/*
self_test
  The wide one for a sheet whose bits are arrays, the narrow one otherwise.
*/
std::string
self_test(
    const sheet&       _sheet,
    const resolution&  _resolution,
    const std::string& _header
)
{
    return is_wide(_sheet.width)
           ? wide_self_test(_sheet,
                            _resolution,
                            _header)
           : narrow_self_test(_sheet,
                              _resolution,
                              _header);
}

}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
