/*******************************************************************************
* vparse [cl_standin]                                                 identity.h
*
* The second half of a unit compiled by the stand-in for cl: the compiler's
* identity, changed.
*   The standard headers have been read by now, under the identity of the
* compiler that is really at work -- a MinGW-w64 Clang -- and their include
* guards keep them from being read again. This header then takes that
* identity away and puts cl 19.51's in its place: the macros djinterp's env
* asks about. Everything after it is read as cl's, so env takes the branches
* it takes under cl, and what those branches select is compiled for real.
*   The stand-in's driver, the script cl beside this file, includes it
* after the standard headers and defines the three CL_STANDIN_* macros it
* reads; nothing else includes it.
*   The names it removes and defines are the compilers' own, reserved ones:
* defining them is what this file is for.
*
*
* path:      /build/cmake/config/djinterp/tools/vparse/cl_standin/identity.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.15
*******************************************************************************/
#ifndef VPARSE_TOOLS_CL_STANDIN_IDENTITY_H
#define VPARSE_TOOLS_CL_STANDIN_IDENTITY_H 1

// the real compiler's identity, and its library's
#undef __clang__
#undef __clang_major__
#undef __clang_minor__
#undef __clang_patchlevel__
#undef __clang_version__
#undef __GNUC__
#undef __GNUC_MINOR__
#undef __GNUC_PATCHLEVEL__
#undef __GNUG__
#undef __MINGW32__
#undef __MINGW64__
#undef __MINGW64_VERSION_MAJOR

// cl 19.51.36260, for a 64-bit target
#define _MSC_VER           1951
#define _MSC_FULL_VER      195136260
#define _MSC_BUILD         0
#define _MSC_EXTENSIONS    1
#define _M_X64             100
#define _M_AMD64           100
#define _INTEGRAL_MAX_BITS 64

// CL_STANDIN_TRADITIONAL
//   0 where the unit was given /Zc:preprocessor, 1 where it was not: what cl
// sets _MSVC_TRADITIONAL to.
#define _MSVC_TRADITIONAL CL_STANDIN_TRADITIONAL

#ifdef __cplusplus
    // CL_STANDIN_MSVC_LANG
    //   the value cl gives _MSVC_LANG at the level the unit was compiled at.
    #define _MSVC_LANG CL_STANDIN_MSVC_LANG
    #define _CPPUNWIND 1
    #define _CPPRTTI   1
#else
    // CL_STANDIN_C_DEFAULT
    //   defined where a C unit was given no /std: -- cl's default C mode,
    // which defines neither of these.
    #ifdef CL_STANDIN_C_DEFAULT
        #undef __STDC_VERSION__
        #undef __STDC__
    #endif  // CL_STANDIN_C_DEFAULT
#endif  // __cplusplus

#endif  // VPARSE_TOOLS_CL_STANDIN_IDENTITY_H
