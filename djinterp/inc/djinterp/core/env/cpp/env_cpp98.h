/******************************************************************************
* djinterp [core]                                                 env_cpp98.h
* 
* djinterp C++98 standard library header detection:
* This header provides compile-time detection of C++98 standard library
* headers, which predate the feature test macro system. This serves as a
* complement to cpp_features.h, which handles C++11+ feature detection via
* __cpp_lib_* macros.
*
* SCOPE:
*   - C++98 standard library headers only
*   - Assumes availability in any conforming C++ implementation
*   - Provides platform-specific workarounds for incomplete implementations
*   - Does NOT duplicate functionality from cpp_features.h
*
* USAGE:
*   Include env.h before including this header:
*     #include "env.h"
*     #include "env_cpp98.h"
*
* NAMING CONVENTION:
*   D_ENV_CPP98_HAS_[HEADER_NAME] - 1 if available, 0 otherwise
*
* path:      \inc\env_cpp98.h                                           
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.02.08
******************************************************************************/

#ifndef DJINTERP_CPP98_ENVIRONMENT_
#define DJINTERP_CPP98_ENVIRONMENT_ 1

// require env.h to be included first
//#ifndef DJINTERP_C_ENVIRONMENT_
//    #error "env_cpp98.h requires env.h to be included first"
//#endif
//
//// only meaningful in C++ mode
//#ifndef __cplusplus
//    #error "env_cpp98.h can only be used in C++ compilation mode"
//#endif


// =============================================================================
// I.   C++98 STANDARD LIBRARY HEADERS
// =============================================================================

// NOTE: In any conforming C++98 or later implementation, these headers should
// be available. The macros primarily serve as documentation and provide hooks
// for handling non-conforming or embedded implementations.

// -----------------------------------------------------------------------------
// A. Containers
// -----------------------------------------------------------------------------

// D_ENV_CPP98_HAS_VECTOR
//   feature: detect if <vector> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_VECTOR
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_VECTOR 1
    #else
        #define D_ENV_CPP98_HAS_VECTOR 0
    #endif
#endif

// D_ENV_CPP98_HAS_LIST
//   feature: detect if <list> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_LIST
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_LIST 1
    #else
        #define D_ENV_CPP98_HAS_LIST 0
    #endif
#endif

// D_ENV_CPP98_HAS_DEQUE
//   feature: detect if <deque> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_DEQUE
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_DEQUE 1
    #else
        #define D_ENV_CPP98_HAS_DEQUE 0
    #endif
#endif

// D_ENV_CPP98_HAS_QUEUE
//   feature: detect if <queue> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_QUEUE
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_QUEUE 1
    #else
        #define D_ENV_CPP98_HAS_QUEUE 0
    #endif
#endif

// D_ENV_CPP98_HAS_STACK
//   feature: detect if <stack> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_STACK
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_STACK 1
    #else
        #define D_ENV_CPP98_HAS_STACK 0
    #endif
#endif

// D_ENV_CPP98_HAS_MAP
//   feature: detect if <map> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_MAP
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_MAP 1
    #else
        #define D_ENV_CPP98_HAS_MAP 0
    #endif
#endif

// D_ENV_CPP98_HAS_SET
//   feature: detect if <set> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_SET
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_SET 1
    #else
        #define D_ENV_CPP98_HAS_SET 0
    #endif
#endif

// D_ENV_CPP98_HAS_BITSET
//   feature: detect if <bitset> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_BITSET
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_BITSET 1
    #else
        #define D_ENV_CPP98_HAS_BITSET 0
    #endif
#endif


// -----------------------------------------------------------------------------
// B. Algorithms and Iterators
// -----------------------------------------------------------------------------

// D_ENV_CPP98_HAS_ALGORITHM
//   feature: detect if <algorithm> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_ALGORITHM
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_ALGORITHM 1
    #else
        #define D_ENV_CPP98_HAS_ALGORITHM 0
    #endif
#endif

// D_ENV_CPP98_HAS_ITERATOR
//   feature: detect if <iterator> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_ITERATOR
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_ITERATOR 1
    #else
        #define D_ENV_CPP98_HAS_ITERATOR 0
    #endif
#endif

// D_ENV_CPP98_HAS_FUNCTIONAL
//   feature: detect if <functional> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_FUNCTIONAL
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_FUNCTIONAL 1
    #else
        #define D_ENV_CPP98_HAS_FUNCTIONAL 0
    #endif
#endif

// D_ENV_CPP98_HAS_NUMERIC
//   feature: detect if <numeric> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_NUMERIC
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_NUMERIC 1
    #else
        #define D_ENV_CPP98_HAS_NUMERIC 0
    #endif
#endif


// -----------------------------------------------------------------------------
// C. Strings and Localization
// -----------------------------------------------------------------------------

// D_ENV_CPP98_HAS_STRING
//   feature: detect if <string> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_STRING
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_STRING 1
    #else
        #define D_ENV_CPP98_HAS_STRING 0
    #endif
#endif

// D_ENV_CPP98_HAS_LOCALE
//   feature: detect if <locale> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_LOCALE
    #if defined(__cplusplus)
        // some embedded platforms may not have locale support
        #if !defined(__AVR__) && !defined(__ANDROID__)
            #define D_ENV_CPP98_HAS_LOCALE 1
        #else
            #define D_ENV_CPP98_HAS_LOCALE 0
        #endif
    #else
        #define D_ENV_CPP98_HAS_LOCALE 0
    #endif
#endif


// -----------------------------------------------------------------------------
// D. I/O Streams
// -----------------------------------------------------------------------------

// D_ENV_CPP98_HAS_IOSTREAM
//   feature: detect if <iostream> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_IOSTREAM
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_IOSTREAM 1
    #else
        #define D_ENV_CPP98_HAS_IOSTREAM 0
    #endif
#endif

// D_ENV_CPP98_HAS_ISTREAM
//   feature: detect if <istream> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_ISTREAM
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_ISTREAM 1
    #else
        #define D_ENV_CPP98_HAS_ISTREAM 0
    #endif
#endif

// D_ENV_CPP98_HAS_OSTREAM
//   feature: detect if <ostream> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_OSTREAM
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_OSTREAM 1
    #else
        #define D_ENV_CPP98_HAS_OSTREAM 0
    #endif
#endif

// D_ENV_CPP98_HAS_FSTREAM
//   feature: detect if <fstream> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_FSTREAM
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_FSTREAM 1
    #else
        #define D_ENV_CPP98_HAS_FSTREAM 0
    #endif
#endif

// D_ENV_CPP98_HAS_SSTREAM
//   feature: detect if <sstream> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_SSTREAM
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_SSTREAM 1
    #else
        #define D_ENV_CPP98_HAS_SSTREAM 0
    #endif
#endif

// D_ENV_CPP98_HAS_IOMANIP
//   feature: detect if <iomanip> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_IOMANIP
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_IOMANIP 1
    #else
        #define D_ENV_CPP98_HAS_IOMANIP 0
    #endif
#endif

// D_ENV_CPP98_HAS_IOS
//   feature: detect if <ios> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_IOS
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_IOS 1
    #else
        #define D_ENV_CPP98_HAS_IOS 0
    #endif
#endif

// D_ENV_CPP98_HAS_IOSFWD
//   feature: detect if <iosfwd> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_IOSFWD
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_IOSFWD 1
    #else
        #define D_ENV_CPP98_HAS_IOSFWD 0
    #endif
#endif

// D_ENV_CPP98_HAS_STREAMBUF
//   feature: detect if <streambuf> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_STREAMBUF
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_STREAMBUF 1
    #else
        #define D_ENV_CPP98_HAS_STREAMBUF 0
    #endif
#endif


// -----------------------------------------------------------------------------
// E. Utilities
// -----------------------------------------------------------------------------

// D_ENV_CPP98_HAS_UTILITY
//   feature: detect if <utility> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_UTILITY
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_UTILITY 1
    #else
        #define D_ENV_CPP98_HAS_UTILITY 0
    #endif
#endif

// D_ENV_CPP98_HAS_MEMORY
//   feature: detect if <memory> is available (C++98+, but limited)
#ifndef D_ENV_CPP98_HAS_MEMORY
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_MEMORY 1
    #else
        #define D_ENV_CPP98_HAS_MEMORY 0
    #endif
#endif

// D_ENV_CPP98_HAS_NEW
//   feature: detect if <new> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_NEW
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_NEW 1
    #else
        #define D_ENV_CPP98_HAS_NEW 0
    #endif
#endif

// D_ENV_CPP98_HAS_TYPEINFO
//   feature: detect if <typeinfo> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_TYPEINFO
    #if defined(__cplusplus)
        // some platforms disable RTTI
        #if !defined(__GXX_RTTI) && !defined(_CPPRTTI) && !defined(__INTEL_RTTI__)
            #define D_ENV_CPP98_HAS_TYPEINFO 0
        #else
            #define D_ENV_CPP98_HAS_TYPEINFO 1
        #endif
    #else
        #define D_ENV_CPP98_HAS_TYPEINFO 0
    #endif
#endif

// D_ENV_CPP98_HAS_EXCEPTION
//   feature: detect if <exception> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_EXCEPTION
    #if defined(__cplusplus)
        // some platforms disable exceptions
        #if !defined(__cpp_exceptions) && !defined(__EXCEPTIONS) && !defined(_CPPUNWIND)
            #define D_ENV_CPP98_HAS_EXCEPTION 0
        #else
            #define D_ENV_CPP98_HAS_EXCEPTION 1
        #endif
    #else
        #define D_ENV_CPP98_HAS_EXCEPTION 0
    #endif
#endif

// D_ENV_CPP98_HAS_STDEXCEPT
//   feature: detect if <stdexcept> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_STDEXCEPT
    #if defined(__cplusplus)
        #if !defined(__cpp_exceptions) && !defined(__EXCEPTIONS) && !defined(_CPPUNWIND)
            #define D_ENV_CPP98_HAS_STDEXCEPT 0
        #else
            #define D_ENV_CPP98_HAS_STDEXCEPT 1
        #endif
    #else
        #define D_ENV_CPP98_HAS_STDEXCEPT 0
    #endif
#endif

// D_ENV_CPP98_HAS_LIMITS
//   feature: detect if <limits> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_LIMITS
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_LIMITS 1
    #else
        #define D_ENV_CPP98_HAS_LIMITS 0
    #endif
#endif


// -----------------------------------------------------------------------------
// F. Numerics
// -----------------------------------------------------------------------------

// D_ENV_CPP98_HAS_COMPLEX
//   feature: detect if <complex> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_COMPLEX
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_COMPLEX 1
    #else
        #define D_ENV_CPP98_HAS_COMPLEX 0
    #endif
#endif

// D_ENV_CPP98_HAS_VALARRAY
//   feature: detect if <valarray> is available (C++98+)
#ifndef D_ENV_CPP98_HAS_VALARRAY
    #if defined(__cplusplus)
        #define D_ENV_CPP98_HAS_VALARRAY 1
    #else
        #define D_ENV_CPP98_HAS_VALARRAY 0
    #endif
#endif


// =============================================================================
// II.  AGGREGATE CHECKS
// =============================================================================

// D_ENV_CPP98_HAS_ALL_CONTAINERS
//   constant: 1 if all C++98 container headers are available
#define D_ENV_CPP98_HAS_ALL_CONTAINERS  \
    ( D_ENV_CPP98_HAS_VECTOR    &&      \
      D_ENV_CPP98_HAS_LIST      &&      \
      D_ENV_CPP98_HAS_DEQUE     &&      \
      D_ENV_CPP98_HAS_QUEUE     &&      \
      D_ENV_CPP98_HAS_STACK     &&      \
      D_ENV_CPP98_HAS_MAP       &&      \
      D_ENV_CPP98_HAS_SET       &&      \
      D_ENV_CPP98_HAS_BITSET )

// D_ENV_CPP98_HAS_ALL_ALGORITHMS
//   constant: 1 if all C++98 algorithm/iterator headers are available
#define D_ENV_CPP98_HAS_ALL_ALGORITHMS  \
    ( D_ENV_CPP98_HAS_ALGORITHM  &&     \
      D_ENV_CPP98_HAS_ITERATOR   &&     \
      D_ENV_CPP98_HAS_FUNCTIONAL &&     \
      D_ENV_CPP98_HAS_NUMERIC )

// D_ENV_CPP98_HAS_ALL_IOSTREAMS
//   constant: 1 if all C++98 I/O stream headers are available
#define D_ENV_CPP98_HAS_ALL_IOSTREAMS  \
    ( D_ENV_CPP98_HAS_IOSTREAM  &&     \
      D_ENV_CPP98_HAS_ISTREAM   &&     \
      D_ENV_CPP98_HAS_OSTREAM   &&     \
      D_ENV_CPP98_HAS_FSTREAM   &&     \
      D_ENV_CPP98_HAS_SSTREAM   &&     \
      D_ENV_CPP98_HAS_IOMANIP   &&     \
      D_ENV_CPP98_HAS_IOS       &&     \
      D_ENV_CPP98_HAS_IOSFWD    &&     \
      D_ENV_CPP98_HAS_STREAMBUF )

// D_ENV_CPP98_HAS_ALL_UTILITIES
//   constant: 1 if all C++98 utility headers are available
#define D_ENV_CPP98_HAS_ALL_UTILITIES  \
    ( D_ENV_CPP98_HAS_UTILITY    &&    \
      D_ENV_CPP98_HAS_MEMORY     &&    \
      D_ENV_CPP98_HAS_NEW        &&    \
      D_ENV_CPP98_HAS_TYPEINFO   &&    \
      D_ENV_CPP98_HAS_EXCEPTION  &&    \
      D_ENV_CPP98_HAS_STDEXCEPT  &&    \
      D_ENV_CPP98_HAS_LIMITS )

// D_ENV_CPP98_HAS_ALL_NUMERICS
//   constant: 1 if all C++98 numeric headers are available
#define D_ENV_CPP98_HAS_ALL_NUMERICS  \
    ( D_ENV_CPP98_HAS_COMPLEX  &&     \
      D_ENV_CPP98_HAS_VALARRAY )

// D_ENV_CPP98_HAS_FULL_STL
//   constant: 1 if complete C++98 STL is available
#define D_ENV_CPP98_HAS_FULL_STL             \
    ( D_ENV_CPP98_HAS_ALL_CONTAINERS  &&     \
      D_ENV_CPP98_HAS_ALL_ALGORITHMS  &&     \
      D_ENV_CPP98_HAS_ALL_IOSTREAMS   &&     \
      D_ENV_CPP98_HAS_ALL_UTILITIES   &&     \
      D_ENV_CPP98_HAS_ALL_NUMERICS    &&     \
      D_ENV_CPP98_HAS_STRING          &&     \
      D_ENV_CPP98_HAS_LOCALE )


// =============================================================================
// III. NOTES ON PLATFORM-SPECIFIC BEHAVIORS
// =============================================================================

// NOTE: Embedded Platforms
//   Some embedded C++ implementations (e.g., Arduino, AVR-GCC) may provide
//   only a subset of the C++98 standard library. Common omissions include:
//   - <locale> (locale support)
//   - <exception> and <stdexcept> (when exceptions are disabled)
//   - <typeinfo> (when RTTI is disabled)
//   - I/O streams on bare-metal systems
//
// NOTE: Exception Handling
//   The D_ENV_CPP98_HAS_EXCEPTION and D_ENV_CPP98_HAS_STDEXCEPT macros
//   detect if exception support is enabled via compiler flags. To disable
//   exceptions manually, define these to 0 before including this header.
//
// NOTE: RTTI Support
//   The D_ENV_CPP98_HAS_TYPEINFO macro detects if RTTI is enabled.
//   Some platforms disable RTTI for performance/size reasons.
//
// NOTE: Custom Override
//   All macros can be pre-defined to override detection, useful for:
//   - Testing partial STL implementations
//   - Working around platform-specific issues
//   - Simulating different environments


#endif  // DJINTERP_CPP98_ENVIRONMENT_
