/******************************************************************************
* djinterp [test]                                          test_options.hpp
*
* Registry-based configuration for the C++ test framework.
*   Provides a typed key-value store for test configuration and metadata.
* Each option is identified by an int64_t key (option_key) so users can
* define their own option enums; the framework provides a standard set
* of keys for common settings. Values are stored in a tagged union
* (option_value) supporting bool, int64, uint64, double, string, and
* void* types.
*
*   The registry pattern from the C test_cvar.h / registry.h is preserved:
* each entry carries a string name (for CLI mapping), a current value, a
* default value, and help text. The test_options class owns the storage
* and provides typed get/set accessors, string-name lookup for CLI
* integration, and reset-to-defaults.
*
* COMPONENTS:
*   djinterp::test::option_key         - option identifier (int64_t alias)
*   djinterp::test::value_type_id     - value type discriminator (int32_t)
*   djinterp::test::option_value       - tagged value union
*   djinterp::test::option_entry       - registered option with metadata
*   djinterp::test::test_options       - configuration registry
*
* STANDARD OPTION KEYS:
*   Keys 0x001 - 0x0FF: core execution options
*   Keys 0x100 - 0x1FF: output and formatting
*   Keys 0x200 - 0x2FF: filtering
*   Keys 0x300 - 0x3FF: reporting and metadata
*   Keys >= D_TEST_OPT_USER_START (0x1000): user-defined
*
* REPLACES:
*   C headers test_options.h, test_cvar.h. The bitfield flag system
*   (DTestMessageFlag, DTestSettingsFlag) is replaced by individual
*   boolean options. The d_min_enum_map and d_registry backing stores
*   are replaced by std::unordered_map.
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/cpp/test/test_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.13
******************************************************************************/

#ifndef DJINTERP_TEST_OPTIONS_
#define DJINTERP_TEST_OPTIONS_ 1

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "test_common.hpp"


NS_DJINTERP
NS_TEST


// =========================================================================
// I.   OPTION KEY TYPE
// =========================================================================

// option_key
//   type: identifier for a configuration option. Defined as
// int64_t so users can specify their own option enums backed
// by this type. The framework provides standard key constants;
// user-defined keys should start at D_TEST_OPT_USER_START.
using option_key = std::int64_t;

// D_TEST_OPT_NONE
//   constant: sentinel key representing no option.
static constexpr option_key D_TEST_OPT_NONE = 0;

// D_TEST_OPT_USER_START
//   constant: first key available for user-defined options.
// All values below this are reserved for framework use.
static constexpr option_key D_TEST_OPT_USER_START = 0x1000;


// =========================================================================
// II.  OPTION VALUE TYPE
// =========================================================================

// value_type_id
//   type: discriminator for the type stored in an option_value.
// Defined as int32_t so users can extend with their own type
// tags (e.g. d_type_info16 values from type_info.h). The
// framework provides constants for the built-in storage
// categories; user-defined type ids should start at
// D_OPTION_TYPE_USER_START.
using value_type_id = std::int32_t;

// ---- framework-reserved type constants ----

static constexpr value_type_id D_OPTION_TYPE_NONE   = 0;
static constexpr value_type_id D_OPTION_TYPE_BOOL   = 1;
static constexpr value_type_id D_OPTION_TYPE_INT    = 2;
static constexpr value_type_id D_OPTION_TYPE_UINT   = 3;
static constexpr value_type_id D_OPTION_TYPE_DOUBLE = 4;
static constexpr value_type_id D_OPTION_TYPE_STRING = 5;
static constexpr value_type_id D_OPTION_TYPE_PTR    = 6;

// D_OPTION_TYPE_USER_START
//   constant: first value available for user-defined type ids.
// All values below this are reserved for framework use.
static constexpr value_type_id D_OPTION_TYPE_USER_START = 0x100;


// =========================================================================
// III. OPTION VALUE
// =========================================================================

// option_value
//   class: tagged value union supporting the types needed for
// test configuration. Construction uses static factory methods
// to avoid overload ambiguity. Typed accessors return a caller-
// supplied default when the stored type does not match.
//
//   String values are heap-managed via std::string; all other
// types are stored in a POD union. Copy and move operations
// are safe (compiler-generated; std::string handles itself).
class option_value
{
public:
    option_value()
        : m_type(D_OPTION_TYPE_NONE)
        , m_pod()
        , m_string()
    {
        m_pod.i = 0;
    };

    // ---- factory methods ----

    // from_bool
    //   creates a boolean option value.
    static option_value from_bool(bool _value)
    {
        option_value v;
        v.m_type  = D_OPTION_TYPE_BOOL;
        v.m_pod.b = _value;

        return v;
    };

    // from_int
    //   creates a signed 64-bit integer option value.
    static option_value from_int(std::int64_t _value)
    {
        option_value v;
        v.m_type  = D_OPTION_TYPE_INT;
        v.m_pod.i = _value;

        return v;
    };

    // from_uint
    //   creates an unsigned 64-bit integer option value.
    static option_value from_uint(std::uint64_t _value)
    {
        option_value v;
        v.m_type  = D_OPTION_TYPE_UINT;
        v.m_pod.u = _value;

        return v;
    };

    // from_double
    //   creates a double-precision floating point option value.
    static option_value from_double(double _value)
    {
        option_value v;
        v.m_type  = D_OPTION_TYPE_DOUBLE;
        v.m_pod.d = _value;

        return v;
    };

    // from_string
    //   creates a string option value (copies the input).
    static option_value from_string(const char* _value)
    {
        option_value v;
        v.m_type   = D_OPTION_TYPE_STRING;
        v.m_string = _value ? _value : "";

        return v;
    };

    // from_string
    //   creates a string option value (copies the input).
    static option_value from_string(const std::string& _value)
    {
        option_value v;
        v.m_type   = D_OPTION_TYPE_STRING;
        v.m_string = _value;

        return v;
    };

    // from_ptr
    //   creates a void pointer option value. The option_value
    // does not own the pointed-to memory.
    static option_value from_ptr(void* _value)
    {
        option_value v;
        v.m_type  = D_OPTION_TYPE_PTR;
        v.m_pod.p = _value;

        return v;
    };

    // ---- type query ----

    // type
    //   returns the type discriminator for this value.
    value_type_id type() const
    {
        return m_type;
    };

    // is_none
    //   returns true if this value has no type assigned.
    bool is_none() const
    {
        return (m_type == D_OPTION_TYPE_NONE);
    };

    // ---- typed accessors ----

    // as_bool
    //   returns the boolean value, or _default if the stored
    // type is not bool.
    bool as_bool(bool _default = false) const
    {
        if (m_type != D_OPTION_TYPE_BOOL)
        {
            return _default;
        }

        return m_pod.b;
    };

    // as_int
    //   returns the int64 value, or _default if the stored
    // type is not int.
    std::int64_t as_int(std::int64_t _default = 0) const
    {
        if (m_type != D_OPTION_TYPE_INT)
        {
            return _default;
        }

        return m_pod.i;
    };

    // as_uint
    //   returns the uint64 value, or _default if the stored
    // type is not uint.
    std::uint64_t as_uint(std::uint64_t _default = 0) const
    {
        if (m_type != D_OPTION_TYPE_UINT)
        {
            return _default;
        }

        return m_pod.u;
    };

    // as_double
    //   returns the double value, or _default if the stored
    // type is not double.
    double as_double(double _default = 0.0) const
    {
        if (m_type != D_OPTION_TYPE_DOUBLE)
        {
            return _default;
        }

        return m_pod.d;
    };

    // as_string
    //   returns a const reference to the string value. If the
    // stored type is not string, returns a reference to an
    // internal empty string.
    const std::string& as_string() const
    {
        return m_string;
    };

    // as_ptr
    //   returns the void pointer, or nullptr if the stored
    // type is not ptr.
    void* as_ptr() const
    {
        if (m_type != D_OPTION_TYPE_PTR)
        {
            return nullptr;
        }

        return m_pod.p;
    };

private:
    value_type_id m_type;

    union pod_union
    {
        bool           b;
        std::int64_t   i;
        std::uint64_t  u;
        double         d;
        void*          p;
    } m_pod;

    std::string m_string;
};


// =========================================================================
// IV.  OPTION ENTRY
// =========================================================================

// option_entry
//   struct: a single registered option in the configuration
// registry. Carries the numeric key, a string name for CLI
// lookup, the current and default values, and optional help
// text for documentation.
struct option_entry
{
    option_key   key;
    std::string  name;
    option_value value;
    option_value default_value;
    std::string  help;
};


// =========================================================================
// V.   TEST OPTIONS (CONFIGURATION REGISTRY)
// =========================================================================

// test_options
//   class: registry-based configuration store. Maps option_key
// values to option_entry records. Provides typed get/set
// accessors, string-name lookup for CLI integration, default
// value registration, and reset-to-defaults.
//
//   Options must be registered before they can be set or
// queried; unregistered keys are silently ignored (set returns
// false, get returns a none-typed value).
class test_options
{
public:
    test_options()
    {
    };

    // ---- registration ----

    // register_option
    //   registers a new option with its metadata. If the key
    // is already registered, the existing entry is updated.
    // returns: true on success, false if key is D_TEST_OPT_NONE.
    bool register_option(option_key          _key,
                         const char*         _name,
                         const option_value& _default_value,
                         const char*         _help = nullptr)
    {
        if (_key == D_TEST_OPT_NONE)
        {
            return false;
        }

        option_entry entry;
        entry.key           = _key;
        entry.name          = _name ? _name : "";
        entry.value         = _default_value;
        entry.default_value = _default_value;
        entry.help          = _help ? _help : "";

        m_entries[_key] = entry;

        return true;
    };

    // ---- raw value access ----

    // get
    //   returns the current value for the given key, or a
    // none-typed value if the key is not registered.
    option_value get(option_key _key) const
    {
        auto it = m_entries.find(_key);

        if (it == m_entries.end())
        {
            return option_value();
        }

        return it->second.value;
    };

    // set
    //   sets the current value for the given key.
    // returns: true if the key was registered, false otherwise.
    bool set(option_key          _key,
             const option_value& _value)
    {
        auto it = m_entries.find(_key);

        if (it == m_entries.end())
        {
            return false;
        }

        it->second.value = _value;

        return true;
    };

    // ---- typed convenience getters ----

    bool get_bool(option_key _key,
                  bool       _default = false) const
    {
        return get(_key).as_bool(_default);
    };

    std::int64_t get_int(option_key   _key,
                         std::int64_t _default = 0) const
    {
        return get(_key).as_int(_default);
    };

    std::uint64_t get_uint(option_key    _key,
                           std::uint64_t _default = 0) const
    {
        return get(_key).as_uint(_default);
    };

    double get_double(option_key _key,
                      double     _default = 0.0) const
    {
        return get(_key).as_double(_default);
    };

    const std::string& get_string(option_key _key) const
    {
        auto it = m_entries.find(_key);

        if (it == m_entries.end())
        {
            static const std::string empty;

            return empty;
        }

        return it->second.value.as_string();
    };

    void* get_ptr(option_key _key) const
    {
        return get(_key).as_ptr();
    };

    // ---- typed convenience setters ----
    //   each returns true if the key was registered.

    bool set_bool(option_key _key, bool _value)
    {
        return set(_key, option_value::from_bool(_value));
    };

    bool set_int(option_key _key, std::int64_t _value)
    {
        return set(_key, option_value::from_int(_value));
    };

    bool set_uint(option_key _key, std::uint64_t _value)
    {
        return set(_key, option_value::from_uint(_value));
    };

    bool set_double(option_key _key, double _value)
    {
        return set(_key, option_value::from_double(_value));
    };

    bool set_string(option_key _key, const char* _value)
    {
        return set(_key, option_value::from_string(_value));
    };

    bool set_string(option_key _key, const std::string& _value)
    {
        return set(_key, option_value::from_string(_value));
    };

    bool set_ptr(option_key _key, void* _value)
    {
        return set(_key, option_value::from_ptr(_value));
    };

    // ---- queries ----

    // contains
    //   returns true if the given key has been registered.
    bool contains(option_key _key) const
    {
        return (m_entries.find(_key) != m_entries.end());
    };

    // find
    //   returns a pointer to the entry for the given key, or
    // nullptr if not registered.
    const option_entry* find(option_key _key) const
    {
        auto it = m_entries.find(_key);

        if (it == m_entries.end())
        {
            return nullptr;
        }

        return &(it->second);
    };

    // find_by_name
    //   returns a pointer to the entry whose string name
    // matches _name, or nullptr if no match is found. Linear
    // scan; suitable for CLI parsing where option count is
    // small.
    const option_entry* find_by_name(const char* _name) const
    {
        if (!_name)
        {
            return nullptr;
        }

        for (const auto& kv : m_entries)
        {
            if (kv.second.name == _name)
            {
                return &(kv.second);
            }
        }

        return nullptr;
    };

    // key_from_name
    //   returns the option_key for the entry whose string name
    // matches _name, or D_TEST_OPT_NONE if not found.
    option_key key_from_name(const char* _name) const
    {
        const option_entry* entry = find_by_name(_name);

        if (!entry)
        {
            return D_TEST_OPT_NONE;
        }

        return entry->key;
    };

    // count
    //   returns the number of registered options.
    std::size_t count() const
    {
        return m_entries.size();
    };

    // ---- defaults ----

    // reset
    //   resets the value for the given key to its registered
    // default.
    // returns: true if the key was registered.
    bool reset(option_key _key)
    {
        auto it = m_entries.find(_key);

        if (it == m_entries.end())
        {
            return false;
        }

        it->second.value = it->second.default_value;

        return true;
    };

    // reset_all
    //   resets all registered options to their defaults.
    void reset_all()
    {
        for (auto& kv : m_entries)
        {
            kv.second.value = kv.second.default_value;
        }
    };

    // get_default
    //   returns the default value for the given key, or a
    // none-typed value if the key is not registered.
    option_value get_default(option_key _key) const
    {
        auto it = m_entries.find(_key);

        if (it == m_entries.end())
        {
            return option_value();
        }

        return it->second.default_value;
    };

    // ---- iteration ----

    // for_each
    //   invokes _fn for every registered entry. The callable
    // receives (const option_entry&).
    template<typename _Callable>
    void for_each(_Callable&& _fn) const
    {
        for (const auto& kv : m_entries)
        {
            _fn(kv.second);
        }
    };

    // ---- clear ----

    // clear
    //   removes all registered options.
    void clear()
    {
        m_entries.clear();
    };

private:
    std::unordered_map<option_key, option_entry> m_entries;
};


// =========================================================================
// VI.  STANDARD OPTION KEYS
// =========================================================================
//
// Framework-provided option keys organized by category. Each
// constant documents its expected value type in the comment.
// These keys are not auto-registered; the handler or session
// registers the subset it needs via register_option().
//

// ---- core execution (0x001 - 0x0FF) ----

// D_TEST_OPT_NAME
//   string: display name of the test node or session.
static constexpr option_key D_TEST_OPT_NAME              = 0x001;

// D_TEST_OPT_ENABLED
//   bool: whether this node is enabled for execution.
static constexpr option_key D_TEST_OPT_ENABLED           = 0x002;

// D_TEST_OPT_SKIP
//   bool: whether to skip this node entirely.
static constexpr option_key D_TEST_OPT_SKIP              = 0x003;

// D_TEST_OPT_TIMEOUT_MS
//   uint64: maximum execution time in milliseconds; 0 = no
// timeout.
static constexpr option_key D_TEST_OPT_TIMEOUT_MS        = 0x004;

// D_TEST_OPT_MAX_FAILURES
//   uint64: stop execution after this many failures; 0 =
// unlimited.
static constexpr option_key D_TEST_OPT_MAX_FAILURES      = 0x005;

// D_TEST_OPT_ABORT_ON_FAILURE
//   bool: stop on the first failure.
static constexpr option_key D_TEST_OPT_ABORT_ON_FAILURE  = 0x006;

// D_TEST_OPT_PRIORITY
//   int64: execution priority (higher = earlier).
static constexpr option_key D_TEST_OPT_PRIORITY          = 0x007;

// D_TEST_OPT_REPEAT_COUNT
//   uint64: number of times to repeat execution; 1 = run once.
static constexpr option_key D_TEST_OPT_REPEAT_COUNT      = 0x008;

// D_TEST_OPT_SHUFFLE
//   bool: randomize child execution order.
static constexpr option_key D_TEST_OPT_SHUFFLE           = 0x009;

// D_TEST_OPT_SHUFFLE_SEED
//   uint64: seed for shuffle randomization.
static constexpr option_key D_TEST_OPT_SHUFFLE_SEED      = 0x00A;

// ---- output and formatting (0x100 - 0x1FF) ----

// D_TEST_OPT_VERBOSITY
//   int64: verbosity level (0 = silent, 1 = minimal,
// 2 = normal, 3 = verbose, 4 = debug).
static constexpr option_key D_TEST_OPT_VERBOSITY         = 0x100;

// D_TEST_OPT_COLOR
//   bool: enable ANSI color codes in output.
static constexpr option_key D_TEST_OPT_COLOR             = 0x101;

// D_TEST_OPT_SHOW_TIMESTAMPS
//   bool: include timestamps in output.
static constexpr option_key D_TEST_OPT_SHOW_TIMESTAMPS   = 0x102;

// D_TEST_OPT_SHOW_DURATION
//   bool: show elapsed time for each node.
static constexpr option_key D_TEST_OPT_SHOW_DURATION     = 0x103;

// D_TEST_OPT_INDENT_STR
//   string: indentation string per depth level.
static constexpr option_key D_TEST_OPT_INDENT_STR        = 0x104;

// D_TEST_OPT_INDENT_MAX_LEVEL
//   uint64: maximum indentation depth.
static constexpr option_key D_TEST_OPT_INDENT_MAX_LEVEL  = 0x105;

// D_TEST_OPT_OUTPUT_FILE
//   string: output filename; empty = stdout.
static constexpr option_key D_TEST_OPT_OUTPUT_FILE       = 0x106;

// ---- filtering (0x200 - 0x2FF) ----

// D_TEST_OPT_FILTER_INCLUDE
//   string: include pattern (glob or substring).
static constexpr option_key D_TEST_OPT_FILTER_INCLUDE    = 0x200;

// D_TEST_OPT_FILTER_EXCLUDE
//   string: exclude pattern (glob or substring).
static constexpr option_key D_TEST_OPT_FILTER_EXCLUDE    = 0x201;

// D_TEST_OPT_FILTER_TAGS
//   string: comma-separated tag filter.
static constexpr option_key D_TEST_OPT_FILTER_TAGS       = 0x202;

// ---- reporting and metadata (0x300 - 0x3FF) ----

// D_TEST_OPT_REPORT_PASSED
//   bool: include passing nodes in output.
static constexpr option_key D_TEST_OPT_REPORT_PASSED     = 0x300;

// D_TEST_OPT_REPORT_SKIPPED
//   bool: include skipped nodes in output.
static constexpr option_key D_TEST_OPT_REPORT_SKIPPED    = 0x301;

// D_TEST_OPT_REPORT_SUMMARY
//   bool: print summary after execution.
static constexpr option_key D_TEST_OPT_REPORT_SUMMARY    = 0x302;

// D_TEST_OPT_DESCRIPTION
//   string: human-readable description.
static constexpr option_key D_TEST_OPT_DESCRIPTION       = 0x310;

// D_TEST_OPT_AUTHORS
//   string: author name(s).
static constexpr option_key D_TEST_OPT_AUTHORS           = 0x311;

// D_TEST_OPT_VERSION
//   string: version string.
static constexpr option_key D_TEST_OPT_VERSION           = 0x312;

// D_TEST_OPT_TAGS
//   string: comma-separated tags.
static constexpr option_key D_TEST_OPT_TAGS              = 0x313;


// =========================================================================
// VII. STANDARD OPTION DEFAULTS
// =========================================================================

NS_INTERNAL

    // register_standard_options
    //   registers the framework-provided standard options with
    // their default values onto the given test_options instance.
    inline void
    register_standard_options
    (
        test_options& _opts
    )
    {
        // ---- core execution ----
        _opts.register_option(
            D_TEST_OPT_NAME,
            "name",
            option_value::from_string(""),
            "display name of the test node or session");

        _opts.register_option(
            D_TEST_OPT_ENABLED,
            "enabled",
            option_value::from_bool(true),
            "whether this node is enabled for execution");

        _opts.register_option(
            D_TEST_OPT_SKIP,
            "skip",
            option_value::from_bool(false),
            "whether to skip this node entirely");

        _opts.register_option(
            D_TEST_OPT_TIMEOUT_MS,
            "timeout",
            option_value::from_uint(0),
            "maximum execution time in milliseconds (0 = none)");

        _opts.register_option(
            D_TEST_OPT_MAX_FAILURES,
            "max-failures",
            option_value::from_uint(0),
            "stop after N failures (0 = unlimited)");

        _opts.register_option(
            D_TEST_OPT_ABORT_ON_FAILURE,
            "abort-on-failure",
            option_value::from_bool(false),
            "stop on the first failure");

        _opts.register_option(
            D_TEST_OPT_PRIORITY,
            "priority",
            option_value::from_int(0),
            "execution priority (higher = earlier)");

        _opts.register_option(
            D_TEST_OPT_REPEAT_COUNT,
            "repeat",
            option_value::from_uint(1),
            "number of times to repeat execution");

        _opts.register_option(
            D_TEST_OPT_SHUFFLE,
            "shuffle",
            option_value::from_bool(false),
            "randomize child execution order");

        _opts.register_option(
            D_TEST_OPT_SHUFFLE_SEED,
            "shuffle-seed",
            option_value::from_uint(0),
            "seed for shuffle randomization");

        // ---- output and formatting ----
        _opts.register_option(
            D_TEST_OPT_VERBOSITY,
            "verbosity",
            option_value::from_int(2),
            "verbosity level (0=silent 1=minimal 2=normal "
            "3=verbose 4=debug)");

        _opts.register_option(
            D_TEST_OPT_COLOR,
            "color",
            option_value::from_bool(true),
            "enable ANSI color codes in output");

        _opts.register_option(
            D_TEST_OPT_SHOW_TIMESTAMPS,
            "timestamps",
            option_value::from_bool(false),
            "include timestamps in output");

        _opts.register_option(
            D_TEST_OPT_SHOW_DURATION,
            "duration",
            option_value::from_bool(true),
            "show elapsed time for each node");

        _opts.register_option(
            D_TEST_OPT_INDENT_STR,
            "indent",
            option_value::from_string("  "),
            "indentation string per depth level");

        _opts.register_option(
            D_TEST_OPT_INDENT_MAX_LEVEL,
            "indent-max",
            option_value::from_uint(10),
            "maximum indentation depth");

        _opts.register_option(
            D_TEST_OPT_OUTPUT_FILE,
            "output-file",
            option_value::from_string(""),
            "output filename (empty = stdout)");

        // ---- filtering ----
        _opts.register_option(
            D_TEST_OPT_FILTER_INCLUDE,
            "include",
            option_value::from_string(""),
            "include pattern (glob or substring)");

        _opts.register_option(
            D_TEST_OPT_FILTER_EXCLUDE,
            "exclude",
            option_value::from_string(""),
            "exclude pattern (glob or substring)");

        _opts.register_option(
            D_TEST_OPT_FILTER_TAGS,
            "filter-tags",
            option_value::from_string(""),
            "comma-separated tag filter");

        // ---- reporting ----
        _opts.register_option(
            D_TEST_OPT_REPORT_PASSED,
            "report-passed",
            option_value::from_bool(true),
            "include passing nodes in output");

        _opts.register_option(
            D_TEST_OPT_REPORT_SKIPPED,
            "report-skipped",
            option_value::from_bool(true),
            "include skipped nodes in output");

        _opts.register_option(
            D_TEST_OPT_REPORT_SUMMARY,
            "report-summary",
            option_value::from_bool(true),
            "print summary after execution");

        // ---- metadata ----
        _opts.register_option(
            D_TEST_OPT_DESCRIPTION,
            "description",
            option_value::from_string(""),
            "human-readable description");

        _opts.register_option(
            D_TEST_OPT_AUTHORS,
            "authors",
            option_value::from_string(""),
            "author name(s)");

        _opts.register_option(
            D_TEST_OPT_VERSION,
            "version",
            option_value::from_string(""),
            "version string");

        _opts.register_option(
            D_TEST_OPT_TAGS,
            "tags",
            option_value::from_string(""),
            "comma-separated tags");

        return;
    };

NS_END  // internal


// =========================================================================
// VIII. PRESET FACTORY FUNCTIONS
// =========================================================================

// test_options_default
//   creates a test_options instance with all standard options
// registered at their default values.
inline test_options
test_options_default()
{
    test_options opts;
    internal::register_standard_options(opts);

    return opts;
};

// test_options_silent
//   creates a test_options instance configured for silent
// execution (no output, stats only).
inline test_options
test_options_silent()
{
    test_options opts;
    internal::register_standard_options(opts);
    opts.set_int(D_TEST_OPT_VERBOSITY, 0);
    opts.set_bool(D_TEST_OPT_REPORT_PASSED, false);
    opts.set_bool(D_TEST_OPT_REPORT_SKIPPED, false);
    opts.set_bool(D_TEST_OPT_REPORT_SUMMARY, false);

    return opts;
};

// test_options_minimal
//   creates a test_options instance configured for minimal
// output (failures only).
inline test_options
test_options_minimal()
{
    test_options opts;
    internal::register_standard_options(opts);
    opts.set_int(D_TEST_OPT_VERBOSITY, 1);
    opts.set_bool(D_TEST_OPT_REPORT_PASSED, false);
    opts.set_bool(D_TEST_OPT_REPORT_SKIPPED, false);

    return opts;
};

// test_options_verbose
//   creates a test_options instance configured for verbose
// output (everything, with timestamps).
inline test_options
test_options_verbose()
{
    test_options opts;
    internal::register_standard_options(opts);
    opts.set_int(D_TEST_OPT_VERBOSITY, 3);
    opts.set_bool(D_TEST_OPT_SHOW_TIMESTAMPS, true);

    return opts;
};

NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_OPTIONS_