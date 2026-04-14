/******************************************************************************
* djinterp [compat]                                                    any.hpp
*
*   Constexpr-friendly type-erased value container. A portable alternative
* to std::any with compile-time evaluation support for small trivial types.
*
*   TWO STORAGE PATHS:
*
*   1. SBO (small buffer optimization) — constexpr-capable (C++14+).
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
*
*   2. Heap — runtime only.
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
*   - Requires C++11 (for <type_traits>, enum class, enable_if).
*   - C++14+: constexpr SBO construction and retrieval.
*   - C++11:  SBO fully functional but not constexpr.
*   - Move semantics gated on D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES.
*   - Heap path gated on D_ENV_CPP98_HAS_NEW.
*   - When D_ENV_CPP_FEATURE_STL_ANY is available (C++17), users may
*     prefer std::any for non-constexpr paths.
*
*   Uses:
*     env.h              — language version detection
*     env_cpp98.h        — header availability (new, utility)
*     env_cpp_features.h — fine-grained feature detection
*     djinterp.hpp       — D_CONSTEXPR, D_STATIC, D_INLINE, namespaces
*
*
* TABLE OF CONTENTS
* =================
* I.    TYPE IDENTITY
* II.   STORAGE CATEGORY
* III.  SBO STORAGE UNION
* IV.   HEAP CONTROL BLOCK
* V.    ANY CLASS
* VI.   FREE FUNCTIONS (any_cast, swap)
*
*
* path:      /inc/djinterp/compat/std/any.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.06
******************************************************************************/

#ifndef DJINTERP_COMPATABILITY_STD_ANY_
#define DJINTERP_COMPATABILITY_STD_ANY_ 1

#include <cstddef>
#include <type_traits>
#include "../../core/djinterp.hpp"


// env detection headers (included transitively via djinterp.hpp,
// listed here for documentation)
//   env.h              — D_ENV_LANG_IS_CPP11_OR_HIGHER et al.
//   env_cpp98.h        — D_ENV_CPP98_HAS_NEW, D_ENV_CPP98_HAS_UTILITY
//   env_cpp_features.h — D_ENV_CPP_FEATURE_LANG_*, D_ENV_CPP_FEATURE_STL_*

// guard: entire module requires C++11
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_CPP98_HAS_NEW
    #include <new>
#endif

#if D_ENV_CPP98_HAS_UTILITY
    #include <utility>

#endif

NS_DJINTERP
NS_COMPAT

///////////////////////////////////////////////////////////////////////////////
///                I.   TYPE IDENTITY                                       ///
///////////////////////////////////////////////////////////////////////////////
// A unique identity per type, derived from the address of a
// function template instantiation. No RTTI required. The address
// of each instantiation is a constant expression, enabling
// constexpr type checking.

// any_type_id
//   type: opaque identifier for a stored type.
using any_type_id = void(*)();

NS_INTERNAL

    // any_type_tag_fn
    //   function: empty function template whose address is
    // unique per _Type instantiation. Never called.
    template<typename _Type>
    void any_type_tag_fn()
    {
        return;
    }

NS_END  // internal

// any_type_id_of
//   trait: yields the any_type_id for _Type.
template<typename _Type>
struct any_type_id_of
{
    D_STATIC D_CONSTEXPR any_type_id value =
        &internal::any_type_tag_fn<_Type>;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // any_type_id_of_v
    //   variable template: value of any_type_id_of<_Type>.
    template<typename _Type>
    D_CONSTEXPR any_type_id any_type_id_of_v =
        any_type_id_of<_Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///                II.  STORAGE CATEGORY                                    ///
///////////////////////////////////////////////////////////////////////////////

// DAnyCategory
//   enum: identifies which union member is active.
enum class DAnyCategory : unsigned
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

NS_INTERNAL

    // any_category_of
    //   trait: maps a type to its SBO storage category.
    // Defaults to cat_heap for types not handled by the SBO.

    // primary template: heap fallback
    template<typename _Type,
             typename = void>
    struct any_category_of
    {
        D_STATIC D_CONSTEXPR DAnyCategory value =
            DAnyCategory::cat_heap;
    };

    // bool
    template<>
    struct any_category_of<bool>
    {
        D_STATIC D_CONSTEXPR DAnyCategory value =
            DAnyCategory::cat_bool;
    };

    // signed integrals (not bool)
    template<typename _Type>
    struct any_category_of<_Type,
        typename std::enable_if<
            ( std::is_integral<_Type>::value &&
              std::is_signed<_Type>::value   &&
              !std::is_same<_Type, bool>::value )
        >::type>
    {
        D_STATIC D_CONSTEXPR DAnyCategory value =
            DAnyCategory::cat_signed;
    };

    // unsigned integrals (not bool)
    template<typename _Type>
    struct any_category_of<_Type,
        typename std::enable_if<
            ( std::is_integral<_Type>::value  &&
              std::is_unsigned<_Type>::value  &&
              !std::is_same<_Type, bool>::value )
        >::type>
    {
        D_STATIC D_CONSTEXPR DAnyCategory value =
            DAnyCategory::cat_unsigned;
    };

    // floating point
    template<typename _Type>
    struct any_category_of<_Type,
        typename std::enable_if<
            std::is_floating_point<_Type>::value
        >::type>
    {
        D_STATIC D_CONSTEXPR DAnyCategory value =
            DAnyCategory::cat_floating;
    };

    // enum types (stored via underlying integral)
    template<typename _Type>
    struct any_category_of<_Type,
        typename std::enable_if<
            std::is_enum<_Type>::value
        >::type>
    {
        D_STATIC D_CONSTEXPR DAnyCategory value =
            ( std::is_signed<
                  typename std::underlying_type<_Type>::type
              >::value
              ? DAnyCategory::cat_signed
              : DAnyCategory::cat_unsigned );
    };

    // non-const pointer (not function pointer)
    template<typename _Type>
    struct any_category_of<_Type*,
        typename std::enable_if<
            ( !std::is_function<_Type>::value &&
              !std::is_const<_Type>::value )
        >::type>
    {
        D_STATIC D_CONSTEXPR DAnyCategory value =
            DAnyCategory::cat_pointer;
    };

    // const pointer (not function pointer)
    template<typename _Type>
    struct any_category_of<const _Type*,
        typename std::enable_if<
            !std::is_function<_Type>::value
        >::type>
    {
        D_STATIC D_CONSTEXPR DAnyCategory value =
            DAnyCategory::cat_cpointer;
    };

    // is_sbo_type
    //   trait: true if _Type uses the SBO path.
    template<typename _Type>
    struct is_sbo_type
    {
        D_STATIC D_CONSTEXPR bool value =
            ( any_category_of<_Type>::value !=
              DAnyCategory::cat_heap );
    };

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
        D_CONSTEXPR any_sbo() noexcept
                : v_unsigned(0)
            {}

        // per-category constructors (the DAnyCategory tag
        // parameter disambiguates overloads)
        D_CONSTEXPR explicit any_sbo(
                bool         _v,
                DAnyCategory
            ) noexcept
                : v_bool(_v)
            {}

        D_CONSTEXPR explicit any_sbo(
                long long    _v,
                DAnyCategory
            ) noexcept
                : v_signed(_v)
            {}

        D_CONSTEXPR explicit any_sbo(
                unsigned long long _v,
                DAnyCategory
            ) noexcept
                : v_unsigned(_v)
            {}

        D_CONSTEXPR explicit any_sbo(
                double       _v,
                DAnyCategory
            ) noexcept
                : v_floating(_v)
            {}

        D_CONSTEXPR explicit any_sbo(
                void*        _v,
                DAnyCategory
            ) noexcept
                : v_pointer(_v)
            {}

        D_CONSTEXPR explicit any_sbo(
                const void*  _v,
                DAnyCategory
            ) noexcept
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
    D_STATIC void heap_destroy(
            void* _p
        )
    {
        delete static_cast<_Type*>(_p);

        return;
    }

    // heap_clone
    //   function: typed clone operation for heap storage.
    template<typename _Type>
    D_STATIC void* heap_clone(
            const void* _p
        )
    {
        return new _Type(
            *static_cast<const _Type*>(_p));
    }

    // any_heap_ops_for
    //   function: returns the operations table for _Type.
    // Uses a local static for safe lazy initialization
    // (thread-safe in C++11 per [stmt.dcl]/4).
    template<typename _Type>
    const any_heap_ops* any_heap_ops_for()
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
public:
    // -----------------------------------------------------------------
    //  construction: empty
    // -----------------------------------------------------------------

    D_CONSTEXPR any() noexcept
            : m_category(DAnyCategory::cat_empty),
              m_type_id(nullptr),
              m_sbo()
#if D_ENV_CPP98_HAS_NEW
            , m_heap(nullptr),
              m_heap_ops(nullptr)
#endif
        {}

    // -----------------------------------------------------------------
    //  construction: bool
    // -----------------------------------------------------------------

    D_CONSTEXPR any(
            bool _v
        ) noexcept
            : m_category(DAnyCategory::cat_bool),
              m_type_id(any_type_id_of<bool>::value),
              m_sbo(_v, DAnyCategory::cat_bool)
#if D_ENV_CPP98_HAS_NEW
            , m_heap(nullptr),
              m_heap_ops(nullptr)
#endif
        {}

    // -----------------------------------------------------------------
    //  construction: signed integrals (not bool)
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 ( std::is_integral<_T>::value &&
                   std::is_signed<_T>::value   &&
                   !std::is_same<_T, bool>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR any(
            _T _v
        ) noexcept
            : m_category(DAnyCategory::cat_signed),
              m_type_id(any_type_id_of<_T>::value),
              m_sbo(static_cast<long long>(_v),
                    DAnyCategory::cat_signed)
#if D_ENV_CPP98_HAS_NEW
            , m_heap(nullptr),
              m_heap_ops(nullptr)
#endif
        {}

    // -----------------------------------------------------------------
    //  construction: unsigned integrals (not bool)
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 ( std::is_integral<_T>::value  &&
                   std::is_unsigned<_T>::value  &&
                   !std::is_same<_T, bool>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR any(
            _T _v
        ) noexcept
            : m_category(DAnyCategory::cat_unsigned),
              m_type_id(any_type_id_of<_T>::value),
              m_sbo(static_cast<unsigned long long>(_v),
                    DAnyCategory::cat_unsigned)
#if D_ENV_CPP98_HAS_NEW
            , m_heap(nullptr),
              m_heap_ops(nullptr)
#endif
        {}

    // -----------------------------------------------------------------
    //  construction: floating point
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 std::is_floating_point<_T>::value,
                 int
             >::type = 0>
    D_CONSTEXPR any(
            _T _v
        ) noexcept
            : m_category(DAnyCategory::cat_floating),
              m_type_id(any_type_id_of<_T>::value),
              m_sbo(static_cast<double>(_v),
                    DAnyCategory::cat_floating)
#if D_ENV_CPP98_HAS_NEW
            , m_heap(nullptr),
              m_heap_ops(nullptr)
#endif
        {}

    // -----------------------------------------------------------------
    //  construction: enum
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 std::is_enum<_T>::value,
                 int
             >::type = 0>
    D_CONSTEXPR any(
            _T _v
        ) noexcept
            : m_category(
                  internal::any_category_of<_T>::value),
              m_type_id(any_type_id_of<_T>::value),
              m_sbo(static_cast<unsigned long long>(
                        static_cast<
                            typename std::underlying_type<
                                _T>::type>(_v)),
                    internal::any_category_of<_T>::value)
#if D_ENV_CPP98_HAS_NEW
            , m_heap(nullptr),
              m_heap_ops(nullptr)
#endif
        {}

    // -----------------------------------------------------------------
    //  construction: non-const pointer (not function)
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 ( std::is_pointer<_T>::value             &&
                   !std::is_const<
                       typename std::remove_pointer<
                           _T>::type>::value              &&
                   !std::is_function<
                       typename std::remove_pointer<
                           _T>::type>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR any(
            _T _v
        ) noexcept
            : m_category(DAnyCategory::cat_pointer),
              m_type_id(any_type_id_of<_T>::value),
              m_sbo(static_cast<void*>(_v),
                    DAnyCategory::cat_pointer)
#if D_ENV_CPP98_HAS_NEW
            , m_heap(nullptr),
              m_heap_ops(nullptr)
#endif
        {}

    // -----------------------------------------------------------------
    //  construction: const pointer (not function)
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 ( std::is_pointer<_T>::value             &&
                   std::is_const<
                       typename std::remove_pointer<
                           _T>::type>::value              &&
                   !std::is_function<
                       typename std::remove_pointer<
                           _T>::type>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR any(
            _T _v
        ) noexcept
            : m_category(DAnyCategory::cat_cpointer),
              m_type_id(any_type_id_of<_T>::value),
              m_sbo(static_cast<const void*>(_v),
                    DAnyCategory::cat_cpointer)
#if D_ENV_CPP98_HAS_NEW
            , m_heap(nullptr),
              m_heap_ops(nullptr)
#endif
        {}

    // -----------------------------------------------------------------
    //  construction: heap (everything else)
    // -----------------------------------------------------------------

#if D_ENV_CPP98_HAS_NEW

    template<typename _T,
             typename std::enable_if<
                 ( !std::is_integral<_T>::value       &&
                   !std::is_floating_point<_T>::value  &&
                   !std::is_enum<_T>::value            &&
                   !std::is_pointer<_T>::value ),
                 int
             >::type = 0>
    any(const _T& _v)
            : m_category(DAnyCategory::cat_heap),
              m_type_id(any_type_id_of<_T>::value),
              m_sbo(),
              m_heap(new _T(_v)),
              m_heap_ops(internal::any_heap_ops_for<_T>())
        {}

#endif  // D_ENV_CPP98_HAS_NEW

    // -----------------------------------------------------------------
    //  copy constructor
    // -----------------------------------------------------------------

    any(const any& _other)
            : m_category(_other.m_category),
              m_type_id(_other.m_type_id),
              m_sbo(_other.m_sbo)
#if D_ENV_CPP98_HAS_NEW
            , m_heap(nullptr),
              m_heap_ops(_other.m_heap_ops)
#endif
    {
#if D_ENV_CPP98_HAS_NEW
        if ( (_other.m_category == DAnyCategory::cat_heap) &&
             (_other.m_heap != nullptr)                     &&
             (_other.m_heap_ops != nullptr) )
        {
            m_heap = m_heap_ops->clone(_other.m_heap);
        }
#endif
    }

    // -----------------------------------------------------------------
    //  move constructor
    // -----------------------------------------------------------------

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    any(any&& _other) noexcept
            : m_category(_other.m_category),
              m_type_id(_other.m_type_id),
              m_sbo(_other.m_sbo)
#if D_ENV_CPP98_HAS_NEW
            , m_heap(_other.m_heap),
              m_heap_ops(_other.m_heap_ops)
#endif
    {
        _other.m_category = DAnyCategory::cat_empty;
        _other.m_type_id  = nullptr;
#if D_ENV_CPP98_HAS_NEW
        _other.m_heap     = nullptr;
        _other.m_heap_ops = nullptr;
#endif
    }

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    // -----------------------------------------------------------------
    //  copy assignment
    // -----------------------------------------------------------------

    any& operator=(const any& _other)
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
                 (_other.m_heap != nullptr)                     &&
                 (_other.m_heap_ops != nullptr) )
            {
                m_heap = m_heap_ops->clone(_other.m_heap);
            }
#endif
        }

        return *this;
    }

    // -----------------------------------------------------------------
    //  move assignment
    // -----------------------------------------------------------------

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    any& operator=(any&& _other) noexcept
    {
        if (this != &_other)
        {
            reset();

            m_category = _other.m_category;
            m_type_id  = _other.m_type_id;
            m_sbo      = _other.m_sbo;

#if D_ENV_CPP98_HAS_NEW
            m_heap     = _other.m_heap;
            m_heap_ops = _other.m_heap_ops;
            _other.m_heap     = nullptr;
            _other.m_heap_ops = nullptr;
#endif

            _other.m_category = DAnyCategory::cat_empty;
            _other.m_type_id  = nullptr;
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

    // -----------------------------------------------------------------
    //  observers
    // -----------------------------------------------------------------

    // has_value
    D_CONSTEXPR bool has_value() const noexcept
    {
        return (m_category != DAnyCategory::cat_empty);
    }

    // operator bool
    D_CONSTEXPR explicit operator bool() const noexcept
    {
        return has_value();
    }

    // category
    D_CONSTEXPR DAnyCategory category() const noexcept
    {
        return m_category;
    }

    // type
    D_CONSTEXPR any_type_id type() const noexcept
    {
        return m_type_id;
    }

    // holds
    //   returns true if the stored value was originally of
    // type _T.
    template<typename _T>
    D_CONSTEXPR bool holds() const noexcept
    {
        return (m_type_id == any_type_id_of<_T>::value);
    }

    // is_sbo
    D_CONSTEXPR bool is_sbo() const noexcept
    {
        return ( (m_category != DAnyCategory::cat_empty) &&
                 (m_category != DAnyCategory::cat_heap) );
    }

    // -----------------------------------------------------------------
    //  typed retrieval: SBO — bool
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 std::is_same<_T, bool>::value,
                 int
             >::type = 0>
    D_CONSTEXPR _T get() const noexcept
    {
        return m_sbo.v_bool;
    }

    // -----------------------------------------------------------------
    //  typed retrieval: SBO — signed integral
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 ( std::is_integral<_T>::value &&
                   std::is_signed<_T>::value   &&
                   !std::is_same<_T, bool>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR _T get() const noexcept
    {
        return static_cast<_T>(m_sbo.v_signed);
    }

    // -----------------------------------------------------------------
    //  typed retrieval: SBO — unsigned integral
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 ( std::is_integral<_T>::value  &&
                   std::is_unsigned<_T>::value  &&
                   !std::is_same<_T, bool>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR _T get() const noexcept
    {
        return static_cast<_T>(m_sbo.v_unsigned);
    }

    // -----------------------------------------------------------------
    //  typed retrieval: SBO — floating point
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 std::is_floating_point<_T>::value,
                 int
             >::type = 0>
    D_CONSTEXPR _T get() const noexcept
    {
        return static_cast<_T>(m_sbo.v_floating);
    }

    // -----------------------------------------------------------------
    //  typed retrieval: SBO — enum
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 std::is_enum<_T>::value,
                 int
             >::type = 0>
    D_CONSTEXPR _T get() const noexcept
    {
        return static_cast<_T>(
            static_cast<
                typename std::underlying_type<_T>::type>(
                    m_sbo.v_unsigned));
    }

    // -----------------------------------------------------------------
    //  typed retrieval: SBO — non-const pointer
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 ( std::is_pointer<_T>::value             &&
                   !std::is_const<
                       typename std::remove_pointer<
                           _T>::type>::value              &&
                   !std::is_function<
                       typename std::remove_pointer<
                           _T>::type>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR _T get() const noexcept
    {
        return static_cast<_T>(m_sbo.v_pointer);
    }

    // -----------------------------------------------------------------
    //  typed retrieval: SBO — const pointer
    // -----------------------------------------------------------------

    template<typename _T,
             typename std::enable_if<
                 ( std::is_pointer<_T>::value             &&
                   std::is_const<
                       typename std::remove_pointer<
                           _T>::type>::value              &&
                   !std::is_function<
                       typename std::remove_pointer<
                           _T>::type>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR _T get() const noexcept
    {
        return static_cast<_T>(m_sbo.v_cpointer);
    }

    // -----------------------------------------------------------------
    //  typed retrieval: heap
    // -----------------------------------------------------------------

#if D_ENV_CPP98_HAS_NEW

    template<typename _T,
             typename std::enable_if<
                 ( !std::is_integral<_T>::value       &&
                   !std::is_floating_point<_T>::value  &&
                   !std::is_enum<_T>::value            &&
                   !std::is_pointer<_T>::value ),
                 int
             >::type = 0>
    const _T& get() const
    {
        return *static_cast<const _T*>(m_heap);
    }

    template<typename _T,
             typename std::enable_if<
                 ( !std::is_integral<_T>::value       &&
                   !std::is_floating_point<_T>::value  &&
                   !std::is_enum<_T>::value            &&
                   !std::is_pointer<_T>::value ),
                 int
             >::type = 0>
    _T& get()
    {
        return *static_cast<_T*>(m_heap);
    }

#endif  // D_ENV_CPP98_HAS_NEW

    // -----------------------------------------------------------------
    //  modifiers
    // -----------------------------------------------------------------

    // reset
    //   destroys the stored value and sets to empty.
    void reset() noexcept
    {
#if D_ENV_CPP98_HAS_NEW
        if ( (m_category == DAnyCategory::cat_heap) &&
             (m_heap != nullptr)                     &&
             (m_heap_ops != nullptr) )
        {
            m_heap_ops->destroy(m_heap);
            m_heap     = nullptr;
            m_heap_ops = nullptr;
        }
#endif

        m_category = DAnyCategory::cat_empty;
        m_type_id  = nullptr;

        return;
    }

    // swap
    void swap(any& _other) noexcept
    {
        any tmp(*this);
        *this = _other;
        _other = tmp;

        return;
    }

private:
    DAnyCategory          m_category;
    any_type_id           m_type_id;
    internal::any_sbo     m_sbo;

#if D_ENV_CPP98_HAS_NEW
    void*                          m_heap;
    const internal::any_heap_ops*  m_heap_ops;
#endif
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  FREE FUNCTIONS                                      ///
///////////////////////////////////////////////////////////////////////////////

// any_cast (by value — SBO and heap)
//   returns a copy of the stored value.
template<typename _T>
D_CONSTEXPR _T any_cast(const any& _a)
{
    return _a.template get<_T>();
}

// any_cast (mutable reference — heap types)
#if D_ENV_CPP98_HAS_NEW
    template<typename _T,
             typename std::enable_if<
                 ( !std::is_integral<_T>::value       &&
                   !std::is_floating_point<_T>::value  &&
                   !std::is_enum<_T>::value            &&
                   !std::is_pointer<_T>::value ),
                 int
             >::type = 0>
    _T& any_cast(any& _a)
    {
        return _a.template get<_T>();
    }
#endif  // D_ENV_CPP98_HAS_NEW

// swap
D_INLINE void swap(
        any& _a,
        any& _b
    ) noexcept
{
    _a.swap(_b);

    return;
}

NS_COMPAT
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_ANY_