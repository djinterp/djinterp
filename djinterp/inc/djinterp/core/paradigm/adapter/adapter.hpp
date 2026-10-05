/*******************************************************************************
* djinterp [core]                                                    adapter.hpp
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
* path:      /inc/djinterp/core/paradigm/adapter/adapter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.09
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONFIGURATION & FEATURE GATES
      -----------------------------
      i.    D_ADAPTER_HAS_IF_CONSTEXPR
      ii.   D_ADAPTER_HAS_CONCEPTS
      iii.  D_ADAPTER_HAS_DEDUCTION_GUIDES

II.   OWNERSHIP POLICIES
      ------------------
      i.    by_reference
      ii.   by_pointer
      iii.  by_value
      iv.   by_shared_ptr
      v.    by_unique_ptr

III.  ADAPTATION TRAITS
      -----------------
      i.    has_value_type (internal)
      ii.   has_size_method (internal)
      iii.  has_begin_end (internal)
      iv.   are_value_type_compatible
      v.    is_structurally_adaptable
      vi.   is_invocable_adapter
      vii.  adaptation_class (aggregate)

IV.   OBJECT ADAPTER (C++11+)
      -----------------------
      i.    object_adapter

V.    CLASS ADAPTER (C++11+)
      ----------------------
      i.    class_adapter

VI.   INTERFACE ADAPTER — CRTP (C++11+)
      ---------------------------------
      i.    interface_adapter

VII.  METHOD FORWARDING POLICIES
      --------------------------
      i.    forward_as_is
      ii.   forward_with_transform
      iii.  forward_with_rename

VIII. FUNCTION ADAPTERS (C++11+)
      --------------------------
      i.    function_adapter
      ii.   result_adapter
      iii.  argument_adapter
      iv.   bind_front_adapter (C++14+)
      v.    compose_adapter

IX.   VIEW ADAPTERS (C++11+)
      ----------------------
      i.    adapted_ref
      ii.   adapted_const_ref
      iii.  adapted_view

X.    CONVENIENCE FACTORIES (C++14+)
      ------------------------------
      i.    make_object_adapter
      ii.   make_function_adapter
      iii.  make_adapted_ref
      iv.   adapt (universal factory)

XI.   CONCEPT-CONSTRAINED ADAPTERS (C++20+)
      -------------------------------------
      i.    adaptable_to (concept)
      ii.   adapter_for (concept)
      iii.  function_adaptable (concept)
*/

#ifndef DJINTERP_PARADIGM_ADAPTER_ADAPTER_HPP
#define DJINTERP_PARADIGM_ADAPTER_ADAPTER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_utility.hpp"  // clean_t
#include "../../meta/type_traits.hpp"
// re_std
#include "../../../../re_std/utility/make_integer_sequence.hpp"  // re_std::index_sequence,
                                                                 // make_index_sequence

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // std
    #include <functional>
    #include <memory>
    #include <utility>
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // std
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
    template<typename Adaptee>
    struct storage
    {
        using stored_type    = Adaptee&;
        using reference_type = Adaptee&;

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
    template<typename Adaptee>
    struct storage
    {
        using stored_type    = Adaptee*;
        using reference_type = Adaptee&;

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
    template<typename Adaptee>
    struct storage
    {
        using stored_type    = Adaptee;
        using reference_type = Adaptee&;

        static D_CONSTEXPR_INLINE reference_type
        access(
            stored_type& _s
        ) noexcept
        {
            return _s;
        }

        static D_CONSTEXPR_INLINE const Adaptee&
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
    template<typename Adaptee>
    struct storage
    {
        using stored_type    = std::shared_ptr<Adaptee>;
        using reference_type = Adaptee&;

        static reference_type
        access(
            stored_type& _s
        ) noexcept
        {
            return *_s;
        }

        static D_CONSTEXPR_INLINE const Adaptee&
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
    template<typename Adaptee>
    struct storage
    {
        using stored_type    = std::unique_ptr<Adaptee>;
        using reference_type = Adaptee&;

        static reference_type
        access(
            stored_type& _s
        ) noexcept
        {
            return *_s;
        }

        static D_CONSTEXPR_INLINE const Adaptee&
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
    //   trait: detects T::value_type.
    template<typename T,
             typename = void>
    struct has_value_type : std::false_type
    {};

    template<typename T>
    struct has_value_type<T, D_VOID_T<typename T::value_type>>
        : std::true_type
    {};

    // has_size_method
    //   trait: detects T::size().
    template<typename T,
             typename = void>
    struct has_size_method : std::false_type
    {};

    template<typename T>
    struct has_size_method<T, D_VOID_T<
        decltype(std::declval<const T>().size())
    >> : std::true_type
    {};

    // has_begin_end
    //   trait: detects T::begin() and T::end().
    template<typename T,
             typename = void>
    struct has_begin_end : std::false_type
    {};

    template<typename T>
    struct has_begin_end<T, D_VOID_T<
        decltype(std::declval<T>().begin()),
        decltype(std::declval<T>().end())
    >> : std::true_type
    {};

    // has_push_back
    //   trait: detects T::push_back(value_type).
    template<typename T,
             typename = void>
    struct has_push_back : std::false_type
    {};

    template<typename T>
    struct has_push_back<T, D_VOID_T<decltype(
        std::declval<T>().push_back(
            std::declval<typename T::value_type>()))
    >> : std::true_type
    {};

    // has_insert
    //   trait: detects T::insert(value_type).
    template<typename T,
             typename = void>
    struct has_insert : std::false_type
    {};

    template<typename T>
    struct has_insert<T, D_VOID_T<decltype(
        std::declval<T>().insert(
            std::declval<typename T::value_type>()))
    >> : std::true_type
    {};

    // has_subscript_operator
    //   trait: detects T::operator[](size_t).
    template<typename T,
             typename = void>
    struct has_subscript_operator : std::false_type
    {};

    template<typename T>
    struct has_subscript_operator<T, D_VOID_T<
        decltype(std::declval<T>()[std::declval<std::size_t>()])
    >> : std::true_type
    {};

    // =====================================================================
    // Value type compatibility
    // =====================================================================

    // value_types_compatible
    //   trait: true if both types expose value_type and those types are
    // convertible (From::value_type -> To::value_type).
    template<typename From,
             typename To,
             typename = void>
    struct value_types_compatible : std::false_type
    {};

    template<typename From,
             typename To>
    struct value_types_compatible<From, To, D_VOID_T<
        typename From::value_type,
        typename To::value_type
    >> : std::is_convertible<typename From::value_type,
                             typename To::value_type>
    {};

    // =====================================================================
    // Invocable mapping
    // =====================================================================

    // is_invocable_mapping
    //   trait: true if Fn can be called with From& and produces a
    // result convertible to To&.
    template<typename Fn,
             typename From,
             typename To,
             typename = void>
    struct is_invocable_mapping : std::false_type
    {};

    template<typename Fn,
             typename From,
             typename To>
    struct is_invocable_mapping<Fn, From, To, D_VOID_T<
        decltype(std::declval<Fn>()(std::declval<From&>()))
    >> : std::is_convertible<
        decltype(std::declval<Fn>()(std::declval<From&>())),
        To>
    {};

NS_END  // internal

// are_value_type_compatible
//   trait: public interface — true if From's value_type is convertible
// to To's value_type.
template<typename From,
         typename To>
struct are_value_type_compatible
    : internal::value_types_compatible<
        clean_t<From>, clean_t<To>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename From,
             typename To>
    constexpr bool are_value_type_compatible_v =
        are_value_type_compatible<From, To>::value;
#endif

// is_structurally_adaptable
//   trait: true if Adaptee has enough structural surface to be
// adapted into Target's interface. Requires at minimum that both
// types share an iterable interface or both are sized.
template<typename Adaptee,
         typename Target>
struct is_structurally_adaptable
{
    static constexpr bool value =
        ( (internal::has_begin_end<Adaptee>::value &&
           internal::has_begin_end<Target>::value)  ||
          (internal::has_size_method<Adaptee>::value &&
           internal::has_size_method<Target>::value) ||
          (internal::has_subscript_operator<Adaptee>::value &&
           internal::has_subscript_operator<Target>::value) );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Adaptee,
             typename Target>
    constexpr bool is_structurally_adaptable_v =
        is_structurally_adaptable<Adaptee, Target>::value;
#endif

// is_invocable_adapter
//   trait: true if Fn maps From to something convertible to To.
template<typename Fn,
         typename From,
         typename To>
struct is_invocable_adapter
    : internal::is_invocable_mapping<Fn, From, To>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Fn,
             typename From,
             typename To>
    constexpr bool is_invocable_adapter_v =
        is_invocable_adapter<Fn, From, To>::value;
#endif

// adaptation_class
//   struct: aggregate classification of an adaptee→target relationship.
template<typename Adaptee,
         typename Target>
struct adaptation_class
{
    static constexpr bool compatible_values =
        are_value_type_compatible<Adaptee, Target>::value;

    static constexpr bool structurally_adaptable =
        is_structurally_adaptable<Adaptee, Target>::value;

    static constexpr bool adaptee_iterable =
        internal::has_begin_end<Adaptee>::value;

    static constexpr bool target_iterable =
        internal::has_begin_end<Target>::value;

    static constexpr bool adaptee_sized =
        internal::has_size_method<Adaptee>::value;

    static constexpr bool target_sized =
        internal::has_size_method<Target>::value;

    static constexpr bool adaptee_indexable =
        internal::has_subscript_operator<Adaptee>::value;

    static constexpr bool target_indexable =
        internal::has_subscript_operator<Target>::value;

    static constexpr bool adaptee_has_push_back =
        internal::has_push_back<Adaptee>::value;

    static constexpr bool adaptee_has_insert =
        internal::has_insert<Adaptee>::value;

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
// specified OwnershipPolicy and delegates target interface calls
// to the adaptee through an AdaptationPolicy.
//
// AdaptationPolicy must provide static methods that map target
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
template<typename Adaptee,
         typename AdaptationPolicy,
         typename OwnershipPolicy = by_reference>
class object_adapter
{
private:
    using storage_policy = typename OwnershipPolicy::template
                               storage<Adaptee>;
    using stored_type    = typename storage_policy::stored_type;

public:
    using adaptee_type      = Adaptee;
    using adaptation_policy = AdaptationPolicy;
    using ownership_policy  = OwnershipPolicy;

    // constructor
    explicit object_adapter(
            stored_type _adaptee
        )
            : m_adaptee(std::forward<stored_type>(_adaptee))
        {}

    // adaptee
    //   function: direct access to the underlying adaptee.
    D_CONSTEXPR_CPP14 Adaptee&
    adaptee() noexcept
    {
        return storage_policy::access(m_adaptee);
    }

    D_CONSTEXPR_INLINE const Adaptee&
    adaptee() const noexcept
    {
        return storage_policy::access(m_adaptee);
    }

    // size — delegated through policy
    template<typename P = AdaptationPolicy>
    auto size() const
        -> decltype(P::size(std::declval<const Adaptee&>()))
    {
        return AdaptationPolicy::size(adaptee());
    }

    // get — delegated through policy
    template<typename P = AdaptationPolicy,
             typename Index>
    auto get(
        Index _i
    )
        -> decltype(P::get(std::declval<Adaptee&>(), _i))
    {
        return AdaptationPolicy::get(adaptee(), _i);
    }

    template<typename P = AdaptationPolicy,
             typename Index>
    auto get(
        Index _i
    ) const
        -> decltype(P::get(std::declval<const Adaptee&>(), _i))
    {
        return AdaptationPolicy::get(adaptee(), _i);
    }

    // forward — generic method forwarding through policy
    template<typename P = AdaptationPolicy,
             typename... Args>
    auto forward(
        Args&&... _args
    )
        -> decltype(P::forward(
            std::declval<Adaptee&>(),
            std::forward<Args>(_args)...))
    {
        return AdaptationPolicy::forward(
            adaptee(),
            std::forward<Args>(_args)...);
    }

private:
    stored_type m_adaptee;
};


///////////////////////////////////////////////////////////////////////////////
///              V.    CLASS ADAPTER (C++11+)                               ///
///////////////////////////////////////////////////////////////////////////////

// class_adapter
//   class: multiple-inheritance-based adapter. Inherits publicly from
// Target (to satisfy the target interface) and privately from Adaptee
// (to gain access to the adaptee's implementation). Derived must
// override target virtual methods and delegate to Adaptee members.
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
template<typename Target,
         typename Adaptee,
         typename Derived>
class class_adapter : public  Target,
                      private Adaptee
{
protected:
    // adaptee_ref
    //   function: gives derived classes access to the private base.
    Adaptee&
    adaptee_ref() noexcept
    {
        return static_cast<Adaptee&>(*this);
    }

    const Adaptee&
    adaptee_ref() const noexcept
    {
        return static_cast<const Adaptee&>(*this);
    }

public:
    using target_type  = Target;
    using adaptee_type = Adaptee;

    class_adapter() = default;

    explicit class_adapter(
            const Adaptee& _a
        )
            : Target(),
              Adaptee(_a)
        {}

    explicit class_adapter(
            Adaptee&& _a
        )
            : Target(),
              Adaptee(std::move(_a))
        {}
};


///////////////////////////////////////////////////////////////////////////////
///         VI.   INTERFACE ADAPTER — CRTP (C++11+)                        ///
///////////////////////////////////////////////////////////////////////////////

// interface_adapter
//   class: CRTP base for zero-overhead interface adaptation. Derived
// implements the target interface by delegating to an internally held
// adaptee. Unlike object_adapter, there is no policy indirection — the
// mapping is hard-coded in Derived, yielding fully inlinable dispatch.
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
template<typename Derived,
         typename Adaptee,
         typename OwnershipPolicy = by_value>
class interface_adapter
{
private:
    using storage_policy = typename OwnershipPolicy::template
                               storage<Adaptee>;
    using stored_type    = typename storage_policy::stored_type;

public:
    using adaptee_type     = Adaptee;
    using ownership_policy = OwnershipPolicy;

    interface_adapter() = default;

    explicit interface_adapter(
            stored_type _adaptee
        )
            : m_adaptee(std::forward<stored_type>(_adaptee))
        {}

protected:
    D_CONSTEXPR_CPP14 Adaptee&
    adaptee() noexcept
    {
        return storage_policy::access(m_adaptee);
    }

    D_CONSTEXPR_INLINE const Adaptee&
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
    template<typename Adaptee,
             typename Method,
             typename... Args>
    static auto
    invoke(
        Adaptee& _a,
        Method    _m,
        Args&&... _args
    )
        -> decltype((_a.*_m)(std::forward<Args>(_args)...))
    {
        return (_a.*_m)(std::forward<Args>(_args)...);
    }
};

// forward_with_transform
//   policy: applies a transformation function to each argument before
// forwarding. Useful for unit conversion, type coercion, etc.
template<typename Transform>
struct forward_with_transform
{
    template<typename Adaptee,
             typename Method,
             typename... Args>
    static auto
    invoke(
        Adaptee&   _a,
        Method      _m,
        Transform& _xform,
        Args&&...  _args
    )
        -> decltype((_a.*_m)(_xform(std::forward<Args>(_args))...))
    {
        return (_a.*_m)(_xform(std::forward<Args>(_args))...);
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
template<typename Fn,
         typename Transform = void>
class function_adapter
{
public:
    using function_type  = Fn;
    using transform_type = Transform;

    function_adapter(
            Fn         _fn,
            Transform _xform
        )
            : m_fn(std::move(_fn)),
              m_xform(std::move(_xform))
        {}

    template<typename... Args>
    auto operator()(
        Args&&... _args
    )
        -> decltype(std::declval<Fn>()(
            std::declval<Transform>()(std::forward<Args>(_args))...))
    {
        return m_fn(m_xform(std::forward<Args>(_args))...);
    }

    template<typename... Args>
    auto operator()(
        Args&&... _args
    ) const
        -> decltype(std::declval<const Fn>()(
            std::declval<const Transform>()(std::forward<Args>(_args))...))
    {
        return m_fn(m_xform(std::forward<Args>(_args))...);
    }

private:
    Fn         m_fn;
    Transform m_xform;
};

// function_adapter (no transform specialization)
//   class: passthrough adapter that simply wraps a callable with no
// argument transformation. Useful as a uniform wrapper type.
template<typename Fn>
class function_adapter<Fn, void>
{
public:
    using function_type = Fn;

    explicit function_adapter(
            Fn _fn
        )
            : m_fn(std::move(_fn))
        {}

    template<typename... Args>
    auto operator()(
        Args&&... _args
    )
        -> decltype(std::declval<Fn>()(std::forward<Args>(_args)...))
    {
        return m_fn(std::forward<Args>(_args)...);
    }

    template<typename... Args>
    auto operator()(
        Args&&... _args
    ) const
        -> decltype(std::declval<const Fn>()(std::forward<Args>(_args)...))
    {
        return m_fn(std::forward<Args>(_args)...);
    }

private:
    Fn m_fn;
};

// result_adapter
//   class: wraps a callable and transforms its return value through
// a post-processing function.
//
// Usage:
//   result_adapter ra(strlen, [](std::size_t n){ return (int)n; });
//   int len = ra("hello");
template<typename Fn,
         typename ResultTransform>
class result_adapter
{
public:
    using function_type  = Fn;
    using transform_type = ResultTransform;

    result_adapter(
            Fn               _fn,
            ResultTransform _xform
        )
            : m_fn(std::move(_fn)),
              m_xform(std::move(_xform))
        {}

    template<typename... Args>
    auto operator()(
        Args&&... _args
    )
        -> decltype(std::declval<ResultTransform>()(
            std::declval<Fn>()(std::forward<Args>(_args)...)))
    {
        return m_xform(m_fn(std::forward<Args>(_args)...));
    }

    template<typename... Args>
    auto operator()(
        Args&&... _args
    ) const
        -> decltype(std::declval<const ResultTransform>()(
            std::declval<const Fn>()(std::forward<Args>(_args)...)))
    {
        return m_xform(m_fn(std::forward<Args>(_args)...));
    }

private:
    Fn               m_fn;
    ResultTransform m_xform;
};

// argument_adapter
//   class: wraps a callable and individually transforms each argument
// position through a tuple of per-position transforms.
//
// Usage:
//   argument_adapter aa(fn, std::make_tuple(to_int, to_float));
//   aa("42", "3.14");  // calls fn(to_int("42"), to_float("3.14"))
template<typename Fn,
         typename ArgTransformTuple>
class argument_adapter
{
public:
    argument_adapter(
            Fn                 _fn,
            ArgTransformTuple _xforms
        )
            : m_fn(std::move(_fn)),
              m_xforms(std::move(_xforms))
        {}

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    template<typename... Args>
    auto operator()(
        Args&&... _args
    )
    {
        return invoke_impl(std::index_sequence_for<Args...>{},
                           std::forward<Args>(_args)...);
    }

private:
    template<std::size_t... Is,
             typename...    Args>
    auto invoke_impl(
        re_std::index_sequence<Is...>,
        Args&&... _args
    )
    {
        return m_fn(
            std::get<Is>(m_xforms)(std::forward<Args>(_args))...);
    }

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

private:
    Fn                 m_fn;
    ArgTransformTuple m_xforms;
};

// compose_adapter
//   class: function composition adapter. Chains two callables such
// that operator()(args...) evaluates Outer(Inner(args...)).
template<typename Outer,
         typename Inner>
class compose_adapter
{
public:
    compose_adapter(
            Outer _outer,
            Inner _inner
        )
            : m_outer(std::move(_outer)),
              m_inner(std::move(_inner))
        {}

    template<typename... Args>
    auto operator()(
        Args&&... _args
    )
        -> decltype(std::declval<Outer>()(
            std::declval<Inner>()(std::forward<Args>(_args)...)))
    {
        return m_outer(m_inner(std::forward<Args>(_args)...));
    }

    template<typename... Args>
    auto operator()(
        Args&&... _args
    ) const
        -> decltype(std::declval<const Outer>()(
            std::declval<const Inner>()(std::forward<Args>(_args)...)))
    {
        return m_outer(m_inner(std::forward<Args>(_args)...));
    }

private:
    Outer m_outer;
    Inner m_inner;
};


///////////////////////////////////////////////////////////////////////////////
///            IX.   VIEW ADAPTERS (C++11+)                                 ///
///////////////////////////////////////////////////////////////////////////////

// adapted_ref
//   class: non-owning mutable reference adapter. Holds a reference to
// an adaptee and exposes the target interface by delegating through
// an AdaptationPolicy — identical to object_adapter<by_reference>
// but with a lighter, view-semantic API.
template<typename Adaptee,
         typename AdaptationPolicy>
class adapted_ref
{
public:
    using adaptee_type      = Adaptee;
    using adaptation_policy = AdaptationPolicy;

    explicit adapted_ref(
            Adaptee& _adaptee
        ) noexcept
            : m_ref(_adaptee)
        {}

    Adaptee&
    adaptee() noexcept
    {
        return m_ref;
    }

    const Adaptee&
    adaptee() const noexcept
    {
        return m_ref;
    }

    template<typename P = AdaptationPolicy>
    auto size() const
        -> decltype(P::size(std::declval<const Adaptee&>()))
    {
        return AdaptationPolicy::size(m_ref);
    }

    template<typename P = AdaptationPolicy,
             typename Index>
    auto get(
        Index _i
    )
        -> decltype(P::get(std::declval<Adaptee&>(), _i))
    {
        return AdaptationPolicy::get(m_ref, _i);
    }

    template<typename P = AdaptationPolicy,
             typename Index>
    auto get(
        Index _i
    ) const
        -> decltype(P::get(std::declval<const Adaptee&>(), _i))
    {
        return AdaptationPolicy::get(m_ref, _i);
    }

private:
    Adaptee& m_ref;
};

// adapted_const_ref
//   class: non-owning const reference adapter. Read-only view of an
// adaptee through a target interface policy.
template<typename Adaptee,
         typename AdaptationPolicy>
class adapted_const_ref
{
public:
    using adaptee_type      = Adaptee;
    using adaptation_policy = AdaptationPolicy;

    explicit adapted_const_ref(
            const Adaptee& _adaptee
        ) noexcept
            : m_ref(_adaptee)
        {}

    const Adaptee&
    adaptee() const noexcept
    {
        return m_ref;
    }

    template<typename P = AdaptationPolicy>
    auto size() const
        -> decltype(P::size(std::declval<const Adaptee&>()))
    {
        return AdaptationPolicy::size(m_ref);
    }

    template<typename P = AdaptationPolicy,
             typename Index>
    auto get(
        Index _i
    ) const
        -> decltype(P::get(std::declval<const Adaptee&>(), _i))
    {
        return AdaptationPolicy::get(m_ref, _i);
    }

private:
    const Adaptee& m_ref;
};

// adapted_view
//   class: non-owning adapter that presents an adaptee's iteration
// interface through a projection function. Each dereferenced element
// is transformed by Projection before being returned.
//
// Usage:
//   std::vector<std::pair<int,std::string>> data = ...;
//   auto keys = adapted_view(data, [](auto& p){ return p.first; });
//   for (auto k : keys) { ... }
template<typename Adaptee,
         typename Projection>
class adapted_view
{
public:
    using adaptee_type   = Adaptee;
    using projection_type = Projection;

    adapted_view(
            Adaptee&   _adaptee,
            Projection _proj
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
            std::declval<Adaptee&>()));

    public:
        explicit iterator(
                inner_iterator _it,
                Projection*   _proj
            )
                : m_it(_it),
                  m_proj(_proj)
            {}

        auto operator*()
            -> decltype(std::declval<Projection>()(*std::declval<inner_iterator>()))
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
        Projection*   m_proj;
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
        -> decltype(std::declval<const Adaptee&>().size())
    {
        return m_ref.size();
    }

private:
    Adaptee&   m_ref;
    Projection m_proj;
};


///////////////////////////////////////////////////////////////////////////////
///          X.    CONVENIENCE FACTORIES (C++14+)                           ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// make_object_adapter
//   function: deduces template arguments for object_adapter.
template<typename AdaptationPolicy,
         typename Adaptee>
inline auto
make_object_adapter(
    Adaptee& _adaptee
)
{
    return object_adapter<Adaptee,
                          AdaptationPolicy,
                          by_reference>(_adaptee);
}

// make_owning_adapter
//   function: creates a by-value owning object_adapter.
template<typename AdaptationPolicy,
         typename Adaptee>
inline auto
make_owning_adapter(
    Adaptee _adaptee
)
{
    return object_adapter<Adaptee,
                          AdaptationPolicy,
                          by_value>(std::move(_adaptee));
}

// make_function_adapter
//   function: deduces template arguments for function_adapter.
template<typename Fn,
         typename Transform>
inline auto
make_function_adapter(
    Fn&&        _fn,
    Transform&& _xform
)
{
    return function_adapter<typename std::decay<Fn>::type,
                            typename std::decay<Transform>::type>(
        std::forward<Fn>(_fn),
        std::forward<Transform>(_xform));
}

// make_function_adapter (no transform)
template<typename Fn>
inline auto
make_function_adapter(
    Fn&& _fn
)
{
    return function_adapter<typename std::decay<Fn>::type>(
        std::forward<Fn>(_fn));
}

// make_result_adapter
//   function: deduces template arguments for result_adapter.
template<typename Fn,
         typename ResultTransform>
inline auto
make_result_adapter(
    Fn&&              _fn,
    ResultTransform&& _xform
)
{
    return result_adapter<typename std::decay<Fn>::type,
                          typename std::decay<ResultTransform>::type>(
        std::forward<Fn>(_fn),
        std::forward<ResultTransform>(_xform));
}

// make_compose
//   function: creates a compose_adapter from two callables.
template<typename Outer,
         typename Inner>
inline auto
make_compose(
    Outer&& _outer,
    Inner&& _inner
)
{
    return compose_adapter<typename std::decay<Outer>::type,
                           typename std::decay<Inner>::type>(
        std::forward<Outer>(_outer),
        std::forward<Inner>(_inner));
}

// make_adapted_ref
//   function: deduces template arguments for adapted_ref.
template<typename AdaptationPolicy,
         typename Adaptee>
inline auto
make_adapted_ref(
    Adaptee& _adaptee
)
{
    return adapted_ref<Adaptee, AdaptationPolicy>(_adaptee);
}

// make_adapted_view
//   function: deduces template arguments for adapted_view.
template<typename Adaptee,
         typename Projection>
inline auto
make_adapted_view(
    Adaptee&    _adaptee,
    Projection&& _proj
)
{
    return adapted_view<Adaptee, typename std::decay<Projection>::type>(
        _adaptee,
        std::forward<Projection>(_proj));
}

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///       XI.   CONCEPT-CONSTRAINED ADAPTERS (C++20+)                      ///
///////////////////////////////////////////////////////////////////////////////

#if D_ADAPTER_HAS_CONCEPTS

// adaptable_to
//   concept: constrains types that share enough structural surface for
// direct adaptation (compatible value types + structural overlap).
template<typename Adaptee,
         typename Target>
concept adaptable_to =
    ( are_value_type_compatible<Adaptee, Target>::value &&
      is_structurally_adaptable<Adaptee, Target>::value );

// adapter_for
//   concept: constrains an adapter type that exposes both adaptee()
// and at least one target-interface method (size or get).
template<typename Adapter>
concept adapter_for = requires(Adapter& _a, const Adapter& _ca)
{
    _a.adaptee();
    { _ca.size() } -> std::convertible_to<std::size_t>;
};

// function_adaptable
//   concept: constrains callables that can be wrapped by
// function_adapter (must be invocable).
template<typename Fn,
         typename... Args>
concept function_adaptable = std::invocable<Fn, Args...>;

// constrained_adapt
//   function: concept-constrained factory for object adapters.
template<typename AdaptationPolicy,
         typename Adaptee,
         typename Target>
    requires adaptable_to<Adaptee, Target>
inline auto
constrained_adapt(
    Adaptee& _adaptee
)
{
    return object_adapter<Adaptee,
                          AdaptationPolicy,
                          by_reference>(_adaptee);
}

#endif  // D_ADAPTER_HAS_CONCEPTS


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARADIGM_ADAPTER_ADAPTER_HPP
