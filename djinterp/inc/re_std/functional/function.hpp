/*******************************************************************************
* djinterp [re_std]                                                 function.hpp
*
* function class header:
* class: type-erased owning callable wrapper -- re_std's portable
*   alternative to `std::function` (C++11).
*   `function<R(Args...)>` stores any CopyConstructible callable that is
* invocable as `R(Args...)` (the result being convertible to `R`, or
* `R` being `void`), erasing its concrete type behind a small virtual
* dispatch table. Calling an empty wrapper throws `bad_function_call`.
* Target dispatch is delegated wholesale to `re_std::invoke`, so
* pointer-to-member-function, pointer-to-member-data, `reference_wrapper`,
* function pointers, lambdas, and arbitrary function objects are all
* handled uniformly and for free.
*
*   Storage strategy: the target is held in a heap-allocated holder
* reached through an abstract base (`internal::fn_base`). This mirrors
* the ops-table type erasure used by `any`'s heap path. A small-buffer
* optimisation (in-place storage for small/trivial targets, as libstdc++
* and libc++ do) is a deliberate follow-on -- see the note below -- not
* a correctness requirement; the heap path is always correct.
*
*   Type identity for `target<T>()` is RTTI-free: it uses the
* address-of-a-static-template-member scheme (`internal::fn_type_id_of`),
* exactly as `any` derives `any_type_id`, so `target<T>()` works even
* when `<typeinfo>` is unreachable. The std-parity `target_type()`
* observer (which must return `const std::type_info&`) is additionally
* gated on `RE_STD_HAS_RTTI`.
*
*   Min standard: C++98, in two tiers. With variadic templates and rvalue
* references (C++11) `function` is the class above, spelling `R(Args...)`
* and forwarding through `invoke`. Below them it is the Boost.Function-
* style fixed-arity form, a specialisation for each arity from 0 to 6
* over a shared owner, calling its target with the call syntax (no
* pointer to member there) and passing arguments as the signature
* declares them; moves, nullptr and operator bool, which C++98 cannot
* spell, are absent. Both tiers have `empty()`, the named test decision
* 3.2 puts at every level (Boost.Function's, not std's). `function` is
* never constexpr (heap allocation and, where present, RTTI), matching
* std.
*
*   Deviations from `std::function`: the (deprecated in C++11, removed in
* C++17) allocator-taking constructors and `assign(f, alloc)` are not
* provided. `operator==`/`operator!=` against `nullptr_t` are provided
* for parity even though std deprecated them in C++20.
*
*
* path:      /inc/re_std/functional/function.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.07.25
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_FUNCTION_HPP
#define RE_STD_FUNCTIONAL_FUNCTION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if (RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&  \
     RE_STD_LANG_HAS_RVALUE_REFERENCES)

#include "re_std/type_traits/type_traits.hpp"
#include "re_std/utility/forward.hpp"
#include "re_std/functional/invoke.hpp"
#include "re_std/functional/bad_function_call.hpp"

// std
#include <cstddef>   // std::nullptr_t

#if !RE_STD_HAS_EXCEPTIONS
    // std
    #include <cstdlib>  // std::abort, an empty call's only outcome
#endif

#if RE_STD_HAS_RTTI
    // std
    #include <typeinfo>
#endif

namespace re_std
{

// function
//   class: primary template, intentionally undefined. Only a genuine
// function type `R(Args...)` names a valid specialisation, so
// `function<int>` is ill-formed -- matching std.
template<typename Signature>
class function;

namespace internal
{

    // fn_declval
    //   function: declval-style helper. Declared, never defined; usable
    // only in unevaluated contexts. Never instantiated with `void` here
    // (every use is a callable or a function-parameter type).
    template<typename Type>
    Type&& fn_declval();

    // fn_type_id / fn_type_id_of
    //   alias: typedef + function: RTTI-free per-type identity. The address of a
    // distinct static member is unique per `Type`, giving a stable,
    // constexpr-address token usable for `target<T>()` comparisons with
    // zero dependence on `<typeinfo>`. Mirrors `any_type_id`.
    typedef const void* fn_type_id;

    template<typename Type>
    struct fn_type_tag
    {
        static const char s_id;
    };

    template<typename Type>
    const char fn_type_tag<Type>::s_id = 0;

    template<typename Type>
    fn_type_id
    fn_type_id_of()
    {
        return &fn_type_tag<Type>::s_id;
    }

    // fn_conv
    //   struct: trait helper: SFINAE probe for "a prvalue of `From` is
    // convertible to `To`". `accept(To)` participates only when the
    // conversion is well-formed.
    template<typename To>
    struct fn_conv
    {
        static void accept(To);

        template<typename From>
        static true_type
        probe(decltype(accept(fn_declval<From>()))*);

        template<typename From>
        static false_type
        probe(...);
    };

    // fn_convertible
    //   trait: `true` if `From` is convertible to `To`. `To == void`
    // is always satisfiable (any result is discardable).
    template<typename To, typename From>
    struct fn_convertible
        : integral_constant<bool,
              is_same<decltype(fn_conv<To>::template probe<From>(0)),
                      true_type>::value>
    {};

    template<typename From>
    struct fn_convertible<void, From>
        : true_type
    {};

    // fn_callable
    //   trait: `true` if `invoke(f, args...)` is well-formed for an
    // lvalue `Fn` and the given `Args`. The result is cast to `void`
    // inside the probe so that reference-returning callables (whose
    // result type cannot be pointer-formed) are still detected.
    template<typename Fn, typename... Args>
    struct fn_callable
    {
        template<typename F>
        static true_type
        probe(int,
              decltype((void)re_std::invoke(fn_declval<F&>(),
                                           fn_declval<Args>()...))* = 0);

        template<typename F>
        static false_type
        probe(...);

        static const bool value =
            is_same<decltype(probe<Fn>(0)), true_type>::value;
    };

    // fn_invocable_r_impl
    //   trait: two-step so the result-type `decltype` is only formed when
    // the call is actually well-formed (guarding against a hard error in
    // the non-callable case).
    template<bool Callable, typename Ret, typename Fn, typename... Args>
    struct fn_invocable_r_impl
    {
        static const bool value = false;
    };

    template<typename Ret, typename Fn, typename... Args>
    struct fn_invocable_r_impl<true, Ret, Fn, Args...>
    {
        static const bool value = fn_convertible<
            Ret,
            decltype(re_std::invoke(fn_declval<Fn&>(),
                                   fn_declval<Args>()...))
        >::value;
    };

    // fn_invocable_r
    //   trait: `true` if an lvalue `Fn` is invocable per `Ret(Args...)`
    // with the result convertible to `Ret` (or `Ret` == void). This is
    // the local stand-in for `is_invocable_r` (which is a follow-on to
    // this milestone in re_std::type_traits).
    template<typename Ret, typename Fn, typename... Args>
    struct fn_invocable_r
        : integral_constant<bool,
              fn_invocable_r_impl<
                  fn_callable<Fn, Args...>::value,
                  Ret, Fn, Args...>::value>
    {};

    // fn_is_null
    //   function: detects a null function pointer / null pointer-to-member
    // target (`f == 0`), which std maps to an *empty* wrapper. The SFINAE
    // is type-based (it never names the runtime parameter), so callables
    // for which `== 0` is ill-formed (lambdas, functors) fall through to
    // the `...` overload and are treated as non-null.
    //   A callable passed *by name* deduces to a function type (or a
    // reference to one); its address is never null, so it is excluded via
    // the `!is_function` guard -- both because the comparison would be a
    // pointless always-false test and to avoid a spurious
    // `-Wnonnull-compare` diagnostic. Genuine null pointers still reach the
    // wrapper as pointer *variables*, which are checked.
    //   The leading `int` / `...` parameter ranks the two overloads so the
    // call is unambiguous: with the `0` argument the `int` overload is
    // preferred whenever its SFINAE succeeds, otherwise the `...` overload
    // is the fallback. (Without a supplied argument to discriminate on,
    // an omitted-defaulted parameter and an ellipsis tie.)
    template<typename Fn,
             typename = typename enable_if<
                 !is_function<typename remove_reference<Fn>::type>::value
             >::type>
    bool
    fn_is_null(const Fn& _f, int,
               decltype((void)(fn_declval<const Fn&>() == 0), 0)* = 0)
    {
        return _f == 0;
    }

    template<typename Fn>
    bool
    fn_is_null(const Fn&, ...)
    {
        return false;
    }

    // fn_call_impl
    //   trait: performs the actual invoke, discarding the result when
    // `Ret` is `void` (C++11 has no `if constexpr` to branch inline).
    template<typename Ret>
    struct fn_call_impl
    {
        template<typename Fd, typename... A>
        static Ret
        call(Fd& _f, A&&... _a)
        {
            return re_std::invoke(_f, re_std::forward<A>(_a)...);
        }
    };

    template<>
    struct fn_call_impl<void>
    {
        template<typename Fd, typename... A>
        static void
        call(Fd& _f, A&&... _a)
        {
            re_std::invoke(_f, re_std::forward<A>(_a)...);
        }
    };

    // fn_base
    //   struct: abstract type-erasure interface for a stored target.
    template<typename Ret, typename... Args>
    struct fn_base
    {
        virtual ~fn_base() {}

        virtual Ret        do_call(Args...) = 0;
        virtual fn_base*    clone() const     = 0;
        virtual fn_type_id  type_id() const   = 0;
        virtual void*       target_ptr()      = 0;

#if RE_STD_HAS_RTTI
        virtual const std::type_info& type_info() const = 0;
#endif
    };

    // fn_holder
    //   struct: concrete holder storing the decayed target by value.
    template<typename Fd, typename Ret, typename... Args>
    struct fn_holder
        : fn_base<Ret, Args...>
    {
        Fd m_f;

        template<typename G>
        explicit fn_holder(G&& _g)
            : m_f(re_std::forward<G>(_g))
        {}

        Ret do_call(Args... _a)
        {
            return fn_call_impl<Ret>::call(
                m_f, re_std::forward<Args>(_a)...);
        }

        fn_base<Ret, Args...>* clone() const
        {
            return new fn_holder(m_f);
        }

        fn_type_id type_id() const
        {
            return fn_type_id_of<Fd>();
        }

        void* target_ptr()
        {
            return static_cast<void*>(&m_f);
        }

#if RE_STD_HAS_RTTI
        const std::type_info& type_info() const
        {
            return typeid(Fd);
        }
#endif
    };

}  // internal

// function<Ret(Args...)>
//   class: the type-erased callable wrapper. See the file header for the
// storage model, type-identity scheme, and deviations from std.
template<typename Ret, typename... Args>
class function<Ret(Args...)>
{
public:

#if !RE_STD_LANG_IS_CPP20_OR_HIGHER
    // typedef: legacy member; present through C++17, removed in C++20
    // (matches std::function).
    typedef Ret result_type;
#endif

    // ---- construction (empty) -------------------------------------------

    // function
    //   function: constructs an empty wrapper.
    function() noexcept
        : m_ptr(0)
    {}

    // function
    //   function: constructs an empty wrapper from `nullptr`.
    function(std::nullptr_t) noexcept
        : m_ptr(0)
    {}

    // ---- construction (copy / move) -------------------------------------

    // function
    //   function: deep-copies the target (requires a CopyConstructible
    // target, as std does).
    function(const function& _other)
        : m_ptr(_other.m_ptr ? _other.m_ptr->clone() : 0)
    {}

    // function
    //   function: steals the target; leaves `_other` empty.
    function(function&& _other) noexcept
        : m_ptr(_other.m_ptr)
    {
        _other.m_ptr = 0;
    }

    // ---- construction (from a callable) ---------------------------------

    // function
    //   function: wraps any callable invocable as `Ret(Args...)`. Excluded
    // for `function` itself (so copy/move win) and for non-invocable
    // types (SFINAE). A null function/member pointer yields an empty
    // wrapper, matching std.
    template<typename Fn,
             typename = typename enable_if<
                 ( !is_same<typename decay<Fn>::type, function>::value &&
                   internal::fn_invocable_r<
                       Ret, typename decay<Fn>::type, Args...>::value )
             >::type>
    function(Fn&& _f)
        : m_ptr(0)
    {
        typedef typename decay<Fn>::type Fd;
        if (!internal::fn_is_null(_f, 0))
        {
            m_ptr = new internal::fn_holder<Fd, Ret, Args...>(
                re_std::forward<Fn>(_f));
        }
    }

    // ---- assignment -----------------------------------------------------

    // operator=
    //   function: copy via copy-and-swap.
    function&
    operator=(const function& _other)
    {
        function(_other).swap(*this);
        return *this;
    }

    // operator=
    //   function: move via swap with a stolen temporary.
    function&
    operator=(function&& _other) noexcept
    {
        function(static_cast<function&&>(_other)).swap(*this);
        return *this;
    }

    // operator=
    //   function: clears the wrapper.
    function&
    operator=(std::nullptr_t) noexcept
    {
        delete m_ptr;
        m_ptr = 0;
        return *this;
    }

    // operator=
    //   function: rebinds to a new callable (same constraints as the
    // callable ctor).
    template<typename Fn>
    typename enable_if<
        ( !is_same<typename decay<Fn>::type, function>::value &&
          internal::fn_invocable_r<
              Ret, typename decay<Fn>::type, Args...>::value ),
        function&
    >::type
    operator=(Fn&& _f)
    {
        function(re_std::forward<Fn>(_f)).swap(*this);
        return *this;
    }

    // ---- destruction ----------------------------------------------------

    ~function()
    {
        delete m_ptr;
    }

    // ---- modifiers ------------------------------------------------------

    // swap
    //   function: O(1) pointer swap.
    void
    swap(function& _other) noexcept
    {
        internal::fn_base<Ret, Args...>* _tmp = m_ptr;
        m_ptr        = _other.m_ptr;
        _other.m_ptr = _tmp;
    }

    // ---- observers ------------------------------------------------------

    // operator bool
    //   function: `true` iff the wrapper holds a target.
    explicit operator bool() const noexcept
    {
        return m_ptr != 0;
    }

    // empty
    //   function: `true` iff the wrapper holds no target. Not std's: the
    // named test decision 3.2 puts at every level, which the C++98 tier
    // has too, where an explicit operator bool cannot be spelled
    // (Boost.Function's empty()).
    bool
    empty() const noexcept
    {
        return m_ptr == 0;
    }

    // operator()
    //   function: invokes the stored target; throws `bad_function_call`
    // when empty.
    Ret
    operator()(Args... _a) const
    {
        // an empty wrapper: throw, as std's does, or with exceptions off
        // abort, as std's does then -- there is no target and no value
        if (!m_ptr)
        {
        #if RE_STD_HAS_EXCEPTIONS
            throw bad_function_call();
        #else
            std::abort();
        #endif
        }
        return m_ptr->do_call(re_std::forward<Args>(_a)...);
    }

#if RE_STD_HAS_RTTI
    // target_type
    //   function: the `type_info` of the stored target, or `typeid(void)`
    // when empty. Only available with `<typeinfo>`.
    const std::type_info&
    target_type() const noexcept
    {
        return m_ptr ? m_ptr->type_info() : typeid(void);
    }
#endif

    // target
    //   function: a pointer to the stored target if it is exactly `Tp`,
    // else null. RTTI-free (uses the address-based type id).
    template<typename Tp>
    Tp*
    target() noexcept
    {
        if (m_ptr && m_ptr->type_id() == internal::fn_type_id_of<Tp>())
        {
            return static_cast<Tp*>(m_ptr->target_ptr());
        }
        return 0;
    }

    // target (const)
    //   function: const overload of the above.
    template<typename Tp>
    const Tp*
    target() const noexcept
    {
        if (m_ptr && m_ptr->type_id() == internal::fn_type_id_of<Tp>())
        {
            return static_cast<const Tp*>(m_ptr->target_ptr());
        }
        return 0;
    }

private:

    internal::fn_base<Ret, Args...>* m_ptr;
};

// ---- non-member swap ----------------------------------------------------

// swap
//   function: exchanges two wrappers; enables the ADL two-step swap.
template<typename Ret, typename... Args>
void
swap(function<Ret(Args...)>& _a, function<Ret(Args...)>& _b) noexcept
{
    _a.swap(_b);
}

// ---- null comparisons (deprecated in std since C++20, kept for parity) --

template<typename Ret, typename... Args>
bool
operator==(const function<Ret(Args...)>& _f, std::nullptr_t) noexcept
{
    return !_f;
}

template<typename Ret, typename... Args>
bool
operator==(std::nullptr_t, const function<Ret(Args...)>& _f) noexcept
{
    return !_f;
}

template<typename Ret, typename... Args>
bool
operator!=(const function<Ret(Args...)>& _f, std::nullptr_t) noexcept
{
    return static_cast<bool>(_f);
}

template<typename Ret, typename... Args>
bool
operator!=(std::nullptr_t, const function<Ret(Args...)>& _f) noexcept
{
    return static_cast<bool>(_f);
}

}  // re_std


#else  // C++98


// std
#include <cstdlib>   // std::abort, where exceptions are off
#if RE_STD_HAS_RTTI
    // std
    #include <typeinfo>  // std::type_info, for target_type
#endif
// re_std
#include "re_std/functional/bad_function_call.hpp"


namespace re_std
{


///////////////////////////////////////////////////////////////////////////////
///                THE C++98 TIER                                           ///
///////////////////////////////////////////////////////////////////////////////
// Below C++11, function is the fixed-arity form Boost.Function used before
// variadic templates: a partial specialisation for each arity from none to
// 6 arguments. Each owns its target through internal::fn98_owner, which
// every arity shares, and calls it through an invoker its constructor
// instantiates for the target's type. The surface is the C++11 tier's,
// less what C++98 cannot spell -- moves, construction from and comparison
// with nullptr, and operator bool, which only `explicit` makes safe; empty()
// is the test at every level -- so code written to it means the same at
// every level. A target is called with the call syntax, f(args...): a
// function, a function pointer or a function object. A pointer to member,
// which the C++11 tier reaches through invoke, is not taken below C++11.
// Arguments pass as the signature declares them, and a void signature
// discards the target's result.

namespace internal
{

    // fn_type_id / fn_type_id_of
    //   alias: typedef + function: RTTI-free per-type identity, as in the
    // C++11 tier: the address of a distinct static member is unique per
    // `Type`, and target<T>() compares it.
    typedef const void* fn_type_id;

    template<typename Type>
    struct fn_type_tag
    {
        static const char s_id;
    };

    template<typename Type>
    const char fn_type_tag<Type>::s_id = 0;

    template<typename Type>
    fn_type_id
    fn_type_id_of()
    {
        return &fn_type_tag<Type>::s_id;
    }

    // fn98_holder
    //   class: an owned target of any type, behind the operations every
    // arity shares; the call itself is the invoker's.
    class fn98_holder
    {
    public:
        virtual ~fn98_holder() {}

        virtual fn98_holder* clone() const = 0;
        virtual fn_type_id   type_id() const = 0;
        virtual void*        target_ptr() = 0;
#if RE_STD_HAS_RTTI
        virtual const std::type_info& type_info() const = 0;
#endif
    };

    // fn98_target
    //   class: the holder of a target of type Fn.
    template<typename Fn>
    class fn98_target : public fn98_holder
    {
    public:
        explicit fn98_target(
            const Fn& _fn
        )
            : m_fn(_fn)
        {}

        fn98_holder*
        clone() const
        {
            return new fn98_target(m_fn);
        }

        fn_type_id
        type_id() const
        {
            return fn_type_id_of<Fn>();
        }

        void*
        target_ptr()
        {
            return &m_fn;
        }

#if RE_STD_HAS_RTTI
        const std::type_info&
        type_info() const
        {
            return typeid(Fn);
        }
#endif

        Fn m_fn;
    };

    // fn98_owner
    //   class: what every arity's function shares: the owned target, its
    // copy, and the observers that need no signature.
    class fn98_owner
    {
    public:
        // empty
        //   function: whether the wrapper holds no target.
        bool
        empty() const
        {
            return (m_held == 0);
        }

#if RE_STD_HAS_RTTI
        // target_type
        //   function: the type_info of the target, or typeid(void) when
        // empty.
        const std::type_info&
        target_type() const
        {
            return (m_held) ? m_held->type_info()
                            : typeid(void);
        }
#endif

        // target
        //   function: the target if it is exactly a Tp, else null;
        // RTTI-free.
        template<typename Tp>
        Tp*
        target()
        {
            return ( (m_held) &&
                     (m_held->type_id() == fn_type_id_of<Tp>()) )
                       ? static_cast<Tp*>(m_held->target_ptr())
                       : 0;
        }

        template<typename Tp>
        const Tp*
        target() const
        {
            return ( (m_held) &&
                     (m_held->type_id() == fn_type_id_of<Tp>()) )
                       ? static_cast<const Tp*>(m_held->target_ptr())
                       : 0;
        }

    protected:
        fn98_owner()
            : m_held(0)
        {}

        explicit fn98_owner(
            fn98_holder* _held
        )
            : m_held(_held)
        {}

        fn98_owner(
            const fn98_owner& _other
        )
            : m_held((_other.m_held) ? _other.m_held->clone()
                                     : 0)
        {}

        ~fn98_owner()
        {
            delete m_held;
        }

        void
        swap_held(
            fn98_owner& _other
        )
        {
            fn98_holder* const held = m_held;

            m_held        = _other.m_held;
            _other.m_held = held;

            return;
        }

        // m_empty_call -- what a call does on an empty wrapper: throws, as
        // std's does, or with exceptions off aborts
        static void
        m_empty_call()
        {
#if RE_STD_HAS_EXCEPTIONS
            throw bad_function_call();
#else
            std::abort();
#endif
        }

        fn98_holder* m_held;

    private:
        // assignment is each arity's, by copy and swap
        fn98_owner& operator=(const fn98_owner&);
    };

    // fn98_invoke
    //   struct: the invokers of a target of type Fn, one an arity: each
    // casts the holder back to its target and calls it, returning the
    // result as Ret. The void specialisation discards the result, which
    // C++98 forbids a void function to return.
    template<typename Fn,
             typename Ret>
    struct fn98_invoke
    {
        static Ret
        call0(
            fn98_holder* _held
        )
        {
            return static_cast<fn98_target<Fn>*>(_held)->m_fn();
        }

        template<typename A1>
        static Ret
        call1(
            fn98_holder* _held,
            A1           _a1
        )
        {
            return static_cast<fn98_target<Fn>*>(_held)->m_fn(_a1);
        }

        template<typename A1,
                 typename A2>
        static Ret
        call2(
            fn98_holder* _held,
            A1           _a1,
            A2           _a2
        )
        {
            return static_cast<fn98_target<Fn>*>(_held)->m_fn(_a1,
                                                              _a2);
        }

        template<typename A1,
                 typename A2,
                 typename A3>
        static Ret
        call3(
            fn98_holder* _held,
            A1           _a1,
            A2           _a2,
            A3           _a3
        )
        {
            return static_cast<fn98_target<Fn>*>(_held)->m_fn(_a1,
                                                              _a2,
                                                              _a3);
        }

        template<typename A1,
                 typename A2,
                 typename A3,
                 typename A4>
        static Ret
        call4(
            fn98_holder* _held,
            A1           _a1,
            A2           _a2,
            A3           _a3,
            A4           _a4
        )
        {
            return static_cast<fn98_target<Fn>*>(_held)->m_fn(_a1,
                                                              _a2,
                                                              _a3,
                                                              _a4);
        }

        template<typename A1,
                 typename A2,
                 typename A3,
                 typename A4,
                 typename A5>
        static Ret
        call5(
            fn98_holder* _held,
            A1           _a1,
            A2           _a2,
            A3           _a3,
            A4           _a4,
            A5           _a5
        )
        {
            return static_cast<fn98_target<Fn>*>(_held)->m_fn(_a1,
                                                              _a2,
                                                              _a3,
                                                              _a4,
                                                              _a5);
        }

        template<typename A1,
                 typename A2,
                 typename A3,
                 typename A4,
                 typename A5,
                 typename A6>
        static Ret
        call6(
            fn98_holder* _held,
            A1           _a1,
            A2           _a2,
            A3           _a3,
            A4           _a4,
            A5           _a5,
            A6           _a6
        )
        {
            return static_cast<fn98_target<Fn>*>(_held)->m_fn(_a1,
                                                              _a2,
                                                              _a3,
                                                              _a4,
                                                              _a5,
                                                              _a6);
        }
    };

    template<typename Fn>
    struct fn98_invoke<Fn, void>
    {
        static void
        call0(
            fn98_holder* _held
        )
        {
            static_cast<fn98_target<Fn>*>(_held)->m_fn();

            return;
        }

        template<typename A1>
        static void
        call1(
            fn98_holder* _held,
            A1           _a1
        )
        {
            static_cast<fn98_target<Fn>*>(_held)->m_fn(_a1);

            return;
        }

        template<typename A1,
                 typename A2>
        static void
        call2(
            fn98_holder* _held,
            A1           _a1,
            A2           _a2
        )
        {
            static_cast<fn98_target<Fn>*>(_held)->m_fn(_a1,
                                                       _a2);

            return;
        }

        template<typename A1,
                 typename A2,
                 typename A3>
        static void
        call3(
            fn98_holder* _held,
            A1           _a1,
            A2           _a2,
            A3           _a3
        )
        {
            static_cast<fn98_target<Fn>*>(_held)->m_fn(_a1,
                                                       _a2,
                                                       _a3);

            return;
        }

        template<typename A1,
                 typename A2,
                 typename A3,
                 typename A4>
        static void
        call4(
            fn98_holder* _held,
            A1           _a1,
            A2           _a2,
            A3           _a3,
            A4           _a4
        )
        {
            static_cast<fn98_target<Fn>*>(_held)->m_fn(_a1,
                                                       _a2,
                                                       _a3,
                                                       _a4);

            return;
        }

        template<typename A1,
                 typename A2,
                 typename A3,
                 typename A4,
                 typename A5>
        static void
        call5(
            fn98_holder* _held,
            A1           _a1,
            A2           _a2,
            A3           _a3,
            A4           _a4,
            A5           _a5
        )
        {
            static_cast<fn98_target<Fn>*>(_held)->m_fn(_a1,
                                                       _a2,
                                                       _a3,
                                                       _a4,
                                                       _a5);

            return;
        }

        template<typename A1,
                 typename A2,
                 typename A3,
                 typename A4,
                 typename A5,
                 typename A6>
        static void
        call6(
            fn98_holder* _held,
            A1           _a1,
            A2           _a2,
            A3           _a3,
            A4           _a4,
            A5           _a5,
            A6           _a6
        )
        {
            static_cast<fn98_target<Fn>*>(_held)->m_fn(_a1,
                                                       _a2,
                                                       _a3,
                                                       _a4,
                                                       _a5,
                                                       _a6);

            return;
        }
    };

}  // internal


// function
//   class: primary template, intentionally undefined. Below C++11 only a
// function type of up to 6 parameters, R(A1, ...), names a wrapper.
template<typename Signature>
class function;

// function<Ret()>
//   class: an owning wrapper for any copyable target callable with 0
// arguments (C++98).
template<typename Ret>
class function<Ret()> : private internal::fn98_owner
{
public:
    typedef Ret result_type;

    using internal::fn98_owner::empty;
    using internal::fn98_owner::target;
#if RE_STD_HAS_RTTI
    using internal::fn98_owner::target_type;
#endif

    // function
    //   function: constructs an empty wrapper.
    function()
        : internal::fn98_owner(),
          m_invoke(0)
    {}

    // function
    //   function: deep-copies the target.
    function(
        const function& _other
    )
        : internal::fn98_owner(_other),
          m_invoke(_other.m_invoke)
    {}

    // function
    //   function: wraps a copy of _fn, which must be callable as
    // Ret().
    template<typename Fn>
    function(
        Fn _fn
    )
        : internal::fn98_owner(new internal::fn98_target<Fn>(_fn)),
          m_invoke(&internal::fn98_invoke<Fn, Ret>::call0)
    {}

    function&
    operator=(
        const function& _other
    )
    {
        function(_other).swap(*this);

        return *this;
    }

    template<typename Fn>
    function&
    operator=(
        Fn _fn
    )
    {
        function(_fn).swap(*this);

        return *this;
    }

    // swap
    //   function: O(1) exchange of the targets and their invokers.
    void
    swap(
        function& _other
    )
    {
        const invoker_type invoke = m_invoke;

        swap_held(_other);
        m_invoke        = _other.m_invoke;
        _other.m_invoke = invoke;

        return;
    }

    // operator()
    //   function: calls the target; throws bad_function_call when empty,
    // or aborts where exceptions are off.
    Ret
    operator()() const
    {
        // an empty wrapper has nothing to call
        if (!m_held)
        {
            m_empty_call();
        }

        return m_invoke(m_held);
    }

private:
    typedef Ret (*invoker_type)(internal::fn98_holder*);

    invoker_type m_invoke;
};

// swap
//   function: exchanges two wrappers; enables the ADL two-step swap.
template<typename Ret>
void
swap(
    function<Ret()>& _a,
    function<Ret()>& _b
)
{
    _a.swap(_b);

    return;
}

// function<Ret(A1)>
//   class: an owning wrapper for any copyable target callable with 1
// argument (C++98).
template<typename Ret,
         typename A1>
class function<Ret(A1)> : private internal::fn98_owner
{
public:
    typedef Ret result_type;

    using internal::fn98_owner::empty;
    using internal::fn98_owner::target;
#if RE_STD_HAS_RTTI
    using internal::fn98_owner::target_type;
#endif

    // function
    //   function: constructs an empty wrapper.
    function()
        : internal::fn98_owner(),
          m_invoke(0)
    {}

    // function
    //   function: deep-copies the target.
    function(
        const function& _other
    )
        : internal::fn98_owner(_other),
          m_invoke(_other.m_invoke)
    {}

    // function
    //   function: wraps a copy of _fn, which must be callable as
    // Ret(A1).
    template<typename Fn>
    function(
        Fn _fn
    )
        : internal::fn98_owner(new internal::fn98_target<Fn>(_fn)),
          m_invoke(&internal::fn98_invoke<Fn, Ret>::template call1<A1>)
    {}

    function&
    operator=(
        const function& _other
    )
    {
        function(_other).swap(*this);

        return *this;
    }

    template<typename Fn>
    function&
    operator=(
        Fn _fn
    )
    {
        function(_fn).swap(*this);

        return *this;
    }

    // swap
    //   function: O(1) exchange of the targets and their invokers.
    void
    swap(
        function& _other
    )
    {
        const invoker_type invoke = m_invoke;

        swap_held(_other);
        m_invoke        = _other.m_invoke;
        _other.m_invoke = invoke;

        return;
    }

    // operator()
    //   function: calls the target; throws bad_function_call when empty,
    // or aborts where exceptions are off.
    Ret
    operator()(
        A1 _a1
    ) const
    {
        // an empty wrapper has nothing to call
        if (!m_held)
        {
            m_empty_call();
        }

        return m_invoke(m_held,
                        _a1);
    }

private:
    typedef Ret (*invoker_type)(internal::fn98_holder*, A1);

    invoker_type m_invoke;
};

// swap
//   function: exchanges two wrappers; enables the ADL two-step swap.
template<typename Ret,
         typename A1>
void
swap(
    function<Ret(A1)>& _a,
    function<Ret(A1)>& _b
)
{
    _a.swap(_b);

    return;
}

// function<Ret(A1, A2)>
//   class: an owning wrapper for any copyable target callable with 2
// arguments (C++98).
template<typename Ret,
         typename A1,
         typename A2>
class function<Ret(A1, A2)> : private internal::fn98_owner
{
public:
    typedef Ret result_type;

    using internal::fn98_owner::empty;
    using internal::fn98_owner::target;
#if RE_STD_HAS_RTTI
    using internal::fn98_owner::target_type;
#endif

    // function
    //   function: constructs an empty wrapper.
    function()
        : internal::fn98_owner(),
          m_invoke(0)
    {}

    // function
    //   function: deep-copies the target.
    function(
        const function& _other
    )
        : internal::fn98_owner(_other),
          m_invoke(_other.m_invoke)
    {}

    // function
    //   function: wraps a copy of _fn, which must be callable as
    // Ret(A1, A2).
    template<typename Fn>
    function(
        Fn _fn
    )
        : internal::fn98_owner(new internal::fn98_target<Fn>(_fn)),
          m_invoke(&internal::fn98_invoke<Fn, Ret>::template call2<A1, A2>)
    {}

    function&
    operator=(
        const function& _other
    )
    {
        function(_other).swap(*this);

        return *this;
    }

    template<typename Fn>
    function&
    operator=(
        Fn _fn
    )
    {
        function(_fn).swap(*this);

        return *this;
    }

    // swap
    //   function: O(1) exchange of the targets and their invokers.
    void
    swap(
        function& _other
    )
    {
        const invoker_type invoke = m_invoke;

        swap_held(_other);
        m_invoke        = _other.m_invoke;
        _other.m_invoke = invoke;

        return;
    }

    // operator()
    //   function: calls the target; throws bad_function_call when empty,
    // or aborts where exceptions are off.
    Ret
    operator()(
        A1 _a1,
        A2 _a2
    ) const
    {
        // an empty wrapper has nothing to call
        if (!m_held)
        {
            m_empty_call();
        }

        return m_invoke(m_held,
                        _a1,
                        _a2);
    }

private:
    typedef Ret (*invoker_type)(internal::fn98_holder*, A1, A2);

    invoker_type m_invoke;
};

// swap
//   function: exchanges two wrappers; enables the ADL two-step swap.
template<typename Ret,
         typename A1,
         typename A2>
void
swap(
    function<Ret(A1, A2)>& _a,
    function<Ret(A1, A2)>& _b
)
{
    _a.swap(_b);

    return;
}

// function<Ret(A1, A2, A3)>
//   class: an owning wrapper for any copyable target callable with 3
// arguments (C++98).
template<typename Ret,
         typename A1,
         typename A2,
         typename A3>
class function<Ret(A1, A2, A3)> : private internal::fn98_owner
{
public:
    typedef Ret result_type;

    using internal::fn98_owner::empty;
    using internal::fn98_owner::target;
#if RE_STD_HAS_RTTI
    using internal::fn98_owner::target_type;
#endif

    // function
    //   function: constructs an empty wrapper.
    function()
        : internal::fn98_owner(),
          m_invoke(0)
    {}

    // function
    //   function: deep-copies the target.
    function(
        const function& _other
    )
        : internal::fn98_owner(_other),
          m_invoke(_other.m_invoke)
    {}

    // function
    //   function: wraps a copy of _fn, which must be callable as
    // Ret(A1, A2, A3).
    template<typename Fn>
    function(
        Fn _fn
    )
        : internal::fn98_owner(new internal::fn98_target<Fn>(_fn)),
          m_invoke(&internal::fn98_invoke<Fn, Ret>::template call3<A1, A2, A3>)
    {}

    function&
    operator=(
        const function& _other
    )
    {
        function(_other).swap(*this);

        return *this;
    }

    template<typename Fn>
    function&
    operator=(
        Fn _fn
    )
    {
        function(_fn).swap(*this);

        return *this;
    }

    // swap
    //   function: O(1) exchange of the targets and their invokers.
    void
    swap(
        function& _other
    )
    {
        const invoker_type invoke = m_invoke;

        swap_held(_other);
        m_invoke        = _other.m_invoke;
        _other.m_invoke = invoke;

        return;
    }

    // operator()
    //   function: calls the target; throws bad_function_call when empty,
    // or aborts where exceptions are off.
    Ret
    operator()(
        A1 _a1,
        A2 _a2,
        A3 _a3
    ) const
    {
        // an empty wrapper has nothing to call
        if (!m_held)
        {
            m_empty_call();
        }

        return m_invoke(m_held,
                        _a1,
                        _a2,
                        _a3);
    }

private:
    typedef Ret (*invoker_type)(internal::fn98_holder*, A1, A2, A3);

    invoker_type m_invoke;
};

// swap
//   function: exchanges two wrappers; enables the ADL two-step swap.
template<typename Ret,
         typename A1,
         typename A2,
         typename A3>
void
swap(
    function<Ret(A1, A2, A3)>& _a,
    function<Ret(A1, A2, A3)>& _b
)
{
    _a.swap(_b);

    return;
}

// function<Ret(A1, A2, A3, A4)>
//   class: an owning wrapper for any copyable target callable with 4
// arguments (C++98).
template<typename Ret,
         typename A1,
         typename A2,
         typename A3,
         typename A4>
class function<Ret(A1, A2, A3, A4)> : private internal::fn98_owner
{
public:
    typedef Ret result_type;

    using internal::fn98_owner::empty;
    using internal::fn98_owner::target;
#if RE_STD_HAS_RTTI
    using internal::fn98_owner::target_type;
#endif

    // function
    //   function: constructs an empty wrapper.
    function()
        : internal::fn98_owner(),
          m_invoke(0)
    {}

    // function
    //   function: deep-copies the target.
    function(
        const function& _other
    )
        : internal::fn98_owner(_other),
          m_invoke(_other.m_invoke)
    {}

    // function
    //   function: wraps a copy of _fn, which must be callable as
    // Ret(A1, A2, A3, A4).
    template<typename Fn>
    function(
        Fn _fn
    )
        : internal::fn98_owner(new internal::fn98_target<Fn>(_fn)),
          m_invoke(&internal::fn98_invoke<Fn, Ret>::template call4<A1,
                                                                   A2,
                                                                   A3,
                                                                   A4>)
    {}

    function&
    operator=(
        const function& _other
    )
    {
        function(_other).swap(*this);

        return *this;
    }

    template<typename Fn>
    function&
    operator=(
        Fn _fn
    )
    {
        function(_fn).swap(*this);

        return *this;
    }

    // swap
    //   function: O(1) exchange of the targets and their invokers.
    void
    swap(
        function& _other
    )
    {
        const invoker_type invoke = m_invoke;

        swap_held(_other);
        m_invoke        = _other.m_invoke;
        _other.m_invoke = invoke;

        return;
    }

    // operator()
    //   function: calls the target; throws bad_function_call when empty,
    // or aborts where exceptions are off.
    Ret
    operator()(
        A1 _a1,
        A2 _a2,
        A3 _a3,
        A4 _a4
    ) const
    {
        // an empty wrapper has nothing to call
        if (!m_held)
        {
            m_empty_call();
        }

        return m_invoke(m_held,
                        _a1,
                        _a2,
                        _a3,
                        _a4);
    }

private:
    typedef Ret (*invoker_type)(internal::fn98_holder*, A1, A2, A3, A4);

    invoker_type m_invoke;
};

// swap
//   function: exchanges two wrappers; enables the ADL two-step swap.
template<typename Ret,
         typename A1,
         typename A2,
         typename A3,
         typename A4>
void
swap(
    function<Ret(A1, A2, A3, A4)>& _a,
    function<Ret(A1, A2, A3, A4)>& _b
)
{
    _a.swap(_b);

    return;
}

// function<Ret(A1, A2, A3, A4, A5)>
//   class: an owning wrapper for any copyable target callable with 5
// arguments (C++98).
template<typename Ret,
         typename A1,
         typename A2,
         typename A3,
         typename A4,
         typename A5>
class function<Ret(A1, A2, A3, A4, A5)> : private internal::fn98_owner
{
public:
    typedef Ret result_type;

    using internal::fn98_owner::empty;
    using internal::fn98_owner::target;
#if RE_STD_HAS_RTTI
    using internal::fn98_owner::target_type;
#endif

    // function
    //   function: constructs an empty wrapper.
    function()
        : internal::fn98_owner(),
          m_invoke(0)
    {}

    // function
    //   function: deep-copies the target.
    function(
        const function& _other
    )
        : internal::fn98_owner(_other),
          m_invoke(_other.m_invoke)
    {}

    // function
    //   function: wraps a copy of _fn, which must be callable as
    // Ret(A1, A2, A3, A4, A5).
    template<typename Fn>
    function(
        Fn _fn
    )
        : internal::fn98_owner(new internal::fn98_target<Fn>(_fn)),
          m_invoke(&internal::fn98_invoke<Fn, Ret>::template call5<A1,
                                                                   A2,
                                                                   A3,
                                                                   A4,
                                                                   A5>)
    {}

    function&
    operator=(
        const function& _other
    )
    {
        function(_other).swap(*this);

        return *this;
    }

    template<typename Fn>
    function&
    operator=(
        Fn _fn
    )
    {
        function(_fn).swap(*this);

        return *this;
    }

    // swap
    //   function: O(1) exchange of the targets and their invokers.
    void
    swap(
        function& _other
    )
    {
        const invoker_type invoke = m_invoke;

        swap_held(_other);
        m_invoke        = _other.m_invoke;
        _other.m_invoke = invoke;

        return;
    }

    // operator()
    //   function: calls the target; throws bad_function_call when empty,
    // or aborts where exceptions are off.
    Ret
    operator()(
        A1 _a1,
        A2 _a2,
        A3 _a3,
        A4 _a4,
        A5 _a5
    ) const
    {
        // an empty wrapper has nothing to call
        if (!m_held)
        {
            m_empty_call();
        }

        return m_invoke(m_held,
                        _a1,
                        _a2,
                        _a3,
                        _a4,
                        _a5);
    }

private:
    typedef Ret (*invoker_type)(internal::fn98_holder*, A1, A2, A3, A4, A5);

    invoker_type m_invoke;
};

// swap
//   function: exchanges two wrappers; enables the ADL two-step swap.
template<typename Ret,
         typename A1,
         typename A2,
         typename A3,
         typename A4,
         typename A5>
void
swap(
    function<Ret(A1, A2, A3, A4, A5)>& _a,
    function<Ret(A1, A2, A3, A4, A5)>& _b
)
{
    _a.swap(_b);

    return;
}

// function<Ret(A1, A2, A3, A4, A5, A6)>
//   class: an owning wrapper for any copyable target callable with 6
// arguments (C++98).
template<typename Ret,
         typename A1,
         typename A2,
         typename A3,
         typename A4,
         typename A5,
         typename A6>
class function<Ret(A1, A2, A3, A4, A5, A6)> : private internal::fn98_owner
{
public:
    typedef Ret result_type;

    using internal::fn98_owner::empty;
    using internal::fn98_owner::target;
#if RE_STD_HAS_RTTI
    using internal::fn98_owner::target_type;
#endif

    // function
    //   function: constructs an empty wrapper.
    function()
        : internal::fn98_owner(),
          m_invoke(0)
    {}

    // function
    //   function: deep-copies the target.
    function(
        const function& _other
    )
        : internal::fn98_owner(_other),
          m_invoke(_other.m_invoke)
    {}

    // function
    //   function: wraps a copy of _fn, which must be callable as
    // Ret(A1, A2, A3, A4, A5, A6).
    template<typename Fn>
    function(
        Fn _fn
    )
        : internal::fn98_owner(new internal::fn98_target<Fn>(_fn)),
          m_invoke(&internal::fn98_invoke<Fn, Ret>::template call6<A1,
                                                                   A2,
                                                                   A3,
                                                                   A4,
                                                                   A5,
                                                                   A6>)
    {}

    function&
    operator=(
        const function& _other
    )
    {
        function(_other).swap(*this);

        return *this;
    }

    template<typename Fn>
    function&
    operator=(
        Fn _fn
    )
    {
        function(_fn).swap(*this);

        return *this;
    }

    // swap
    //   function: O(1) exchange of the targets and their invokers.
    void
    swap(
        function& _other
    )
    {
        const invoker_type invoke = m_invoke;

        swap_held(_other);
        m_invoke        = _other.m_invoke;
        _other.m_invoke = invoke;

        return;
    }

    // operator()
    //   function: calls the target; throws bad_function_call when empty,
    // or aborts where exceptions are off.
    Ret
    operator()(
        A1 _a1,
        A2 _a2,
        A3 _a3,
        A4 _a4,
        A5 _a5,
        A6 _a6
    ) const
    {
        // an empty wrapper has nothing to call
        if (!m_held)
        {
            m_empty_call();
        }

        return m_invoke(m_held,
                        _a1,
                        _a2,
                        _a3,
                        _a4,
                        _a5,
                        _a6);
    }

private:
    typedef Ret (*invoker_type)(internal::fn98_holder*, A1, A2, A3, A4, A5, A6);

    invoker_type m_invoke;
};

// swap
//   function: exchanges two wrappers; enables the ADL two-step swap.
template<typename Ret,
         typename A1,
         typename A2,
         typename A3,
         typename A4,
         typename A5,
         typename A6>
void
swap(
    function<Ret(A1, A2, A3, A4, A5, A6)>& _a,
    function<Ret(A1, A2, A3, A4, A5, A6)>& _b
)
{
    _a.swap(_b);

    return;
}


}  // re_std

#endif // variadic templates + rvalue references

#endif  // RE_STD_FUNCTIONAL_FUNCTION_HPP
