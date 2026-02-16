/******************************************************************************
* djinterp [core]                                                   djinterp.h
*
*   
* 
* 
* path:      \inc\djinterp.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2023.11.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.   FUNDAMENTAL TYPES & PRE-DEFINED CONSTANTS
     -----------------------------------------
i.   ENVIRONMENT
     1. 
ii.  TYPES
     1.  Cross-platform type compatability
         a.  boolean type     (bool, true, false)
         b.  signed size type (ssize_t)
     2.  Negative indexing support
         a.  Negative index type (d_index)
         b.  Functions:
     3.  Additional types
         a.  64-bit signed size type (ssize64_t)
     4.  Function pointers
         a.  fn_callback
         b.  fn_comparator
         c.  fn_free
         d.  fn_print
         e.  fn_to_string
         f.  fn_write
iii. CONSTANTS
     1.  Defined constants
         a.  D_SUCCESS
         b.  D_FAILURE
         c.  D_IS_ENABLED
         d.  D_IS_DISABLED
         e.  D_INDENT
         f.  D_INLINE
     2.  Global Keywords 
         a. D_KEYWORD_FRAMEWORK_NAME
         b. D_KEYWORD_CONTAINER       
         c. D_KEYWORD_ERROR       
         d. D_KEYWORD_EXCEPTION
         e. D_KEYWORD_INTERNAL
         f. D_KEYWORD_MESSAGE
         g. D_KEYWORD_WARNING

II.  CONVENIENCE MACROS
     ------------------
ii.  CONSTANTS
     1.  Defined constants
         a.  D_SUCCESS
         b.  D_FAILURE
         c.  D_IS_ENABLED
         d.  D_IS_DISABLED
         e.  D_INDENT
         f.  D_INLINE

II.  PREPROCESSOR DEFINITIONS AND CONSTANTS
     --------------------------------------
i.   DEFINED CONSTANTS
     1.  Defined constants
         a.  D_SUCCESS
         b.  D_FAILURE
         c.  D_IS_ENABLED
         d.  D_IS_DISABLED
         e.  D_INDENT
         f.  D_INLINE
ii.  MACROS
     1.  
         a.  D_IS_ENABLED
         b.
         c. 

II.  CORE FRAMEWORK TYPES
     --------------------


 III. INDEXING MACROS & UTILITIES
      ---------------------------
      a. index validation macros
      b. negative index conversion macros
      c. vector access macros

 IV.  GENERAL PURPOSE MACROS
      ----------------------
      a. bit field generation
      b. string conversion utilities
      c. boolean and status constants

 V.   FUNCTION DECLARATIONS
      ---------------------
      a. index conversion and validation functions
*/

/*
Do something with this, either here or in env.h OR in dconfig.h

// suppress MSVC security warnings - this library provides its own safe wrappers
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
    #define _CRT_SECURE_NO_WARNINGS 1
#endif

*/

#ifndef DJINTERP_
#define	DJINTERP_ 1

#include <stdint.h>
#include ".\env.h"
#include ".\dmacro.h"


///////////////////////////////////////////////////////////////////////////////
///                         I.   FUNDAMENTAL TYPES                          ///
///////////////////////////////////////////////////////////////////////////////

/// i.   ENVIRONMENT 
//////////////////////////////////////////

/// ii.  TYPES 
//////////////////////////////////////////


/// I.i.1.   Cross-platform type compatability

// a.
// bool
//   type: The following defines the type `bool`, which has been defined as 
// standard since C99 but may not be recognized on older machines.
#if ( defined(__bool_true_false_are_defined) ||     \
      defined(bool)                          ||     \
      defined(true)                          ||     \
	  defined(false) )
    // `stdbool.h` has already been included, do nothing

// #elif defined(D_ENV_LANG_IS_C23_OR_HIGHER) && D_ENV_LANG_IS_C23_OR_HIGHER()
#elif D_ENV_LANG_IS_C23_OR_HIGHER
    // C23 or newer - bool is a built-in keyword
    // nothing to do, C23 has bool, true, false as keywords

#elif D_ENV_LANG_IS_C99_OR_HIGHER
    // C99 or newer - use the standard header
    #include <stdbool.h>
#elif defined(__cplusplus)
    // C++ has built-in bool
    // nothing to do, C++ already has bool, true, false
#elif ( defined(D_ENV_COMPILER_MSVC) &&  \
        (D_ENV_COMPILER_MSVC == 1) )
    // Microsoft compiler without C99 support
    #if !defined(__bool_true_false_are_defined)
        #define __bool_true_false_are_defined 1

        #if !defined(bool)
            typedef unsigned char bool;
        #endif

        #if !defined(true)
            #define true 1
        #endif

        #if !defined(false)
            #define false 0
        #endif
    #endif
#else
    // generic implementation for other compilers
    #if !defined(__bool_true_false_are_defined)
        #define __bool_true_false_are_defined 1

        #if !defined(bool)
            typedef unsigned char bool;
        #endif

        #if !defined(true)
            #define true 1
        #endif

        #if !defined(false)
            #define false 0
        #endif
    #endif
#endif

#if D_ENV_LANG_IS_C99_OR_HIGHER
    #define D_RESTRICT restrict
#elif ( defined(D_ENV_COMPILER_GCC) ||  \
        defined(D_ENV_COMPILER_CLANG) )
    #define D_RESTRICT __restrict__
#elif defined(D_ENV_COMPILER_MSVC)
    #define D_RESTRICT __restrict
#else
    #define D_RESTRICT
#endif  // D_ENV_LANG_IS_C99_OR_HIGHER

// b.
// ssize_t
//   type: signed-size_t; 
// b.
// ssize_t
//   type: signed-size_t; 
#ifndef _SSIZE_T_DEFINED
    #ifndef _SSIZE_T
        #ifndef __ssize_t_defined
            #ifndef _SSIZE_T_
                #ifndef ssize_t
                    #if D_ENV_OS_USING_WINDOWS64
                        typedef long long ssize_t;
                        #define SSIZE_MAX LLONG_MAX
                    #else
                        typedef long ssize_t;
                        #define SSIZE_MAX LONG_MAX
                    #endif	// defined(D_ENV_OS_USING_WINDOWS64)

                    #define _SSIZE_T_
                    #define _SSIZE_T_DEFINED
                    #define __ssize_t_defined
                    #define _SSIZE_T
                #endif	// ssize_t
            #endif	// _SSIZE_T_
        #endif	// __ssize_t_defined
    #endif	// _SSIZE_T
#endif	// _SSIZE_T_DEFINED

// Prefer __COUNTER__ to avoid collisions if multiple asserts expand on one line.
#if defined(__COUNTER__)
    #define D_INTERNAL_STATIC_ASSERT_UID __COUNTER__
#else
    #define D_INTERNAL_STATIC_ASSERT_UID __LINE__
#endif

#ifndef D_STATIC_ASSERT
    #if D_ENV_LANG_DETECTED_CPP
        // >=C++11 has the static_assert keyword.
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER()
            #define D_STATIC_ASSERT(cond, msg) static_assert((cond), msg)
        #else
            #define D_STATIC_ASSERT(cond, msg)                              \
                typedef char D_CONCAT(static_assertion_failed_,             \
                                      D_INTERNAL_STATIC_ASSERT_UID)[(cond)  \
                                      ? 1                                   \
                                      : -1]
        #endif  //  #if ( ( defined(D_ENV_LANG_IS_CPP11_OR_HIGHER) && ...
    #else
        // C11+ has `_Static_assert`.
        #if D_ENV_LANG_IS_C11_OR_HIGHER
            #define D_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
        #else
            #define D_STATIC_ASSERT(cond, msg)                              \
                typedef char D_CONCAT(static_assertion_failed_,             \
                                      D_INTERNAL_STATIC_ASSERT_UID)[(cond)  \
                                      ? 1                                   \
                                      : -1]
        #endif
    #endif
#endif

                        // D_INLINE
//   constant: used for inlining functions; should be turned off with 
// `/D D_TESTING=1` in compiler args.
#ifdef D_TESTING
    #define D_INLINE  
    #define D_STATIC
    #define D_STATIC_INLINE
    #define D_CONSTEXPR
    #define D_STATIC_CONSTEXPR
    #define D_CONSTEXPR_INLINE
    #define D_STATIC_CONSTEXPR_INLINE
#else
    #define D_STATIC        static 
    
    // per-compiler inline attributes for optimal performance
    #if defined(D_ENV_COMPILER_MSVC)
        #define D_INLINE        __forceinline
        #define D_STATIC_INLINE static __forceinline
    #elif ( defined(D_ENV_COMPILER_GCC) ||  \
            defined(D_ENV_COMPILER_CLANG) )
        #define D_INLINE        inline __attribute__((always_inline))
        #define D_STATIC_INLINE static inline __attribute__((always_inline))
    #else
        #define D_INLINE        inline
        #define D_STATIC_INLINE static inline
    #endif

    // D_CONSTEXPR, D_STATIC_CONSTEXPR, D_CONSTEXPR_INLINE, 
    // D_STATIC_CONSTEXPR_INLINE
    //   constants: constexpr qualifiers for C++ (C++11 and above); fall back
    // to const (or const + inline/static) in C or pre-C++11 environments.
    #if defined(D_ENV_LANG_DETECTED_CPP) &&                             \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP11)
        #define D_CONSTEXPR                 constexpr
        #define D_STATIC_CONSTEXPR          static constexpr
        #define D_CONSTEXPR_INLINE          constexpr D_INLINE
        #define D_STATIC_CONSTEXPR_INLINE   static constexpr D_INLINE
    #else
        #define D_CONSTEXPR                 const
        #define D_STATIC_CONSTEXPR          static const
        #define D_CONSTEXPR_INLINE          const D_INLINE
        #define D_STATIC_CONSTEXPR_INLINE   static const D_INLINE
    #endif
#endif

// D_NOINLINE
//   constant: used to explicitly prevent inlining for debugging/profiling.
#if defined(D_ENV_COMPILER_MSVC)
    #define D_NOINLINE     __declspec(noinline)

#elif ( defined(D_ENV_COMPILER_GCC) ||  \
        defined(D_ENV_COMPILER_CLANG) )
    #define D_NOINLINE     __attribute__((noinline))

#else
    #define D_NOINLINE
#endif

/// I.i.2.   Additional types


/// I.i.3.   Function pointers
// fn_apply
//   typedef: function pointer type for applying an operation to an element.
typedef void (*fn_apply)(void* _element);

// fn_apply_context
//   typedef: function pointer type for applying an operation to an element
// with additional context.
typedef void (*fn_apply_context)(void* _element, void* _context);

// fn_callback
//   function pointer: generic callback with optional context parameter.
// _context may be NULL.
typedef void (*fn_callback)(void* _context);

// fn_comparator
//   function pointer: compares two values of identical type, and returns a
// value that is:
//   LESS than 0:    if the first parameter is LESS than the second
//   EQUAL to 0:     if both of the arguments are EQUAL
//   GREATER than 0: if the first parameter is GREATER than the other
typedef int (*fn_comparator)(const void* _a, const void* _b);

// fn_free
//   function pointer: function to be used to frees the memory associated 
// with the composite data type provided.
typedef void (*fn_free)(void* _ptr);

// fn_print
//   function pointer: prints a string in to the desired output.
typedef void (*fn_print)(void* _type, ...);

// fn_string
//   function pointer: pointers to a 
typedef const char* (*fn_to_string)();

// fn_write
//   function pointer: writes to a buffer
typedef size_t (*fn_write)(char* const _buffer, size_t _size);

// D_KEYWORD_FRAMEWORK_NAME
//   constant: keyword corresponding to the name of this framework.
#define D_KEYWORD_FRAMEWORK_NAME    djinterp

// D_KEYWORD_CONTAINER
//   constant: keyword used to specify that the given module or namespace deals
// with data structures.
#define D_KEYWORD_CONTAINER         container

// D_KEYWORD_ERROR
//   constant: keyword for invalid state. 
// From Latin `errare` -- to wander.
#define D_KEYWORD_ERROR		        error

// D_KEYWORD_EXCEPTION
//   constant: keyword used to indicate a severe deviation from a valid state.
// Note the etymology -- from Latin `errare`: to wander.
#define D_KEYWORD_EXCEPTION	        exception

// D_KEYWORD_FUNCTIONAL
//   keyword: resolves to `functional`.
// Used to specify that a unit of code pertains to functional programming.
#define D_KEYWORD_FUNCTIONAL       functional

// D_KEYWORD_INTERNAL
//   keyword: resolves to `internal`.
// Used to specify that a unit of code is part of the `internal` namespace, 
// hiding the details of implementation from regular use.
#define D_KEYWORD_INTERNAL	        internal

// D_KEYWORD_MATHS
//   keyword: resolves to `maths`.
// Used for variables, macros, namespaces, etc. that pertain to the `maths`
// submodule.
#define D_KEYWORD_MATH	            math

// D_KEYWORD_MESSAGE
//   keyword: resolves to `message`.
// Used for variables, macros, namespaces, etc. that convey (usually string-
// based) human-readable information that is conveyed to the user, often 
// (but not limited to) debugging and error-handling.
#define D_KEYWORD_MESSAGE	        message

// D_KEYWORD_UI
//   keyword: resolves to `ui`.
// Used to specify that a unit of code is part of error handling, including 
// messages.
#define D_KEYWORD_USER_INTERFACE    ui

// D_KEYWORD_MESSAGE
//   keyword: resolves to `warning`.
// Used to specify that the program has an anomalous state that is not 
// necessarily the end of the world.
#define D_KEYWORD_WARNING	        warning


///////////////////////////////////////////////////////////////////////////////
///                         I.   FUNDAMENTAL TYPES                          ///                 
///////////////////////////////////////////////////////////////////////////////

/// I.ii.1.   Defined constants

// D_SUCCESS
//   alias: corresponds to a SUCCESSFUL operation; evaluates to D_TRUE.
#define D_SUCCESS true

// D_FAILURE
//   alias: corresponds to a FAILED operation; evaluates to D_FALSE.
#define D_FAILURE false

// D_IS_ENABLED
//   alias: corresponds to a SUCCESSFUL operation; evaluates to D_TRUE.
#define D_ENABLED  true

// D_IS_DISABLED
//   alias: corresponds to a FAILED operation; evaluates to D_FALSE.
#define D_DISABLED false

// D_IS_ENABLED
//   macro: returns a value equivalent to `true` if and only if parameter
// `token` is both defined AND equal to the value held by `D_IS_ENABLED`.
#define D_IS_ENABLED(token)                  \
    ( ( defined(token) ) &&                  \
      ( (token) == (D_ENABLED) ) ) ? true :  \
                                     false

// D_IS_DISABLED
//   macro: returns a value equivalent to `false` if and only if parameter
// `token` is both: 
// 1. defined, and
// 2. equal to the value held by `D_ENABLED`.
#define D_IS_DISABLED(token)                  \
    ( ( defined(token) ) &&                   \
      ( (token) == (D_DISABLED) ) ) ? true :  \
                                      false

// D_INDENT
//   constant: string corresponding to one (1) level of indentation. Defaults
// to two single spaces; 
#ifndef D_INDENT
#   define D_INDENT "  "
#endif  // D_INDENT

// d_index
//   type: a type corresponding to an vector index that may be negative (in
// addition to the traditional positive or zero vector indices).
// A negative 'd_index' goes from the last element to 0, rather than the 
// opposite way it is for positive indices.  E.g. an index of -1 would 
// return the last element, -n would be 0.
typedef ssize_t d_index;


size_t d_index_convert_fast(d_index _index,
                            size_t  _count);
bool   d_index_convert_safe(d_index _index, 
                            size_t  _count, 
                            size_t* _destination);
bool   d_index_is_valid(d_index _index, 
                        size_t  _count);


// D_ARRAY_TOTAL_SIZE
//   macro: shorthand for calculating the total memory occupied, in bytes, of 
// an vector of elements.
// Equal to the product of `ELEMENT_SIZE` and `ELEMENTS_COUNT`.
#define D_ARRAY_TOTAL_SIZE(ELEMENT_SIZE, ELEMENTS_COUNT)	\
	((size_t)( (ELEMENT_SIZE) * (ELEMENTS_COUNT) ))

// D_CLAMP_INDEX
//   macro: clamps an index to valid range for given array size
//   Returns 0 for negative indices, (SIZE-1) for too-large indices
#define D_CLAMP_INDEX(index, arr_size)          \
    ( (arr_size) == 0                           \
      ? 0                                       \
      : ( (index) < 0                           \
          ? 0                                   \
          : ( (index) >= (ssize_t)(arr_size) )  \
            ? ( (arr_size) - 1 )                \
            : (index) ) )

// D_INDEX_IN_BOUNDS  
//   macro: alias for D_IS_VALID_INDEX_N for compatibility
#define D_INDEX_IN_BOUNDS(index, arr_size) \
    D_IS_VALID_INDEX_N((index), (arr_size))

// D_SAFE_ARR_IDX
//   macro: safe array indexing that returns the element value, not a pointer
//   Note: only to be used on stack-allocated arrays whose size is known at compile time
#define D_SAFE_ARR_IDX(arr, arr_size)                                \
    ( D_IS_VALID_INDEX_N((arr_size), sizeof(arr)/sizeof((arr)[0]) )  \
        ? D_ARR_IDX((arr), (arr_size))                               \
        : (arr)[0] )

// D_IS_VALID_INDEX
//   macro: validates that an INDEX is within bounds for an array of given SIZE
#define D_IS_VALID_INDEX(INDEX, SIZE)                   \
    ( ((SIZE) > 0) &&                                   \
      ( ((INDEX) >= 0 && (INDEX) < (ssize_t)(SIZE)) ||  \
        ((INDEX) < 0 && (-(INDEX)) <= (ssize_t)(SIZE)) ) )

// D_IS_VALID_INDEX_N
//   macro: 
#define D_IS_VALID_INDEX_N(INDEX, SIZE)		\
    ((INDEX) >= -(ssize_t)(SIZE) && (INDEX) < (ssize_t)(SIZE))

// D_NEG_IDX
//   macro: given a negative index and the size of the vector (in number of
// elements), returns the non-negative valid index equivalent.
//   Note: this does not check if INDEX corresponds to a valid index within the
// span of the vector; that must be done by the caller to avoid an out-of-bounds
// index.
#define D_NEG_IDX(INDEX, SIZE)          \
	( (INDEX) < 0 ? (SIZE) + (INDEX) :  \
				    (INDEX) )

// D_ARR_IDX
//   macro: given a negative index and array, returns the array element at the
// equivalent positive index.
// Note: only to be used on stack-allocated arrays whose size is known at
// compile time.
#define D_ARR_IDX(ARR, INDEX)           \
     (ARR)[(INDEX) < 0 ? ( (sizeof(ARR)/sizeof((ARR)[0])) + (INDEX) ) :  \
                                                            (INDEX)]

#ifdef __cplusplus
    //#include <cstddef>
    //#include <memory>

/**********   IV.    namespaces   *******************/
    // D_KEYWORD_CPP
    //   keyword: resolves to `cpp`.
    // Used to specify that a unit of code pertains to the C++ standard.
    #define D_KEYWORD_CPP               cpp

    // D_KEYWORD_STL
    //   keyword: resolves to `stl`.
    // Used to specify that a unit of code pertains to the STL (Standard Template Library) part of the C++ 
    // standard.
    #define D_KEYWORD_STL               stl

    // D_KEYWORD_TRAITS
    //   keyword: resolves to `traits`.
    // Used to specify that a unit of code uses template metaprogramming and 
    // SFINAE for compile-time logic.
    #define D_KEYWORD_TRAITS            traits

    // D_NAMESPACE
    //   macro: wraps a block of code in a namespace with the name specified by parameter `NAME`.
    #define D_NAMESPACE(NAME)	namespace NAME {

    // NS_END
    //   Namespace idiom; used to close any namespace.
    #define NS_END				};

    // NS_CONTAINER
    //   namespace: used to indicate the `djinterp` top-level namespace.
    // Should be used as the top-level namesapce of any and all modules within the djinterp tool-chain.
    #define NS_CONTAINER		D_NAMESPACE(D_KEYWORD_CONTAINER)

    // NS_DJINTERP
    //   namespace: used to indicate the `djinterp` top-level namespace.
    // Should be used as the top-level namesapce of any and all modules within the djinterp tool-chain.
    #define NS_DJINTERP			D_NAMESPACE(D_KEYWORD_FRAMEWORK_NAME)

    // NS_ERROR
    //   
    #define NS_ERROR			D_NAMESPACE(D_KEYWORD_ERROR)

    // NS_EXCEPTION
    //   
    #define NS_EXCEPTION		D_NAMESPACE(D_KEYWORD_EXCEPTION)

    // NS_EXCEPTION
    //   
    #define NS_FUNCTIONAL		D_NAMESPACE(D_KEYWORD_FUNCTIONAL)

    // NS_INTERNAL
    //   namespace: declares an `internal` namespace.
    // Used to hide any messy implementation details from regular use, such as "helper" types, structs and functions.
    // Should be closed with `NS_END`.
    #define NS_INTERNAL			D_NAMESPACE(D_KEYWORD_INTERNAL)

    // NS_MATHS
    //   namespace: used for the `maths` submodule namespace for C++.
    #define NS_MATH 			D_NAMESPACE(D_KEYWORD_MATH)

    // NS_MESSAGE
    //   namespace: used for variables, macros, namespaces, etc. that convey (usually string-based) human-readable information that is conveyed to the user.
    // These messages are often (but not limited to) debugging and error-handling.
    #define NS_MESSAGE			D_NAMESPACE(D_KEYWORD_MESSAGE)

    // NS_STL
    //   
    #define NS_STL			    D_NAMESPACE(D_KEYWORD_STL)

    // NS_TRAITS
    //   
    #define NS_TRAITS			D_NAMESPACE(D_KEYWORD_TRAITS)

    // Macro version if inline functions aren't supported
    #ifndef D_NO_INLINE
        #ifdef _MSC_VER
            #define D_MEMCPY_S(dest, dest_size, src, count)       \
                ((memcpy_s((dest), (dest_size), (src), (count)) == 0) ? D_SUCCESS : D_FAILURE)
        #else
            #define D_MEMCPY_S(dest, dest_size, src, count)       \
                (((dest) && (src) && ((count) <= (dest_size))) ?  \
                    (memcpy((dest), (src), (count)), D_SUCCESS) : D_FAILURE)
        #endif  // _MSC_VER
    #endif  // D_NO_INLINE

NS_DJINTERP

    // abs_value
    //   type trait: computes the absolute value of a compile-time integral
    // constant.
    template<typename _Type,
              _Type   _N>
    struct abs_value
    {
        static_assert(std::is_integral<_Type>::value,
                      "Type parameter `_Type` must be an integral type.");

        static constexpr _Type value = (_N < 0) 
                                       ? -_N
                                       : _N;
    };

    //
    #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        // abs_value_v
        //   
        template<typename _Type,
                  _Type    _N>
        constexpr _Type abs_value_v = abs_value<_Type, _N>::value;
    #endif

    // 
    #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        // abs_value_to_size_t
        //   
        template<typename _Type,
                  _Type   _N>
        constexpr std::size_t abs_value_to_size_t =
            std::integral_constant<std::size_t, abs_value<_Type, _N>::value>::value;
    #endif

    // clean
    //   type modifier: strips cv-qualifiers and references
    template<typename _Type>
    struct clean
    {
        using type = std::remove_cv_t<std::remove_reference_t<_Type>>;
    };

    template<typename _Type>
    using clean_t = clean<_Type>::type;

    // repeat_type_helper (needed for repeat)
    NS_INTERNAL
        template<typename    _Type,
                 std::size_t _N,
                 typename... _Types>
        struct repeat_type_helper 
        {
            using type = typename repeat_type_helper<_Type, _N - 1, _Type, _Types...>::type;
        };

        template<typename    _Type,
                 typename... _Types>
        struct repeat_type_helper<_Type, 0, _Types...> 
        {
            using type = std::tuple<_Types...>;
        };

    NS_END  // internal

    // repeat
    //   
    template<typename    _Type,
             std::size_t _NumTimes>
    struct repeat 
    {
        using type = std::conditional_t<(_NumTimes > 0), 
                                         typename internal::repeat_type_helper<_Type, _NumTimes>::type,
                                         std::tuple<>>;
    };

    template<typename    _Type,
             std::size_t _NumTimes>
    using repeat_t = typename repeat<_Type, _NumTimes>::type;

    // self-reference marker
    struct self
    {};

    // detection trait
    template<typename _Type>
    struct is_self : std::false_type
    {};

    template<>
    struct is_self<self> : std::true_type
    {};

    template<typename _Type>
    inline constexpr bool is_self_v = is_self<_Type>::value;

    template<typename _Type, 
             typename _ResolveTo>
    struct resolve_self 
    {
        using type = _Type;
    };

    template<typename _Type,
             typename _ResolveTo>
    struct resolve_self 
    {
        using type = _Type;
    };

    // basic self resolution
    template<typename _ResolveTo>
    struct resolve_self<self, _ResolveTo> 
    {
        using type = _ResolveTo;
    };

    // smart pointer specializations
    template<typename _ResolveTo>
    struct resolve_self<std::unique_ptr<self>, _ResolveTo> 
    {
        using type = std::unique_ptr<_ResolveTo>;
    };

    template<typename _ResolveTo>
    struct resolve_self<std::shared_ptr<self>, _ResolveTo> 
    {
        using type = std::shared_ptr<_ResolveTo>;
    };

    template<typename _ResolveTo>
    struct resolve_self<std::weak_ptr<self>, _ResolveTo> 
    {
        using type = std::weak_ptr<_ResolveTo>;
    };

    // raw pointer specialization
    template<typename _ResolveTo>
    struct resolve_self<self*, _ResolveTo> 
    {
        using type = _ResolveTo*;
    };

    // repeat resolution - recursively resolve the repeated type
    template<typename    _Type, 
             std::size_t _NumTimes, 
             typename    _ResolveTo>
    struct resolve_self<repeat<_Type, _NumTimes>, _ResolveTo> 
    {
        using resolved_inner = resolve_self_t<_Type, _ResolveTo>;
        using type = repeat_t<resolved_inner, _NumTimes>;
    };

    // convenience alias
    template<typename _Type,
             typename _ResolveTo>
    using resolve_self_t = typename resolve_self<_Type, _ResolveTo>::type;

    template<typename...>
    using void_t = void;

NS_END  // djinterp

#endif	// __cplusplus


#endif	// DJINTERP_