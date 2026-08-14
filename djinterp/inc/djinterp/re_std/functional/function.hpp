/***********************************************************************
* restd                                                     function.hpp
*
* class: type-erased owning callable wrapper -- restd's portable
*   alternative to `std::function` (C++11).
*   `function<R(Args...)>` stores any CopyConstructible callable that is
* invocable as `R(Args...)` (the result being convertible to `R`, or
* `R` being `void`), erasing its concrete type behind a small virtual
* dispatch table. Calling an empty wrapper throws `bad_function_call`.
* Target dispatch is delegated wholesale to `restd::invoke`, so
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
* gated on `D_ENV_CPP98_HAS_TYPEINFO`.
*
*   Min standard: C++11. `function` needs variadic templates (to spell
* `R(Args...)`) and rvalue references (move, perfect forwarding); the
* whole header is gated on both, matching `invoke` / `not_fn`. There is
* no C++98 path in this milestone -- a conforming C++98 `function` would
* require Boost.Function-style fixed-arity (arity 0..N) specialisations,
* which is tracked as a follow-on. `function` is never constexpr (heap
* allocation and, where present, RTTI), matching std.
*
*   Deviations from `std::function`: the (deprecated in C++11, removed in
* C++17) allocator-taking constructors and `assign(f, alloc)` are not
* provided. `operator==`/`operator!=` against `nullptr_t` are provided
* for parity even though std deprecated them in C++20.
*
*
* path:      /inc/djinterp/re_std/functional/function.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.07.25
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_FUNCTION_
#define RESTD_FUNCTIONAL_FUNCTION_ 1

#include "djinterp.hpp"

#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&  \
     D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES)

#include "restd/type_traits/type_traits.hpp"
#include "restd/utility/forward.hpp"
#include "restd/functional/invoke.hpp"
#include "restd/functional/bad_function_call.hpp"

#include <cstddef>   // std::nullptr_t

#if D_ENV_CPP98_HAS_TYPEINFO
    #include <typeinfo>
#endif

namespace restd
{

// function
//   class: primary template, intentionally undefined. Only a genuine
// function type `R(Args...)` names a valid specialisation, so
// `function<int>` is ill-formed -- matching std.
template<typename _Signature>
class function;

NS_INTERNAL

    // fn_declval
    //   function: declval-style helper. Declared, never defined; usable
    // only in unevaluated contexts. Never instantiated with `void` here
    // (every use is a callable or a function-parameter type).
    template<typename _Type>
    _Type&& fn_declval();

    // fn_type_id / fn_type_id_of
    //   typedef + function: RTTI-free per-type identity. The address of a
    // distinct static member is unique per `_Type`, giving a stable,
    // constexpr-address token usable for `target<T>()` comparisons with
    // zero dependence on `<typeinfo>`. Mirrors `any_type_id`.
    typedef const void* fn_type_id;

    template<typename _Type>
    struct fn_type_tag
    {
        static const char s_id;
    };

    template<typename _Type>
    const char fn_type_tag<_Type>::s_id = 0;

    template<typename _Type>
    fn_type_id
    fn_type_id_of()
    {
        return &fn_type_tag<_Type>::s_id;
    }

    // fn_conv
    //   trait helper: SFINAE probe for "a prvalue of `_From` is
    // convertible to `_To`". `accept(_To)` participates only when the
    // conversion is well-formed.
    template<typename _To>
    struct fn_conv
    {
        static void accept(_To);

        template<typename _From>
        static true_type
        probe(decltype(accept(fn_declval<_From>()))*);

        template<typename _From>
        static false_type
        probe(...);
    };

    // fn_convertible
    //   trait: `true` if `_From` is convertible to `_To`. `_To == void`
    // is always satisfiable (any result is discardable).
    template<typename _To, typename _From>
    struct fn_convertible
        : integral_constant<bool,
              is_same<decltype(fn_conv<_To>::template probe<_From>(0)),
                      true_type>::value>
    {};

    template<typename _From>
    struct fn_convertible<void, _From>
        : true_type
    {};

    // fn_callable
    //   trait: `true` if `invoke(f, args...)` is well-formed for an
    // lvalue `_Fn` and the given `_Args`. The result is cast to `void`
    // inside the probe so that reference-returning callables (whose
    // result type cannot be pointer-formed) are still detected.
    template<typename _Fn, typename... _Args>
    struct fn_callable
    {
        template<typename _F>
        static true_type
        probe(int,
              decltype((void)restd::invoke(fn_declval<_F&>(),
                                           fn_declval<_Args>()...))* = 0);

        template<typename _F>
        static false_type
        probe(...);

        static const bool value =
            is_same<decltype(probe<_Fn>(0)), true_type>::value;
    };

    // fn_invocable_r_impl
    //   trait: two-step so the result-type `decltype` is only formed when
    // the call is actually well-formed (guarding against a hard error in
    // the non-callable case).
    template<bool _Callable, typename _Ret, typename _Fn, typename... _Args>
    struct fn_invocable_r_impl
    {
        static const bool value = false;
    };

    template<typename _Ret, typename _Fn, typename... _Args>
    struct fn_invocable_r_impl<true, _Ret, _Fn, _Args...>
    {
        static const bool value = fn_convertible<
            _Ret,
            decltype(restd::invoke(fn_declval<_Fn&>(),
                                   fn_declval<_Args>()...))
        >::value;
    };

    // fn_invocable_r
    //   trait: `true` if an lvalue `_Fn` is invocable per `_Ret(_Args...)`
    // with the result convertible to `_Ret` (or `_Ret` == void). This is
    // the local stand-in for `is_invocable_r` (which is a follow-on to
    // this milestone in restd::type_traits).
    template<typename _Ret, typename _Fn, typename... _Args>
    struct fn_invocable_r
        : integral_constant<bool,
              fn_invocable_r_impl<
                  fn_callable<_Fn, _Args...>::value,
                  _Ret, _Fn, _Args...>::value>
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
    template<typename _Fn,
             typename = typename enable_if<
                 !is_function<typename remove_reference<_Fn>::type>::value
             >::type>
    bool
    fn_is_null(const _Fn& _f, int,
               decltype((void)(fn_declval<const _Fn&>() == 0), 0)* = 0)
    {
        return _f == 0;
    }

    template<typename _Fn>
    bool
    fn_is_null(const _Fn&, ...)
    {
        return false;
    }

    // fn_call_impl
    //   helper: performs the actual invoke, discarding the result when
    // `_Ret` is `void` (C++11 has no `if constexpr` to branch inline).
    template<typename _Ret>
    struct fn_call_impl
    {
        template<typename _Fd, typename... _A>
        static _Ret
        call(_Fd& _f, _A&&... _a)
        {
            return restd::invoke(_f, restd::forward<_A>(_a)...);
        }
    };

    template<>
    struct fn_call_impl<void>
    {
        template<typename _Fd, typename... _A>
        static void
        call(_Fd& _f, _A&&... _a)
        {
            restd::invoke(_f, restd::forward<_A>(_a)...);
        }
    };

    // fn_base
    //   struct: abstract type-erasure interface for a stored target.
    template<typename _Ret, typename... _Args>
    struct fn_base
    {
        virtual ~fn_base() {}

        virtual _Ret        do_call(_Args...) = 0;
        virtual fn_base*    clone() const     = 0;
        virtual fn_type_id  type_id() const   = 0;
        virtual void*       target_ptr()      = 0;

#if D_ENV_CPP98_HAS_TYPEINFO
        virtual const std::type_info& type_info() const = 0;
#endif
    };

    // fn_holder
    //   struct: concrete holder storing the decayed target by value.
    template<typename _Fd, typename _Ret, typename... _Args>
    struct fn_holder
        : fn_base<_Ret, _Args...>
    {
        _Fd m_f;

        template<typename _G>
        explicit fn_holder(_G&& _g)
            : m_f(restd::forward<_G>(_g))
        {}

        _Ret do_call(_Args... _a)
        {
            return fn_call_impl<_Ret>::call(
                m_f, restd::forward<_Args>(_a)...);
        }

        fn_base<_Ret, _Args...>* clone() const
        {
            return new fn_holder(m_f);
        }

        fn_type_id type_id() const
        {
            return fn_type_id_of<_Fd>();
        }

        void* target_ptr()
        {
            return static_cast<void*>(&m_f);
        }

#if D_ENV_CPP98_HAS_TYPEINFO
        const std::type_info& type_info() const
        {
            return typeid(_Fd);
        }
#endif
    };

NS_END  // internal

// function<_Ret(_Args...)>
//   class: the type-erased callable wrapper. See the file header for the
// storage model, type-identity scheme, and deviations from std.
template<typename _Ret, typename... _Args>
class function<_Ret(_Args...)>
{
public:

#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    // typedef: legacy member; present through C++17, removed in C++20
    // (matches std::function).
    typedef _Ret result_type;
#endif

    // ---- construction (empty) -------------------------------------------

    // function
    //   ctor: constructs an empty wrapper.
    function() noexcept
        : m_ptr(0)
    {}

    // function
    //   ctor: constructs an empty wrapper from `nullptr`.
    function(std::nullptr_t) noexcept
        : m_ptr(0)
    {}

    // ---- construction (copy / move) -------------------------------------

    // function
    //   ctor: deep-copies the target (requires a CopyConstructible
    // target, as std does).
    function(const function& _other)
        : m_ptr(_other.m_ptr ? _other.m_ptr->clone() : 0)
    {}

    // function
    //   ctor: steals the target; leaves `_other` empty.
    function(function&& _other) noexcept
        : m_ptr(_other.m_ptr)
    {
        _other.m_ptr = 0;
    }

    // ---- construction (from a callable) ---------------------------------

    // function
    //   ctor: wraps any callable invocable as `_Ret(_Args...)`. Excluded
    // for `function` itself (so copy/move win) and for non-invocable
    // types (SFINAE). A null function/member pointer yields an empty
    // wrapper, matching std.
    template<typename _Fn,
             typename = typename enable_if<
                 ( !is_same<typename decay<_Fn>::type, function>::value &&
                   internal::fn_invocable_r<
                       _Ret, typename decay<_Fn>::type, _Args...>::value )
             >::type>
    function(_Fn&& _f)
        : m_ptr(0)
    {
        typedef typename decay<_Fn>::type _Fd;
        if (!internal::fn_is_null(_f, 0))
        {
            m_ptr = new internal::fn_holder<_Fd, _Ret, _Args...>(
                restd::forward<_Fn>(_f));
        }
    }

    // ---- assignment -----------------------------------------------------

    // operator=
    //   assign: copy via copy-and-swap.
    function&
    operator=(const function& _other)
    {
        function(_other).swap(*this);
        return *this;
    }

    // operator=
    //   assign: move via swap with a stolen temporary.
    function&
    operator=(function&& _other) noexcept
    {
        function(static_cast<function&&>(_other)).swap(*this);
        return *this;
    }

    // operator=
    //   assign: clears the wrapper.
    function&
    operator=(std::nullptr_t) noexcept
    {
        delete m_ptr;
        m_ptr = 0;
        return *this;
    }

    // operator=
    //   assign: rebinds to a new callable (same constraints as the
    // callable ctor).
    template<typename _Fn>
    typename enable_if<
        ( !is_same<typename decay<_Fn>::type, function>::value &&
          internal::fn_invocable_r<
              _Ret, typename decay<_Fn>::type, _Args...>::value ),
        function&
    >::type
    operator=(_Fn&& _f)
    {
        function(restd::forward<_Fn>(_f)).swap(*this);
        return *this;
    }

    // ---- destruction ----------------------------------------------------

    ~function()
    {
        delete m_ptr;
    }

    // ---- modifiers ------------------------------------------------------

    // swap
    //   modifier: O(1) pointer swap.
    void
    swap(function& _other) noexcept
    {
        internal::fn_base<_Ret, _Args...>* _tmp = m_ptr;
        m_ptr        = _other.m_ptr;
        _other.m_ptr = _tmp;
    }

    // ---- observers ------------------------------------------------------

    // operator bool
    //   observer: `true` iff the wrapper holds a target.
    explicit operator bool() const noexcept
    {
        return m_ptr != 0;
    }

    // operator()
    //   function: invokes the stored target; throws `bad_function_call`
    // when empty.
    _Ret
    operator()(_Args... _a) const
    {
        if (!m_ptr)
        {
            throw bad_function_call();
        }
        return m_ptr->do_call(restd::forward<_Args>(_a)...);
    }

#if D_ENV_CPP98_HAS_TYPEINFO
    // target_type
    //   observer: the `type_info` of the stored target, or `typeid(void)`
    // when empty. Only available with `<typeinfo>`.
    const std::type_info&
    target_type() const noexcept
    {
        return m_ptr ? m_ptr->type_info() : typeid(void);
    }
#endif

    // target
    //   observer: a pointer to the stored target if it is exactly `_Tp`,
    // else null. RTTI-free (uses the address-based type id).
    template<typename _Tp>
    _Tp*
    target() noexcept
    {
        if (m_ptr && m_ptr->type_id() == internal::fn_type_id_of<_Tp>())
        {
            return static_cast<_Tp*>(m_ptr->target_ptr());
        }
        return 0;
    }

    // target (const)
    //   observer: const overload of the above.
    template<typename _Tp>
    const _Tp*
    target() const noexcept
    {
        if (m_ptr && m_ptr->type_id() == internal::fn_type_id_of<_Tp>())
        {
            return static_cast<const _Tp*>(m_ptr->target_ptr());
        }
        return 0;
    }

private:

    internal::fn_base<_Ret, _Args...>* m_ptr;
};

// ---- non-member swap ----------------------------------------------------

// swap
//   function: exchanges two wrappers; enables the ADL two-step swap.
template<typename _Ret, typename... _Args>
void
swap(function<_Ret(_Args...)>& _a, function<_Ret(_Args...)>& _b) noexcept
{
    _a.swap(_b);
}

// ---- null comparisons (deprecated in std since C++20, kept for parity) --

template<typename _Ret, typename... _Args>
bool
operator==(const function<_Ret(_Args...)>& _f, std::nullptr_t) noexcept
{
    return !_f;
}

template<typename _Ret, typename... _Args>
bool
operator==(std::nullptr_t, const function<_Ret(_Args...)>& _f) noexcept
{
    return !_f;
}

template<typename _Ret, typename... _Args>
bool
operator!=(const function<_Ret(_Args...)>& _f, std::nullptr_t) noexcept
{
    return static_cast<bool>(_f);
}

template<typename _Ret, typename... _Args>
bool
operator!=(std::nullptr_t, const function<_Ret(_Args...)>& _f) noexcept
{
    return static_cast<bool>(_f);
}

} // namespace restd

#endif // variadic templates + rvalue references

#endif // RESTD_FUNCTIONAL_FUNCTION_
