/*******************************************************************************
* bitwisedom [tests]                                                 x86_cpuid.c
*
* The x86_cpuid example against the compiler's <cpuid.h>: every bit
* it names, its mask in its register compared with the compiler's at
* compile time, under each name GCC's and Clang's headers give it.
* Built where the build targets x86 with GCC or Clang. Written by
* tools/x86_cpuid_bitfield.py with the example; a name the
* compiler's header does not define is passed over.
*   A <cpuid.h> names a bit's mask, not the register it is in, so the
* register each bit is in is the example's word; the example's notes
* say which register each word is.
*
* path:      /tests/djinterp/tools/bitwisedom/platform/x86_cpuid.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.11
*******************************************************************************/

#include <cpuid.h>               // bit_*
#include "x86_cpuid.h"           // the example, as bitwisedom-gen wrote it


// SAME
//   our bit's mask in its word is the compiler's mask for it.
#define SAME(_ours, _theirs)                                                   \
    _Static_assert(CPUID_MASK(_ours) == (unsigned int)(_theirs),               \
                   #_ours " is " #_theirs)


// Leaf 1, ECX, word 0
#if defined(bit_SSE3)
SAME(CPUID_SSE3, bit_SSE3);
#endif
#if defined(bit_PCLMUL)
SAME(CPUID_PCLMULQDQ, bit_PCLMUL);
#endif
#if defined(bit_PCLMULQDQ)
SAME(CPUID_PCLMULQDQ, bit_PCLMULQDQ);
#endif
#if defined(bit_DTES64)
SAME(CPUID_DTES64, bit_DTES64);
#endif
#if defined(bit_MONITOR)
SAME(CPUID_MONITOR, bit_MONITOR);
#endif
#if defined(bit_DSCPL)
SAME(CPUID_DS_CPL, bit_DSCPL);
#endif
#if defined(bit_VMX)
SAME(CPUID_VMX, bit_VMX);
#endif
#if defined(bit_SMX)
SAME(CPUID_SMX, bit_SMX);
#endif
#if defined(bit_EIST)
SAME(CPUID_EIST, bit_EIST);
#endif
#if defined(bit_TM2)
SAME(CPUID_TM2, bit_TM2);
#endif
#if defined(bit_SSSE3)
SAME(CPUID_SSSE3, bit_SSSE3);
#endif
#if defined(bit_CNXTID)
SAME(CPUID_CNXT_ID, bit_CNXTID);
#endif
#if defined(bit_FMA)
SAME(CPUID_FMA, bit_FMA);
#endif
#if defined(bit_CMPXCHG16B)
SAME(CPUID_CMPXCHG16B, bit_CMPXCHG16B);
#endif
#if defined(bit_xTPR)
SAME(CPUID_XTPR, bit_xTPR);
#endif
#if defined(bit_PDCM)
SAME(CPUID_PDCM, bit_PDCM);
#endif
#if defined(bit_PCID)
SAME(CPUID_PCID, bit_PCID);
#endif
#if defined(bit_DCA)
SAME(CPUID_DCA, bit_DCA);
#endif
#if defined(bit_SSE4_1)
SAME(CPUID_SSE4_1, bit_SSE4_1);
#endif
#if defined(bit_SSE41)
SAME(CPUID_SSE4_1, bit_SSE41);
#endif
#if defined(bit_SSE4_2)
SAME(CPUID_SSE4_2, bit_SSE4_2);
#endif
#if defined(bit_SSE42)
SAME(CPUID_SSE4_2, bit_SSE42);
#endif
#if defined(bit_x2APIC)
SAME(CPUID_X2APIC, bit_x2APIC);
#endif
#if defined(bit_MOVBE)
SAME(CPUID_MOVBE, bit_MOVBE);
#endif
#if defined(bit_POPCNT)
SAME(CPUID_POPCNT, bit_POPCNT);
#endif
#if defined(bit_TSCDeadline)
SAME(CPUID_TSC_DEADLINE, bit_TSCDeadline);
#endif
#if defined(bit_AES)
SAME(CPUID_AES, bit_AES);
#endif
#if defined(bit_AESNI)
SAME(CPUID_AES, bit_AESNI);
#endif
#if defined(bit_XSAVE)
SAME(CPUID_XSAVE, bit_XSAVE);
#endif
#if defined(bit_OSXSAVE)
SAME(CPUID_OSXSAVE, bit_OSXSAVE);
#endif
#if defined(bit_AVX)
SAME(CPUID_AVX, bit_AVX);
#endif
#if defined(bit_F16C)
SAME(CPUID_F16C, bit_F16C);
#endif
#if defined(bit_RDRND)
SAME(CPUID_RDRAND, bit_RDRND);
#endif


// Leaf 1, EDX, word 1
#if defined(bit_FPU)
SAME(CPUID_FPU, bit_FPU);
#endif
#if defined(bit_VME)
SAME(CPUID_VME, bit_VME);
#endif
#if defined(bit_DE)
SAME(CPUID_DE, bit_DE);
#endif
#if defined(bit_PSE)
SAME(CPUID_PSE, bit_PSE);
#endif
#if defined(bit_TSC)
SAME(CPUID_TSC, bit_TSC);
#endif
#if defined(bit_MSR)
SAME(CPUID_MSR, bit_MSR);
#endif
#if defined(bit_PAE)
SAME(CPUID_PAE, bit_PAE);
#endif
#if defined(bit_MCE)
SAME(CPUID_MCE, bit_MCE);
#endif
#if defined(bit_CMPXCHG8B)
SAME(CPUID_CX8, bit_CMPXCHG8B);
#endif
#if defined(bit_CX8)
SAME(CPUID_CX8, bit_CX8);
#endif
#if defined(bit_APIC)
SAME(CPUID_APIC, bit_APIC);
#endif
#if defined(bit_SEP)
SAME(CPUID_SEP, bit_SEP);
#endif
#if defined(bit_MTRR)
SAME(CPUID_MTRR, bit_MTRR);
#endif
#if defined(bit_PGE)
SAME(CPUID_PGE, bit_PGE);
#endif
#if defined(bit_MCA)
SAME(CPUID_MCA, bit_MCA);
#endif
#if defined(bit_CMOV)
SAME(CPUID_CMOV, bit_CMOV);
#endif
#if defined(bit_PAT)
SAME(CPUID_PAT, bit_PAT);
#endif
#if defined(bit_PSE36)
SAME(CPUID_PSE_36, bit_PSE36);
#endif
#if defined(bit_PSN)
SAME(CPUID_PSN, bit_PSN);
#endif
#if defined(bit_CLFSH)
SAME(CPUID_CLFSH, bit_CLFSH);
#endif
#if defined(bit_DS)
SAME(CPUID_DS, bit_DS);
#endif
#if defined(bit_ACPI)
SAME(CPUID_ACPI, bit_ACPI);
#endif
#if defined(bit_MMX)
SAME(CPUID_MMX, bit_MMX);
#endif
#if defined(bit_FXSAVE)
SAME(CPUID_FXSR, bit_FXSAVE);
#endif
#if defined(bit_FXSR)
SAME(CPUID_FXSR, bit_FXSR);
#endif
#if defined(bit_SSE)
SAME(CPUID_SSE, bit_SSE);
#endif
#if defined(bit_SSE2)
SAME(CPUID_SSE2, bit_SSE2);
#endif
#if defined(bit_SS)
SAME(CPUID_SS, bit_SS);
#endif
#if defined(bit_HTT)
SAME(CPUID_HTT, bit_HTT);
#endif
#if defined(bit_TM)
SAME(CPUID_TM, bit_TM);
#endif
#if defined(bit_PBE)
SAME(CPUID_PBE, bit_PBE);
#endif


// Leaf 7, EBX, word 2
#if defined(bit_FSGSBASE)
SAME(CPUID_FSGSBASE, bit_FSGSBASE);
#endif
#if defined(bit_SGX)
SAME(CPUID_SGX, bit_SGX);
#endif
#if defined(bit_BMI)
SAME(CPUID_BMI1, bit_BMI);
#endif
#if defined(bit_HLE)
SAME(CPUID_HLE, bit_HLE);
#endif
#if defined(bit_AVX2)
SAME(CPUID_AVX2, bit_AVX2);
#endif
#if defined(bit_SMEP)
SAME(CPUID_SMEP, bit_SMEP);
#endif
#if defined(bit_BMI2)
SAME(CPUID_BMI2, bit_BMI2);
#endif
#if defined(bit_ENH_MOVSB)
SAME(CPUID_ERMS, bit_ENH_MOVSB);
#endif
#if defined(bit_INVPCID)
SAME(CPUID_INVPCID, bit_INVPCID);
#endif
#if defined(bit_RTM)
SAME(CPUID_RTM, bit_RTM);
#endif
#if defined(bit_MPX)
SAME(CPUID_MPX, bit_MPX);
#endif
#if defined(bit_AVX512F)
SAME(CPUID_AVX512F, bit_AVX512F);
#endif
#if defined(bit_AVX512DQ)
SAME(CPUID_AVX512DQ, bit_AVX512DQ);
#endif
#if defined(bit_RDSEED)
SAME(CPUID_RDSEED, bit_RDSEED);
#endif
#if defined(bit_ADX)
SAME(CPUID_ADX, bit_ADX);
#endif
#if defined(bit_AVX512IFMA)
SAME(CPUID_AVX512_IFMA, bit_AVX512IFMA);
#endif
#if defined(bit_CLFLUSHOPT)
SAME(CPUID_CLFLUSHOPT, bit_CLFLUSHOPT);
#endif
#if defined(bit_CLWB)
SAME(CPUID_CLWB, bit_CLWB);
#endif
#if defined(bit_AVX512PF)
SAME(CPUID_AVX512PF, bit_AVX512PF);
#endif
#if defined(bit_AVX512ER)
SAME(CPUID_AVX512ER, bit_AVX512ER);
#endif
#if defined(bit_AVX512CD)
SAME(CPUID_AVX512CD, bit_AVX512CD);
#endif
#if defined(bit_SHA)
SAME(CPUID_SHA, bit_SHA);
#endif
#if defined(bit_AVX512BW)
SAME(CPUID_AVX512BW, bit_AVX512BW);
#endif
#if defined(bit_AVX512VL)
SAME(CPUID_AVX512VL, bit_AVX512VL);
#endif


// Leaf 7, ECX, word 3
#if defined(bit_PREFETCHWT1)
SAME(CPUID_PREFETCHWT1, bit_PREFETCHWT1);
#endif
#if defined(bit_PREFTCHWT1)
SAME(CPUID_PREFETCHWT1, bit_PREFTCHWT1);
#endif
#if defined(bit_AVX512VBMI)
SAME(CPUID_AVX512_VBMI, bit_AVX512VBMI);
#endif
#if ( (defined(bit_PKU)) && (!defined(__clang__)) )
SAME(CPUID_PKU, bit_PKU);
#endif
#if defined(bit_OSPKE)
SAME(CPUID_OSPKE, bit_OSPKE);
#endif
#if defined(bit_WAITPKG)
SAME(CPUID_WAITPKG, bit_WAITPKG);
#endif
#if defined(bit_AVX512VBMI2)
SAME(CPUID_AVX512_VBMI2, bit_AVX512VBMI2);
#endif
#if defined(bit_SHSTK)
SAME(CPUID_CET_SS, bit_SHSTK);
#endif
#if defined(bit_GFNI)
SAME(CPUID_GFNI, bit_GFNI);
#endif
#if defined(bit_VAES)
SAME(CPUID_VAES, bit_VAES);
#endif
#if defined(bit_VPCLMULQDQ)
SAME(CPUID_VPCLMULQDQ, bit_VPCLMULQDQ);
#endif
#if defined(bit_AVX512VNNI)
SAME(CPUID_AVX512_VNNI, bit_AVX512VNNI);
#endif
#if defined(bit_AVX512BITALG)
SAME(CPUID_AVX512_BITALG, bit_AVX512BITALG);
#endif
#if defined(bit_AVX512VPOPCNTDQ)
SAME(CPUID_AVX512_VPOPCNTDQ, bit_AVX512VPOPCNTDQ);
#endif
#if defined(bit_RDPID)
SAME(CPUID_RDPID, bit_RDPID);
#endif
#if defined(bit_KL)
SAME(CPUID_KL, bit_KL);
#endif
#if defined(bit_CLDEMOTE)
SAME(CPUID_CLDEMOTE, bit_CLDEMOTE);
#endif
#if defined(bit_MOVDIRI)
SAME(CPUID_MOVDIRI, bit_MOVDIRI);
#endif
#if defined(bit_MOVDIR64B)
SAME(CPUID_MOVDIR64B, bit_MOVDIR64B);
#endif
#if defined(bit_ENQCMD)
SAME(CPUID_ENQCMD, bit_ENQCMD);
#endif


// Leaf 7, EDX, word 4
#if defined(bit_AVX5124VNNIW)
SAME(CPUID_AVX512_4VNNIW, bit_AVX5124VNNIW);
#endif
#if defined(bit_AVX5124FMAPS)
SAME(CPUID_AVX512_4FMAPS, bit_AVX5124FMAPS);
#endif
#if defined(bit_UINTR)
SAME(CPUID_UINTR, bit_UINTR);
#endif
#if defined(bit_AVX512VP2INTERSECT)
SAME(CPUID_AVX512_VP2INTERSECT, bit_AVX512VP2INTERSECT);
#endif
#if defined(bit_SERIALIZE)
SAME(CPUID_SERIALIZE, bit_SERIALIZE);
#endif
#if defined(bit_TSXLDTRK)
SAME(CPUID_TSXLDTRK, bit_TSXLDTRK);
#endif
#if defined(bit_PCONFIG)
SAME(CPUID_PCONFIG, bit_PCONFIG);
#endif
#if defined(bit_IBT)
SAME(CPUID_CET_IBT, bit_IBT);
#endif
#if defined(bit_AMX_BF16)
SAME(CPUID_AMX_BF16, bit_AMX_BF16);
#endif
#if defined(bit_AMXBF16)
SAME(CPUID_AMX_BF16, bit_AMXBF16);
#endif
#if defined(bit_AVX512FP16)
SAME(CPUID_AVX512_FP16, bit_AVX512FP16);
#endif
#if defined(bit_AMX_TILE)
SAME(CPUID_AMX_TILE, bit_AMX_TILE);
#endif
#if defined(bit_AMXTILE)
SAME(CPUID_AMX_TILE, bit_AMXTILE);
#endif
#if defined(bit_AMX_INT8)
SAME(CPUID_AMX_INT8, bit_AMX_INT8);
#endif
#if defined(bit_AMXINT8)
SAME(CPUID_AMX_INT8, bit_AMXINT8);
#endif


// Leaf 7.1, EAX, word 5
#if defined(bit_RAOINT)
SAME(CPUID_RAO_INT, bit_RAOINT);
#endif
#if defined(bit_AVXVNNI)
SAME(CPUID_AVX_VNNI, bit_AVXVNNI);
#endif
#if defined(bit_AVX512BF16)
SAME(CPUID_AVX512_BF16, bit_AVX512BF16);
#endif
#if defined(bit_CMPCCXADD)
SAME(CPUID_CMPCCXADD, bit_CMPCCXADD);
#endif
#if defined(bit_AMX_FP16)
SAME(CPUID_AMX_FP16, bit_AMX_FP16);
#endif
#if defined(bit_AMXFP16)
SAME(CPUID_AMX_FP16, bit_AMXFP16);
#endif
#if defined(bit_HRESET)
SAME(CPUID_HRESET, bit_HRESET);
#endif
#if defined(bit_AVXIFMA)
SAME(CPUID_AVX_IFMA, bit_AVXIFMA);
#endif


// Leaf 7.1, EDX, word 6
#if defined(bit_AVXVNNIINT8)
SAME(CPUID_AVX_VNNI_INT8, bit_AVXVNNIINT8);
#endif
#if defined(bit_AVXNECONVERT)
SAME(CPUID_AVX_NE_CONVERT, bit_AVXNECONVERT);
#endif
#if defined(bit_AMX_COMPLEX)
SAME(CPUID_AMX_COMPLEX, bit_AMX_COMPLEX);
#endif
#if defined(bit_PREFETCHI)
SAME(CPUID_PREFETCHI, bit_PREFETCHI);
#endif


// Leaf 0Dh.1, EAX, word 7
#if defined(bit_XSAVEOPT)
SAME(CPUID_XSAVEOPT, bit_XSAVEOPT);
#endif
#if defined(bit_XSAVEC)
SAME(CPUID_XSAVEC, bit_XSAVEC);
#endif
#if defined(bit_XSAVES)
SAME(CPUID_XSAVES, bit_XSAVES);
#endif


// Leaf 80000001h, ECX, word 8
#if defined(bit_LAHF_LM)
SAME(CPUID_LAHF_LM, bit_LAHF_LM);
#endif
#if defined(bit_LZCNT)
SAME(CPUID_LZCNT, bit_LZCNT);
#endif
#if defined(bit_ABM)
SAME(CPUID_LZCNT, bit_ABM);
#endif
#if defined(bit_SSE4a)
SAME(CPUID_SSE4A, bit_SSE4a);
#endif
#if defined(bit_PRFCHW)
SAME(CPUID_PREFETCHW, bit_PRFCHW);
#endif
#if defined(bit_XOP)
SAME(CPUID_XOP, bit_XOP);
#endif
#if defined(bit_LWP)
SAME(CPUID_LWP, bit_LWP);
#endif
#if defined(bit_FMA4)
SAME(CPUID_FMA4, bit_FMA4);
#endif
#if defined(bit_TBM)
SAME(CPUID_TBM, bit_TBM);
#endif
#if defined(bit_MWAITX)
SAME(CPUID_MONITORX, bit_MWAITX);
#endif


// Leaf 80000001h, EDX, word 9
#if defined(bit_MMXEXT)
SAME(CPUID_MMXEXT, bit_MMXEXT);
#endif
#if defined(bit_LM)
SAME(CPUID_LM, bit_LM);
#endif
#if defined(bit_3DNOWP)
SAME(CPUID_AMD_3DNOWEXT, bit_3DNOWP);
#endif
#if defined(bit_3DNOW)
SAME(CPUID_AMD_3DNOW, bit_3DNOW);
#endif
