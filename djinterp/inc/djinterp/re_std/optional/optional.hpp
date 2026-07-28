/******************************************************************************
* djinterp [restd]                                                  optional.hpp
*
* optional<T> class:
*   Type-safe nullable value container. Either holds an instance of T
* ("engaged") or is "disengaged" (no value). Mirrors std::optional from
* C++17 in interface and semantics.
*
*   ARCHITECTURE:
*   Three-layer base class structure:
*     1. internal::optional_storage_base<T, IsTriviallyDestructible>
*        - holds the union and engaged flag; partial specialization on
*          a trivial T gets a defaulted destructor so that
*          optional<TrivialT> is itself trivially destructible.
*     2. internal::optional_ops_base<T>
*        - shared management code: construct_value, destroy_value,
*          construct/assign-from-other helpers. Inherits from storage.
*     3. optional<T>
*        - public-facing class with all constructors/assignments/
*          observers/modifiers/comparison operators. Inherits from
*          ops_base.
*
*   The trivial-dtor optimization is the only triviality propagated in
* this implementation; trivial-copy and trivial-move propagation are
* deferred to a future polish batch (each requires another tower of
* conditional base classes).
*
*   STANDARD STATUS:
*   Introduced in C++17. restd provides on C++11+. Lower tiers omit the
* entire optional module (no fallback shimmed; would require mimicking
* unrestricted unions via aligned_storage + manual lifetime, which is
* a substantial amount of additional code for a tier where users have
* mature alternatives like Boost.Optional).
*
*   CONSTEXPR:
*   Limited constexpr coverage on the C++11 floor: default ctor,
* nullopt ctor, has_value, operator bool, plus pointer-style accessors
* on engaged-and-trivial T. Constructors that run user code, assignment
* operators, the destructor, and modifiers are not constexpr.
*
*   EXCEPTION POLICY:
*   When <exception> is available, value() on a disengaged optional
* throws bad_optional_access. When unavailable, value() on disengaged
* is undefined behavior (matches the policy used by any_cast). Pointer-
* style access (operator*, operator->) is always UB on disengaged
* optionals, matching the std contract regardless of exception
* availability.
*
*   PORTABILITY:
*   Available on C++11 and later. Requires unrestricted unions
* (C++11), placement new (C++98 via <new>), and rvalue references
* (C++11) at minimum. Conditional support for trivially-destructible
* T also requires is_trivially_destructible from the type_traits
* module.
*
*   NOT YET IMPLEMENTED:
*   - Trivial copy/move propagation.
*   - C++23 monadic operations (and_then, transform, or_else).
*   - C++20 spaceship operator and three-way comparison.
*   - std::hash<optional<T>> equivalent (hash story not yet ported).
*
*
* path:      /inc/djinterp/restd/optional/optional.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_OPTIONAL_OPTIONAL_
#define DJINTERP_RESTD_OPTIONAL_OPTIONAL_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if    D_ENV_LANG_IS_CPP11_OR_HIGHER \
    && D_ENV_CPP98_HAS_NEW

#include <new>                  // placement new
#include <initializer_list>     // std::initializer_list

// restd
#include "./nullopt.hpp"
#include "./bad_optional_access.hpp"
#include "../utility/in_place.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_same.hpp"
#include "../type_traits/is_void.hpp"
#include "../type_traits/is_convertible.hpp"
#include "../type_traits/is_constructible.hpp"
#include "../type_traits/is_assignable.hpp"
#include "../type_traits/is_nothrow_move_constructible.hpp"
#include "../type_traits/is_nothrow_move_assignable.hpp"
#include "../type_traits/is_trivially_destructible.hpp"
#include "../type_traits/decay.hpp"
#include "../type_traits/remove_cv.hpp"
#include "../type_traits/remove_reference.hpp"
#include "../utility/declval.hpp"
#include "../utility/swap.hpp"


NS_RESTD


    // forward declaration so the conversion constructors / converting
    // constructors from optional<U> can name optional<U>.
    template<typename _T>
    class optional;


    NS_INTERNAL

        // optional_storage_base
        //   class: layer 1 -- holds the union and the engaged flag.
        //          Primary template assumes T is NOT trivially
        //          destructible: provides an explicit destructor that
        //          destroys m_value when engaged.
        //
        //          The dummy char member ensures the union has a
        //          definite active member after default construction
        //          without constructing T.
        template<typename _T,
                 bool _TrivialDtor = is_trivially_destructible<_T>::value>
        struct optional_storage_base
        {
            union
            {
                char m_dummy;
                _T   m_value;
            };
            bool m_engaged;

            // default constructor: disengaged, dummy active.
            D_CONSTEXPR optional_storage_base() D_NOEXCEPT
                : m_dummy(0)
                , m_engaged(false)
            {}

            // tag constructor: in-place construction of m_value from
            // forwarded args. Not constexpr because user code runs.
            template<typename... _Args>
            optional_storage_base(in_place_t, _Args&&... args)
                : m_value(static_cast<_Args&&>(args)...)
                , m_engaged(true)
            {}

            // destructor: destroys m_value if engaged. The conditional
            // call avoids running ~T() on an inactive union member.
            ~optional_storage_base()
            {
                if (m_engaged)
                {
                    m_value.~_T();
                }
            }
        };

        // optional_storage_base<_T, true>
        //   class: specialization for trivially-destructible T. The
        //          destructor is implicitly trivial because none of
        //          the members has a non-trivial destructor (the
        //          union's "destructor" is trivial when all members
        //          are trivially destructible). This is what makes
        //          is_trivially_destructible<optional<int>> true.
        template<typename _T>
        struct optional_storage_base<_T, true>
        {
            union
            {
                char m_dummy;
                _T   m_value;
            };
            bool m_engaged;

            D_CONSTEXPR optional_storage_base() D_NOEXCEPT
                : m_dummy(0)
                , m_engaged(false)
            {}

            template<typename... _Args>
            D_CONSTEXPR optional_storage_base(in_place_t, _Args&&... args)
                : m_value(static_cast<_Args&&>(args)...)
                , m_engaged(true)
            {}

            // no user-declared destructor -- compiler synthesizes a
            // trivial one because every member is trivially destructible.
        };

        // optional_ops_base
        //   class: layer 2 -- shared management code. All the actual
        //          "construct/destroy the value" plumbing lives here.
        //          Inherits from storage_base, which already picked
        //          the right destructor variant for T.
        template<typename _T>
        struct optional_ops_base
            : optional_storage_base<_T>
        {
            typedef optional_storage_base<_T> base_;

            // inherit constructors so optional<T> can pass tag args
            // (in_place_t etc.) through to storage.
            D_CONSTEXPR optional_ops_base() D_NOEXCEPT
                : base_()
            {}

            template<typename... _Args>
            optional_ops_base(in_place_t, _Args&&... args)
                : base_(in_place, static_cast<_Args&&>(args)...)
            {}

            // construct_value
            //   method: in-place construct m_value from forwarded args
            //           (placement new on the union storage). Sets
            //           m_engaged. Caller must ensure the optional is
            //           currently disengaged.
            template<typename... _Args>
            void construct_value(_Args&&... args)
            {
                ::new (static_cast<void*>(&this->m_value))
                    _T(static_cast<_Args&&>(args)...);
                this->m_engaged = true;
            }

            // destroy_value
            //   method: destroy m_value and mark disengaged. Caller
            //           must ensure the optional is currently engaged.
            void destroy_value() D_NOEXCEPT
            {
                this->m_value.~_T();
                this->m_engaged = false;
            }

            // construct_from_other
            //   method: copy- or move-construct m_value from another
            //           optional's m_value. Other must be engaged;
            //           this must be disengaged.
            template<typename _Other>
            void construct_from_other(_Other&& other)
            {
                construct_value(static_cast<_Other&&>(other).m_value);
            }

            // assign_from_other
            //   method: copy- or move-assign m_value from another
            //           optional's m_value. Both this and other are
            //           engaged. Uses native assignment.
            template<typename _Other>
            void assign_from_other(_Other&& other)
            {
                this->m_value = static_cast<_Other&&>(other).m_value;
            }
        };

    NS_END  // internal


    // optional
    //   class: nullable value container. Holds either no value or
    //          an instance of _T.
    template<typename _T>
    class optional
        : private internal::optional_ops_base<_T>
    {
        // the standard's prohibitions, enforced via static_assert for
        // a clearer diagnostic than partial-spec deletion gives.
        static_assert(
            !is_same<typename remove_cv<_T>::type, nullopt_t>::value,
            "optional<T>: T may not be (cv) nullopt_t");
        static_assert(
            !is_same<typename remove_cv<_T>::type, in_place_t>::value,
            "optional<T>: T may not be (cv) in_place_t");
        static_assert(
            !is_void<_T>::value,
            "optional<T>: T may not be cv void");

        typedef internal::optional_ops_base<_T> base_;


    public:

        // ---------------------------------------------------------------
        // member types
        // ---------------------------------------------------------------

        typedef _T value_type;


        // ---------------------------------------------------------------
        // constructors
        // ---------------------------------------------------------------

        // default constructor: disengaged.
        D_CONSTEXPR optional() D_NOEXCEPT
            : base_()
        {}

        // nullopt constructor: disengaged.
        D_CONSTEXPR optional(nullopt_t) D_NOEXCEPT
            : base_()
        {}

        // copy constructor: deep-copies the held value if engaged.
        optional(const optional& other)
            : base_()
        {
            if (other.has_value())
            {
                this->construct_value(other.m_value_ref_());
            }
        }

        // move constructor: move-constructs the held value if engaged.
        // noexcept iff T's move constructor is noexcept.
        optional(optional&& other)
            noexcept(is_nothrow_move_constructible<_T>::value)
            : base_()
        {
            if (other.has_value())
            {
                this->construct_value(
                    static_cast<_T&&>(other.m_value_ref_()));
            }
        }

        // converting copy from optional<U>.
        // SFINAE-constrained to avoid ambiguity with the U-conversion
        // constructor when U == T.
        template<typename _U,
                 typename enable_if<
                     (    is_constructible<_T, const _U&>::value
                       && !is_same<_T, _U>::value ),
                     int>::type = 0>
        optional(const optional<_U>& other)
            : base_()
        {
            if (other.has_value())
            {
                this->construct_value(*other);
            }
        }

        // converting move from optional<U>.
        template<typename _U,
                 typename enable_if<
                     (    is_constructible<_T, _U&&>::value
                       && !is_same<_T, _U>::value ),
                     int>::type = 0>
        optional(optional<_U>&& other)
            : base_()
        {
            if (other.has_value())
            {
                this->construct_value(static_cast<_U&&>(*other));
            }
        }

        // in-place construction from forwarded args.
        template<typename... _Args,
                 typename enable_if<
                     is_constructible<_T, _Args...>::value,
                     int>::type = 0>
        D_CONSTEXPR explicit optional(in_place_t, _Args&&... args)
            : base_(in_place, static_cast<_Args&&>(args)...)
        {}

        // in-place construction with initializer_list.
        template<typename _U,
                 typename... _Args,
                 typename enable_if<
                     is_constructible<
                         _T,
                         std::initializer_list<_U>&,
                         _Args... >::value,
                     int>::type = 0>
        D_CONSTEXPR explicit optional(in_place_t,
                                      std::initializer_list<_U> il,
                                      _Args&&... args)
            : base_(in_place, il, static_cast<_Args&&>(args)...)
        {}

        // conversion constructor from a U value.
        // SFINAE-constrained so it does not capture nullopt_t /
        // in_place_t / optional<X> arguments and does not collide with
        // the copy/move constructors when U decays to T.
        template<typename _U = _T,
                 typename enable_if<
                     (    is_constructible<_T, _U&&>::value
                       && !is_same<
                              typename decay<_U>::type,
                              in_place_t>::value
                       && !is_same<
                              typename decay<_U>::type,
                              optional>::value ),
                     int>::type = 0>
        D_CONSTEXPR optional(_U&& value)
            : base_(in_place, static_cast<_U&&>(value))
        {}


        // ---------------------------------------------------------------
        // destructor
        // ---------------------------------------------------------------
        // implicitly defined -- inherited via base_/storage_base, which
        // picks the trivial vs nontrivial variant based on T.


        // ---------------------------------------------------------------
        // assignment
        // ---------------------------------------------------------------

        // nullopt assignment: disengage if engaged.
        optional& operator=(nullopt_t) D_NOEXCEPT
        {
            if (this->has_value())
            {
                this->destroy_value();
            }
            return *this;
        }

        // copy assignment.
        optional& operator=(const optional& other)
        {
            if (other.has_value())
            {
                if (this->has_value())
                {
                    this->assign_from_other(other);
                }
                else
                {
                    this->construct_value(other.m_value_ref_());
                }
            }
            else
            {
                if (this->has_value())
                {
                    this->destroy_value();
                }
            }
            return *this;
        }

        // move assignment. noexcept iff T's move ctor and move assign
        // are both noexcept.
        optional& operator=(optional&& other)
            noexcept(    is_nothrow_move_constructible<_T>::value
                      && is_nothrow_move_assignable<_T>::value )
        {
            if (other.has_value())
            {
                if (this->has_value())
                {
                    this->assign_from_other(static_cast<optional&&>(other));
                }
                else
                {
                    this->construct_value(
                        static_cast<_T&&>(other.m_value_ref_()));
                }
            }
            else
            {
                if (this->has_value())
                {
                    this->destroy_value();
                }
            }
            return *this;
        }

        // U-value assignment with the standard's SFINAE constraints:
        // U must not decay to optional, must be constructible AND
        // assignable to T, and must not be a scalar-self case (the
        // U == T scalar self-assignment goes through copy/move
        // assignment).
        template<typename _U = _T,
                 typename enable_if<
                     (    !is_same<
                              typename decay<_U>::type,
                              optional>::value
                       && is_constructible<_T, _U>::value
                       && is_assignable<_T&, _U>::value ),
                     int>::type = 0>
        optional& operator=(_U&& value)
        {
            if (this->has_value())
            {
                this->m_value_ref_() = static_cast<_U&&>(value);
            }
            else
            {
                this->construct_value(static_cast<_U&&>(value));
            }
            return *this;
        }

        // converting copy assignment from optional<U>.
        template<typename _U,
                 typename enable_if<
                     (    !is_same<_T, _U>::value
                       && is_constructible<_T, const _U&>::value
                       && is_assignable<_T&, const _U&>::value ),
                     int>::type = 0>
        optional& operator=(const optional<_U>& other)
        {
            if (other.has_value())
            {
                if (this->has_value())
                {
                    this->m_value_ref_() = *other;
                }
                else
                {
                    this->construct_value(*other);
                }
            }
            else
            {
                if (this->has_value())
                {
                    this->destroy_value();
                }
            }
            return *this;
        }

        // converting move assignment from optional<U>.
        template<typename _U,
                 typename enable_if<
                     (    !is_same<_T, _U>::value
                       && is_constructible<_T, _U>::value
                       && is_assignable<_T&, _U>::value ),
                     int>::type = 0>
        optional& operator=(optional<_U>&& other)
        {
            if (other.has_value())
            {
                if (this->has_value())
                {
                    this->m_value_ref_() = static_cast<_U&&>(*other);
                }
                else
                {
                    this->construct_value(static_cast<_U&&>(*other));
                }
            }
            else
            {
                if (this->has_value())
                {
                    this->destroy_value();
                }
            }
            return *this;
        }


        // ---------------------------------------------------------------
        // observers
        // ---------------------------------------------------------------

        D_CONSTEXPR bool has_value() const D_NOEXCEPT
        { return this->m_engaged; }

        D_CONSTEXPR explicit operator bool() const D_NOEXCEPT
        { return this->m_engaged; }

        // operator-> : UB if disengaged.
        _T*       operator->()       D_NOEXCEPT { return &this->m_value; }
        const _T* operator->() const D_NOEXCEPT { return &this->m_value; }

        // operator* : UB if disengaged. Four reference-qualified
        // overloads to preserve value category.
        _T&        operator*()       &  D_NOEXCEPT { return this->m_value; }
        const _T&  operator*() const &  D_NOEXCEPT { return this->m_value; }
        _T&&       operator*()       && D_NOEXCEPT
        { return static_cast<_T&&>(this->m_value); }
        const _T&& operator*() const && D_NOEXCEPT
        { return static_cast<const _T&&>(this->m_value); }

        // value() : throws bad_optional_access if disengaged (when
        // exceptions are available); otherwise UB on disengaged.
        _T& value() &
        {
            if (!this->has_value())
            {
                throw_bad_access_();
            }
            return this->m_value;
        }

        const _T& value() const &
        {
            if (!this->has_value())
            {
                throw_bad_access_();
            }
            return this->m_value;
        }

        _T&& value() &&
        {
            if (!this->has_value())
            {
                throw_bad_access_();
            }
            return static_cast<_T&&>(this->m_value);
        }

        const _T&& value() const &&
        {
            if (!this->has_value())
            {
                throw_bad_access_();
            }
            return static_cast<const _T&&>(this->m_value);
        }

        // value_or: returns the held value if engaged, else the
        // forwarded default. T must be copy-constructible (lvalue
        // overload) or move-constructible (rvalue overload), and U
        // must be convertible to T.
        template<typename _U>
        _T value_or(_U&& default_value) const &
        {
            return this->has_value()
                ? this->m_value
                : static_cast<_T>(static_cast<_U&&>(default_value));
        }

        template<typename _U>
        _T value_or(_U&& default_value) &&
        {
            return this->has_value()
                ? static_cast<_T&&>(this->m_value)
                : static_cast<_T>(static_cast<_U&&>(default_value));
        }


        // ---------------------------------------------------------------
        // modifiers
        // ---------------------------------------------------------------

        // reset: disengage. No-op if already disengaged.
        void reset() D_NOEXCEPT
        {
            if (this->has_value())
            {
                this->destroy_value();
            }
        }

        // emplace: destroy any held value, then in-place construct a
        // new one from forwarded args. Returns a reference to the new
        // value.
        template<typename... _Args>
        _T& emplace(_Args&&... args)
        {
            if (this->has_value())
            {
                this->destroy_value();
            }
            this->construct_value(static_cast<_Args&&>(args)...);
            return this->m_value;
        }

        // emplace with initializer_list.
        template<typename _U, typename... _Args>
        _T& emplace(std::initializer_list<_U> il, _Args&&... args)
        {
            if (this->has_value())
            {
                this->destroy_value();
            }
            this->construct_value(il, static_cast<_Args&&>(args)...);
            return this->m_value;
        }

        // member swap: see optional_swap.hpp for the free function.
        //
        // The noexcept specification is conservatively just
        // is_nothrow_move_constructible<T>::value. The standard's
        // precise spec also includes is_nothrow_swappable<T>, but
        // probing that here would require a using-declaration on
        // restd::swap inside the noexcept operand, which is awkward
        // outside a dedicated detection namespace. Dropping the
        // swappable arm slightly under-promises noexcept for types
        // that have nothrow-swap but throwing-move (rare); it never
        // wrongly promises noexcept for a type whose swap can throw,
        // which is the only direction that matters for callers.
        void swap(optional& other)
            noexcept(is_nothrow_move_constructible<_T>::value)
        {
            const bool a = this->has_value();
            const bool b = other.has_value();

            if (a && b)
            {
                // Bring restd::swap into scope so unqualified swap
                // here picks up both ADL and the restd fallback,
                // matching the std swap protocol.
                using restd::swap;
                swap(this->m_value, other.m_value);
            }
            else if (a && !b)
            {
                other.construct_value(static_cast<_T&&>(this->m_value));
                this->destroy_value();
            }
            else if (!a && b)
            {
                this->construct_value(static_cast<_T&&>(other.m_value));
                other.destroy_value();
            }
            // !a && !b: nothing to do.
        }


    private:

        // ---------------------------------------------------------------
        // internal helpers
        // ---------------------------------------------------------------

        // m_value_ref_
        //   helper: returns a non-const reference to the held value.
        //           Used by methods that mutate m_value through what
        //           would otherwise be a const-correctness obstacle
        //           (the converting copy ctor reading other.m_value).
        //
        //           Pre: this->m_engaged == true.
        _T& m_value_ref_() D_NOEXCEPT
        { return this->m_value; }

        const _T& m_value_ref_() const D_NOEXCEPT
        { return this->m_value; }

        // throw_bad_access_
        //   helper: factored out so the inline value() overloads stay
        //           small. When exceptions are available, throws
        //           bad_optional_access; otherwise the function
        //           returns normally and the caller proceeds with
        //           UB on the disengaged value() access. We
        //           intentionally do not abort -- this matches the
        //           any_cast policy for builds without exceptions.
        static void throw_bad_access_()
        {
        #if D_ENV_CPP98_HAS_EXCEPTION
            throw bad_optional_access();
        #else
            // no exceptions available; UB on disengaged value()
            // access. Callers that build without exceptions are
            // expected to ensure has_value() before calling value().
        #endif
        }


        // friendship for the converting constructors of optional<U>
        // when they need to read this->m_value. Each instantiation of
        // optional is friends with every other.
        template<typename _OtherT>
        friend class optional;

    };  // class optional


    // ===================================================================
    // free function comparisons
    // ===================================================================
    //
    // Six operators (==, !=, <, <=, >, >=) across three pairings:
    //   - optional<T> vs optional<U>     (heterogeneous-comparison)
    //   - optional<T> vs nullopt_t       (both directions)
    //   - optional<T> vs U value         (both directions)
    //
    // Per the standard, optional<T> vs optional<U> is well-formed iff
    // the corresponding T-vs-U comparison is well-formed; we forward
    // directly so the user gets a clean compile error if the
    // underlying comparison is missing.
    // ===================================================================

    // --- optional<T> vs optional<U> -----------------------------------

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator==(const optional<_T>& a, const optional<_U>& b)
    {
        return    bool(a) == bool(b)
               && (!bool(a) || *a == *b);
    }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator!=(const optional<_T>& a, const optional<_U>& b)
    {
        return    bool(a) != bool(b)
               || (bool(a) && *a != *b);
    }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator<(const optional<_T>& a, const optional<_U>& b)
    {
        // disengaged < anything-engaged; engaged < engaged by *.
        return bool(b) && (!bool(a) || *a < *b);
    }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator>(const optional<_T>& a, const optional<_U>& b)
    {
        return bool(a) && (!bool(b) || *a > *b);
    }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator<=(const optional<_T>& a, const optional<_U>& b)
    {
        return !bool(a) || (bool(b) && *a <= *b);
    }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator>=(const optional<_T>& a, const optional<_U>& b)
    {
        return !bool(b) || (bool(a) && *a >= *b);
    }

    // --- optional<T> vs nullopt_t -------------------------------------

    template<typename _T>
    D_CONSTEXPR bool operator==(const optional<_T>& a, nullopt_t) D_NOEXCEPT
    { return !bool(a); }

    template<typename _T>
    D_CONSTEXPR bool operator==(nullopt_t, const optional<_T>& a) D_NOEXCEPT
    { return !bool(a); }

    template<typename _T>
    D_CONSTEXPR bool operator!=(const optional<_T>& a, nullopt_t) D_NOEXCEPT
    { return bool(a); }

    template<typename _T>
    D_CONSTEXPR bool operator!=(nullopt_t, const optional<_T>& a) D_NOEXCEPT
    { return bool(a); }

    template<typename _T>
    D_CONSTEXPR bool operator<(const optional<_T>&, nullopt_t) D_NOEXCEPT
    { return false; }    // nothing < disengaged

    template<typename _T>
    D_CONSTEXPR bool operator<(nullopt_t, const optional<_T>& a) D_NOEXCEPT
    { return bool(a); }

    template<typename _T>
    D_CONSTEXPR bool operator>(const optional<_T>& a, nullopt_t) D_NOEXCEPT
    { return bool(a); }

    template<typename _T>
    D_CONSTEXPR bool operator>(nullopt_t, const optional<_T>&) D_NOEXCEPT
    { return false; }

    template<typename _T>
    D_CONSTEXPR bool operator<=(const optional<_T>& a, nullopt_t) D_NOEXCEPT
    { return !bool(a); }

    template<typename _T>
    D_CONSTEXPR bool operator<=(nullopt_t, const optional<_T>&) D_NOEXCEPT
    { return true; }

    template<typename _T>
    D_CONSTEXPR bool operator>=(const optional<_T>&, nullopt_t) D_NOEXCEPT
    { return true; }

    template<typename _T>
    D_CONSTEXPR bool operator>=(nullopt_t, const optional<_T>& a) D_NOEXCEPT
    { return !bool(a); }

    // --- optional<T> vs U value ---------------------------------------

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator==(const optional<_T>& a, const _U& v)
    { return bool(a) && (*a == v); }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator==(const _U& v, const optional<_T>& a)
    { return bool(a) && (v == *a); }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator!=(const optional<_T>& a, const _U& v)
    { return !bool(a) || (*a != v); }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator!=(const _U& v, const optional<_T>& a)
    { return !bool(a) || (v != *a); }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator<(const optional<_T>& a, const _U& v)
    { return !bool(a) || (*a < v); }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator<(const _U& v, const optional<_T>& a)
    { return bool(a) && (v < *a); }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator>(const optional<_T>& a, const _U& v)
    { return bool(a) && (*a > v); }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator>(const _U& v, const optional<_T>& a)
    { return !bool(a) || (v > *a); }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator<=(const optional<_T>& a, const _U& v)
    { return !bool(a) || (*a <= v); }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator<=(const _U& v, const optional<_T>& a)
    { return bool(a) && (v <= *a); }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator>=(const optional<_T>& a, const _U& v)
    { return bool(a) && (*a >= v); }

    template<typename _T, typename _U>
    D_CONSTEXPR bool operator>=(const _U& v, const optional<_T>& a)
    { return !bool(a) || (v >= *a); }


NS_END  // restd


#endif  // CPP11+ && HAS_NEW

#endif  // DJINTERP_RESTD_OPTIONAL_OPTIONAL_
