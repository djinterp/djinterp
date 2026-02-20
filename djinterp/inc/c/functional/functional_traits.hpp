/******************************************************************************
* djinterp [core]                                          functional_traits.hpp
*
* SFINAE type traits for functional programming types.
*   This header provides compile-time detection of callable signatures and
* functional programming patterns. Uses detection idiom (void_t) for maximum
* compatibility without requiring tagging.
*
* NAMING CONVENTIONS:
*   is_<pattern>             - primary trait (inherits from bool_constant)
*   is_<pattern>_v           - variable template helper
*   <pattern>_result_t       - result type extractor
*
* DETECTED PATTERNS:
*   - Callable detection (invocable, function-like)
*   - Predicate detection (returns bool)
*   - Consumer detection (returns void)
*   - Producer/Supplier detection (no parameters)
*   - Transformer detection (unary with different result)
*   - Comparator detection (binary returning int or bool)
*   - Functor detection (has operator())
*
* path:      \inc\functional_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.15
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_TRAITS_
#define DJINTERP_FUNCTIONAL_TRAITS_ 1

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include ".\env.h"
#include ".\cpp_features.h"
#include ".\djinterp.h"
#include ".\functional.hpp"


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///             I.    DETECTION IDIOM UTILITIES                             ///
///////////////////////////////////////////////////////////////////////////////

// These utilities provide a portable implementation of the detection idiom
// for use across different C++ standard versions.

NS_INTERNAL

    // void_t_impl
    //   helper: maps any types to void for SFINAE contexts.
    template<typename...>
    struct void_t_impl
    {
        using type = void;
    };

NS_END  // internal

// void_t
//   type alias: maps any type sequence to void.
// Backport of std::void_t for pre-C++17 compilers.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    template<typename... _Types>
    using void_t = std::void_t<_Types...>;
#else
    template<typename... _Types>
    using void_t = typename internal::void_t_impl<_Types...>::type;
#endif

// nonesuch
//   type: represents a type that cannot be instantiated.
// Used as the default for detection idiom failures.
struct nonesuch
{
    nonesuch()                         = delete;
    ~nonesuch()                        = delete;
    nonesuch(const nonesuch&)          = delete;
    nonesuch& operator=(const nonesuch&) = delete;
};

NS_INTERNAL

    // detector
    //   helper: primary template for detection idiom (failure case).
    template<typename _Default,
             typename _AlwaysVoid,
             template<typename...> class _Op,
             typename... _Args>
    struct detector
    {
        using value_t = std::false_type;
        using type    = _Default;
    };

    // detector
    //   helper: specialization for detection idiom (success case).
    template<typename _Default,
             template<typename...> class _Op,
             typename... _Args>
    struct detector<_Default, void_t<_Op<_Args...>>, _Op, _Args...>
    {
        using value_t = std::true_type;
        using type    = _Op<_Args...>;
    };

NS_END  // internal

// is_detected
//   type trait: checks if _Op<_Args...> is well-formed.
template<template<typename...> class _Op,
         typename... _Args>
using is_detected = typename internal::detector<nonesuch, void, _Op, _Args...>::value_t;

// detected_t
//   type alias: yields _Op<_Args...> if well-formed, nonesuch otherwise.
template<template<typename...> class _Op,
         typename... _Args>
using detected_t = typename internal::detector<nonesuch, void, _Op, _Args...>::type;

// detected_or
//   type alias: yields _Op<_Args...> if well-formed, _Default otherwise.
template<typename _Default,
         template<typename...> class _Op,
         typename... _Args>
using detected_or = internal::detector<_Default, void, _Op, _Args...>;

// detected_or_t
//   type alias: shorthand for detected_or::type.
template<typename _Default,
         template<typename...> class _Op,
         typename... _Args>
using detected_or_t = typename detected_or<_Default, _Op, _Args...>::type;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
// is_detected_v
//   variable template: value helper for is_detected.
template<template<typename...> class _Op,
         typename... _Args>
inline constexpr bool is_detected_v = is_detected<_Op, _Args...>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             II.   CALLABLE DETECTION                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // call_result_helper
    //   helper: extracts result type of calling _Fn with _Args.
    template<typename _Fn,
             typename... _Args>
    using call_result_helper = decltype(std::declval<_Fn>()(
        std::declval<_Args>()...));

    // invoke_result_helper
    //   helper: extracts result type using invoke semantics.
    template<typename _Fn,
             typename... _Args>
    using invoke_result_helper = decltype(stl::invoke(std::declval<_Fn>(),
                                                      std::declval<_Args>()...));

NS_END  // internal

// is_callable
//   type trait: checks if _Fn can be called with _Args.
// Uses direct call syntax (not invoke semantics).
template<typename _Fn,
         typename... _Args>
struct is_callable : is_detected<internal::call_result_helper, _Fn, _Args...>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename... _Args>
inline constexpr bool is_callable_v = is_callable<_Fn, _Args...>::value;
#endif

// is_callable_r
//   type trait: checks if _Fn can be called and result converts to _Ret.
template<typename _Ret,
         typename _Fn,
         typename... _Args>
struct is_callable_r
{
private:
    template<typename _F,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _F>
    struct check<_F,
        typename std::enable_if<
            is_callable<_F, _Args...>::value &&
            (std::is_void<_Ret>::value ||
             std::is_convertible<
                 detected_t<internal::call_result_helper, _F, _Args...>,
                 _Ret>::value)
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Ret,
         typename _Fn,
         typename... _Args>
inline constexpr bool is_callable_r_v = is_callable_r<_Ret, _Fn, _Args...>::value;
#endif

// callable_result
//   type trait: extracts the result type of calling _Fn with _Args.
template<typename _Fn,
         typename... _Args>
struct callable_result
{
    using type = detected_or_t<void, internal::call_result_helper, _Fn, _Args...>;
};

template<typename _Fn,
         typename... _Args>
using callable_result_t = typename callable_result<_Fn, _Args...>::type;


///////////////////////////////////////////////////////////////////////////////
///             III.  FUNCTOR/FUNCTION OBJECT DETECTION                     ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // has_call_operator_helper
    //   helper: checks for existence of operator().
    template<typename _Type>
    using has_call_operator_helper = decltype(&_Type::operator());

NS_END  // internal

// is_functor
//   type trait: checks if _Type has an operator() member.
// Detects function objects, lambdas, and callable classes.
template<typename _Type>
struct is_functor : is_detected<internal::has_call_operator_helper, _Type>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Type>
inline constexpr bool is_functor_v = is_functor<_Type>::value;
#endif

// is_function_like
//   type trait: checks if _Type is callable (function, functor, or pointer).
template<typename _Type>
struct is_function_like
    : std::integral_constant<bool,
          std::is_function<typename std::remove_pointer<
              typename std::decay<_Type>::type>::type>::value ||
          is_functor<typename std::decay<_Type>::type>::value ||
          std::is_member_function_pointer<
              typename std::decay<_Type>::type>::value>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Type>
inline constexpr bool is_function_like_v = is_function_like<_Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             IV.   PREDICATE DETECTION                                   ///
///////////////////////////////////////////////////////////////////////////////

// is_predicate
//   type trait: checks if _Fn is a predicate (returns bool-convertible).
// A predicate is a callable that returns a value convertible to bool.
template<typename _Fn,
         typename... _Args>
struct is_predicate
{
private:
    template<typename _F,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _F>
    struct check<_F,
        typename std::enable_if<
            is_callable<_F, _Args...>::value &&
            std::is_convertible<
                callable_result_t<_F, _Args...>,
                bool>::value
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename... _Args>
inline constexpr bool is_predicate_v = is_predicate<_Fn, _Args...>::value;
#endif

// is_unary_predicate
//   type trait: checks if _Fn is a unary predicate for type _Arg.
template<typename _Fn,
         typename _Arg>
using is_unary_predicate = is_predicate<_Fn, _Arg>;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Arg>
inline constexpr bool is_unary_predicate_v = is_unary_predicate<_Fn, _Arg>::value;
#endif

// is_binary_predicate
//   type trait: checks if _Fn is a binary predicate for types _Arg1, _Arg2.
template<typename _Fn,
         typename _Arg1,
         typename _Arg2 = _Arg1>
using is_binary_predicate = is_predicate<_Fn, _Arg1, _Arg2>;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Arg1,
         typename _Arg2 = _Arg1>
inline constexpr bool is_binary_predicate_v = 
    is_binary_predicate<_Fn, _Arg1, _Arg2>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             V.    CONSUMER DETECTION                                    ///
///////////////////////////////////////////////////////////////////////////////

// is_consumer
//   type trait: checks if _Fn is a consumer (returns void).
// A consumer is a callable that returns void.
template<typename _Fn,
         typename... _Args>
struct is_consumer
{
private:
    template<typename _F,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _F>
    struct check<_F,
        typename std::enable_if<
            is_callable<_F, _Args...>::value &&
            std::is_void<callable_result_t<_F, _Args...>>::value
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename... _Args>
inline constexpr bool is_consumer_v = is_consumer<_Fn, _Args...>::value;
#endif

// is_unary_consumer
//   type trait: checks if _Fn is a unary consumer for type _Arg.
template<typename _Fn,
         typename _Arg>
using is_unary_consumer = is_consumer<_Fn, _Arg>;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Arg>
inline constexpr bool is_unary_consumer_v = is_unary_consumer<_Fn, _Arg>::value;
#endif

// is_binary_consumer
//   type trait: checks if _Fn is a binary consumer for types _Arg1, _Arg2.
template<typename _Fn,
         typename _Arg1,
         typename _Arg2 = _Arg1>
using is_binary_consumer = is_consumer<_Fn, _Arg1, _Arg2>;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Arg1,
         typename _Arg2 = _Arg1>
inline constexpr bool is_binary_consumer_v = 
    is_binary_consumer<_Fn, _Arg1, _Arg2>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             VI.   PRODUCER/SUPPLIER DETECTION                           ///
///////////////////////////////////////////////////////////////////////////////

// is_producer
//   type trait: checks if _Fn is a producer (callable with no args).
// A producer is a callable that takes no arguments and returns a value.
template<typename _Fn,
         typename _Result = void>
struct is_producer
{
private:
    template<typename _F,
             typename _R,
             typename = void>
    struct check : std::false_type
    {};

    // any result type
    template<typename _F>
    struct check<_F, void,
        typename std::enable_if<
            is_callable<_F>::value &&
            !std::is_void<callable_result_t<_F>>::value
        >::type>
        : std::true_type
    {};

    // specific result type
    template<typename _F,
             typename _R>
    struct check<_F, _R,
        typename std::enable_if<
            !std::is_void<_R>::value &&
            is_callable<_F>::value &&
            std::is_convertible<callable_result_t<_F>, _R>::value
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn, _Result>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Result = void>
inline constexpr bool is_producer_v = is_producer<_Fn, _Result>::value;
#endif

// is_supplier
//   type trait: alias for is_producer (common Java naming).
template<typename _Fn,
         typename _Result = void>
using is_supplier = is_producer<_Fn, _Result>;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Result = void>
inline constexpr bool is_supplier_v = is_supplier<_Fn, _Result>::value;
#endif

// is_generator
//   type trait: alias for is_producer.
template<typename _Fn,
         typename _Result = void>
using is_generator = is_producer<_Fn, _Result>;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Result = void>
inline constexpr bool is_generator_v = is_generator<_Fn, _Result>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             VII.  TRANSFORMER/MAPPER DETECTION                          ///
///////////////////////////////////////////////////////////////////////////////

// is_transformer
//   type trait: checks if _Fn transforms _Input to _Output.
// A transformer is a unary function that converts one type to another.
template<typename _Fn,
         typename _Input,
         typename _Output = void>
struct is_transformer
{
private:
    template<typename _F,
             typename _I,
             typename _O,
             typename = void>
    struct check : std::false_type
    {};

    // any output type
    template<typename _F,
             typename _I>
    struct check<_F, _I, void,
        typename std::enable_if<
            is_callable<_F, _I>::value &&
            !std::is_void<callable_result_t<_F, _I>>::value
        >::type>
        : std::true_type
    {};

    // specific output type
    template<typename _F,
             typename _I,
             typename _O>
    struct check<_F, _I, _O,
        typename std::enable_if<
            !std::is_void<_O>::value &&
            is_callable<_F, _I>::value &&
            std::is_convertible<callable_result_t<_F, _I>, _O>::value
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn, _Input, _Output>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Input,
         typename _Output = void>
inline constexpr bool is_transformer_v = is_transformer<_Fn, _Input, _Output>::value;
#endif

// is_mapper
//   type trait: alias for is_transformer.
template<typename _Fn,
         typename _Input,
         typename _Output = void>
using is_mapper = is_transformer<_Fn, _Input, _Output>;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Input,
         typename _Output = void>
inline constexpr bool is_mapper_v = is_mapper<_Fn, _Input, _Output>::value;
#endif

// transformer_result
//   type trait: extracts the output type of a transformer.
template<typename _Fn,
         typename _Input>
struct transformer_result
{
    using type = callable_result_t<_Fn, _Input>;
};

template<typename _Fn,
         typename _Input>
using transformer_result_t = typename transformer_result<_Fn, _Input>::type;


///////////////////////////////////////////////////////////////////////////////
///             VIII. COMPARATOR DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

// is_comparator
//   type trait: checks if _Fn is a comparator (binary, returns int-like).
// A comparator returns negative, zero, or positive for ordering.
template<typename _Fn,
         typename _Type>
struct is_comparator
{
private:
    template<typename _F,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _F>
    struct check<_F,
        typename std::enable_if<
            is_callable<_F, const _Type&, const _Type&>::value &&
            (std::is_integral<callable_result_t<_F, const _Type&, const _Type&>>::value ||
             std::is_same<callable_result_t<_F, const _Type&, const _Type&>, bool>::value)
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Type>
inline constexpr bool is_comparator_v = is_comparator<_Fn, _Type>::value;
#endif

// is_strict_weak_ordering
//   type trait: checks if _Fn provides strict weak ordering.
// Requires: irreflexive, asymmetric, transitive comparator.
template<typename _Fn,
         typename _Type>
using is_strict_weak_ordering = is_binary_predicate<_Fn, _Type, _Type>;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Type>
inline constexpr bool is_strict_weak_ordering_v = 
    is_strict_weak_ordering<_Fn, _Type>::value;
#endif

// is_equality_comparer
//   type trait: checks if _Fn compares for equality.
template<typename _Fn,
         typename _Type>
using is_equality_comparer = is_binary_predicate<_Fn, _Type, _Type>;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Type>
inline constexpr bool is_equality_comparer_v = 
    is_equality_comparer<_Fn, _Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             IX.   BINARY OPERATION DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////

// is_binary_op
//   type trait: checks if _Fn is a binary operation on _Type.
// A binary operation takes two values of _Type and returns _Result.
template<typename _Fn,
         typename _Type,
         typename _Result = _Type>
struct is_binary_op
{
private:
    template<typename _F,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _F>
    struct check<_F,
        typename std::enable_if<
            is_callable<_F, const _Type&, const _Type&>::value &&
            std::is_convertible<
                callable_result_t<_F, const _Type&, const _Type&>,
                _Result>::value
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Type,
         typename _Result = _Type>
inline constexpr bool is_binary_op_v = is_binary_op<_Fn, _Type, _Result>::value;
#endif

// is_unary_op
//   type trait: checks if _Fn is a unary operation on _Type.
template<typename _Fn,
         typename _Type,
         typename _Result = _Type>
struct is_unary_op
{
private:
    template<typename _F,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _F>
    struct check<_F,
        typename std::enable_if<
            is_callable<_F, const _Type&>::value &&
            std::is_convertible<
                callable_result_t<_F, const _Type&>,
                _Result>::value
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Type,
         typename _Result = _Type>
inline constexpr bool is_unary_op_v = is_unary_op<_Fn, _Type, _Result>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             X.    ACCUMULATOR/REDUCER DETECTION                         ///
///////////////////////////////////////////////////////////////////////////////

// is_accumulator
//   type trait: checks if _Fn is an accumulator function.
// An accumulator combines an accumulated value with a new element.
template<typename _Fn,
         typename _Accumulated,
         typename _Element,
         typename _Result = _Accumulated>
struct is_accumulator
{
private:
    template<typename _F,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _F>
    struct check<_F,
        typename std::enable_if<
            is_callable<_F, const _Accumulated&, const _Element&>::value &&
            std::is_convertible<
                callable_result_t<_F, const _Accumulated&, const _Element&>,
                _Result>::value
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Accumulated,
         typename _Element,
         typename _Result = _Accumulated>
inline constexpr bool is_accumulator_v = 
    is_accumulator<_Fn, _Accumulated, _Element, _Result>::value;
#endif

// is_reducer
//   type trait: checks if _Fn is a reducer (accumulator with same type).
template<typename _Fn,
         typename _Type>
using is_reducer = is_accumulator<_Fn, _Type, _Type, _Type>;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Type>
inline constexpr bool is_reducer_v = is_reducer<_Fn, _Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             XI.   HASHER DETECTION                                      ///
///////////////////////////////////////////////////////////////////////////////

// is_hasher
//   type trait: checks if _Fn is a hash function for _Type.
// A hasher takes a value and returns size_t.
template<typename _Fn,
         typename _Type>
struct is_hasher
{
private:
    template<typename _F,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _F>
    struct check<_F,
        typename std::enable_if<
            is_callable<_F, const _Type&>::value &&
            std::is_convertible<
                callable_result_t<_F, const _Type&>,
                std::size_t>::value
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Type>
inline constexpr bool is_hasher_v = is_hasher<_Fn, _Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             XII.  FACTORY DETECTION                                     ///
///////////////////////////////////////////////////////////////////////////////

// is_factory
//   type trait: checks if _Fn is a factory for _Type.
// A factory creates instances of _Type from given arguments.
template<typename _Fn,
         typename _Type,
         typename... _Args>
struct is_factory
{
private:
    template<typename _F,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _F>
    struct check<_F,
        typename std::enable_if<
            is_callable<_F, _Args...>::value &&
            std::is_convertible<callable_result_t<_F, _Args...>, _Type>::value
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename _Type,
         typename... _Args>
inline constexpr bool is_factory_v = is_factory<_Fn, _Type, _Args...>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             XIII. ACTION/RUNNABLE DETECTION                             ///
///////////////////////////////////////////////////////////////////////////////

// is_action
//   type trait: checks if _Fn is an action (no args, returns void).
template<typename _Fn>
struct is_action
{
private:
    template<typename _F,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _F>
    struct check<_F,
        typename std::enable_if<
            is_callable<_F>::value &&
            std::is_void<callable_result_t<_F>>::value
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn>
inline constexpr bool is_action_v = is_action<_Fn>::value;
#endif

// is_runnable
//   type trait: alias for is_action.
template<typename _Fn>
using is_runnable = is_action<_Fn>;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn>
inline constexpr bool is_runnable_v = is_runnable<_Fn>::value;
#endif

// is_thunk
//   type trait: alias for is_action.
template<typename _Fn>
using is_thunk = is_action<_Fn>;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn>
inline constexpr bool is_thunk_v = is_thunk<_Fn>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             XIV.  TRANSPARENT FUNCTOR DETECTION                         ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // has_is_transparent_helper
    //   helper: detects is_transparent member type.
    template<typename _Type>
    using has_is_transparent_helper = typename _Type::is_transparent;

NS_END  // internal

// is_transparent_functor
//   type trait: checks if _Type has is_transparent member type.
// Transparent functors enable heterogeneous lookup in containers.
template<typename _Type>
struct is_transparent_functor 
    : is_detected<internal::has_is_transparent_helper, _Type>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Type>
inline constexpr bool is_transparent_functor_v = 
    is_transparent_functor<_Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             XV.   FUNCTION SIGNATURE EXTRACTION                         ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // function_traits_helper
    //   helper: primary template (undefined for non-function types).
    template<typename _Fn>
    struct function_traits_helper;

    // function_traits_helper
    //   helper: specialization for function types.
    template<typename _Ret,
             typename... _Args>
    struct function_traits_helper<_Ret(_Args...)>
    {
        using result_type      = _Ret;
        using argument_types   = std::tuple<_Args...>;
        static constexpr std::size_t arity = sizeof...(_Args);

        template<std::size_t _N>
        using argument = typename std::tuple_element<_N, argument_types>::type;
    };

    // function_traits_helper
    //   helper: specialization for function pointers.
    template<typename _Ret,
             typename... _Args>
    struct function_traits_helper<_Ret(*)(_Args...)>
        : function_traits_helper<_Ret(_Args...)>
    {};

    // function_traits_helper
    //   helper: specialization for member function pointers.
    template<typename _Class,
             typename _Ret,
             typename... _Args>
    struct function_traits_helper<_Ret(_Class::*)(_Args...)>
        : function_traits_helper<_Ret(_Args...)>
    {
        using class_type = _Class;
    };

    // const member function
    template<typename _Class,
             typename _Ret,
             typename... _Args>
    struct function_traits_helper<_Ret(_Class::*)(_Args...) const>
        : function_traits_helper<_Ret(_Args...)>
    {
        using class_type = _Class;
    };

    // volatile member function
    template<typename _Class,
             typename _Ret,
             typename... _Args>
    struct function_traits_helper<_Ret(_Class::*)(_Args...) volatile>
        : function_traits_helper<_Ret(_Args...)>
    {
        using class_type = _Class;
    };

    // const volatile member function
    template<typename _Class,
             typename _Ret,
             typename... _Args>
    struct function_traits_helper<_Ret(_Class::*)(_Args...) const volatile>
        : function_traits_helper<_Ret(_Args...)>
    {
        using class_type = _Class;
    };

    // functor/lambda detection helper
    template<typename _Fn,
             typename = void>
    struct function_traits_impl;

    // function types and pointers
    template<typename _Fn>
    struct function_traits_impl<_Fn,
        typename std::enable_if<
            std::is_function<typename std::remove_pointer<_Fn>::type>::value ||
            std::is_member_function_pointer<_Fn>::value
        >::type>
        : function_traits_helper<_Fn>
    {};

    // functors and lambdas (has operator())
    template<typename _Fn>
    struct function_traits_impl<_Fn,
        typename std::enable_if<
            !std::is_function<typename std::remove_pointer<_Fn>::type>::value &&
            !std::is_member_function_pointer<_Fn>::value &&
            is_functor<_Fn>::value
        >::type>
        : function_traits_helper<decltype(&_Fn::operator())>
    {};

NS_END  // internal

// function_traits
//   type trait: extracts signature information from callable types.
// Works with functions, function pointers, member functions, and functors.
template<typename _Fn>
struct function_traits 
    : internal::function_traits_impl<typename std::decay<_Fn>::type>
{};

// function_result_t
//   type alias: extracts return type from callable.
template<typename _Fn>
using function_result_t = typename function_traits<_Fn>::result_type;

// function_arity_v
//   constant: number of parameters the callable takes.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn>
inline constexpr std::size_t function_arity_v = function_traits<_Fn>::arity;
#endif

// function_argument_t
//   type alias: extracts Nth argument type from callable.
template<std::size_t _N,
         typename    _Fn>
using function_argument_t = typename function_traits<_Fn>::template argument<_N>;


///////////////////////////////////////////////////////////////////////////////
///             XVI.  STD::FUNCTION DETECTION                               ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // is_std_function_helper
    //   helper: primary template (not std::function).
    template<typename _Type>
    struct is_std_function_helper : std::false_type
    {};

    // is_std_function_helper
    //   helper: specialization for std::function.
    template<typename _Sig>
    struct is_std_function_helper<std::function<_Sig>> : std::true_type
    {};

NS_END  // internal

// is_std_function
//   type trait: checks if _Type is a std::function instantiation.
template<typename _Type>
struct is_std_function 
    : internal::is_std_function_helper<typename std::decay<_Type>::type>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Type>
inline constexpr bool is_std_function_v = is_std_function<_Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             XVII. NOTHROW CALLABLE DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////

// is_nothrow_callable
//   type trait: checks if _Fn can be called without throwing.
template<typename _Fn,
         typename... _Args>
struct is_nothrow_callable
{
private:
    template<typename _F,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _F>
    struct check<_F,
        typename std::enable_if<
            is_callable<_F, _Args...>::value &&
            noexcept(std::declval<_F>()(std::declval<_Args>()...))
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<_Fn>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename... _Args>
inline constexpr bool is_nothrow_callable_v = 
    is_nothrow_callable<_Fn, _Args...>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_TRAITS_
