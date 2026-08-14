/******************************************************************************
* re_std [functional]                                    move_only_function.hpp
*
*   move_only_function - an OWNING type-erased callable, holding a target that need not be copyable.
*
*   Move-only by design: the target need not be copyable, which is what lets a
wrapper hold a unique_ptr capture or a move-only lambda. std::function
cannot, because it requires CopyConstructible targets even when nothing ever
copies it - a requirement that rejects perfectly good callables.
*
*   THE SIGNATURE'S QUALIFIERS ARE PART OF THE TYPE, and honouring that is the
* whole reason this file has twelve specialisations rather than one.
* `move_only_function<void() const>` must be callable on a const wrapper and must invoke
* the target as const; `move_only_function<void() &&>` must only be callable on an rvalue
* wrapper and must invoke the target as an rvalue.  A single specialisation
* that ignored the qualifiers would silently let a const wrapper call a
* non-const target - which is exactly the const-correctness hole
* std::function has and that these types were introduced to close.
*
*   EMPTY IS `m_ops == 0`, and nothing else.  There is no separate flag and no
* engaged bit: a default-constructed or moved-from wrapper has a null
* operations pointer, operator bool tests it, and calling through it is
* undefined exactly as std specifies.
*
*   INVOKING A MOVED-FROM WRAPPER IS UNDEFINED, not empty-checked.  std makes
* the same choice; a branch on every call to catch a bug the caller already
* has would be paid by every correct program.
*
*   STD IS C++23; re_std IS C++11 (noexcept signature forms C++17).
*   `R(Args...) noexcept` only became a distinct TYPE in C++17, so those six
* specialisations cannot exist below it - not a re_std limitation but a
* language one.  Everything else needs only variadic templates and
* ref-qualified member functions, both C++11.
*
*
* path:      /inc/djinterp/re_std/functional/move_only_function.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_FUNCTIONAL_MOVE_ONLY_FUNCTION_
#define RESTD_FUNCTIONAL_MOVE_ONLY_FUNCTION_ 1

// re_std
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "./invoke.hpp"
#include "./func_storage.hpp"

NS_DJINTERP
NS_RESTD

// move_only_function
//   class: primary template, deliberately undefined - only function-type
// specialisations are valid, so a non-signature argument is a clear error.
template<typename _Signature>
class move_only_function;


// move_only_function<_Result(_Args...)>
//   class: target invoked as the target&.
template<typename _Result, typename... _Args>
class move_only_function<_Result(_Args...)>
{
    typedef _Result (*_Invoker)(internal::func_buffer&, _Args&&...);

    internal::func_buffer   m_buffer;
    const internal::func_ops* m_ops;
    _Invoker                m_invoke;

    template<typename _Target>
    static _Result invoke_target(internal::func_buffer& buffer,
                                 _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            static_cast<_Target&>(
                internal::func_manager<_Target>::get(buffer)),
            static_cast<_Args&&>(args)...));
    }

public:
    typedef _Result result_type;

    move_only_function() D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}
    move_only_function(decltype(nullptr)) D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}

    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, move_only_function>::value,
                 int>::type = 0>
    move_only_function(_Func&& func)
        : m_buffer(), m_ops(0), m_invoke(0)
    {
        typedef typename decay<_Func>::type _Target;
        internal::func_manager<_Target>::construct(
            m_buffer, static_cast<_Func&&>(func));
        m_ops    = &internal::func_ops_holder<_Target, false>::value;
        m_invoke = &invoke_target<_Target>;
        return;
    }

    move_only_function(move_only_function&& other) D_NOEXCEPT
        : m_buffer(), m_ops(other.m_ops), m_invoke(other.m_invoke)
    {
        if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
        other.m_ops    = 0;
        other.m_invoke = 0;
        return;
    }

    move_only_function& operator=(move_only_function&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            m_ops    = other.m_ops;
            m_invoke = other.m_invoke;
            if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
            other.m_ops    = 0;
            other.m_invoke = 0;
        }
        return *this;
    }

    move_only_function(const move_only_function&)            = delete;
    move_only_function& operator=(const move_only_function&) = delete;

    ~move_only_function() { reset(); }

    void reset() D_NOEXCEPT
    {
        if (m_ops) { m_ops->destroy(m_buffer); m_ops = 0; m_invoke = 0; }
        return;
    }

    void swap(move_only_function& other) D_NOEXCEPT
    {
        move_only_function tmp(static_cast<move_only_function&&>(*this));
        *this = static_cast<move_only_function&&>(other);
        other = static_cast<move_only_function&&>(tmp);
        return;
    }

    explicit operator bool() const D_NOEXCEPT { return m_ops != 0; }

    //   Undefined when empty, per std - see the header note.
    _Result operator()(_Args... args)
    {
        return m_invoke(
            const_cast<internal::func_buffer&>(this->m_buffer),
            static_cast<_Args&&>(args)...);
    }
};

// move_only_function<_Result(_Args...) const>
//   class: target invoked as const the target&.
template<typename _Result, typename... _Args>
class move_only_function<_Result(_Args...) const>
{
    typedef _Result (*_Invoker)(internal::func_buffer&, _Args&&...);

    internal::func_buffer   m_buffer;
    const internal::func_ops* m_ops;
    _Invoker                m_invoke;

    template<typename _Target>
    static _Result invoke_target(internal::func_buffer& buffer,
                                 _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            static_cast<const _Target&>(
                internal::func_manager<_Target>::get(buffer)),
            static_cast<_Args&&>(args)...));
    }

public:
    typedef _Result result_type;

    move_only_function() D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}
    move_only_function(decltype(nullptr)) D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}

    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, move_only_function>::value,
                 int>::type = 0>
    move_only_function(_Func&& func)
        : m_buffer(), m_ops(0), m_invoke(0)
    {
        typedef typename decay<_Func>::type _Target;
        internal::func_manager<_Target>::construct(
            m_buffer, static_cast<_Func&&>(func));
        m_ops    = &internal::func_ops_holder<_Target, false>::value;
        m_invoke = &invoke_target<_Target>;
        return;
    }

    move_only_function(move_only_function&& other) D_NOEXCEPT
        : m_buffer(), m_ops(other.m_ops), m_invoke(other.m_invoke)
    {
        if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
        other.m_ops    = 0;
        other.m_invoke = 0;
        return;
    }

    move_only_function& operator=(move_only_function&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            m_ops    = other.m_ops;
            m_invoke = other.m_invoke;
            if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
            other.m_ops    = 0;
            other.m_invoke = 0;
        }
        return *this;
    }

    move_only_function(const move_only_function&)            = delete;
    move_only_function& operator=(const move_only_function&) = delete;

    ~move_only_function() { reset(); }

    void reset() D_NOEXCEPT
    {
        if (m_ops) { m_ops->destroy(m_buffer); m_ops = 0; m_invoke = 0; }
        return;
    }

    void swap(move_only_function& other) D_NOEXCEPT
    {
        move_only_function tmp(static_cast<move_only_function&&>(*this));
        *this = static_cast<move_only_function&&>(other);
        other = static_cast<move_only_function&&>(tmp);
        return;
    }

    explicit operator bool() const D_NOEXCEPT { return m_ops != 0; }

    //   Undefined when empty, per std - see the header note.
    _Result operator()(_Args... args) const
    {
        return m_invoke(
            const_cast<internal::func_buffer&>(this->m_buffer),
            static_cast<_Args&&>(args)...);
    }
};

// move_only_function<_Result(_Args...) &>
//   class: target invoked as the target&.
template<typename _Result, typename... _Args>
class move_only_function<_Result(_Args...) &>
{
    typedef _Result (*_Invoker)(internal::func_buffer&, _Args&&...);

    internal::func_buffer   m_buffer;
    const internal::func_ops* m_ops;
    _Invoker                m_invoke;

    template<typename _Target>
    static _Result invoke_target(internal::func_buffer& buffer,
                                 _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            static_cast<_Target&>(
                internal::func_manager<_Target>::get(buffer)),
            static_cast<_Args&&>(args)...));
    }

public:
    typedef _Result result_type;

    move_only_function() D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}
    move_only_function(decltype(nullptr)) D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}

    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, move_only_function>::value,
                 int>::type = 0>
    move_only_function(_Func&& func)
        : m_buffer(), m_ops(0), m_invoke(0)
    {
        typedef typename decay<_Func>::type _Target;
        internal::func_manager<_Target>::construct(
            m_buffer, static_cast<_Func&&>(func));
        m_ops    = &internal::func_ops_holder<_Target, false>::value;
        m_invoke = &invoke_target<_Target>;
        return;
    }

    move_only_function(move_only_function&& other) D_NOEXCEPT
        : m_buffer(), m_ops(other.m_ops), m_invoke(other.m_invoke)
    {
        if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
        other.m_ops    = 0;
        other.m_invoke = 0;
        return;
    }

    move_only_function& operator=(move_only_function&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            m_ops    = other.m_ops;
            m_invoke = other.m_invoke;
            if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
            other.m_ops    = 0;
            other.m_invoke = 0;
        }
        return *this;
    }

    move_only_function(const move_only_function&)            = delete;
    move_only_function& operator=(const move_only_function&) = delete;

    ~move_only_function() { reset(); }

    void reset() D_NOEXCEPT
    {
        if (m_ops) { m_ops->destroy(m_buffer); m_ops = 0; m_invoke = 0; }
        return;
    }

    void swap(move_only_function& other) D_NOEXCEPT
    {
        move_only_function tmp(static_cast<move_only_function&&>(*this));
        *this = static_cast<move_only_function&&>(other);
        other = static_cast<move_only_function&&>(tmp);
        return;
    }

    explicit operator bool() const D_NOEXCEPT { return m_ops != 0; }

    //   Undefined when empty, per std - see the header note.
    _Result operator()(_Args... args) &
    {
        return m_invoke(
            const_cast<internal::func_buffer&>(this->m_buffer),
            static_cast<_Args&&>(args)...);
    }
};

// move_only_function<_Result(_Args...) const &>
//   class: target invoked as const the target&.
template<typename _Result, typename... _Args>
class move_only_function<_Result(_Args...) const &>
{
    typedef _Result (*_Invoker)(internal::func_buffer&, _Args&&...);

    internal::func_buffer   m_buffer;
    const internal::func_ops* m_ops;
    _Invoker                m_invoke;

    template<typename _Target>
    static _Result invoke_target(internal::func_buffer& buffer,
                                 _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            static_cast<const _Target&>(
                internal::func_manager<_Target>::get(buffer)),
            static_cast<_Args&&>(args)...));
    }

public:
    typedef _Result result_type;

    move_only_function() D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}
    move_only_function(decltype(nullptr)) D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}

    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, move_only_function>::value,
                 int>::type = 0>
    move_only_function(_Func&& func)
        : m_buffer(), m_ops(0), m_invoke(0)
    {
        typedef typename decay<_Func>::type _Target;
        internal::func_manager<_Target>::construct(
            m_buffer, static_cast<_Func&&>(func));
        m_ops    = &internal::func_ops_holder<_Target, false>::value;
        m_invoke = &invoke_target<_Target>;
        return;
    }

    move_only_function(move_only_function&& other) D_NOEXCEPT
        : m_buffer(), m_ops(other.m_ops), m_invoke(other.m_invoke)
    {
        if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
        other.m_ops    = 0;
        other.m_invoke = 0;
        return;
    }

    move_only_function& operator=(move_only_function&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            m_ops    = other.m_ops;
            m_invoke = other.m_invoke;
            if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
            other.m_ops    = 0;
            other.m_invoke = 0;
        }
        return *this;
    }

    move_only_function(const move_only_function&)            = delete;
    move_only_function& operator=(const move_only_function&) = delete;

    ~move_only_function() { reset(); }

    void reset() D_NOEXCEPT
    {
        if (m_ops) { m_ops->destroy(m_buffer); m_ops = 0; m_invoke = 0; }
        return;
    }

    void swap(move_only_function& other) D_NOEXCEPT
    {
        move_only_function tmp(static_cast<move_only_function&&>(*this));
        *this = static_cast<move_only_function&&>(other);
        other = static_cast<move_only_function&&>(tmp);
        return;
    }

    explicit operator bool() const D_NOEXCEPT { return m_ops != 0; }

    //   Undefined when empty, per std - see the header note.
    _Result operator()(_Args... args) const &
    {
        return m_invoke(
            const_cast<internal::func_buffer&>(this->m_buffer),
            static_cast<_Args&&>(args)...);
    }
};

// move_only_function<_Result(_Args...) &&>
//   class: target invoked as the target&&.
template<typename _Result, typename... _Args>
class move_only_function<_Result(_Args...) &&>
{
    typedef _Result (*_Invoker)(internal::func_buffer&, _Args&&...);

    internal::func_buffer   m_buffer;
    const internal::func_ops* m_ops;
    _Invoker                m_invoke;

    template<typename _Target>
    static _Result invoke_target(internal::func_buffer& buffer,
                                 _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            static_cast<_Target&&>(
                internal::func_manager<_Target>::get(buffer)),
            static_cast<_Args&&>(args)...));
    }

public:
    typedef _Result result_type;

    move_only_function() D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}
    move_only_function(decltype(nullptr)) D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}

    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, move_only_function>::value,
                 int>::type = 0>
    move_only_function(_Func&& func)
        : m_buffer(), m_ops(0), m_invoke(0)
    {
        typedef typename decay<_Func>::type _Target;
        internal::func_manager<_Target>::construct(
            m_buffer, static_cast<_Func&&>(func));
        m_ops    = &internal::func_ops_holder<_Target, false>::value;
        m_invoke = &invoke_target<_Target>;
        return;
    }

    move_only_function(move_only_function&& other) D_NOEXCEPT
        : m_buffer(), m_ops(other.m_ops), m_invoke(other.m_invoke)
    {
        if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
        other.m_ops    = 0;
        other.m_invoke = 0;
        return;
    }

    move_only_function& operator=(move_only_function&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            m_ops    = other.m_ops;
            m_invoke = other.m_invoke;
            if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
            other.m_ops    = 0;
            other.m_invoke = 0;
        }
        return *this;
    }

    move_only_function(const move_only_function&)            = delete;
    move_only_function& operator=(const move_only_function&) = delete;

    ~move_only_function() { reset(); }

    void reset() D_NOEXCEPT
    {
        if (m_ops) { m_ops->destroy(m_buffer); m_ops = 0; m_invoke = 0; }
        return;
    }

    void swap(move_only_function& other) D_NOEXCEPT
    {
        move_only_function tmp(static_cast<move_only_function&&>(*this));
        *this = static_cast<move_only_function&&>(other);
        other = static_cast<move_only_function&&>(tmp);
        return;
    }

    explicit operator bool() const D_NOEXCEPT { return m_ops != 0; }

    //   Undefined when empty, per std - see the header note.
    _Result operator()(_Args... args) &&
    {
        return m_invoke(
            const_cast<internal::func_buffer&>(this->m_buffer),
            static_cast<_Args&&>(args)...);
    }
};

// move_only_function<_Result(_Args...) const &&>
//   class: target invoked as const the target&&.
template<typename _Result, typename... _Args>
class move_only_function<_Result(_Args...) const &&>
{
    typedef _Result (*_Invoker)(internal::func_buffer&, _Args&&...);

    internal::func_buffer   m_buffer;
    const internal::func_ops* m_ops;
    _Invoker                m_invoke;

    template<typename _Target>
    static _Result invoke_target(internal::func_buffer& buffer,
                                 _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            static_cast<const _Target&&>(
                internal::func_manager<_Target>::get(buffer)),
            static_cast<_Args&&>(args)...));
    }

public:
    typedef _Result result_type;

    move_only_function() D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}
    move_only_function(decltype(nullptr)) D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}

    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, move_only_function>::value,
                 int>::type = 0>
    move_only_function(_Func&& func)
        : m_buffer(), m_ops(0), m_invoke(0)
    {
        typedef typename decay<_Func>::type _Target;
        internal::func_manager<_Target>::construct(
            m_buffer, static_cast<_Func&&>(func));
        m_ops    = &internal::func_ops_holder<_Target, false>::value;
        m_invoke = &invoke_target<_Target>;
        return;
    }

    move_only_function(move_only_function&& other) D_NOEXCEPT
        : m_buffer(), m_ops(other.m_ops), m_invoke(other.m_invoke)
    {
        if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
        other.m_ops    = 0;
        other.m_invoke = 0;
        return;
    }

    move_only_function& operator=(move_only_function&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            m_ops    = other.m_ops;
            m_invoke = other.m_invoke;
            if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
            other.m_ops    = 0;
            other.m_invoke = 0;
        }
        return *this;
    }

    move_only_function(const move_only_function&)            = delete;
    move_only_function& operator=(const move_only_function&) = delete;

    ~move_only_function() { reset(); }

    void reset() D_NOEXCEPT
    {
        if (m_ops) { m_ops->destroy(m_buffer); m_ops = 0; m_invoke = 0; }
        return;
    }

    void swap(move_only_function& other) D_NOEXCEPT
    {
        move_only_function tmp(static_cast<move_only_function&&>(*this));
        *this = static_cast<move_only_function&&>(other);
        other = static_cast<move_only_function&&>(tmp);
        return;
    }

    explicit operator bool() const D_NOEXCEPT { return m_ops != 0; }

    //   Undefined when empty, per std - see the header note.
    _Result operator()(_Args... args) const &&
    {
        return m_invoke(
            const_cast<internal::func_buffer&>(this->m_buffer),
            static_cast<_Args&&>(args)...);
    }
};

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
//   `R(Args...) noexcept` is a distinct TYPE only from C++17.

// move_only_function<_Result(_Args...) noexcept>
//   class: target invoked as the target&.
template<typename _Result, typename... _Args>
class move_only_function<_Result(_Args...) noexcept>
{
    typedef _Result (*_Invoker)(internal::func_buffer&, _Args&&...);

    internal::func_buffer   m_buffer;
    const internal::func_ops* m_ops;
    _Invoker                m_invoke;

    template<typename _Target>
    static _Result invoke_target(internal::func_buffer& buffer,
                                 _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            static_cast<_Target&>(
                internal::func_manager<_Target>::get(buffer)),
            static_cast<_Args&&>(args)...));
    }

public:
    typedef _Result result_type;

    move_only_function() D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}
    move_only_function(decltype(nullptr)) D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}

    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, move_only_function>::value,
                 int>::type = 0>
    move_only_function(_Func&& func)
        : m_buffer(), m_ops(0), m_invoke(0)
    {
        typedef typename decay<_Func>::type _Target;
        internal::func_manager<_Target>::construct(
            m_buffer, static_cast<_Func&&>(func));
        m_ops    = &internal::func_ops_holder<_Target, false>::value;
        m_invoke = &invoke_target<_Target>;
        return;
    }

    move_only_function(move_only_function&& other) D_NOEXCEPT
        : m_buffer(), m_ops(other.m_ops), m_invoke(other.m_invoke)
    {
        if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
        other.m_ops    = 0;
        other.m_invoke = 0;
        return;
    }

    move_only_function& operator=(move_only_function&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            m_ops    = other.m_ops;
            m_invoke = other.m_invoke;
            if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
            other.m_ops    = 0;
            other.m_invoke = 0;
        }
        return *this;
    }

    move_only_function(const move_only_function&)            = delete;
    move_only_function& operator=(const move_only_function&) = delete;

    ~move_only_function() { reset(); }

    void reset() D_NOEXCEPT
    {
        if (m_ops) { m_ops->destroy(m_buffer); m_ops = 0; m_invoke = 0; }
        return;
    }

    void swap(move_only_function& other) D_NOEXCEPT
    {
        move_only_function tmp(static_cast<move_only_function&&>(*this));
        *this = static_cast<move_only_function&&>(other);
        other = static_cast<move_only_function&&>(tmp);
        return;
    }

    explicit operator bool() const D_NOEXCEPT { return m_ops != 0; }

    //   Undefined when empty, per std - see the header note.
    _Result operator()(_Args... args) D_NOEXCEPT
    {
        return m_invoke(
            const_cast<internal::func_buffer&>(this->m_buffer),
            static_cast<_Args&&>(args)...);
    }
};

// move_only_function<_Result(_Args...) const noexcept>
//   class: target invoked as const the target&.
template<typename _Result, typename... _Args>
class move_only_function<_Result(_Args...) const noexcept>
{
    typedef _Result (*_Invoker)(internal::func_buffer&, _Args&&...);

    internal::func_buffer   m_buffer;
    const internal::func_ops* m_ops;
    _Invoker                m_invoke;

    template<typename _Target>
    static _Result invoke_target(internal::func_buffer& buffer,
                                 _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            static_cast<const _Target&>(
                internal::func_manager<_Target>::get(buffer)),
            static_cast<_Args&&>(args)...));
    }

public:
    typedef _Result result_type;

    move_only_function() D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}
    move_only_function(decltype(nullptr)) D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}

    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, move_only_function>::value,
                 int>::type = 0>
    move_only_function(_Func&& func)
        : m_buffer(), m_ops(0), m_invoke(0)
    {
        typedef typename decay<_Func>::type _Target;
        internal::func_manager<_Target>::construct(
            m_buffer, static_cast<_Func&&>(func));
        m_ops    = &internal::func_ops_holder<_Target, false>::value;
        m_invoke = &invoke_target<_Target>;
        return;
    }

    move_only_function(move_only_function&& other) D_NOEXCEPT
        : m_buffer(), m_ops(other.m_ops), m_invoke(other.m_invoke)
    {
        if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
        other.m_ops    = 0;
        other.m_invoke = 0;
        return;
    }

    move_only_function& operator=(move_only_function&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            m_ops    = other.m_ops;
            m_invoke = other.m_invoke;
            if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
            other.m_ops    = 0;
            other.m_invoke = 0;
        }
        return *this;
    }

    move_only_function(const move_only_function&)            = delete;
    move_only_function& operator=(const move_only_function&) = delete;

    ~move_only_function() { reset(); }

    void reset() D_NOEXCEPT
    {
        if (m_ops) { m_ops->destroy(m_buffer); m_ops = 0; m_invoke = 0; }
        return;
    }

    void swap(move_only_function& other) D_NOEXCEPT
    {
        move_only_function tmp(static_cast<move_only_function&&>(*this));
        *this = static_cast<move_only_function&&>(other);
        other = static_cast<move_only_function&&>(tmp);
        return;
    }

    explicit operator bool() const D_NOEXCEPT { return m_ops != 0; }

    //   Undefined when empty, per std - see the header note.
    _Result operator()(_Args... args) const D_NOEXCEPT
    {
        return m_invoke(
            const_cast<internal::func_buffer&>(this->m_buffer),
            static_cast<_Args&&>(args)...);
    }
};

// move_only_function<_Result(_Args...) & noexcept>
//   class: target invoked as the target&.
template<typename _Result, typename... _Args>
class move_only_function<_Result(_Args...) & noexcept>
{
    typedef _Result (*_Invoker)(internal::func_buffer&, _Args&&...);

    internal::func_buffer   m_buffer;
    const internal::func_ops* m_ops;
    _Invoker                m_invoke;

    template<typename _Target>
    static _Result invoke_target(internal::func_buffer& buffer,
                                 _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            static_cast<_Target&>(
                internal::func_manager<_Target>::get(buffer)),
            static_cast<_Args&&>(args)...));
    }

public:
    typedef _Result result_type;

    move_only_function() D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}
    move_only_function(decltype(nullptr)) D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}

    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, move_only_function>::value,
                 int>::type = 0>
    move_only_function(_Func&& func)
        : m_buffer(), m_ops(0), m_invoke(0)
    {
        typedef typename decay<_Func>::type _Target;
        internal::func_manager<_Target>::construct(
            m_buffer, static_cast<_Func&&>(func));
        m_ops    = &internal::func_ops_holder<_Target, false>::value;
        m_invoke = &invoke_target<_Target>;
        return;
    }

    move_only_function(move_only_function&& other) D_NOEXCEPT
        : m_buffer(), m_ops(other.m_ops), m_invoke(other.m_invoke)
    {
        if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
        other.m_ops    = 0;
        other.m_invoke = 0;
        return;
    }

    move_only_function& operator=(move_only_function&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            m_ops    = other.m_ops;
            m_invoke = other.m_invoke;
            if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
            other.m_ops    = 0;
            other.m_invoke = 0;
        }
        return *this;
    }

    move_only_function(const move_only_function&)            = delete;
    move_only_function& operator=(const move_only_function&) = delete;

    ~move_only_function() { reset(); }

    void reset() D_NOEXCEPT
    {
        if (m_ops) { m_ops->destroy(m_buffer); m_ops = 0; m_invoke = 0; }
        return;
    }

    void swap(move_only_function& other) D_NOEXCEPT
    {
        move_only_function tmp(static_cast<move_only_function&&>(*this));
        *this = static_cast<move_only_function&&>(other);
        other = static_cast<move_only_function&&>(tmp);
        return;
    }

    explicit operator bool() const D_NOEXCEPT { return m_ops != 0; }

    //   Undefined when empty, per std - see the header note.
    _Result operator()(_Args... args) & D_NOEXCEPT
    {
        return m_invoke(
            const_cast<internal::func_buffer&>(this->m_buffer),
            static_cast<_Args&&>(args)...);
    }
};

// move_only_function<_Result(_Args...) const & noexcept>
//   class: target invoked as const the target&.
template<typename _Result, typename... _Args>
class move_only_function<_Result(_Args...) const & noexcept>
{
    typedef _Result (*_Invoker)(internal::func_buffer&, _Args&&...);

    internal::func_buffer   m_buffer;
    const internal::func_ops* m_ops;
    _Invoker                m_invoke;

    template<typename _Target>
    static _Result invoke_target(internal::func_buffer& buffer,
                                 _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            static_cast<const _Target&>(
                internal::func_manager<_Target>::get(buffer)),
            static_cast<_Args&&>(args)...));
    }

public:
    typedef _Result result_type;

    move_only_function() D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}
    move_only_function(decltype(nullptr)) D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}

    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, move_only_function>::value,
                 int>::type = 0>
    move_only_function(_Func&& func)
        : m_buffer(), m_ops(0), m_invoke(0)
    {
        typedef typename decay<_Func>::type _Target;
        internal::func_manager<_Target>::construct(
            m_buffer, static_cast<_Func&&>(func));
        m_ops    = &internal::func_ops_holder<_Target, false>::value;
        m_invoke = &invoke_target<_Target>;
        return;
    }

    move_only_function(move_only_function&& other) D_NOEXCEPT
        : m_buffer(), m_ops(other.m_ops), m_invoke(other.m_invoke)
    {
        if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
        other.m_ops    = 0;
        other.m_invoke = 0;
        return;
    }

    move_only_function& operator=(move_only_function&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            m_ops    = other.m_ops;
            m_invoke = other.m_invoke;
            if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
            other.m_ops    = 0;
            other.m_invoke = 0;
        }
        return *this;
    }

    move_only_function(const move_only_function&)            = delete;
    move_only_function& operator=(const move_only_function&) = delete;

    ~move_only_function() { reset(); }

    void reset() D_NOEXCEPT
    {
        if (m_ops) { m_ops->destroy(m_buffer); m_ops = 0; m_invoke = 0; }
        return;
    }

    void swap(move_only_function& other) D_NOEXCEPT
    {
        move_only_function tmp(static_cast<move_only_function&&>(*this));
        *this = static_cast<move_only_function&&>(other);
        other = static_cast<move_only_function&&>(tmp);
        return;
    }

    explicit operator bool() const D_NOEXCEPT { return m_ops != 0; }

    //   Undefined when empty, per std - see the header note.
    _Result operator()(_Args... args) const & D_NOEXCEPT
    {
        return m_invoke(
            const_cast<internal::func_buffer&>(this->m_buffer),
            static_cast<_Args&&>(args)...);
    }
};

// move_only_function<_Result(_Args...) && noexcept>
//   class: target invoked as the target&&.
template<typename _Result, typename... _Args>
class move_only_function<_Result(_Args...) && noexcept>
{
    typedef _Result (*_Invoker)(internal::func_buffer&, _Args&&...);

    internal::func_buffer   m_buffer;
    const internal::func_ops* m_ops;
    _Invoker                m_invoke;

    template<typename _Target>
    static _Result invoke_target(internal::func_buffer& buffer,
                                 _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            static_cast<_Target&&>(
                internal::func_manager<_Target>::get(buffer)),
            static_cast<_Args&&>(args)...));
    }

public:
    typedef _Result result_type;

    move_only_function() D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}
    move_only_function(decltype(nullptr)) D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}

    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, move_only_function>::value,
                 int>::type = 0>
    move_only_function(_Func&& func)
        : m_buffer(), m_ops(0), m_invoke(0)
    {
        typedef typename decay<_Func>::type _Target;
        internal::func_manager<_Target>::construct(
            m_buffer, static_cast<_Func&&>(func));
        m_ops    = &internal::func_ops_holder<_Target, false>::value;
        m_invoke = &invoke_target<_Target>;
        return;
    }

    move_only_function(move_only_function&& other) D_NOEXCEPT
        : m_buffer(), m_ops(other.m_ops), m_invoke(other.m_invoke)
    {
        if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
        other.m_ops    = 0;
        other.m_invoke = 0;
        return;
    }

    move_only_function& operator=(move_only_function&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            m_ops    = other.m_ops;
            m_invoke = other.m_invoke;
            if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
            other.m_ops    = 0;
            other.m_invoke = 0;
        }
        return *this;
    }

    move_only_function(const move_only_function&)            = delete;
    move_only_function& operator=(const move_only_function&) = delete;

    ~move_only_function() { reset(); }

    void reset() D_NOEXCEPT
    {
        if (m_ops) { m_ops->destroy(m_buffer); m_ops = 0; m_invoke = 0; }
        return;
    }

    void swap(move_only_function& other) D_NOEXCEPT
    {
        move_only_function tmp(static_cast<move_only_function&&>(*this));
        *this = static_cast<move_only_function&&>(other);
        other = static_cast<move_only_function&&>(tmp);
        return;
    }

    explicit operator bool() const D_NOEXCEPT { return m_ops != 0; }

    //   Undefined when empty, per std - see the header note.
    _Result operator()(_Args... args) && D_NOEXCEPT
    {
        return m_invoke(
            const_cast<internal::func_buffer&>(this->m_buffer),
            static_cast<_Args&&>(args)...);
    }
};

// move_only_function<_Result(_Args...) const && noexcept>
//   class: target invoked as const the target&&.
template<typename _Result, typename... _Args>
class move_only_function<_Result(_Args...) const && noexcept>
{
    typedef _Result (*_Invoker)(internal::func_buffer&, _Args&&...);

    internal::func_buffer   m_buffer;
    const internal::func_ops* m_ops;
    _Invoker                m_invoke;

    template<typename _Target>
    static _Result invoke_target(internal::func_buffer& buffer,
                                 _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            static_cast<const _Target&&>(
                internal::func_manager<_Target>::get(buffer)),
            static_cast<_Args&&>(args)...));
    }

public:
    typedef _Result result_type;

    move_only_function() D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}
    move_only_function(decltype(nullptr)) D_NOEXCEPT : m_buffer(), m_ops(0), m_invoke(0) {}

    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, move_only_function>::value,
                 int>::type = 0>
    move_only_function(_Func&& func)
        : m_buffer(), m_ops(0), m_invoke(0)
    {
        typedef typename decay<_Func>::type _Target;
        internal::func_manager<_Target>::construct(
            m_buffer, static_cast<_Func&&>(func));
        m_ops    = &internal::func_ops_holder<_Target, false>::value;
        m_invoke = &invoke_target<_Target>;
        return;
    }

    move_only_function(move_only_function&& other) D_NOEXCEPT
        : m_buffer(), m_ops(other.m_ops), m_invoke(other.m_invoke)
    {
        if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
        other.m_ops    = 0;
        other.m_invoke = 0;
        return;
    }

    move_only_function& operator=(move_only_function&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            m_ops    = other.m_ops;
            m_invoke = other.m_invoke;
            if (m_ops) { m_ops->move(m_buffer, other.m_buffer); }
            other.m_ops    = 0;
            other.m_invoke = 0;
        }
        return *this;
    }

    move_only_function(const move_only_function&)            = delete;
    move_only_function& operator=(const move_only_function&) = delete;

    ~move_only_function() { reset(); }

    void reset() D_NOEXCEPT
    {
        if (m_ops) { m_ops->destroy(m_buffer); m_ops = 0; m_invoke = 0; }
        return;
    }

    void swap(move_only_function& other) D_NOEXCEPT
    {
        move_only_function tmp(static_cast<move_only_function&&>(*this));
        *this = static_cast<move_only_function&&>(other);
        other = static_cast<move_only_function&&>(tmp);
        return;
    }

    explicit operator bool() const D_NOEXCEPT { return m_ops != 0; }

    //   Undefined when empty, per std - see the header note.
    _Result operator()(_Args... args) const && D_NOEXCEPT
    {
        return m_invoke(
            const_cast<internal::func_buffer&>(this->m_buffer),
            static_cast<_Args&&>(args)...);
    }
};

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_FUNCTIONAL_MOVE_ONLY_FUNCTION_
