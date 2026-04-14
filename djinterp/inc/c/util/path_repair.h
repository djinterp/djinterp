/******************************************************************************
* djinterp [util]                                               path_repair.h
*
*   Scans files within a directory subtree, extracts candidate relative file
* paths according to configurable matching rules, validates them against the
* subtree, resolves broken paths when possible, and optionally rewrites the
* source files.
*   The repair pipeline is intentionally modular. Discovery, extraction,
* validation, resolution, and rewriting are each exposed as separate functions
* so they can be tested or replaced independently.
*   Uses d_file_tree for subtree indexing, which collapses the two recursive
* directory walks (index build + target file collection) into a single scan,
* and enables O(depth) path validation via tree traversal.
*
*
* path:      \inc\djinterp\c\util\path_repair.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.13
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    TYPE DEFINITIONS
      ----------------
      1.  d_path_repair_match_mode     (match strategy)
      2.  d_path_repair_log_mode       (verbosity)
      3.  d_path_repair_flags          (traversal flags)
      4.  d_path_repair_config         (user-supplied configuration)
      5.  d_path_repair_candidate      (one path occurrence)
      6.  d_path_repair_candidate_list (growable array of candidates)
      7.  d_path_repair_resolution     (result of validating one path)
      8.  d_path_repair_resolution_list(growable array of resolutions)
      9.  d_path_repair_index          (file tree + basename lookup table)
      10. d_path_repair_stats          (aggregate statistics)

II.   VALIDATION
      ----------
      a.  d_path_repair_config_is_valid

III.  SUBTREE INDEXING
      ----------------
      a.  d_path_repair_build_index
      b.  d_path_repair_index_free

IV.   DISCOVERY
      ---------
      a.  d_path_repair_collect_target_files
      b.  d_path_repair_free_file_list

V.    EXTRACTION
      ----------
      a.  d_path_repair_find_candidates
      b.  d_path_repair_candidate_list_free

VI.   VALIDATION AND RESOLUTION
      -------------------------
      a.  d_path_repair_resolve_candidates
      b.  d_path_repair_resolution_list_free

VII.  REWRITING
      --------
      a.  d_path_repair_rewrite_contents

VIII. FILE PROCESSING
      ---------------
      a.  d_path_repair_process_file

IX.   RUN
      ---
      a.  d_path_repair_run

X.    UTILITIES
      ---------
      a.  d_path_repair_stats_clear
*/

#ifndef DJINTERP_C_UTILITY_PATH_REPAIR_
#define DJINTERP_C_UTILITY_PATH_REPAIR_ 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "../container/tree/file/file_tree.h"


// d_path_repair_match_mode
//   enum: determines how candidate relative paths are discovered
// in text.
enum d_path_repair_match_mode
{
    D_PATH_REPAIR_MATCH_NONE   = 0x00,
    D_PATH_REPAIR_MATCH_PREFIX = 0x01,
    D_PATH_REPAIR_MATCH_SUFFIX = 0x02,
    D_PATH_REPAIR_MATCH_REGEX  = 0x04
};

// d_path_repair_log_mode
//   enum: controls diagnostic output verbosity.
enum d_path_repair_log_mode
{
    D_PATH_REPAIR_LOG_QUIET   = 0x00,
    D_PATH_REPAIR_LOG_NORMAL  = 0x01,
    D_PATH_REPAIR_LOG_VERBOSE = 0x02
};

// d_path_repair_flags
//   enum: bit flags controlling directory traversal and filtering.
enum d_path_repair_flags
{
    D_PATH_REPAIR_FLAG_NONE              = 0x00,
    D_PATH_REPAIR_FLAG_RECURSIVE         = 0x01,
    D_PATH_REPAIR_FLAG_INCLUDE_HIDDEN    = 0x02,
    D_PATH_REPAIR_FLAG_INCLUDE_NO_NAME   = 0x04,
    D_PATH_REPAIR_FLAG_NORMALIZE_SLASHES = 0x08
};

// d_path_repair_config
//   struct: user-supplied configuration for a repair run.
struct d_path_repair_config
{
    const char*                    root_path;
    const char*                    file_extension;
    const char*                    prefix;
    const char*                    suffix;
    const char*                    regex_pattern;
    uint32_t                       flags;
    enum d_path_repair_match_mode  match_mode;
    enum d_path_repair_log_mode    log_mode;
    bool                           dry_run;
};

// d_path_repair_candidate
//   struct: one candidate relative path occurrence found in source
// text.
struct d_path_repair_candidate
{
    size_t  start_offset;
    size_t  end_offset;
    size_t  line;
    size_t  column;
    char*   matched_text;
    char*   extracted_path;
};

// d_path_repair_candidate_list
//   struct: growable array of candidate path occurrences.
struct d_path_repair_candidate_list
{
    struct d_path_repair_candidate* items;
    size_t                          count;
    size_t                          capacity;
};

// d_path_repair_resolution
//   struct: result of validating or resolving one extracted
// relative path.
struct d_path_repair_resolution
{
    bool    is_valid;
    bool    was_fixed;
    bool    was_ambiguous;
    char*   original_path;
    char*   normalized_path;
    char*   repaired_path;
};

// d_path_repair_resolution_list
//   struct: growable array of path resolutions corresponding to
// candidates.
struct d_path_repair_resolution_list
{
    struct d_path_repair_resolution* items;
    size_t                           count;
    size_t                           capacity;
};

// d_path_repair_basename_entry
//   struct: one entry in the sorted basename lookup table. Points
// into the file tree for the name and stores an owned copy of the
// relative path.
struct d_path_repair_basename_entry
{
    char*  basename;
    char*  relative_path;
};

// d_path_repair_index
//   struct: indexed view of the subtree, combining a d_file_tree
// for O(depth) path validation with a sorted basename table for
// O(log n) repair lookups.
struct d_path_repair_index
{
    struct d_file_tree*                 tree;
    struct d_path_repair_basename_entry* basenames;
    size_t                              basename_count;
    size_t                              basename_capacity;
};

// d_path_repair_stats
//   struct: aggregate statistics for a repair run.
struct d_path_repair_stats
{
    size_t files_seen;
    size_t files_changed;
    size_t candidates_seen;
    size_t valid_paths;
    size_t fixed_paths;
    size_t ambiguous_paths;
    size_t unresolved_paths;
    size_t duplicate_basenames;
};


// II.    validation
bool d_path_repair_config_is_valid(
         const struct d_path_repair_config* _config);

// III.   subtree indexing
bool d_path_repair_build_index(
         const struct d_path_repair_config* _config,
         struct d_path_repair_index*        _index);
void d_path_repair_index_free(
         struct d_path_repair_index* _index);

// IV.    discovery
bool d_path_repair_collect_target_files(
         const struct d_path_repair_config* _config,
         const struct d_path_repair_index*  _index,
         char***                            _files,
         size_t*                            _file_count);
void d_path_repair_free_file_list(
         char** _files,
         size_t _file_count);

// V.     extraction
bool d_path_repair_find_candidates(
         const struct d_path_repair_config*  _config,
         const char*                         _file_contents,
         struct d_path_repair_candidate_list* _candidates);
void d_path_repair_candidate_list_free(
         struct d_path_repair_candidate_list* _candidates);

// VI.    validation and resolution
bool d_path_repair_resolve_candidates(
         const struct d_path_repair_config*         _config,
         const struct d_path_repair_index*           _index,
         const char*                                 _current_file_path,
         const struct d_path_repair_candidate_list*  _candidates,
         struct d_path_repair_resolution_list*        _resolutions);
void d_path_repair_resolution_list_free(
         struct d_path_repair_resolution_list* _resolutions);

// VII.   rewriting
bool d_path_repair_rewrite_contents(
         const char*                                 _file_contents,
         const struct d_path_repair_candidate_list*  _candidates,
         const struct d_path_repair_resolution_list* _resolutions,
         char**                                      _rewritten_contents,
         bool*                                       _was_changed);

// VIII.  file processing
bool d_path_repair_process_file(
         const struct d_path_repair_config* _config,
         const struct d_path_repair_index*  _index,
         const char*                        _file_path,
         struct d_path_repair_stats*        _stats);

// IX.    run
bool d_path_repair_run(
         const struct d_path_repair_config* _config,
         struct d_path_repair_stats*        _stats);

// X.     utilities
void d_path_repair_stats_clear(
         struct d_path_repair_stats* _stats);


#endif  // DJINTERP_C_UTILITY_PATH_REPAIR_
