#!/usr/bin/env python3
#*******************************************************************************
# bitwisedom [tools]                                       x86_cpuid_bitfield.py
#
# The x86_cpuid example: ten of CPUID's feature registers, end to end, as a
# bitfield of 320 bits (docs/bitwisedom/json.md) -- word n of the header's array is the
# nth register below, as CPUID returns it -- with x86-64's microarchitecture
# levels as sets of them; and the platform check, which compares each bit
# with the compiler's own <cpuid.h>. The grid draws bit 0 at the left, so the
# registers read left to right in the order of the rows.
#   The bits are the Intel SDM's (volume 2A, CPUID) and AMD's APM's
# (volume 3, appendix E), named as the SDM names them, but where C wants
# otherwise; a bit only AMD defines says so. The levels are the x86-64
# psABI's.
#   A <cpuid.h> names a bit's mask, not its register; a mask is compared
# under every name GCC's and Clang's give the bit, and a name the compiler's
# header does not define is passed over.
#
#     python3 tools/x86_cpuid_bitfield.py \
#         res/djinterp/tools/bitwisedom/examples/large/x86_cpuid.bitfield.json tests/djinterp/tools/bitwisedom/platform/x86_cpuid.c
#
# path:      /src/djinterp/tools/bitwisedom/tools/x86_cpuid_bitfield.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
#                                                            revised: 2026.10.11
#*******************************************************************************
import sys

import bitfield_json

# the registers, word by word: the segment's name, the group's, its note, and
# its bits -- each its number, its name, its note, and the names <cpuid.h>
# gives it
REGISTERS = [
    ("1 ECX", "Leaf 1, ECX",
     "CPUID.01H:ECX: feature information.", [
        (0, "SSE3", "SSE3", ["bit_SSE3"]),
        (1, "PCLMULQDQ", "carry-less multiplication",
         ["bit_PCLMUL", "bit_PCLMULQDQ"]),
        (2, "DTES64", "the 64-bit debug store", ["bit_DTES64"]),
        (3, "MONITOR", "MONITOR and MWAIT", ["bit_MONITOR"]),
        (4, "DS_CPL", "the CPL-qualified debug store", ["bit_DSCPL"]),
        (5, "VMX", "virtual machine extensions", ["bit_VMX"]),
        (6, "SMX", "safer mode extensions", ["bit_SMX"]),
        (7, "EIST", "Enhanced Intel SpeedStep", ["bit_EIST"]),
        (8, "TM2", "thermal monitor 2", ["bit_TM2"]),
        (9, "SSSE3", "supplemental SSE3", ["bit_SSSE3"]),
        (10, "CNXT_ID", "the L1 data cache's context ID", ["bit_CNXTID"]),
        (11, "SDBG", "the silicon debug MSR", []),
        (12, "FMA", "fused multiply-add of three operands", ["bit_FMA"]),
        (13, "CMPXCHG16B", "CMPXCHG16B", ["bit_CMPXCHG16B"]),
        (14, "XTPR", "xTPR update control", ["bit_xTPR"]),
        (15, "PDCM", "the perfmon and debug capability MSR", ["bit_PDCM"]),
        (17, "PCID", "process-context identifiers", ["bit_PCID"]),
        (18, "DCA", "direct cache access", ["bit_DCA"]),
        (19, "SSE4_1", "SSE4.1", ["bit_SSE4_1", "bit_SSE41"]),
        (20, "SSE4_2", "SSE4.2", ["bit_SSE4_2", "bit_SSE42"]),
        (21, "X2APIC", "x2APIC", ["bit_x2APIC"]),
        (22, "MOVBE", "MOVBE", ["bit_MOVBE"]),
        (23, "POPCNT", "POPCNT", ["bit_POPCNT"]),
        (24, "TSC_DEADLINE", "the local APIC's TSC-deadline timer",
         ["bit_TSCDeadline"]),
        (25, "AES", "the AES instructions", ["bit_AES", "bit_AESNI"]),
        (26, "XSAVE", "XSAVE, XRSTOR, XSETBV and XGETBV", ["bit_XSAVE"]),
        (27, "OSXSAVE", "the OS has set CR4.OSXSAVE", ["bit_OSXSAVE"]),
        (28, "AVX", "AVX", ["bit_AVX"]),
        (29, "F16C", "half-precision conversions", ["bit_F16C"]),
        (30, "RDRAND", "RDRAND", ["bit_RDRND"]),
        (31, "HYPERVISOR", "0 on the bare processor; a hypervisor sets it",
         []),
    ]),
    ("1 EDX", "Leaf 1, EDX",
     "CPUID.01H:EDX: feature information.", [
        (0, "FPU", "an x87 FPU on the chip", ["bit_FPU"]),
        (1, "VME", "virtual-8086 mode enhancements", ["bit_VME"]),
        (2, "DE", "debugging extensions", ["bit_DE"]),
        (3, "PSE", "page size extension", ["bit_PSE"]),
        (4, "TSC", "the time stamp counter", ["bit_TSC"]),
        (5, "MSR", "RDMSR and WRMSR", ["bit_MSR"]),
        (6, "PAE", "physical address extension", ["bit_PAE"]),
        (7, "MCE", "the machine check exception", ["bit_MCE"]),
        (8, "CX8", "CMPXCHG8B", ["bit_CMPXCHG8B", "bit_CX8"]),
        (9, "APIC", "an APIC on the chip", ["bit_APIC"]),
        (11, "SEP", "SYSENTER and SYSEXIT", ["bit_SEP"]),
        (12, "MTRR", "memory type range registers", ["bit_MTRR"]),
        (13, "PGE", "the page global bit", ["bit_PGE"]),
        (14, "MCA", "the machine check architecture", ["bit_MCA"]),
        (15, "CMOV", "conditional moves", ["bit_CMOV"]),
        (16, "PAT", "the page attribute table", ["bit_PAT"]),
        (17, "PSE_36", "36-bit page size extension", ["bit_PSE36"]),
        (18, "PSN", "the processor serial number", ["bit_PSN"]),
        (19, "CLFSH", "CLFLUSH", ["bit_CLFSH"]),
        (21, "DS", "the debug store", ["bit_DS"]),
        (22, "ACPI", "thermal monitor and software-controlled clock MSRs",
         ["bit_ACPI"]),
        (23, "MMX", "MMX", ["bit_MMX"]),
        (24, "FXSR", "FXSAVE and FXRSTOR", ["bit_FXSAVE", "bit_FXSR"]),
        (25, "SSE", "SSE", ["bit_SSE"]),
        (26, "SSE2", "SSE2", ["bit_SSE2"]),
        (27, "SS", "self snoop", ["bit_SS"]),
        (28, "HTT", "more than one logical processor in the package",
         ["bit_HTT"]),
        (29, "TM", "the thermal monitor", ["bit_TM"]),
        (31, "PBE", "pending break enable", ["bit_PBE"]),
    ]),
    ("7 EBX", "Leaf 7, EBX",
     "CPUID.(EAX=07H,ECX=0):EBX: structured extended features.", [
        (0, "FSGSBASE", "RDFSBASE, RDGSBASE, WRFSBASE and WRGSBASE",
         ["bit_FSGSBASE"]),
        (1, "TSC_ADJUST", "the IA32_TSC_ADJUST MSR", []),
        (2, "SGX", "Software Guard Extensions", ["bit_SGX"]),
        (3, "BMI1", "bit manipulation, the first group", ["bit_BMI"]),
        (4, "HLE", "hardware lock elision", ["bit_HLE"]),
        (5, "AVX2", "AVX2", ["bit_AVX2"]),
        (6, "FDP_EXCPTN_ONLY", "the x87 FDP is updated only on exceptions",
         []),
        (7, "SMEP", "supervisor-mode execution prevention", ["bit_SMEP"]),
        (8, "BMI2", "bit manipulation, the second group", ["bit_BMI2"]),
        (9, "ERMS", "enhanced REP MOVSB and REP STOSB", ["bit_ENH_MOVSB"]),
        (10, "INVPCID", "INVPCID", ["bit_INVPCID"]),
        (11, "RTM", "restricted transactional memory", ["bit_RTM"]),
        (12, "RDT_M", "resource director technology: monitoring", []),
        (13, "FPCSDS", "the x87 FPU CS and DS are deprecated", []),
        (14, "MPX", "memory protection extensions", ["bit_MPX"]),
        (15, "RDT_A", "resource director technology: allocation", []),
        (16, "AVX512F", "AVX-512: the foundation", ["bit_AVX512F"]),
        (17, "AVX512DQ", "AVX-512: doublewords and quadwords",
         ["bit_AVX512DQ"]),
        (18, "RDSEED", "RDSEED", ["bit_RDSEED"]),
        (19, "ADX", "ADCX and ADOX", ["bit_ADX"]),
        (20, "SMAP", "supervisor-mode access prevention", []),
        (21, "AVX512_IFMA", "AVX-512: integer fused multiply-add",
         ["bit_AVX512IFMA"]),
        (23, "CLFLUSHOPT", "CLFLUSHOPT", ["bit_CLFLUSHOPT"]),
        (24, "CLWB", "CLWB", ["bit_CLWB"]),
        (25, "INTEL_PT", "processor trace", []),
        (26, "AVX512PF", "AVX-512: prefetches", ["bit_AVX512PF"]),
        (27, "AVX512ER", "AVX-512: exponentials and reciprocals",
         ["bit_AVX512ER"]),
        (28, "AVX512CD", "AVX-512: conflict detection", ["bit_AVX512CD"]),
        (29, "SHA", "SHA-1 and SHA-256", ["bit_SHA"]),
        (30, "AVX512BW", "AVX-512: bytes and words", ["bit_AVX512BW"]),
        (31, "AVX512VL", "AVX-512 at 128 and 256 bits", ["bit_AVX512VL"]),
    ]),
    ("7 ECX", "Leaf 7, ECX",
     "CPUID.(EAX=07H,ECX=0):ECX: structured extended features.", [
        (0, "PREFETCHWT1", "PREFETCHWT1",
         ["bit_PREFETCHWT1", "bit_PREFTCHWT1"]),
        (1, "AVX512_VBMI", "AVX-512: vector bit manipulation",
         ["bit_AVX512VBMI"]),
        (2, "UMIP", "user-mode instruction prevention", []),
        # Clang's <cpuid.h> has had bit_PKU as bit 2, UMIP's
        (3, "PKU", "protection keys for user-mode pages", ["bit_PKU"]),
        (4, "OSPKE", "the OS has set CR4.PKE", ["bit_OSPKE"]),
        (5, "WAITPKG", "TPAUSE, UMONITOR and UMWAIT", ["bit_WAITPKG"]),
        (6, "AVX512_VBMI2", "AVX-512: vector bit manipulation 2",
         ["bit_AVX512VBMI2"]),
        (7, "CET_SS", "control-flow enforcement: shadow stacks",
         ["bit_SHSTK"]),
        (8, "GFNI", "Galois field instructions", ["bit_GFNI"]),
        (9, "VAES", "vector AES", ["bit_VAES"]),
        (10, "VPCLMULQDQ", "vector carry-less multiplication",
         ["bit_VPCLMULQDQ"]),
        (11, "AVX512_VNNI", "AVX-512: vector neural network instructions",
         ["bit_AVX512VNNI"]),
        (12, "AVX512_BITALG", "AVX-512: bit algorithms",
         ["bit_AVX512BITALG"]),
        (13, "TME_EN", "total memory encryption", []),
        (14, "AVX512_VPOPCNTDQ", "AVX-512: VPOPCNTD and VPOPCNTQ",
         ["bit_AVX512VPOPCNTDQ"]),
        (16, "LA57", "57-bit linear addresses: five-level paging", []),
        ((17, 21), "MAWAU",
         "the MPX address-width adjust: the value BNDLDX and BNDSTX use in "
         "64-bit mode", []),
        (22, "RDPID", "RDPID", ["bit_RDPID"]),
        (23, "KL", "Key Locker", ["bit_KL"]),
        (24, "BUS_LOCK_DETECT", "OS bus-lock detection", []),
        (25, "CLDEMOTE", "cache line demote", ["bit_CLDEMOTE"]),
        (27, "MOVDIRI", "MOVDIRI", ["bit_MOVDIRI"]),
        (28, "MOVDIR64B", "MOVDIR64B", ["bit_MOVDIR64B"]),
        (29, "ENQCMD", "ENQCMD and ENQCMDS", ["bit_ENQCMD"]),
        (30, "SGX_LC", "SGX launch configuration", []),
        (31, "PKS", "protection keys for supervisor-mode pages", []),
    ]),
    ("7 EDX", "Leaf 7, EDX",
     "CPUID.(EAX=07H,ECX=0):EDX: structured extended features.", [
        (1, "SGX_KEYS", "SGX attestation services", []),
        (2, "AVX512_4VNNIW", "AVX-512: four-iteration VNNI on words",
         ["bit_AVX5124VNNIW"]),
        (3, "AVX512_4FMAPS",
         "AVX-512: four-iteration fused multiply-add, single precision",
         ["bit_AVX5124FMAPS"]),
        (4, "FSRM", "fast short REP MOV", []),
        (5, "UINTR", "user interrupts", ["bit_UINTR"]),
        (8, "AVX512_VP2INTERSECT", "AVX-512: VP2INTERSECTD and VP2INTERSECTQ",
         ["bit_AVX512VP2INTERSECT"]),
        (9, "SRBDS_CTRL", "the SRBDS mitigation MSR", []),
        (10, "MD_CLEAR", "VERW clears the CPU's buffers", []),
        (11, "RTM_ALWAYS_ABORT", "every RTM transaction aborts", []),
        (13, "RTM_FORCE_ABORT", "the TSX_FORCE_ABORT MSR", []),
        (14, "SERIALIZE", "SERIALIZE", ["bit_SERIALIZE"]),
        (15, "HYBRID", "a hybrid part, of more than one type of core", []),
        (16, "TSXLDTRK", "TSX: suspending load address tracking",
         ["bit_TSXLDTRK"]),
        (18, "PCONFIG", "PCONFIG", ["bit_PCONFIG"]),
        (19, "ARCH_LBR", "architectural last branch records", []),
        (20, "CET_IBT", "control-flow enforcement: indirect branch tracking",
         ["bit_IBT"]),
        (22, "AMX_BF16", "AMX: bfloat16", ["bit_AMX_BF16", "bit_AMXBF16"]),
        (23, "AVX512_FP16", "AVX-512: half precision", ["bit_AVX512FP16"]),
        (24, "AMX_TILE", "AMX: the tiles", ["bit_AMX_TILE", "bit_AMXTILE"]),
        (25, "AMX_INT8", "AMX: 8-bit integers",
         ["bit_AMX_INT8", "bit_AMXINT8"]),
        (26, "IBRS_IBPB",
         "indirect branch restricted speculation and the predictor barrier",
         []),
        (27, "STIBP", "single thread indirect branch predictors", []),
        (28, "L1D_FLUSH", "the IA32_FLUSH_CMD MSR", []),
        (29, "ARCH_CAPABILITIES", "the IA32_ARCH_CAPABILITIES MSR", []),
        (30, "CORE_CAPABILITIES", "the IA32_CORE_CAPABILITIES MSR", []),
        (31, "SSBD", "speculative store bypass disable", []),
    ]),
    ("7.1 EAX", "Leaf 7.1, EAX",
     "CPUID.(EAX=07H,ECX=1):EAX: structured extended features.", [
        (0, "SHA512", "SHA512", []),
        (1, "SM3", "SM3", []),
        (2, "SM4", "SM4", []),
        (3, "RAO_INT", "remote atomic operations on integers",
         ["bit_RAOINT"]),
        (4, "AVX_VNNI", "AVX: vector neural network instructions, in VEX",
         ["bit_AVXVNNI"]),
        (5, "AVX512_BF16", "AVX-512: bfloat16", ["bit_AVX512BF16"]),
        (6, "LASS", "linear address space separation", []),
        (7, "CMPCCXADD", "CMPccXADD", ["bit_CMPCCXADD"]),
        (8, "ARCH_PERFMON_EXT", "the architectural perfmon extended leaf",
         []),
        (10, "FZLRM", "fast zero-length REP MOVSB", []),
        (11, "FSRS", "fast short REP STOSB", []),
        (12, "FSRCS", "fast short REP CMPSB and REP SCASB", []),
        (17, "FRED", "flexible return and event delivery", []),
        (18, "LKGS", "LKGS", []),
        (19, "WRMSRNS", "non-serializing WRMSR", []),
        (21, "AMX_FP16", "AMX: half precision",
         ["bit_AMX_FP16", "bit_AMXFP16"]),
        (22, "HRESET", "history reset", ["bit_HRESET"]),
        (23, "AVX_IFMA", "AVX: integer fused multiply-add, in VEX",
         ["bit_AVXIFMA"]),
        (26, "LAM", "linear address masking", []),
        (27, "MSRLIST", "RDMSRLIST and WRMSRLIST", []),
        (31, "MOVRS", "MOVRS and PREFETCHRST2", []),
    ]),
    ("7.1 EDX", "Leaf 7.1, EDX",
     "CPUID.(EAX=07H,ECX=1):EDX: structured extended features.", [
        (4, "AVX_VNNI_INT8", "AVX: VNNI on 8-bit integers",
         ["bit_AVXVNNIINT8"]),
        (5, "AVX_NE_CONVERT", "AVX: conversions that raise no exceptions",
         ["bit_AVXNECONVERT"]),
        (8, "AMX_COMPLEX", "AMX: complex numbers", ["bit_AMX_COMPLEX"]),
        (10, "AVX_VNNI_INT16", "AVX: VNNI on 16-bit integers", []),
        (14, "PREFETCHI", "PREFETCHIT0 and PREFETCHIT1", ["bit_PREFETCHI"]),
        (15, "USER_MSR", "URDMSR and UWRMSR", []),
        (18, "CET_SSS", "control-flow enforcement: supervisor shadow stacks",
         []),
        (19, "AVX10", "AVX10; leaf 24h says which version", []),
        (21, "APX_F", "advanced performance extensions: the foundation", []),
    ]),
    ("0Dh.1 EAX", "Leaf 0Dh.1, EAX",
     "CPUID.(EAX=0DH,ECX=1):EAX: processor extended state.", [
        (0, "XSAVEOPT", "XSAVEOPT", ["bit_XSAVEOPT"]),
        (1, "XSAVEC", "XSAVEC and the compacted XRSTOR", ["bit_XSAVEC"]),
        (2, "XGETBV_ECX1", "XGETBV with ECX = 1", []),
        (3, "XSAVES", "XSAVES, XRSTORS and the IA32_XSS MSR", ["bit_XSAVES"]),
        (4, "XFD", "extended feature disable", []),
    ]),
    ("80000001h ECX", "Leaf 80000001h, ECX",
     "CPUID.80000001H:ECX: extended features, many of them AMD's alone.", [
        (0, "LAHF_LM", "LAHF and SAHF in 64-bit mode", ["bit_LAHF_LM"]),
        (1, "CMP_LEGACY", "core multi-processing legacy mode (AMD)", []),
        (2, "SVM", "secure virtual machine (AMD)", []),
        (3, "EXTAPIC", "extended APIC space (AMD)", []),
        (4, "CR8_LEGACY", "LOCK MOV CR0 means MOV CR8 (AMD)", []),
        (5, "LZCNT", "LZCNT; AMD's ABM, with POPCNT",
         ["bit_LZCNT", "bit_ABM"]),
        (6, "SSE4A", "SSE4A (AMD)", ["bit_SSE4a"]),
        (7, "MISALIGNSSE", "misaligned SSE mode (AMD)", []),
        (8, "PREFETCHW", "PREFETCHW", ["bit_PRFCHW"]),
        (9, "OSVW", "OS-visible workarounds (AMD)", []),
        (10, "IBS", "instruction-based sampling (AMD)", []),
        (11, "XOP", "extended operations (AMD)", ["bit_XOP"]),
        (12, "SKINIT", "SKINIT and STGI (AMD)", []),
        (13, "WDT", "the watchdog timer (AMD)", []),
        (15, "LWP", "lightweight profiling (AMD)", ["bit_LWP"]),
        (16, "FMA4", "fused multiply-add of four operands (AMD)",
         ["bit_FMA4"]),
        (17, "TCE", "the translation cache extension (AMD)", []),
        (19, "NODEID_MSR", "the node ID MSR (AMD)", []),
        (21, "TBM", "trailing bit manipulation (AMD)", ["bit_TBM"]),
        (22, "TOPOEXT", "topology extensions (AMD)", []),
        (23, "PERFCTR_CORE", "core performance counter extensions (AMD)", []),
        (24, "PERFCTR_NB", "northbridge performance counter extensions (AMD)",
         []),
        (26, "DATA_BP_EXT", "data breakpoint extensions (AMD)", []),
        (27, "PERF_TSC", "the performance time-stamp counter (AMD)", []),
        (28, "PERFCTR_LLC",
         "last-level cache performance counter extensions (AMD)", []),
        (29, "MONITORX", "MONITORX and MWAITX (AMD)", ["bit_MWAITX"]),
    ]),
    ("80000001h EDX", "Leaf 80000001h, EDX",
     "CPUID.80000001H:EDX: extended features. AMD's repeat leaf 1's EDX in "
     "the bits not named here.", [
        (11, "SYSCALL", "SYSCALL and SYSRET", []),
        (20, "NX", "execute disable", []),
        (22, "MMXEXT", "AMD's extensions to MMX (AMD)", ["bit_MMXEXT"]),
        (25, "FFXSR", "FXSAVE and FXRSTOR optimizations (AMD)", []),
        (26, "PAGE1GB", "1-GByte pages", []),
        (27, "RDTSCP", "RDTSCP and the IA32_TSC_AUX MSR", []),
        (29, "LM", "64-bit mode: Intel 64 and AMD64", ["bit_LM"]),
        (30, "AMD_3DNOWEXT", "AMD's extensions to 3DNow! (AMD)",
         ["bit_3DNOWP"]),
        (31, "AMD_3DNOW", "3DNow! (AMD)", ["bit_3DNOW"]),
    ]),
]

# the x86-64 psABI's microarchitecture levels, each the one before it and
# the bits it adds
LEVELS = [
    ("X86_64_V1",
     "x86-64's baseline; the psABI also asks for CR4.OSFXSR, which CPUID "
     "does not say",
     ["CMOV", "CX8", "FPU", "FXSR", "MMX", "SYSCALL", "SSE", "SSE2"]),
    ("X86_64_V2", "x86-64-v2: the generation of SSE4.2 and POPCNT",
     ["CMPXCHG16B", "LAHF_LM", "POPCNT", "SSE3", "SSE4_1", "SSE4_2",
      "SSSE3"]),
    ("X86_64_V3", "x86-64-v3: the generation of AVX2 and FMA",
     ["AVX", "AVX2", "BMI1", "BMI2", "F16C", "FMA", "LZCNT", "MOVBE",
      "OSXSAVE"]),
    ("X86_64_V4", "x86-64-v4: AVX-512's common core",
     ["AVX512F", "AVX512BW", "AVX512CD", "AVX512DQ", "AVX512VL"]),
]

# the probe: a processor of x86-64-v3, its every bit and no more
PROBE = "X86_64_V3"


def main(bitfield, check):
    places = {}
    rows = []
    segments = []

    for word, (segment, group, note, bits) in enumerate(REGISTERS):
        lowest = word * 32
        segments.append({"name": segment,
                         "bits": "%d..%d" % (lowest, lowest + 31)})
        listed = []
        for number, name, brief, _ in bits:
            if isinstance(number, tuple):
                listed.append({"name": name,
                               "bits": "%d..%d" % (lowest + number[0],
                                                   lowest + number[1]),
                               "note": brief})
                continue
            places[name] = lowest + number
            listed.append({"name": name, "bit": lowest + number,
                           "note": brief})
        rows.append({"group": group, "note": note, "rows": listed})

    # the levels, and the probe a set of bits
    sets = {}
    levels = []
    before = None
    for name, brief, adds in LEVELS:
        terms = ([before] if before else []) + adds
        sets[name] = (sets[before] if before else set()) | set(adds)
        levels.append({"name": name, "expr": " | ".join(terms),
                       "note": brief})
        before = name
    rows.append({
        "group": "Levels",
        "note": "x86-64's microarchitecture levels, as the psABI sets them "
                "out: each the bits a processor of that level has, and so "
                "the bits of the level below it.",
        "rows": levels,
    })

    probe = 0
    for name in sets[PROBE]:
        probe |= 1 << places[name]

    sheet = {
        "format": "bitwisedom.bitfield",
        "version": 1,
        "title": "x86_cpuid",
        "width": 32 * len(REGISTERS),
        "prefix": "CPUID_",
        "msb_left": False,
        "probe": "0x%X" % probe,
        "rows": rows,
        "segments": segments,
    }

    with open(bitfield, "w", newline="\n") as out:
        out.write(bitfield_json.dumps(sheet))

    count = write_check(check)
    print("%d registers, %d bits, %d levels; %d comparisons with <cpuid.h>"
          % (len(REGISTERS), len(places), len(LEVELS), count))


def spread(left, right, width=80):
    """left and right on one line of width columns, right flush with its
    end."""
    return left + " " * (width - len(left) - len(right)) + right


def continued(text):
    """A line of a macro's definition, its backslash in column 80."""
    return text + " " * (79 - len(text)) + "\\"


def write_check(path):
    lines = [
        "/" + "*" * 79,
        spread("* bitwisedom [tests]", "x86_cpuid.c"),
        "*",
        "* The x86_cpuid example against the compiler's <cpuid.h>: every bit",
        "* it names, its mask in its register compared with the compiler's at",
        "* compile time, under each name GCC's and Clang's headers give it.",
        "* Built where the build targets x86 with GCC or Clang. Written by",
        "* tools/x86_cpuid_bitfield.py with the example; a name the",
        "* compiler's header does not define is passed over.",
        "*   A <cpuid.h> names a bit's mask, not the register it is in, so the",
        "* register each bit is in is the example's word; the example's notes",
        "* say which register each word is.",
        "*",
        "* path:      /tests/djinterp/tools/bitwisedom/platform/x86_cpuid.c",
        "* link(s):   TBA",
        spread("* author(s): Samuel 'teer' Neal-Blim", "created: 2026.10.11"),
        spread("*", "revised: 2026.10.11"),
        "*" * 79 + "/",
        "",
        "#include <cpuid.h>               // bit_*",
        "#include \"x86_cpuid.h\"           // the example, as bitwisedom-gen "
        "wrote it",
        "",
        "",
        "// SAME",
        "//   our bit's mask in its word is the compiler's mask for it.",
        continued("#define SAME(_ours, _theirs)"),
        continued("    _Static_assert(CPUID_MASK(_ours) == "
                  "(unsigned int)(_theirs),"),
        "                   #_ours \" is \" #_theirs)",
    ]
    count = 0
    for word, (segment, group, note, bits) in enumerate(REGISTERS):
        lines += ["", "", "// %s, word %d" % (group, word)]
        for number, name, brief, theirs in bits:
            for each in theirs:
                if each == "bit_PKU":
                    # Clang's has had it as bit 2, UMIP's
                    lines += ["#if ( (defined(%s)) && (!defined(__clang__)) )"
                              % each]
                else:
                    lines += ["#if defined(%s)" % each]
                lines += ["SAME(CPUID_%s, %s);" % (name, each), "#endif"]
                count += 1
    with open(path, "w", newline="\n") as out:
        out.write("\n".join(lines) + "\n")
    return count


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("usage: x86_cpuid_bitfield.py BITFIELD.json CHECK.c")
    main(*sys.argv[1:])
