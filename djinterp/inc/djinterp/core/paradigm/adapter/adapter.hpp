/******************************************************************************
* djinterp [core]                                                  adapter.hpp
*
* Adapter Pattern Module:
*   Provides a comprehensive, abstract, version-portable foundation for the
* adapter pattern. Supports multiple adaptation strategies — object (compo-
* sition), class (inheritance), interface (CRTP), function (callable trans-
* form), and view (non-owning projection) — all decoupled from any specific
* interface or container.
*
*   DESIGN:
*   The module is organized in four layers:
*     1. TRAITS — SFINAE-based detection of adaptable relationships between
*        types: compatible value types, invocable mappings, structural
*        interface overlap.
*     2. CORE — adapter bases parameterized on ownership and delegation
*        policy: object_adapter (composition), class_adapter (MI),
*        interface_adapter (CRTP).
*     3. FUNCTION ADAPTERS — callable wrappers that transform signatures,
*        argument order, return types, or arity.
*     4. VIEW ADAPTERS — non-owning projections that present one type's
*        interface through another's lens without copying data.
*
*   PORTABILITY:
*   - C++11  : object_adapter, class_adapter, interface_adapter,
*              function_adapter, adapted_ref, adaptation traits
*   - C++14  : generic lambda support in make_adapter, auto return
*   - C++17  : if constexpr dispatch, deduction guides, CTAD
*   - C++20  : concept-constrained adapters, adaptable_to concept
*
*
* path:      /inc/djinterp/paradigm/adapter/adapter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.09
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONFIGURATION & FEATURE GATES
      --------------------------------
      i.    D_ADAPTER_HAS_IF_CONSTEXPR
      ii.   D_ADAPTER_HAS_CONCEPTS
      iii.  D_ADAPTER_HAS_DEDUCTION_GUIDES

II.   OWNERSHIP POLICIES
      ---------------------
      i.    by_reference
      ii.   by_pointer
      iii.  by_value
      iv.   by_shared_ptr
      v.    by_unique_ptr

III.  ADAPTATION TRAITS
      --------------------
      i.    has_value_type (internal)
      ii.   has_size_method (internal)
      iii.  has_begin_end (internal)
      iv.   are_value_type_compatible
      v.    is_structurally_adaptable
      vi.   is_invocable_adapter
      vii.  adaptation_class (aggregate)

IV.   OBJECT ADAPTER (C++11+)
      --------------------------
      i.    object_adapter

V.    CLASS ADAPTER (C++11+)
      -------------------------
      i.    class_adapter

VI.   INTERFACE ADAPTER — CRTP (C++11+)
      -------------------------------------
      i.    interface_adapter

VII.  METHOD FORWARDING POLICIES
      -----------------------------
      i.    forward_as_is
      ii.   forward_with_transform
      iii.  forward_with_rename

VIII. FUNCTION ADAPTERS (C++11+)
      -----------------------------
      i.    function_adapter
      ii.   result_adapter
      iii.  argument_adapter
      iv.   bind_front_adapter (C++14+)
      v.    compose_adapter

IX.   VIEW ADAPTERS (C++11+)
      -------------------------
      i.    adapted_ref
      ii.   adapted_const_ref
      iii.  adapted_view

X.    CONVENIENCE FACTORIES (C++14+)
      ----------------------------------
      i.    make_object_adapter
      ii.   make_function_adapter
      iii.  make_adapted_ref
      iv.   adapt (universal factory)

XI.   CONCEPT-CONSTRAINED ADAPTERS (C++20+)
      -----------------------------------------
      i.    adaptable_to (concept)
      ii.   adapter_for (concept)
      iii.  function_adaptable (concept)
*/

#ifndef DJINTERP_PARADIGM_ADAPTER_
#define DJINTERP_PARADIGM_ADAPTER_ 1

#include <cstddef>
#include <type_traits>
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <utility>
    #include <functional>
    #include <memory>
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <tuple>
#endif


///////////////////////////////////////////////////////////////////////////////
///           I.    CONFIGURATION & FEATURE GATES                           ///
///////////////////////////////////////////////////////////////////////////////

// D_ADAPTER_HAS_IF_CONSTEXPR
//   macro: 1 if if-constexpr dispatch is available (C++17+).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_ADAPTER_HAS_IF_CONSTEXPR 1
#else
    #define D_ADAPTER_HAS_IF_CONSTEXPR 0
#endif

// D_ADAPTER_HAS_CONCEPTS
//   macro: 1 if concepts are available (C++20+).
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #define D_ADAPTER_HAS_CONCEPTS 1
#else
    #define D_ADAPTER_HAS_CONCEPTS 0
#endif

// D_ADAPTER_HAS_DEDUCTION_GUIDES
//   macro: 1 if class template argument deduction is available (C++17+).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_ADAPTER_HAS_DEDUCTION_GUIDES 1
#else
    #define D_ADAPTER_HAS_DEDUCTION_GUIDES 0
#endif


NS_DJINTERP

#if D_ENV_LANG_IS_CPP11_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///            II.   OWNERSHIP POLICIES                                     ///
///////////////////////////////////////////////////////////////////////////////

// Ownership policies control how the adapter holds a reference to the
// adaptee. Each policy provides:
//   using stored_type    — the internal storage type
//   using reference_type — what access() returns (lvalue ref)
//   static reference_type access(stored_type&)
//   static const reference_type access(const stored_type&)  [const]

// by_reference
//   policy: stores a raw reference to the adaptee. Zero overhead,
// non-owning. The adaptee must outlive the adapter.
struct by_reference
{
    template<typename _Adaptee>
    struct storage
    {
        using stored_type    = _Adaptee&;
        using reference_type = _Adaptee&;

        static D_CONSTEXPR_INLINE reference_type
        access(
            stored_type _s
        ) noexcept
        {
            return _s;
        }
    };
};

// by_pointer
//   policy: stores a raw pointer to the adaptee. Nullable,
// non-owning.
struct by_pointer
{
    template<typename _Adaptee>
    struct storage
    {
        using stored_type    = _Adaptee*;
        using reference_type = _Adaptee&;

        static D_CONSTEXPR_INLINE reference_type
        access(
            stored_type _s
        ) noexcept
        {
            return *_s;
        }
    };
};

// by_value
//   policy: stores the adaptee by value (owning copy). The adapter
// owns the adaptee and its lifetime.
struct by_value
{
    template<typename _Adaptee>
    struct storage
    {
        using stored_type    = _Adaptee;
        using reference_type = _Adaptee&;

        static D_CONSTEXPR_INLINE reference_type
        access(
            stored_type& _s
        ) noexcept
        {
            return _s;
        }

        static D_CONSTEXPR_INLINE const _Adaptee&
        access(
            const stored_type& _s
        ) noexcept
        {
            return _s;
        }
    };
};

// by_shared_ptr
//   policy: stores the adaptee via std::shared_ptr. Shared
// ownership with reference counting.
struct by_shared_ptr
{
    template<typename _Adaptee>
    struct storage
    {
        using stored_type    = std::shared_ptr<_Adaptee>;
        using reference_type = _Adaptee&;

        static reference_type
        access(
            stored_type& _s
        ) noexcept
        {
            return *_s;
        }

        static D_CONSTEXPR_INLINE const _Adaptee&
        access(
            const stored_type& _s
        ) noexcept
        {
            return *_s;
        }
    };
};

// by_unique_ptr
//   policy: stores the adaptee via std::unique_ptr. Exclusive
// ownership; the adapter is move-only.
struct by_unique_ptr
{
    template<typename _Adaptee>
    struct storage
    {
        using stored_type    = std::unique_ptr<_Adaptee>;
        using reference_type = _Adaptee&;

        static reference_type
        access(
            stored_type& _s
        ) noexcept
        {
            return *_s;
        }

        static D_CONSTEXPR_INLINE const _Adaptee&
        access(
            const stored_type& _s
        ) noexcept
        {
            return *_s;
        }
    };
};


///////////////////////////////////////////////////////////////////////////////
///           III.  ADAPTATION TRAITS                                       ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // =====================================================================
    // Structural probes
    // =====================================================================

    // has_value_type
    //   trait: detects _T::value_type.
    template<typename _T,
             typename = void>
    struct has_value_type : std::false_type
    {};

    template<typename _T>
    struct has_value_type<_T, D_VOID_T<typename _T::value_type>>
        : std::true_type
    {};

    // has_size_method
    //   trait: detects _T::size().
    template<typename _T,
             typename = void>
    struct has_size_method : std::false_type
    {};

    template<typename _T>
    struct has_size_method<_T, D_VOID_T<
        decltype(std::declval<const _T>().size())
    >> : std::true_type
    {};

    // has_begin_end
    //   trait: detects _T::begin() and _T::end().
    template<typename _T,
             typename = void>
    struct has_begin_end : std::false_type
    {};

    template<typename _T>
    struct has_begin_end<_T, D_VOID_T<
        decltype(std::declval<_T>().begin()),
        decltype(std::declval<_T>().end())
    >> : std::true_type
    {};

    // has_push_back
    //   trait: detects _T::push_back(value_type).
    template<typename _T,
             typename = void>
    struct has_push_back : std::false_type
    {};

    template<typename _T>
    struct has_push_back<_T, D_VOID_T<decltype(
        std::declval<_T>().push_back(
            std::declval<typename _T::value_type>()))
    >> : std::true_type
    {};

    // has_insert
    //   trait: detects _T::insert(value_type).
    template<typename _T,
             typename = void>
    struct has_insert : std::false_type
    {};

    template<typename _T>
    struct has_insert<_T, D_VOID_T<decltype(
        std::declval<_T>().insert(
            std::declval<typename _T::value_type>()))
    >> : std::true_type
    {};

    // has_subscript_operator
    //   trait: detects _T::operator[](size_t).
    template<typename _T,
             typename = void>
    struct has_subscript_operator : std::false_type
    {};

    template<typename _T>
    struct has_subscript_operator<_T, D_VOID_T<
        decltype(std::declval<_T>()[std::declval<std::size_t>()])
    >> : std::true_type
    {};

    // =====================================================================
    // Value type compatibility
    // =====================================================================

    // value_types_compatible
    //   trait: true if both types expose value_type and those types are
    // convertible (From::value_type -> To::value_type).
    template<typename _From,
             typename _To,
             typename = void>
    struct value_types_compatible : std::false_type
    {};

    template<typename _From,
             typename _To>
    struct value_types_compatible<_From, _To, D_VOID_T<
        typename _From::value_type,
        typename _To::value_type
    >> : std::is_convertible<typename _From::value_type,
                             typename _To::value_type>
    {};

    // =====================================================================
    // Invocable mapping
    // =====================================================================

    // is_invocable_mapping
    //   trait: true if _Fn can be called with _From& and produces a
    // result convertible to _To&.
    template<typename _Fn,
             typename _From,
             typename _To,
             typename = void>
    struct is_invocable_mapping : std::false_type
    {};

    template<typename _Fn,
             typename _From,
             typename _To>
    struct is_invocable_mapping<_Fn, _From, _To, D_VOID_T<
        decltype(std::declval<_Fn>()(std::declval<_From&>()))
    >> : std::is_convertible<
        decltype(std::declval<_Fn>()(std::declval<_From&>())),
        _To>
    {};

NS_END  // internal

// are_value_type_compatible
//   trait: public interface — true if _From's value_type is convertible
// to _To's value_type.
template<typename _From,
         typename _To>
struct are_value_type_compatible
    : internal::value_types_compatible<
        clean_t<_From>, clean_t<_To>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _From,
             typename _To>
    constexpr bool are_value_type_compatible_v =
        are_value_type_compatible<_From, _To>::value;
#endif

// is_structurally_adaptable
//   trait: true if _Adaptee has enough structural surface to be
// adapted into _Target's interface. Requires at minimum that both
// types share an iterable interface or both are sized.
template<typename _Adaptee,
         typename _Target>
struct is_structurally_adaptable
{
    static constexpr bool value =
        ( (internal::has_begin_end<_Adaptee>::value &&
           internal::has_begin_end<_Target>::value)  ||
          (internal::has_size_method<_Adaptee>::value &&
           internal::has_size_method<_Target>::value) ||
          (internal::has_subscript_operator<_Adaptee>::value &&
           internal::has_subscript_operator<_Target>::value) );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Adaptee,
             typename _Target>
    constexpr bool is_structurally_adaptable_v =
        is_structurally_adaptable<_Adaptee, _Target>::value;
#endif

// is_invocable_adapter
//   trait: true if _Fn maps _From to something convertible to _To.
template<typename _Fn,
         typename _From,
         typename _To>
struct is_invocable_adapter
    : internal::is_invocable_mapping<_Fn, _From, _To>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Fn,
             typename _From,
             typename _To>
    constexpr bool is_invocable_adapter_v =
        is_invocable_adapter<_Fn, _From, _To>::value;
#endif

// adaptation_class
//   struct: aggregate classification of an adaptee→target relationship.
template<typename _Adaptee,
         typename _Target>
struct adaptation_class
{
    static constexpr bool compatible_values =
        are_value_type_compatible<_Adaptee, _Target>::value;

    static constexpr bool structurally_adaptable =
        is_structurally_adaptable<_Adaptee, _Target>::value;

    static constexpr bool adaptee_iterable =
        internal::has_begin_end<_Adaptee>::value;

    static constexpr bool target_iterable =
        internal::has_begin_end<_Target>::value;

    static constexpr bool adaptee_sized =
        internal::has_size_method<_Adaptee>::value;

    static constexpr bool target_sized =
        internal::has_size_method<_Target>::value;

    static constexpr bool adaptee_indexable =
        internal::has_subscript_operator<_Adaptee>::value;

    static constexpr bool target_indexable =
        internal::has_subscript_operator<_Target>::value;

    static constexpr bool adaptee_has_push_back =
        internal::has_push_back<_Adaptee>::value;

    static constexpr bool adaptee_has_insert =
        internal::has_insert<_Adaptee>::value;

    // true if direct (no transform) adaptation is plausible
    static constexpr bool is_directly_adaptable =
        ( compatible_values &&
          structurally_adaptable );
};


///////////////////////////////////////////////////////////////////////////////
///             IV.   OBJECT ADAPTER (C++11+)                              ///
///////////////////////////////////////////////////////////////////////////////

// object_adapter
//   class: composition-based adapter. Holds an adaptee via the
// specified _OwnershipPolicy and delegates target interface calls
// to the adaptee through an _AdaptationPolicy.
//
// _AdaptationPolicy must provide static methods that map target
// operations onto adaptee operations:
//   static auto size(adaptee&) -> size_type;
//   static auto get(adaptee&, index) -> reference;
//   etc.
//
// Usage:
//   struct my_policy
//   {
//       static std::size_t size(const legacy_container& c)
//           { return c.num_elements(); }
//       static int& get(legacy_container& c, std::size_t i)
//           { return c.element_at(i); }
//   };
//
//   object_adapter<legacy_container, my_policy> adapted(legacy);
//   adapted.size();      // calls legacy.num_elements()
//   adapted.get(0);      // calls legacy.element_at(0)
template<typename _Adaptee,
         typename _AdaptationPolicy,
         typename _OwnershipPolicy = by_reference>
class object_adapter
{
private:
    using storage_policy = typename _OwnershipPolicy::template
                               storage<_Adaptee>;
    using stored_type    = typename storage_policy::stored_type;

public:
    using adaptee_type      = _Adaptee;
    using adaptation_policy = _AdaptationPolicy;
    using ownership_policy  = _OwnershipPolicy;

    // constructor
    explicit object_adapter(
            stored_type _adaptee
        )
            : m_adaptee(std::forward<stored_type>(_adaptee))
        {}

    // adaptee
    //   function: direct access to the underlying adaptee.
    D_CONSTEXPR_INLINE _Adaptee&
    adaptee() noexcept
    {
        return storage_policy::access(m_adaptee);
    }

    D_CONSTEXPR_INLINE const _Adaptee&
    adaptee() const noexcept
    {
        return storage_policy::access(m_adaptee);
    }

    // size — delegated through policy
    template<typename _P = _AdaptationPolicy>
    auto size() const
        -> decltype(_P::size(std::declval<const _Adaptee&>()))
    {
        return _AdaptationPolicy::size(adaptee());
    }

    // get — delegated through policy
    template<typename _P = _AdaptationPolicy,
             typename _Index>
    auto get(
        _Index _i
    )
        -> decltype(_P::get(std::declval<_Adaptee&>(), _i))
    {
        return _AdaptationPolicy::get(adaptee(), _i);
    }

    template<typename _P = _AdaptationPolicy,
             typename _Index>
    auto get(
        _Index _i
    ) const
        -> decltype(_P::get(std::declval<const _Adaptee&>(), _i))
    {
        return _AdaptationPolicy::get(adaptee(), _i);
    }

    // forward — generic method forwarding through policy
    template<typename _P = _AdaptationPolicy,
             typename... _Args>
    auto forward(
        _Args&&... _args
    )
        -> decltype(_P::forward(
            std::declval<_Adaptee&>(),
            std::forward<_Args>(_args)...))
    {
        return _AdaptationPolicy::forward(
            adaptee(),
            std::forward<_Args>(_args)...);
    }

private:
    stored_type m_adaptee;
};


///////////////////////////////////////////////////////////////////////////////
///              V.    CLASS ADAPTER (C++11+)                               ///
///////////////////////////////////////////////////////////////////////////////

// class_adapter
//   class: multiple-inheritance-based adapter. Inherits publicly from
// _Target (to satisfy the target interface) and privately from _Adaptee
// (to gain access to the adaptee's implementation). _Derived must
// override target virtual methods and delegate to _Adaptee members.
//
// Usage:
//   class my_adapter
//       : public class_adapter<target_interface, legacy_impl, my_adapter>
//   {
//   public:
//       void target_method() override
//       {
//           this->adaptee_ref().legacy_method();
//       }
//   };
template<typename _Target,
         typename _Adaptee,
         typename _Derived>
class class_adapter : public  _Target,
                      private _Adaptee
{
protected:
    // adaptee_ref
    //   function: gives derived classes access to the private base.
    _Adaptee&
    adaptee_ref() noexcept
    {
        return static_cast<_Adaptee&>(*this);
    }

    const _Adaptee&
    adaptee_ref() const noexcept
    {
        return static_cast<const _Adaptee&>(*this);
    }

public:
    using target_type  = _Target;
    using adaptee_type = _Adaptee;

    class_adapter() = default;

    explicit class_adapter(
            const _Adaptee& _a
        )
            : _Target(),
              _Adaptee(_a)
        {}

    explicit class_adapter(
            _Adaptee&& _a
        )
            : _Target(),
              _Adaptee(std::move(_a))
        {}
};


///////////////////////////////////////////////////////////////////////////////
///         VI.   INTERFACE ADAPTER — CRTP (C++11+)                        ///
///////////////////////////////////////////////////////////////////////////////

// interface_adapter
//   class: CRTP base for zero-overhead interface adaptation. _Derived
// implements the target interface by delegating to an internally held
// adaptee. Unlike object_adapter, there is no policy indirection — the
// mapping is hard-coded in _Derived, yielding fully inlinable dispatch.
//
// Usage:
//   class stack_as_deque
//       : public interface_adapter<stack_as_deque, std::stack<int>>
//   {
//   public:
//       using interface_adapter::interface_adapter;
//
//       void push_back(int v)  { adaptee().push(v); }
//       void pop_back()        { adaptee().pop(); }
//       int& back()            { return adaptee().top(); }
//       std::size_t size()     { return adaptee().size(); }
//   };
template<typename _Derived,
         typename _Adaptee,
         typename _OwnershipPolicy = by_value>
class interface_adapter
{
private:
    using storage_policy = typename _OwnershipPolicy::template
                               storage<_Adaptee>;
    using stored_type    = typename storage_policy::stored_type;

public:
    using adaptee_type     = _Adaptee;
    using ownership_policy = _OwnershipPolicy;

    interface_adapter() = default;

    explicit interface_adapter(
            stored_type _adaptee
        )
            : m_adaptee(std::forward<stored_type>(_adaptee))
        {}

protected:
    D_CONSTEXPR_INLINE _Adaptee&
    adaptee() noexcept
    {
        return storage_policy::access(m_adaptee);
    }

    D_CONSTEXPR_INLINE const _Adaptee&
    adaptee() const noexcept
    {
        return storage_policy::access(m_adaptee);
    }

private:
    stored_type m_adaptee;
};


///////////////////////////////////////////////////////////////////////////////
///        VII.  METHOD FORWARDING POLICIES                                 ///
///////////////////////////////////////////////////////////////////////////////

// These are mix-in policies for object_adapter. Each defines how
// individual method calls are mapped from the target to the adaptee.

// forward_as_is
//   policy: forwards calls to identically-named methods on the
// adaptee. The simplest mapping — target.foo(args) → adaptee.foo(args).
struct forward_as_is
{
    template<typename _Adaptee,
             typename _Method,
             typename... _Args>
    static auto
    invoke(
        _Adaptee& _a,
        _Method   _m,
        _Args&&... _args
    )
        -> decltype((_a.*_m)(std::forward<_Args>(_args)...))
    {
        return (_a.*_m)(std::forward<_Args>(_args)...);
    }
};

// forward_with_transform
//   policy: applies a transformation function to each argument before
// forwarding. Useful for unit conversion, type coercion, etc.
template<typename _Transform>
struct forward_with_transform
{
    template<typename _Adaptee,
             typename _Method,
             typename... _Args>
    static auto
    invoke(
        _Adaptee&   _a,
        _Method     _m,
        _Transform& _xform,
        _Args&&...  _args
    )
        -> decltype((_a.*_m)(_xform(std::forward<_Args>(_args))...))
    {
        return (_a.*_m)(_xform(std::forward<_Args>(_args))...);
    }
};


///////////////////////////////////////////////////////////////////////////////
///          VIII. FUNCTION ADAPTERS (C++11+)                               ///
///////////////////////////////////////////////////////////////////////////////

// function_adapter
//   class: wraps a callable and adapts its signature. Stores an inner
// callable and an optional pre-processing transform applied to
// arguments before forwarding.
//
// Usage:
//   auto adapted = function_adapter<decltype(fn), decltype(xform)>(fn, xform);
//   adapted(args...);  // calls xform on each arg, then fn
template<typename _Fn,
         typename _Transform = void>
class function_adapter
{
public:
    using function_type  = _Fn;
    using transform_type = _Transform;

    function_adapter(
            _Fn        _fn,
            _Transform _xform
        )
            : m_fn(std::move(_fn)),
              m_xform(std::move(_xform))
        {}

    template<typename... _Args>
    auto operator()(
        _Args&&... _args
    )
        -> decltype(std::declval<_Fn>()(
            std::declval<_Transform>()(std::forward<_Args>(_args))...))
    {
        return m_fn(m_xform(std::forward<_Args>(_args))...);
    }

    template<typename... _Args>
    auto operator()(
        _Args&&... _args
    ) const
        -> decltype(std::declval<const _Fn>()(
            std::declval<const _Transform>()(std::forward<_Args>(_args))...))
    {
        return m_fn(m_xform(std::forward<_Args>(_args))...);
    }

private:
    _Fn        m_fn;
    _Transform m_xform;
};

// function_adapter (no transform specialization)
//   class: passthrough adapter that simply wraps a callable with no
// argument transformation. Useful as a uniform wrapper type.
template<typename _Fn>
class function_adapter<_Fn, void>
{
public:
    using function_type = _Fn;

    explicit function_adapter(
            _Fn _fn
        )
            : m_fn(std::move(_fn))
        {}

    template<typename... _Args>
    auto operator()(
        _Args&&... _args
    )
        -> decltype(std::declval<_Fn>()(std::forward<_Args>(_args)...))
    {
        return m_fn(std::forward<_Args>(_args)...);
    }

    template<typename... _Args>
    auto operator()(
        _Args&&... _args
    ) const
        -> decltype(std::declval<const _Fn>()(std::forward<_Args>(_args)...))
    {
        return m_fn(std::forward<_Args>(_args)...);
    }

private:
    _Fn m_fn;
};

// result_adapter
//   class: wraps a callable and transforms its return value through
// a post-processing function.
//
// Usage:
//   result_adapter ra(strlen, [](std::size_t n){ return (int)n; });
//   int len = ra("hello");
template<typename _Fn,
         typename _ResultTransform>
class result_adapter
{
public:
    using function_type  = _Fn;
    using transform_type = _ResultTransform;

    result_adapter(
            _Fn              _fn,
            _ResultTransform _xform
        )
            : m_fn(std::move(_fn)),
              m_xform(std::move(_xform))
        {}

    template<typename... _Args>
    auto operator()(
        _Args&&... _args
    )
        -> decltype(std::declval<_ResultTransform>()(
            std::declval<_Fn>()(std::forward<_Args>(_args)...)))
    {
        return m_xform(m_fn(std::forward<_Args>(_args)...));
    }

    template<typename... _Args>
    auto operator()(
        _Args&&... _args
    ) const
        -> decltype(std::declval<const _ResultTransform>()(
            std::declval<const _Fn>()(std::forward<_Args>(_args)...)))
    {
        return m_xform(m_fn(std::forward<_Args>(_args)...));
    }

private:
    _Fn              m_fn;
    _ResultTransform m_xform;
};

// argument_adapter
//   class: wraps a callable and individually transforms each argument
// position through a tuple of per-position transforms.
//
// Usage:
//   argument_adapter aa(fn, std::make_tuple(to_int, to_float));
//   aa("42", "3.14");  // calls fn(to_int("42"), to_float("3.14"))
template<typename _Fn,
         typename _ArgTransformTuple>
class argument_adapter
{
public:
    argument_adapter(
            _Fn                _fn,
            _ArgTransformTuple _xforms
        )
            : m_fn(std::move(_fn)),
              m_xforms(std::move(_xforms))
        {}

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    template<typename... _Args>
    auto operator()(
        _Args&&... _args
    )
    {
        return invoke_impl(std::index_sequence_for<_Args...>{},
                           std::forward<_Args>(_args)...);
    }

private:
    template<std::size_t... _Is,
             typename...    _Args>
    auto invoke_impl(
        std::index_sequence<_Is...>,
        _Args&&... _args
    )
    {
        return m_fn(
            std::get<_Is>(m_xforms)(std::forward<_Args>(_args))...);
    }

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

private:
    _Fn                m_fn;
    _ArgTransformTuple m_xforms;
};

// compose_adapter
//   class: function composition adapter. Chains two callables such
// that operator()(args...) evaluates _Outer(_Inner(args...)).
template<typename _Outer,
         typename _Inner>
class compose_adapter
{
public:
    compose_adapter(
            _Outer _outer,
            _Inner _inner
        )
            : m_outer(std::move(_outer)),
              m_inner(std::move(_inner))
        {}

    template<typename... _Args>
    auto operator()(
        _Args&&... _args
    )
        -> decltype(std::declval<_Outer>()(
            std::declval<_Inner>()(std::forward<_Args>(_args)...)))
    {
        return m_outer(m_inner(std::forward<_Args>(_args)...));
    }

    template<typename... _Args>
    auto operator()(
        _Args&&... _args
    ) const
        -> decltype(std::declval<const _Outer>()(
            std::declval<const _Inner>()(std::forward<_Args>(_args)...)))
    {
        return m_outer(m_inner(std::forward<_Args>(_args)...));
    }

private:
    _Outer m_outer;
    _Inner m_inner;
};


///////////////////////////////////////////////////////////////////////////////
///            IX.   VIEW ADAPTERS (C++11+)                                 ///
///////////////////////////////////////////////////////////////////////////////

// adapted_ref
//   class: non-owning mutable reference adapter. Holds a reference to
// an adaptee and exposes the target interface by delegating through
// an _AdaptationPolicy — identical to object_adapter<by_reference>
// but with a lighter, view-semantic API.
template<typename _Adaptee,
         typename _AdaptationPolicy>
class adapted_ref
{
public:
    using adaptee_type      = _Adaptee;
    using adaptation_policy = _AdaptationPolicy;

    explicit adapted_ref(
            _Adaptee& _adaptee
        ) noexcept
            : m_ref(_adaptee)
        {}

    _Adaptee&
    adaptee() noexcept
    {
        return m_ref;
    }

    const _Adaptee&
    adaptee() const noexcept
    {
        return m_ref;
    }

    template<typename _P = _AdaptationPolicy>
    auto size() const
        -> decltype(_P::size(std::declval<const _Adaptee&>()))
    {
        return _AdaptationPolicy::size(m_ref);
    }

    template<typename _P = _AdaptationPolicy,
             typename _Index>
    auto get(
        _Index _i
    )
        -> decltype(_P::get(std::declval<_Adaptee&>(), _i))
    {
        return _AdaptationPolicy::get(m_ref, _i);
    }

    template<typename _P = _AdaptationPolicy,
             typename _Index>
    auto get(
        _Index _i
    ) const
        -> decltype(_P::get(std::declval<const _Adaptee&>(), _i))
    {
        return _AdaptationPolicy::get(m_ref, _i);
    }

private:
    _Adaptee& m_ref;
};

// adapted_const_ref
//   class: non-owning const reference adapter. Read-only view of an
// adaptee through a target interface policy.
template<typename _Adaptee,
         typename _AdaptationPolicy>
class adapted_const_ref
{
public:
    using adaptee_type      = _Adaptee;
    using adaptation_policy = _AdaptationPolicy;

    explicit adapted_const_ref(
            const _Adaptee& _adaptee
        ) noexcept
            : m_ref(_adaptee)
        {}

    const _Adaptee&
    adaptee() const noexcept
    {
        return m_ref;
    }

    template<typename _P = _AdaptationPolicy>
    auto size() const
        -> decltype(_P::size(std::declval<const _Adaptee&>()))
    {
        return _AdaptationPolicy::size(m_ref);
    }

    template<typename _P = _AdaptationPolicy,
             typename _Index>
    auto get(
        _Index _i
    ) const
        -> decltype(_P::get(std::declval<const _Adaptee&>(), _i))
    {
        return _AdaptationPolicy::get(m_ref, _i);
    }

private:
    const _Adaptee& m_ref;
};

// adapted_view
//   class: non-owning adapter that presents an adaptee's iteration
// interface through a projection function. Each dereferenced element
// is transformed by _Projection before being returned.
//
// Usage:
//   std::vector<std::pair<int,std::string>> data = ...;
//   auto keys = adapted_view(data, [](auto& p){ return p.first; });
//   for (auto k : keys) { ... }
template<typename _Adaptee,
         typename _Projection>
class adapted_view
{
public:
    using adaptee_type   = _Adaptee;
    using projection_type = _Projection;

    adapted_view(
            _Adaptee&   _adaptee,
            _Projection _proj
        )
            : m_ref(_adaptee),
              m_proj(std::move(_proj))
        {}

    // projected_iterator
    //   class: iterator that applies the projection on dereference.
    class iterator
    {
    private:
        using inner_iterator = decltype(std::begin(
            std::declval<_Adaptee&>()));

    public:
        explicit iterator(
                inner_iterator _it,
                _Projection*   _proj
            )
                : m_it(_it),
                  m_proj(_proj)
            {}

        auto operator*()
            -> decltype(std::declval<_Projection>()(*std::declval<inner_iterator>()))
        {
            return (*m_proj)(*m_it);
        }

        iterator& operator++()
        {
            ++m_it;

            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp = *this;
            ++m_it;

            return tmp;
        }

        friend bool operator==(
            const iterator& _lhs,
            const iterator& _rhs
        )
        {
            return (_lhs.m_it == _rhs.m_it);
        }

        friend bool operator!=(
            const iterator& _lhs,
            const iterator& _rhs
        )
        {
            return !(_lhs == _rhs);
        }

    private:
        inner_iterator m_it;
        _Projection*   m_proj;
    };

    iterator begin()
    {
        return iterator(std::begin(m_ref), &m_proj);
    }

    iterator end()
    {
        return iterator(std::end(m_ref), &m_proj);
    }

    auto size() const
        -> decltype(std::declval<const _Adaptee&>().size())
    {
        return m_ref.size();
    }

private:
    _Adaptee&   m_ref;
    _Projection m_proj;
};


///////////////////////////////////////////////////////////////////////////////
///          X.    CONVENIENCE FACTORIES (C++14+)                           ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// make_object_adapter
//   function: deduces template arguments for object_adapter.
template<typename _AdaptationPolicy,
         typename _Adaptee>
inline auto
make_object_adapter(
    _Adaptee& _adaptee
)
{
    return object_adapter<_Adaptee,
                          _AdaptationPolicy,
                          by_reference>(_adaptee);
}

// make_owning_adapter
//   function: creates a by-value owning object_adapter.
template<typename _AdaptationPolicy,
         typename _Adaptee>
inline auto
make_owning_adapter(
    _Adaptee _adaptee
)
{
    return object_adapter<_Adaptee,
                          _AdaptationPolicy,
                          by_value>(std::move(_adaptee));
}

// make_function_adapter
//   function: deduces template arguments for function_adapter.
template<typename _Fn,
         typename _Transform>
inline auto
make_function_adapter(
    _Fn&&        _fn,
    _Transform&& _xform
)
{
    return function_adapter<std::decay_t<_Fn>,
                            std::decay_t<_Transform>>(
        std::forward<_Fn>(_fn),
        std::forward<_Transform>(_xform));
}

// make_function_adapter (no transform)
template<typename _Fn>
inline auto
make_function_adapter(
    _Fn&& _fn
)
{
    return function_adapter<std::decay_t<_Fn>>(
        std::forward<_Fn>(_fn));
}

// make_result_adapter
//   function: deduces template arguments for result_adapter.
template<typename _Fn,
         typename _ResultTransform>
inline auto
make_result_adapter(
    _Fn&&              _fn,
    _ResultTransform&& _xform
)
{
    return result_adapter<std::decay_t<_Fn>,
                          std::decay_t<_ResultTransform>>(
        std::forward<_Fn>(_fn),
        std::forward<_ResultTransform>(_xform));
}

// make_compose
//   function: creates a compose_adapter from two callables.
template<typename _Outer,
         typename _Inner>
inline auto
make_compose(
    _Outer&& _outer,
    _Inner&& _inner
)
{
    return compose_adapter<std::decay_t<_Outer>,
                           std::decay_t<_Inner>>(
        std::forward<_Outer>(_outer),
        std::forward<_Inner>(_inner));
}

// make_adapted_ref
//   function: deduces template arguments for adapted_ref.
template<typename _AdaptationPolicy,
         typename _Adaptee>
inline auto
make_adapted_ref(
    _Adaptee& _adaptee
)
{
    return adapted_ref<_Adaptee, _AdaptationPolicy>(_adaptee);
}

// make_adapted_view
//   function: deduces template arguments for adapted_view.
template<typename _Adaptee,
         typename _Projection>
inline auto
make_adapted_view(
    _Adaptee&    _adaptee,
    _Projection&& _proj
)
{
    return adapted_view<_Adaptee, std::decay_t<_Projection>>(
        _adaptee,
        std::forward<_Projection>(_proj));
}

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///       XI.   CONCEPT-CONSTRAINED ADAPTERS (C++20+)                      ///
///////////////////////////////////////////////////////////////////////////////

#if D_ADAPTER_HAS_CONCEPTS

// adaptable_to
//   concept: constrains types that share enough structural surface for
// direct adaptation (compatible value types + structural overlap).
template<typename _Adaptee,
         typename _Target>
concept adaptable_to =
    ( are_value_type_compatible<_Adaptee, _Target>::value &&
      is_structurally_adaptable<_Adaptee, _Target>::value );

// adapter_for
//   concept: constrains an adapter type that exposes both adaptee()
// and at least one target-interface method (size or get).
template<typename _Adapter>
concept adapter_for = requires(_Adapter& _a, const _Adapter& _ca)
{
    _a.adaptee();
    { _ca.size() } -> std::convertible_to<std::size_t>;
};

// function_adaptable
//   concept: constrains callables that can be wrapped by
// function_adapter (must be invocable).
template<typename _Fn,
         typename... _Args>
concept function_adaptable = std::invocable<_Fn, _Args...>;

// constrained_adapt
//   function: concept-constrained factory for object adapters.
template<typename _AdaptationPolicy,
         typename _Adaptee,
         typename _Target>
    requires adaptable_to<_Adaptee, _Target>
inline auto
constrained_adapt(
    _Adaptee& _adaptee
)
{
    return object_adapter<_Adaptee,
                          _AdaptationPolicy,
                          by_reference>(_adaptee);
}

#endif  // D_ADAPTER_HAS_CONCEPTS


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // djinterp


#endif  // DJINTERP_PARADIGM_ADAPTER_