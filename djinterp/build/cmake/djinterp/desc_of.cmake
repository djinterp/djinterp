# =============================================================================
# djinterp - build/cmake/djinterp/desc_of.cmake
#
#   djinterp_c_desc_of: desc's describers of other modules' types (desc's
# decision 82), above the modules they describe and above desc's core: a
# unit a module, each built where its knob keeps it (cfg_desc_of.h):
# memory's, filesys's and DTest's. A describer reads its types' fields and
# calls none of their module's functions, so the library needs the
# modules' headers and not their units.
#   desc's own units (desc_units.c and its kin) are not in a library yet:
# a program that links this one compiles them itself, as every leaf that
# describes already does, and its d_desc_facts calls resolve there.
# =============================================================================

djinterp_add_library(djinterp_c_desc_of
    SOURCES
        "${DJINTERP_SRC_ROOT}/djinterp/c/desc/of/mem_desc.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/desc/of/filesys_desc.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/desc/of/test_desc.c"
    LINKS
        djinterp_c_runtime)
