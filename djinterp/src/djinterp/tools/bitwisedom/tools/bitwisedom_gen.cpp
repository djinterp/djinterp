/*******************************************************************************
* bitwisedom [tools]                                          bitwisedom_gen.cpp
*
* bitwisedom-gen: a sheet to a header, without the GUI.
*   The generator the GUI's Macros tab runs, for build scripts: read a sheet
* file (or a built-in example), check it, and write its header. It can also
* write the examples out as sheet files, and write a self-test -- a C or
* C++ file that includes the header and fails to compile unless every macro
* has the value the sheet gives it, and every range's GET and SET templates
* agree with its named values. A sheet wider than an integer keeps its bits
* in arrays, which no compile-time check reads: its self-test checks them as
* it runs, and returns 0 only if each holds. CTest compiles that file at
* every language level, and runs a wide sheet's, which is how the generator
* is proved, not just run.
*
*     bitwisedom-gen <sheet.json | --example NAME> [options]
*       -o, --out FILE        write the header to FILE (default: stdout)
*       --self-test FILE      write the compile-time self-test to FILE
*       --bitfield FILE       write the sheet as a bitfield (JSON) to FILE
*       --export FILE         write everything about it (JSON) to FILE
*       --lang c|cpp          --form macros|constants
*       --level c99|...|cpp23 --style djinterp|plain
*       --word 8|16|32|64     a wide sheet's array words, in bits
*       --date YYYY.MM.DD     the banner's date (default: today's)
*       --check               report the sheet's issues; write nothing
*     bitwisedom-gen --save-example NAME FILE
*     bitwisedom-gen --list-examples
*
*   The sheet may be any of the JSON shapes exchange.hpp reads: a sheet
* file, a bitfield, or an export. Asked for a bitfield or an export and not
* for -o, it writes no header to stdout.
*   EXIT STATUS. 0 when it did what it was asked; 1 when the sheet has
* errors (the header is still written, without the symbols in error); 2 for
* a usage, file or format problem.
*
* path:      /src/djinterp/tools/bitwisedom/tools/bitwisedom_gen.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
// std
#include <algorithm>  // std::min
#include <cstdio>     // std::fprintf, std::fputs, std::snprintf
#include <cstdlib>    // std::atoi
#include <ctime>      // std::time, std::localtime
#include <string>     // std::string
#include <utility>    // std::move
#include <vector>     // std::vector
// bitwisedom
#include <djinterp/tools/bitwisedom/codegen.hpp>   // generate, standard_name
#include <djinterp/tools/bitwisedom/examples.hpp>  // make_example, example_names
#include <djinterp/tools/bitwisedom/exchange.hpp>  // read_json_sheet, write_export
#include <djinterp/tools/bitwisedom/persist.hpp>   // load_sheet, save_sheet
#include <djinterp/tools/bitwisedom/resolve.hpp>   // resolve


namespace
{

using namespace ::bitwisedom;

// USAGE
//   constant: what --help prints.
const char* const USAGE =
    "usage: bitwisedom-gen <sheet.json | --example NAME> [options]\n"
    "  -o, --out FILE        write the header to FILE (default: stdout)\n"
    "  --self-test FILE      write a compile-time self-test of the header\n"
    "  --bitfield FILE       write the sheet as a bitfield (JSON)\n"
    "  --export FILE         write everything about it (JSON)\n"
    "  --lang c|cpp          --form macros|constants\n"
    "  --level c99|c11|c17|c23|cpp98|cpp11|cpp14|cpp17|cpp20|cpp23\n"
    "  --style djinterp|plain\n"
    "  --word 8|16|32|64     a wide sheet's array words, in bits\n"
    "  --date YYYY.MM.DD     the banner's date (default: today's)\n"
    "  --check               report the sheet's issues; write nothing\n"
    "       bitwisedom-gen --save-example NAME FILE\n"
    "       bitwisedom-gen --list-examples\n";

// options
//   struct: what the command line asked for.
struct options
{
    std::string sheet_path;
    std::string example;
    std::string out;
    std::string self_test;
    std::string bitfield;
    std::string everything;
    std::string lang;
    std::string form;
    std::string level;
    std::string style;
    std::string word;
    std::string date;
    bool        check = false;
};

// today
//   the local date, as the banner wants it; a build that must reproduce
// its header byte for byte passes --date instead.
std::string
today()
{
    const std::time_t now   = std::time(nullptr);
    const std::tm*    local = std::localtime(&now);
    char              text[48];

    if (local == nullptr)
    {
        return "2026.01.01";
    }

    std::snprintf(text,
                  sizeof(text),
                  "%04d.%02d.%02d",
                  local->tm_year + 1900,
                  local->tm_mon + 1,
                  local->tm_mday);

    return text;
}

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

// self_test
//   a translation unit that compiles only if the header is right.
std::string
self_test(
    const sheet&       _sheet,
    const resolution&  _resolution,
    const std::string& _header
)
{
    const standard level = _sheet.gen.level;
    std::string    out;
    int            count = 0;

    out += "/* the self-test of " + _header + ", written by bitwisedom-gen:\n"
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

    out += "/* the self-test of " + _header + ", written by bitwisedom-gen:\n"
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

// parse_arguments
//   the command line into options; false, having said why, on a mistake.
bool
parse_arguments(
    int      _argc,
    char**   _argv,
    options& _options
)
{
    for (int i = 1; i < _argc; ++i)
    {
        const std::string each = _argv[i];

        // value
        //   the argument after a flag that takes one.
        auto value = [&](std::string& _into)
        {
            if (i + 1 >= _argc)
            {
                std::fprintf(stderr, "bitwisedom-gen: %s needs a value\n",
                             each.c_str());

                return false;
            }

            _into = _argv[++i];

            return true;
        };

        bool ok = true;

        if ( (each == "-o") ||
             (each == "--out") )
        {
            ok = value(_options.out);
        }
        else if (each == "--example")   { ok = value(_options.example);   }
        else if (each == "--self-test") { ok = value(_options.self_test); }
        else if (each == "--bitfield")  { ok = value(_options.bitfield);  }
        else if (each == "--export")    { ok = value(_options.everything); }
        else if (each == "--lang")      { ok = value(_options.lang);      }
        else if (each == "--form")      { ok = value(_options.form);      }
        else if (each == "--level")     { ok = value(_options.level);     }
        else if (each == "--style")     { ok = value(_options.style);     }
        else if (each == "--word")      { ok = value(_options.word);      }
        else if (each == "--date")      { ok = value(_options.date);      }
        else if (each == "--check")     { _options.check = true;          }
        else if ( (!each.empty()) &&
                  (each[0] != '-') &&
                  (_options.sheet_path.empty()) )
        {
            _options.sheet_path = each;
        }
        else
        {
            std::fprintf(stderr, "bitwisedom-gen: unknown argument '%s'\n%s",
                         each.c_str(), USAGE);

            return false;
        }

        if (!ok)
        {
            return false;
        }
    }

    if (_options.sheet_path.empty() == _options.example.empty())
    {
        std::fprintf(stderr, "bitwisedom-gen: name one sheet file, or one "
                             "--example\n%s", USAGE);

        return false;
    }

    return true;
}

// apply_overrides
//   the command line's generator settings over the sheet's.
bool
apply_overrides(
    const options& _options,
    gen_settings&  _gen
)
{
    if (!_options.lang.empty())
    {
        _gen.lang = (_options.lang == "cpp") ? language::cpp : language::c;
    }

    if (!_options.form.empty())
    {
        _gen.form = (_options.form == "constants") ? gen_form::constants
                                                   : gen_form::macros;
    }

    if (!_options.style.empty())
    {
        _gen.style = (_options.style == "plain") ? gen_style::plain
                                                 : gen_style::djinterp;
    }

    if (!_options.level.empty())
    {
        bool found = false;

        for (int i = 0; i < STANDARD_COUNT; ++i)
        {
            const standard  level = static_cast<standard>(i);
            std::string     name  = standard_name(level);

            // "C++17" and "cpp17", "C99" and "c99"
            for (char& each : name)
            {
                each = (each == '+') ? 'p'
                                     : static_cast<char>(each | 0x20);
            }

            if (name == _options.level)
            {
                _gen.level = level;
                found      = true;
            }
        }

        if (!found)
        {
            std::fprintf(stderr, "bitwisedom-gen: unknown level '%s'\n",
                         _options.level.c_str());

            return false;
        }
    }

    if (!_options.word.empty())
    {
        if ( (_options.word != "8")  &&
             (_options.word != "16") &&
             (_options.word != "32") &&
             (_options.word != "64") )
        {
            std::fprintf(stderr, "bitwisedom-gen: a word is 8, 16, 32 or 64 "
                                 "bits, not '%s'\n",
                         _options.word.c_str());

            return false;
        }

        _gen.word = std::atoi(_options.word.c_str());
    }

    // a C++ level means a C++ header
    if (is_cpp(_gen.level))
    {
        _gen.lang = language::cpp;
    }

    return true;
}

int
report(
    const sheet&      _sheet,
    const resolution& _resolution
)
{
    for (const issue& each : _resolution.issues)
    {
        const resolved_row* where = _resolution.find(each.row);
        const char*         level =
            (each.level == severity::error)   ? "error"
          : (each.level == severity::warning) ? "warning"
                                              : "note";

        std::fprintf(stderr, "%s: %s: %s: %s\n",
                     _sheet.title.c_str(),
                     (where != nullptr) ? where->full.c_str() : "?",
                     level,
                     each.message.c_str());
    }

    return (_resolution.errors == 0) ? 0 : 1;
}

}  // anonymous namespace


int
main(
    int    _argc,
    char** _argv
)
{
    const std::string first = (_argc > 1) ? _argv[1] : "";

    if ( (first == "--help") ||
         (first == "-h") )
    {
        std::fputs(USAGE, stdout);

        return 0;
    }

    if (first == "--list-examples")
    {
        for (const std::string& name : example_names())
        {
            std::printf("%s\n", name.c_str());
        }

        return 0;
    }

    if (first == "--save-example")
    {
        std::string error;

        if (_argc != 4)
        {
            std::fputs(USAGE, stderr);

            return 2;
        }

        if (!write_file(_argv[3], save_sheet(make_example(_argv[2])), error))
        {
            std::fprintf(stderr, "bitwisedom-gen: %s\n", error.c_str());

            return 2;
        }

        return 0;
    }

    options chosen;

    if (!parse_arguments(_argc, _argv, chosen))
    {
        return 2;
    }

    sheet       loaded;
    std::string error;

    if (!chosen.example.empty())
    {
        loaded = make_example(chosen.example);
    }
    else
    {
        std::string text;

        if (!read_file(chosen.sheet_path, text, error))
        {
            std::fprintf(stderr, "bitwisedom-gen: %s: %s\n",
                         chosen.sheet_path.c_str(), error.c_str());

            return 2;
        }

        // a sheet file, a bitfield or an export
        json_read read = read_json_sheet(text);

        for (const std::string& note : read.notes)
        {
            std::fprintf(stderr, "%s: note: %s\n",
                         chosen.sheet_path.c_str(), note.c_str());
        }

        if (!read.ok)
        {
            std::fprintf(stderr, "bitwisedom-gen: %s: %s\n",
                         chosen.sheet_path.c_str(), read.error.c_str());

            return 2;
        }

        loaded = std::move(read.result);
    }

    if (!apply_overrides(chosen, loaded.gen))
    {
        return 2;
    }

    const resolution resolved = resolve(loaded);
    const int        status   = report(loaded, resolved);

    if (chosen.check)
    {
        return status;
    }

    // the header is named for the file it is written to
    if (!chosen.out.empty())
    {
        const std::size_t slash = chosen.out.find_last_of("/\\");

        loaded.gen.file_name = (slash == std::string::npos)
                                   ? chosen.out
                                   : chosen.out.substr(slash + 1);
    }

    const generated header = generate(loaded,
                                      resolved,
                                      chosen.date.empty() ? today()
                                                          : chosen.date);

    for (const std::string& note : header.notes)
    {
        std::fprintf(stderr, "%s: note: %s\n",
                     loaded.title.c_str(), note.c_str());
    }

    const bool json_asked = ( (!chosen.bitfield.empty()) ||
                              (!chosen.everything.empty()) );

    if ( (!chosen.bitfield.empty()) &&
         (!write_file(chosen.bitfield, write_bitfield(loaded), error)) )
    {
        std::fprintf(stderr, "bitwisedom-gen: %s\n", error.c_str());

        return 2;
    }

    if ( (!chosen.everything.empty()) &&
         (!write_file(chosen.everything,
                      write_export(loaded, resolved, header),
                      error)) )
    {
        std::fprintf(stderr, "bitwisedom-gen: %s\n", error.c_str());

        return 2;
    }

    if (chosen.out.empty())
    {
        if (!json_asked)
        {
            std::fputs(header.text.c_str(), stdout);
        }
    }
    else if (!write_file(chosen.out, header.text, error))
    {
        std::fprintf(stderr, "bitwisedom-gen: %s\n", error.c_str());

        return 2;
    }

    if ( (!chosen.self_test.empty()) &&
         (!write_file(chosen.self_test,
                      is_wide(loaded.width)
                          ? wide_self_test(loaded, resolved,
                                           loaded.gen.file_name)
                          : self_test(loaded, resolved, loaded.gen.file_name),
                      error)) )
    {
        std::fprintf(stderr, "bitwisedom-gen: %s\n", error.c_str());

        return 2;
    }

    return status;
}
