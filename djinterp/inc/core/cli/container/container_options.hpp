/******************************************************************************
* djinterp [container]                                   container_options.hpp
*
* Compile-time container configuration via bitmask flags.
*   Provides a single `DContainerOption` flags enumeration covering the
* mutability, ordering, and storage axes common to all containers.  All
* extraction, validation, and default-resolution logic is constexpr; after
* template instantiation every flag test compiles down to nothing.
*
* STRING MAPPING:
*   A constexpr `string_kv` table and lookup functions allow CLI tools to
* resolve human-readable option names (e.g. "immutable", "ordered") into
* flag bits at compile time or at negligible runtime cost.
*
* DESIGN INVARIANTS:
*   - Within each axis at most one bit may be set; setting two bits in the
*     same axis is a static_assert failure in any container that consumes
*     the flags.
*   - When no bit is set for an axis the container applies its own default
*     (typically writable + unordered; storage inferred from capacity).
*   - All functions in this header are constexpr and free of side effects.
*
*
* path:      /inc/container/container_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.15
******************************************************************************/

#ifndef DJINTERP_CONTAINER_OPTIONS_
#define DJINTERP_CONTAINER_OPTIONS_ 1

#include <cstddef>
#include <type_traits>


///////////////////////////////////////////////////////////////////////////////
///        I.    FLAGS ENUMERATION                                          ///
///////////////////////////////////////////////////////////////////////////////

// DContainerOption
//   enum: bitmask flags selecting container behavior along three
// orthogonal axes.  Flags are combined with bitwise OR.
//
// mutability axis  (bits 0 - 2)
//   writable     — full read/write access after construction.
//   immutable    — read-only after construction.
//   compile_time — constexpr-only; requires fixed capacity.
//
// ordering axis   (bits 3 - 4)
//   ordered      — elements maintained in sorted order.
//   unordered    — insertion-order; no ordering invariant.
//
// storage axis    (bits 5 - 6)
//   fixed_size   — stack-allocated; capacity is a template param.
//   dynamic_size — heap-allocated; grows as needed.
enum class DContainerOption : unsigned
{
    none         = 0x00,

    writable     = 0x01,
    immutable    = 0x02,
    compile_time = 0x04,

    ordered      = 0x08,
    unordered    = 0x10,

    fixed_size   = 0x20,
    dynamic_size = 0x40
};


///////////////////////////////////////////////////////////////////////////////
///        II.   CONSTEXPR BITWISE OPERATORS                               ///
///////////////////////////////////////////////////////////////////////////////

// operator|
//   function: combines two option flags.
inline constexpr DContainerOption
operator|(DContainerOption _lhs, DContainerOption _rhs) noexcept
{
    return static_cast<DContainerOption>(
        static_cast<unsigned>(_lhs) |
        static_cast<unsigned>(_rhs));
}

// operator&
//   function: intersects two option flags.
inline constexpr DContainerOption
operator&(DContainerOption _lhs, DContainerOption _rhs) noexcept
{
    return static_cast<DContainerOption>(
        static_cast<unsigned>(_lhs) &
        static_cast<unsigned>(_rhs));
}

// operator^
//   function: XORs two option flags.
inline constexpr DContainerOption
operator^(DContainerOption _lhs, DContainerOption _rhs) noexcept
{
    return static_cast<DContainerOption>(
        static_cast<unsigned>(_lhs) ^
        static_cast<unsigned>(_rhs));
}

// operator~
//   function: inverts option flags.
inline constexpr DContainerOption
operator~(DContainerOption _flags) noexcept
{
    return static_cast<DContainerOption>(
        ~static_cast<unsigned>(_flags));
}

// operator|=
//   function: compound OR assignment.
inline constexpr DContainerOption&
operator|=(DContainerOption& _lhs, DContainerOption _rhs) noexcept
{
    _lhs = _lhs | _rhs;

    return _lhs;
}

// operator&=
//   function: compound AND assignment.
inline constexpr DContainerOption&
operator&=(DContainerOption& _lhs, DContainerOption _rhs) noexcept
{
    _lhs = _lhs & _rhs;

    return _lhs;
}


///////////////////////////////////////////////////////////////////////////////
///        III.  AXIS MASKS AND EXTRACTION                                  ///
///////////////////////////////////////////////////////////////////////////////

// D_CONTAINER_OPT_MUTABILITY_MASK
//   constant: bitmask covering the mutability axis (bits 0-2).
#define D_CONTAINER_OPT_MUTABILITY_MASK  0x07u

// D_CONTAINER_OPT_ORDERING_MASK
//   constant: bitmask covering the ordering axis (bits 3-4).
#define D_CONTAINER_OPT_ORDERING_MASK    0x18u

// D_CONTAINER_OPT_STORAGE_MASK
//   constant: bitmask covering the storage axis (bits 5-6).
#define D_CONTAINER_OPT_STORAGE_MASK     0x60u

// container_option_has
//   function: returns true if _flags contains every bit in _test.
inline constexpr bool
container_option_has
(
    DContainerOption _flags,
    DContainerOption _test
) noexcept
{
    return ((_flags & _test) == _test);
}

// container_option_mutability
//   function: extracts the mutability bits from a flag set.
inline constexpr DContainerOption
container_option_mutability(DContainerOption _flags) noexcept
{
    return static_cast<DContainerOption>(
        static_cast<unsigned>(_flags) &
        D_CONTAINER_OPT_MUTABILITY_MASK);
}

// container_option_ordering
//   function: extracts the ordering bits from a flag set.
inline constexpr DContainerOption
container_option_ordering(DContainerOption _flags) noexcept
{
    return static_cast<DContainerOption>(
        static_cast<unsigned>(_flags) &
        D_CONTAINER_OPT_ORDERING_MASK);
}

// container_option_storage
//   function: extracts the storage bits from a flag set.
inline constexpr DContainerOption
container_option_storage(DContainerOption _flags) noexcept
{
    return static_cast<DContainerOption>(
        static_cast<unsigned>(_flags) &
        D_CONTAINER_OPT_STORAGE_MASK);
}


///////////////////////////////////////////////////////////////////////////////
///        IV.   AXIS VALIDATION                                           ///
///////////////////////////////////////////////////////////////////////////////

namespace internal {

    // popcount_constexpr
    //   function: compile-time population count.
    inline constexpr unsigned
    popcount_constexpr(unsigned _v) noexcept
    {
        unsigned count = 0;

        while (_v)
        {
            count += (_v & 1u);
            _v >>= 1u;
        }

        return count;
    }

} // namespace internal

// container_option_axis_valid
//   function: returns true if at most one bit is set within
// each axis.  Containers should static_assert on this.
inline constexpr bool
container_option_axis_valid(DContainerOption _flags) noexcept
{
    unsigned m = static_cast<unsigned>(
                     container_option_mutability(_flags));
    unsigned o = static_cast<unsigned>(
                     container_option_ordering(_flags));
    unsigned s = static_cast<unsigned>(
                     container_option_storage(_flags));

    return ( (internal::popcount_constexpr(m) <= 1) &&
             (internal::popcount_constexpr(o) <= 1) &&
             (internal::popcount_constexpr(s) <= 1) );
}


///////////////////////////////////////////////////////////////////////////////
///        V.    DEFAULT RESOLUTION                                         ///
///////////////////////////////////////////////////////////////////////////////

// container_option_resolve
//   function: fills unset axes with sensible defaults.
//     mutability  →  writable
//     ordering    →  unordered
//     storage     →  fixed_size if _capacity > 0, else dynamic_size
// The result always has exactly one bit per axis.
inline constexpr DContainerOption
container_option_resolve
(
    DContainerOption _flags,
    std::size_t      _capacity
) noexcept
{
    DContainerOption m = container_option_mutability(_flags);
    DContainerOption o = container_option_ordering(_flags);
    DContainerOption s = container_option_storage(_flags);

    if (m == DContainerOption::none)
    {
        m = DContainerOption::writable;
    }

    if (o == DContainerOption::none)
    {
        o = DContainerOption::unordered;
    }

    if (s == DContainerOption::none)
    {
        s = (_capacity > 0)
            ? DContainerOption::fixed_size
            : DContainerOption::dynamic_size;
    }

    return (m | o | s);
}


///////////////////////////////////////////////////////////////////////////////
///        VI.   STRING-TO-VALUE MAPPING (string_kv)                        ///
///////////////////////////////////////////////////////////////////////////////

// string_kv
//   struct: compile-time association of a name string with an
// enum value.  Used for CLI resolution.
template<typename _Enum>
struct string_kv
{
    const char* name;
    _Enum       value;
};

namespace internal {

    // constexpr_str_eq
    //   function: constexpr character-by-character string
    // comparison.
    inline constexpr bool
    constexpr_str_eq
    (
        const char* _a,
        const char* _b
    ) noexcept
    {
        while ( (*_a) &&
                (*_b) )
        {
            if (*_a != *_b)
            {
                return false;
            }

            ++_a;
            ++_b;
        }

        return (*_a == *_b);
    }

} // namespace internal

// string_kv_lookup
//   function: searches a constexpr string_kv table for _name.
// Writes the corresponding value to _out and returns true on
// match; returns false if _name is not found.  Fully constexpr.
template<typename    _Enum,
         std::size_t _N>
inline constexpr bool
string_kv_lookup
(
    const string_kv<_Enum> (&_table)[_N],
    const char*               _name,
    _Enum&                    _out
) noexcept
{
    for (std::size_t i = 0; i < _N; ++i)
    {
        if (internal::constexpr_str_eq(_table[i].name, _name))
        {
            _out = _table[i].value;

            return true;
        }
    }

    return false;
}

// string_kv_reverse
//   function: searches a constexpr string_kv table for _value.
// Returns the corresponding name, or nullptr if not found.
template<typename    _Enum,
         std::size_t _N>
inline constexpr const char*
string_kv_reverse
(
    const string_kv<_Enum> (&_table)[_N],
    _Enum                    _value
) noexcept
{
    for (std::size_t i = 0; i < _N; ++i)
    {
        if (_table[i].value == _value)
        {
            return _table[i].name;
        }
    }

    return nullptr;
}

// string_kv_combine
//   function: resolves a null-terminated array of option name
// strings into a combined flag set by OR-ing each match.
// Returns false and writes DContainerOption::none if any name
// is unrecognized.
template<typename    _Enum,
         std::size_t _N>
inline constexpr bool
string_kv_combine
(
    const string_kv<_Enum> (&_table)[_N],
    const char* const*       _names,
    std::size_t              _name_count,
    _Enum&                   _out
) noexcept
{
    _out = static_cast<_Enum>(0);

    for (std::size_t i = 0; i < _name_count; ++i)
    {
        _Enum tmp{};

        if (!string_kv_lookup(_table, _names[i], tmp))
        {
            _out = static_cast<_Enum>(0);

            return false;
        }

        _out = static_cast<_Enum>(
            static_cast<unsigned>(_out) |
            static_cast<unsigned>(tmp));
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///        VII.  CONTAINER OPTION STRING TABLE                              ///
///////////////////////////////////////////////////////////////////////////////

// D_CONTAINER_OPTION_TABLE
//   constant: constexpr string_kv table mapping CLI names to
// DContainerOption flag bits.
static constexpr string_kv<DContainerOption>
D_CONTAINER_OPTION_TABLE[] =
{
    { "writable",     DContainerOption::writable     },
    { "immutable",    DContainerOption::immutable     },
    { "compile_time", DContainerOption::compile_time  },
    { "ordered",      DContainerOption::ordered       },
    { "unordered",    DContainerOption::unordered     },
    { "fixed",        DContainerOption::fixed_size    },
    { "dynamic",      DContainerOption::dynamic_size  }
};

// D_CONTAINER_OPTION_TABLE_SIZE
//   constant: number of entries in the option table.
static constexpr std::size_t D_CONTAINER_OPTION_TABLE_SIZE =
    sizeof(D_CONTAINER_OPTION_TABLE) /
    sizeof(D_CONTAINER_OPTION_TABLE[0]);

// container_option_from_string
//   function: resolves a single CLI option name to a flag bit.
inline constexpr bool
container_option_from_string
(
    const char*       _name,
    DContainerOption& _out
) noexcept
{
    return string_kv_lookup(D_CONTAINER_OPTION_TABLE,
                            _name,
                            _out);
}

// container_option_to_string
//   function: returns the CLI name for a single flag bit,
// or nullptr if the value is not a single recognized flag.
inline constexpr const char*
container_option_to_string(DContainerOption _flag) noexcept
{
    return string_kv_reverse(D_CONTAINER_OPTION_TABLE, _flag);
}

// container_options_from_strings
//   function: resolves an array of CLI option names into a
// combined flag set.
inline constexpr bool
container_options_from_strings
(
    const char* const* _names,
    std::size_t        _count,
    DContainerOption&  _out
) noexcept
{
    return string_kv_combine(D_CONTAINER_OPTION_TABLE,
                             _names,
                             _count,
                             _out);
}


#endif  // DJINTERP_CONTAINER_OPTIONS_
