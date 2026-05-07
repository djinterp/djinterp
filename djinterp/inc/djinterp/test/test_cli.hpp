/******************************************************************************
* djinterp [test]                                              test_cli.hpp
*
* CLI adapter for the C++ test framework.
*   Parses command-line arguments (argc/argv) into a test_options
* instance. The mapping between CLI flag names and option keys is
* defined by a cli_option_table - a flat array of cli_option_def
* entries that the user can declare in a single statement and
* customize per module.
*
*   The backing container for cli_option_table is templated so
* that any djinterp container (or standard container) satisfying
* the sequential protocol can be used in place of std::vector.
* The sequential protocol requires:
*   - push_back(element)
*   - operator[](index) or at(index)
*   - size() -> integral
*   - empty() -> bool
*   - begin() / end() (for range-based for)
*
*   Parse functions are templated on the test_options map type
* and the cli_option_table container type, so any backing
* container parameterization is accepted without modification.
*
* USAGE:
*   // 1. declare a table (one statement)
*   static const auto my_cli = test::cli_option_defs({
*       { "name",    'n', D_TEST_OPT_NAME,      D_OPTION_TYPE_STRING, "test name"    },
*       { "verbose", 'v', D_TEST_OPT_VERBOSITY,  D_OPTION_TYPE_INT,   "verbosity"    },
*       { "color",    0,  D_TEST_OPT_COLOR,      D_OPTION_TYPE_BOOL,  "enable colors" },
*       { "shuffle",  0,  D_TEST_OPT_SHUFFLE,    D_OPTION_TYPE_BOOL,  "shuffle tests" },
*   });
*
*   // 2. parse in one statement
*   auto [opts, result] = test::cli_parse(argc, argv, my_cli);
*
*   // -- or parse into an existing test_options --
*   auto opts = test::test_options_default();
*   auto result = test::cli_parse_into(argc, argv, my_cli, opts);
*
*   // -- or use the framework default table --
*   auto [opts, result] = test::cli_parse(argc, argv);
*
* CLI SYNTAX:
*   --key value       long option with separate value
*   --key=value       long option with D_INLINE value
*   -k value          short option with separate value
*   --flag            boolean toggle (sets to true)
*   --no-flag         boolean negation (sets to false)
*   --help / -h       prints help and returns help_requested
*
* COMPONENTS:
*   test::cli_option_def     - single option definition
*   test::cli_option_table   - array of definitions (template)
*   test::DCliParseStatus    - parse result code
*   test::cli_parse_result   - parse result with details
*   test::cli_parse          - parse into new test_options
*   test::cli_parse_into     - parse into existing test_options
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/cpp/test/test_cli.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.13
******************************************************************************/

#ifndef DJINTERP_TEST_CLI_
#define DJINTERP_TEST_CLI_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_options.hpp"


NS_DJINTERP
NS_TEST


// =========================================================================
// I.   OPTION DEFINITION
// =========================================================================

// cli_option_def
//   struct: describes a single CLI-accessible option. Maps a
// long name string and optional short character to an option_key
// and value_type_id from test_options.hpp.
struct cli_option_def
{
    const char*   long_name;   // long option name (without leading dashes)
    char          short_flag;  // single-character short flag; 0 = no short flag
    option_key    key;         // target option_key in the test_options registry
    value_type_id type;        // expected value type (D_OPTION_TYPE_BOOL, _INT, etc.)
    const char*   help;        // help text description for --help output
};


// =========================================================================
// Ia.  SEQUENTIAL PROTOCOL DETECTION
// =========================================================================

NS_INTERNAL

    // seq_has_push_back
    //   trait: true if _Seq supports push_back(element).
    template<typename _Seq,
             typename = void>
    struct seq_has_push_back : std::false_type
    {};

    template<typename _Seq>
    struct seq_has_push_back<_Seq,
        void_t<decltype(
            std::declval<_Seq&>().push_back(
                std::declval<typename _Seq::value_type>()))>>
        : std::true_type
    {};

    // seq_has_size
    //   trait: true if _Seq supports size().
    template<typename _Seq,
             typename = void>
    struct seq_has_size : std::false_type
    {};

    template<typename _Seq>
    struct seq_has_size<_Seq,
        void_t<decltype(
            std::declval<const _Seq&>().size())>>
        : std::true_type
    {};

    // seq_has_empty
    //   trait: true if _Seq supports empty().
    template<typename _Seq,
             typename = void>
    struct seq_has_empty : std::false_type
    {};

    template<typename _Seq>
    struct seq_has_empty<_Seq,
        void_t<decltype(
            std::declval<const _Seq&>().empty())>>
        : std::true_type
    {};

    // seq_has_subscript
    //   trait: true if _Seq supports operator[](index).
    template<typename _Seq,
             typename = void>
    struct seq_has_subscript : std::false_type
    {};

    template<typename _Seq>
    struct seq_has_subscript<_Seq,
        void_t<decltype(
            std::declval<const _Seq&>()[
                std::declval<std::size_t>()])>>
        : std::true_type
    {};

    // seq_is_iterable
    //   trait: true if _Seq supports begin()/end().
    template<typename _Seq,
             typename = void>
    struct seq_is_iterable : std::false_type
    {};

    template<typename _Seq>
    struct seq_is_iterable<_Seq,
        void_t<
            decltype(std::declval<const _Seq&>().begin()),
            decltype(std::declval<const _Seq&>().end())>>
        : std::true_type
    {};

NS_END  // internal


// =========================================================================
// II.  OPTION TABLE
// =========================================================================

// cli_option_table
//   class: holds an array of cli_option_def entries. Provides
// lookup by long name, short flag, or option_key.
//
// Template parameters:
//   _Container: the sequential container used for definition
//     storage (default: std::vector<cli_option_def>). Must
//     support push_back, size, empty, operator[], and
//     begin/end iteration. Any djinterp container with these
//     capabilities may be used.
template<typename _Container = std::vector<cli_option_def>>
class cli_option_table
{
    static_assert(
        internal::seq_has_push_back<_Container>::value,
        "_Container must support "
        "push_back(value_type).");

    static_assert(
        internal::seq_has_size<_Container>::value,
        "_Container must support size().");

    static_assert(
        internal::seq_has_empty<_Container>::value,
        "_Container must support empty().");

    static_assert(
        internal::seq_has_subscript<_Container>::value,
        "_Container must support "
        "operator[](index).");

public:
    using container_type = _Container;

    cli_option_table()
    {};

    // construct from initializer list
    cli_option_table(
        std::initializer_list<cli_option_def> _defs)
        : m_defs(_defs)
    {};

    // construct from container
    cli_option_table(const _Container& _defs)
        : m_defs(_defs)
    {};

    // construct from move
    cli_option_table(_Container&& _defs)
        : m_defs(std::move(_defs))
    {};

    // ---- lookup ----

    // find_by_long
    //   returns a pointer to the def with the given long name,
    // or nullptr if not found.
    const cli_option_def* find_by_long(const char* _name) const
    {
        if (!_name)
        {
            return nullptr;
        }

        for (std::size_t i = 0; i < m_defs.size(); ++i)
        {
            if ( (m_defs[i].long_name) &&
                 (std::strcmp(m_defs[i].long_name,
                              _name) == 0) )
            {
                return &m_defs[i];
            }
        }

        return nullptr;
    };

    // find_by_short
    //   returns a pointer to the def with the given short flag,
    // or nullptr if not found.
    const cli_option_def* find_by_short(char _flag) const
    {
        if (_flag == 0)
        {
            return nullptr;
        }

        for (std::size_t i = 0; i < m_defs.size(); ++i)
        {
            if (m_defs[i].short_flag == _flag)
            {
                return &m_defs[i];
            }
        }

        return nullptr;
    };

    // find_by_key
    //   returns a pointer to the def with the given option_key,
    // or nullptr if not found.
    const cli_option_def* find_by_key(option_key _key) const
    {
        for (std::size_t i = 0; i < m_defs.size(); ++i)
        {
            if (m_defs[i].key == _key)
            {
                return &m_defs[i];
            }
        }

        return nullptr;
    };

    // ---- queries ----

    // size
    //   returns the number of definitions.
    std::size_t size() const
    {
        return m_defs.size();
    };

    // empty
    //   returns true if there are no definitions.
    bool empty() const
    {
        return m_defs.empty();
    };

    // at
    //   returns a const reference to the def at the given index.
    const cli_option_def& at(std::size_t _index) const
    {
        return m_defs[_index];
    };

    // ---- mutation ----

    // add
    //   appends a definition.
    void add(const cli_option_def& _def)
    {
        m_defs.push_back(_def);
    };

    // merge
    //   appends all definitions from another table. Entries
    // with duplicate long names are skipped. Templated on
    // the other table's container type so that tables backed
    // by different djinterp containers can be merged.
    template<typename _OtherContainer>
    void merge(
        const cli_option_table<_OtherContainer>& _other)
    {
        for (std::size_t i = 0; i < _other.size(); ++i)
        {
            const cli_option_def& def = _other.at(i);

            if (!find_by_long(def.long_name))
            {
                m_defs.push_back(def);
            }
        }
    };

    // ---- backing container access ----

    // defs
    //   returns a const reference to the underlying
    // container for inspection or integration with
    // container-aware algorithms.
    const _Container& defs() const
    {
        return m_defs;
    };

private:
    _Container m_defs;
};

// cli_option_defs
//   convenience: constructs a cli_option_table from a brace-
// enclosed initializer list. Enables one-statement declaration:
//   static const auto my_cli = cli_option_defs({ ... });
D_INLINE cli_option_table<>
cli_option_defs
(
    std::initializer_list<cli_option_def> _defs
)
{
    return cli_option_table<>(_defs);
}


// =========================================================================
// III. PARSE RESULT
// =========================================================================

// DCliParseStatus
//   constants: outcome codes for CLI parsing.
static constexpr std::int32_t D_CLI_PARSE_OK             = 0;
static constexpr std::int32_t D_CLI_PARSE_HELP_REQUESTED = 1;
static constexpr std::int32_t D_CLI_PARSE_ERROR_UNKNOWN  = -1;
static constexpr std::int32_t D_CLI_PARSE_ERROR_MISSING  = -2;
static constexpr std::int32_t D_CLI_PARSE_ERROR_CONVERT  = -3;

// cli_parse_result
//   struct: result of a CLI parse operation. On failure,
// error_arg holds the argument that caused the problem and
// message provides a human-readable description.
struct cli_parse_result
{
    std::int32_t status;
    std::string  error_arg;
    std::string  message;
};


// =========================================================================
// IV.  STRING-TO-VALUE CONVERSION
// =========================================================================

NS_INTERNAL

    // str_to_bool
    //   converts a string to a boolean. Recognizes:
    // true/1/yes/on -> true, false/0/no/off -> false.
    // returns: true if conversion succeeded.
    D_INLINE bool
    str_to_bool
    (
        const char* _str,
        bool&       _out
    )
    {
        if ( (!_str) ||
             (_str[0] == '\0') )
        {
            return false;
        }

        // true variants
        if ( (std::strcmp(_str, "true")  == 0) ||
             (std::strcmp(_str, "1")     == 0) ||
             (std::strcmp(_str, "yes")   == 0) ||
             (std::strcmp(_str, "on")    == 0) )
        {
            _out = true;

            return true;
        }

        // false variants
        if ( (std::strcmp(_str, "false") == 0) ||
             (std::strcmp(_str, "0")     == 0) ||
             (std::strcmp(_str, "no")    == 0) ||
             (std::strcmp(_str, "off")   == 0) )
        {
            _out = false;

            return true;
        }

        return false;
    }

    // str_to_int64
    //   converts a string to int64_t via strtoll.
    // returns: true if conversion succeeded.
    D_INLINE bool
    str_to_int64
    (
        const char*   _str,
        std::int64_t& _out
    )
    {
        if ( (!_str) ||
             (_str[0] == '\0') )
        {
            return false;
        }

        char* end = nullptr;
        long long val = std::strtoll(_str, &end, 0);

        if ( (!end) ||
             (*end != '\0') )
        {
            return false;
        }

        _out = static_cast<std::int64_t>(val);

        return true;
    }

    // str_to_uint64
    //   converts a string to uint64_t via strtoull.
    // returns: true if conversion succeeded.
    D_INLINE bool
    str_to_uint64
    (
        const char*    _str,
        std::uint64_t& _out
    )
    {
        if ( (!_str) ||
             (_str[0] == '\0') )
        {
            return false;
        }

        char* end = nullptr;
        unsigned long long val = std::strtoull(_str, &end, 0);

        if ( (!end) ||
             (*end != '\0') )
        {
            return false;
        }

        _out = static_cast<std::uint64_t>(val);

        return true;
    }

    // str_to_double
    //   converts a string to double via strtod.
    // returns: true if conversion succeeded.
    D_INLINE bool
    str_to_double
    (
        const char* _str,
        double&     _out
    )
    {
        if ( (!_str) ||
             (_str[0] == '\0') )
        {
            return false;
        }

        char* end = nullptr;
        double val = std::strtod(_str, &end);

        if ( (!end) ||
             (*end != '\0') )
        {
            return false;
        }

        _out = val;

        return true;
    }

    // set_value_from_string
    //   converts a string to the appropriate option_value type
    // and sets it on the given test_options instance. Templated
    // on the options map type.
    template<typename _Map>
    D_INLINE bool
    set_value_from_string
    (
        test_options<_Map>&    _opts,
        const cli_option_def& _def,
        const char*           _str
    )
    {
        switch (_def.type)
        {
            case D_OPTION_TYPE_BOOL:
            {
                bool val = false;

                if (!str_to_bool(_str, val))
                {
                    return false;
                }

                return _opts.set(_def.key,
                                 option_value::from_bool(val));
            }

            case D_OPTION_TYPE_INT:
            {
                std::int64_t val = 0;

                if (!str_to_int64(_str, val))
                {
                    return false;
                }

                return _opts.set(_def.key,
                                 option_value::from_int(val));
            }

            case D_OPTION_TYPE_UINT:
            {
                std::uint64_t val = 0;

                if (!str_to_uint64(_str, val))
                {
                    return false;
                }

                return _opts.set(_def.key,
                                 option_value::from_uint(val));
            }

            case D_OPTION_TYPE_DOUBLE:
            {
                double val = 0.0;

                if (!str_to_double(_str, val))
                {
                    return false;
                }

                return _opts.set(_def.key,
                                 option_value::from_double(val));
            }

            case D_OPTION_TYPE_STRING:
            {
                return _opts.set(_def.key,
                                 option_value::from_string(_str));
            }

            default:
            {
                return false;
            }
        }
    }

NS_END  // internal


// =========================================================================
// V.   HELP FORMATTER
// =========================================================================

// cli_format_help
//   generates a help string from the given option table.
// Templated on the table's container type.
template<typename _Container>
D_INLINE std::string
cli_format_help
(
    const cli_option_table<_Container>& _table,
    const char*                         _program_name = nullptr
)
{
    std::string out;

    if (_program_name)
    {
        out += "usage: ";
        out += _program_name;
        out += " [options]\n\n";
    }

    out += "options:\n";

    for (std::size_t i = 0; i < _table.size(); ++i)
    {
        const cli_option_def& def = _table.at(i);

        out += "  ";

        // short flag
        if (def.short_flag != 0)
        {
            out += '-';
            out += def.short_flag;
            out += ", ";
        }
        else
        {
            out += "    ";
        }

        // long name
        out += "--";
        out += def.long_name ? def.long_name : "?";

        // padding to column 30
        std::size_t col = out.size();
        std::size_t last_newline = out.rfind('\n');

        if (last_newline != std::string::npos)
        {
            col = col - last_newline - 1;
        }

        while (col < 30)
        {
            out += ' ';
            ++col;
        }

        // help text
        if (def.help)
        {
            out += def.help;
        }

        out += '\n';
    }

    return out;
}


// =========================================================================
// VI.  PARSER
// =========================================================================

// cli_parse_into
//   parses argc/argv against the given option table, writing
// values into the given test_options instance. Options not
// found in the table are reported as errors. Boolean options
// can be toggled with --flag (true) or --no-flag (false)
// without a value argument.
//
// Templated on both the table container type and the options
// map type so that any djinterp backing container is accepted.
template<typename _Container,
         typename _Map>
D_INLINE cli_parse_result
cli_parse_into
(
    int                                 _argc,
    const char* const*                  _argv,
    const cli_option_table<_Container>& _table,
    test_options<_Map>&                 _opts
)
{
    cli_parse_result result;
    result.status = D_CLI_PARSE_OK;

    int i = 1;

    while (i < _argc)
    {
        const char* arg = _argv[i];

        // skip empty args
        if ( (!arg) ||
             (arg[0] == '\0') )
        {
            ++i;

            continue;
        }

        // ---- long options (--name) ----
        if ( (arg[0] == '-') &&
             (arg[1] == '-') )
        {
            const char* name = arg + 2;

            // --help
            if (std::strcmp(name, "help") == 0)
            {
                result.status  = D_CLI_PARSE_HELP_REQUESTED;
                result.message = cli_format_help(_table,
                                                 _argv[0]);

                return result;
            }

            // check for --no-flag (boolean negation)
            bool is_negation = false;

            if ( (name[0] == 'n') &&
                 (name[1] == 'o') &&
                 (name[2] == '-') )
            {
                const cli_option_def* def =
                    _table.find_by_long(name + 3);

                if ( (def) &&
                     (def->type == D_OPTION_TYPE_BOOL) )
                {
                    _opts.set(def->key,
                              option_value::from_bool(false));
                    ++i;
                    is_negation = true;
                }
            }

            if (is_negation)
            {
                continue;
            }

            // check for --key=value
            const char* eq = std::strchr(name, '=');
            std::string key_str;
            const char* inline_value = nullptr;

            if (eq)
            {
                key_str      = std::string(name, eq);
                inline_value = eq + 1;
                name         = key_str.c_str();
            }

            const cli_option_def* def =
                _table.find_by_long(name);

            if (!def)
            {
                result.status    = D_CLI_PARSE_ERROR_UNKNOWN;
                result.error_arg = arg;
                result.message   = "unknown option: ";
                result.message  += arg;

                return result;
            }

            // boolean flags: no value needed (toggle true)
            if ( (def->type == D_OPTION_TYPE_BOOL) &&
                 (!inline_value) )
            {
                // peek: if next arg looks like a bool value,
                // consume it; otherwise just toggle true
                if ( (i + 1 < _argc) &&
                     (_argv[i + 1]) &&
                     (_argv[i + 1][0] != '-') )
                {
                    bool probe = false;

                    if (internal::str_to_bool(
                            _argv[i + 1], probe))
                    {
                        _opts.set(def->key,
                                  option_value::from_bool(
                                      probe));
                        i += 2;

                        continue;
                    }
                }

                _opts.set(def->key,
                          option_value::from_bool(true));
                ++i;

                continue;
            }

            // non-boolean: require a value
            const char* val_str = inline_value;

            if (!val_str)
            {
                if (i + 1 >= _argc)
                {
                    result.status    = D_CLI_PARSE_ERROR_MISSING;
                    result.error_arg = arg;
                    result.message   = "missing value for: ";
                    result.message  += arg;

                    return result;
                }

                val_str = _argv[i + 1];
                ++i;
            }

            if (!internal::set_value_from_string(
                    _opts, *def, val_str))
            {
                result.status    = D_CLI_PARSE_ERROR_CONVERT;
                result.error_arg = arg;
                result.message   = "invalid value for ";
                result.message  += arg;
                result.message  += ": ";
                result.message  += val_str;

                return result;
            }

            ++i;

            continue;
        }

        // ---- short options (-k) ----
        if (arg[0] == '-')
        {
            char flag = arg[1];

            // -h
            if (flag == 'h')
            {
                result.status  = D_CLI_PARSE_HELP_REQUESTED;
                result.message = cli_format_help(_table,
                                                 _argv[0]);

                return result;
            }

            const cli_option_def* def =
                _table.find_by_short(flag);

            if (!def)
            {
                result.status    = D_CLI_PARSE_ERROR_UNKNOWN;
                result.error_arg = arg;
                result.message   = "unknown option: ";
                result.message  += arg;

                return result;
            }

            // value might be glued: -v3 or separate: -v 3
            const char* val_str = nullptr;

            if (arg[2] != '\0')
            {
                // glued: -v3
                val_str = arg + 2;
            }
            else if (def->type == D_OPTION_TYPE_BOOL)
            {
                // bool flag: no value, toggle true
                _opts.set(def->key,
                          option_value::from_bool(true));
                ++i;

                continue;
            }
            else if (i + 1 < _argc)
            {
                // separate: -v 3
                val_str = _argv[i + 1];
                ++i;
            }
            else
            {
                result.status    = D_CLI_PARSE_ERROR_MISSING;
                result.error_arg = arg;
                result.message   = "missing value for: ";
                result.message  += arg;

                return result;
            }

            if (!internal::set_value_from_string(
                    _opts, *def, val_str))
            {
                result.status    = D_CLI_PARSE_ERROR_CONVERT;
                result.error_arg = arg;
                result.message   = "invalid value for ";
                result.message  += arg;
                result.message  += ": ";
                result.message  += val_str;

                return result;
            }

            ++i;

            continue;
        }

        // ---- positional argument (ignored) ----
        ++i;
    }

    return result;
}


// =========================================================================
// VII. DEFAULT OPTION TABLE
// =========================================================================

// default_cli_option_table
//   returns the framework default CLI option table covering
// all standard option keys from test_options.hpp.
D_INLINE const cli_option_table<>&
default_cli_option_table()
{
    static const cli_option_table<> table =
    {
        // ---- core execution ----
        { "name",
           'n', D_TEST_OPT_NAME,
           D_OPTION_TYPE_STRING,
           "test session name" },

        { "enabled",
           0,   D_TEST_OPT_ENABLED,
           D_OPTION_TYPE_BOOL,
           "enable/disable execution" },

        { "skip",
           's', D_TEST_OPT_SKIP,
           D_OPTION_TYPE_BOOL,
           "skip this node" },

        { "timeout",
           't', D_TEST_OPT_TIMEOUT_MS,
           D_OPTION_TYPE_UINT,
           "timeout in milliseconds (0=none)" },

        { "max-failures",
           0,   D_TEST_OPT_MAX_FAILURES,
           D_OPTION_TYPE_UINT,
           "stop after N failures (0=unlimited)" },

        { "abort-on-failure",
           'a', D_TEST_OPT_ABORT_ON_FAILURE,
           D_OPTION_TYPE_BOOL,
           "stop on first failure" },

        { "priority",
           'p', D_TEST_OPT_PRIORITY,
           D_OPTION_TYPE_INT,
           "execution priority (higher=earlier)" },

        { "repeat",
           'r', D_TEST_OPT_REPEAT_COUNT,
           D_OPTION_TYPE_UINT,
           "repeat count (1=once)" },

        { "shuffle",
           0,   D_TEST_OPT_SHUFFLE,
           D_OPTION_TYPE_BOOL,
           "randomize execution order" },

        { "shuffle-seed",
           0,   D_TEST_OPT_SHUFFLE_SEED,
           D_OPTION_TYPE_UINT,
           "seed for shuffle randomization" },

        // ---- output and formatting ----
        { "verbosity",
           'v', D_TEST_OPT_VERBOSITY,
           D_OPTION_TYPE_INT,
           "verbosity (0=silent 1=min 2=normal 3=verbose)" },

        { "color",
           'c', D_TEST_OPT_COLOR,
           D_OPTION_TYPE_BOOL,
           "enable ANSI color output" },

        { "timestamps",
           0,   D_TEST_OPT_SHOW_TIMESTAMPS,
           D_OPTION_TYPE_BOOL,
           "show timestamps" },

        { "duration",
           'd', D_TEST_OPT_SHOW_DURATION,
           D_OPTION_TYPE_BOOL,
           "show elapsed time per node" },

        { "indent",
           0,   D_TEST_OPT_INDENT_STR,
           D_OPTION_TYPE_STRING,
           "indentation string per level" },

        { "indent-max",
           0,   D_TEST_OPT_INDENT_MAX_LEVEL,
           D_OPTION_TYPE_UINT,
           "maximum indentation depth" },

        { "output-file",
           'o', D_TEST_OPT_OUTPUT_FILE,
           D_OPTION_TYPE_STRING,
           "write output to file" },

        // ---- filtering ----
        { "include",
           'i', D_TEST_OPT_FILTER_INCLUDE,
           D_OPTION_TYPE_STRING,
           "include pattern" },

        { "exclude",
           'x', D_TEST_OPT_FILTER_EXCLUDE,
           D_OPTION_TYPE_STRING,
           "exclude pattern" },

        { "filter-tags",
           0,   D_TEST_OPT_FILTER_TAGS,
           D_OPTION_TYPE_STRING,
           "comma-separated tag filter" },

        // ---- reporting ----
        { "report-passed",
           0,   D_TEST_OPT_REPORT_PASSED,
           D_OPTION_TYPE_BOOL,
           "include passing nodes in output" },

        { "report-skipped",
           0,   D_TEST_OPT_REPORT_SKIPPED,
           D_OPTION_TYPE_BOOL,
           "include skipped nodes in output" },

        { "report-summary",
           0,   D_TEST_OPT_REPORT_SUMMARY,
           D_OPTION_TYPE_BOOL,
           "print summary after execution" },
    };

    return table;
}


// =========================================================================
// VIII. CONVENIENCE PARSE FUNCTIONS
// =========================================================================

// cli_parse
//   parses argc/argv against the given option table into a
// new test_options instance. Returns a pair: the populated
// options and the parse result. Templated on both the table
// container type and the options map type.
template<typename _Container = std::vector<cli_option_def>,
         typename _Map       = std::unordered_map<
             option_key, option_entry>>
D_INLINE std::pair<test_options<_Map>, cli_parse_result>
cli_parse
(
    int                                 _argc,
    const char* const*                  _argv,
    const cli_option_table<_Container>& _table
)
{
    test_options<_Map> opts = test_options_default<_Map>();
    cli_parse_result result = cli_parse_into(
        _argc, _argv, _table, opts);

    return std::make_pair(std::move(opts), std::move(result));
}

// cli_parse (default table)
//   parses argc/argv using the framework default option table.
template<typename _Map = std::unordered_map<
             option_key, option_entry>>
D_INLINE std::pair<test_options<_Map>, cli_parse_result>
cli_parse
(
    int                _argc,
    const char* const* _argv
)
{
    return cli_parse<std::vector<cli_option_def>, _Map>(
        _argc, _argv,
        default_cli_option_table());
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_CLI_
