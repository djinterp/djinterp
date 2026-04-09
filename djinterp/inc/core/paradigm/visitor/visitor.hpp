/******************************************************************************
* djinterp [core]                                                 visitor.hpp
*
* djinterp visitor pattern template module:
*   This header provides a comprehensive, version-portable implementation of
* the visitor pattern supporting multiple dispatch strategies:
*   - classic (Gamma-style) double dispatch via virtual accept/visit
*   - acyclic visitor decoupling via dynamic_cast
*   - static visitor via CRTP and compile-time dispatch
*   - variant visitor for std::variant-based type-safe visitation (C++17+)
*
*   PORTABILITY:
*   This header uses env.h for C++ version detection and cpp_features.h for
* fine-grained feature detection. It provides:
*   - C++98/03 : classic visitor (virtual-based, macro-assisted)
*   - C++11    : acyclic visitor, static visitor (CRTP + variadic templates)
*   - C++14    : generic lambdas in overload sets
*   - C++17    : variant_visitor, overloaded (fold + deduction guides)
*   - C++20    : concept-constrained visitors
*
* NAMING CONVENTIONS:
*   visitor_base         - abstract visitor interface
*   visitable_base       - abstract element interface
*   acyclic_visitor      - type-erased acyclic visitor base
*   visitor_of           - acyclic per-type visitor interface
*   static_visitor       - CRTP-based compile-time visitor
*   variant_visitor      - std::visit wrapper with overload support
*   overloaded           - lambda overload set builder
*   visit_result         - return type deduction trait
*
* path:      /inc/patterns/visitor.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.08
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    FORWARD DECLARATIONS & CONFIGURATION
      -------------------------------------
      i.    feature gate macros
            a. D_VISITOR_HAS_VARIADIC_TEMPLATES
            b. D_VISITOR_HAS_VARIANT
            c. D_VISITOR_HAS_CONCEPTS
            d. D_VISITOR_HAS_FOLD_EXPRESSIONS
            e. D_VISITOR_HAS_DEDUCTION_GUIDES
      ii.   return type configuration
            a. D_VISITOR_DEFAULT_RETURN_TYPE

II.   CLASSIC VISITOR (C++98+)
      -------------------------
      i.    visitor_base
      ii.   visitable_base
      iii.  D_VISITABLE  (macro)
      iv.   D_VISITOR_OF  (macro, C++98 only)

III.  ACYCLIC VISITOR (C++11+)
      --------------------------
      i.    acyclic_visitor
      ii.   visitor_of
      iii.  acyclic_visitable
      iv.   D_ACYCLIC_VISITABLE  (macro)

IV.   STATIC VISITOR (C++11+)
      -------------------------
      i.    visit_result (internal)
      ii.   static_visitor
      iii.  static_visitable

V.    VARIANT VISITOR (C++17+)
      --------------------------
      i.    overloaded
      ii.   make_visitor
      iii.  variant_visit
      iv.   variant_visit_with_index

VI.   CONCEPT-CONSTRAINED VISITOR (C++20+)
      --------------------------------------
      i.    visitable_type (concept)
      ii.   visitor_for (concept)
      iii.  acyclic_visitor_for (concept)
*/

#ifndef DJINTERP_VISITOR_
#define DJINTERP_VISITOR_ 1

#include <type_traits>
#include ".\djinterp.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <variant>
    #include <tuple>
#endif

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <utility>
#endif


///////////////////////////////////////////////////////////////////////////////
///           I.   FORWARD DECLARATIONS & CONFIGURATION                     ///
///////////////////////////////////////////////////////////////////////////////

// i.   feature gate macros
//////////////////////////////////////////

// D_VISITOR_HAS_VARIADIC_TEMPLATES
//   macro: 1 if variadic templates are available (C++11+).
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #define D_VISITOR_HAS_VARIADIC_TEMPLATES 1
#else
    #define D_VISITOR_HAS_VARIADIC_TEMPLATES 0
#endif

// D_VISITOR_HAS_VARIANT
//   macro: 1 if std::variant is available (C++17+).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_VISITOR_HAS_VARIANT 1
#else
    #define D_VISITOR_HAS_VARIANT 0
#endif

// D_VISITOR_HAS_CONCEPTS
//   macro: 1 if concepts are available (C++20+).
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #define D_VISITOR_HAS_CONCEPTS 1
#else
    #define D_VISITOR_HAS_CONCEPTS 0
#endif

// D_VISITOR_HAS_FOLD_EXPRESSIONS
//   macro: 1 if fold expressions are available (C++17+).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_VISITOR_HAS_FOLD_EXPRESSIONS 1
#else
    #define D_VISITOR_HAS_FOLD_EXPRESSIONS 0
#endif

// D_VISITOR_HAS_DEDUCTION_GUIDES
//   macro: 1 if class template argument deduction is available (C++17+).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_VISITOR_HAS_DEDUCTION_GUIDES 1
#else
    #define D_VISITOR_HAS_DEDUCTION_GUIDES 0
#endif


// ii.  return type configuration
//////////////////////////////////////////

// D_VISITOR_DEFAULT_RETURN_TYPE
//   macro: default return type for visitor visit() methods.
// Users may define this before including visitor.hpp to override.
#ifndef D_VISITOR_DEFAULT_RETURN_TYPE
    #define D_VISITOR_DEFAULT_RETURN_TYPE void
#endif


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                  II.   CLASSIC VISITOR (C++98+)                         ///
///////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// visitor_base
// -----------------------------------------------------------------------------

// visitor_base
//   class: abstract base class for the classic (Gamma-style) visitor.
// Parameterized on _ReturnType to allow visitors that produce values.
// Derive from this and add virtual visit() overloads for each concrete
// element type.
template<typename _ReturnType = D_VISITOR_DEFAULT_RETURN_TYPE>
class visitor_base
{
public:
    // return_type
    //   type: the return type of all visit() methods in this visitor.
    typedef _ReturnType return_type;

    virtual ~visitor_base()
    {}
};

// -----------------------------------------------------------------------------
// visitable_base
// -----------------------------------------------------------------------------

// visitable_base
//   class: abstract base class for elements in the classic visitor pattern.
// Parameterized on _ReturnType to match the visitor's return type.
// Concrete elements must implement accept() to call the appropriate
// visit() overload on the visitor.
template<typename _ReturnType = D_VISITOR_DEFAULT_RETURN_TYPE>
class visitable_base
{
public:
    // return_type
    //   type: the return type produced by accept().
    typedef _ReturnType return_type;

    virtual ~visitable_base()
    {}

    virtual _ReturnType accept(visitor_base<_ReturnType>&) = 0;
};

// D_VISITABLE
//   macro: injects an accept() implementation into a concrete element
// class. The element must inherit from visitable_base (or provide a
// compatible interface). Calls visitor.visit(*this).
//
// Usage:
//   class circle : public visitable_base<void>
//   {
//   public:
//       D_VISITABLE(void)
//   };
#define D_VISITABLE(_ReturnType)                                             \
    virtual _ReturnType accept(                                              \
        ::djinterp::visitor_base<_ReturnType>& _visitor                      \
    )                                                                        \
    {                                                                        \
        return static_cast<                                                  \
            ::djinterp::visitor_of_impl<_ReturnType,                         \
                typename ::djinterp::clean_t<                                \
                    decltype(*this)>>&>(_visitor)                             \
            .visit(*this);                                                   \
    }


#if D_VISITOR_HAS_VARIADIC_TEMPLATES

// D_VISITOR_OF_IMPL
//   (internal): variadic-aware per-type visitor interface, used by
// D_VISITABLE. Not for direct use.

NS_INTERNAL

    // visitor_of_base
    //   trait: recursive base for building visit() overload sets
    // (terminal case).
    template<typename _ReturnType,
             typename... _Types>
    struct visitor_of_base
    {
        virtual ~visitor_of_base()
        {}
    };

    // visitor_of_base<_ReturnType, _Head, _Tail...>
    //   trait: recursive case. Adds a pure virtual visit(_Head&) and
    // inherits the rest.
    template<typename    _ReturnType,
             typename    _Head,
             typename... _Tail>
    struct visitor_of_base<_ReturnType, _Head, _Tail...>
        : public visitor_of_base<_ReturnType, _Tail...>
    {
        using visitor_of_base<_ReturnType, _Tail...>::visit;
        virtual _ReturnType visit(_Head&) = 0;
    };

NS_END  // internal


// visitor_of_impl
//   class: per-type visitor interface. Given a return type and a single
// element type, provides the virtual visit() overload.
template<typename _ReturnType,
         typename _ElementType>
class visitor_of_impl
{
public:
    virtual ~visitor_of_impl()
    {}

    virtual _ReturnType visit(_ElementType&) = 0;
};


// concrete_visitor
//   class: convenience base that composes visitor_base with visit()
// overloads for all specified element types. Derive from this and
// implement each visit() overload.
//
// Usage:
//   class my_visitor : public concrete_visitor<void, circle, rect>
//   {
//   public:
//       void visit(circle&) override { ... }
//       void visit(rect&) override   { ... }
//   };
template<typename    _ReturnType,
         typename... _ElementTypes>
class concrete_visitor : public visitor_base<_ReturnType>,
                         public internal::visitor_of_base<_ReturnType,
                                                          _ElementTypes...>
{
public:
    typedef _ReturnType return_type;
};


///////////////////////////////////////////////////////////////////////////////
///                 III.  ACYCLIC VISITOR (C++11+)                          ///
///////////////////////////////////////////////////////////////////////////////

// The acyclic visitor breaks the cyclic dependency between the visitor
// and element hierarchies by using dynamic_cast at the point of
// dispatch. This allows new element types to be added without modifying
// the visitor base, at the cost of a runtime dynamic_cast per visit.

// -----------------------------------------------------------------------------
// acyclic_visitor
// -----------------------------------------------------------------------------

// acyclic_visitor
//   class: type-erased base for acyclic visitors. Concrete visitors
// inherit from both acyclic_visitor and one or more visitor_of<T>
// instantiations.
class acyclic_visitor
{
public:
    virtual ~acyclic_visitor()
    {}
};

// -----------------------------------------------------------------------------
// visitor_of
// -----------------------------------------------------------------------------

// visitor_of
//   class: per-type acyclic visitor interface. Provides a single
// virtual visit() method for _ElementType. A concrete acyclic visitor
// inherits from visitor_of<T> for each type it wishes to handle.
template<typename _ElementType,
         typename _ReturnType = D_VISITOR_DEFAULT_RETURN_TYPE>
class visitor_of
{
public:
    // return_type
    //   type: the return type produced by visit().
    typedef _ReturnType return_type;

    virtual ~visitor_of()
    {}

    virtual _ReturnType visit(_ElementType&) = 0;
};

// -----------------------------------------------------------------------------
// acyclic_visitable
// -----------------------------------------------------------------------------

// acyclic_visitable
//   class: base for elements in the acyclic visitor pattern. Uses
// dynamic_cast internally to find the correct visitor_of<T> interface
// on the visiting object. Returns _DefaultReturn if the visitor does
// not handle this element type.
template<typename _ReturnType    = D_VISITOR_DEFAULT_RETURN_TYPE,
         _ReturnType _DefaultReturn = _ReturnType()>
class acyclic_visitable
{
public:
    // return_type
    //   type: the return type produced by accept().
    typedef _ReturnType return_type;

    virtual ~acyclic_visitable()
    {}

    virtual _ReturnType accept(acyclic_visitor&) = 0;
};

// D_ACYCLIC_VISITABLE
//   macro: injects an accept() implementation for the acyclic visitor
// pattern into a concrete element class. Uses dynamic_cast to locate
// the matching visitor_of<ThisType> on the visitor.
//
// Usage:
//   class circle : public acyclic_visitable<void>
//   {
//   public:
//       D_ACYCLIC_VISITABLE(circle, void)
//   };
#define D_ACYCLIC_VISITABLE(_ThisType, _ReturnType)                          \
    virtual _ReturnType accept(                                              \
        ::djinterp::acyclic_visitor& _visitor                                \
    ) override                                                               \
    {                                                                        \
        typedef ::djinterp::visitor_of<_ThisType, _ReturnType> target_type;  \
        target_type* p = dynamic_cast<target_type*>(&_visitor);              \
        if (p)                                                               \
        {                                                                    \
            return p->visit(*this);                                          \
        }                                                                    \
                                                                             \
        return _ReturnType();                                                \
    }


///////////////////////////////////////////////////////////////////////////////
///                  IV.   STATIC VISITOR (C++11+)                          ///
///////////////////////////////////////////////////////////////////////////////

// The static visitor uses CRTP to achieve compile-time dispatch. No
// virtual functions are involved; the derived visitor type is known
// at compile time and the visit() call is resolved statically. This
// is the highest-performance variant but requires the full type set
// to be known at the call site.

NS_INTERNAL

    // visit_result
    //   trait: deduces the return type of calling _Visitor::visit(_Element&).
    template<typename _Visitor,
             typename _Element,
             typename = void>
    struct visit_result
    {};

    // visit_result (well-formed case)
    //   trait: specialization for when visit() is callable.
    template<typename _Visitor,
             typename _Element>
    struct visit_result<_Visitor, _Element, void_t<
        decltype(std::declval<_Visitor>().visit(std::declval<_Element&>()))
    >>
    {
        using type = decltype(
            std::declval<_Visitor>().visit(std::declval<_Element&>()));
    };

    // visit_result_t
    //   type: convenience alias for visit_result<...>::type.
    template<typename _Visitor,
             typename _Element>
    using visit_result_t = typename visit_result<_Visitor, _Element>::type;

NS_END  // internal

// -----------------------------------------------------------------------------
// static_visitor
// -----------------------------------------------------------------------------

// static_visitor
//   class: CRTP base for compile-time visitors. _Derived must implement
// visit() overloads for each element type it wishes to handle.
// Provides apply() which statically dispatches to the derived visit().
//
// Usage:
//   class my_visitor : public static_visitor<my_visitor>
//   {
//   public:
//       void visit(circle& c)  { ... }
//       void visit(rect& r)    { ... }
//   };
//
//   my_visitor v;
//   v.apply(some_circle);
template<typename _Derived>
class static_visitor
{
public:
    template<typename _Element>
    auto apply(_Element& _element)
        -> internal::visit_result_t<_Derived, _Element>
    {
        return static_cast<_Derived*>(this)->visit(_element);
    }

    template<typename _Element>
    auto apply(const _Element& _element)
        -> internal::visit_result_t<_Derived, const _Element>
    {
        return static_cast<_Derived*>(this)->visit(_element);
    }

    template<typename _Element>
    auto apply(const _Element& _element) const
        -> internal::visit_result_t<const _Derived, const _Element>
    {
        return static_cast<const _Derived*>(this)->visit(_element);
    }
};

// -----------------------------------------------------------------------------
// static_visitable
// -----------------------------------------------------------------------------

// static_visitable
//   class: CRTP base for elements that accept static visitors. _Derived
// is the concrete element type. Provides accept() which forwards to the
// visitor's apply() method.
template<typename _Derived>
class static_visitable
{
public:
    template<typename _Visitor>
    auto accept(_Visitor& _visitor)
        -> internal::visit_result_t<_Visitor, _Derived>
    {
        return _visitor.apply(static_cast<_Derived&>(*this));
    }

    template<typename _Visitor>
    auto accept(_Visitor& _visitor) const
        -> internal::visit_result_t<_Visitor, const _Derived>
    {
        return _visitor.apply(static_cast<const _Derived&>(*this));
    }
};


#endif  // D_VISITOR_HAS_VARIADIC_TEMPLATES


///////////////////////////////////////////////////////////////////////////////
///                  V.    VARIANT VISITOR (C++17+)                         ///
///////////////////////////////////////////////////////////////////////////////

#if D_VISITOR_HAS_VARIANT

// The variant visitor leverages std::variant and std::visit to provide
// type-safe, closed-set visitation with zero boilerplate base classes.
// Combined with the overloaded lambda pattern, this is the most
// ergonomic visitor form available in modern C++.

// -----------------------------------------------------------------------------
// overloaded
// -----------------------------------------------------------------------------

// overloaded
//   class: aggregates multiple callable objects (typically lambdas) into
// a single overload set. Uses C++17 variadic inheritance and fold
// expressions.
//
// Usage:
//   auto vis = overloaded {
//       [](circle& c)  { ... },
//       [](rect& r)    { ... },
//       [](auto& other) { ... }
//   };
template<typename... _Fns>
struct overloaded : _Fns...
{
    using _Fns::operator()...;
};

#if D_VISITOR_HAS_DEDUCTION_GUIDES
    // overloaded deduction guide
    //   guide: deduces template arguments from constructor arguments.
    template<typename... _Fns>
    overloaded(_Fns...) -> overloaded<_Fns...>;
#endif

// -----------------------------------------------------------------------------
// make_visitor
// -----------------------------------------------------------------------------

// make_visitor
//   function: factory for overloaded lambda visitors. Equivalent to
// constructing overloaded{...} but available as a function call for
// contexts where CTAD is unavailable or undesirable.
template<typename... _Fns>
D_CONSTEXPR_INLINE overloaded<std::decay_t<_Fns>...>
make_visitor(
    _Fns&&... _fns
)
{
    return overloaded<std::decay_t<_Fns>...>{
        std::forward<_Fns>(_fns)...};
}

// -----------------------------------------------------------------------------
// variant_visit
// -----------------------------------------------------------------------------

// variant_visit
//   function: applies a visitor (overload set) to a variant. Thin
// wrapper around std::visit for naming consistency.
template<typename _Visitor,
         typename _Variant>
D_CONSTEXPR_INLINE decltype(auto)
variant_visit(
    _Visitor&& _visitor,
    _Variant&& _variant
)
{
    return std::visit(std::forward<_Visitor>(_visitor),
                      std::forward<_Variant>(_variant));
}

// variant_visit (multi-variant)
//   function: applies a visitor to multiple variants simultaneously.
// Enables multi-dispatch over variant types.
template<typename    _Visitor,
         typename... _Variants>
D_CONSTEXPR_INLINE decltype(auto)
variant_visit(
    _Visitor&&    _visitor,
    _Variants&&... _variants
)
{
    return std::visit(std::forward<_Visitor>(_visitor),
                      std::forward<_Variants>(_variants)...);
}

NS_INTERNAL

    // variant_visit_with_index_helper
    //   function: internal helper that wraps each variant alternative
    // dispatch to include the runtime index as a compile-time constant.
    template<typename _Visitor,
             typename _Variant,
             std::size_t... _Is>
    D_CONSTEXPR_INLINE decltype(auto)
    variant_visit_with_index_impl(
        _Visitor&&          _visitor,
        _Variant&&          _variant,
        std::index_sequence<_Is...>
    )
    {
        using return_type = std::common_type_t<
            decltype(_visitor(
                std::integral_constant<std::size_t, _Is>{},
                std::get<_Is>(std::forward<_Variant>(_variant))))...
        >;

        using dispatch_fn = return_type(*)(
            _Visitor&&, _Variant&&);

        // dispatch table
        static constexpr dispatch_fn table[] =
        {
            [](
                _Visitor&& _v,
                _Variant&& _var
            ) -> return_type
            {
                return _v(
                    std::integral_constant<std::size_t, _Is>{},
                    std::get<_Is>(std::forward<_Variant>(_var)));
            }...
        };

        return table[_variant.index()](
            std::forward<_Visitor>(_visitor),
            std::forward<_Variant>(_variant));
    }

NS_END  // internal

// variant_visit_with_index
//   function: like variant_visit, but the visitor receives the
// alternative index as a compile-time std::integral_constant as its
// first argument. Useful when the visitor needs to know which
// alternative is active.
//
// Usage:
//   variant_visit_with_index(
//       [](auto _index, auto& _val) {
//           std::cout << "index=" << _index() << "\n";
//       },
//       my_variant);
template<typename _Visitor,
         typename _Variant>
D_CONSTEXPR_INLINE decltype(auto)
variant_visit_with_index(
    _Visitor&& _visitor,
    _Variant&& _variant
)
{
    return internal::variant_visit_with_index_impl(
        std::forward<_Visitor>(_visitor),
        std::forward<_Variant>(_variant),
        std::make_index_sequence<
            std::variant_size_v<std::remove_reference_t<_Variant>>>{});
}


#endif  // D_VISITOR_HAS_VARIANT


///////////////////////////////////////////////////////////////////////////////
///            VI.   CONCEPT-CONSTRAINED VISITOR (C++20+)                   ///
///////////////////////////////////////////////////////////////////////////////

#if D_VISITOR_HAS_CONCEPTS

// visitable_type
//   concept: constrains types that expose an accept() method taking a
// reference to a visitor. Matches both classic and acyclic visitables.
template<typename _T,
         typename _Visitor>
concept visitable_type = requires(_T _t, _Visitor& _v)
{
    _t.accept(_v);
};

// visitor_for
//   concept: constrains a visitor type that can visit all of the given
// element types. Each element must be callable via visit().
template<typename    _Visitor,
         typename... _Elements>
concept visitor_for =
    (requires(_Visitor& _v, _Elements& _e) { _v.visit(_e); } && ...);

// acyclic_visitor_for
//   concept: constrains a type that is both an acyclic_visitor and
// provides visitor_of<T> interfaces for all specified element types.
template<typename    _Visitor,
         typename... _Elements>
concept acyclic_visitor_for =
    ( std::derived_from<_Visitor, acyclic_visitor> &&
      (std::derived_from<_Visitor, visitor_of<_Elements>>  && ...) );

// constrained_accept
//   function: accept() that statically verifies the visitor handles
// the element type. Provides a clear compile error when a visitor
// is missing a required visit() overload.
template<typename _Element,
         typename _Visitor>
    requires visitor_for<_Visitor, _Element>
auto constrained_accept(
    _Element& _element,
    _Visitor& _visitor
)
    -> decltype(_visitor.visit(_element))
{
    return _visitor.visit(_element);
}

#endif  // D_VISITOR_HAS_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_VISITOR_
