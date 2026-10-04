/***********************************************************************
* re_std                                             source_location.hpp
*
* class source_location:
*   A value type capturing a point in the source — file, function, line,
* column — mirroring std::source_location (C++20). re_std back-ports it to
* C++11 using the compiler builtins __builtin_FILE / __builtin_FUNCTION /
* __builtin_LINE / __builtin_COLUMN, which long predate C++20, so the
* RE_STD-AHEAD window is wide (re_std surfaces source_location ~9 years early).
*
*   current() is the factory: it relies on the builtins being supplied as
* default arguments, which the compiler evaluates at the *call site* — so
* `source_location::current()` reports the caller's location. std makes
* current() consteval in C++20; re_std makes it constexpr instead (a single
* return, valid C++11 constexpr), which is strictly more permissive
* (usable at compile time AND runtime) — a deliberate, documented
* divergence in service of the back-port.
*
*   __builtin_COLUMN is newer (GCC 9+, Clang 9+); where it is unavailable
* re_std reports column 0 (detected via D_RE_STD_HAS_BUILTIN_COLUMN). All
* other fields are exact on every supported compiler. re_std::source_location
* is re_std's OWN type (NOT a std re-export): no language construct emits a
* source_location, so identity with std::source_location buys nothing, and
* owning the type keeps behaviour identical across every tier.
*
*
* path:      /inc/djinterp/re_std/source_location/source_location.hpp
* link(s):   TBA
* author(s): re_std contributors                       date: 2026.06.05
***********************************************************************/

#ifndef DJINTERP_RE_STD_SOURCE_LOCATION_SOURCE_LOCATION_
#define DJINTERP_RE_STD_SOURCE_LOCATION_SOURCE_LOCATION_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstdint>  // uint_least32_t

// __builtin_COLUMN detection. The real core exposes this via its intrinsic
// table; this mirrors the same decision.
#if defined(__has_builtin)
#  if __has_builtin(__builtin_COLUMN)
#    define D_RE_STD_HAS_BUILTIN_COLUMN 1
#  else
#    define D_RE_STD_HAS_BUILTIN_COLUMN 0
#  endif
#elif defined(__GNUC__) && (__GNUC__ >= 9)
#  define D_RE_STD_HAS_BUILTIN_COLUMN 1
#else
#  define D_RE_STD_HAS_BUILTIN_COLUMN 0
#endif

#if D_RE_STD_HAS_BUILTIN_COLUMN
#  define D_RE_STD_SL_COLUMN_DEFAULT __builtin_COLUMN()
#else
#  define D_RE_STD_SL_COLUMN_DEFAULT 0u
#endif

namespace re_std
{

    // source_location
    //   class: an aggregate-ish value type describing a source position.
    class source_location
    {
    public:

        // default ctor: an empty location (line 0, column 0, "" strings),
        // matching std's default-constructed source_location.
        constexpr source_location() noexcept
            : m_file(""), m_function(""), m_line(0), m_column(0)
        {}

        // current()
        //   function: returns a source_location for the call site. The
        //   builtins are default arguments, so they bind to the *caller's*
        //   position (the canonical std technique). constexpr (not consteval)
        //   for portability to C++11.
        static constexpr source_location current(
            const char* _file            = __builtin_FILE(),
            const char* _function        = __builtin_FUNCTION(),
            std::uint_least32_t _line     = __builtin_LINE(),
            std::uint_least32_t _column   = D_RE_STD_SL_COLUMN_DEFAULT) noexcept
        {
            return source_location(_file, _function, _line, _column);
        }

        constexpr std::uint_least32_t line() const noexcept
        { return m_line; }

        constexpr std::uint_least32_t column() const noexcept
        { return m_column; }

        constexpr const char* file_name() const noexcept
        { return m_file; }

        constexpr const char* function_name() const noexcept
        { return m_function; }

    private:

        // private field ctor used by current(); single-expression bodies keep
        // everything valid as C++11 constexpr.
        constexpr source_location(const char* _file, const char* _function,
                                  std::uint_least32_t _line,
                                  std::uint_least32_t _column) noexcept
            : m_file(_file), m_function(_function),
              m_line(_line), m_column(_column)
        {}

        const char*         m_file;
        const char*         m_function;
        std::uint_least32_t m_line;
        std::uint_least32_t m_column;
    };

}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_SOURCE_LOCATION_SOURCE_LOCATION_
