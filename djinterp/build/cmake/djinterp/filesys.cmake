# =============================================================================
# djinterp - build/cmake/djinterp/filesys.cmake
#
#   djinterp_c_filesys: c/filesys, the framework's files, descriptors,
# directories, links, locks and paths, on the platform's own calls (ruled
# 2026.11.22, as the module's library of ruling 9). It links the runtime. A
# leaf or a program that touches a file links it and lists none of its units,
# so a call one unit gains into another links everywhere: three log leaves
# failed to link when ostream_file.c first called file_desc and file_io.
#   Its own suites compile its units themselves, at each level of C they
# test, and so link nothing of it.
# =============================================================================

djinterp_add_library(djinterp_c_filesys
    SOURCES
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_desc.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_dir.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_io.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_link.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_lock.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_open.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_ops.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_path.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_pipe.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_seek.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_space.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_stat.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_sync.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/filesys/file_temp.c"
    LINKS
        djinterp_c_runtime)
