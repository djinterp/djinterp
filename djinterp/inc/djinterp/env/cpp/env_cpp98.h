/*******************************************************************************
* djinterp [env]                                                     env_cpp98.h
*
* djinterp C++98 standard-library header detection.
*   Detects which C++98 standard-library headers are available. They predate
* the feature-test macros that env_cpp_features.h reads, so each flag assumes
* its header exists in any C++ build, except where a platform or a compiler
* switch is known to remove it: <locale> on AVR and Android, <typeinfo>
* without RTTI, and <exception> and <stdexcept> without exception support. In
* C, every flag is 0. A header a freestanding implementation need not
* provide reads 0 unless __STDC_HOSTED__ says hosted; <new>, <typeinfo>,
* <exception> and <limits>, which every C++98 implementation has, do not
* depend on it.
*   D_ENV_CPP98_HAS_<HEADER> is 1 if <header> is available and 0 otherwise;
* D_ENV_CPP98_HAS_ALL_* and D_ENV_CPP98_HAS_FULL_STL combine them.
*   Every D_ENV_CPP98_HAS_<HEADER> flag is pre-definable: #define it before
* including this header to override the detected value, for instance to test a
* partial library, to simulate another platform, or to switch exceptions off.
*   It reads only the compiler's own predefined macros, __STDC_HOSTED__
* among them, and includes nothing.
*
*
* path:      /inc/djinterp/env/cpp/env_cpp98.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.08
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  C++98 STANDARD LIBRARY HEADERS
    ------------------------------
    1.  Containers
         1.  D_ENV_CPP98_HAS_VECTOR
         2.  D_ENV_CPP98_HAS_LIST
         3.  D_ENV_CPP98_HAS_DEQUE
         4.  D_ENV_CPP98_HAS_QUEUE
         5.  D_ENV_CPP98_HAS_STACK
         6.  D_ENV_CPP98_HAS_MAP
         7.  D_ENV_CPP98_HAS_SET
         8.  D_ENV_CPP98_HAS_BITSET
    2.  Algorithms and iterators
         1.  D_ENV_CPP98_HAS_ALGORITHM
         2.  D_ENV_CPP98_HAS_ITERATOR
         3.  D_ENV_CPP98_HAS_FUNCTIONAL
         4.  D_ENV_CPP98_HAS_NUMERIC
    3.  Strings and localization
         1.  D_ENV_CPP98_HAS_STRING
         2.  D_ENV_CPP98_HAS_LOCALE
    4.  I/O streams
         1.  D_ENV_CPP98_HAS_IOSTREAM
         2.  D_ENV_CPP98_HAS_ISTREAM
         3.  D_ENV_CPP98_HAS_OSTREAM
         4.  D_ENV_CPP98_HAS_FSTREAM
         5.  D_ENV_CPP98_HAS_SSTREAM
         6.  D_ENV_CPP98_HAS_IOMANIP
         7.  D_ENV_CPP98_HAS_IOS
         8.  D_ENV_CPP98_HAS_IOSFWD
         9.  D_ENV_CPP98_HAS_STREAMBUF
    5.  Utilities
         1.  D_ENV_CPP98_HAS_UTILITY
         2.  D_ENV_CPP98_HAS_MEMORY
         3.  D_ENV_CPP98_HAS_NEW
         4.  D_ENV_CPP98_HAS_TYPEINFO
         5.  D_ENV_CPP98_HAS_EXCEPTION
         6.  D_ENV_CPP98_HAS_STDEXCEPT
         7.  D_ENV_CPP98_HAS_LIMITS
    6.  Numerics
         1.  D_ENV_CPP98_HAS_COMPLEX
         2.  D_ENV_CPP98_HAS_VALARRAY
2.  AGGREGATE CHECKS
    ----------------
    1.  Header groups
         1.  D_ENV_CPP98_HAS_ALL_CONTAINERS
         2.  D_ENV_CPP98_HAS_ALL_ALGORITHMS
         3.  D_ENV_CPP98_HAS_ALL_IOSTREAMS
         4.  D_ENV_CPP98_HAS_ALL_UTILITIES
         5.  D_ENV_CPP98_HAS_ALL_NUMERICS
    2.  Whole library
         1.  D_ENV_CPP98_HAS_FULL_STL
*/

#ifndef DJINTERP_ENV_CPP_ENV_CPP98_H
#define DJINTERP_ENV_CPP_ENV_CPP98_H 1


// D_INTERNAL_ENV_CPP98_HOSTED
//   macro: 1 where the implementation says it is hosted (__STDC_HOSTED__ 1),
// and 0 where it says it is freestanding -- and where it says nothing: this
// header assumes no library it has no word for (decision 91 of the register).
// Every header a freestanding implementation need not provide reads 0 where
// this is 0; <new>, <typeinfo>, <exception> and <limits>, which C++98
// requires even of freestanding implementations, do not depend on it.
#if ( (defined(__STDC_HOSTED__)) &&                                            \
      (__STDC_HOSTED__) )
    #define D_INTERNAL_ENV_CPP98_HOSTED 1
#else
    #define D_INTERNAL_ENV_CPP98_HOSTED 0
#endif


//==============================================================================
// 1.  C++98 STANDARD LIBRARY HEADERS
//==============================================================================
// Any conforming C++98 or later implementation provides these headers, so the
// flags mostly document a dependency, and give non-conforming or embedded
// implementations a hook. Embedded libraries (Arduino, AVR-GCC) often ship a
// subset, commonly without <locale>, exceptions, RTTI, or, on bare metal, I/O
// streams.


// 1.1    Containers
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_CPP98_HAS_VECTOR
//   feature: 1 if <vector> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_VECTOR
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_VECTOR 1
    #else
        #define D_ENV_CPP98_HAS_VECTOR 0
    #endif
#endif  // D_ENV_CPP98_HAS_VECTOR

// 1.1.2
// D_ENV_CPP98_HAS_LIST
//   feature: 1 if <list> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_LIST
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_LIST 1
    #else
        #define D_ENV_CPP98_HAS_LIST 0
    #endif
#endif  // D_ENV_CPP98_HAS_LIST

// 1.1.3
// D_ENV_CPP98_HAS_DEQUE
//   feature: 1 if <deque> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_DEQUE
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_DEQUE 1
    #else
        #define D_ENV_CPP98_HAS_DEQUE 0
    #endif
#endif  // D_ENV_CPP98_HAS_DEQUE

// 1.1.4
// D_ENV_CPP98_HAS_QUEUE
//   feature: 1 if <queue> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_QUEUE
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_QUEUE 1
    #else
        #define D_ENV_CPP98_HAS_QUEUE 0
    #endif
#endif  // D_ENV_CPP98_HAS_QUEUE

// 1.1.5
// D_ENV_CPP98_HAS_STACK
//   feature: 1 if <stack> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_STACK
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_STACK 1
    #else
        #define D_ENV_CPP98_HAS_STACK 0
    #endif
#endif  // D_ENV_CPP98_HAS_STACK

// 1.1.6
// D_ENV_CPP98_HAS_MAP
//   feature: 1 if <map> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_MAP
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_MAP 1
    #else
        #define D_ENV_CPP98_HAS_MAP 0
    #endif
#endif  // D_ENV_CPP98_HAS_MAP

// 1.1.7
// D_ENV_CPP98_HAS_SET
//   feature: 1 if <set> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_SET
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_SET 1
    #else
        #define D_ENV_CPP98_HAS_SET 0
    #endif
#endif  // D_ENV_CPP98_HAS_SET

// 1.1.8
// D_ENV_CPP98_HAS_BITSET
//   feature: 1 if <bitset> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_BITSET
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_BITSET 1
    #else
        #define D_ENV_CPP98_HAS_BITSET 0
    #endif
#endif  // D_ENV_CPP98_HAS_BITSET

// 1.2    Algorithms and iterators
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_CPP98_HAS_ALGORITHM
//   feature: 1 if <algorithm> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_ALGORITHM
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_ALGORITHM 1
    #else
        #define D_ENV_CPP98_HAS_ALGORITHM 0
    #endif
#endif  // D_ENV_CPP98_HAS_ALGORITHM

// 1.2.2
// D_ENV_CPP98_HAS_ITERATOR
//   feature: 1 if <iterator> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_ITERATOR
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_ITERATOR 1
    #else
        #define D_ENV_CPP98_HAS_ITERATOR 0
    #endif
#endif  // D_ENV_CPP98_HAS_ITERATOR

// 1.2.3
// D_ENV_CPP98_HAS_FUNCTIONAL
//   feature: 1 if <functional> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_FUNCTIONAL
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_FUNCTIONAL 1
    #else
        #define D_ENV_CPP98_HAS_FUNCTIONAL 0
    #endif
#endif  // D_ENV_CPP98_HAS_FUNCTIONAL

// 1.2.4
// D_ENV_CPP98_HAS_NUMERIC
//   feature: 1 if <numeric> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_NUMERIC
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_NUMERIC 1
    #else
        #define D_ENV_CPP98_HAS_NUMERIC 0
    #endif
#endif  // D_ENV_CPP98_HAS_NUMERIC

// 1.3    Strings and localization
//------------------------------------------------------------------------------
// 1.3.1
// D_ENV_CPP98_HAS_STRING
//   feature: 1 if <string> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_STRING
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_STRING 1
    #else
        #define D_ENV_CPP98_HAS_STRING 0
    #endif
#endif  // D_ENV_CPP98_HAS_STRING

// 1.3.2
// D_ENV_CPP98_HAS_LOCALE
//   feature: 1 if <locale> is available (C++98), 0 otherwise. Assumed
// absent on AVR and Android, where locale support may be missing.
#ifndef D_ENV_CPP98_HAS_LOCALE
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #if ( (!defined(__AVR__)) &&                                           \
              (!defined(__ANDROID__)) )
            #define D_ENV_CPP98_HAS_LOCALE 1
        #else
            #define D_ENV_CPP98_HAS_LOCALE 0
        #endif
    #else
        #define D_ENV_CPP98_HAS_LOCALE 0
    #endif
#endif  // D_ENV_CPP98_HAS_LOCALE

// 1.4    I/O streams
//------------------------------------------------------------------------------
// 1.4.1
// D_ENV_CPP98_HAS_IOSTREAM
//   feature: 1 if <iostream> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_IOSTREAM
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_IOSTREAM 1
    #else
        #define D_ENV_CPP98_HAS_IOSTREAM 0
    #endif
#endif  // D_ENV_CPP98_HAS_IOSTREAM

// 1.4.2
// D_ENV_CPP98_HAS_ISTREAM
//   feature: 1 if <istream> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_ISTREAM
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_ISTREAM 1
    #else
        #define D_ENV_CPP98_HAS_ISTREAM 0
    #endif
#endif  // D_ENV_CPP98_HAS_ISTREAM

// 1.4.3
// D_ENV_CPP98_HAS_OSTREAM
//   feature: 1 if <ostream> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_OSTREAM
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_OSTREAM 1
    #else
        #define D_ENV_CPP98_HAS_OSTREAM 0
    #endif
#endif  // D_ENV_CPP98_HAS_OSTREAM

// 1.4.4
// D_ENV_CPP98_HAS_FSTREAM
//   feature: 1 if <fstream> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_FSTREAM
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_FSTREAM 1
    #else
        #define D_ENV_CPP98_HAS_FSTREAM 0
    #endif
#endif  // D_ENV_CPP98_HAS_FSTREAM

// 1.4.5
// D_ENV_CPP98_HAS_SSTREAM
//   feature: 1 if <sstream> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_SSTREAM
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_SSTREAM 1
    #else
        #define D_ENV_CPP98_HAS_SSTREAM 0
    #endif
#endif  // D_ENV_CPP98_HAS_SSTREAM

// 1.4.6
// D_ENV_CPP98_HAS_IOMANIP
//   feature: 1 if <iomanip> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_IOMANIP
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_IOMANIP 1
    #else
        #define D_ENV_CPP98_HAS_IOMANIP 0
    #endif
#endif  // D_ENV_CPP98_HAS_IOMANIP

// 1.4.7
// D_ENV_CPP98_HAS_IOS
//   feature: 1 if <ios> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_IOS
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_IOS 1
    #else
        #define D_ENV_CPP98_HAS_IOS 0
    #endif
#endif  // D_ENV_CPP98_HAS_IOS

// 1.4.8
// D_ENV_CPP98_HAS_IOSFWD
//   feature: 1 if <iosfwd> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_IOSFWD
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_IOSFWD 1
    #else
        #define D_ENV_CPP98_HAS_IOSFWD 0
    #endif
#endif  // D_ENV_CPP98_HAS_IOSFWD

// 1.4.9
// D_ENV_CPP98_HAS_STREAMBUF
//   feature: 1 if <streambuf> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_STREAMBUF
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_STREAMBUF 1
    #else
        #define D_ENV_CPP98_HAS_STREAMBUF 0
    #endif
#endif  // D_ENV_CPP98_HAS_STREAMBUF

// 1.5    Utilities
//------------------------------------------------------------------------------
// 1.5.1
// D_ENV_CPP98_HAS_UTILITY
//   feature: 1 if <utility> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_UTILITY
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_UTILITY 1
    #else
        #define D_ENV_CPP98_HAS_UTILITY 0
    #endif
#endif  // D_ENV_CPP98_HAS_UTILITY

// 1.5.2
// D_ENV_CPP98_HAS_MEMORY
//   feature: 1 if <memory> is available (C++98, in its limited C++98
// form), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_MEMORY
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_MEMORY 1
    #else
        #define D_ENV_CPP98_HAS_MEMORY 0
    #endif
#endif  // D_ENV_CPP98_HAS_MEMORY

// 1.5.3
// D_ENV_CPP98_HAS_NEW
//   feature: 1 if <new> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_NEW
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_NEW 1
    #else
        #define D_ENV_CPP98_HAS_NEW 0
    #endif
#endif  // D_ENV_CPP98_HAS_NEW

// 1.5.4
// D_ENV_CPP98_HAS_TYPEINFO
//   feature: 1 if <typeinfo> is available (C++98) and RTTI is on, 0
// otherwise. RTTI counts as on when __GXX_RTTI, _CPPRTTI or __INTEL_RTTI__ is
// defined.
#ifndef D_ENV_CPP98_HAS_TYPEINFO
    #if defined(__cplusplus)
        #if ( (!defined(__GXX_RTTI)) &&                                        \
              (!defined(_CPPRTTI))   &&                                        \
              (!defined(__INTEL_RTTI__)) )
            #define D_ENV_CPP98_HAS_TYPEINFO 0
        #else
            #define D_ENV_CPP98_HAS_TYPEINFO 1
        #endif
    #else
        #define D_ENV_CPP98_HAS_TYPEINFO 0
    #endif
#endif  // D_ENV_CPP98_HAS_TYPEINFO

// 1.5.5
// D_ENV_CPP98_HAS_EXCEPTION
//   feature: 1 if <exception> is available (C++98) and exceptions are on, 0
// otherwise. Exceptions count as on when __cpp_exceptions, __EXCEPTIONS or
// _CPPUNWIND is defined.
#ifndef D_ENV_CPP98_HAS_EXCEPTION
    #if defined(__cplusplus)
        #if ( (!defined(__cpp_exceptions)) &&                                  \
              (!defined(__EXCEPTIONS))     &&                                  \
              (!defined(_CPPUNWIND)) )
            #define D_ENV_CPP98_HAS_EXCEPTION 0
        #else
            #define D_ENV_CPP98_HAS_EXCEPTION 1
        #endif
    #else
        #define D_ENV_CPP98_HAS_EXCEPTION 0
    #endif
#endif  // D_ENV_CPP98_HAS_EXCEPTION

// 1.5.6
// D_ENV_CPP98_HAS_STDEXCEPT
//   feature: 1 if <stdexcept> is available (C++98) and exceptions are on, 0
// otherwise, as for D_ENV_CPP98_HAS_EXCEPTION.
#ifndef D_ENV_CPP98_HAS_STDEXCEPT
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #if ( (!defined(__cpp_exceptions)) &&                                  \
              (!defined(__EXCEPTIONS))     &&                                  \
              (!defined(_CPPUNWIND)) )
            #define D_ENV_CPP98_HAS_STDEXCEPT 0
        #else
            #define D_ENV_CPP98_HAS_STDEXCEPT 1
        #endif
    #else
        #define D_ENV_CPP98_HAS_STDEXCEPT 0
    #endif
#endif  // D_ENV_CPP98_HAS_STDEXCEPT

// 1.5.7
// D_ENV_CPP98_HAS_LIMITS
//   feature: 1 if <limits> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_LIMITS
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_LIMITS 1
    #else
        #define D_ENV_CPP98_HAS_LIMITS 0
    #endif
#endif  // D_ENV_CPP98_HAS_LIMITS

// 1.6    Numerics
//------------------------------------------------------------------------------
// 1.6.1
// D_ENV_CPP98_HAS_COMPLEX
//   feature: 1 if <complex> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_COMPLEX
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_COMPLEX 1
    #else
        #define D_ENV_CPP98_HAS_COMPLEX 0
    #endif
#endif  // D_ENV_CPP98_HAS_COMPLEX

// 1.6.2
// D_ENV_CPP98_HAS_VALARRAY
//   feature: 1 if <valarray> is available (C++98), 0 otherwise.
#ifndef D_ENV_CPP98_HAS_VALARRAY
    #if ( (defined(__cplusplus)) &&                                            \
          (D_INTERNAL_ENV_CPP98_HOSTED) )
        #define D_ENV_CPP98_HAS_VALARRAY 1
    #else
        #define D_ENV_CPP98_HAS_VALARRAY 0
    #endif
#endif  // D_ENV_CPP98_HAS_VALARRAY


//==============================================================================
// 2.  AGGREGATE CHECKS
//==============================================================================
// 1 when every header of a group in section 1 is available.


// 2.1    Header groups
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_CPP98_HAS_ALL_CONTAINERS
//   feature: 1 if all C++98 container headers are available.
#define D_ENV_CPP98_HAS_ALL_CONTAINERS                                         \
    ( (D_ENV_CPP98_HAS_VECTOR) &&                                              \
      (D_ENV_CPP98_HAS_LIST)   &&                                              \
      (D_ENV_CPP98_HAS_DEQUE)  &&                                              \
      (D_ENV_CPP98_HAS_QUEUE)  &&                                              \
      (D_ENV_CPP98_HAS_STACK)  &&                                              \
      (D_ENV_CPP98_HAS_MAP)    &&                                              \
      (D_ENV_CPP98_HAS_SET)    &&                                              \
      (D_ENV_CPP98_HAS_BITSET) )

// 2.1.2
// D_ENV_CPP98_HAS_ALL_ALGORITHMS
//   feature: 1 if all C++98 algorithm/iterator headers are available.
#define D_ENV_CPP98_HAS_ALL_ALGORITHMS                                         \
    ( (D_ENV_CPP98_HAS_ALGORITHM)  &&                                          \
      (D_ENV_CPP98_HAS_ITERATOR)   &&                                          \
      (D_ENV_CPP98_HAS_FUNCTIONAL) &&                                          \
      (D_ENV_CPP98_HAS_NUMERIC) )

// 2.1.3
// D_ENV_CPP98_HAS_ALL_IOSTREAMS
//   feature: 1 if all C++98 I/O stream headers are available.
#define D_ENV_CPP98_HAS_ALL_IOSTREAMS                                          \
    ( (D_ENV_CPP98_HAS_IOSTREAM) &&                                            \
      (D_ENV_CPP98_HAS_ISTREAM)  &&                                            \
      (D_ENV_CPP98_HAS_OSTREAM)  &&                                            \
      (D_ENV_CPP98_HAS_FSTREAM)  &&                                            \
      (D_ENV_CPP98_HAS_SSTREAM)  &&                                            \
      (D_ENV_CPP98_HAS_IOMANIP)  &&                                            \
      (D_ENV_CPP98_HAS_IOS)      &&                                            \
      (D_ENV_CPP98_HAS_IOSFWD)   &&                                            \
      (D_ENV_CPP98_HAS_STREAMBUF) )

// 2.1.4
// D_ENV_CPP98_HAS_ALL_UTILITIES
//   feature: 1 if all C++98 utility headers are available.
#define D_ENV_CPP98_HAS_ALL_UTILITIES                                          \
    ( (D_ENV_CPP98_HAS_UTILITY)   &&                                           \
      (D_ENV_CPP98_HAS_MEMORY)    &&                                           \
      (D_ENV_CPP98_HAS_NEW)       &&                                           \
      (D_ENV_CPP98_HAS_TYPEINFO)  &&                                           \
      (D_ENV_CPP98_HAS_EXCEPTION) &&                                           \
      (D_ENV_CPP98_HAS_STDEXCEPT) &&                                           \
      (D_ENV_CPP98_HAS_LIMITS) )

// 2.1.5
// D_ENV_CPP98_HAS_ALL_NUMERICS
//   feature: 1 if all C++98 numeric headers are available.
#define D_ENV_CPP98_HAS_ALL_NUMERICS                                           \
    ( (D_ENV_CPP98_HAS_COMPLEX) &&                                             \
      (D_ENV_CPP98_HAS_VALARRAY) )

// 2.2    Whole library
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_CPP98_HAS_FULL_STL
//   feature: 1 if the complete C++98 standard library is available.
#define D_ENV_CPP98_HAS_FULL_STL                                               \
    ( (D_ENV_CPP98_HAS_ALL_CONTAINERS) &&                                      \
      (D_ENV_CPP98_HAS_ALL_ALGORITHMS) &&                                      \
      (D_ENV_CPP98_HAS_ALL_IOSTREAMS)  &&                                      \
      (D_ENV_CPP98_HAS_ALL_UTILITIES)  &&                                      \
      (D_ENV_CPP98_HAS_ALL_NUMERICS)   &&                                      \
      (D_ENV_CPP98_HAS_STRING)         &&                                      \
      (D_ENV_CPP98_HAS_LOCALE) )


#endif  // DJINTERP_ENV_CPP_ENV_CPP98_H
