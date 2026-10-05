/*******************************************************************************
* djinterp [core]                                                    visitor.hpp
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
*
* path:      /inc/djinterp/core/paradigm/visitor/visitor.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.08
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    FORWARD DECLARATIONS & CONFIGURATION
      ------------------------------------
      i.    feature gate macros
            a. D_VISITOR_HAS_VARIADIC_TEMPLATES
            b. D_VISITOR_HAS_VARIANT
            c.    D_VISITOR_HAS_CONCEPTS
            d. D_VISITOR_HAS_FOLD_EXPRESSIONS
            e. D_VISITOR_HAS_DEDUCTION_GUIDES
      ii.   return type configuration
            a. D_VISITOR_DEFAULT_RETURN_TYPE

II.   CLASSIC VISITOR (C++98+)
      ------------------------
      i.    visitor_base
      ii.   visitable_base
      iii.  D_VISITABLE  (macro)
      iv.   D_VISITOR_OF  (macro, C++98 only)

III.  ACYCLIC VISITOR (C++11+)
      ------------------------
      i.    acyclic_visitor
      ii.   visitor_of
      iii.  acyclic_visitable
      iv.   D_ACYCLIC_VISITABLE  (macro)

IV.   STATIC VISITOR (C++11+)
      -----------------------
      i.    visit_result (internal)
      ii.   static_visitor
      iii.  static_visitable

V.    VARIANT VISITOR (C++17+)
      ------------------------
      i.    overloaded
      ii.   make_visitor
      iii.  variant_visit
      iv.   variant_visit_with_index

VI.   CONCEPT-CONSTRAINED VISITOR (C++20+)
      ------------------------------------
      i.    visitable_type (concept)
      ii.   visitor_for (concept)
      iii.  acyclic_visitor_for (concept)
*/

#ifndef DJINTERP_PARADIGM_VISITOR_VISITOR_HPP
#define DJINTERP_PARADIGM_VISITOR_VISITOR_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>
#include <typeinfo>        // std::bad_cast -- D_VISITABLE's dynamic_cast may throw
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_utility.hpp"  // void_t
// re_std
#include "../../../../re_std/utility/make_integer_sequence.hpp"  // re_std::index_sequence,
                                                                 // make_index_sequence

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // std
    #include <tuple>
    #include <variant>
#endif

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // std
    #include <utility>
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // std
    #include <concepts>    // std::derived_from -- used by acyclic_visitor_for
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
// Parameterized on ReturnType to allow visitors that produce values.
// Derive from this and add virtual visit() overloads for each concrete
// element type.
template<typename ReturnType = D_VISITOR_DEFAULT_RETURN_TYPE>
class visitor_base
{
public:
    // return_type
    //   type: the return type of all visit() methods in this visitor.
    typedef ReturnType return_type;

    virtual ~visitor_base()
    {}
};

// -----------------------------------------------------------------------------
// visitable_base
// -----------------------------------------------------------------------------

// visitable_base
//   class: abstract base class for elements in the classic visitor pattern.
// Parameterized on ReturnType to match the visitor's return type.
// Concrete elements must implement accept() to call the appropriate
// visit() overload on the visitor.
template<typename ReturnType = D_VISITOR_DEFAULT_RETURN_TYPE>
class visitable_base
{
public:
    // return_type
    //   type: the return type produced by accept().
    typedef ReturnType return_type;

    virtual ~visitable_base()
    {}

    virtual ReturnType accept(visitor_base<ReturnType>&) = 0;
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
//
//   NOTE ON THE CAST: the visitor arrives type-erased as visitor_base<R>&, which
// declares no visit() at all.  The concrete interface for this element type,
// visitor_of_impl<R, ThisType>, is a SIBLING base of visitor_base<R> within the
// concrete visitor -- not a derived class of it -- so recovering it is a
// cross-cast.  static_cast cannot express that (the two types are unrelated, and
// the cast is simply ill-formed); only dynamic_cast can, and both types are
// polymorphic, so it is well-formed here.
//
//   The reference form is deliberate.  The classic visitor is a CLOSED set: a
// visitor is REQUIRED to handle every element, and concrete_visitor makes each
// visit() pure virtual precisely to enforce that.  A visitor that cannot accept
// this element is therefore a program error, and throwing std::bad_cast reports
// it loudly.  D_ACYCLIC_VISITABLE, whose element set is open, is the tolerant
// counterpart: it returns a default instead.
#define D_VISITABLE(ReturnType)                                             \
    virtual ReturnType accept(                                              \
        ::djinterp::visitor_base<ReturnType>& _visitor                      \
    )                                                                        \
    {                                                                        \
        return dynamic_cast<                                                 \
            ::djinterp::visitor_of_impl<ReturnType,                         \
                typename ::djinterp::clean_t<                                \
                    decltype(*this)>>&>(_visitor)                            \
            .visit(*this);                                                   \
    }


#if D_VISITOR_HAS_VARIADIC_TEMPLATES

// D_VISITOR_OF_IMPL
//   (internal): variadic-aware per-type visitor interface, used by
// D_VISITABLE. Not for direct use.

// visitor_of_impl
//   class: per-type visitor interface. Given a return type and a single
// element type, provides the virtual visit() overload.
//
//   Declared BEFORE the overload-set chain below, because that chain now
// INHERITS it: visitor_of_impl<R, X> must be a base of the visitor for every
// element type X in the set, since that is the subobject D_VISITABLE recovers in
// order to dispatch.  Previously the chain declared its own visit() overloads and
// left visitor_of_impl unrelated to everything, so D_VISITABLE's cast had no
// subobject to find.
template<typename ReturnType,
         typename ElementType>
class visitor_of_impl
{
public:
    virtual ~visitor_of_impl()
    {}

    virtual ReturnType visit(ElementType&) = 0;
};


NS_INTERNAL

    // visitor_of_base
    //   trait: recursive base for building visit() overload sets.  TERMINAL
    // case: no element types, and therefore no visit() -- which is exactly why
    // the recursive case must never name this one in a using-declaration.
    template<typename ReturnType,
             typename... Types>
    struct visitor_of_base
    {
        virtual ~visitor_of_base()
        {}
    };

    // visitor_of_base<ReturnType, Head>
    //   trait: the recursion's BASE CASE -- exactly one element type.  Inherits
    // that type's interface and stops.  There is nothing below it to import, so
    // it carries no using-declaration; this is the specialization that keeps the
    // empty terminal above from ever being named by one.
    template<typename ReturnType,
             typename Head>
    struct visitor_of_base<ReturnType, Head>
        : public visitor_of_impl<ReturnType, Head>
    {};

    // visitor_of_base<ReturnType, Head, Next, Tail...>
    //   trait: RECURSIVE case -- two or more element types.  Inherits Head's
    // interface alongside the chain for the rest, and merges BOTH visit() names
    // into this scope.  Both using-declarations are required: without them the
    // name would be found in two distinct base subobjects and every call would be
    // ambiguous before overload resolution ever ran.
    template<typename    ReturnType,
             typename    Head,
             typename    Next,
             typename... Tail>
    struct visitor_of_base<ReturnType, Head, Next, Tail...>
        : public visitor_of_impl<ReturnType, Head>,
          public visitor_of_base<ReturnType, Next, Tail...>
    {
        using visitor_of_impl<ReturnType, Head>::visit;
        using visitor_of_base<ReturnType, Next, Tail...>::visit;
    };

NS_END  // internal


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
template<typename    ReturnType,
         typename... ElementTypes>
class concrete_visitor : public visitor_base<ReturnType>,
                         public internal::visitor_of_base<ReturnType,
                                                          ElementTypes...>
{
public:
    typedef ReturnType return_type;
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
// virtual visit() method for ElementType. A concrete acyclic visitor
// inherits from visitor_of<T> for each type it wishes to handle.
template<typename ElementType,
         typename ReturnType = D_VISITOR_DEFAULT_RETURN_TYPE>
class visitor_of
{
public:
    // return_type
    //   type: the return type produced by visit().
    typedef ReturnType return_type;

    virtual ~visitor_of()
    {}

    virtual ReturnType visit(ElementType&) = 0;
};

// -----------------------------------------------------------------------------
// acyclic_visitable
// -----------------------------------------------------------------------------

// acyclic_visitable
//   class: base for elements in the acyclic visitor pattern. Uses
// dynamic_cast internally to find the correct visitor_of<T> interface
// on the visiting object. Returns DefaultReturn if the visitor does
// not handle this element type.
//
//   The no-match default is ReturnType(), produced by D_ACYCLIC_VISITABLE.  It
// is NOT a template parameter: a second parameter of the form
// `ReturnType DefaultReturn = ReturnType()` is a NON-TYPE parameter whose type
// is ReturnType, and void is not a permitted type for one -- so it made
// acyclic_visitable<void>, both the default and the documented spelling, ill-
// formed.  It was also never referenced by the class or the macro.
template<typename ReturnType = D_VISITOR_DEFAULT_RETURN_TYPE>
class acyclic_visitable
{
public:
    // return_type
    //   type: the return type produced by accept().
    typedef ReturnType return_type;

    virtual ~acyclic_visitable()
    {}

    virtual ReturnType accept(acyclic_visitor&) = 0;
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
#define D_ACYCLIC_VISITABLE(ThisType, ReturnType)                           \
    virtual ReturnType accept(                                              \
        ::djinterp::acyclic_visitor& _visitor                                \
    ) override                                                               \
    {                                                                        \
        typedef ::djinterp::visitor_of<ThisType, ReturnType> target_type;   \
        target_type* p = dynamic_cast<target_type*>(&_visitor);              \
        if (p)                                                               \
        {                                                                    \
            return p->visit(*this);                                          \
        }                                                                    \
                                                                             \
        return ReturnType();                                                \
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
    //   trait: deduces the return type of calling Visitor::visit(Element&).
    template<typename Visitor,
             typename Element,
             typename = void>
    struct visit_result
    {};

    // visit_result (well-formed case)
    //   trait: specialization for when visit() is callable.
    template<typename Visitor,
             typename Element>
    struct visit_result<Visitor, Element, void_t<
        decltype(std::declval<Visitor>().visit(std::declval<Element&>()))
    >>
    {
        using type = decltype(
            std::declval<Visitor>().visit(std::declval<Element&>()));
    };

    // visit_result_t
    //   type: convenience alias for visit_result<...>::type.
    template<typename Visitor,
             typename Element>
    using visit_result_t = typename visit_result<Visitor, Element>::type;

NS_END  // internal

// -----------------------------------------------------------------------------
// static_visitor
// -----------------------------------------------------------------------------

// static_visitor
//   class: CRTP base for compile-time visitors. Derived must implement
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
template<typename Derived>
class static_visitor
{
public:
    template<typename Element>
    auto apply(Element& _element)
        -> internal::visit_result_t<Derived, Element>
    {
        return static_cast<Derived*>(this)->visit(_element);
    }

    template<typename Element>
    auto apply(const Element& _element)
        -> internal::visit_result_t<Derived, const Element>
    {
        return static_cast<Derived*>(this)->visit(_element);
    }

    template<typename Element>
    auto apply(const Element& _element) const
        -> internal::visit_result_t<const Derived, const Element>
    {
        return static_cast<const Derived*>(this)->visit(_element);
    }
};

// -----------------------------------------------------------------------------
// static_visitable
// -----------------------------------------------------------------------------

// static_visitable
//   class: CRTP base for elements that accept static visitors. Derived
// is the concrete element type. Provides accept() which forwards to the
// visitor's apply() method.
template<typename Derived>
class static_visitable
{
public:
    template<typename Visitor>
    auto accept(Visitor& _visitor)
        -> internal::visit_result_t<Visitor, Derived>
    {
        return _visitor.apply(static_cast<Derived&>(*this));
    }

    template<typename Visitor>
    auto accept(Visitor& _visitor) const
        -> internal::visit_result_t<Visitor, const Derived>
    {
        return _visitor.apply(static_cast<const Derived&>(*this));
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
template<typename... Fns>
struct overloaded : Fns...
{
    using Fns::operator()...;
};

#if D_VISITOR_HAS_DEDUCTION_GUIDES
    // overloaded deduction guide
    //   guide: deduces template arguments from constructor arguments.
    template<typename... Fns>
    overloaded(Fns...) -> overloaded<Fns...>;
#endif

// -----------------------------------------------------------------------------
// make_visitor
// -----------------------------------------------------------------------------

// make_visitor
//   function: factory for overloaded lambda visitors. Equivalent to
// constructing overloaded{...} but available as a function call for
// contexts where CTAD is unavailable or undesirable.
template<typename... Fns>
D_CONSTEXPR_INLINE overloaded<typename std::decay<Fns>::type...>
make_visitor(
    Fns&&... _fns
)
{
    return overloaded<typename std::decay<Fns>::type...>{
        std::forward<Fns>(_fns)...};
}

// -----------------------------------------------------------------------------
// variant_visit
// -----------------------------------------------------------------------------

// variant_visit
//   function: applies a visitor (overload set) to a variant. Thin
// wrapper around std::visit for naming consistency.
template<typename Visitor,
         typename Variant>
D_CONSTEXPR_INLINE decltype(auto)
variant_visit(
    Visitor&& _visitor,
    Variant&& _variant
)
{
    return std::visit(std::forward<Visitor>(_visitor),
                      std::forward<Variant>(_variant));
}

// variant_visit (multi-variant)
//   function: applies a visitor to multiple variants simultaneously.
// Enables multi-dispatch over variant types.
template<typename    Visitor,
         typename... Variants>
D_CONSTEXPR_INLINE decltype(auto)
variant_visit(
    Visitor&&    _visitor,
    Variants&&... _variants
)
{
    return std::visit(std::forward<Visitor>(_visitor),
                      std::forward<Variants>(_variants)...);
}

NS_INTERNAL

    // variant_visit_with_index_helper
    //   function: internal helper that wraps each variant alternative
    // dispatch to include the runtime index as a compile-time constant.
    template<typename Visitor,
             typename Variant,
             std::size_t... Is>
    D_CONSTEXPR_INLINE decltype(auto)
    variant_visit_with_index_impl(
        Visitor&&          _visitor,
        Variant&&          _variant,
        re_std::index_sequence<Is...>
    )
    {
        using return_type = typename std::common_type<
            decltype(_visitor(
                std::integral_constant<std::size_t, Is>{},
                std::get<Is>(std::forward<Variant>(_variant))))...
        >::type;

        using dispatch_fn = return_type(*)(
            Visitor&&, Variant&&);

        // dispatch table.  NOT `static`: a variable of static storage duration
        // is not permitted in a constexpr function before C++23, and this
        // function is D_CONSTEXPR_INLINE -- so the `static` spelling made the
        // whole header fail to compile at C++17 and C++20, the very dialects the
        // variant visitor exists for.
        constexpr dispatch_fn table[] =
        {
            [](
                Visitor&& _v,
                Variant&& _var
            ) -> return_type
            {
                return _v(
                    std::integral_constant<std::size_t, Is>{},
                    std::get<Is>(std::forward<Variant>(_var)));
            }...
        };

        return table[_variant.index()](
            std::forward<Visitor>(_visitor),
            std::forward<Variant>(_variant));
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
template<typename Visitor,
         typename Variant>
D_CONSTEXPR_INLINE decltype(auto)
variant_visit_with_index(
    Visitor&& _visitor,
    Variant&& _variant
)
{
    return internal::variant_visit_with_index_impl(
        std::forward<Visitor>(_visitor),
        std::forward<Variant>(_variant),
        re_std::make_index_sequence<
            std::variant_size<typename std::remove_reference<Variant>::type>::value>{});
}


#endif  // D_VISITOR_HAS_VARIANT


///////////////////////////////////////////////////////////////////////////////
///            VI.   CONCEPT-CONSTRAINED VISITOR (C++20+)                   ///
///////////////////////////////////////////////////////////////////////////////

#if D_VISITOR_HAS_CONCEPTS

// visitable_type
//   concept: constrains types that expose an accept() method taking a
// reference to a visitor. Matches both classic and acyclic visitables.
template<typename T,
         typename Visitor>
concept visitable_type = requires(T _t, Visitor& _v)
{
    _t.accept(_v);
};

// visitor_for
//   concept: constrains a visitor type that can visit all of the given
// element types. Each element must be callable via visit().
template<typename    Visitor,
         typename... Elements>
concept visitor_for =
    (requires(Visitor& _v, Elements& _e) { _v.visit(_e); } && ...);

// acyclic_visitor_for
//   concept: constrains a type that is both an acyclic_visitor and
// provides visitor_of<T> interfaces for all specified element types.
template<typename    Visitor,
         typename... Elements>
concept acyclic_visitor_for =
    ( std::derived_from<Visitor, acyclic_visitor> &&
      (std::derived_from<Visitor, visitor_of<Elements>>  && ...) );

// constrained_accept
//   function: accept() that statically verifies the visitor handles
// the element type. Provides a clear compile error when a visitor
// is missing a required visit() overload.
template<typename Element,
         typename Visitor>
    requires visitor_for<Visitor, Element>
auto constrained_accept(
    Element& _element,
    Visitor& _visitor
)
    -> decltype(_visitor.visit(_element))
{
    return _visitor.visit(_element);
}

#endif  // D_VISITOR_HAS_CONCEPTS


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARADIGM_VISITOR_VISITOR_HPP
