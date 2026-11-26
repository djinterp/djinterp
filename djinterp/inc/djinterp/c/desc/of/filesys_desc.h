/*******************************************************************************
* djinterp [c][desc]                                              filesys_desc.h
*
*   desc's describers of filesys's types (desc's decisions 34, 68 and 82): a
* file's status (struct dj_stat64_t and dj_stat32_t), a directory entry
* (dj_dirent64_t and dj_dirent32_t) and a filesys notice (dj_file_notice),
* each a struct dj_desc_type, so d_desc_say writes it in prose and
* d_desc_facts as facts. They are desc's, in djinterp_c_desc_of: filesys
* includes nothing of desc, and a program that describes none of these
* links none of this.
*   Each describer reads the record's fields and calls none of filesys's
* functions, so the library needs filesys's headers and not its units.
* Each width has its own description; d_desc_type_dj_stat_t and
* d_desc_type_dj_dirent_t name the width filesys builds by default, as
* dj_stat_t and dj_dirent_t do, so D_DESC_OF(dj_stat_t) is that width's.
*   A status's kind and permissions come from its mode; its change and
* creation times are none where filesys reports 0 for want of them. An
* entry's kind is its d_type's. A notice's strings are filesys's literals.
* None of these types holds a lock or a field another thread writes. The
* fields each describer reads are recorded beside it (section 2), and the
* layout rows check the records against the types and the library's build
* against the caller's (desc_of.h).
*
* HOW TO USE
* ----------
*   d_desc_say(D_DESC_OF(dj_stat_t), &status, ring, layers, 0, put, context)
* writes a file's status in prose: "a regular file with 1234 bytes";
* d_desc_facts(D_DESC_OF(dj_file_notice), &notice, sink) gives a notice's
* facts, as a log record's might.
*
* OPTIONS
* -------
*   desc's own: desc.ring, desc.layers and the rest; desc.stat64.ring,
* desc.dirent64.ring and desc.file_notice.ring, once d_desc_declare has
* declared the type.
*
* CONFIGURATION
* -------------
*   D_CFG_DESC_OF and D_CFG_DESC_OF_FILESYS (cfg_desc_of.h); desc's knobs;
* and filesys's D_FILE_WIDTH and D_CFG_FILE_NAME_MAX, which the records
* follow.
*
*
* path:      /inc/djinterp/c/desc/of/filesys_desc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DESCRIPTIONS
    ------------
    1.  d_desc_type_dj_stat64_t, d_desc_type_dj_stat32_t
    2.  d_desc_type_dj_dirent64_t, d_desc_type_dj_dirent32_t
    3.  d_desc_type_dj_stat_t, d_desc_type_dj_dirent_t
    4.  d_desc_type_dj_file_notice
2.  RECORDS
    -------
    1.  dj_fs_desc_*_record
3.  LAYOUT
    ------
    1.  d_desc_of_filesys_layout_here
    2.  d_desc_of_filesys_layout
    3.  d_desc_of_filesys_agrees
*/

#ifndef DJINTERP_C_DESC_OF_FILESYS_DESC_H
#define DJINTERP_C_DESC_OF_FILESYS_DESC_H 1

// djinterp
#include "./desc_of.h"                     // the layout row, the knobs
#include "../../filesys/file_common.h"     // the status, entry and notice


D_EXTERN_C_BEGIN


#if D_INTERNAL_DESC_OF_FILESYS

//==============================================================================
// 1.  DESCRIPTIONS
//==============================================================================

// 1.1
// d_desc_type_dj_stat64_t, d_desc_type_dj_stat32_t
//   constants: a file's status: "a regular file with 1234 bytes", "an
// empty regular file", "a directory". Its facts: kind, size, mode, owner,
// group, other, links, uid, gid, modified, accessed, changed, created,
// modified_nsec, accessed_nsec, changed_nsec, device and inode. owner,
// group and other are permission triplets, "rwx" to "---".
#if D_FILE_64_IS_AVAILABLE
extern const struct dj_desc_type d_desc_type_dj_stat64_t;
#endif
extern const struct dj_desc_type d_desc_type_dj_stat32_t;

// 1.2
// d_desc_type_dj_dirent64_t, d_desc_type_dj_dirent32_t
//   constants: a directory entry: "a directory entry named src". Its facts:
// name, kind and inode.
#if D_FILE_64_IS_AVAILABLE
extern const struct dj_desc_type d_desc_type_dj_dirent64_t;
#endif
extern const struct dj_desc_type d_desc_type_dj_dirent32_t;

// 1.3
// d_desc_type_dj_stat_t, d_desc_type_dj_dirent_t
//   aliases: the descriptions of the width filesys builds by default, as
// dj_stat_t and dj_dirent_t name its records.
#if (D_FILE_WIDTH == 64)
    #define d_desc_type_dj_stat_t    d_desc_type_dj_stat64_t
    #define d_desc_type_dj_dirent_t  d_desc_type_dj_dirent64_t
#else
    #define d_desc_type_dj_stat_t    d_desc_type_dj_stat32_t
    #define d_desc_type_dj_dirent_t  d_desc_type_dj_dirent32_t
#endif

// 1.4
// d_desc_type_dj_file_notice
//   constant: a filesys notice: "a filesys error from d_file_open_stream",
// with its message, path and error code in the sentence. Its facts: level,
// function, path, message and error.
extern const struct dj_desc_type d_desc_type_dj_file_notice;

#endif  // D_INTERNAL_DESC_OF_FILESYS


#if D_INTERNAL_DESC_OF_FILESYS

//==============================================================================
// 2.  RECORDS
//==============================================================================
//   Each described type as its describer knows it, as memory's are
// (mem_desc.h): the type's fields, in its order, under the same knobs.
// Every field of each is read.

#if D_FILE_64_IS_AVAILABLE
struct dj_fs_desc_stat64_record
{
    uint64_t st_size;
    int64_t  st_modified;
    int64_t  st_accessed;
    int64_t  st_changed;
    int64_t  st_created;
    uint32_t st_modified_nsec;
    uint32_t st_accessed_nsec;
    uint32_t st_changed_nsec;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_dev;
    uint64_t st_ino;
};

struct dj_fs_desc_dirent64_record
{
    char     d_name[D_INTERNAL_FILE_NAME_MAX + 1];
    uint64_t d_ino;
    uint8_t  d_type;
};
#endif  // D_FILE_64_IS_AVAILABLE

struct dj_fs_desc_stat32_record
{
    uint32_t st_size;
    int32_t  st_modified;
    int32_t  st_accessed;
    int32_t  st_changed;
    int32_t  st_created;
    uint32_t st_modified_nsec;
    uint32_t st_accessed_nsec;
    uint32_t st_changed_nsec;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t st_dev;
    uint32_t st_ino;
};

struct dj_fs_desc_dirent32_record
{
    char     d_name[D_INTERNAL_FILE_NAME_MAX + 1];
    uint32_t d_ino;
    uint8_t  d_type;
};

struct dj_fs_desc_notice_record
{
    int         level;
    int         error;
    const char* function;
    const char* path;
    const char* message;
};

#endif  // D_INTERNAL_DESC_OF_FILESYS


//==============================================================================
// 3.  LAYOUT
//==============================================================================

// the rows of a status and of an entry, at a width
#define D_FS_DESC_STAT(width_, field_)                                         \
    D_DESC_OF_LAYOUT_FIELD(dj_stat##width_##_t,                                \
                           dj_fs_desc_stat##width_##_record, field_)
#define D_FS_DESC_STAT_ROWS(width_)                                            \
    D_DESC_OF_LAYOUT_SIZE(dj_stat##width_##_t,                                 \
                          dj_fs_desc_stat##width_##_record),                   \
    D_FS_DESC_STAT(width_, st_size),                                           \
    D_FS_DESC_STAT(width_, st_modified),                                       \
    D_FS_DESC_STAT(width_, st_accessed),                                       \
    D_FS_DESC_STAT(width_, st_changed),                                        \
    D_FS_DESC_STAT(width_, st_created),                                        \
    D_FS_DESC_STAT(width_, st_modified_nsec),                                  \
    D_FS_DESC_STAT(width_, st_accessed_nsec),                                  \
    D_FS_DESC_STAT(width_, st_changed_nsec),                                   \
    D_FS_DESC_STAT(width_, st_mode),                                           \
    D_FS_DESC_STAT(width_, st_nlink),                                          \
    D_FS_DESC_STAT(width_, st_uid),                                            \
    D_FS_DESC_STAT(width_, st_gid),                                            \
    D_FS_DESC_STAT(width_, st_dev),                                            \
    D_FS_DESC_STAT(width_, st_ino)
#define D_FS_DESC_DIRENT_ROWS(width_)                                          \
    D_DESC_OF_LAYOUT_SIZE(dj_dirent##width_##_t,                               \
                          dj_fs_desc_dirent##width_##_record),                 \
    D_DESC_OF_LAYOUT_FIELD(dj_dirent##width_##_t,                              \
                           dj_fs_desc_dirent##width_##_record, d_name),        \
    D_DESC_OF_LAYOUT_FIELD(dj_dirent##width_##_t,                              \
                           dj_fs_desc_dirent##width_##_record, d_ino),         \
    D_DESC_OF_LAYOUT_FIELD(dj_dirent##width_##_t,                              \
                           dj_fs_desc_dirent##width_##_record, d_type)

/**
 * @brief filesys's layout rows as the caller is built: each described
 *        type's size, and the offset and width of each field its describer
 *        reads, as recorded and as the type has them.
 *
 * @param[out] _rows  set to the rows, static; null when filesys's
 *                    describers are not built. May be null.
 * @return the rows' count; 0 when filesys's describers are not built.
 */
D_INLINE size_t
d_desc_of_filesys_layout_here(
    const struct dj_desc_of_layout** _rows
)
{
#if D_INTERNAL_DESC_OF_FILESYS
    static const struct dj_desc_of_layout rows[] =
    {
#if D_FILE_64_IS_AVAILABLE
        D_FS_DESC_STAT_ROWS(64),
        D_FS_DESC_DIRENT_ROWS(64),
#endif
        D_FS_DESC_STAT_ROWS(32),
        D_FS_DESC_DIRENT_ROWS(32),
        D_DESC_OF_LAYOUT_SIZE(dj_file_notice, dj_fs_desc_notice_record),
        D_DESC_OF_LAYOUT_FIELD(dj_file_notice, dj_fs_desc_notice_record,
                               level),
        D_DESC_OF_LAYOUT_FIELD(dj_file_notice, dj_fs_desc_notice_record,
                               error),
        D_DESC_OF_LAYOUT_FIELD(dj_file_notice, dj_fs_desc_notice_record,
                               function),
        D_DESC_OF_LAYOUT_FIELD(dj_file_notice, dj_fs_desc_notice_record,
                               path),
        D_DESC_OF_LAYOUT_FIELD(dj_file_notice, dj_fs_desc_notice_record,
                               message)
    };

    if (_rows != NULL)
    {
        *_rows = rows;
    }

    return sizeof(rows) / sizeof(rows[0]);
#else
    if (_rows != NULL)
    {
        *_rows = NULL;
    }

    return 0u;
#endif
}

#undef D_FS_DESC_STAT
#undef D_FS_DESC_STAT_ROWS
#undef D_FS_DESC_DIRENT_ROWS

/**
 * @brief filesys's layout rows as the library was built.
 *
 * @param[out] _rows  as d_desc_of_filesys_layout_here.
 * @return as d_desc_of_filesys_layout_here.
 */
size_t d_desc_of_filesys_layout(const struct dj_desc_of_layout** _rows);

/**
 * @brief Whether the library lays filesys's described types out as the
 *        caller does: built with the same filesys knobs, as far as its
 *        describers read.
 *
 * @param[out] _index  set to the first row the two differ in; may be null.
 * @return 1 when they agree, or neither has filesys's describers; else 0.
 */
D_INLINE int
d_desc_of_filesys_agrees(
    size_t* _index
)
{
    const struct dj_desc_of_layout* built = NULL;
    const struct dj_desc_of_layout* here  = NULL;
    size_t built_count = d_desc_of_filesys_layout(&built);
    size_t here_count  = d_desc_of_filesys_layout_here(&here);

    return d_desc_of_agree(built, built_count, here, here_count, _index);
}


D_EXTERN_C_END


#endif  // DJINTERP_C_DESC_OF_FILESYS_DESC_H
