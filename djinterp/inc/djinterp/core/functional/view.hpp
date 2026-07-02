/******************************************************************************
* djinterp [functional]                                               view.hpp
*
* Lazy, pull-based views and a pipeline operator for functional dataflow (C++).
*   A view is a non-owning, lazily-evaluated wrapper around a sequence of
* values. Views compose via operator| to form a pipeline: each adapter in
* the chain transforms its input view into a new view without eagerly
* materializing intermediate results. The pipeline only does work when
* something forces it: iteration, a terminal operator like to_vector(), or
* a count().
*
*   View composition is the connective tissue of the functional module. It
* lets producers, transformers, filters, and accumulators be chained
* together in a single expression that reads top-to-bottom, with full
* compile-time type checking and no runtime overhead beyond an iterator
* tag.
*
*   The pipeline operator | is overloaded for (view | adapter), where
* `adapter` is a factory object built by one of the functions in
* `namespace views`.  It also accepts a container on the left, which is
* implicitly lifted to a ref_view first.
*
* USAGE:
*   std::vector<int> v = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
*
*   auto result = v
*               | filter([](int x) { return x % 2 == 0; })
*               | transform([](int x) { return x * x; })
*               | take(3)
*               | to_vector();
*   // result == { 4, 16, 36 }
*
*   // infinite source + bounded sink
*   auto fibs = iota(0)
*             | take(20)
*             | to_vector();
*
*   // enumerate
*   for (auto&& p : v | enumerate())
*   {
*       std::cout << p.first << ": " << p.second << '\n';
*   }
*
* 
* path:      /inc/djinterp/core/functional/view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    VIEW BASE AND TRAITS
      1.  view_base                              (CRTP marker)
      2.  is_view<T>                             (SFINAE detector)
      3.  has_begin_end<T>                        (container-like detector)
II.   FUNDAMENTAL VIEWS
      1.  ref_view<_Container>                   (non-owning wrap)
      2.  owning_view<_Container>                (by-value capture)
      3.  iterator_pair_view<_Iterator>                (raw iterator pair)
III.  SOURCE VIEWS (no input)
      1.  iota_view<_Int>                        (infinite numeric source)
      2.  repeat_view<_Type>                     (infinite same-value source)
      3.  generate_view<_F>                      (call f() forever)
      4.  empty_view<_Type>                      (zero-element source)
      5.  single_view<_Type>                     (one-element source)
IV.   ADAPTER VIEWS
      1.  transform_view<_V, _F>
      2.  filter_view<_V, _P>
      3.  take_view<_V>
      4.  drop_view<_V>
      5.  take_while_view<_V, _P>
      6.  drop_while_view<_V, _P>
      7.  enumerate_view<_V>                     (yields (size_t, T))
      8.  zip_view<_V1, _V2>                     (pairs from two views)
      9.  concat_view<_V1, _V2>                  (sequence)
      10. reverse_view<_V>                       (bidirectional req'd)
      11. chunk_view<_V>                         (groups of N as vector<T>)
      12. stride_view<_V>                        (every N-th element)
V.    ADAPTER FACTORIES   (namespace views)
      1.  transform(f), filter(p)
      2.  take(n), drop(n), take_while(p), drop_while(p)
      3.  enumerate(), zip(other)
      4.  concat(other), reverse()
      5.  chunk(n), stride(n)
      6.  iota(start) / iota(start, end)
      7.  repeat(value) / repeat_n(value, n)
      8.  generate(f), empty<T>(), single(v)
VI.   PIPELINE OPERATORS
      1.  operator|(view, adapter)
      2.  operator|(container, adapter)
VII.  TERMINAL OPERATORS
      1.  to_vector()
      2.  to<C>()
      3.  count()
      4.  fold(init, f)
      5.  for_each(f)
      6.  reduce(f)                              (fold w/o init using first)
      7.  any_of(p), all_of(p), none_of(p)
      8.  find_if(p)
      9.  min_element(), max_element()
VIII. VIEW SFINAE STRUCTURAL TRAITS & CONCEPTS
      1.  view_value_type<V>                     (element-type extractor)
      2.  is_pipeable_to_view<T>                 (view OR container)
      3.  is_view_v / has_begin_end_v /
          is_adapter_v / is_terminal_v /
          is_pipeable_to_view_v                  (variable-template shorthands)
      4.  view_type / view_adapter / view_terminal /
          pipeable_to_view                       (C++20 concept parallels)
*/

#ifndef DJINTERP_FUNCTIONAL_VIEW_
#define DJINTERP_FUNCTIONAL_VIEW_ 1

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./functor.hpp"
#include "./foldable.hpp"


NS_DJINTERP

//   DUAL DOMAIN (boundary).  A view is a lazy, iterator-based pipeline over a
// runtime sequence; forcing it - iteration, to_vector(), count() - is a RUNTIME
// act.  COMPOSITION lifts to compile time: the operator| chain that assembles a
// view is built from D_CONSTEXPR-constructible adapter objects (see the ctors
// throughout this file), so the pipeline shape is fixed during translation even
// though the traversal that does the work runs later.  A view is therefore the
// value-domain RUNTIME face of the dataflow.  The COMPILE-TIME face of the very
// same map / filter / take vocabulary is the reduction substrate: reduce_ct and
// the unfold_ct driver folding a value_list (see reduce.hpp and producer.hpp),
// with transducers as the bridge that lets one transformation chain target
// either face without being rewritten.

///////////////////////////////////////////////////////////////////////////////
///             I.    VIEW BASE AND TRAITS                                  ///
///////////////////////////////////////////////////////////////////////////////

// view_base
//   struct: CRTP marker that all view types inherit from. Allows
// SFINAE-based detection via is_view<T> without an intrusive trait.
// Empty by design; views supply their own iterator interface.
template<typename _Derived>
struct view_base
{};


NS_INTERNAL
    // is_view_helper
    //   helper: detects if _Type publicly inherits view_base<_Type>.
    // SFINAE-based: the test() overload taking view_base<_Type>* is
    // chosen when _Type derives from view_base<_Type>; otherwise the
    // ellipsis overload wins.
    template<typename _Type>
    struct is_view_helper
    {
    private:
        template<typename _U>
        static std::true_type  test(const view_base<_U>*);
        static std::false_type test(...);

    public:
        using type = decltype(test(static_cast<_Type*>(nullptr)));
    };

NS_END  // internal


// is_view
//   trait: true if _Type is (or inherits from) a view_base
// specialization. Used to SFINAE-constrain operator| so it only
// fires on views (and, separately, containers).
template<typename _Type>
struct is_view
    : internal::is_view_helper<typename std::decay<_Type>::type>::type
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    static constexpr bool is_view_v = is_view<_Type>::value;
#endif


NS_INTERNAL
    // has_begin_end_helper
    //   helper: SFINAE-detects whether _Type has std::begin/std::end.
    // Used to identify "container-like" types for implicit lifting to
    // ref_view in the pipeline operator.
    template<typename _Type>
    struct has_begin_end_helper
    {
    private:
        template<typename _U>
        static auto test(int) -> decltype(
            std::begin(std::declval<_U&>()),
            std::end(std::declval<_U&>()),
            std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// has_begin_end
//   trait: detects container-like types (anything supporting
// std::begin/std::end).
template<typename _Type>
struct has_begin_end
    : internal::has_begin_end_helper<typename std::decay<_Type>::type>::type
{};


///////////////////////////////////////////////////////////////////////////////
///             II.   FUNDAMENTAL VIEWS                                     ///
///////////////////////////////////////////////////////////////////////////////

// ref_view
//   class: non-owning view over a container. Stores a pointer to the
// underlying container; iteration delegates to the container's
// own iterators. The container must outlive any ref_view referring
// to it.
template<typename _Container>
class ref_view : public view_base<ref_view<_Container>>
{
public:
    using container_type  = _Container;
    using iterator        = typename _Container::const_iterator;
    using const_iterator  = iterator;
    using value_type      = typename _Container::value_type;
    using reference       = typename _Container::const_reference;
    using size_type       = typename _Container::size_type;

    explicit D_CONSTEXPR ref_view(
        const _Container& _container
    ) noexcept
        : m_container(&_container)
    {}

    D_NODISCARD     iterator begin() const
    {
        return m_container->begin();
    }

    D_NODISCARD     iterator end() const
    {
        return m_container->end();
    }

private:
    const _Container* m_container;
};


// owning_view
//   class: by-value view that takes ownership of a container. Used
// when a temporary or rvalue is piped into the view machinery and
// the underlying data must survive the expression.
template<typename _Container>
class owning_view : public view_base<owning_view<_Container>>
{
public:
    using container_type  = _Container;
    using iterator        = typename _Container::const_iterator;
    using const_iterator  = iterator;
    using value_type      = typename _Container::value_type;
    using reference       = typename _Container::const_reference;
    using size_type       = typename _Container::size_type;

    template<typename _ContainerFwd>
    explicit D_CONSTEXPR owning_view(
        _ContainerFwd&& _container
    )
        : m_container(std::forward<_ContainerFwd>(_container))
    {}

    D_NODISCARD iterator
    begin() const
    {
        return m_container.begin();
    }

    D_NODISCARD iterator
    end() const
    {
        return m_container.end();
    }

private:
    _Container m_container;
};


// iterator_pair_view
//   class: view defined by a pair of iterators (begin, end). Useful
// for adapting arbitrary iterator ranges (e.g. from a third-party
// container or a substring of a sequence) to the view machinery.
template<typename _Iterator>
class iterator_pair_view : public view_base<iterator_pair_view<_Iterator>>
{
public:
    using iterator       = _Iterator;
    using const_iterator = _Iterator;
    using value_type     = typename std::iterator_traits<_Iterator>::value_type;
    using reference      = typename std::iterator_traits<_Iterator>::reference;

    D_CONSTEXPR
    iterator_pair_view(
        _Iterator _first,
        _Iterator _last
    )
        : m_first(_first),
          m_last(_last)
    {}

    D_NODISCARD     iterator begin() const
    {
        return m_first;
    }

    D_NODISCARD     iterator end() const
    {
        return m_last;
    }

private:
    _Iterator m_first;
    _Iterator m_last;
};


///////////////////////////////////////////////////////////////////////////////
///             III.  SOURCE VIEWS                                          ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // iota_iterator
    //   helper: forward iterator over a half-open numeric range, or
    // an infinite ascending sequence when no end sentinel is given.
    // For the infinite variant, the "end" iterator stores a sentinel
    // flag and compares unequal to any in-range iterator.
    template<typename _Int>
    class iota_iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = _Int;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const _Int*;
        using reference         = _Int;

        D_CONSTEXPR
        iota_iterator()
            : m_value(_Int()),
              m_end(_Int()),
              m_has_end(false),
              m_is_end(true)
        {}

        D_CONSTEXPR iota_iterator(
            _Int _value,
            bool _is_end
        )
            : m_value(_value),
              m_end(_Int()),
              m_has_end(false),
              m_is_end(_is_end)
        {}

        D_CONSTEXPR iota_iterator(
            _Int _value,
            _Int _end,
            bool _is_end
        )
            : m_value(_value),
              m_end(_end),
              m_has_end(true),
              m_is_end(_is_end)
        {}

        D_CONSTEXPR _Int
        operator*() const
        {
            return m_value;
        }

        iota_iterator&
        operator++()
        {
            ++m_value;

            if (m_has_end && !(m_value < m_end))
            {
                m_is_end = true;
            }

            return *this;
        }

        iota_iterator
        operator++(
            int
        )
        {
            iota_iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        D_CONSTEXPR bool
        operator==(
            const iota_iterator& _other
        ) const
        {
            return ( (m_is_end == _other.m_is_end) &&
                     ( m_is_end || (m_value == _other.m_value) ) );
        }

        D_CONSTEXPR bool
        operator!=(
            const iota_iterator& _other
        ) const
        {
            return !(*this == _other);
        }

    private:
        _Int m_value;
        _Int m_end;
        bool m_has_end;
        bool m_is_end;
    };

NS_END  // internal


// iota_view
//   class: view over a numeric range, either bounded
// [_start, _end) or infinite [_start, ...). The infinite variant
// becomes finite when combined with take or take_while.
template<typename _Int>
class iota_view : public view_base<iota_view<_Int>>
{
public:
    using iterator       = internal::iota_iterator<_Int>;
    using const_iterator = iterator;
    using value_type     = _Int;
    using reference      = _Int;

    // constructor (bounded)
    D_CONSTEXPR
    iota_view(
        _Int _start,
        _Int _end
    )
        : m_start(_start)
        , m_end(_end)
        , m_has_end(true)
    {}

    // constructor (infinite)
    explicit D_CONSTEXPR
    iota_view(
        _Int _start
    )
        : m_start(_start)
        , m_end(_Int())
        , m_has_end(false)
    {}

    D_NODISCARD     iterator begin() const
    {
        if (m_has_end)
        {
            return iterator(m_start, m_end, !(m_start < m_end));
        }

        return iterator(m_start, false);
    }

    D_NODISCARD     iterator end() const
    {
        if (m_has_end)
        {
            return iterator(m_end, m_end, true);
        }

        return iterator(_Int(), true);
    }

private:
    _Int m_start;
    _Int m_end;
    bool m_has_end;
};


NS_INTERNAL
    // repeat_iterator
    //   helper: iterator that emits the same stored value forever (or
    // until a count is reached). For the unbounded variant the end
    // iterator is a sentinel that never compares equal to a live one.
    template<typename _Type>
    class repeat_iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = _Type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const _Type*;
        using reference         = const _Type&;

        D_CONSTEXPR
        repeat_iterator()
            : m_value(_Type())
            , m_remaining(0)
            , m_has_bound(false)
            , m_is_end(true)
        {}

        D_CONSTEXPR
        repeat_iterator(
            _Type          _value,
            std::size_t _remaining,
            bool        _has_bound,
            bool        _is_end
        )
            : m_value(std::move(_value))
            , m_remaining(_remaining)
            , m_has_bound(_has_bound)
            , m_is_end(_is_end)
        {}

        D_CONSTEXPR
        const _Type& operator*() const
        {
            return m_value;
        }

        repeat_iterator& operator++()
        {
            if (m_has_bound)
            {
                if (m_remaining > 0)
                {
                    --m_remaining;
                }

                if (m_remaining == 0)
                {
                    m_is_end = true;
                }
            }

            return *this;
        }

        repeat_iterator operator++(int)
        {
            repeat_iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        D_CONSTEXPR
        bool operator==(
            const repeat_iterator& _other
        ) const
        {
            return (m_is_end == _other.m_is_end);
        }

        D_CONSTEXPR
        bool operator!=(
            const repeat_iterator& _other
        ) const
        {
            return !(*this == _other);
        }

    private:
        _Type          m_value;
        std::size_t m_remaining;
        bool        m_has_bound;
        bool        m_is_end;
    };

NS_END  // internal


// repeat_view
//   class: view that yields the same value indefinitely, or a
// bounded number of copies if _n is supplied.
template<typename _Type>
class repeat_view : public view_base<repeat_view<_Type>>
{
public:
    using iterator       = internal::repeat_iterator<_Type>;
    using const_iterator = iterator;
    using value_type     = _Type;
    using reference      = const _Type&;

    // constructor (infinite)
    explicit D_CONSTEXPR
    repeat_view(
        _Type _value
    )
        : m_value(std::move(_value))
        , m_n(0)
        , m_has_bound(false)
    {}

    // constructor (bounded)
    D_CONSTEXPR
    repeat_view(
        _Type          _value,
        std::size_t _n
    )
        : m_value(std::move(_value))
        , m_n(_n)
        , m_has_bound(true)
    {}

    D_NODISCARD     iterator begin() const
    {
        if (m_has_bound)
        {
            return iterator(m_value, m_n, true, (m_n == 0));
        }

        return iterator(m_value, 0, false, false);
    }

    D_NODISCARD     iterator end() const
    {
        return iterator(m_value, 0, m_has_bound, true);
    }

private:
    _Type          m_value;
    std::size_t m_n;
    bool        m_has_bound;
};


NS_INTERNAL
    // generate_iterator
    //   helper: iterator that invokes a nullary function on each
    // dereference (well, on each advance) and caches the result.
    // Treated as infinite; pair with take or take_while to bound.
    template<typename _F>
    class generate_iterator
    {
    public:
        using value_type        = typename std::decay<decltype(
            std::declval<_F&>()())>::type;
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        // end-sentinel constructor
        D_CONSTEXPR
        generate_iterator()
            : m_fn_ptr(nullptr)
            , m_cached()
            , m_is_end(true)
        {}

        // live constructor
        explicit
        generate_iterator(
            _F& _fn
        )
            : m_fn_ptr(&_fn)
            , m_cached(_fn())
            , m_is_end(false)
        {}

        const value_type& operator*() const
        {
            return m_cached;
        }

        generate_iterator& operator++()
        {
            m_cached = (*m_fn_ptr)();

            return *this;
        }

        generate_iterator operator++(int)
        {
            generate_iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        D_CONSTEXPR
        bool operator==(
            const generate_iterator& _other
        ) const
        {
            return (m_is_end == _other.m_is_end);
        }

        D_CONSTEXPR
        bool operator!=(
            const generate_iterator& _other
        ) const
        {
            return !(*this == _other);
        }

    private:
        _F*        m_fn_ptr;
        value_type m_cached;
        bool       m_is_end;
    };

NS_END  // internal


// generate_view
//   class: infinite view in which every position is computed by
// calling a stored nullary function. The function is held mutably
// so generators with internal state (e.g. random number engines)
// work correctly.
template<typename _F>
class generate_view : public view_base<generate_view<_F>>
{
public:
    using iterator       = internal::generate_iterator<_F>;
    using const_iterator = iterator;
    using value_type     = typename iterator::value_type;
    using reference      = const value_type&;

    template<typename _FFwd>
    explicit D_CONSTEXPR
    generate_view(
        _FFwd&& _fn
    )
        : m_fn(std::forward<_FFwd>(_fn))
    {}

    iterator begin()
    {
        return iterator(m_fn);
    }

    iterator end() const
    {
        return iterator{};
    }

    // const begin/end are not provided since the function is invoked
    // on advance, mutating its state. Callers that need const access
    // should materialize via to_vector() first.

private:
    mutable _F m_fn;
};


// empty_view
//   class: view containing zero elements. Iterator type is a trivial
// sentinel that always compares equal to itself.
template<typename _Type>
class empty_view : public view_base<empty_view<_Type>>
{
public:
    using value_type     = _Type;
    using reference      = const _Type&;

    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = _Type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const _Type*;
        using reference         = const _Type&;

        // not D_CONSTEXPR: unreachable precondition-violation path that
        // returns a reference to a function-local static, which is not
        // permitted in a constexpr function before C++23. (fixed 2026-05-30)
        const _Type& operator*() const
        {
            static const _Type sentinel = _Type();

            return sentinel;
        }

        iterator& operator++()
        {
            return *this;
        }

        D_CONSTEXPR
        bool operator==(const iterator&) const
        {
            return true;
        }

        D_CONSTEXPR
        bool operator!=(const iterator&) const
        {
            return false;
        }
    };

    using const_iterator = iterator;

    D_NODISCARD     iterator begin() const
    {
        return iterator{};
    }

    D_NODISCARD     iterator end() const
    {
        return iterator{};
    }
};


// single_view
//   class: view containing exactly one element. The element is
// stored by value.
template<typename _Type>
class single_view : public view_base<single_view<_Type>>
{
public:
    using value_type = _Type;
    using reference  = const _Type&;

    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = _Type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const _Type*;
        using reference         = const _Type&;

        D_CONSTEXPR iterator()
            : m_ptr(nullptr),
              m_is_end(true)
        {}

        D_CONSTEXPR explicit iterator(
            const _Type* _ptr
        )
            : m_ptr(_ptr),
              m_is_end(false)
        {}

        D_CONSTEXPR const _Type&
        operator*() const
        {
            return *m_ptr;
        }

        iterator&
        operator++()
        {
            m_is_end = true;

            return *this;
        }

        iterator 
        operator++(int)
        {
            iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        D_CONSTEXPR bool
        operator==(
            const iterator& _other
        ) const
        {
            return (m_is_end == _other.m_is_end);
        }

        D_CONSTEXPR bool
        operator!=(
            const iterator& _other
        ) const
        {
            return !(*this == _other);
        }

    private:
        const _Type* m_ptr;
        bool      m_is_end;
    };

    using const_iterator = iterator;

    explicit D_CONSTEXPR
    single_view(
        _Type _value
    )
        : m_value(std::move(_value))
    {}

    D_NODISCARD iterator
    begin() const
    {
        return iterator(&m_value);
    }

    D_NODISCARD iterator
    end() const
    {
        return iterator{};
    }

private:
    _Type m_value;
};


///////////////////////////////////////////////////////////////////////////////
///             IV.   ADAPTER VIEWS                                         ///
///////////////////////////////////////////////////////////////////////////////

// transform_view
//   class: lazy view that applies _Function to each element of an
// inner view. The function is invoked on dereference; values are
// not cached, so the function should be cheap and pure for
// repeated reads.
template<typename _View,
         typename _Function>
class transform_view
    : public view_base<transform_view<_View, _Function>>
{
public:
    class iterator
    {
    public:
        using inner_iterator    = typename _View::const_iterator;
        using inner_reference   = typename std::iterator_traits<inner_iterator>::reference;
        using iterator_category = std::input_iterator_tag;
        using value_type        = typename std::decay<decltype(std::declval<const _Function&>()(std::declval<inner_reference>()))>::type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = void;
        using reference         = value_type;

        D_CONSTEXPR
        iterator(
            inner_iterator   _iterator,
            const _Function* _fn
        )
            : m_it(_iterator),
              m_fn(_fn)
        {}

        D_CONSTEXPR value_type
        operator*() const
        {
            return (*m_fn)(*m_it);
        }

        iterator&
        operator++()
        {
            ++m_it;

            return *this;
        }

        iterator 
        operator++(
            int
        )
        {
            iterator tmp(*this);
            ++m_it;

            return tmp;
        }

        D_CONSTEXPR bool
        operator==(
            const iterator& _other
        ) const
        {
            return (m_it == _other.m_it);
        }

        D_CONSTEXPR bool
        operator!=(
            const iterator& _other
        ) const
        {
            return (m_it != _other.m_it);
        }

    private:
        inner_iterator   m_it;
        const _Function* m_fn;
    };

    using const_iterator = iterator;
    using value_type     = typename iterator::value_type;
    using reference      = value_type;

    template<typename _VFwd,
             typename _FFwd>
    D_CONSTEXPR transform_view(
        _VFwd&& _view,
        _FFwd&& _function
    )
        : m_view(std::forward<_VFwd>(_view)),
          m_function(std::forward<_FFwd>(_function))
    {}

    D_NODISCARD iterator
    begin() const
    {
        return iterator(m_view.begin(), &m_function);
    }

    D_NODISCARD iterator
    end() const
    {
        return iterator(m_view.end(), &m_function);
    }

private:
    _View     m_view;
    _Function m_function;
};


// filter_view
//   class: lazy view that yields only those elements of an inner
// view satisfying _Predicate. Advancement scans through the inner
// iterator until a match is found (or end is reached).
template<typename _View,
         typename _Predicate>
class filter_view
    : public view_base<filter_view<_View, _Predicate>>
{
public:
    class iterator
    {
    public:
        using inner_iterator    = typename _View::const_iterator;
        using iterator_category = std::input_iterator_tag;
        using value_type        = typename std::iterator_traits<inner_iterator>::value_type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = typename std::iterator_traits<inner_iterator>::pointer;
        using reference         = typename std::iterator_traits<inner_iterator>::reference;

        iterator(
            inner_iterator    _iterator,
            inner_iterator    _end,
            const _Predicate* _predicate
        )
            : m_it(_iterator),
              m_end(_end),
              m_predicate(_predicate)
        {
            advance_to_match();
        }

        D_NODISCARD reference
        operator*() const
        {
            return *m_it;
        }

        iterator&
        operator++()
        {
            ++m_it;
            advance_to_match();

            return *this;
        }

        iterator
        operator++(
            int
        )
        {
            iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        D_CONSTEXPR bool
        operator==(
            const iterator& _other
        ) const
        {
            return (m_it == _other.m_it);
        }

        D_CONSTEXPR bool
        operator!=(
            const iterator& _other
        ) const
        {
            return (m_it != _other.m_it);
        }

    private:
        // advance_to_match
        //   skip non-matching elements; called from the constructor
        // and from each ++ to maintain the invariant that *m_it
        // satisfies the predicate (or m_it == m_end).
        void
        advance_to_match()
        {
            while ( (m_it != m_end) && 
                    !(*m_predicate)(*m_it) )
            {
                ++m_it;
            }

            return;
        }

        inner_iterator    m_it;
        inner_iterator    m_end;
        const _Predicate* m_predicate;
    };

    using const_iterator = iterator;
    using value_type     = typename iterator::value_type;
    using reference      = typename iterator::reference;

    template<typename _VFwd,
             typename _PFwd>
    D_CONSTEXPR filter_view(
        _VFwd&& _view,
        _PFwd&& _predicate
    )
        : m_view(std::forward<_VFwd>(_view)),
          m_predicate(std::forward<_PFwd>(_predicate))
    {}

    D_NODISCARD iterator
    begin() const
    {
        return iterator(m_view.begin(), m_view.end(), &m_predicate);
    }

    D_NODISCARD iterator
    end() const
    {
        return iterator(m_view.end(), m_view.end(), &m_predicate);
    }

private:
    _View      m_view;
    _Predicate m_predicate;
};


// take_view
//   class: lazy view that yields at most _n elements from an inner
// view. The iterator carries a remaining counter; when it reaches
// zero, the iterator compares equal to end().
template<typename _View>
class take_view 
    : public view_base<take_view<_View>>
{
public:
    class iterator
    {
    public:
        using inner_iterator    = typename _View::const_iterator;
        using iterator_category = std::input_iterator_tag;
        using value_type        = typename std::iterator_traits<inner_iterator>::value_type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = typename std::iterator_traits<inner_iterator>::pointer;
        using reference         = typename std::iterator_traits<inner_iterator>::reference;

        D_CONSTEXPR
        iterator(
            inner_iterator _iterator,
            inner_iterator _end,
            std::size_t    _remaining
        )
            : m_it(_iterator),
              m_end(_end),
              m_remaining(_remaining)
        {}

        D_NODISCARD reference
        operator*() const
        {
            return *m_it;
        }

        iterator& operator++()
        {
            if (m_remaining > 0)
            {
                ++m_it;
                --m_remaining;
            }

            return *this;
        }

        iterator operator++(
            int
        )
        {
            iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        D_CONSTEXPR bool
        operator==(
            const iterator& _other
        ) const
        {
            return ( ( (m_remaining == 0) || (m_it == m_end) )
                  ?  ( (_other.m_remaining == 0)  ||
                          (_other.m_it == _other.m_end) )
                  :   ( (m_it == _other.m_it)     &&
                        (m_remaining == _other.m_remaining) ) );
        }

        D_CONSTEXPR
        bool operator!=(
            const iterator& _other
        ) const
        {
            return !(*this == _other);
        }

    private:
        inner_iterator m_it;
        inner_iterator m_end;
        std::size_t    m_remaining;
    };

    using const_iterator = iterator;
    using value_type     = typename iterator::value_type;
    using reference      = typename iterator::reference;

    template<typename _VFwd>
    D_CONSTEXPR take_view(
        _VFwd&&     _view,
        std::size_t _n
    )
        : m_view(std::forward<_VFwd>(_view)),
          m_n(_n)
    {}

    D_NODISCARD iterator
    begin() const
    {
        return iterator(m_view.begin(), m_view.end(), m_n);
    }

    D_NODISCARD iterator
    end() const
    {
        return iterator(m_view.end(), m_view.end(), 0);
    }

private:
    _View       m_view;
    std::size_t m_n;
};


// drop_view
//   class: lazy view that skips the first _n elements of an inner
// view and yields the rest. The begin() call advances the iterator
// _n times eagerly (at most once per view instance, on each
// begin() call).
template<typename _View>
class drop_view : public view_base<drop_view<_View>>
{
public:
    using iterator       = typename _View::const_iterator;
    using const_iterator = iterator;
    using value_type     = typename std::iterator_traits<iterator>::value_type;
    using reference      = typename std::iterator_traits<
        iterator>::reference;

    template<typename _VFwd>
    D_CONSTEXPR drop_view(
        _VFwd&&     _view,
        std::size_t _n
    )
        : m_view(std::forward<_VFwd>(_view)),
          m_n(_n)
    {}

    D_NODISCARD iterator
    begin() const
    {
        iterator it = m_view.begin();
        iterator e  = m_view.end();
        std::size_t i = 0;

        while ((i < m_n) && (it != e))
        {
            ++it;
            ++i;
        }

        return it;
    }

    D_NODISCARD iterator
    end() const
    {
        return m_view.end();
    }

private:
    _View       m_view;
    std::size_t m_n;
};


// take_while_view
//   class: lazy view that yields elements while _Predicate holds,
// then stops. The iterator caches a "done" flag and short-circuits
// further reads once the predicate has failed.
template<typename _View,
         typename _Predicate>
class take_while_view
    : public view_base<take_while_view<_View, _Predicate>>
{
public:
    class iterator
    {
    public:
        using inner_iterator    = typename _View::const_iterator;
        using iterator_category = std::input_iterator_tag;
        using value_type        = typename std::iterator_traits<inner_iterator>::value_type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = typename std::iterator_traits<inner_iterator>::pointer;
        using reference         = typename std::iterator_traits<inner_iterator>::reference;

        iterator(
            inner_iterator    _iterator,
            inner_iterator    _end,
            const _Predicate* _predicate,
            bool              _is_end
        )
            : m_it(_iterator),
              m_end(_end),
              m_predicate(_predicate),
              m_is_end(_is_end)
        {
            recheck();
        }

        D_NODISCARD         reference operator*() const
        {
            return *m_it;
        }

        iterator& operator++()
        {
            ++m_it;
            recheck();

            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        D_CONSTEXPR
        bool operator==(
            const iterator& _other
        ) const
        {
            return ( (m_is_end == _other.m_is_end) &&
                     ( m_is_end || (m_it == _other.m_it) ) );
        }

        D_CONSTEXPR
        bool operator!=(
            const iterator& _other
        ) const
        {
            return !(*this == _other);
        }

    private:
        // recheck
        //   updates m_is_end based on the current position: true
        // if the inner iterator is at end, or if the predicate
        // currently fails.
        void recheck()
        {
            if (m_is_end)
            {
                return;
            }

            if (m_it == m_end)
            {
                m_is_end = true;

                return;
            }

            if (!(*m_predicate)(*m_it))
            {
                m_is_end = true;
            }

            return;
        }

        inner_iterator    m_it;
        inner_iterator    m_end;
        const _Predicate* m_predicate;
        bool              m_is_end;
    };

    using const_iterator = iterator;
    using value_type     = typename iterator::value_type;
    using reference      = typename iterator::reference;

    template<typename _VFwd,
             typename _PFwd>
    D_CONSTEXPR
    take_while_view(
        _VFwd&& _view,
        _PFwd&& _predicate
    )
        : m_view(std::forward<_VFwd>(_view)),
          m_predicate(std::forward<_PFwd>(_predicate))
    {}

    D_NODISCARD iterator
    begin() const
    {
        return iterator(
            m_view.begin(),
            m_view.end(),
            &m_predicate,
            false);
    }

    D_NODISCARD iterator
    end() const
    {
        return iterator(
            m_view.end(),
            m_view.end(),
            &m_predicate,
            true);
    }

private:
    _View      m_view;
    _Predicate m_predicate;
};


// drop_while_view
//   class: lazy view that skips initial elements satisfying
// _Predicate, then yields all subsequent elements unconditionally.
// begin() performs the skip eagerly on each call.
template<typename _View,
         typename _Predicate>
class drop_while_view
    : public view_base<drop_while_view<_View, _Predicate>>
{
public:
    using iterator       = typename _View::const_iterator;
    using const_iterator = iterator;
    using value_type     = typename std::iterator_traits<iterator>::value_type;
    using reference      = typename std::iterator_traits<iterator>::reference;

    template<typename _VFwd,
             typename _PFwd>
    D_CONSTEXPR
    drop_while_view(
        _VFwd&& _view,
        _PFwd&& _predicate
    )
        : m_view(std::forward<_VFwd>(_view)),
          m_predicate(std::forward<_PFwd>(_predicate))
    {}

    D_NODISCARD iterator
    begin() const
    {
        iterator it = m_view.begin();
        iterator e  = m_view.end();

        while ((it != e) && m_predicate(*it))
        {
            ++it;
        }

        return it;
    }

    D_NODISCARD iterator
    end() const
    {
        return m_view.end();
    }

private:
    _View      m_view;
    _Predicate m_predicate;
};


// enumerate_view
//   class: lazy view that yields std::pair<size_t, T> where the
// first element is the zero-based index. Useful for iterating with
// an index variable in the same expression.
template<typename _View>
class enumerate_view : public view_base<enumerate_view<_View>>
{
public:
    using inner_iterator  = typename _View::const_iterator;
    using inner_reference = typename std::iterator_traits<
        inner_iterator>::reference;
    using inner_value     = typename std::iterator_traits<
        inner_iterator>::value_type;

    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = std::pair<std::size_t, inner_value>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = void;
        using reference         = value_type;

        D_CONSTEXPR
        iterator(
            inner_iterator _iterator,
            std::size_t    _index
        )
            : m_it(_iterator)
            , m_index(_index)
        {}

        D_NODISCARD         value_type operator*() const
        {
            return value_type(m_index, *m_it);
        }

        iterator& operator++()
        {
            ++m_it;
            ++m_index;

            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        D_CONSTEXPR
        bool operator==(
            const iterator& _other
        ) const
        {
            return (m_it == _other.m_it);
        }

        D_CONSTEXPR
        bool operator!=(
            const iterator& _other
        ) const
        {
            return (m_it != _other.m_it);
        }

    private:
        inner_iterator m_it;
        std::size_t    m_index;
    };

    using const_iterator = iterator;
    using value_type     = typename iterator::value_type;
    using reference      = value_type;

    template<typename _VFwd>
    explicit D_CONSTEXPR enumerate_view(
        _VFwd&& _view
    )
        : m_view(std::forward<_VFwd>(_view))
    {}

    D_NODISCARD iterator
    begin() const
    {
        return iterator(m_view.begin(), 0);
    }

    D_NODISCARD iterator
    end() const
    {
        return iterator(m_view.end(), 0);
    }

private:
    _View m_view;
};


// zip_view
//   class: lazy view that yields pairs of elements from two inner
// views, in lockstep. Exhausts as soon as either inner view is
// exhausted.
template<typename _V1,
         typename _V2>
class zip_view : public view_base<zip_view<_V1, _V2>>
{
public:
    using inner_iterator_1 = typename _V1::const_iterator;
    using inner_iterator_2 = typename _V2::const_iterator;
    using inner_value_1    = typename std::iterator_traits<inner_iterator_1>::value_type;
    using inner_value_2    = typename std::iterator_traits<inner_iterator_2>::value_type;

    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = std::pair<inner_value_1, inner_value_2>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = void;
        using reference         = value_type;

        D_CONSTEXPR
        iterator(
            inner_iterator_1 _iterator1,
            inner_iterator_1 _end1,
            inner_iterator_2 _iterator2,
            inner_iterator_2 _end2
        )
            : m_it1(_iterator1),
              m_end1(_end1),
              m_it2(_iterator2),
              m_end2(_end2)
        {}

        D_NODISCARD value_type
        operator*() const
        {
            return value_type(*m_it1, *m_it2);
        }

        iterator&
        operator++()
        {
            ++m_it1;
            ++m_it2;

            return *this;
        }

        iterator
        operator++(int)
        {
            iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        // equality compares true once *either* side is at end.
        // This lets a single end iterator stand in for both possible
        // exhaustion conditions.
        D_CONSTEXPR bool
        operator==(
            const iterator& _other
        ) const
        {
            return ( ( (m_it1 == m_end1) || (m_it2 == m_end2) ) &&
                     ( (_other.m_it1 == _other.m_end1) ||
                       (_other.m_it2 == _other.m_end2) ) );
        }

        D_CONSTEXPR bool
        operator!=(
            const iterator& _other
        ) const
        {
            return !(*this == _other);
        }

    private:
        inner_iterator_1 m_it1;
        inner_iterator_1 m_end1;
        inner_iterator_2 m_it2;
        inner_iterator_2 m_end2;
    };

    using const_iterator = iterator;
    using value_type     = typename iterator::value_type;
    using reference      = value_type;

    template<typename _V1Fwd,
             typename _V2Fwd>
    D_CONSTEXPR
    zip_view(
        _V1Fwd&& _v1,
        _V2Fwd&& _v2
    )
        : m_v1(std::forward<_V1Fwd>(_v1)),
          m_v2(std::forward<_V2Fwd>(_v2))
    {}

    D_NODISCARD iterator
    begin() const
    {
        return iterator(
            m_v1.begin(), m_v1.end(),
            m_v2.begin(), m_v2.end());
    }

    D_NODISCARD iterator
    end() const
    {
        return iterator(
            m_v1.end(), m_v1.end(),
            m_v2.end(), m_v2.end());
    }

private:
    _V1 m_v1;
    _V2 m_v2;
};


// concat_view
//   class: lazy view that yields all elements of the first inner
// view, then all elements of the second. Both inner views must
// have the same value_type (or compatible).
template<typename _V1,
         typename _V2>
class concat_view : public view_base<concat_view<_V1, _V2>>
{
public:
    using inner_iterator_1 = typename _V1::const_iterator;
    using inner_iterator_2 = typename _V2::const_iterator;
    using inner_value_1    = typename std::iterator_traits<
        inner_iterator_1>::value_type;

    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = inner_value_1;
        using difference_type   = std::ptrdiff_t;
        using pointer           = void;
        using reference         = value_type;

        D_CONSTEXPR
        iterator(
            inner_iterator_1 _iterator1,
            inner_iterator_1 _end1,
            inner_iterator_2 _iterator2,
            inner_iterator_2 _end2,
            bool             _in_second
        )
            : m_it1(_iterator1),
              m_end1(_end1),
              m_it2(_iterator2),
              m_end2(_end2),
              m_in_second( (_in_second) || 
                           (_iterator1 == _end1))
        {}

        D_NODISCARD value_type
        operator*() const
        {
            if (m_in_second)
            {
                return *m_it2;
            }

            return *m_it1;
        }

        iterator& 
        operator++()
        {
            if (m_in_second)
            {
                ++m_it2;
            }
            else
            {
                ++m_it1;

                if (m_it1 == m_end1)
                {
                    m_in_second = true;
                }
            }

            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        D_CONSTEXPR bool
        operator==(
            const iterator& _other
        ) const
        {
            // both exhausted iff (in second and at end of second)
            const bool a_done = m_in_second && (m_it2 == m_end2);
            const bool b_done = _other.m_in_second
                                && (_other.m_it2 == _other.m_end2);

            if (a_done && b_done)
            {
                return true;
            }

            if (a_done || b_done)
            {
                return false;
            }

            return ( (m_in_second == _other.m_in_second) &&
                     ( m_in_second
                       ? (m_it2 == _other.m_it2)
                       : (m_it1 == _other.m_it1) ) );
        }

        D_CONSTEXPR
        bool operator!=(
            const iterator& _other
        ) const
        {
            return !(*this == _other);
        }

    private:
        inner_iterator_1 m_it1;
        inner_iterator_1 m_end1;
        inner_iterator_2 m_it2;
        inner_iterator_2 m_end2;
        bool             m_in_second;
    };

    using const_iterator = iterator;
    using value_type     = typename iterator::value_type;
    using reference      = value_type;

    template<typename _V1Fwd,
             typename _V2Fwd>
    D_CONSTEXPR
    concat_view(
        _V1Fwd&& _v1,
        _V2Fwd&& _v2
    )
        : m_v1(std::forward<_V1Fwd>(_v1)),
          m_v2(std::forward<_V2Fwd>(_v2))
    {}

    D_NODISCARD     iterator begin() const
    {
        return iterator(
            m_v1.begin(), m_v1.end(),
            m_v2.begin(), m_v2.end(),
            false);
    }

    D_NODISCARD     iterator end() const
    {
        return iterator(
            m_v1.end(), m_v1.end(),
            m_v2.end(), m_v2.end(),
            true);
    }

private:
    _V1 m_v1;
    _V2 m_v2;
};

// reverse_view
//   class: lazy view that yields the elements of an inner view in
// reverse order. Requires bidirectional inner iterators (this is a
// compile-time requirement enforced by the use of operator-- on
// the inner iterator).
template<typename _View>
class reverse_view : public view_base<reverse_view<_View>>
{
public:
    using inner_iterator = typename _View::const_iterator;

    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = typename std::iterator_traits<
            inner_iterator>::value_type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = typename std::iterator_traits<
            inner_iterator>::pointer;
        using reference         = typename std::iterator_traits<
            inner_iterator>::reference;

        D_CONSTEXPR
        iterator(
            inner_iterator _iterator,
            inner_iterator _begin,
            bool           _is_end
        )
            : m_it(_iterator),
             m_begin(_begin),
             m_is_end(_is_end)
        {}

        D_NODISCARD         reference operator*() const
        {
            inner_iterator prev = m_it;
            --prev;

            return *prev;
        }

        iterator& operator++()
        {
            --m_it;

            if (m_it == m_begin)
            {
                m_is_end = true;
            }

            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        D_CONSTEXPR
        bool operator==(
            const iterator& _other
        ) const
        {
            return (m_is_end == _other.m_is_end);
        }

        D_CONSTEXPR
        bool operator!=(
            const iterator& _other
        ) const
        {
            return !(*this == _other);
        }

    private:
        inner_iterator m_it;
        inner_iterator m_begin;
        bool           m_is_end;
    };

    using const_iterator = iterator;
    using value_type     = typename iterator::value_type;
    using reference      = typename iterator::reference;

    template<typename _VFwd>
    explicit D_CONSTEXPR
    reverse_view(
        _VFwd&& _view
    )
        : m_view(std::forward<_VFwd>(_view))
    {}

    D_NODISCARD iterator 
    begin() const
    {
        return iterator(m_view.end(), m_view.begin(),
                        m_view.begin() == m_view.end());
    }

    D_NODISCARD iterator 
    end() const
    {
        return iterator(m_view.begin(), m_view.begin(), true);
    }

private:
    _View m_view;
};


// chunk_view
//   class: lazy view that yields successive groups of N elements
// as std::vector<T>. The final group may have fewer than N
// elements if the inner view's length is not a multiple of N.
template<typename _View>
class chunk_view : public view_base<chunk_view<_View>>
{
public:
    using inner_iterator = typename _View::const_iterator;
    using inner_value    = typename std::iterator_traits<
        inner_iterator>::value_type;

    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = std::vector<inner_value>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = void;
        using reference         = value_type;

        iterator(
            inner_iterator _iterator,
            inner_iterator _end,
            std::size_t    _chunk_size
        )
            : m_it(_iterator)
            , m_end(_end)
            , m_chunk_size(_chunk_size < 1 ? 1 : _chunk_size)
            , m_current()
        {
            load();
        }

        D_NODISCARD         const value_type& operator*() const
        {
            return m_current;
        }

        iterator& operator++()
        {
            load();

            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        D_CONSTEXPR
        bool operator==(
            const iterator& _other
        ) const
        {
            return ( m_current.empty() && _other.m_current.empty() &&
                     (m_it == _other.m_it) );
        }

        D_CONSTEXPR bool 
        operator!=(
            const iterator& _other
        ) const
        {
            return !(*this == _other);
        }

    private:
        // load
        //   pulls up to m_chunk_size elements from the underlying
        // iterator into m_current. If no elements are available,
        // m_current is left empty, which signals end-of-stream.
        void load()
        {
            m_current.clear();

            for (std::size_t i = 0;
                 (i < m_chunk_size) && (m_it != m_end);
                 ++i)
            {
                m_current.push_back(*m_it);
                ++m_it;
            }

            return;
        }

        inner_iterator m_it;
        inner_iterator m_end;
        std::size_t    m_chunk_size;
        value_type     m_current;
    };

    using const_iterator = iterator;
    using value_type     = typename iterator::value_type;
    using reference      = value_type;

    template<typename _VFwd>
    D_CONSTEXPR
    chunk_view(
        _VFwd&&     _view,
        std::size_t _chunk_size
    )
        : m_view(std::forward<_VFwd>(_view))
        , m_chunk_size(_chunk_size)
    {}

    D_NODISCARD     iterator begin() const
    {
        return iterator(m_view.begin(), m_view.end(), m_chunk_size);
    }

    D_NODISCARD     iterator end() const
    {
        return iterator(m_view.end(), m_view.end(), m_chunk_size);
    }

private:
    _View       m_view;
    std::size_t m_chunk_size;
};


// stride_view
//   class: lazy view that yields every N-th element of an inner
// view, starting with the first. N == 1 yields every element;
// N == 0 is treated as N == 1 to avoid divide-by-zero issues.
template<typename _View>
class stride_view : public view_base<stride_view<_View>>
{
public:
    class iterator
    {
    public:
        using inner_iterator    = typename _View::const_iterator;
        using iterator_category = std::input_iterator_tag;
        using value_type        = typename std::iterator_traits<
            inner_iterator>::value_type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = typename std::iterator_traits<
            inner_iterator>::pointer;
        using reference         = typename std::iterator_traits<
            inner_iterator>::reference;

        D_CONSTEXPR
        iterator(
            inner_iterator _iterator,
            inner_iterator _end,
            std::size_t    _stride
        )
            : m_it(_iterator)
            , m_end(_end)
            , m_stride(_stride < 1 ? 1 : _stride)
        {}

        D_NODISCARD         reference operator*() const
        {
            return *m_it;
        }

        iterator& operator++()
        {
            for (std::size_t i = 0;
                 (i < m_stride) && (m_it != m_end);
                 ++i)
            {
                ++m_it;
            }

            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp(*this);
            ++(*this);

            return tmp;
        }

        D_CONSTEXPR
        bool operator==(
            const iterator& _other
        ) const
        {
            return (m_it == _other.m_it);
        }

        D_CONSTEXPR
        bool operator!=(
            const iterator& _other
        ) const
        {
            return (m_it != _other.m_it);
        }

    private:
        inner_iterator m_it;
        inner_iterator m_end;
        std::size_t    m_stride;
    };

    using const_iterator = iterator;
    using value_type     = typename iterator::value_type;
    using reference      = typename iterator::reference;

    template<typename _VFwd>
    D_CONSTEXPR
    stride_view(
        _VFwd&&     _view,
        std::size_t _stride
    )
        : m_view(std::forward<_VFwd>(_view))
        , m_stride(_stride)
    {}

    D_NODISCARD     iterator begin() const
    {
        return iterator(m_view.begin(), m_view.end(), m_stride);
    }

    D_NODISCARD     iterator end() const
    {
        return iterator(m_view.end(), m_view.end(), m_stride);
    }

private:
    _View       m_view;
    std::size_t m_stride;
};


///////////////////////////////////////////////////////////////////////////////
///             V.    ADAPTER FACTORIES                                     ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // Adapter factories return lightweight "adapter" objects that hold
    // the parameters of a future view but not the input view itself.
    // When piped with a view via operator|, the adapter constructs
    // the appropriate concrete view from the LHS view plus its stored
    // arguments. This is the standard ranges-style pattern.

    // transform_adapter
    //   helper: stores a transform function for later application.
    template<typename _Function>
    class transform_adapter
    {
    public:
        template<typename _FFwd>
        D_CONSTEXPR
        explicit transform_adapter(
            _FFwd&& _function
        )
            : m_function(std::forward<_FFwd>(_function))
        {}

        // apply
        //   constructs a transform_view over the supplied LHS view.
        template<typename _View>
        D_CONSTEXPR
        transform_view<typename std::decay<_View>::type, _Function>
        apply(
            _View&& _view
        ) const
        {
            return transform_view<
                typename std::decay<_View>::type, _Function>(
                    std::forward<_View>(_view), m_function);
        }

    private:
        _Function m_function;
    };

    // filter_adapter
    //   helper: stores a predicate for later application.
    template<typename _Predicate>
    class filter_adapter
    {
    public:
        template<typename _PFwd>
        D_CONSTEXPR
        explicit filter_adapter(
            _PFwd&& _predicate
        )
            : m_predicate(std::forward<_PFwd>(_predicate))
        {}

        template<typename _View>
        D_CONSTEXPR
        filter_view<typename std::decay<_View>::type, _Predicate>
        apply(
            _View&& _view
        ) const
        {
            return filter_view<
                typename std::decay<_View>::type, _Predicate>(
                    std::forward<_View>(_view), m_predicate);
        }

    private:
        _Predicate m_predicate;
    };

    // take_adapter
    template<typename = void>
    class take_adapter_t
    {
    public:
        explicit D_CONSTEXPR
        take_adapter_t(
            std::size_t _n
        )
            : m_n(_n)
        {}

        template<typename _View>
        D_CONSTEXPR
        take_view<typename std::decay<_View>::type>
        apply(
            _View&& _view
        ) const
        {
            return take_view<typename std::decay<_View>::type>(
                std::forward<_View>(_view), m_n);
        }

    private:
        std::size_t m_n;
    };
    using take_adapter = take_adapter_t<>;

    // drop_adapter
    template<typename = void>
    class drop_adapter_t
    {
    public:
        explicit D_CONSTEXPR
        drop_adapter_t(
            std::size_t _n
        )
            : m_n(_n)
        {}

        template<typename _View>
        D_CONSTEXPR
        drop_view<typename std::decay<_View>::type>
        apply(
            _View&& _view
        ) const
        {
            return drop_view<typename std::decay<_View>::type>(
                std::forward<_View>(_view), m_n);
        }

    private:
        std::size_t m_n;
    };
    using drop_adapter = drop_adapter_t<>;

    // take_while_adapter
    template<typename _Predicate>
    class take_while_adapter
    {
    public:
        template<typename _PFwd>
        D_CONSTEXPR explicit take_while_adapter(
            _PFwd&& _predicate
        )
            : m_predicate(std::forward<_PFwd>(_predicate))
        {}

        template<typename _View>
        D_CONSTEXPR take_while_view<typename std::decay<_View>::type, _Predicate>
        apply(
            _View&& _view
        ) const
        {
            return take_while_view<
                typename std::decay<_View>::type, _Predicate>(
                    std::forward<_View>(_view), m_predicate);
        }

    private:
        _Predicate m_predicate;
    };

    // drop_while_adapter
    template<typename _Predicate>
    class drop_while_adapter
    {
    public:
        template<typename _PFwd>
        D_CONSTEXPR explicit drop_while_adapter(
            _PFwd&& _predicate
        )
            : m_predicate(std::forward<_PFwd>(_predicate))
        {}

        template<typename _View>
        D_CONSTEXPR drop_while_view<typename std::decay<_View>::type, _Predicate>
        apply(
            _View&& _view
        ) const
        {
            return drop_while_view<
                typename std::decay<_View>::type, _Predicate>(
                    std::forward<_View>(_view), m_predicate);
        }

    private:
        _Predicate m_predicate;
    };

    // enumerate_adapter
    struct enumerate_adapter
    {
        template<typename _View>
        D_CONSTEXPR
        enumerate_view<typename std::decay<_View>::type>
        apply(
            _View&& _view
        ) const
        {
            return enumerate_view<typename std::decay<_View>::type>(
                std::forward<_View>(_view));
        }
    };

    // zip_adapter
    template<typename _Other>
    class zip_adapter
    {
    public:
        template<typename _OFwd>
        D_CONSTEXPR explicit zip_adapter(
            _OFwd&& _other
        )
            : m_other(std::forward<_OFwd>(_other))
        {}

        template<typename _View>
        D_CONSTEXPR zip_view<typename std::decay<_View>::type, _Other>
        apply(
            _View&& _view
        ) const
        {
            return zip_view<
                typename std::decay<_View>::type, _Other>(
                    std::forward<_View>(_view), m_other);
        }

    private:
        _Other m_other;
    };

    // concat_adapter
    template<typename _Other>
    class concat_adapter
    {
    public:
        template<typename _OFwd>
        D_CONSTEXPR
        explicit concat_adapter(
            _OFwd&& _other
        )
            : m_other(std::forward<_OFwd>(_other))
        {}

        template<typename _View>
        D_CONSTEXPR
        concat_view<typename std::decay<_View>::type, _Other>
        apply(
            _View&& _view
        ) const
        {
            return concat_view<
                typename std::decay<_View>::type, _Other>(
                    std::forward<_View>(_view), m_other);
        }

    private:
        _Other m_other;
    };

    // reverse_adapter
    struct reverse_adapter
    {
        template<typename _View>
        D_CONSTEXPR
        reverse_view<typename std::decay<_View>::type>
        apply(
            _View&& _view
        ) const
        {
            return reverse_view<typename std::decay<_View>::type>(
                std::forward<_View>(_view));
        }
    };

    // chunk_adapter
    template<typename = void>
    class chunk_adapter_t
    {
    public:
        explicit D_CONSTEXPR
        chunk_adapter_t(
            std::size_t _n
        )
            : m_n(_n)
        {}

        template<typename _View>
        D_CONSTEXPR
        chunk_view<typename std::decay<_View>::type>
        apply(
            _View&& _view
        ) const
        {
            return chunk_view<typename std::decay<_View>::type>(
                std::forward<_View>(_view), m_n);
        }

    private:
        std::size_t m_n;
    };
    using chunk_adapter = chunk_adapter_t<>;

    // stride_adapter
    template<typename = void>
    class stride_adapter_t
    {
    public:
        explicit D_CONSTEXPR
        stride_adapter_t(
            std::size_t _n
        )
            : m_n(_n)
        {}

        template<typename _View>
        D_CONSTEXPR
        stride_view<typename std::decay<_View>::type>
        apply(
            _View&& _view
        ) const
        {
            return stride_view<typename std::decay<_View>::type>(
                std::forward<_View>(_view), m_n);
        }

    private:
        std::size_t m_n;
    };
    using stride_adapter = stride_adapter_t<>;


    // is_adapter_helper
    //   helper: SFINAE detection for "has apply method that accepts
    // a view". Used to constrain operator| to adapter RHS.
    template<typename _Type>
    struct is_adapter_helper
    {
    private:
        template<typename _U>
        static auto test(int) -> decltype(
            std::declval<const _U&>().apply(
                std::declval<single_view<int>>()),
            std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_adapter
//   trait: true if _Type has an apply(view) method whose result is
// itself a view. The apply-returns-a-view requirement is what
// distinguishes an adapter from a terminal (which also has apply()
// but returns a non-view); making them mutually exclusive is required
// so that `view | to_vector()` is not ambiguous between the adapter
// and terminal operator| overloads. (fixed 2026-05-30)
NS_INTERNAL
    template<typename _Type>
    struct apply_result_is_view_helper
    {
    private:
        template<typename _U>
        static auto test(int) -> typename std::enable_if<
            is_view<decltype(
                std::declval<const _U&>().apply(
                    std::declval<single_view<int>>()))>::value,
            std::true_type>::type;

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };
NS_END  // internal

template<typename _Type>
struct is_adapter
    : internal::apply_result_is_view_helper<
          typename std::decay<_Type>::type>::type
{};


// transform
//   function: adapter factory for transform_view. Returns an
// adapter that, when piped onto a view, yields a lazy view of
// _fn(x) for each x in the input.
template<typename _Function>
D_CONSTEXPR
internal::transform_adapter<typename std::decay<_Function>::type>
transform(
    _Function&& _function
)
{
    return internal::transform_adapter<
        typename std::decay<_Function>::type>(
            std::forward<_Function>(_function));
}


// filter
//   function: adapter factory for filter_view. The resulting
// view yields only inputs satisfying _predicate.
template<typename _Predicate>
D_CONSTEXPR
internal::filter_adapter<typename std::decay<_Predicate>::type>
filter(
    _Predicate&& _predicate
)
{
    return internal::filter_adapter<
        typename std::decay<_Predicate>::type>(
            std::forward<_Predicate>(_predicate));
}


// take
//   function: adapter factory for take_view. The resulting view
// yields at most _n elements from the input.
inline
internal::take_adapter
take(
    std::size_t _n
)
{
    return internal::take_adapter(_n);
}


// drop
//   function: adapter factory for drop_view. The resulting view
// skips the first _n elements of the input.
inline
internal::drop_adapter
drop(
    std::size_t _n
)
{
    return internal::drop_adapter(_n);
}


// take_while
//   function: adapter factory for take_while_view.
template<typename _Predicate>
D_CONSTEXPR
internal::take_while_adapter<typename std::decay<_Predicate>::type>
take_while(
    _Predicate&& _predicate
)
{
    return internal::take_while_adapter<
        typename std::decay<_Predicate>::type>(
            std::forward<_Predicate>(_predicate));
}


// drop_while
//   function: adapter factory for drop_while_view.
template<typename _Predicate>
D_CONSTEXPR
internal::drop_while_adapter<typename std::decay<_Predicate>::type>
drop_while(
    _Predicate&& _predicate
)
{
    return internal::drop_while_adapter<
        typename std::decay<_Predicate>::type>(
            std::forward<_Predicate>(_predicate));
}


// enumerate
//   function: adapter factory for enumerate_view. Pairs every
// element with its zero-based index.
inline
internal::enumerate_adapter
enumerate()
{
    return internal::enumerate_adapter{};
}


// zip
//   function: adapter factory for zip_view. Pairs elements of
// the LHS view with elements of _other in lockstep.
template<typename _Other>
D_CONSTEXPR
internal::zip_adapter<typename std::decay<_Other>::type>
zip(
    _Other&& _other
)
{
    return internal::zip_adapter<
        typename std::decay<_Other>::type>(
            std::forward<_Other>(_other));
}


// concat
//   function: adapter factory for concat_view. Yields all of
// the LHS view, then all of _other.
template<typename _Other>
D_CONSTEXPR
internal::concat_adapter<typename std::decay<_Other>::type>
concat(
    _Other&& _other
)
{
    return internal::concat_adapter<
        typename std::decay<_Other>::type>(
            std::forward<_Other>(_other));
}


// reverse
//   function: adapter factory for reverse_view. Requires the
// LHS view's iterator to be bidirectional.
inline
internal::reverse_adapter
reverse()
{
    return internal::reverse_adapter{};
}


// chunk
//   function: adapter factory for chunk_view. Groups elements
// into vectors of size _n; the final group may be shorter.
inline
internal::chunk_adapter
chunk(
    std::size_t _n
)
{
    return internal::chunk_adapter(_n);
}


// stride
//   function: adapter factory for stride_view. Yields every
// _n-th element, starting with the first.
inline
internal::stride_adapter
stride(
    std::size_t _n
)
{
    return internal::stride_adapter(_n);
}


// iota (bounded)
//   function: builds an iota_view over [_start, _end).
template<typename _Int>
D_CONSTEXPR
iota_view<_Int>
iota(
    _Int _start,
    _Int _end
)
{
    return iota_view<_Int>(_start, _end);
}


// iota (unbounded)
//   function: builds an infinite iota_view starting at _start.
// Pair with take or take_while to bound.
template<typename _Int>
D_CONSTEXPR
iota_view<_Int>
iota(
    _Int _start
)
{
    return iota_view<_Int>(_start);
}


// repeat (unbounded)
//   function: builds an infinite view yielding _value forever.
template<typename _Type>
D_CONSTEXPR
repeat_view<typename std::decay<_Type>::type>
repeat(
    _Type&& _value
)
{
    return repeat_view<typename std::decay<_Type>::type>(
        std::forward<_Type>(_value));
}


// repeat_n
//   function: builds a view that yields _value exactly _n times.
template<typename _Type>
D_CONSTEXPR
repeat_view<typename std::decay<_Type>::type>
repeat_n(
    _Type&&         _value,
    std::size_t  _n
)
{
    return repeat_view<typename std::decay<_Type>::type>(
        std::forward<_Type>(_value), _n);
}


// generate
//   function: builds an infinite view that, on each advance,
// invokes _fn to produce the next value. Useful for random,
// time-based, or external-state sources.
template<typename _F>
D_CONSTEXPR
generate_view<typename std::decay<_F>::type>
generate(
    _F&& _fn
)
{
    return generate_view<typename std::decay<_F>::type>(
        std::forward<_F>(_fn));
}


// empty
//   function: builds an empty_view of the given type.
template<typename _Type>
D_CONSTEXPR
empty_view<_Type>
empty()
{
    return empty_view<_Type>{};
}


// single
//   function: builds a single_view containing one element.
template<typename _Type>
D_CONSTEXPR
single_view<typename std::decay<_Type>::type>
single(
    _Type&& _value
)
{
    return single_view<typename std::decay<_Type>::type>(
        std::forward<_Type>(_value));
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   PIPELINE OPERATORS                                    ///
///////////////////////////////////////////////////////////////////////////////

// operator| (view | adapter)
//   forwards the view to the adapter's apply method, yielding a
// new view of the appropriate concrete type. SFINAE-constrained
// to view LHS and adapter RHS so that the operator does not
// accidentally fire on unrelated types.
template<typename _View,
         typename _Adapter,
         typename std::enable_if<
             is_view<_View>::value && is_adapter<_Adapter>::value,
             int>::type = 0>
D_CONSTEXPR
auto operator|(
    _View&&    _view,
    _Adapter&& _adapter
) -> decltype(_adapter.apply(std::forward<_View>(_view)))
{
    return _adapter.apply(std::forward<_View>(_view));
}


// operator| (container | adapter)
//   lifts a container to a ref_view, then forwards to the
// adapter. SFINAE-constrained so that the operator only fires
// when the LHS is container-like but NOT already a view.
template<typename _Container,
         typename _Adapter,
         typename std::enable_if<
             ( has_begin_end<_Container>::value &&
               !is_view<_Container>::value     &&
               is_adapter<_Adapter>::value ),
             int>::type = 0>
D_CONSTEXPR
auto operator|(
    const _Container& _container,
    _Adapter&&        _adapter
) -> decltype(_adapter.apply(
       ref_view<typename std::decay<_Container>::type>(_container)))
{
    return _adapter.apply(
        ref_view<typename std::decay<_Container>::type>(_container));
}


///////////////////////////////////////////////////////////////////////////////
///             VII.  TERMINAL OPERATORS                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // to_vector_terminal
    //   helper: drains a view by iteration into a std::vector.
    struct to_vector_terminal
    {
        template<typename _View>
        auto apply(
            const _View& _view
        ) const
        -> std::vector<typename _View::value_type>
        {
            std::vector<typename _View::value_type> result;

            for (auto it = _view.begin(); it != _view.end(); ++it)
            {
                result.push_back(*it);
            }

            return result;
        }
    };

    // to_container_terminal
    //   helper: drains a view into an explicitly-typed container.
    // The container must support push_back (or a free-standing
    // insert(end(), x) shim, not implemented here).
    template<typename _Container>
    struct to_container_terminal
    {
        template<typename _View>
        _Container
        apply(
            const _View& _view
        ) const
        {
            _Container result;

            for (auto it = _view.begin(); it != _view.end(); ++it)
            {
                result.push_back(*it);
            }

            return result;
        }
    };

    // count_terminal
    //   helper: drains a view, returning element count.
    struct count_terminal
    {
        template<typename _View>
        std::size_t
        apply(
            const _View& _view
        ) const
        {
            std::size_t n = 0;

            for (auto it = _view.begin(); it != _view.end(); ++it)
            {
                ++n;
            }

            return n;
        }
    };

    // fold_terminal
    //   helper: drains a view through a binary step function.
    template<typename _Init,
             typename _Step>
    class fold_terminal
    {
    public:
        template<typename _IFwd,
                 typename _SFwd>
        D_CONSTEXPR
        fold_terminal(
            _IFwd&& _init,
            _SFwd&& _step
        )
            : m_init(std::forward<_IFwd>(_init)),
              m_step(std::forward<_SFwd>(_step))
        {}

        template<typename _View>
        _Init
        apply(
            const _View& _view
        ) const
        {
            _Init acc = m_init;

            for (auto it = _view.begin(); it != _view.end(); ++it)
            {
                acc = m_step(acc, *it);
            }

            return acc;
        }

    private:
        _Init m_init;
        _Step m_step;
    };

    // for_each_terminal
    //   helper: applies a consumer to every element.
    template<typename _Consumer>
    class for_each_terminal
    {
    public:
        template<typename _CFwd>
        D_CONSTEXPR explicit for_each_terminal(
            _CFwd&& _consumer
        )
            : m_consumer(std::forward<_CFwd>(_consumer))
        {}

        template<typename _View>
        void
        apply(
            const _View& _view
        ) const
        {
            for (auto it = _view.begin(); it != _view.end(); ++it)
            {
                m_consumer(*it);
            }

            return;
        }

    private:
        _Consumer m_consumer;
    };

    // any_of_terminal / all_of_terminal / none_of_terminal
    //   helpers: short-circuit predicate drains.
    template<typename _Predicate>
    class any_of_terminal
    {
    public:
        template<typename _PFwd>
        D_CONSTEXPR
        explicit any_of_terminal(
            _PFwd&& _predicate
        )
            : m_predicate(std::forward<_PFwd>(_predicate))
        {}

        template<typename _View>
        bool
        apply(
            const _View& _view
        ) const
        {
            for (auto it = _view.begin(); it != _view.end(); ++it)
            {
                if (m_predicate(*it))
                {
                    return true;
                }
            }

            return false;
        }

    private:
        _Predicate m_predicate;
    };

    template<typename _Predicate>
    class all_of_terminal
    {
    public:
        template<typename _PFwd>
        D_CONSTEXPR
        explicit all_of_terminal(
            _PFwd&& _predicate
        )
            : m_predicate(std::forward<_PFwd>(_predicate))
        {}

        template<typename _View>
        bool
        apply(
            const _View& _view
        ) const
        {
            for (auto it = _view.begin(); it != _view.end(); ++it)
            {
                if (!m_predicate(*it))
                {
                    return false;
                }
            }

            return true;
        }

    private:
        _Predicate m_predicate;
    };

    template<typename _Predicate>
    class none_of_terminal
    {
    public:
        template<typename _PFwd>
        D_CONSTEXPR
        explicit none_of_terminal(
            _PFwd&& _predicate
        )
            : m_predicate(std::forward<_PFwd>(_predicate))
        {}

        template<typename _View>
        bool
        apply(
            const _View& _view
        ) const
        {
            for (auto it = _view.begin(); it != _view.end(); ++it)
            {
                if (m_predicate(*it))
                {
                    return false;
                }
            }

            return true;
        }

    private:
        _Predicate m_predicate;
    };

    // is_terminal_helper
    //   helper: SFINAE detection for terminal operator types. A
    // terminal has an apply(view) method returning a non-view value.
    template<typename _Type>
    struct is_terminal_helper
    {
    private:
        template<typename _U>
        static auto test(int) -> decltype(
            std::declval<const _U&>().apply(
                std::declval<single_view<int>>()),
            std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_terminal
//   trait: detects terminal operator types. A terminal has an
// apply(view) whose result is a non-view value (e.g. to_vector ->
// std::vector). Defined as the exact complement of is_adapter so the
// adapter and terminal operator| overloads never both match.
// (fixed 2026-05-30)
template<typename _Type>
struct is_terminal
    : std::integral_constant<bool,
          internal::is_terminal_helper<
              typename std::decay<_Type>::type>::type::value
          && !is_adapter<_Type>::value>
{};


// operator| (view | terminal)  and  (container | terminal)
//   drives a view (or a container, lifted to a ref_view) into a
// terminal operator such as to_vector()/to<C>()/count(). These were
// documented -- `(some_view) | to_vector()` -- but never implemented;
// only the adapter pipe overloads existed, so terminals could not be
// piped at all. Constrained on is_terminal, which is now the exact
// complement of is_adapter, so there is no overlap with the adapter
// overloads. (added 2026-05-30)
template<typename _View,
         typename _Terminal,
         typename std::enable_if<
             is_view<_View>::value && is_terminal<_Terminal>::value,
             int>::type = 0>
D_CONSTEXPR
auto operator|(
    _View&&     _view,
    _Terminal&& _terminal
) -> decltype(_terminal.apply(std::forward<_View>(_view)))
{
    return _terminal.apply(std::forward<_View>(_view));
}

template<typename _Container,
         typename _Terminal,
         typename std::enable_if<
             ( has_begin_end<_Container>::value &&
               !is_view<_Container>::value      &&
               is_terminal<_Terminal>::value ),
             int>::type = 0>
D_CONSTEXPR
auto operator|(
    const _Container& _container,
    _Terminal&&       _terminal
) -> decltype(_terminal.apply(
       ref_view<typename std::decay<_Container>::type>(_container)))
{
    return _terminal.apply(
        ref_view<typename std::decay<_Container>::type>(_container));
}


// to_vector
//   function: terminal operator that drains a view into a
// std::vector<value_type>. Usage:
//     auto v = (some_view) | to_vector();
inline
internal::to_vector_terminal
to_vector()
{
    return internal::to_vector_terminal{};
}


// to
//   function: terminal operator that drains a view into an
// explicitly-typed container _Container. The container must
// support push_back. Usage:
//     auto s = (some_view) | to<std::deque<int>>();
template<typename _Container>
D_CONSTEXPR
internal::to_container_terminal<_Container>
to()
{
    return internal::to_container_terminal<_Container>{};
}


// count
//   function: terminal operator that returns the number of
// elements in a view. Iterates through all elements; views over
// random-access containers do NOT short-circuit.
inline
internal::count_terminal
count()
{
    return internal::count_terminal{};
}


// fold
//   function: terminal operator that drains a view through a
// binary step function starting from _init. Equivalent to
// std::accumulate.
template<typename _Init,
         typename _Step>
D_CONSTEXPR
internal::fold_terminal<typename std::decay<_Init>::type,
                       typename std::decay<_Step>::type>
fold(
    _Init&& _init,
    _Step&& _step
)
{
    return internal::fold_terminal<
        typename std::decay<_Init>::type,
        typename std::decay<_Step>::type>(
            std::forward<_Init>(_init),
            std::forward<_Step>(_step));
}


// for_each
//   function: terminal operator that applies a consumer to each
// element of a view. Returns void.
template<typename _Consumer>
D_CONSTEXPR
internal::for_each_terminal<typename std::decay<_Consumer>::type>
for_each(
    _Consumer&& _consumer
)
{
    return internal::for_each_terminal<
        typename std::decay<_Consumer>::type>(
            std::forward<_Consumer>(_consumer));
}


// any_of (terminal)
//   function: terminal operator that returns true iff at least one
// element of the view satisfies _predicate. Short-circuits on
// first match.
template<typename _Predicate>
D_CONSTEXPR
internal::any_of_terminal<typename std::decay<_Predicate>::type>
any_of(
    _Predicate&& _predicate
)
{
    return internal::any_of_terminal<
        typename std::decay<_Predicate>::type>(
            std::forward<_Predicate>(_predicate));
}


// all_of (terminal)
//   function: terminal operator that returns true iff every
// element of the view satisfies _predicate. Vacuously true for
// empty views.
template<typename _Predicate>
D_CONSTEXPR internal::all_of_terminal<typename std::decay<_Predicate>::type>
all_of(
    _Predicate&& _predicate
)
{
    return internal::all_of_terminal<
        typename std::decay<_Predicate>::type>(
            std::forward<_Predicate>(_predicate));
}


// none_of (terminal)
//   function: terminal operator that returns true iff no element
// of the view satisfies _predicate.
template<typename _Predicate>
D_CONSTEXPR
internal::none_of_terminal<typename std::decay<_Predicate>::type>
none_of(
    _Predicate&& _predicate
)
{
    return internal::none_of_terminal<
        typename std::decay<_Predicate>::type>(
            std::forward<_Predicate>(_predicate));
}

///////////////////////////////////////////////////////////////////////////////
///             VIII. VIEW SFINAE STRUCTURAL TRAITS & CONCEPTS              ///
///////////////////////////////////////////////////////////////////////////////
//   Extensions to the detection vocabulary introduced piecewise above
// (is_view / has_begin_end in section I, is_adapter / is_terminal alongside
// the factories).  Gathered here, after every view, adapter, and terminal is
// defined, so the value-type extractor and the C++20 concepts can see the
// whole protocol.  Each predicate reduces to a `static constexpr bool value`;
// view_value_type yields a `::type`.  The concepts close the section.

NS_INTERNAL
    // view_value_type_helper
    //   helper: primary has no members (soft failure for non-views); the
    // void_t-guarded specialization exposes _View::value_type when present.
    template<typename _AlwaysVoid,
             typename _View>
    struct view_value_type_helper
    {};

    template<typename _View>
    struct view_value_type_helper<
        void_t<typename _View::value_type>,
        _View>
    {
        using type = typename _View::value_type;
    };

NS_END  // internal


// view_value_type
//   trait: the element type a view yields, i.e. _View::value_type.
// SFINAE-friendly: has a `::type` only when _View exposes value_type
// (every view in this header does; non-views resolve cleanly to no
// member rather than a hard error).
template<typename _View>
struct view_value_type
{
    using type = typename internal::view_value_type_helper<
        void, typename std::decay<_View>::type>::type;
};

// view_value_type_t
//   alias: shorthand for view_value_type<_View>::type.
template<typename _View>
using view_value_type_t = typename view_value_type<_View>::type;


// is_pipeable_to_view
//   trait: true if _Type may appear on the left of operator| as a
// pipeline source -- either it is already a view, or it is a
// container-like type (has begin/end) that the pipeline implicitly
// lifts to a ref_view. This is exactly the disjunction the operator|
// overloads accept on the left-hand side.
template<typename _Type>
struct is_pipeable_to_view
    : std::integral_constant<bool,
          is_view<_Type>::value || has_begin_end<_Type>::value>
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// variable-template shorthands. is_view_v is defined with is_view in
// section I; the remaining detectors gain their _v forms here.
template<typename _Type>
static constexpr bool has_begin_end_v = has_begin_end<_Type>::value;

template<typename _Type>
static constexpr bool is_adapter_v = is_adapter<_Type>::value;

template<typename _Type>
static constexpr bool is_terminal_v = is_terminal<_Type>::value;

template<typename _Type>
static constexpr bool is_pipeable_to_view_v =
    is_pipeable_to_view<_Type>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
// view_type
//   concept: satisfied by any view (a type deriving from view_base).
// The C++20 parallel of is_view. Named view_type to avoid colliding
// with the views factory namespace and to mirror maybe_type /
// result_type in the sibling modules.
template<typename _Type>
concept view_type = is_view<_Type>::value;

// view_adapter
//   concept: satisfied by a pipeline adapter -- a type whose apply(view)
// yields another view. The C++20 parallel of is_adapter.
template<typename _Type>
concept view_adapter = is_adapter<_Type>::value;

// view_terminal
//   concept: satisfied by a terminal operator -- a type whose
// apply(view) yields a non-view. The C++20 parallel of is_terminal.
template<typename _Type>
concept view_terminal = is_terminal<_Type>::value;

// pipeable_to_view
//   concept: satisfied by any valid pipeline source (a view or a
// container-like type). The C++20 parallel of is_pipeable_to_view.
template<typename _Type>
concept pipeable_to_view = is_pipeable_to_view<_Type>::value;
#endif




///////////////////////////////////////////////////////////////////////////////
///             VIII.  FUNCTOR INSTANCE                                     ///
///////////////////////////////////////////////////////////////////////////////
//   A view is a Functor: transform is its map. This teaches the generic
// functor_map (functor.hpp) to drive a view through the one canonical name, so
// the same call that maps a maybe / result / producer also maps a view. A
// view's mapped type is transform_view<V, F> -- it depends on the mapping
// function F, so there is no single F<U> to rebind; the result type is named
// directly. Keyed on is_view, mutually exclusive with the monad bridge in
// functor.hpp (a view is not a monad) and the producer instance.

template<typename _View>
struct functor_traits<
    _View,
    typename std::enable_if<is_view<_View>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename view_value_type<_View>::type;

    // map
    //   functorial map via transform_view (the engine behind
    // transform). Lazy: no element is evaluated until the resulting
    // view is iterated or forced by a terminal.
    template<typename _ViewArg,
             typename _Function>
    static
    D_CONSTEXPR
    transform_view<typename std::decay<_ViewArg>::type,
                   typename std::decay<_Function>::type>
    map(
        _ViewArg&&  _view,
        _Function&& _function
    )
    {
        return transform_view<
            typename std::decay<_ViewArg>::type,
            typename std::decay<_Function>::type>(
                std::forward<_ViewArg>(_view),
                std::forward<_Function>(_function));
    }
};


///////////////////////////////////////////////////////////////////////////////
///             IX.   FOLDABLE INSTANCE                                     ///
///////////////////////////////////////////////////////////////////////////////
//   A view is a Foldable: its elements are collapsed by iterating the lazy
// pipeline once and threading the reducer through them. This teaches the
// generic fold_left (foldable.hpp) -- and therefore fold_to_vector,
// fold_length, fold_any, fold_all, fold_right, ... -- to drive a view through
// the one canonical name. Keyed on is_view so the single instance covers every
// view type; mutually exclusive with the maybe / result / producer instances
// (a view is none of those). Forcing the fold is a runtime act, so an infinite
// source must be bounded (take / take_while) before folding.

template<typename _View>
struct foldable_traits<
    _View,
    typename std::enable_if<is_view<_View>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename view_value_type<_View>::type;

    // fold_left
    //   strict left fold by iterating the view; the accumulator is threaded
    // by move so collecting folds stay O(n). D_CONSTEXPR -- a view is not a
    // literal type before C++20.
    template<typename _Acc,
             typename _Function>
    static
    D_CONSTEXPR
    _Acc fold_left(
        const _View& _view,
        _Acc         _init,
        _Function    _function
    )
    {
        for (auto _it = _view.begin(); _it != _view.end(); ++_it)
        {
            _init = _function(std::move(_init), *_it);
        }

        return _init;
    }
};


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_VIEW_