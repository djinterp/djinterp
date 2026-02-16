#include "..\..\inc\c\env.h"


#ifdef D_DEBUG_
    // print compiler info function (for runtime use)
    
    void print_compiler_info(void) 
    {
        printf("compiler: %s\n", D_ENV_COMPILER_FULL_NAME);
        printf("version: %d.%d.%d\n", D_ENV_COMPILER_MAJOR, D_ENV_COMPILER_MINOR, D_ENV_COMPILER_PATCHLEVEL);
        printf("version String: %s\n", D_ENV_COMPILER_VERSION_STRING);
        printf("architecture: %s (%d-bit)\n", D_ENV_ARCH_NAME, D_ENV_ARCH_BITS);
        printf("endianness: %s\n", 
                (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE) ? "Little Endian" :
                (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG)    ? "Big Endian" : "Unknown");
        printf("operating System: %s\n", D_ENV_OS_NAME);
        printf("OS ID: 0x%02X\n", D_ENV_OS_ID);
    
        // OS classification
        if (D_ENV_IS_OS_MOBILE(D_ENV_OS_ID)) 
        {
            printf("OS Type: Mobile\n");
        } 
        else if (D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)) 
        {
            printf("OS Type: Windows\n");
        } 
        else if (D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)) 
        {
            printf("OS Type: Unix-like\n");
        } 
        else if (D_ENV_IS_OS_DISCONTINUED(D_ENV_OS_ID)) 
        {
            printf("OS Type: Discontinued\n");
        } 
        else if (D_ENV_IS_OS_UNSUPPORTED(D_ENV_OS_ID)) 
        {
            printf("OS Type: Unsupported\n");
        }
    
    #ifdef __cplusplus
        printf("Language Standard: %s\n", D_ENV_LANG_CPP_STANDARD_NAME);
    #else
        printf("Language Standard: %s\n", D_ENV_LANG_C_STANDARD_NAME);
    #endif
    
        printf("Build Type: %s\n", D_ENV_BUILD_TYPE);
    
        // Architecture family info
        if (D_ENV_ARCH_IS_X86_FAMILY()) 
        {
            printf("Architecture Family: x86\n");
        } 
        else if (D_ENV_ARCH_IS_ARM_FAMILY()) 
        {
            printf("Architecture Family: ARM\n");
        }
    
        // Additional compiler-specific info
    #ifdef D_ENV_COMPILER_CLANG
        printf("Clang version string: %s\n", __clang_version__);
    #ifdef __apple_build_version__
        printf("Apple build version: %d\n", __apple_build_version__);
    #endif
    #endif

        #ifdef D_ENV_DETECTED_COMPILER_MSVC
            printf("MSVC version: %d\n", _MSC_VER);
            printf("MSVC full version: %d\n", _MSC_FULL_VER);
        #endif

        #ifdef D_ENV_COMPILER_GCC
            #ifdef  __VERSION__
                printf("GCC version string: %s\n", __VERSION__);
            #endif  // __VERSION__
        #endif
    }

    // compile-time assertions for version checking (C++11 and later)
    #ifdef __cplusplus
        #if __cplusplus >= 201103L
            #include <type_traits>
            #define D_ENV_COMPILER_STATIC_ASSERT(condition, message) static_assert(condition, message)
        #else
            #define D_ENV_COMPILER_STATIC_ASSERT(condition, message) typedef char static_assertion_##__LINE__[(condition) ? 1 : -1]
        #endif  // __cplusplus >= ...
    #else
        #define D_ENV_COMPILER_STATIC_ASSERT(condition, message) typedef char static_assertion_##__LINE__[(condition) ? 1 : -1]
    #endif  // __cplusplus
#endif  // D_DEBUG_

