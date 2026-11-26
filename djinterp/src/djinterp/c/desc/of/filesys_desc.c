/*******************************************************************************
* djinterp [c][desc]                                              filesys_desc.c
*
*   desc's describers of filesys's types: a file's status and a directory
* entry, each read at either width into one wide view that a plan and a
* reader share; and a filesys notice. For each, its units and its field
* list; and the layout rows as this library was built. A describer reads
* fields only, so this unit calls none of filesys's functions.
*
*
* path:      /src/djinterp/c/desc/of/filesys_desc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#include "../../../../../inc/djinterp/c/desc/of/filesys_desc.h"  // header

#if D_INTERNAL_DESC_OF_FILESYS

//==============================================================================
// 1.  COMMON
//==============================================================================

// the widest numbers filesys builds
#if D_FILE_64_IS_AVAILABLE
    typedef uint64_t dj_fs_desc_unsigned;
    typedef int64_t  dj_fs_desc_signed;
#else
    typedef uint32_t dj_fs_desc_unsigned;
    typedef int32_t  dj_fs_desc_signed;
#endif

// kinds, by number: their facts' words, and the plan's bit is 1 << kind
#define D_FS_DESC_KIND_UNKNOWN    0u
#define D_FS_DESC_KIND_REGULAR    1u
#define D_FS_DESC_KIND_DIRECTORY  2u
#define D_FS_DESC_KIND_LINK       3u
#define D_FS_DESC_KIND_CHARACTER  4u
#define D_FS_DESC_KIND_BLOCK      5u
#define D_FS_DESC_KIND_FIFO       6u
#define D_FS_DESC_KIND_SOCKET     7u
#define D_FS_DESC_KIND_BIT(kind_) (1u << (kind_))

static const char* const D_FS_DESC_KIND_KEYS[] =
{
    "unknown", "regular", "directory", "symbolic_link", "character_device",
    "block_device", "fifo", "socket"
};

// a permission triplet, by its three bits
static const char* const D_FS_DESC_PERMS[] =
{
    "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"
};

/*
d_internal_fs_desc_unsigned
  An unsigned number as a stream value, whatever its width.
*/
static struct dj_ostream_value
d_internal_fs_desc_unsigned(
    dj_fs_desc_unsigned _value
)
{
    struct dj_ostream_value value = d_ostream_value_size(0u);

    value.low  = (uint32_t)(_value & 0xFFFFFFFFu);
    value.high = (uint32_t)(((_value >> 16) >> 16) & 0xFFFFFFFFu);

    return value;
}

/*
d_internal_fs_desc_signed
  A signed number as a stream value: its magnitude, taken in unsigned
arithmetic, which the least value survives, and its sign.
*/
static struct dj_ostream_value
d_internal_fs_desc_signed(
    dj_fs_desc_signed _value
)
{
    struct dj_ostream_value value = d_internal_fs_desc_unsigned(
        (_value < 0) ? (dj_fs_desc_unsigned)(0u - (dj_fs_desc_unsigned)_value)
                     : (dj_fs_desc_unsigned)_value);

    value.negative = (_value < 0) ? 1 : 0;

    return value;
}

/*
d_internal_fs_desc_mode_kind
  A mode's kind, by filesys's S_IS* tests.
*/
static unsigned
d_internal_fs_desc_mode_kind(
    uint32_t _mode
)
{
    if (S_ISREG(_mode))  { return D_FS_DESC_KIND_REGULAR; }
    if (S_ISDIR(_mode))  { return D_FS_DESC_KIND_DIRECTORY; }
    if (S_ISLNK(_mode))  { return D_FS_DESC_KIND_LINK; }
    if (S_ISCHR(_mode))  { return D_FS_DESC_KIND_CHARACTER; }
    if (S_ISBLK(_mode))  { return D_FS_DESC_KIND_BLOCK; }
    if (S_ISFIFO(_mode)) { return D_FS_DESC_KIND_FIFO; }
    if (S_ISSOCK(_mode)) { return D_FS_DESC_KIND_SOCKET; }

    return D_FS_DESC_KIND_UNKNOWN;
}

/*
d_internal_fs_desc_entry_kind
  An entry's kind, by its d_type.
*/
static unsigned
d_internal_fs_desc_entry_kind(
    unsigned _type
)
{
    switch (_type)
    {
        case DT_REG:  return D_FS_DESC_KIND_REGULAR;
        case DT_DIR:  return D_FS_DESC_KIND_DIRECTORY;
        case DT_LNK:  return D_FS_DESC_KIND_LINK;
        case DT_CHR:  return D_FS_DESC_KIND_CHARACTER;
        case DT_BLK:  return D_FS_DESC_KIND_BLOCK;
        case DT_FIFO: return D_FS_DESC_KIND_FIFO;
        case DT_SOCK: return D_FS_DESC_KIND_SOCKET;
        default:      return D_FS_DESC_KIND_UNKNOWN;
    }
}


//==============================================================================
// 2.  THE FILE'S STATUS
//==============================================================================

// the fields, in the field list's order
#define D_FS_DESC_STAT_KIND           0u
#define D_FS_DESC_STAT_SIZE           1u
#define D_FS_DESC_STAT_MODE           2u
#define D_FS_DESC_STAT_OWNER          3u
#define D_FS_DESC_STAT_GROUP          4u
#define D_FS_DESC_STAT_OTHER          5u
#define D_FS_DESC_STAT_LINKS          6u
#define D_FS_DESC_STAT_UID            7u
#define D_FS_DESC_STAT_GID            8u
#define D_FS_DESC_STAT_MODIFIED       9u
#define D_FS_DESC_STAT_ACCESSED       10u
#define D_FS_DESC_STAT_CHANGED        11u
#define D_FS_DESC_STAT_CREATED        12u
#define D_FS_DESC_STAT_MODIFIED_NSEC  13u
#define D_FS_DESC_STAT_ACCESSED_NSEC  14u
#define D_FS_DESC_STAT_CHANGED_NSEC   15u
#define D_FS_DESC_STAT_DEVICE         16u
#define D_FS_DESC_STAT_INODE          17u

// the plan's bits beside the kind's
#define D_FS_DESC_STAT_IF_EMPTY       (1u << 8)
#define D_FS_DESC_STAT_IF_INODE       (1u << 9)

// dj_fs_desc_status
//   struct: a status at the widest width filesys builds.
struct dj_fs_desc_status
{
    dj_fs_desc_unsigned size;
    dj_fs_desc_signed   modified;
    dj_fs_desc_signed   accessed;
    dj_fs_desc_signed   changed;
    dj_fs_desc_signed   created;
    uint32_t            modified_nsec;
    uint32_t            accessed_nsec;
    uint32_t            changed_nsec;
    uint32_t            mode;
    uint32_t            nlink;
    uint32_t            uid;
    uint32_t            gid;
    dj_fs_desc_unsigned dev;
    dj_fs_desc_unsigned ino;
};

// D_FS_DESC_STATUS_FROM
//   macro: a status of either width, widened field by field.
#define D_FS_DESC_STATUS_FROM(out_, in_)                                       \
    do                                                                         \
    {                                                                          \
        (out_)->size          = (dj_fs_desc_unsigned)(in_)->st_size;           \
        (out_)->modified      = (dj_fs_desc_signed)(in_)->st_modified;         \
        (out_)->accessed      = (dj_fs_desc_signed)(in_)->st_accessed;         \
        (out_)->changed       = (dj_fs_desc_signed)(in_)->st_changed;          \
        (out_)->created       = (dj_fs_desc_signed)(in_)->st_created;          \
        (out_)->modified_nsec = (in_)->st_modified_nsec;                       \
        (out_)->accessed_nsec = (in_)->st_accessed_nsec;                       \
        (out_)->changed_nsec  = (in_)->st_changed_nsec;                        \
        (out_)->mode          = (in_)->st_mode;                                \
        (out_)->nlink         = (in_)->st_nlink;                               \
        (out_)->uid           = (in_)->st_uid;                                 \
        (out_)->gid           = (in_)->st_gid;                                 \
        (out_)->dev           = (dj_fs_desc_unsigned)(in_)->st_dev;            \
        (out_)->ino           = (dj_fs_desc_unsigned)(in_)->st_ino;            \
    } while (0)

/*
d_internal_fs_desc_status_plan
  The kind at every ring, for the noun; emptiness at the line; the inode at
the detail.
*/
static uint32_t
d_internal_fs_desc_status_plan(
    const struct dj_fs_desc_status* _status,
    uint32_t                        _ring
)
{
    unsigned kind = d_internal_fs_desc_mode_kind(_status->mode);
    uint32_t bits = D_FS_DESC_KIND_BIT(kind);

    if ( (_ring >= D_DESC_RING_LINE)          &&
         (kind == D_FS_DESC_KIND_REGULAR)     &&
         (_status->size == 0u) )
    {
        bits |= D_FS_DESC_STAT_IF_EMPTY;
    }

    if ( (_ring >= D_DESC_RING_DETAILED) && (_status->ino != 0u) )
    {
        bits |= D_FS_DESC_STAT_IF_INODE;
    }

    return bits;
}

/*
d_internal_fs_desc_status_read
  Reads one field; a change or creation time of 0, which filesys reports
for want of one, is none.
*/
static struct dj_ostream_value
d_internal_fs_desc_status_read(
    const struct dj_fs_desc_status* _status,
    uint32_t                        _field
)
{
    uint32_t mode = _status->mode;

    switch (_field)
    {
        case D_FS_DESC_STAT_KIND:
            return d_ostream_value_text(
                D_FS_DESC_KIND_KEYS[d_internal_fs_desc_mode_kind(mode)]);
        case D_FS_DESC_STAT_SIZE:
            return d_internal_fs_desc_unsigned(_status->size);
        case D_FS_DESC_STAT_MODE:
            return d_ostream_value_ulong((unsigned long)mode);
        case D_FS_DESC_STAT_OWNER:
            return d_ostream_value_text(D_FS_DESC_PERMS[(mode >> 6) & 7u]);
        case D_FS_DESC_STAT_GROUP:
            return d_ostream_value_text(D_FS_DESC_PERMS[(mode >> 3) & 7u]);
        case D_FS_DESC_STAT_OTHER:
            return d_ostream_value_text(D_FS_DESC_PERMS[mode & 7u]);
        case D_FS_DESC_STAT_LINKS:
            return d_ostream_value_ulong((unsigned long)_status->nlink);
        case D_FS_DESC_STAT_UID:
            return d_ostream_value_ulong((unsigned long)_status->uid);
        case D_FS_DESC_STAT_GID:
            return d_ostream_value_ulong((unsigned long)_status->gid);
        case D_FS_DESC_STAT_MODIFIED:
            return d_internal_fs_desc_signed(_status->modified);
        case D_FS_DESC_STAT_ACCESSED:
            return d_internal_fs_desc_signed(_status->accessed);
        case D_FS_DESC_STAT_CHANGED:
            return (_status->changed != 0)
                       ? d_internal_fs_desc_signed(_status->changed)
                       : d_ostream_value_none();
        case D_FS_DESC_STAT_CREATED:
            return (_status->created != 0)
                       ? d_internal_fs_desc_signed(_status->created)
                       : d_ostream_value_none();
        case D_FS_DESC_STAT_MODIFIED_NSEC:
            return d_ostream_value_ulong((unsigned long)_status->modified_nsec);
        case D_FS_DESC_STAT_ACCESSED_NSEC:
            return d_ostream_value_ulong((unsigned long)_status->accessed_nsec);
        case D_FS_DESC_STAT_CHANGED_NSEC:
            return d_ostream_value_ulong((unsigned long)_status->changed_nsec);
        case D_FS_DESC_STAT_DEVICE:
            return d_internal_fs_desc_unsigned(_status->dev);
        case D_FS_DESC_STAT_INODE:
            return d_internal_fs_desc_unsigned(_status->ino);
        default:
            return d_ostream_value_none();
    }
}

#if D_FILE_64_IS_AVAILABLE
/*
d_internal_fs_desc_stat64_plan, d_internal_fs_desc_stat64_read
  The plan and the reader of a 64-bit status.
*/
static uint32_t
d_internal_fs_desc_stat64_plan(
    const void* _object,
    uint32_t    _ring
)
{
    struct dj_fs_desc_status status;

    D_FS_DESC_STATUS_FROM(&status, (const struct dj_stat64_t*)_object);

    return d_internal_fs_desc_status_plan(&status, _ring);
}

static struct dj_ostream_value
d_internal_fs_desc_stat64_read(
    const void* _object,
    uint32_t    _field
)
{
    struct dj_fs_desc_status status;

    D_FS_DESC_STATUS_FROM(&status, (const struct dj_stat64_t*)_object);

    return d_internal_fs_desc_status_read(&status, _field);
}
#endif  // D_FILE_64_IS_AVAILABLE

/*
d_internal_fs_desc_stat32_plan, d_internal_fs_desc_stat32_read
  The plan and the reader of a 32-bit status.
*/
static uint32_t
d_internal_fs_desc_stat32_plan(
    const void* _object,
    uint32_t    _ring
)
{
    struct dj_fs_desc_status status;

    D_FS_DESC_STATUS_FROM(&status, (const struct dj_stat32_t*)_object);

    return d_internal_fs_desc_status_plan(&status, _ring);
}

static struct dj_ostream_value
d_internal_fs_desc_stat32_read(
    const void* _object,
    uint32_t    _field
)
{
    struct dj_fs_desc_status status;

    D_FS_DESC_STATUS_FROM(&status, (const struct dj_stat32_t*)_object);

    return d_internal_fs_desc_status_read(&status, _field);
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
// the nouns, one a kind
static const struct dj_desc_op D_FS_DESC_OPS_FILE[] =
    { D_DESC_T("file"), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_REGULAR[] =
    { D_DESC_T("regular file"), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_DIRECTORY[] =
    { D_DESC_T("directory"), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_LINK[] =
    { D_DESC_T("symbolic link"), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_CHARACTER[] =
    { D_DESC_T("character device"), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_BLOCK[] =
    { D_DESC_T("block device"), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_FIFO[] =
    { D_DESC_T("FIFO"), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_SOCKET[] =
    { D_DESC_T("socket"), D_DESC_END };
#if D_DESC_BUILT(LINE, STATE)
static const struct dj_desc_op D_FS_DESC_OPS_EMPTY[] =
    { D_DESC_T("empty"), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_BYTES[] =
    { D_DESC_F(D_FS_DESC_STAT_SIZE), D_DESC_T(" byte"), D_DESC_PL,
      D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
static const struct dj_desc_op D_FS_DESC_OPS_PERMS[] =
    { D_DESC_T("Its permissions are "), D_DESC_F(D_FS_DESC_STAT_OWNER),
      D_DESC_F(D_FS_DESC_STAT_GROUP), D_DESC_F(D_FS_DESC_STAT_OTHER),
      D_DESC_T("."), D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, IDENTITY)
static const struct dj_desc_op D_FS_DESC_OPS_OWNERS[] =
    { D_DESC_T("It belongs to user "), D_DESC_F(D_FS_DESC_STAT_UID),
      D_DESC_T(" and group "), D_DESC_F(D_FS_DESC_STAT_GID), D_DESC_T("."),
      D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_STAT_INODE[] =
    { D_DESC_T("Its inode is "), D_DESC_F(D_FS_DESC_STAT_INODE),
      D_DESC_T(" on device "), D_DESC_F(D_FS_DESC_STAT_DEVICE),
      D_DESC_T("."), D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, STATE)
static const struct dj_desc_op D_FS_DESC_OPS_LINKS[] =
    { D_DESC_T("It has "), D_DESC_F(D_FS_DESC_STAT_LINKS),
      D_DESC_T(" hard link"), D_DESC_PL, D_DESC_T("."), D_DESC_END };
#endif

// D_FS_DESC_NOUN
//   macro: the noun unit of a kind.
#define D_FS_DESC_NOUN(kind_, ops_)                                            \
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,         \
                D_FS_DESC_KIND_BIT(kind_), 0u, (ops_))

static const struct dj_desc_unit d_internal_fs_desc_stat_units[] =
{
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_FS_DESC_STAT_IF_EMPTY, 0u, D_FS_DESC_OPS_EMPTY),
#endif
    D_FS_DESC_NOUN(D_FS_DESC_KIND_UNKNOWN,   D_FS_DESC_OPS_FILE),
    D_FS_DESC_NOUN(D_FS_DESC_KIND_REGULAR,   D_FS_DESC_OPS_REGULAR),
    D_FS_DESC_NOUN(D_FS_DESC_KIND_DIRECTORY, D_FS_DESC_OPS_DIRECTORY),
    D_FS_DESC_NOUN(D_FS_DESC_KIND_LINK,      D_FS_DESC_OPS_LINK),
    D_FS_DESC_NOUN(D_FS_DESC_KIND_CHARACTER, D_FS_DESC_OPS_CHARACTER),
    D_FS_DESC_NOUN(D_FS_DESC_KIND_BLOCK,     D_FS_DESC_OPS_BLOCK),
    D_FS_DESC_NOUN(D_FS_DESC_KIND_FIFO,      D_FS_DESC_OPS_FIFO),
    D_FS_DESC_NOUN(D_FS_DESC_KIND_SOCKET,    D_FS_DESC_OPS_SOCKET),
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH,
                D_FS_DESC_KIND_BIT(D_FS_DESC_KIND_REGULAR),
                D_FS_DESC_STAT_IF_EMPTY, D_FS_DESC_OPS_BYTES),
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_REMARK, 0u, 0u, D_FS_DESC_OPS_PERMS),
#endif
#if D_DESC_BUILT(DETAILED, IDENTITY)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_IDENTITY,
                D_DESC_SLOT_REMARK, 0u, 0u, D_FS_DESC_OPS_OWNERS),
#endif
#if D_DESC_BUILT(DETAILED, STATE)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_STATE,
                D_DESC_SLOT_REMARK, 0u, 0u, D_FS_DESC_OPS_LINKS),
#endif
#if D_DESC_BUILT(DETAILED, IDENTITY)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_IDENTITY,
                D_DESC_SLOT_REMARK, D_FS_DESC_STAT_IF_INODE, 0u,
                D_FS_DESC_OPS_STAT_INODE),
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
// D_FS_DESC_SEEN
//   macro: a field list's row of a status's state now.
#define D_FS_DESC_SEEN(key_, label_, layer_, importance_)                      \
    D_DESC_FIELD((key_), (label_), (layer_), (importance_),                    \
                 D_DESC_TRUTH_OBSERVED)

static const struct dj_desc_field d_internal_fs_desc_stat_fields[] =
{
    D_FS_DESC_SEEN("kind", "kind", D_DESC_LAYER_TYPE, 0u),
    D_FS_DESC_SEEN("size", "bytes", D_DESC_LAYER_STATE, 0u),
    D_FS_DESC_SEEN("mode", "mode", D_DESC_LAYER_POLICY, 2u),
    D_FS_DESC_SEEN("owner", "owner's permissions", D_DESC_LAYER_POLICY, 1u),
    D_FS_DESC_SEEN("group", "group's permissions", D_DESC_LAYER_POLICY, 1u),
    D_FS_DESC_SEEN("other", "others' permissions", D_DESC_LAYER_POLICY, 1u),
    D_FS_DESC_SEEN("links", "hard links", D_DESC_LAYER_STATE, 2u),
    D_FS_DESC_SEEN("uid", "user", D_DESC_LAYER_IDENTITY, 2u),
    D_FS_DESC_SEEN("gid", "group", D_DESC_LAYER_IDENTITY, 2u),
    D_FS_DESC_SEEN("modified", "modified", D_DESC_LAYER_STATE, 1u),
    D_FS_DESC_SEEN("accessed", "accessed", D_DESC_LAYER_STATE, 2u),
    D_FS_DESC_SEEN("changed", "changed", D_DESC_LAYER_STATE, 2u),
    D_FS_DESC_SEEN("created", "created", D_DESC_LAYER_STATE, 2u),
    D_FS_DESC_SEEN("modified_nsec", NULL, D_DESC_LAYER_STATE, 3u),
    D_FS_DESC_SEEN("accessed_nsec", NULL, D_DESC_LAYER_STATE, 3u),
    D_FS_DESC_SEEN("changed_nsec", NULL, D_DESC_LAYER_STATE, 3u),
    D_FS_DESC_SEEN("device", "device", D_DESC_LAYER_IDENTITY, 3u),
    D_FS_DESC_SEEN("inode", "inode", D_DESC_LAYER_IDENTITY, 3u)
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

#if D_FILE_64_IS_AVAILABLE
const struct dj_desc_type d_desc_type_dj_stat64_t =
{
    "stat64",
    "file status",
    D_DESC_UNITS_TABLE(d_internal_fs_desc_stat_units),
    D_DESC_FIELDS_TABLE(d_internal_fs_desc_stat_fields),
    d_internal_fs_desc_stat64_plan,
    d_internal_fs_desc_stat64_read,
    NULL
};
#endif  // D_FILE_64_IS_AVAILABLE

const struct dj_desc_type d_desc_type_dj_stat32_t =
{
    "stat32",
    "file status",
    D_DESC_UNITS_TABLE(d_internal_fs_desc_stat_units),
    D_DESC_FIELDS_TABLE(d_internal_fs_desc_stat_fields),
    d_internal_fs_desc_stat32_plan,
    d_internal_fs_desc_stat32_read,
    NULL
};


//==============================================================================
// 3.  THE DIRECTORY ENTRY
//==============================================================================

#define D_FS_DESC_ENTRY_NAME      0u
#define D_FS_DESC_ENTRY_KIND      1u
#define D_FS_DESC_ENTRY_INODE     2u

// the plan's bits beside the kind's
#define D_FS_DESC_ENTRY_IF_NAMED  (1u << 8)
#define D_FS_DESC_ENTRY_IF_INODE  (1u << 9)

// dj_fs_desc_entry
//   struct: an entry at the widest width filesys builds; its name is the
// entry's own.
struct dj_fs_desc_entry
{
    const char*         name;
    dj_fs_desc_unsigned ino;
    unsigned            type;
};

// D_FS_DESC_ENTRY_FROM
//   macro: an entry of either width, widened.
#define D_FS_DESC_ENTRY_FROM(out_, in_)                                        \
    do                                                                         \
    {                                                                          \
        (out_)->name = (in_)->d_name;                                          \
        (out_)->ino  = (dj_fs_desc_unsigned)(in_)->d_ino;                      \
        (out_)->type = (unsigned)(in_)->d_type;                                \
    } while (0)

/*
d_internal_fs_desc_entry_plan
  The kind at the sentence; the name at the line; the inode at the detail.
*/
static uint32_t
d_internal_fs_desc_entry_plan(
    const struct dj_fs_desc_entry* _entry,
    uint32_t                       _ring
)
{
    uint32_t bits = 0u;

    if ( (_ring >= D_DESC_RING_LINE) && (_entry->name[0] != '\0') )
    {
        bits |= D_FS_DESC_ENTRY_IF_NAMED;
    }

    if (_ring >= D_DESC_RING_SENTENCE)
    {
        bits |= D_FS_DESC_KIND_BIT(
                    d_internal_fs_desc_entry_kind(_entry->type));
    }

    if ( (_ring >= D_DESC_RING_DETAILED) && (_entry->ino != 0u) )
    {
        bits |= D_FS_DESC_ENTRY_IF_INODE;
    }

    return bits;
}

/*
d_internal_fs_desc_entry_read
  Reads one field; an empty name is none.
*/
static struct dj_ostream_value
d_internal_fs_desc_entry_read(
    const struct dj_fs_desc_entry* _entry,
    uint32_t                       _field
)
{
    switch (_field)
    {
        case D_FS_DESC_ENTRY_NAME:
            return (_entry->name[0] != '\0')
                       ? d_ostream_value_text(_entry->name)
                       : d_ostream_value_none();
        case D_FS_DESC_ENTRY_KIND:
            return d_ostream_value_text(
                D_FS_DESC_KIND_KEYS[d_internal_fs_desc_entry_kind(
                                        _entry->type)]);
        case D_FS_DESC_ENTRY_INODE:
            return d_internal_fs_desc_unsigned(_entry->ino);
        default:
            return d_ostream_value_none();
    }
}

#if D_FILE_64_IS_AVAILABLE
/*
d_internal_fs_desc_dirent64_plan, d_internal_fs_desc_dirent64_read
  The plan and the reader of a 64-bit entry.
*/
static uint32_t
d_internal_fs_desc_dirent64_plan(
    const void* _object,
    uint32_t    _ring
)
{
    struct dj_fs_desc_entry entry;

    D_FS_DESC_ENTRY_FROM(&entry, (const struct dj_dirent64_t*)_object);

    return d_internal_fs_desc_entry_plan(&entry, _ring);
}

static struct dj_ostream_value
d_internal_fs_desc_dirent64_read(
    const void* _object,
    uint32_t    _field
)
{
    struct dj_fs_desc_entry entry;

    D_FS_DESC_ENTRY_FROM(&entry, (const struct dj_dirent64_t*)_object);

    return d_internal_fs_desc_entry_read(&entry, _field);
}
#endif  // D_FILE_64_IS_AVAILABLE

/*
d_internal_fs_desc_dirent32_plan, d_internal_fs_desc_dirent32_read
  The plan and the reader of a 32-bit entry.
*/
static uint32_t
d_internal_fs_desc_dirent32_plan(
    const void* _object,
    uint32_t    _ring
)
{
    struct dj_fs_desc_entry entry;

    D_FS_DESC_ENTRY_FROM(&entry, (const struct dj_dirent32_t*)_object);

    return d_internal_fs_desc_entry_plan(&entry, _ring);
}

static struct dj_ostream_value
d_internal_fs_desc_dirent32_read(
    const void* _object,
    uint32_t    _field
)
{
    struct dj_fs_desc_entry entry;

    D_FS_DESC_ENTRY_FROM(&entry, (const struct dj_dirent32_t*)_object);

    return d_internal_fs_desc_entry_read(&entry, _field);
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
static const struct dj_desc_op D_FS_DESC_OPS_ENTRY[] =
    { D_DESC_T("directory entry"), D_DESC_END };
#if D_DESC_BUILT(LINE, IDENTITY)
static const struct dj_desc_op D_FS_DESC_OPS_NAMED[] =
    { D_DESC_T("named "), D_DESC_F(D_FS_DESC_ENTRY_NAME), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, TYPE)
static const struct dj_desc_op D_FS_DESC_OPS_NAMES_UNKNOWN[] =
    { D_DESC_T("Its kind is not known."), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_NAMES_REGULAR[] =
    { D_DESC_T("It names a regular file."), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_NAMES_DIRECTORY[] =
    { D_DESC_T("It names a directory."), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_NAMES_LINK[] =
    { D_DESC_T("It names a symbolic link."), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_NAMES_CHARACTER[] =
    { D_DESC_T("It names a character device."), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_NAMES_BLOCK[] =
    { D_DESC_T("It names a block device."), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_NAMES_FIFO[] =
    { D_DESC_T("It names a FIFO."), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_NAMES_SOCKET[] =
    { D_DESC_T("It names a socket."), D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, IDENTITY)
static const struct dj_desc_op D_FS_DESC_OPS_ENTRY_INODE[] =
    { D_DESC_T("Its inode is "), D_DESC_F(D_FS_DESC_ENTRY_INODE),
      D_DESC_T("."), D_DESC_END };
#endif

// D_FS_DESC_NAMES
//   macro: the sentence's unit of an entry's kind.
#define D_FS_DESC_NAMES(kind_, ops_)                                           \
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_TYPE, D_DESC_SLOT_REMARK,   \
                D_FS_DESC_KIND_BIT(kind_), 0u, (ops_))

static const struct dj_desc_unit d_internal_fs_desc_entry_units[] =
{
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,
                0u, 0u, D_FS_DESC_OPS_ENTRY),
#if D_DESC_BUILT(LINE, IDENTITY)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_IDENTITY,
                D_DESC_SLOT_COMPLEMENT, D_FS_DESC_ENTRY_IF_NAMED, 0u,
                D_FS_DESC_OPS_NAMED),
#endif
#if D_DESC_BUILT(SENTENCE, TYPE)
    D_FS_DESC_NAMES(D_FS_DESC_KIND_UNKNOWN, D_FS_DESC_OPS_NAMES_UNKNOWN),
    D_FS_DESC_NAMES(D_FS_DESC_KIND_REGULAR, D_FS_DESC_OPS_NAMES_REGULAR),
    D_FS_DESC_NAMES(D_FS_DESC_KIND_DIRECTORY,
                    D_FS_DESC_OPS_NAMES_DIRECTORY),
    D_FS_DESC_NAMES(D_FS_DESC_KIND_LINK, D_FS_DESC_OPS_NAMES_LINK),
    D_FS_DESC_NAMES(D_FS_DESC_KIND_CHARACTER,
                    D_FS_DESC_OPS_NAMES_CHARACTER),
    D_FS_DESC_NAMES(D_FS_DESC_KIND_BLOCK, D_FS_DESC_OPS_NAMES_BLOCK),
    D_FS_DESC_NAMES(D_FS_DESC_KIND_FIFO, D_FS_DESC_OPS_NAMES_FIFO),
    D_FS_DESC_NAMES(D_FS_DESC_KIND_SOCKET, D_FS_DESC_OPS_NAMES_SOCKET),
#endif
#if D_DESC_BUILT(DETAILED, IDENTITY)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_IDENTITY,
                D_DESC_SLOT_REMARK, D_FS_DESC_ENTRY_IF_INODE, 0u,
                D_FS_DESC_OPS_ENTRY_INODE),
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
static const struct dj_desc_field d_internal_fs_desc_entry_fields[] =
{
    D_DESC_FIELD("name", "named", D_DESC_LAYER_IDENTITY, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("kind", "kind", D_DESC_LAYER_TYPE, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("inode", "inode", D_DESC_LAYER_IDENTITY, 3u,
                 D_DESC_TRUTH_OBSERVED)
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

#if D_FILE_64_IS_AVAILABLE
const struct dj_desc_type d_desc_type_dj_dirent64_t =
{
    "dirent64",
    "directory entry",
    D_DESC_UNITS_TABLE(d_internal_fs_desc_entry_units),
    D_DESC_FIELDS_TABLE(d_internal_fs_desc_entry_fields),
    d_internal_fs_desc_dirent64_plan,
    d_internal_fs_desc_dirent64_read,
    NULL
};
#endif  // D_FILE_64_IS_AVAILABLE

const struct dj_desc_type d_desc_type_dj_dirent32_t =
{
    "dirent32",
    "directory entry",
    D_DESC_UNITS_TABLE(d_internal_fs_desc_entry_units),
    D_DESC_FIELDS_TABLE(d_internal_fs_desc_entry_fields),
    d_internal_fs_desc_dirent32_plan,
    d_internal_fs_desc_dirent32_read,
    NULL
};


//==============================================================================
// 4.  THE NOTICE
//==============================================================================

#define D_FS_DESC_NOTICE_LEVEL        0u
#define D_FS_DESC_NOTICE_FUNCTION     1u
#define D_FS_DESC_NOTICE_PATH         2u
#define D_FS_DESC_NOTICE_MESSAGE      3u
#define D_FS_DESC_NOTICE_ERROR        4u

// the plan's bits: the level's is 1 << level, then these
#define D_FS_DESC_NOTICE_IF_FUNCTION  (1u << 5)
#define D_FS_DESC_NOTICE_IF_PATH      (1u << 6)
#define D_FS_DESC_NOTICE_IF_MESSAGE   (1u << 7)
#define D_FS_DESC_NOTICE_IF_CODE      (1u << 8)

// the levels' words, enum dj_file_notify_level's order
static const char* const D_FS_DESC_LEVEL_KEYS[] =
{
    "none", "error", "warning", "info", "trace"
};

/*
d_internal_fs_desc_level
  A notice's level, or none for a level filesys does not have.
*/
static unsigned
d_internal_fs_desc_level(
    const struct dj_file_notice* _notice
)
{
    return ( (_notice->level >= (int)D_FILE_NOTIFY_NONE) &&
             (_notice->level <= (int)D_FILE_NOTIFY_TRACE) )
               ? (unsigned)_notice->level
               : (unsigned)D_FILE_NOTIFY_NONE;
}

/*
d_internal_fs_desc_notice_plan
  The level at every ring, for the noun; where it came from at the line;
what it says at the sentence.
*/
static uint32_t
d_internal_fs_desc_notice_plan(
    const void* _object,
    uint32_t    _ring
)
{
    const struct dj_file_notice* notice =
        (const struct dj_file_notice*)_object;
    uint32_t bits = 1u << d_internal_fs_desc_level(notice);

    if ( (_ring >= D_DESC_RING_LINE) && (notice->function != NULL) )
    {
        bits |= D_FS_DESC_NOTICE_IF_FUNCTION;
    }

    if (_ring >= D_DESC_RING_SENTENCE)
    {
        bits |= (notice->path != NULL) ? D_FS_DESC_NOTICE_IF_PATH : 0u;
        bits |= (notice->message != NULL) ? D_FS_DESC_NOTICE_IF_MESSAGE
                                          : 0u;
        bits |= (notice->error != 0) ? D_FS_DESC_NOTICE_IF_CODE : 0u;
    }

    return bits;
}

/*
d_internal_fs_desc_notice_read
  Reads one field; a string filesys left null is none.
*/
static struct dj_ostream_value
d_internal_fs_desc_notice_read(
    const void* _object,
    uint32_t    _field
)
{
    const struct dj_file_notice* notice =
        (const struct dj_file_notice*)_object;

    switch (_field)
    {
        case D_FS_DESC_NOTICE_LEVEL:
            return d_ostream_value_text(
                D_FS_DESC_LEVEL_KEYS[d_internal_fs_desc_level(notice)]);
        case D_FS_DESC_NOTICE_FUNCTION:
            return d_ostream_value_text(notice->function);
        case D_FS_DESC_NOTICE_PATH:
            return d_ostream_value_text(notice->path);
        case D_FS_DESC_NOTICE_MESSAGE:
            return d_ostream_value_text(notice->message);
        case D_FS_DESC_NOTICE_ERROR:
            return d_ostream_value_long((long)notice->error);
        default:
            return d_ostream_value_none();
    }
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
static const struct dj_desc_op D_FS_DESC_OPS_NOTICE[] =
    { D_DESC_T("filesys notice"), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_ERROR[] =
    { D_DESC_T("filesys error"), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_WARNING[] =
    { D_DESC_T("filesys warning"), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_NOTE[] =
    { D_DESC_T("filesys note"), D_DESC_END };
static const struct dj_desc_op D_FS_DESC_OPS_TRACE[] =
    { D_DESC_T("filesys trace"), D_DESC_END };
#if D_DESC_BUILT(LINE, PROVENANCE)
static const struct dj_desc_op D_FS_DESC_OPS_FROM[] =
    { D_DESC_T("from "), D_DESC_F(D_FS_DESC_NOTICE_FUNCTION), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, STATE)
static const struct dj_desc_op D_FS_DESC_OPS_SAYS[] =
    { D_DESC_T("It says \""), D_DESC_F(D_FS_DESC_NOTICE_MESSAGE),
      D_DESC_T("\"."), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, IDENTITY)
static const struct dj_desc_op D_FS_DESC_OPS_PATH[] =
    { D_DESC_T("Its path is "), D_DESC_F(D_FS_DESC_NOTICE_PATH),
      D_DESC_T("."), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, HEALTH)
static const struct dj_desc_op D_FS_DESC_OPS_CODE[] =
    { D_DESC_T("Its error code is "), D_DESC_F(D_FS_DESC_NOTICE_ERROR),
      D_DESC_T("."), D_DESC_END };
#endif

// D_FS_DESC_LEVEL_NOUN
//   macro: the noun unit of a level.
#define D_FS_DESC_LEVEL_NOUN(level_, ops_)                                     \
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,         \
                1u << (level_), 0u, (ops_))

static const struct dj_desc_unit d_internal_fs_desc_notice_units[] =
{
    D_FS_DESC_LEVEL_NOUN(D_FILE_NOTIFY_NONE,  D_FS_DESC_OPS_NOTICE),
    D_FS_DESC_LEVEL_NOUN(D_FILE_NOTIFY_ERROR, D_FS_DESC_OPS_ERROR),
    D_FS_DESC_LEVEL_NOUN(D_FILE_NOTIFY_WARN,  D_FS_DESC_OPS_WARNING),
    D_FS_DESC_LEVEL_NOUN(D_FILE_NOTIFY_INFO,  D_FS_DESC_OPS_NOTE),
    D_FS_DESC_LEVEL_NOUN(D_FILE_NOTIFY_TRACE, D_FS_DESC_OPS_TRACE),
#if D_DESC_BUILT(LINE, PROVENANCE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_PROVENANCE,
                D_DESC_SLOT_COMPLEMENT, D_FS_DESC_NOTICE_IF_FUNCTION, 0u,
                D_FS_DESC_OPS_FROM),
#endif
#if D_DESC_BUILT(SENTENCE, STATE)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_STATE,
                D_DESC_SLOT_REMARK, D_FS_DESC_NOTICE_IF_MESSAGE, 0u,
                D_FS_DESC_OPS_SAYS),
#endif
#if D_DESC_BUILT(SENTENCE, IDENTITY)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_IDENTITY,
                D_DESC_SLOT_REMARK, D_FS_DESC_NOTICE_IF_PATH, 0u,
                D_FS_DESC_OPS_PATH),
#endif
#if D_DESC_BUILT(SENTENCE, HEALTH)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_HEALTH,
                D_DESC_SLOT_REMARK, D_FS_DESC_NOTICE_IF_CODE, 0u,
                D_FS_DESC_OPS_CODE),
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
static const struct dj_desc_field d_internal_fs_desc_notice_fields[] =
{
    D_DESC_FIELD("level", "level", D_DESC_LAYER_HEALTH, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("function", "from", D_DESC_LAYER_PROVENANCE, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("path", "path", D_DESC_LAYER_IDENTITY, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("message", "says", D_DESC_LAYER_STATE, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("error", "error code", D_DESC_LAYER_HEALTH, 1u,
                 D_DESC_TRUTH_OBSERVED)
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

const struct dj_desc_type d_desc_type_dj_file_notice =
{
    "file_notice",
    "filesys notice",
    D_DESC_UNITS_TABLE(d_internal_fs_desc_notice_units),
    D_DESC_FIELDS_TABLE(d_internal_fs_desc_notice_fields),
    d_internal_fs_desc_notice_plan,
    d_internal_fs_desc_notice_read,
    NULL
};

#endif  // D_INTERNAL_DESC_OF_FILESYS


//==============================================================================
// 5.  LAYOUT
//==============================================================================

/*
d_desc_of_filesys_layout
  filesys's layout rows as this library was built: the header's table,
compiled with the library's knobs.
*/
size_t
d_desc_of_filesys_layout(
    const struct dj_desc_of_layout** _rows
)
{
    return d_desc_of_filesys_layout_here(_rows);
}
