# =============================================================================
# djinterp - build/cmake/djinterp/log_codes.cmake
#
#   The log's units for the code tables of the modules below it (the log's
# decision 87): a unit a module under src/djinterp/c/log/codes/, two lines
# each, added here in the window its table lands. The modules below the log
# are in the runtime, so a table is built wherever the log is: a leaf that
# compiles log.c includes this file and compiles DJINTERP_LOG_CODES_SOURCES
# beside it, and log.c registers the tables (log_codes.h). None has landed
# yet, so the list is empty.
#   It needs nothing of the enclosing project, so a leaf that stands alone
# includes it as well.
# =============================================================================

get_filename_component(_djinterp_log_codes_src
    "${CMAKE_CURRENT_LIST_DIR}/../../../src/djinterp/c/log/codes" ABSOLUTE)

# a unit a table, in the order log.c lists them, each
#     "${_djinterp_log_codes_src}/<module>_codes.c"
set(DJINTERP_LOG_CODES_SOURCES "")
