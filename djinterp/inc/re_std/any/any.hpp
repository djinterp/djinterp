/*******************************************************************************
* djinterp [re_std]                                                      any.hpp
*
* any class header:
*   Constexpr-friendly type-erased value container. A portable alternative
* to std::any with compile-time evaluation support for small trivial types.
*
*   TWO STORAGE PATHS:
*
*   1. SBO (small buffer optimization) - constexpr-capable (C++14+).
*      A union of fundamental categories stores any type whose value can
*      be losslessly represented as one of:
*        - bool
*        - signed integral     (char through long long)
*        - unsigned integral   (unsigned char through unsigned long long)
*        - floating point      (float, double)
*        - enum                (stored as underlying integral type)
*        - pointer             (T* stored as void*)
*        - const pointer       (const T* stored as const void*)
*      Construction, copy, and typed retrieval via get<T>() are constexpr
*      for these types on C++14 and later.
*   2. Heap - runtime only.
*      Types that do not fit the SBO (class types, containers, large
*      aggregates) are stored in a heap-allocated control block with
*      type-erased copy/move/destroy via function pointer ops table.
*      Requires RE_STD_HAS_HEADER_NEW. NOT constexpr.
*
*   TYPE IDENTITY:
*   Each stored type has a unique identity derived from the address of
* a static function template instantiation. RTTI-free type checking
* via holds<T>() with zero virtual dispatch overhead.
*
*   PORTABILITY:
*   - C++98/03: SBO with explicit per-type constructors, tag-dispatched
*     get<T>(), safe-bool idiom. No constexpr.
*   - C++11:    SBO via SFINAE-dispatched template constructors,
*     explicit operator bool, noexcept. Not constexpr.
*   - C++14+:   constexpr SBO construction and retrieval.
*   - Enum SBO gated on RE_STD_HAS_IS_ENUM / RE_STD_HAS_UNDERLYING_TYPE.
*   - Move semantics gated on RE_STD_LANG_HAS_RVALUE_REFERENCES.
*   - Heap path gated on RE_STD_HAS_HEADER_NEW.
*   - Emplace gated on RE_STD_LANG_HAS_VARIADIC_TEMPLATES.
*
*   Uses:
*     env.h              - language version detection
*     env_cpp98.h        - header availability (new, utility)
*     env_cpp_features.h - fine-grained feature detection
*     config.hpp         - RE_STD_CONSTEXPR, RE_STD_INLINE
*     type_traits.hpp    - re_std type traits (no <type_traits> dependency)
*
*
* path:      /inc/re_std/any/any.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.06
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    COMPATIBILITY MACROS
      --------------------

I.    TYPE IDENTITY
      -------------

II.   STORAGE CATEGORY
      ----------------

III.  SBO STORAGE UNION
      -----------------

IV.   HEAP CONTROL BLOCK
      ------------------

V.    ANY CLASS
      ---------
*/

#ifndef RE_STD_ANY_ANY_HPP
#define RE_STD_ANY_ANY_HPP 1

// `any` keeps a scalar in a `long long` or `unsigned long long` member, so it
// exists wherever `long long` does (decision 4.6): every C++11 build, and
// C++98 as the extension, where the diagnostic pair keeps -pedantic quiet.
// Under ISO strict C++98 there is no `long long`, and `any` is absent --
// re_std's rule is omit, don't degrade.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_HAS_LONG_LONG
RE_STD_LONG_LONG_DIAG_PUSH

// std
#include <cstddef>                       // size_t
// re_std

#if RE_STD_HAS_HEADER_NEW
    // std
    #include <new>
#endif

#if RE_STD_HAS_HEADER_UTILITY
    // std
    #include <utility>
#endif

// re_std -- re_std's own traits, never std's: the names below are declared
// in namespace re_std, so importing std's would collide with them the moment
// a translation unit also includes re_std's type_traits.
#include "../type_traits/enable_if.hpp"          // enable_if
#include "../type_traits/is_const.hpp"           // is_const
#include "../type_traits/is_enum.hpp"            // is_enum
#include "../type_traits/is_floating_point.hpp"  // is_floating_point
#include "../type_traits/is_function.hpp"        // is_function
#include "../type_traits/is_integral.hpp"        // is_integral
#include "../type_traits/is_pointer.hpp"         // is_pointer
#include "../type_traits/is_same.hpp"            // is_same
#include "../type_traits/is_signed.hpp"          // is_signed
#include "../type_traits/is_unsigned.hpp"        // is_unsigned
#include "../type_traits/remove_pointer.hpp"     // remove_pointer
#include "../type_traits/underlying_type.hpp"    // underlying_type


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================
// Null pointer constant for C++98/03 compatibility. On C++11+,
// resolves to nullptr. On C++98/03, resolves to 0.
// NOTE: should migrate to the core header in future.


namespace re_std
{


///////////////////////////////////////////////////////////////////////////////
///                I.   TYPE IDENTITY                                       ///
///////////////////////////////////////////////////////////////////////////////
// A unique identity per type, derived from the address of a
// function template instantiation. No RTTI required. The address
// of each instantiation is a constant expression (C++11+),
// enabling constexpr type checking on those compilers.

// any_type_id
//   type: opaque identifier for a stored type.
typedef void(*any_type_id)();

namespace internal
{

    // any_type_tag_fn
    //   function: empty function template whose address is
    // unique per Type instantiation. Never called.
    template<typename Type>
    void
    any_type_tag_fn()
    {
        return;
    }

}  // internal

// any_type_id_of
//   trait: yields the any_type_id for Type.
template<typename Type>
struct any_type_id_of
{
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    RE_STD_STATIC_CONSTEXPR any_type_id value = &internal::any_type_tag_fn<Type>;
#else
    static const any_type_id value;
#endif
};

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    // out-of-class definition (ODR safety)
    template<typename Type>
    RE_STD_CONSTEXPR any_type_id any_type_id_of<Type>::value;
#else
    // C++98/03: function pointer address requires out-of-class
    // definition; not a constant expression.
    template<typename Type>
    const any_type_id any_type_id_of<Type>::value =
        &internal::any_type_tag_fn<Type>;
#endif

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
    // any_type_id_of_v
    //   variable: variable template: value of any_type_id_of<Type>.
    template<typename Type>
    RE_STD_CONSTEXPR any_type_id any_type_id_of_v = any_type_id_of<Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///                II.  STORAGE CATEGORY                                    ///
///////////////////////////////////////////////////////////////////////////////

// DAnyCategory
//   enum: identifies which union member is active.
// note: struct-wrapped enum for C++98/03 compatibility. On
// C++11+, could be enum class; kept as struct for a single
// implementation path.
struct DAnyCategory
{
    enum Value
    {
        cat_empty    = 0,
        cat_bool     = 1,
        cat_signed   = 2,
        cat_unsigned = 3,
        cat_floating = 4,
        cat_pointer  = 5,
        cat_cpointer = 6,
        cat_heap     = 7
    };
};


namespace internal
{

    // any_category_of
    //   trait: maps a type to its SBO storage category.
    // Defaults to cat_heap for types not handled by the SBO.

    // -----------------------------------------------------------------
    // C++11+ path: SFINAE partial specializations
    // -----------------------------------------------------------------

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // primary template: heap fallback
    template<typename Type,
             typename = void>
    struct any_category_of
    {
        static const int value = DAnyCategory::cat_heap;
    };

    // bool
    template<>
    struct any_category_of<bool>
    {
        static const int value = DAnyCategory::cat_bool;
    };

    // signed integrals (not bool)
    template<typename Type>
    struct any_category_of<Type,
        typename enable_if<
            ( is_integral<Type>::value  &&
              is_signed<Type>::value    &&
              !is_same<Type, bool>::value )
        >::type>
    {
        static const int value = DAnyCategory::cat_signed;
    };

    // unsigned integrals (not bool)
    template<typename Type>
    struct any_category_of<Type,
        typename enable_if<
            ( is_integral<Type>::value  &&
              is_unsigned<Type>::value  &&
              !is_same<Type, bool>::value )
        >::type>
    {
        static const int value = DAnyCategory::cat_unsigned;
    };

    // floating point
    template<typename Type>
    struct any_category_of<Type,
        typename enable_if<
            is_floating_point<Type>::value
        >::type>
    {
        static const int value = DAnyCategory::cat_floating;
    };

    // enum types (stored via underlying integral)
#if RE_STD_HAS_IS_ENUM && RE_STD_HAS_UNDERLYING_TYPE
    template<typename Type>
    struct any_category_of<Type,
        typename enable_if<
            is_enum<Type>::value
        >::type>
    {
        static const int value =
            ( is_signed<
                  typename underlying_type<Type>::type
              >::value
              ? DAnyCategory::cat_signed
              : DAnyCategory::cat_unsigned );
    };
#endif  // RE_STD_HAS_IS_ENUM && RE_STD_HAS_UNDERLYING_TYPE

    // non-const pointer (not function pointer)
    template<typename Type>
    struct any_category_of<Type*,
        typename enable_if<
            ( !is_function<Type>::value &&
              !is_const<Type>::value )
        >::type>
    {
        static const int value = DAnyCategory::cat_pointer;
    };

    // const pointer (not function pointer)
    template<typename Type>
    struct any_category_of<const Type*,
        typename enable_if<
            !is_function<Type>::value
        >::type>
    {
        static const int value = DAnyCategory::cat_cpointer;
    };

    // -----------------------------------------------------------------
    // C++98/03 path: explicit specializations
    // -----------------------------------------------------------------

#else  // C++98/03

    // primary template: heap fallback
    template<typename Type>
    struct any_category_of
    {
        static const int value = DAnyCategory::cat_heap;
    };

    // bool
    template<>
    struct any_category_of<bool>
    {
        static const int value = DAnyCategory::cat_bool;
    };

    // signed integrals
    template<> struct any_category_of<signed char>
    { static const int value = DAnyCategory::cat_signed; };

    template<> struct any_category_of<short>
    { static const int value = DAnyCategory::cat_signed; };

    template<> struct any_category_of<int>
    { static const int value = DAnyCategory::cat_signed; };

    template<> struct any_category_of<long>
    { static const int value = DAnyCategory::cat_signed; };

    template<> struct any_category_of<long long>
    { static const int value = DAnyCategory::cat_signed; };

    // unsigned integrals
    template<> struct any_category_of<unsigned char>
    { static const int value = DAnyCategory::cat_unsigned; };

    template<> struct any_category_of<unsigned short>
    { static const int value = DAnyCategory::cat_unsigned; };

    template<> struct any_category_of<unsigned int>
    { static const int value = DAnyCategory::cat_unsigned; };

    template<> struct any_category_of<unsigned long>
    { static const int value = DAnyCategory::cat_unsigned; };

    template<> struct any_category_of<unsigned long long>
    { static const int value = DAnyCategory::cat_unsigned; };

    // char: signedness is implementation-defined
    template<> struct any_category_of<char>
    {
        static const int value =
            ( is_signed<char>::value
              ? DAnyCategory::cat_signed
              : DAnyCategory::cat_unsigned );
    };

    // wchar_t
    template<> struct any_category_of<wchar_t>
    {
        static const int value =
            ( is_signed<wchar_t>::value
              ? DAnyCategory::cat_signed
              : DAnyCategory::cat_unsigned );
    };

    // floating point
    template<> struct any_category_of<float>
    { static const int value = DAnyCategory::cat_floating; };

    template<> struct any_category_of<double>
    { static const int value = DAnyCategory::cat_floating; };

    template<> struct any_category_of<long double>
    { static const int value = DAnyCategory::cat_floating; };

    // pointers (partial specialization - works in C++98)
    template<typename Type>
    struct any_category_of<Type*>
    {
        static const int value = DAnyCategory::cat_pointer;
    };

    template<typename Type>
    struct any_category_of<const Type*>
    {
        static const int value = DAnyCategory::cat_cpointer;
    };

    // note: function pointers will match Type* and attempt
    // static_cast<void*>, which is ill-formed. This produces
    // a compile error (not silent misbehavior).

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

    // is_sbo_type
    //   trait: true if Type uses the SBO path.
    template<typename Type>
    struct is_sbo_type
    {
        static const bool value =
            ( any_category_of<Type>::value != DAnyCategory::cat_heap );
    };

    // get_tag
    //   type: tag for category-based dispatch of get<T>().
    template<int Cat>
    struct get_tag
    {};

}  // internal


///////////////////////////////////////////////////////////////////////////////
///                III. SBO STORAGE UNION                                   ///
///////////////////////////////////////////////////////////////////////////////

namespace internal
{

    // any_sbo
    //   union: small buffer storage with per-member constexpr
    // constructors. Each constructor initializes exactly one
    // member, satisfying constexpr union rules (C++14+).
    union any_sbo
    {
        bool               v_bool;
        long long          v_signed;
        unsigned long long v_unsigned;
        double             v_floating;
        void*              v_pointer;
        const void*        v_cpointer;

        // default: zero-initialized unsigned
        RE_STD_CONSTEXPR any_sbo() RE_STD_NOEXCEPT
            : v_unsigned(0)
        {}

        // per-category constructors (the DAnyCategory::Value tag
        // parameter disambiguates overloads)
        RE_STD_CONSTEXPR explicit
        any_sbo(
            bool                _v,
            DAnyCategory::Value
        ) RE_STD_NOEXCEPT
            : v_bool(_v)
        {}

        RE_STD_CONSTEXPR explicit
        any_sbo(
            long long           _v,
            DAnyCategory::Value
        ) RE_STD_NOEXCEPT
            : v_signed(_v)
        {}

        RE_STD_CONSTEXPR explicit
        any_sbo(
            unsigned long long  _v,
            DAnyCategory::Value
        ) RE_STD_NOEXCEPT
            : v_unsigned(_v)
        {}

        RE_STD_CONSTEXPR explicit
        any_sbo(
            double              _v,
            DAnyCategory::Value
        ) RE_STD_NOEXCEPT
            : v_floating(_v)
        {}

        RE_STD_CONSTEXPR explicit
        any_sbo(
            void*               _v,
            DAnyCategory::Value
        ) RE_STD_NOEXCEPT
            : v_pointer(_v)
        {}

        RE_STD_CONSTEXPR explicit
        any_sbo(
            const void*         _v,
            DAnyCategory::Value
        ) RE_STD_NOEXCEPT
            : v_cpointer(_v)
        {}
    };

}  // internal


///////////////////////////////////////////////////////////////////////////////
///                IV.  HEAP CONTROL BLOCK                                  ///
///////////////////////////////////////////////////////////////////////////////

#if RE_STD_HAS_HEADER_NEW

namespace internal
{

    // any_heap_ops
    //   struct: type-erased operations table for heap storage.
    struct any_heap_ops
    {
        void  (*destroy)(void*);
        void* (*clone)(const void*);
    };

    // heap_destroy
    //   function: typed destroy operation for heap storage.
    template<typename Type>
    static void
    heap_destroy(
        void* _p
    )
    {
        delete static_cast<Type*>(_p);

        return;
    }

    // heap_clone
    //   function: typed clone operation for heap storage.
    template<typename Type>
    static void*
    heap_clone(
        const void* _p
    )
    {
        return new Type(*static_cast<const Type*>(_p));
    }

    // any_heap_ops_for
    //   function: returns the operations table for Type.
    // Uses a local static for safe lazy initialization
    // (thread-safe in C++11 per [stmt.dcl]/4).
    template<typename Type>
    const any_heap_ops*
    any_heap_ops_for()
    {
        static const any_heap_ops ops =
        {
            &heap_destroy<Type>,
            &heap_clone<Type>
        };

        return &ops;
    }

}  // internal

#endif  // RE_STD_HAS_HEADER_NEW


///////////////////////////////////////////////////////////////////////////////
///                V.   ANY CLASS                                           ///
///////////////////////////////////////////////////////////////////////////////

// any
//   class: type-erased value container. Constexpr for SBO
// types (trivial scalars, enums, pointers). Runtime-only for
// heap types (class types, containers, aggregates).
class any
{
    // -----------------------------------------------------------------
    //  safe-bool idiom (C++98/03)
    // -----------------------------------------------------------------

#if (!RE_STD_LANG_IS_CPP11_OR_HIGHER)
private:
    // safe_bool_member
    //   type: pointer-to-member used for the safe-bool idiom.
    // Prevents implicit conversion to int while allowing
    // boolean contexts.
    typedef void (any::*safe_bool_type)() const;

    // safe_bool_fn
    //   function: dummy member function whose address is the
    // "true" value for the safe-bool idiom.
    void safe_bool_fn() const
    {
        return;
    }
#endif

public:
    // -----------------------------------------------------------------
    //  function: empty
    // -----------------------------------------------------------------

    RE_STD_CONSTEXPR any() RE_STD_NOEXCEPT
        : m_category(DAnyCategory::cat_empty),
          m_type_id(RE_STD_NULLPTR),
          m_sbo()
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // =================================================================
    // CONSTRUCTORS: C++11+ (SFINAE-dispatched templates)
    // =================================================================

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // -----------------------------------------------------------------
    //  function: bool
    // -----------------------------------------------------------------

    RE_STD_CONSTEXPR any(
        bool _v
    ) RE_STD_NOEXCEPT
        : m_category(DAnyCategory::cat_bool),
          m_type_id(any_type_id_of<bool>::value),
          m_sbo(_v, DAnyCategory::cat_bool)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: signed integrals (not bool)
    // -----------------------------------------------------------------

    template<typename Type,
             typename enable_if<
                 ( is_integral<Type>::value &&
                   is_signed<Type>::value   &&
                   !is_same<Type, bool>::value ),
                 int>::type = 0>
    RE_STD_CONSTEXPR any(
        Type _v
    ) RE_STD_NOEXCEPT
        : m_category(DAnyCategory::cat_signed),
          m_type_id(any_type_id_of<Type>::value),
          m_sbo(static_cast<long long>(_v), DAnyCategory::cat_signed)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: unsigned integrals (not bool)
    // -----------------------------------------------------------------

    template<typename Type,
             typename enable_if<
                 ( is_integral<Type>::value  &&
                   is_unsigned<Type>::value  &&
                   !is_same<Type, bool>::value ),
                 int>::type = 0>
    RE_STD_CONSTEXPR any(
        Type _v
    ) RE_STD_NOEXCEPT
        : m_category(DAnyCategory::cat_unsigned),
          m_type_id(any_type_id_of<Type>::value),
          m_sbo(static_cast<unsigned long long>(_v),
                DAnyCategory::cat_unsigned)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: floating point
    // -----------------------------------------------------------------

    template<typename Type,
             typename enable_if<
                 is_floating_point<Type>::value,
                 int
             >::type = 0>
    RE_STD_CONSTEXPR any(
        Type _v
    ) RE_STD_NOEXCEPT
        : m_category(DAnyCategory::cat_floating),
          m_type_id(any_type_id_of<Type>::value),
          m_sbo(static_cast<double>(_v),
                DAnyCategory::cat_floating)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: enum
    // -----------------------------------------------------------------

#if RE_STD_HAS_IS_ENUM && RE_STD_HAS_UNDERLYING_TYPE
    template<typename Type,
             typename enable_if<
                 is_enum<Type>::value,
                 int
             >::type = 0>
    RE_STD_CONSTEXPR any(
        Type _v
    ) RE_STD_NOEXCEPT
        : m_category(
              static_cast<DAnyCategory::Value>(
                  internal::any_category_of<Type>::value)),
          m_type_id(any_type_id_of<Type>::value),
          m_sbo(static_cast<unsigned long long>(
                    static_cast<
                        typename underlying_type<Type>::type>(_v)),
                static_cast<DAnyCategory::Value>(
                    internal::any_category_of<Type>::value))
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}
#endif  // RE_STD_HAS_IS_ENUM && RE_STD_HAS_UNDERLYING_TYPE

    // -----------------------------------------------------------------
    //  function: non-const pointer (not function pointer)
    // -----------------------------------------------------------------

    template<typename Type,
             typename enable_if<
                 ( is_pointer<Type>::value &&
                   !is_const<
                       typename remove_pointer<Type>::type
                   >::value &&
                   !is_function<
                       typename remove_pointer<Type>::type
                   >::value ),
                 int>::type = 0>
    RE_STD_CONSTEXPR any(
        Type _v
    ) RE_STD_NOEXCEPT
        : m_category(DAnyCategory::cat_pointer),
          m_type_id(any_type_id_of<Type>::value),
          m_sbo(static_cast<void*>(_v),
                DAnyCategory::cat_pointer)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: const pointer (not function pointer)
    // -----------------------------------------------------------------

    template<typename Type,
             typename enable_if<
                 ( is_pointer<Type>::value &&
                   is_const<
                       typename remove_pointer<Type>::type
                   >::value &&
                   !is_function<
                       typename remove_pointer<Type>::type
                   >::value ),
                 int>::type = 0>
    RE_STD_CONSTEXPR any(
        Type _v
    ) RE_STD_NOEXCEPT
        : m_category(DAnyCategory::cat_cpointer),
          m_type_id(any_type_id_of<Type>::value),
          m_sbo(static_cast<const void*>(_v),
                DAnyCategory::cat_cpointer)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: heap (everything else)
    // -----------------------------------------------------------------

#if RE_STD_HAS_HEADER_NEW
    template<typename Type,
             typename enable_if<
                 ( !is_integral<Type>::value        &&
                   !is_floating_point<Type>::value   &&
                   !is_enum<Type>::value             &&
                   !is_pointer<Type>::value ),
                 int
             >::type = 0>
    any(
        const Type& _v
    )
        : m_category(DAnyCategory::cat_heap),
          m_type_id(any_type_id_of<Type>::value),
          m_sbo(),
          m_heap(new Type(_v)),
          m_heap_ops(internal::any_heap_ops_for<Type>())
    {}
#endif  // RE_STD_HAS_HEADER_NEW

    // =================================================================
    // CONSTRUCTORS: C++98/03 (explicit per-type overloads)
    // =================================================================

#else  // C++98/03

    // -----------------------------------------------------------------
    //  function: bool
    // -----------------------------------------------------------------

    any(
        bool _v
    )
        : m_category(DAnyCategory::cat_bool),
          m_type_id(any_type_id_of<bool>::value),
          m_sbo(_v, DAnyCategory::cat_bool)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: signed integrals
    // -----------------------------------------------------------------

    any(
        signed char _v
    )
        : m_category(DAnyCategory::cat_signed),
          m_type_id(any_type_id_of<signed char>::value),
          m_sbo(static_cast<long long>(_v), DAnyCategory::cat_signed)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    any(
        short _v
    )
        : m_category(DAnyCategory::cat_signed),
          m_type_id(any_type_id_of<short>::value),
          m_sbo(static_cast<long long>(_v), DAnyCategory::cat_signed)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    any(
        int _v
    )
        : m_category(DAnyCategory::cat_signed),
          m_type_id(any_type_id_of<int>::value),
          m_sbo(static_cast<long long>(_v), DAnyCategory::cat_signed)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    any(
        long _v
    )
        : m_category(DAnyCategory::cat_signed),
          m_type_id(any_type_id_of<long>::value),
          m_sbo(static_cast<long long>(_v), DAnyCategory::cat_signed)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    any(
        long long _v
    )
        : m_category(DAnyCategory::cat_signed),
          m_type_id(any_type_id_of<long long>::value),
          m_sbo(_v, DAnyCategory::cat_signed)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: unsigned integrals
    // -----------------------------------------------------------------

    any(
        unsigned char _v
    )
        : m_category(DAnyCategory::cat_unsigned),
          m_type_id(any_type_id_of<unsigned char>::value),
          m_sbo(static_cast<unsigned long long>(_v),
                DAnyCategory::cat_unsigned)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    any(
        unsigned short _v
    )
        : m_category(DAnyCategory::cat_unsigned),
          m_type_id(any_type_id_of<unsigned short>::value),
          m_sbo(static_cast<unsigned long long>(_v),
                DAnyCategory::cat_unsigned)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    any(
        unsigned int _v
    )
        : m_category(DAnyCategory::cat_unsigned),
          m_type_id(any_type_id_of<unsigned int>::value),
          m_sbo(static_cast<unsigned long long>(_v),
                DAnyCategory::cat_unsigned)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    any(
        unsigned long _v
    )
        : m_category(DAnyCategory::cat_unsigned),
          m_type_id(any_type_id_of<unsigned long>::value),
          m_sbo(static_cast<unsigned long long>(_v),
                DAnyCategory::cat_unsigned)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    any(
        unsigned long long _v
    )
        : m_category(DAnyCategory::cat_unsigned),
          m_type_id(any_type_id_of<unsigned long long>::value),
          m_sbo(_v, DAnyCategory::cat_unsigned)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: char / wchar_t (platform-dependent signedness)
    //  note: char may be signed or unsigned; the category_of
    // specialization handles this. Type identity is preserved.
    // -----------------------------------------------------------------

    any(
        char _v
    )
        : m_category(
              static_cast<DAnyCategory::Value>(
                  internal::any_category_of<char>::value)),
          m_type_id(any_type_id_of<char>::value),
          m_sbo(static_cast<long long>(_v),
                static_cast<DAnyCategory::Value>(
                    internal::any_category_of<char>::value))
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    any(
        wchar_t _v
    )
        : m_category(
              static_cast<DAnyCategory::Value>(
                  internal::any_category_of<wchar_t>::value)),
          m_type_id(any_type_id_of<wchar_t>::value),
          m_sbo(static_cast<long long>(_v),
                static_cast<DAnyCategory::Value>(
                    internal::any_category_of<wchar_t>::value))
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: floating point
    // -----------------------------------------------------------------

    any(
        float _v
    )
        : m_category(DAnyCategory::cat_floating),
          m_type_id(any_type_id_of<float>::value),
          m_sbo(static_cast<double>(_v),
                DAnyCategory::cat_floating)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    any(
        double _v
    )
        : m_category(DAnyCategory::cat_floating),
          m_type_id(any_type_id_of<double>::value),
          m_sbo(_v, DAnyCategory::cat_floating)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    any(
        long double _v
    )
        : m_category(DAnyCategory::cat_floating),
          m_type_id(any_type_id_of<long double>::value),
          m_sbo(static_cast<double>(_v),
                DAnyCategory::cat_floating)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: non-const pointer
    //  note: template deduction on Type* restricts to pointer
    // types. Function pointers will fail at the static_cast to
    // void* (compile error, not silent misbehavior).
    // -----------------------------------------------------------------

    template<typename Type>
    any(
        Type* _v
    )
        : m_category(DAnyCategory::cat_pointer),
          m_type_id(any_type_id_of<Type*>::value),
          m_sbo(static_cast<void*>(_v),
                DAnyCategory::cat_pointer)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: const pointer
    // -----------------------------------------------------------------

    template<typename Type>
    any(
        const Type* _v
    )
        : m_category(DAnyCategory::cat_cpointer),
          m_type_id(any_type_id_of<const Type*>::value),
          m_sbo(static_cast<const void*>(_v),
                DAnyCategory::cat_cpointer)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(RE_STD_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  function: heap (const reference - C++98 only)
    //  note: the template parameter is unconstrained in C++98.
    // Overload resolution prefers the explicit non-template
    // constructors above for SBO types; only non-SBO types
    // (class types, containers, etc.) reach this overload.
    // The pointer constructors above (taking Type* and
    // const Type*) are more specialized than this template
    // and will always be preferred for pointer arguments.
    // -----------------------------------------------------------------

#if RE_STD_HAS_HEADER_NEW
    template<typename Type>
    any(
        const Type& _v
    )
        : m_category(DAnyCategory::cat_heap),
          m_type_id(any_type_id_of<Type>::value),
          m_sbo(),
          m_heap(new Type(_v)),
          m_heap_ops(internal::any_heap_ops_for<Type>())
    {}
#endif  // RE_STD_HAS_HEADER_NEW

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER (constructors)

    // =================================================================
    // COPY / MOVE / DESTRUCTOR (shared across all tiers)
    // =================================================================

    // -----------------------------------------------------------------
    //  copy constructor
    // -----------------------------------------------------------------

    any(
        const any& _other
    )
        : m_category(_other.m_category),
          m_type_id(_other.m_type_id),
          m_sbo(_other.m_sbo)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(RE_STD_NULLPTR),
          m_heap_ops(_other.m_heap_ops)
#endif
    {
#if RE_STD_HAS_HEADER_NEW
        if ( (_other.m_category == DAnyCategory::cat_heap) &&
             (_other.m_heap != RE_STD_NULLPTR)                  &&
             (_other.m_heap_ops != RE_STD_NULLPTR) )
        {
            m_heap = m_heap_ops->clone(_other.m_heap);
        }
#endif
    }

    // -----------------------------------------------------------------
    //  move constructor (C++11+)
    // -----------------------------------------------------------------

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

    any(
        any&& _other
    ) RE_STD_NOEXCEPT
        : m_category(_other.m_category),
          m_type_id(_other.m_type_id),
          m_sbo(_other.m_sbo)
#if RE_STD_HAS_HEADER_NEW
        , m_heap(_other.m_heap),
          m_heap_ops(_other.m_heap_ops)
#endif
    {
        _other.m_category = DAnyCategory::cat_empty;
        _other.m_type_id  = RE_STD_NULLPTR;
#if RE_STD_HAS_HEADER_NEW
        _other.m_heap     = RE_STD_NULLPTR;
        _other.m_heap_ops = RE_STD_NULLPTR;
#endif
    }

#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES

    // -----------------------------------------------------------------
    //  copy assignment
    // -----------------------------------------------------------------

    any&
    operator=(
        const any& _other
    )
    {
        if (this != &_other)
        {
            reset();

            m_category = _other.m_category;
            m_type_id  = _other.m_type_id;
            m_sbo      = _other.m_sbo;

#if RE_STD_HAS_HEADER_NEW
            m_heap_ops = _other.m_heap_ops;

            if ( (_other.m_category == DAnyCategory::cat_heap) &&
                 (_other.m_heap != RE_STD_NULLPTR)                  &&
                 (_other.m_heap_ops != RE_STD_NULLPTR) )
            {
                m_heap = m_heap_ops->clone(_other.m_heap);
            }
#endif
        }

        return *this;
    }

    // -----------------------------------------------------------------
    //  move assignment (C++11+)
    // -----------------------------------------------------------------

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

    any&
    operator=(
        any&& _other
    ) RE_STD_NOEXCEPT
    {
        if (this != &_other)
        {
            reset();

            m_category        = _other.m_category;
            m_type_id         = _other.m_type_id;
            m_sbo             = _other.m_sbo;

#if RE_STD_HAS_HEADER_NEW
            m_heap            = _other.m_heap;
            m_heap_ops        = _other.m_heap_ops;
            _other.m_heap     = RE_STD_NULLPTR;
            _other.m_heap_ops = RE_STD_NULLPTR;
#endif

            _other.m_category = DAnyCategory::cat_empty;
            _other.m_type_id  = RE_STD_NULLPTR;
        }

        return *this;
    }

#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES

    // -----------------------------------------------------------------
    //  destructor
    // -----------------------------------------------------------------

    ~any()
    {
        reset();
    }

    // =================================================================
    // OBSERVERS
    // =================================================================
    //   constexpr from C++14 (RE_STD_CONSTEXPR_CPP14). any is not a literal type
    // -- it has a user-provided destructor -- and C++11 requires the class
    // of a constexpr member function to be one; C++14 dropped that rule.

    // has_value
    RE_STD_CONSTEXPR_CPP14 bool
    has_value() const RE_STD_NOEXCEPT
    {
        return (m_category != DAnyCategory::cat_empty);
    }

    // operator bool
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    RE_STD_CONSTEXPR_CPP14 explicit operator bool() const RE_STD_NOEXCEPT
    {
        return has_value();
    }
#else
    // safe-bool idiom (C++98/03)
    //   returns a pointer-to-member that is non-null when the
    // any contains a value. Prevents implicit conversion to int
    // while allowing use in boolean contexts (if, while, &&, etc.).
    operator safe_bool_type() const
    {
        return has_value() ? &any::safe_bool_fn : RE_STD_NULLPTR;
    }
#endif

    // category
    RE_STD_CONSTEXPR_CPP14 DAnyCategory::Value
    category() const RE_STD_NOEXCEPT
    {
        return m_category;
    }

    // type
    RE_STD_CONSTEXPR_CPP14 any_type_id
    type() const RE_STD_NOEXCEPT
    {
        return m_type_id;
    }

    // holds
    //   returns true if the stored value was originally of type Type.
    template<typename Type>
    RE_STD_CONSTEXPR_CPP14 bool
    holds() const RE_STD_NOEXCEPT
    {
        return (m_type_id == any_type_id_of<Type>::value);
    }

    // is_sbo
    RE_STD_CONSTEXPR_CPP14 bool
    is_sbo() const RE_STD_NOEXCEPT
    {
        return ( (m_category != DAnyCategory::cat_empty) &&
                 (m_category != DAnyCategory::cat_heap) );
    }

    // =================================================================
    // TYPED RETRIEVAL: get<T>()
    // =================================================================
    //
    // C++11+: SFINAE-dispatched overloads, one per SBO category
    //         plus heap const/mutable overloads.
    //
    // C++98:  Tag-dispatched via internal::get_tag. A single
    //         public get<T>() delegates to private get_impl<T>()
    //         overloads keyed by the type's SBO category.

    // =================================================================
    // C++11+ path: SFINAE-dispatched get<T>()
    // =================================================================

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // -----------------------------------------------------------------
    //  SBO - bool
    // -----------------------------------------------------------------

    template<typename Type,
             typename enable_if<
                 is_same<Type, bool>::value,
                 int
             >::type = 0>
    RE_STD_CONSTEXPR_CPP14 Type
    get() const RE_STD_NOEXCEPT
    {
        return m_sbo.v_bool;
    }

    // -----------------------------------------------------------------
    //  SBO - signed integral
    // -----------------------------------------------------------------

    template<typename Type,
             typename enable_if<
                 ( is_integral<Type>::value &&
                   is_signed<Type>::value   &&
                   !is_same<Type, bool>::value ),
                 int
             >::type = 0>
    RE_STD_CONSTEXPR_CPP14 Type
    get() const RE_STD_NOEXCEPT
    {
        return static_cast<Type>(m_sbo.v_signed);
    }

    // -----------------------------------------------------------------
    //  SBO - unsigned integral
    // -----------------------------------------------------------------

    template<typename Type,
             typename enable_if<
                 ( is_integral<Type>::value  &&
                   is_unsigned<Type>::value  &&
                   !is_same<Type, bool>::value ),
                 int
             >::type = 0>
    RE_STD_CONSTEXPR_CPP14 Type
    get() const RE_STD_NOEXCEPT
    {
        return static_cast<Type>(m_sbo.v_unsigned);
    }

    // -----------------------------------------------------------------
    //  SBO - floating point
    // -----------------------------------------------------------------

    template<typename Type,
             typename enable_if<
                 is_floating_point<Type>::value,
                 int
             >::type = 0>
    RE_STD_CONSTEXPR_CPP14 Type
    get() const RE_STD_NOEXCEPT
    {
        return static_cast<Type>(m_sbo.v_floating);
    }

    // -----------------------------------------------------------------
    //  SBO - enum
    // -----------------------------------------------------------------

#if RE_STD_HAS_IS_ENUM && RE_STD_HAS_UNDERLYING_TYPE
    template<typename Type,
             typename enable_if<
                 is_enum<Type>::value,
                 int
             >::type = 0>
    RE_STD_CONSTEXPR_CPP14 Type
    get() const RE_STD_NOEXCEPT
    {
        return static_cast<Type>(
            static_cast<
                typename underlying_type<Type>::type>(
                    m_sbo.v_unsigned));
    }
#endif  // RE_STD_HAS_IS_ENUM && RE_STD_HAS_UNDERLYING_TYPE

    // -----------------------------------------------------------------
    //  SBO - non-const pointer
    // -----------------------------------------------------------------

    template<typename Type,
             typename enable_if<
                 ( is_pointer<Type>::value &&
                   !is_const<
                       typename remove_pointer<Type>::type
                   >::value &&
                   !is_function<
                       typename remove_pointer<Type>::type
                   >::value ),
                 int
             >::type = 0>
    RE_STD_CONSTEXPR_CPP14 Type
    get() const RE_STD_NOEXCEPT
    {
        return static_cast<Type>(m_sbo.v_pointer);
    }

    // -----------------------------------------------------------------
    //  SBO - const pointer
    // -----------------------------------------------------------------

    template<typename Type,
             typename enable_if<
                 ( is_pointer<Type>::value &&
                   is_const<
                       typename remove_pointer<Type>::type
                   >::value &&
                   !is_function<
                       typename remove_pointer<Type>::type
                   >::value ),
                 int
             >::type = 0>
    RE_STD_CONSTEXPR_CPP14 Type
    get() const RE_STD_NOEXCEPT
    {
        return static_cast<Type>(m_sbo.v_cpointer);
    }

    // -----------------------------------------------------------------
    //  heap (const and mutable)
    // -----------------------------------------------------------------

#if RE_STD_HAS_HEADER_NEW

    template<typename Type,
             typename enable_if<
                 ( !is_integral<Type>::value        &&
                   !is_floating_point<Type>::value  &&
                   !is_enum<Type>::value            &&
                   !is_pointer<Type>::value ),
                 int>::type = 0>
    const Type&
    get() const
    {
        return *static_cast<const Type*>(m_heap);
    }

    template<typename Type,
             typename enable_if<
                 ( !is_integral<Type>::value        &&
                   !is_floating_point<Type>::value  &&
                   !is_enum<Type>::value            &&
                   !is_pointer<Type>::value ),
                 int>::type = 0>
    Type&
    get()
    {
        return *static_cast<Type*>(m_heap);
    }

#endif  // RE_STD_HAS_HEADER_NEW

    // =================================================================
    // C++98/03 path: tag-dispatched get<T>()
    // =================================================================

#else  // C++98/03

    // get (by value)
    //   returns the stored value cast to Type. Dispatches to
    // the appropriate SBO member or heap pointer based on the
    // type's storage category.
    template<typename Type>
    Type
    get() const
    {
        return get_impl<Type>(
            internal::get_tag<
                internal::any_category_of<Type>::value>());
    }

    // get (mutable reference - heap only)
#if RE_STD_HAS_HEADER_NEW
    template<typename Type>
    Type&
    get_mut()
    {
        return *static_cast<Type*>(m_heap);
    }
#endif  // RE_STD_HAS_HEADER_NEW

private:
    // -----------------------------------------------------------------
    //  get_impl: tag-dispatched overloads
    // -----------------------------------------------------------------

    // bool
    template<typename Type>
    Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_bool>
    ) const
    {
        return static_cast<Type>(m_sbo.v_bool);
    }

    // signed integral
    template<typename Type>
    Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_signed>
    ) const
    {
        return static_cast<Type>(m_sbo.v_signed);
    }

    // unsigned integral
    template<typename Type>
    Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_unsigned>
    ) const
    {
        return static_cast<Type>(m_sbo.v_unsigned);
    }

    // floating point
    template<typename Type>
    Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_floating>
    ) const
    {
        return static_cast<Type>(m_sbo.v_floating);
    }

    // non-const pointer
    template<typename Type>
    Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_pointer>
    ) const
    {
        return static_cast<Type>(m_sbo.v_pointer);
    }

    // const pointer
    template<typename Type>
    Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_cpointer>
    ) const
    {
        return static_cast<Type>(m_sbo.v_cpointer);
    }

    // heap
#if RE_STD_HAS_HEADER_NEW
    template<typename Type>
    Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_heap>
    ) const
    {
        return *static_cast<const Type*>(m_heap);
    }
#endif  // RE_STD_HAS_HEADER_NEW

public:

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER (get)

    // =================================================================
    // TYPED RETRIEVAL: get_ref<T>() (heap types)
    // =================================================================
    // Returns a reference to the heap-stored value. Only valid
    // when the any holds a heap-allocated value of type Type.
    // Used by any_cast pointer and reference overloads.

#if RE_STD_HAS_HEADER_NEW

    template<typename Type>
    Type&
    get_ref()
    {
        return *static_cast<Type*>(m_heap);
    }

    template<typename Type>
    const Type&
    get_ref() const
    {
        return *static_cast<const Type*>(m_heap);
    }

#endif  // RE_STD_HAS_HEADER_NEW

    // =================================================================
    // MODIFIERS
    // =================================================================

    // -----------------------------------------------------------------
    //  function: SBO path (C++11+ only - requires variadics)
    // -----------------------------------------------------------------

#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES

    template<typename    Type,
             typename... Args,
             typename enable_if<
                 ( internal::any_category_of<Type>::value !=
                   DAnyCategory::cat_heap ),
                 int>::type = 0>
    void
    emplace(
        Args&&... _args
    )
    {
        *this = any(Type(static_cast<Args&&>(_args)...));

        return;
    }

    // -----------------------------------------------------------------
    //  function: heap path
    // -----------------------------------------------------------------

#if RE_STD_HAS_HEADER_NEW

    template<typename    Type,
             typename... Args,
             typename enable_if<
                 ( internal::any_category_of<Type>::value ==
                   DAnyCategory::cat_heap ),
                 int>::type = 0>
    void
    emplace(
        Args&&... _args
    )
    {
        reset();

        m_heap     = new Type(static_cast<Args&&>(_args)...);
        m_heap_ops = internal::any_heap_ops_for<Type>();
        m_category = DAnyCategory::cat_heap;
        m_type_id  = any_type_id_of<Type>::value;

        return;
    }

    // -----------------------------------------------------------------
    //  function: heap path (initializer_list)
    // -----------------------------------------------------------------

    template<typename    Type,
             typename    U,
             typename... Args,
             typename enable_if<
                 ( internal::any_category_of<Type>::value ==
                   DAnyCategory::cat_heap ),
                 int>::type = 0>
    void
    emplace(
        std::initializer_list<U> _il,
        Args&&...                _args
    )
    {
        reset();

        m_heap     = new Type(_il, static_cast<Args&&>(_args)...);
        m_heap_ops = internal::any_heap_ops_for<Type>();
        m_category = DAnyCategory::cat_heap;
        m_type_id  = any_type_id_of<Type>::value;

        return;
    }

#endif  // RE_STD_HAS_HEADER_NEW
#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES

    // -----------------------------------------------------------------
    //  reset
    //    destroys the stored value and sets to empty.
    // -----------------------------------------------------------------

    void
    reset() RE_STD_NOEXCEPT
    {
#if RE_STD_HAS_HEADER_NEW
        if ( (m_category == DAnyCategory::cat_heap) &&
             (m_heap != RE_STD_NULLPTR)                  &&
             (m_heap_ops != RE_STD_NULLPTR) )
        {
            m_heap_ops->destroy(m_heap);
            m_heap     = RE_STD_NULLPTR;
            m_heap_ops = RE_STD_NULLPTR;
        }
#endif

        m_category = DAnyCategory::cat_empty;
        m_type_id  = RE_STD_NULLPTR;

        return;
    }

    // -----------------------------------------------------------------
    //  swap
    // -----------------------------------------------------------------

    void
    swap(
        any& _other
    ) RE_STD_NOEXCEPT
    {
        any tmp(*this);
        *this = _other;
        _other = tmp;

        return;
    }

private:
    DAnyCategory::Value            m_category;
    any_type_id                    m_type_id;
    internal::any_sbo              m_sbo;

#if RE_STD_HAS_HEADER_NEW
    void*                          m_heap;
    const internal::any_heap_ops*  m_heap_ops;
#endif
};


}  // re_std

RE_STD_LONG_LONG_DIAG_POP
#endif  // RE_STD_HAS_LONG_LONG


#endif  // RE_STD_ANY_ANY_HPP
