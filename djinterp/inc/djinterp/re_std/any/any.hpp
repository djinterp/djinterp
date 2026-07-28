/******************************************************************************
* djinterp [restd]                                                     any.hpp
*
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
*      Requires D_ENV_CPP98_HAS_NEW. NOT constexpr.
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
*   - Enum SBO gated on D_RESTD_HAS_IS_ENUM / D_RESTD_HAS_UNDERLYING_TYPE.
*   - Move semantics gated on D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES.
*   - Heap path gated on D_ENV_CPP98_HAS_NEW.
*   - Emplace gated on D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES.
*
*   Uses:
*     env.h              - language version detection
*     env_cpp98.h        - header availability (new, utility)
*     env_cpp_features.h - fine-grained feature detection
*     djinterp.hpp       - D_CONSTEXPR, D_STATIC, D_INLINE, namespaces
*     type_traits.hpp    - restd type traits (no <type_traits> dependency)
*
*
* TABLE OF CONTENTS
* =================
* 0.    COMPATIBILITY MACROS
* I.    TYPE IDENTITY
* II.   STORAGE CATEGORY
* III.  SBO STORAGE UNION
* IV.   HEAP CONTROL BLOCK
* V.    ANY CLASS
*
*
* path:      /inc/djinterp/restd/any/any.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.06
******************************************************************************/

#ifndef DJINTERP_RESTD_ANY_
#define DJINTERP_RESTD_ANY_ 1

// std
#include <cstddef>
// env detection headers (included transitively via djinterp.hpp,
// listed here for documentation)
//   env.h              - D_ENV_LANG_IS_CPP11_OR_HIGHER et al.
//   env_cpp98.h        - D_ENV_CPP98_HAS_NEW, D_ENV_CPP98_HAS_UTILITY
//   env_cpp_features.h - D_ENV_CPP_FEATURE_LANG_*, D_ENV_CPP_FEATURE_STL_*

#if D_ENV_CPP98_HAS_NEW
    #include <new>
#endif

#if D_ENV_CPP98_HAS_UTILITY
    #include <utility>
#endif

// restd
//#include "../type_traits/type_traits.hpp"
// djinterp
#include "../../core/djinterp.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================
// Null pointer constant for C++98/03 compatibility. On C++11+,
// resolves to nullptr. On C++98/03, resolves to 0.
// NOTE: should migrate to the core header in future.

#ifndef D_NULLPTR
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_NULLPTR   nullptr
    #else
        #define D_NULLPTR   0
    #endif
#endif


NS_RESTD

using std::enable_if;
using std::is_const;
using std::is_enum;
using std::is_floating_point;
using std::is_function;
using std::is_integral;
using std::is_pointer;
using std::is_same;
using std::is_signed;
using std::is_unsigned;
using std::remove_pointer;
using std::underlying_type;



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

NS_INTERNAL

    // any_type_tag_fn
    //   function: empty function template whose address is
    // unique per _Type instantiation. Never called.
    template<typename _Type>
    void
    any_type_tag_fn()
    {
        return;
    }

NS_END  // internal

// any_type_id_of
//   trait: yields the any_type_id for _Type.
template<typename _Type>
struct any_type_id_of
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_STATIC_CONSTEXPR any_type_id value = &internal::any_type_tag_fn<_Type>;
#else
    static const any_type_id value;
#endif
};

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // out-of-class definition (ODR safety)
    template<typename _Type>
    D_CONSTEXPR any_type_id any_type_id_of<_Type>::value;
#else
    // C++98/03: function pointer address requires out-of-class
    // definition; not a constant expression.
    template<typename _Type>
    const any_type_id any_type_id_of<_Type>::value =
        &internal::any_type_tag_fn<_Type>;
#endif

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // any_type_id_of_v
    //   variable template: value of any_type_id_of<_Type>.
    template<typename _Type>
    D_CONSTEXPR any_type_id any_type_id_of_v = any_type_id_of<_Type>::value;
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


NS_INTERNAL

    // any_category_of
    //   trait: maps a type to its SBO storage category.
    // Defaults to cat_heap for types not handled by the SBO.

    // -----------------------------------------------------------------
    // C++11+ path: SFINAE partial specializations
    // -----------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // primary template: heap fallback
    template<typename _Type,
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
    template<typename _Type>
    struct any_category_of<_Type,
        typename enable_if<
            ( is_integral<_Type>::value  &&
              is_signed<_Type>::value    &&
              !is_same<_Type, bool>::value )
        >::type>
    {
        static const int value = DAnyCategory::cat_signed;
    };

    // unsigned integrals (not bool)
    template<typename _Type>
    struct any_category_of<_Type,
        typename enable_if<
            ( is_integral<_Type>::value  &&
              is_unsigned<_Type>::value  &&
              !is_same<_Type, bool>::value )
        >::type>
    {
        static const int value = DAnyCategory::cat_unsigned;
    };

    // floating point
    template<typename _Type>
    struct any_category_of<_Type,
        typename enable_if<
            is_floating_point<_Type>::value
        >::type>
    {
        static const int value = DAnyCategory::cat_floating;
    };

    // enum types (stored via underlying integral)
#if D_RESTD_HAS_IS_ENUM && D_RESTD_HAS_UNDERLYING_TYPE
    template<typename _Type>
    struct any_category_of<_Type,
        typename enable_if<
            is_enum<_Type>::value
        >::type>
    {
        static const int value =
            ( is_signed<
                  typename underlying_type<_Type>::type
              >::value
              ? DAnyCategory::cat_signed
              : DAnyCategory::cat_unsigned );
    };
#endif  // D_RESTD_HAS_IS_ENUM && D_RESTD_HAS_UNDERLYING_TYPE

    // non-const pointer (not function pointer)
    template<typename _Type>
    struct any_category_of<_Type*,
        typename enable_if<
            ( !is_function<_Type>::value &&
              !is_const<_Type>::value )
        >::type>
    {
        static const int value = DAnyCategory::cat_pointer;
    };

    // const pointer (not function pointer)
    template<typename _Type>
    struct any_category_of<const _Type*,
        typename enable_if<
            !is_function<_Type>::value
        >::type>
    {
        static const int value = DAnyCategory::cat_cpointer;
    };

    // -----------------------------------------------------------------
    // C++98/03 path: explicit specializations
    // -----------------------------------------------------------------

#else  // C++98/03

    // primary template: heap fallback
    template<typename _Type>
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
    template<typename _Type>
    struct any_category_of<_Type*>
    {
        static const int value = DAnyCategory::cat_pointer;
    };

    template<typename _Type>
    struct any_category_of<const _Type*>
    {
        static const int value = DAnyCategory::cat_cpointer;
    };

    // note: function pointers will match _Type* and attempt
    // static_cast<void*>, which is ill-formed. This produces
    // a compile error (not silent misbehavior).

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // is_sbo_type
    //   trait: true if _Type uses the SBO path.
    template<typename _Type>
    struct is_sbo_type
    {
        static const bool value =
            ( any_category_of<_Type>::value != DAnyCategory::cat_heap );
    };

    // get_tag
    //   type: tag for category-based dispatch of get<T>().
    template<int _Cat>
    struct get_tag
    {};

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                III. SBO STORAGE UNION                                   ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

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
        D_CONSTEXPR any_sbo() D_NOEXCEPT
            : v_unsigned(0)
        {}

        // per-category constructors (the DAnyCategory::Value tag
        // parameter disambiguates overloads)
        D_CONSTEXPR explicit
        any_sbo(
            bool                _v,
            DAnyCategory::Value
        ) D_NOEXCEPT
            : v_bool(_v)
        {}

        D_CONSTEXPR explicit
        any_sbo(
            long long           _v,
            DAnyCategory::Value
        ) D_NOEXCEPT
            : v_signed(_v)
        {}

        D_CONSTEXPR explicit
        any_sbo(
            unsigned long long  _v,
            DAnyCategory::Value
        ) D_NOEXCEPT
            : v_unsigned(_v)
        {}

        D_CONSTEXPR explicit
        any_sbo(
            double              _v,
            DAnyCategory::Value
        ) D_NOEXCEPT
            : v_floating(_v)
        {}

        D_CONSTEXPR explicit
        any_sbo(
            void*               _v,
            DAnyCategory::Value
        ) D_NOEXCEPT
            : v_pointer(_v)
        {}

        D_CONSTEXPR explicit
        any_sbo(
            const void*         _v,
            DAnyCategory::Value
        ) D_NOEXCEPT
            : v_cpointer(_v)
        {}
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                IV.  HEAP CONTROL BLOCK                                  ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP98_HAS_NEW

NS_INTERNAL

    // any_heap_ops
    //   struct: type-erased operations table for heap storage.
    struct any_heap_ops
    {
        void  (*destroy)(void*);
        void* (*clone)(const void*);
    };

    // heap_destroy
    //   function: typed destroy operation for heap storage.
    template<typename _Type>
    D_STATIC void
    heap_destroy(
        void* _p
    )
    {
        delete static_cast<_Type*>(_p);

        return;
    }

    // heap_clone
    //   function: typed clone operation for heap storage.
    template<typename _Type>
    D_STATIC void*
    heap_clone(
        const void* _p
    )
    {
        return new _Type(*static_cast<const _Type*>(_p));
    }

    // any_heap_ops_for
    //   function: returns the operations table for _Type.
    // Uses a local static for safe lazy initialization
    // (thread-safe in C++11 per [stmt.dcl]/4).
    template<typename _Type>
    const any_heap_ops*
    any_heap_ops_for()
    {
        D_STATIC const any_heap_ops ops =
        {
            &heap_destroy<_Type>,
            &heap_clone<_Type>
        };

        return &ops;
    }

NS_END  // internal

#endif  // D_ENV_CPP98_HAS_NEW


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

#if (!D_ENV_LANG_IS_CPP11_OR_HIGHER)
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
    //  construction: empty
    // -----------------------------------------------------------------

    D_CONSTEXPR any() D_NOEXCEPT
        : m_category(DAnyCategory::cat_empty),
          m_type_id(D_NULLPTR),
          m_sbo()
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // =================================================================
    // CONSTRUCTORS: C++11+ (SFINAE-dispatched templates)
    // =================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // -----------------------------------------------------------------
    //  construction: bool
    // -----------------------------------------------------------------

    D_CONSTEXPR any(
        bool _v
    ) D_NOEXCEPT
        : m_category(DAnyCategory::cat_bool),
          m_type_id(any_type_id_of<bool>::value),
          m_sbo(_v, DAnyCategory::cat_bool)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: signed integrals (not bool)
    // -----------------------------------------------------------------

    template<typename _Type,
             typename enable_if<
                 ( is_integral<_Type>::value &&
                   is_signed<_Type>::value   &&
                   !is_same<_Type, bool>::value ),
                 int>::type = 0>
    D_CONSTEXPR any(
        _Type _v
    ) D_NOEXCEPT
        : m_category(DAnyCategory::cat_signed),
          m_type_id(any_type_id_of<_Type>::value),
          m_sbo(static_cast<long long>(_v), DAnyCategory::cat_signed)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: unsigned integrals (not bool)
    // -----------------------------------------------------------------

    template<typename _Type,
             typename enable_if<
                 ( is_integral<_Type>::value  &&
                   is_unsigned<_Type>::value  &&
                   !is_same<_Type, bool>::value ),
                 int>::type = 0>
    D_CONSTEXPR any(
        _Type _v
    ) D_NOEXCEPT
        : m_category(DAnyCategory::cat_unsigned),
          m_type_id(any_type_id_of<_Type>::value),
          m_sbo(static_cast<unsigned long long>(_v),
                DAnyCategory::cat_unsigned)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: floating point
    // -----------------------------------------------------------------

    template<typename _Type,
             typename enable_if<
                 is_floating_point<_Type>::value,
                 int
             >::type = 0>
    D_CONSTEXPR any(
        _Type _v
    ) D_NOEXCEPT
        : m_category(DAnyCategory::cat_floating),
          m_type_id(any_type_id_of<_Type>::value),
          m_sbo(static_cast<double>(_v),
                DAnyCategory::cat_floating)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: enum
    // -----------------------------------------------------------------

#if D_RESTD_HAS_IS_ENUM && D_RESTD_HAS_UNDERLYING_TYPE
    template<typename _Type,
             typename enable_if<
                 is_enum<_Type>::value,
                 int
             >::type = 0>
    D_CONSTEXPR any(
        _Type _v
    ) D_NOEXCEPT
        : m_category(
              static_cast<DAnyCategory::Value>(
                  internal::any_category_of<_Type>::value)),
          m_type_id(any_type_id_of<_Type>::value),
          m_sbo(static_cast<unsigned long long>(
                    static_cast<
                        typename underlying_type<_Type>::type>(_v)),
                static_cast<DAnyCategory::Value>(
                    internal::any_category_of<_Type>::value))
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}
#endif  // D_RESTD_HAS_IS_ENUM && D_RESTD_HAS_UNDERLYING_TYPE

    // -----------------------------------------------------------------
    //  construction: non-const pointer (not function pointer)
    // -----------------------------------------------------------------

    template<typename _Type,
             typename enable_if<
                 ( is_pointer<_Type>::value &&
                   !is_const<
                       typename remove_pointer<_Type>::type
                   >::value &&
                   !is_function<
                       typename remove_pointer<_Type>::type
                   >::value ),
                 int>::type = 0>
    D_CONSTEXPR any(
        _Type _v
    ) D_NOEXCEPT
        : m_category(DAnyCategory::cat_pointer),
          m_type_id(any_type_id_of<_Type>::value),
          m_sbo(static_cast<void*>(_v),
                DAnyCategory::cat_pointer)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: const pointer (not function pointer)
    // -----------------------------------------------------------------

    template<typename _Type,
             typename enable_if<
                 ( is_pointer<_Type>::value &&
                   is_const<
                       typename remove_pointer<_Type>::type
                   >::value &&
                   !is_function<
                       typename remove_pointer<_Type>::type
                   >::value ),
                 int>::type = 0>
    D_CONSTEXPR any(
        _Type _v
    ) D_NOEXCEPT
        : m_category(DAnyCategory::cat_cpointer),
          m_type_id(any_type_id_of<_Type>::value),
          m_sbo(static_cast<const void*>(_v),
                DAnyCategory::cat_cpointer)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: heap (everything else)
    // -----------------------------------------------------------------

#if D_ENV_CPP98_HAS_NEW
    template<typename _Type,
             typename enable_if<
                 ( !is_integral<_Type>::value        &&
                   !is_floating_point<_Type>::value   &&
                   !is_enum<_Type>::value             &&
                   !is_pointer<_Type>::value ),
                 int
             >::type = 0>
    any(
        const _Type& _v
    )
        : m_category(DAnyCategory::cat_heap),
          m_type_id(any_type_id_of<_Type>::value),
          m_sbo(),
          m_heap(new _Type(_v)),
          m_heap_ops(internal::any_heap_ops_for<_Type>())
    {}
#endif  // D_ENV_CPP98_HAS_NEW

    // =================================================================
    // CONSTRUCTORS: C++98/03 (explicit per-type overloads)
    // =================================================================

#else  // C++98/03

    // -----------------------------------------------------------------
    //  construction: bool
    // -----------------------------------------------------------------

    any(
        bool _v
    )
        : m_category(DAnyCategory::cat_bool),
          m_type_id(any_type_id_of<bool>::value),
          m_sbo(_v, DAnyCategory::cat_bool)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: signed integrals
    // -----------------------------------------------------------------

    any(
        signed char _v
    )
        : m_category(DAnyCategory::cat_signed),
          m_type_id(any_type_id_of<signed char>::value),
          m_sbo(static_cast<long long>(_v), DAnyCategory::cat_signed)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    any(
        short _v
    )
        : m_category(DAnyCategory::cat_signed),
          m_type_id(any_type_id_of<short>::value),
          m_sbo(static_cast<long long>(_v), DAnyCategory::cat_signed)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    any(
        int _v
    )
        : m_category(DAnyCategory::cat_signed),
          m_type_id(any_type_id_of<int>::value),
          m_sbo(static_cast<long long>(_v), DAnyCategory::cat_signed)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    any(
        long _v
    )
        : m_category(DAnyCategory::cat_signed),
          m_type_id(any_type_id_of<long>::value),
          m_sbo(static_cast<long long>(_v), DAnyCategory::cat_signed)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    any(
        long long _v
    )
        : m_category(DAnyCategory::cat_signed),
          m_type_id(any_type_id_of<long long>::value),
          m_sbo(_v, DAnyCategory::cat_signed)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: unsigned integrals
    // -----------------------------------------------------------------

    any(
        unsigned char _v
    )
        : m_category(DAnyCategory::cat_unsigned),
          m_type_id(any_type_id_of<unsigned char>::value),
          m_sbo(static_cast<unsigned long long>(_v),
                DAnyCategory::cat_unsigned)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    any(
        unsigned short _v
    )
        : m_category(DAnyCategory::cat_unsigned),
          m_type_id(any_type_id_of<unsigned short>::value),
          m_sbo(static_cast<unsigned long long>(_v),
                DAnyCategory::cat_unsigned)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    any(
        unsigned int _v
    )
        : m_category(DAnyCategory::cat_unsigned),
          m_type_id(any_type_id_of<unsigned int>::value),
          m_sbo(static_cast<unsigned long long>(_v),
                DAnyCategory::cat_unsigned)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    any(
        unsigned long _v
    )
        : m_category(DAnyCategory::cat_unsigned),
          m_type_id(any_type_id_of<unsigned long>::value),
          m_sbo(static_cast<unsigned long long>(_v),
                DAnyCategory::cat_unsigned)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    any(
        unsigned long long _v
    )
        : m_category(DAnyCategory::cat_unsigned),
          m_type_id(any_type_id_of<unsigned long long>::value),
          m_sbo(_v, DAnyCategory::cat_unsigned)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: char / wchar_t (platform-dependent signedness)
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
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
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
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: floating point
    // -----------------------------------------------------------------

    any(
        float _v
    )
        : m_category(DAnyCategory::cat_floating),
          m_type_id(any_type_id_of<float>::value),
          m_sbo(static_cast<double>(_v),
                DAnyCategory::cat_floating)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    any(
        double _v
    )
        : m_category(DAnyCategory::cat_floating),
          m_type_id(any_type_id_of<double>::value),
          m_sbo(_v, DAnyCategory::cat_floating)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    any(
        long double _v
    )
        : m_category(DAnyCategory::cat_floating),
          m_type_id(any_type_id_of<long double>::value),
          m_sbo(static_cast<double>(_v),
                DAnyCategory::cat_floating)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: non-const pointer
    //  note: template deduction on _Type* restricts to pointer
    // types. Function pointers will fail at the static_cast to
    // void* (compile error, not silent misbehavior).
    // -----------------------------------------------------------------

    template<typename _Type>
    any(
        _Type* _v
    )
        : m_category(DAnyCategory::cat_pointer),
          m_type_id(any_type_id_of<_Type*>::value),
          m_sbo(static_cast<void*>(_v),
                DAnyCategory::cat_pointer)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: const pointer
    // -----------------------------------------------------------------

    template<typename _Type>
    any(
        const _Type* _v
    )
        : m_category(DAnyCategory::cat_cpointer),
          m_type_id(any_type_id_of<const _Type*>::value),
          m_sbo(static_cast<const void*>(_v),
                DAnyCategory::cat_cpointer)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(D_NULLPTR)
#endif
    {}

    // -----------------------------------------------------------------
    //  construction: heap (const reference - C++98 only)
    //  note: the template parameter is unconstrained in C++98.
    // Overload resolution prefers the explicit non-template
    // constructors above for SBO types; only non-SBO types
    // (class types, containers, etc.) reach this overload.
    // The pointer constructors above (taking _Type* and
    // const _Type*) are more specialized than this template
    // and will always be preferred for pointer arguments.
    // -----------------------------------------------------------------

#if D_ENV_CPP98_HAS_NEW
    template<typename _Type>
    any(
        const _Type& _v
    )
        : m_category(DAnyCategory::cat_heap),
          m_type_id(any_type_id_of<_Type>::value),
          m_sbo(),
          m_heap(new _Type(_v)),
          m_heap_ops(internal::any_heap_ops_for<_Type>())
    {}
#endif  // D_ENV_CPP98_HAS_NEW

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER (constructors)

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
#if D_ENV_CPP98_HAS_NEW
        , m_heap(D_NULLPTR),
          m_heap_ops(_other.m_heap_ops)
#endif
    {
#if D_ENV_CPP98_HAS_NEW
        if ( (_other.m_category == DAnyCategory::cat_heap) &&
             (_other.m_heap != D_NULLPTR)                  &&
             (_other.m_heap_ops != D_NULLPTR) )
        {
            m_heap = m_heap_ops->clone(_other.m_heap);
        }
#endif
    }

    // -----------------------------------------------------------------
    //  move constructor (C++11+)
    // -----------------------------------------------------------------

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    any(
        any&& _other
    ) D_NOEXCEPT
        : m_category(_other.m_category),
          m_type_id(_other.m_type_id),
          m_sbo(_other.m_sbo)
#if D_ENV_CPP98_HAS_NEW
        , m_heap(_other.m_heap),
          m_heap_ops(_other.m_heap_ops)
#endif
    {
        _other.m_category = DAnyCategory::cat_empty;
        _other.m_type_id  = D_NULLPTR;
#if D_ENV_CPP98_HAS_NEW
        _other.m_heap     = D_NULLPTR;
        _other.m_heap_ops = D_NULLPTR;
#endif
    }

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

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

#if D_ENV_CPP98_HAS_NEW
            m_heap_ops = _other.m_heap_ops;

            if ( (_other.m_category == DAnyCategory::cat_heap) &&
                 (_other.m_heap != D_NULLPTR)                  &&
                 (_other.m_heap_ops != D_NULLPTR) )
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

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    any&
    operator=(
        any&& _other
    ) D_NOEXCEPT
    {
        if (this != &_other)
        {
            reset();

            m_category        = _other.m_category;
            m_type_id         = _other.m_type_id;
            m_sbo             = _other.m_sbo;

#if D_ENV_CPP98_HAS_NEW
            m_heap            = _other.m_heap;
            m_heap_ops        = _other.m_heap_ops;
            _other.m_heap     = D_NULLPTR;
            _other.m_heap_ops = D_NULLPTR;
#endif

            _other.m_category = DAnyCategory::cat_empty;
            _other.m_type_id  = D_NULLPTR;
        }

        return *this;
    }

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

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

    // has_value
    D_CONSTEXPR bool
    has_value() const D_NOEXCEPT
    {
        return (m_category != DAnyCategory::cat_empty);
    }

    // operator bool
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_CONSTEXPR explicit operator bool() const D_NOEXCEPT
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
        return has_value() ? &any::safe_bool_fn : D_NULLPTR;
    }
#endif

    // category
    D_CONSTEXPR DAnyCategory::Value
    category() const D_NOEXCEPT
    {
        return m_category;
    }

    // type
    D_CONSTEXPR any_type_id
    type() const D_NOEXCEPT
    {
        return m_type_id;
    }

    // holds
    //   returns true if the stored value was originally of type _Type.
    template<typename _Type>
    D_CONSTEXPR bool
    holds() const D_NOEXCEPT
    {
        return (m_type_id == any_type_id_of<_Type>::value);
    }

    // is_sbo
    D_CONSTEXPR bool
    is_sbo() const D_NOEXCEPT
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

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // -----------------------------------------------------------------
    //  SBO - bool
    // -----------------------------------------------------------------

    template<typename _Type,
             typename enable_if<
                 is_same<_Type, bool>::value,
                 int
             >::type = 0>
    D_CONSTEXPR _Type
    get() const D_NOEXCEPT
    {
        return m_sbo.v_bool;
    }

    // -----------------------------------------------------------------
    //  SBO - signed integral
    // -----------------------------------------------------------------

    template<typename _Type,
             typename enable_if<
                 ( is_integral<_Type>::value &&
                   is_signed<_Type>::value   &&
                   !is_same<_Type, bool>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR _Type
    get() const D_NOEXCEPT
    {
        return static_cast<_Type>(m_sbo.v_signed);
    }

    // -----------------------------------------------------------------
    //  SBO - unsigned integral
    // -----------------------------------------------------------------

    template<typename _Type,
             typename enable_if<
                 ( is_integral<_Type>::value  &&
                   is_unsigned<_Type>::value  &&
                   !is_same<_Type, bool>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR _Type
    get() const D_NOEXCEPT
    {
        return static_cast<_Type>(m_sbo.v_unsigned);
    }

    // -----------------------------------------------------------------
    //  SBO - floating point
    // -----------------------------------------------------------------

    template<typename _Type,
             typename enable_if<
                 is_floating_point<_Type>::value,
                 int
             >::type = 0>
    D_CONSTEXPR _Type
    get() const D_NOEXCEPT
    {
        return static_cast<_Type>(m_sbo.v_floating);
    }

    // -----------------------------------------------------------------
    //  SBO - enum
    // -----------------------------------------------------------------

#if D_RESTD_HAS_IS_ENUM && D_RESTD_HAS_UNDERLYING_TYPE
    template<typename _Type,
             typename enable_if<
                 is_enum<_Type>::value,
                 int
             >::type = 0>
    D_CONSTEXPR _Type
    get() const D_NOEXCEPT
    {
        return static_cast<_Type>(
            static_cast<
                typename underlying_type<_Type>::type>(
                    m_sbo.v_unsigned));
    }
#endif  // D_RESTD_HAS_IS_ENUM && D_RESTD_HAS_UNDERLYING_TYPE

    // -----------------------------------------------------------------
    //  SBO - non-const pointer
    // -----------------------------------------------------------------

    template<typename _Type,
             typename enable_if<
                 ( is_pointer<_Type>::value &&
                   !is_const<
                       typename remove_pointer<_Type>::type
                   >::value &&
                   !is_function<
                       typename remove_pointer<_Type>::type
                   >::value ),
                 int
             >::type = 0>
    D_CONSTEXPR _Type
    get() const D_NOEXCEPT
    {
        return static_cast<_Type>(m_sbo.v_pointer);
    }

    // -----------------------------------------------------------------
    //  SBO - const pointer
    // -----------------------------------------------------------------

    template<typename _Type,
             typename enable_if<
                 ( is_pointer<_Type>::value &&
                   is_const<
                       typename remove_pointer<_Type>::type
                   >::value &&
                   !is_function<
                       typename remove_pointer<_Type>::type
                   >::value ),
                 int
             >::type = 0>
    D_CONSTEXPR _Type
    get() const D_NOEXCEPT
    {
        return static_cast<_Type>(m_sbo.v_cpointer);
    }

    // -----------------------------------------------------------------
    //  heap (const and mutable)
    // -----------------------------------------------------------------

#if D_ENV_CPP98_HAS_NEW

    template<typename _Type,
             typename enable_if<
                 ( !is_integral<_Type>::value        &&
                   !is_floating_point<_Type>::value  &&
                   !is_enum<_Type>::value            &&
                   !is_pointer<_Type>::value ),
                 int>::type = 0>
    const _Type&
    get() const
    {
        return *static_cast<const _Type*>(m_heap);
    }

    template<typename _Type,
             typename enable_if<
                 ( !is_integral<_Type>::value        &&
                   !is_floating_point<_Type>::value  &&
                   !is_enum<_Type>::value            &&
                   !is_pointer<_Type>::value ),
                 int>::type = 0>
    _Type&
    get()
    {
        return *static_cast<_Type*>(m_heap);
    }

#endif  // D_ENV_CPP98_HAS_NEW

    // =================================================================
    // C++98/03 path: tag-dispatched get<T>()
    // =================================================================

#else  // C++98/03

    // get (by value)
    //   returns the stored value cast to _Type. Dispatches to
    // the appropriate SBO member or heap pointer based on the
    // type's storage category.
    template<typename _Type>
    _Type
    get() const
    {
        return get_impl<_Type>(
            internal::get_tag<
                internal::any_category_of<_Type>::value>());
    }

    // get (mutable reference - heap only)
#if D_ENV_CPP98_HAS_NEW
    template<typename _Type>
    _Type&
    get_mut()
    {
        return *static_cast<_Type*>(m_heap);
    }
#endif  // D_ENV_CPP98_HAS_NEW

private:
    // -----------------------------------------------------------------
    //  get_impl: tag-dispatched overloads
    // -----------------------------------------------------------------

    // bool
    template<typename _Type>
    _Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_bool>
    ) const
    {
        return static_cast<_Type>(m_sbo.v_bool);
    }

    // signed integral
    template<typename _Type>
    _Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_signed>
    ) const
    {
        return static_cast<_Type>(m_sbo.v_signed);
    }

    // unsigned integral
    template<typename _Type>
    _Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_unsigned>
    ) const
    {
        return static_cast<_Type>(m_sbo.v_unsigned);
    }

    // floating point
    template<typename _Type>
    _Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_floating>
    ) const
    {
        return static_cast<_Type>(m_sbo.v_floating);
    }

    // non-const pointer
    template<typename _Type>
    _Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_pointer>
    ) const
    {
        return static_cast<_Type>(m_sbo.v_pointer);
    }

    // const pointer
    template<typename _Type>
    _Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_cpointer>
    ) const
    {
        return static_cast<_Type>(m_sbo.v_cpointer);
    }

    // heap
#if D_ENV_CPP98_HAS_NEW
    template<typename _Type>
    _Type
    get_impl(
        internal::get_tag<DAnyCategory::cat_heap>
    ) const
    {
        return *static_cast<const _Type*>(m_heap);
    }
#endif  // D_ENV_CPP98_HAS_NEW

public:

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER (get)

    // =================================================================
    // TYPED RETRIEVAL: get_ref<T>() (heap types)
    // =================================================================
    // Returns a reference to the heap-stored value. Only valid
    // when the any holds a heap-allocated value of type _Type.
    // Used by any_cast pointer and reference overloads.

#if D_ENV_CPP98_HAS_NEW

    template<typename _Type>
    _Type&
    get_ref()
    {
        return *static_cast<_Type*>(m_heap);
    }

    template<typename _Type>
    const _Type&
    get_ref() const
    {
        return *static_cast<const _Type*>(m_heap);
    }

#endif  // D_ENV_CPP98_HAS_NEW

    // =================================================================
    // MODIFIERS
    // =================================================================

    // -----------------------------------------------------------------
    //  emplace: SBO path (C++11+ only - requires variadics)
    // -----------------------------------------------------------------

#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

    template<typename    _Type,
             typename... _Args,
             typename enable_if<
                 ( internal::any_category_of<_Type>::value !=
                   DAnyCategory::cat_heap ),
                 int>::type = 0>
    void
    emplace(
        _Args&&... _args
    )
    {
        *this = any(_Type(static_cast<_Args&&>(_args)...));

        return;
    }

    // -----------------------------------------------------------------
    //  emplace: heap path
    // -----------------------------------------------------------------

#if D_ENV_CPP98_HAS_NEW

    template<typename    _Type,
             typename... _Args,
             typename enable_if<
                 ( internal::any_category_of<_Type>::value ==
                   DAnyCategory::cat_heap ),
                 int>::type = 0>
    void
    emplace(
        _Args&&... _args
    )
    {
        reset();

        m_heap     = new _Type(static_cast<_Args&&>(_args)...);
        m_heap_ops = internal::any_heap_ops_for<_Type>();
        m_category = DAnyCategory::cat_heap;
        m_type_id  = any_type_id_of<_Type>::value;

        return;
    }

    // -----------------------------------------------------------------
    //  emplace: heap path (initializer_list)
    // -----------------------------------------------------------------

    template<typename    _Type,
             typename    _U,
             typename... _Args,
             typename enable_if<
                 ( internal::any_category_of<_Type>::value ==
                   DAnyCategory::cat_heap ),
                 int>::type = 0>
    void
    emplace(
        std::initializer_list<_U> _il,
        _Args&&...                _args
    )
    {
        reset();

        m_heap     = new _Type(_il, static_cast<_Args&&>(_args)...);
        m_heap_ops = internal::any_heap_ops_for<_Type>();
        m_category = DAnyCategory::cat_heap;
        m_type_id  = any_type_id_of<_Type>::value;

        return;
    }

#endif  // D_ENV_CPP98_HAS_NEW
#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

    // -----------------------------------------------------------------
    //  reset
    //    destroys the stored value and sets to empty.
    // -----------------------------------------------------------------

    void
    reset() D_NOEXCEPT
    {
#if D_ENV_CPP98_HAS_NEW
        if ( (m_category == DAnyCategory::cat_heap) &&
             (m_heap != D_NULLPTR)                  &&
             (m_heap_ops != D_NULLPTR) )
        {
            m_heap_ops->destroy(m_heap);
            m_heap     = D_NULLPTR;
            m_heap_ops = D_NULLPTR;
        }
#endif

        m_category = DAnyCategory::cat_empty;
        m_type_id  = D_NULLPTR;

        return;
    }

    // -----------------------------------------------------------------
    //  swap
    // -----------------------------------------------------------------

    void
    swap(
        any& _other
    ) D_NOEXCEPT
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

#if D_ENV_CPP98_HAS_NEW
    void*                          m_heap;
    const internal::any_heap_ops*  m_heap_ops;
#endif
};


NS_END  // restd


#endif  // DJINTERP_RESTD_ANY_
