/*******************************************************************************
* djinterp [tools]                                                  dgit_index.c
*
* Definitions for the index and the work tree of dgit_repo.h.
*   The index is read whole, as git's read_index reads it: its header, each
* entry -- the stat data, the name, the flags, version 3's extended flags,
* and the path, padded in versions 2 and 3, and in version 4 the path the
* one before it gave, cut and continued -- then its extensions, of which
* only the split index's is one this does not read, and its checksum.
*   The work tree's rules are read as git reads them for a path: the ignore
* files of every directory above it and the repository's own, and the
* attribute files likewise, each kept once read. Status is git's: the index
* against HEAD's tree, then the work tree against the index -- the stat
* data first, the content, made clean as git would add it, when the stat
* data cannot say -- then the files the index does not hold, a directory of
* them shown as the directory.
*
* path:      /src/djinterp/tools/dawk/ext/git/dgit_index.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./dgit_internal.h"  // corresponding header: d_git_repo layout
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, calloc, realloc, free, getenv, qsort
#include <string.h>  // memchr, memcmp, memcpy, memset, strchr, strcmp,
                     // strlen, strncmp, strrchr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"                // d_fs_names
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_repo.h"  // d_git_index_entry
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_text.h"  // d_git_text
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, uint16_t,
                                                     // uint32_t


// D_INTERNAL_STAGE_SHIFT ... D_INTERNAL_INTENT
//   constant: an entry's flags as the index keeps them: its stage, whether
//   it is assumed unchanged, whether extended flags follow, the length of
//   its path; and, of the extended, skip-worktree and intent-to-add.
#define D_INTERNAL_STAGE_SHIFT 12u
#define D_INTERNAL_VALID       0x8000u
#define D_INTERNAL_EXTENDED    0x4000u
#define D_INTERNAL_NAME_MASK   0x0FFFu
#define D_INTERNAL_SKIP        0x4000u
#define D_INTERNAL_INTENT      0x2000u

//==============================================================================
// 1.  READING THE INDEX
//==============================================================================


/*
d_internal_be32, d_internal_be16
  Four and two bytes read big-endian.
*/
static uint32_t
d_internal_be32(
    const uint8_t* _bytes
)
{
    return ((uint32_t)_bytes[0] << 24) |
           ((uint32_t)_bytes[1] << 16) |
           ((uint32_t)_bytes[2] << 8)  |
           (uint32_t)_bytes[3];
}


static uint16_t
d_internal_be16(
    const uint8_t* _bytes
)
{
    return (uint16_t)(((unsigned)_bytes[0] << 8) | (unsigned)_bytes[1]);
}


void
d_git_index_release(
    struct d_git_repo* _repo
)
{
    free(_repo->entries);
    free(_repo->index_names);
    _repo->entries     = NULL;
    _repo->index_names = NULL;
    _repo->entry_count = 0u;
    _repo->index_read  = false;

    return;
}


// d_internal_index
//   struct: an index being read: its bytes, its version, where the next
//   entry begins, the path the last gave -- version 4 cuts it for the
//   next -- and the paths kept, end to end.
struct d_internal_index
{
    const uint8_t*      bytes;
    size_t              length;
    size_t              end;
    unsigned            version;
    size_t              at;
    struct d_git_buffer last;
    struct d_git_buffer names;
    size_t*             name_at;
};


/*
d_internal_v4_strip
  Version 4's count of bytes to cut from the last path, in git's offset
encoding: seven bits a byte, one more added before each shift.
*/
static bool
d_internal_v4_strip(
    struct d_internal_index* _index,
    size_t*                  _out
)
{
    // a byte to read
    if (_index->at >= _index->end)
    {
        return false;
    }

    uint8_t byte = _index->bytes[_index->at++];
    size_t  strip = byte & 0x7Fu;

    // each further byte
    while ( (byte & 0x80u) != 0u )
    {
        // a count that runs past the entries
        if (_index->at >= _index->end)
        {
            return false;
        }

        byte  = _index->bytes[_index->at++];
        strip = ((strip + 1u) << 7) | (byte & 0x7Fu);
    }

    *_out = strip;

    return true;
}


/*
d_internal_entry_path
  An entry's path: version 4's, the last cut and continued; the others',
to its NUL -- a length below 0xFFF is the flags' -- and padded with NULs
to a multiple of eight past the entry's start.
*/
static bool
d_internal_entry_path(
    struct d_internal_index* _index,
    size_t                   _start,
    uint16_t                 _flags
)
{
    const uint8_t* const at  = &_index->bytes[_index->at];
    const size_t         max = _index->end - _index->at;

    // version 4: the last path cut, then what follows
    if (_index->version == 4u)
    {
        size_t strip = 0u;

        // the cut, within the last path
        if ( (!d_internal_v4_strip(_index, &strip)) ||
             (strip > _index->last.length) )
        {
            return false;
        }

        const uint8_t* const rest = &_index->bytes[_index->at];
        const uint8_t* const nul  = memchr(rest, '\0',
                                           _index->end - _index->at);

        // the rest, to its NUL
        if (!nul)
        {
            return false;
        }

        _index->last.length -= strip;
        (void)d_git_buffer_add(&_index->last, (const char*)rest,
                               (size_t)(nul - rest));
        _index->at = (size_t)(nul - _index->bytes) + 1u;

        return !_index->last.failed;
    }

    const size_t   named = _flags & D_INTERNAL_NAME_MASK;
    const uint8_t* nul   = (named < D_INTERNAL_NAME_MASK)
                           ? ( (named < max) ? &at[named] : NULL )
                           : memchr(&at[D_INTERNAL_NAME_MASK], '\0',
                                    (max > D_INTERNAL_NAME_MASK)
                                    ? (max - D_INTERNAL_NAME_MASK)
                                    : 0u);

    // the path, and its NUL
    if ( (!nul) ||
         (*nul != '\0') )
    {
        return false;
    }

    const size_t length = (size_t)(nul - at);

    _index->last.length = 0u;
    (void)d_git_buffer_add(&_index->last, (const char*)at, length);
    _index->at = _start + ((_index->at - _start + length + 8u) & ~(size_t)7u);

    return ( (!_index->last.failed) &&
             (_index->at <= _index->end) );
}


/*
d_internal_read_entry
  One entry: its stat data, name and flags, then its path, kept.
*/
static bool
d_internal_read_entry(
    struct d_git_repo*        _repo,
    struct d_internal_index*  _index,
    struct d_git_index_entry* _out,
    size_t                    _entry
)
{
    const size_t   hash  = d_git_repo_hash_size(_repo);
    const size_t   start = _index->at;
    const uint8_t* bytes = &_index->bytes[start];

    // the fixed part
    if ((start + 40u + hash + 2u) > _index->end)
    {
        return false;
    }

    memset(_out, 0, sizeof(*_out));
    _out->ctime             = d_internal_be32(&bytes[0]);
    _out->ctime_nanoseconds = d_internal_be32(&bytes[4]);
    _out->mtime             = d_internal_be32(&bytes[8]);
    _out->mtime_nanoseconds = d_internal_be32(&bytes[12]);
    _out->dev               = d_internal_be32(&bytes[16]);
    _out->ino               = d_internal_be32(&bytes[20]);
    _out->mode              = d_internal_be32(&bytes[24]);
    _out->uid               = d_internal_be32(&bytes[28]);
    _out->gid               = d_internal_be32(&bytes[32]);
    _out->size              = d_internal_be32(&bytes[36]);
    memcpy(_out->oid.bytes, &bytes[40], hash);

    const uint16_t flags = d_internal_be16(&bytes[40u + hash]);

    _out->stage        = (flags >> D_INTERNAL_STAGE_SHIFT) & 3u;
    _out->assume_valid = ((flags & D_INTERNAL_VALID) != 0u);
    _index->at         = start + 40u + hash + 2u;

    // version 3's extended flags
    if ((flags & D_INTERNAL_EXTENDED) != 0u)
    {
        // a version that has them, and room for them
        if ( (_index->version < 3u) ||
             ((_index->at + 2u) > _index->end) )
        {
            return false;
        }

        const uint16_t extended = d_internal_be16(&_index->bytes[_index->at]);

        _out->skip_worktree = ((extended & D_INTERNAL_SKIP) != 0u);
        _out->intent_to_add = ((extended & D_INTERNAL_INTENT) != 0u);
        _index->at         += 2u;
    }

    // the path, kept end to end with the others
    if (!d_internal_entry_path(_index, start, flags))
    {
        return false;
    }

    _index->name_at[_entry] = _index->names.length;
    _out->path_length       = _index->last.length;
    (void)d_git_buffer_add(&_index->names, _index->last.text,
                           _index->last.length);
    (void)d_git_buffer_add_char(&_index->names, '\0');

    return !_index->names.failed;
}


/*
d_internal_index_extensions
  The extensions after the entries, each a signature and a length: an
optional one -- its signature's first letter a capital -- passed over; a
sparse index's directories read as entries; and any other, the split
index's among them, refused, as git refuses one it does not know.
*/
static bool
d_internal_index_extensions(
    struct d_git_repo*             _repo,
    const struct d_internal_index* _index
)
{
    size_t at = _index->at;

    // each extension, to the checksum
    while ((at + 8u) <= _index->end)
    {
        const uint8_t* const signature = &_index->bytes[at];
        const uint32_t       length    = d_internal_be32(&signature[4]);

        // one that runs past the checksum
        if ((at + 8u + length) > _index->end)
        {
            return d_git_repo_fail(_repo, "index extension runs past the "
                                          "index", NULL);
        }

        // a required one, other than a sparse index's
        if ( ( (signature[0] < 'A') || (signature[0] > 'Z') ) &&
             (memcmp(signature, "sdir", 4u) != 0) )
        {
            char named[5];

            memcpy(named, signature, 4u);
            named[4] = '\0';

            return d_git_repo_fail(_repo,
                                   "index uses an extension this does not "
                                   "read",
                                   named);
        }

        at += 8u + length;
    }

    return true;
}


/*
d_internal_index_checksum
  Whether the index's checksum holds: the hash of all before it, or, as
index.skipHash writes, none.
*/
static bool
d_internal_index_checksum(
    const struct d_git_repo* _repo,
    const uint8_t*           _bytes,
    size_t                   _length
)
{
    const size_t      hash = d_git_repo_hash_size(_repo);
    uint8_t           computed[D_GIT_HASH_MOST];
    uint8_t           zero[D_GIT_HASH_MOST];
    struct d_git_hash state;

    memset(zero, 0, sizeof(zero));

    // a checksum of zeros is none
    if (memcmp(&_bytes[_length - hash], zero, hash) == 0)
    {
        return true;
    }

    d_git_hash_begin(&state, _repo->hash);
    d_git_hash_feed(&state, _bytes, _length - hash);
    d_git_hash_end(&state, computed);

    return (memcmp(computed, &_bytes[_length - hash], hash) == 0);
}


/*
d_internal_index_header
  The index's header -- "DIRC", a version from 2 to 4, a count -- and its
checksum.
*/
static bool
d_internal_index_header(
    struct d_git_repo*       _repo,
    struct d_internal_index* _index,
    uint32_t*                _out_count
)
{
    const size_t hash = d_git_repo_hash_size(_repo);

    // a header, a checksum, and a version this reads
    if ( (_index->length < (12u + hash)) ||
         (memcmp(_index->bytes, "DIRC", 4u) != 0) )
    {
        return d_git_repo_fail(_repo, "bad index signature", NULL);
    }

    _index->version = d_internal_be32(&_index->bytes[4]);
    *_out_count     = d_internal_be32(&_index->bytes[8]);
    _index->at      = 12u;
    _index->end     = _index->length - hash;

    // a version, and a checksum that holds
    if ( (_index->version < 2u) ||
         (_index->version > 4u) )
    {
        return d_git_repo_fail(_repo, "bad index version", NULL);
    }

    return (d_internal_index_checksum(_repo, _index->bytes, _index->length))
           ? true
           : d_git_repo_fail(_repo, "bad index file sha1 signature", NULL);
}


/*
d_internal_index_entries
  The index's entries, each with its path; then the paths pointed at,
once they are all kept.
*/
static bool
d_internal_index_entries(
    struct d_git_repo*       _repo,
    struct d_internal_index* _index,
    uint32_t                 _count
)
{
    _repo->entries  = calloc((size_t)_count + 1u, sizeof(*_repo->entries));
    _index->name_at = calloc((size_t)_count + 1u, sizeof(*_index->name_at));

    // room for them
    if ( (!_repo->entries) ||
         (!_index->name_at) )
    {
        return d_git_repo_fail(_repo, "out of memory", NULL);
    }

    // each entry
    for (uint32_t at = 0u; at < _count; ++at)
    {
        // one that reads
        if (!d_internal_read_entry(_repo, _index, &_repo->entries[at], at))
        {
            return d_git_repo_fail(_repo, "bad index entry", NULL);
        }
    }

    _repo->entry_count = _count;
    _repo->index_names = d_git_buffer_take(&_index->names);

    // each path, now that none will move
    for (size_t at = 0u; ( (_repo->index_names) && (at < _count) ); ++at)
    {
        _repo->entries[at].path = &_repo->index_names[_index->name_at[at]];
    }

    return (_repo->index_names != NULL)
           ? d_internal_index_extensions(_repo, _index)
           : d_git_repo_fail(_repo, "out of memory", NULL);
}


/*
d_internal_index_read
  The index, read the first time it is wanted: GIT_INDEX_FILE's, or the
repository's; none at all is an index of nothing. Its file's time is kept
for status, which an entry changed in the same second cannot be trusted
by.
*/
static bool
d_internal_index_read(
    struct d_git_repo* _repo
)
{
    // read already, or failed already
    if (_repo->index_read)
    {
        return !_repo->index_failed;
    }

    const char* const named = getenv("GIT_INDEX_FILE");
    char* const       path  = ( (named) && (named[0] != '\0') )
                              ? d_git_path_join("", named)
                              : d_git_path_join(_repo->git_dir, "index");
    struct d_internal_index index;
    struct d_git_file_info  info;
    uint32_t                count = 0u;

    memset(&index, 0, sizeof(index));
    _repo->index_read = true;
    index.bytes       = (path) ? (const uint8_t*)d_git_file_read(path,
                                                                 &index.length)
                               : NULL;
    _repo->index_mtime = ( (path) && (d_git_file_info(path, &info)) )
                         ? info.mtime
                         : 0u;
    free(path);

    // no index is an index of nothing
    if (!index.bytes)
    {
        return true;
    }

    const bool read = ( (d_internal_index_header(_repo, &index, &count)) &&
                        (d_internal_index_entries(_repo, &index, count)) );

    free((void*)index.bytes);
    free(index.name_at);
    d_git_buffer_free(&index.last);
    d_git_buffer_free(&index.names);

    // a failed index is none, and stays failed
    if (!read)
    {
        free(_repo->entries);
        free(_repo->index_names);
        _repo->entries      = NULL;
        _repo->index_names  = NULL;
        _repo->entry_count  = 0u;
        _repo->index_failed = true;
    }

    return read;
}


size_t
d_git_repo_index_count(
    struct d_git_repo* _repo
)
{
    return ( (_repo) && (d_internal_index_read(_repo)) )
           ? _repo->entry_count
           : D_GIT_REPO_NONE;
}


const struct d_git_index_entry*
d_git_repo_index_entry(
    struct d_git_repo* _repo,
    size_t             _index
)
{
    return ( (_repo) &&
             (d_internal_index_read(_repo)) &&
             (_index < _repo->entry_count) )
           ? &_repo->entries[_index]
           : NULL;
}


/*
d_internal_compare_path
  Orders a path and stage against an entry's, as the index orders them:
the paths' bytes, a shorter path first, then the stage.
*/
static int
d_internal_compare_path(
    const char*                     _path,
    size_t                          _length,
    unsigned                        _stage,
    const struct d_git_index_entry* _entry
)
{
    const size_t shorter = (_length < _entry->path_length)
                           ? _length
                           : _entry->path_length;
    const int    order   = memcmp(_path, _entry->path, shorter);

    // the bytes, the lengths, the stage
    if (order != 0)
    {
        return order;
    }
    else if (_length != _entry->path_length)
    {
        return (_length < _entry->path_length) ? -1 : 1;
    }

    return (_stage < _entry->stage) ? -1
         : ( (_stage > _entry->stage) ? 1 : 0 );
}


/*
d_internal_index_position
  Where a path and stage are, or would be, among the entries.
*/
static size_t
d_internal_index_position(
    const struct d_git_repo* _repo,
    const char*              _path,
    size_t                   _length,
    unsigned                 _stage,
    bool*                    _out_found
)
{
    size_t low  = 0u;
    size_t high = _repo->entry_count;

    *_out_found = false;

    // halving the range
    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);
        const int    order  = d_internal_compare_path(_path, _length, _stage,
                                                      &_repo->entries[middle]);

        // the entry, or a half
        if (order == 0)
        {
            *_out_found = true;

            return middle;
        }
        else if (order < 0)
        {
            high = middle;
        }
        else
        {
            low = middle + 1u;
        }
    }

    return low;
}


size_t
d_git_repo_index_find(
    struct d_git_repo* _repo,
    const char*        _path,
    unsigned           _stage
)
{
    bool found = false;

    // an index, and a path
    if ( (!_path) ||
         (d_git_repo_index_count(_repo) == D_GIT_REPO_NONE) )
    {
        return D_GIT_REPO_NONE;
    }

    const size_t at = d_internal_index_position(_repo, _path, strlen(_path),
                                                _stage, &found);

    return (found) ? at : D_GIT_REPO_NONE;
}


//==============================================================================
// 2.  THE WORK TREE'S RULES
//==============================================================================


void
d_git_rules_release(
    struct d_git_repo* _repo
)
{
    // each file kept
    for (size_t at = 0u; at < _repo->rule_count; ++at)
    {
        d_git_text_release(&_repo->rules[at].records);
        free(_repo->rules[at].name);
        free(_repo->rules[at].base);
        free(_repo->rules[at].text);
    }

    free(_repo->rules);
    _repo->rules         = NULL;
    _repo->rule_count    = 0u;
    _repo->rule_capacity = 0u;

    return;
}


// d_internal_from
//   enum: where a rules file's text comes from: a file outside the work
//   tree, links followed; one in it, not a link, or else -- as git falls
//   back -- an ignore file's skip-worktree entry in the index, or an
//   attribute file's entry at stage 0 or 2; or the attribute source's
//   tree.
enum d_internal_from
{
    D_INTERNAL_FROM_FILE = 0,
    D_INTERNAL_FROM_WORK_IGNORE,
    D_INTERNAL_FROM_WORK_ATTRIBUTES,
    D_INTERNAL_FROM_TREE
};

// d_internal_rule_source
//   struct: an ignore or attribute file to read: what git names it, where
//   it is, the base its patterns are relative to, its kind, whether its
//   macros count, and where its text comes from.
struct d_internal_rule_source
{
    const char*          name;
    const char*          path;
    const char*          base;
    enum d_git_text_kind kind;
    bool                 macros;
    enum d_internal_from from;
};


/*
d_internal_rules_find
  A file kept already, by the name git gives it and its kind.
*/
static size_t
d_internal_rules_find(
    const struct d_git_repo* _repo,
    const char*              _name,
    enum d_git_text_kind     _kind
)
{
    // each kept
    for (size_t at = 0u; at < _repo->rule_count; ++at)
    {
        // the name, of the kind
        if ( (_repo->rules[at].records.kind == _kind) &&
             (strcmp(_repo->rules[at].name, _name) == 0) )
        {
            return at;
        }
    }

    return D_GIT_REPO_NONE;
}


/*
d_internal_blob_text
  A blob's bytes, terminated, as an allocation of their own; NULL for an
object that is not a blob.
*/
static char*
d_internal_blob_text(
    struct d_git_repo*      _repo,
    const struct d_git_oid* _oid,
    size_t*                 _out_length
)
{
    struct d_git_object object;
    char*               text = NULL;

    // a blob
    if ( (d_git_repo_read(_repo, _oid, &object)) &&
         (object.type == D_GIT_OBJECT_BLOB) )
    {
        text = malloc(object.size + 1u);

        // room for it
        if (text)
        {
            memcpy(text, object.data, object.size);
            text[object.size] = '\0';
            *_out_length      = object.size;
        }
    }

    d_git_object_release(&object);

    return text;
}


/*
d_internal_index_text
  A rules file's text from the index, as git falls back to it: an ignore
file's entry only when it is skip-worktree; an attribute file's at stage
0, or else at stage 2, as a merge leaves "ours".
*/
static char*
d_internal_index_text(
    struct d_git_repo*   _repo,
    const char*          _name,
    enum d_internal_from _from,
    size_t*              _out_length
)
{
    size_t at = d_git_repo_index_find(_repo, _name, 0u);

    // an attribute file mid-merge: ours
    if ( (at == D_GIT_REPO_NONE) &&
         (_from == D_INTERNAL_FROM_WORK_ATTRIBUTES) )
    {
        at = d_git_repo_index_find(_repo, _name, 2u);
    }

    const struct d_git_index_entry* const entry =
        (at != D_GIT_REPO_NONE) ? d_git_repo_index_entry(_repo, at) : NULL;

    // an entry git reads
    if ( (!entry) ||
         ( (_from == D_INTERNAL_FROM_WORK_IGNORE) &&
           (!entry->skip_worktree) ) )
    {
        return NULL;
    }

    return d_internal_blob_text(_repo, &entry->oid, _out_length);
}


/*
d_internal_rules_text
  A rules file's text, NULL when there is none: the file's, a link in the
work tree passed over, as git opens them without following; else the
index's, or the attribute source tree's.
*/
static char*
d_internal_rules_text(
    struct d_git_repo*                   _repo,
    const struct d_internal_rule_source* _source,
    size_t*                              _out_length
)
{
    struct d_git_oid       oid;
    struct d_git_file_info info;
    uint32_t               mode = 0u;
    char*                  text = NULL;

    *_out_length = 0u;

    // the tree's
    if (_source->from == D_INTERNAL_FROM_TREE)
    {
        return (d_git_tree_find(_repo, &_repo->attr_tree, _source->name,
                                &oid, &mode))
               ? d_internal_blob_text(_repo, &oid, _out_length)
               : NULL;
    }

    // a file there -- in the work tree, not a link
    if ( (_source->path) &&
         ( (_source->from == D_INTERNAL_FROM_FILE) ||
           ( (d_git_file_info(_source->path, &info)) &&
             (info.kind != D_GIT_FILE_LINK) ) ) )
    {
        text = d_git_file_read(_source->path, _out_length);
    }

    // none in the work tree: the index's
    if ( (!text) &&
         (_source->from != D_INTERNAL_FROM_FILE) )
    {
        text = d_internal_index_text(_repo, _source->name, _source->from,
                                     _out_length);
    }

    return text;
}


/*
d_internal_rules_keep
  A file read and kept: its text, if it is there, and its records; one not
there is kept too, as there being none.
*/
static size_t
d_internal_rules_keep(
    struct d_git_repo*                   _repo,
    const struct d_internal_rule_source* _source
)
{
    const size_t found = d_internal_rules_find(_repo, _source->name,
                                               _source->kind);

    // kept already
    if (found != D_GIT_REPO_NONE)
    {
        return found;
    }

    // room for one more
    if (_repo->rule_count == _repo->rule_capacity)
    {
        const size_t              wanted = (_repo->rule_capacity > 0u)
                                           ? (_repo->rule_capacity * 2u)
                                           : 16u;
        struct d_git_rules* const grown  = realloc(_repo->rules,
                                                   wanted * sizeof(*grown));

        // memory that ran out
        if (!grown)
        {
            return D_GIT_REPO_NONE;
        }

        _repo->rules         = grown;
        _repo->rule_capacity = wanted;
    }

    struct d_git_rules* const rules  = &_repo->rules[_repo->rule_count];
    size_t                    length = 0u;
    char                      problem[64];

    memset(rules, 0, sizeof(*rules));
    rules->name    = d_git_path_join("", _source->name);
    rules->base    = d_git_path_join("", _source->base);
    rules->text    = d_internal_rules_text(_repo, _source, &length);
    rules->present = (rules->text != NULL);
    rules->macros  = _source->macros;

    // the names kept, and the text read
    if ( (!rules->name) ||
         (!rules->base) ||
         (!d_git_text_read(&rules->records,
                           (rules->text) ? rules->text : "",
                           length,
                           _source->kind,
                           problem,
                           sizeof(problem))) )
    {
        d_git_text_release(&rules->records);
        free(rules->name);
        free(rules->base);
        free(rules->text);

        return D_GIT_REPO_NONE;
    }

    return _repo->rule_count++;
}


/*
d_internal_home
  The user's home directory: HOME, or USERPROFILE where there is no HOME.
*/
static const char*
d_internal_home(void)
{
    const char* const home = getenv("HOME");

    return ( (home) && (home[0] != '\0') ) ? home : getenv("USERPROFILE");
}


/*
d_internal_user_file
  A user's file of rules: what a setting names, `~/` expanded, or git's
own under XDG_CONFIG_HOME, or ~/.config -- git/ignore, git/attributes.
*/
static char*
d_internal_user_file(
    const struct d_git_repo* _repo,
    const char*              _key,
    const char*              _own
)
{
    bool              set   = false;
    const char* const value = d_git_repo_config(_repo, _key, &set);
    const char* const home  = d_internal_home();

    // the setting's, `~/` expanded
    if ( (set) &&
         (value) )
    {
        return ( (value[0] == '~') && (value[1] == '/') && (home) )
               ? d_git_path_join(home, &value[2])
               : d_git_path_join("", value);
    }

    const char* const xdg  = getenv("XDG_CONFIG_HOME");
    char* const       base = ( (xdg) && (xdg[0] != '\0') )
                             ? d_git_path_join("", xdg)
                             : ( (home) ? d_git_path_join(home, ".config")
                                        : NULL );
    char* const       path = (base) ? d_git_path_join(base, _own) : NULL;

    free(base);

    return path;
}


/*
d_internal_under_work_tree
  A path of the repository's directory as git names it from the top of the
work tree -- .git/info/exclude -- or whole when it is elsewhere.
*/
static char*
d_internal_under_work_tree(
    const struct d_git_repo* _repo,
    const char*              _path
)
{
    const size_t length = (_repo->work_tree) ? strlen(_repo->work_tree) : 0u;

    // within the work tree, from its top
    if ( (length > 0u) &&
         (strncmp(_path, _repo->work_tree, length) == 0) &&
         (_path[length] == '/') )
    {
        return d_git_path_join("", &_path[length + 1u]);
    }

    return d_git_path_join("", _path);
}


// d_internal_scopes
//   struct: the scopes of a lookup, and the files they are, lowest first.
struct d_internal_scopes
{
    struct d_git_text_scope* items;
    size_t*                  rules;
    size_t                   count;
    size_t                   capacity;
};


/*
d_internal_scope_add
  A file as a scope of a lookup, if it is there.
*/
static bool
d_internal_scope_add(
    struct d_git_repo*                   _repo,
    struct d_internal_scopes*            _scopes,
    const struct d_internal_rule_source* _source
)
{
    const size_t rule = d_internal_rules_keep(_repo, _source);

    // memory that ran out, or no file there
    if (rule == D_GIT_REPO_NONE)
    {
        return false;
    }
    else if (!_repo->rules[rule].present)
    {
        return true;
    }

    // room for one more
    if (_scopes->count == _scopes->capacity)
    {
        const size_t                   wanted = (_scopes->capacity > 0u)
                                                ? (_scopes->capacity * 2u)
                                                : 16u;
        struct d_git_text_scope* const items  =
            realloc(_scopes->items, wanted * sizeof(*items));
        size_t* const                  rules  =
            (items) ? realloc(_scopes->rules, wanted * sizeof(*rules)) : NULL;

        // memory that ran out
        if ( (!items) ||
             (!rules) )
        {
            _scopes->items = (items) ? items : _scopes->items;

            return false;
        }

        _scopes->items    = items;
        _scopes->rules    = rules;
        _scopes->capacity = wanted;
    }

    _scopes->rules[_scopes->count]        = rule;
    _scopes->items[_scopes->count].text   = &_repo->rules[rule].records;
    _scopes->items[_scopes->count].base   = _repo->rules[rule].base;
    _scopes->items[_scopes->count].macros = _source->macros;
    ++_scopes->count;

    return true;
}


/*
d_internal_scopes_point
  The scopes pointed at their files' records where they are now: keeping
another file may have moved them all.
*/
static void
d_internal_scopes_point(
    const struct d_git_repo*  _repo,
    struct d_internal_scopes* _scopes
)
{
    // each scope, at its file's records
    for (size_t at = 0u; at < _scopes->count; ++at)
    {
        _scopes->items[at].text = &_repo->rules[_scopes->rules[at]].records;
        _scopes->items[at].base = _repo->rules[_scopes->rules[at]].base;
    }

    return;
}


/*
d_internal_scope_file
  A file outside the work tree as a scope: its path, named as git names
it.
*/
static bool
d_internal_scope_file(
    struct d_git_repo*        _repo,
    struct d_internal_scopes* _scopes,
    const char*               _path,
    enum d_git_text_kind      _kind
)
{
    // no file named
    if (!_path)
    {
        return true;
    }

    char* const                   name   = d_internal_under_work_tree(_repo,
                                                                      _path);
    const struct d_internal_rule_source source = { name, _path, "", _kind,
                                                   true,
                                                   D_INTERNAL_FROM_FILE };
    const bool                    added  = ( (name) &&
                                             (d_internal_scope_add(_repo,
                                                                   _scopes,
                                                                   &source)) );

    free(name);

    return added;
}


/*
d_internal_scope_directories
  Each directory's file down to a path's own directory, from the top, as
scopes: .gitignore or .gitattributes, its base the directory; only the
top's macros count. The files are the work tree's, or the attribute
source tree's.
*/
static bool
d_internal_scope_directories(
    struct d_git_repo*        _repo,
    struct d_internal_scopes* _scopes,
    const char*               _path,
    size_t                    _length,
    enum d_git_text_kind      _kind
)
{
    const char* const          file = (_kind == D_GIT_TEXT_IGNORE)
                                      ? ".gitignore"
                                      : ".gitattributes";
    const enum d_internal_from from = (_kind == D_GIT_TEXT_IGNORE)
                                      ? D_INTERNAL_FROM_WORK_IGNORE
                                      : (_repo->attr_from_tree)
                                        ? D_INTERNAL_FROM_TREE
                                        : D_INTERNAL_FROM_WORK_ATTRIBUTES;
    struct d_git_buffer        base;
    bool                       added = true;

    memset(&base, 0, sizeof(base));

    // the top, then each directory down to the path's
    for (size_t at = 0u; ( (added) && (at <= _length) ); ++at)
    {
        // a directory ends where a `/` is, the top at the start
        if ( (at > 0u) &&
             ( (at == _length) || (_path[at] != '/') ) )
        {
            continue;
        }

        base.length = 0u;
        (void)d_git_buffer_add(&base, _path, at);

        char* const name  = d_git_path_join((base.text) ? base.text : "",
                                            file);
        char* const where = ( (name) && (_repo->work_tree) )
                            ? d_git_path_join(_repo->work_tree, name)
                            : NULL;
        const struct d_internal_rule_source source =
            { name, where, (base.text) ? base.text : "", _kind, (at == 0u),
              from };

        added = ( (name) &&
                  ( (where) || (from == D_INTERNAL_FROM_TREE) ) &&
                  (!base.failed) &&
                  (d_internal_scope_add(_repo, _scopes, &source)) );
        free(name);
        free(where);
    }

    d_git_buffer_free(&base);

    return added;
}


/*
d_git_repo_ignored
  Long as the scopes are gathered, in git's order, before the lookup.
*/
bool
d_git_repo_ignored(
    struct d_git_repo*         _repo,
    const char*                _path,
    struct d_git_ignore_match* _out
)
{
    struct d_internal_scopes scopes;
    struct d_git_text_match  match;

    memset(_out, 0, sizeof(*_out));
    memset(&scopes, 0, sizeof(scopes));

    // a work tree, and a path
    if ( (!_repo) ||
         (!_repo->work_tree) ||
         (!_path) )
    {
        return false;
    }

    size_t length = strlen(_path);
    bool   dir    = ( (length > 0u) && (_path[length - 1u] == '/') );

    // a trailing slash says a directory; else the work tree does
    while ( (length > 0u) &&
            (_path[length - 1u] == '/') )
    {
        --length;
    }

    char* const where = d_git_path_join(_repo->work_tree, _path);
    struct d_git_file_info info;

    dir = ( (dir) ||
            ( (where) && (d_git_file_info(where, &info)) &&
              (info.kind == D_GIT_FILE_DIRECTORY) ) );
    free(where);

    char* const exclude  = d_internal_user_file(_repo, "core.excludesfile",
                                                "git/ignore");
    char* const info_dir = d_git_path_join(_repo->common_dir, "info/exclude");
    const bool  scoped   =
        ( (info_dir) &&
          (d_internal_scope_file(_repo, &scopes, exclude,
                                 D_GIT_TEXT_IGNORE)) &&
          (d_internal_scope_file(_repo, &scopes, info_dir,
                                 D_GIT_TEXT_IGNORE)) &&
          (d_internal_scope_directories(_repo, &scopes, _path, length,
                                        D_GIT_TEXT_IGNORE)) );
    const struct d_git_text_query query = { _path, length, dir,
                                            d_git_repo_flag(_repo,
                                                            "core.ignorecase",
                                                            false) };

    d_internal_scopes_point(_repo, &scopes);

    const bool  decided  = ( (scoped) &&
                             (d_git_text_ignored(scopes.items, scopes.count,
                                                 &query, &match)) );

    free(exclude);
    free(info_dir);

    // what decided, borrowed from the rules kept
    if (decided)
    {
        const struct d_git_rules* const rules =
            &_repo->rules[scopes.rules[match.scope]];
        const struct d_git_text_pattern* const pattern =
            &rules->records.patterns[match.pattern];

        _out->file           = rules->name;
        _out->line           = pattern->line + 1u;
        _out->pattern        = &rules->records.text[pattern->start];
        _out->pattern_length = pattern->end - pattern->start;
        _out->ignored        = match.ignored;
    }

    free(scopes.items);
    free(scopes.rules);

    return decided;
}


/*
d_internal_attribute_source
  The tree attributes are read from in place of the work tree, as git's
compute_default_attr_source picks it: GIT_ATTR_SOURCE's, attr.tree's or --
for a bare repository -- HEAD's, the last two passed over when they name
no tree; picked once. False, said, for a GIT_ATTR_SOURCE that names none.
*/
static bool
d_internal_attribute_source(
    struct d_git_repo* _repo
)
{
    // picked already
    if (_repo->attr_source_read)
    {
        return (_repo->attr_source_bad)
               ? d_git_repo_fail(_repo, "bad GIT_ATTR_SOURCE", NULL)
               : true;
    }

    const char* const env   = getenv("GIT_ATTR_SOURCE");
    bool              set   = false;
    const char* const value = d_git_repo_config(_repo, "attr.tree", &set);
    const char* const name  = (env) ? env
                                    : ( (set) && (value) )
                                      ? value
                                      : (!_repo->work_tree) ? "HEAD" : NULL;
    char              hex[(2u * D_GIT_OID_MOST) + 8u];

    _repo->attr_source_read = true;

    // a tree-ish named, and the tree it peels to
    if ( (name) &&
         (d_git_repo_rev(_repo, name, &_repo->attr_tree)) )
    {
        d_git_repo_hex(_repo, &_repo->attr_tree, hex);
        (void)strcat(hex, "^{tree}");
        _repo->attr_from_tree = d_git_repo_rev(_repo, hex, &_repo->attr_tree);
    }

    _repo->attr_source_bad = ( (env) && (!_repo->attr_from_tree) );
    _repo->problem[0]      = '\0';

    return (_repo->attr_source_bad)
           ? d_git_repo_fail(_repo, "bad GIT_ATTR_SOURCE", env)
           : true;
}


/*
d_internal_attribute_scopes
  The scopes of an attribute lookup, lowest first: the system's file --
unless GIT_ATTR_NOSYSTEM -- the user's, each directory's down to the
path's, and info/attributes.
*/
static bool
d_internal_attribute_scopes(
    struct d_git_repo*        _repo,
    struct d_internal_scopes* _scopes,
    const char*               _path,
    size_t                    _length
)
{
    const char* const nosystem = getenv("GIT_ATTR_NOSYSTEM");
    bool              skip     = false;

    // a setting git reads as a boolean
    if (nosystem)
    {
        (void)d_git_text_bool(nosystem, &skip);
    }

    char* const user = d_internal_user_file(_repo, "core.attributesfile",
                                            "git/attributes");
    char* const info = d_git_path_join(_repo->common_dir, "info/attributes");
    const enum d_git_text_kind kind  = D_GIT_TEXT_ATTRIBUTES;
    const bool                 added =
        ( (info) &&
          (d_internal_attribute_source(_repo)) &&
          ( (skip) ||
            (d_internal_scope_file(_repo, _scopes, "/etc/gitattributes",
                                   kind)) ) &&
          (d_internal_scope_file(_repo, _scopes, user, kind)) &&
          ( ( (!_repo->work_tree) &&
              (!_repo->attr_from_tree) ) ||
            (d_internal_scope_directories(_repo, _scopes, _path, _length,
                                          kind)) ) &&
          (d_internal_scope_file(_repo, _scopes, info, kind)) );

    free(user);
    free(info);

    return added;
}


bool
d_git_repo_attributes(
    struct d_git_repo*        _repo,
    const char*               _path,
    struct d_git_text_values* _out
)
{
    struct d_internal_scopes scopes;

    memset(_out, 0, sizeof(*_out));
    memset(&scopes, 0, sizeof(scopes));

    // a repository, and a path
    if ( (!_repo) ||
         (!_path) )
    {
        return false;
    }

    size_t     length = strlen(_path);
    const bool dir    = ( (length > 0u) && (_path[length - 1u] == '/') );

    // a trailing slash says a directory
    while ( (length > 0u) &&
            (_path[length - 1u] == '/') )
    {
        --length;
    }

    const struct d_git_text_query query = { _path, length, dir,
                                            d_git_repo_flag(_repo,
                                                            "core.ignorecase",
                                                            false) };
    const bool scoped = d_internal_attribute_scopes(_repo, &scopes, _path,
                                                    length);

    d_internal_scopes_point(_repo, &scopes);

    const bool found = ( (scoped) &&
                         (d_git_text_attributes(scopes.items, scopes.count,
                                                &query, _out)) );

    free(scopes.items);
    free(scopes.rules);

    return found;
}


//==============================================================================
// 3.  CLEANING
//==============================================================================


// d_internal_crlf
//   enum: how a file's line endings are made clean, as convert.c names
//   the actions.
enum d_internal_crlf
{
    D_INTERNAL_CRLF_UNDEFINED = 0,
    D_INTERNAL_CRLF_BINARY,
    D_INTERNAL_CRLF_TEXT,
    D_INTERNAL_CRLF_TEXT_INPUT,
    D_INTERNAL_CRLF_TEXT_CRLF,
    D_INTERNAL_CRLF_AUTO,
    D_INTERNAL_CRLF_AUTO_INPUT,
    D_INTERNAL_CRLF_AUTO_CRLF
};

// d_internal_stats
//   struct: what convert.c's gather_stats counts of content.
struct d_internal_stats
{
    size_t nul;
    size_t lonecr;
    size_t lonelf;
    size_t crlf;
    size_t printable;
    size_t nonprintable;
};


/*
d_internal_gather
  The counts git's gather_stats makes: `\r\n` pairs, a `\r` or `\n` alone,
NULs, and bytes printable or not -- backspace, tab, escape and form feed
printable -- a last ^Z not counted.
*/
static void
d_internal_gather(
    const uint8_t*           _data,
    size_t                   _size,
    struct d_internal_stats* _out
)
{
    memset(_out, 0, sizeof(*_out));

    // each byte
    for (size_t at = 0u; at < _size; ++at)
    {
        const uint8_t c = _data[at];

        // a `\r`, alone or before `\n`; a `\n`; a control; any other
        if (c == '\r')
        {
            const bool pair = ( ((at + 1u) < _size) &&
                                (_data[at + 1u] == '\n') );

            _out->crlf   += (pair) ? 1u : 0u;
            _out->lonecr += (pair) ? 0u : 1u;
            at           += (pair) ? 1u : 0u;
        }
        else if (c == '\n')
        {
            ++_out->lonelf;
        }
        else if ( (c == 127u) ||
                  ( (c < 32u) &&
                    (c != '\b') && (c != '\t') && (c != 27u) && (c != 12u) ) )
        {
            _out->nul += (c == 0u) ? 1u : 0u;
            ++_out->nonprintable;
        }
        else
        {
            ++_out->printable;
        }
    }

    // a last ^Z is not counted
    if ( (_size > 0u) &&
         (_data[_size - 1u] == 26u) )
    {
        --_out->nonprintable;
    }

    return;
}


/*
d_internal_is_binary
  Whether content is binary, as convert_is_binary decides: a lone `\r`, a
NUL, or more than one byte in 128 not printable.
*/
static bool
d_internal_is_binary(
    const struct d_internal_stats* _stats
)
{
    return ( (_stats->lonecr > 0u) ||
             (_stats->nul > 0u) ||
             ((_stats->printable >> 7) < _stats->nonprintable) );
}


/*
d_internal_state_of
  An attribute's state among a path's, and its value: unspecified, and
NULL, when none is said.
*/
static enum d_git_text_state
d_internal_state_of(
    const struct d_git_text_values* _values,
    const char*                     _name,
    char*                           _value,
    size_t                          _capacity
)
{
    const struct d_git_text_value* const value = d_git_text_value_of(_values,
                                                                     _name);

    _value[0] = '\0';

    // none said
    if (!value)
    {
        return D_GIT_TEXT_UNSPECIFIED;
    }

    // a value, as much as fits
    if (value->state == D_GIT_TEXT_VALUE)
    {
        const size_t length = (value->value_length < _capacity)
                              ? value->value_length
                              : (_capacity - 1u);

        memcpy(_value, value->value, length);
        _value[length] = '\0';
    }

    return value->state;
}


/*
d_internal_crlf_of
  An attribute as convert.c's git_path_check_crlf reads it: set text,
unset binary, "input", "auto", and anything else undefined.
*/
static enum d_internal_crlf
d_internal_crlf_of(
    enum d_git_text_state _state,
    const char*           _value
)
{
    // each state, and the values git knows
    if (_state == D_GIT_TEXT_SET)
    {
        return D_INTERNAL_CRLF_TEXT;
    }
    else if (_state == D_GIT_TEXT_UNSET)
    {
        return D_INTERNAL_CRLF_BINARY;
    }
    else if ( (_state == D_GIT_TEXT_VALUE) &&
              (strcmp(_value, "input") == 0) )
    {
        return D_INTERNAL_CRLF_TEXT_INPUT;
    }
    else if ( (_state == D_GIT_TEXT_VALUE) &&
              (strcmp(_value, "auto") == 0) )
    {
        return D_INTERNAL_CRLF_AUTO;
    }

    return D_INTERNAL_CRLF_UNDEFINED;
}


/*
d_internal_autocrlf
  core.autocrlf: 1 true, -1 input, 0 false.
*/
static int
d_internal_autocrlf(
    const struct d_git_repo* _repo
)
{
    bool              set   = false;
    bool              flag  = false;
    const char* const value = d_git_repo_config(_repo, "core.autocrlf", &set);

    // input, or a boolean
    if ( (set) &&
         (value) &&
         (strcmp(value, "input") == 0) )
    {
        return -1;
    }

    return ( (set) && (d_git_text_bool(value, &flag)) && (flag) ) ? 1 : 0;
}


/*
d_internal_crlf_action
  How a path's line endings are made clean, as convert_attrs decides: the
text attribute, or the old crlf; eol's choice of input or CRLF; then
text's by core.autocrlf or core.eol, and an undefined one by
core.autocrlf alone.
*/
static enum d_internal_crlf
d_internal_crlf_action(
    const struct d_git_repo*        _repo,
    const struct d_git_text_values* _values
)
{
    char                 value[16];
    enum d_internal_crlf action = d_internal_crlf_of(
                                      d_internal_state_of(_values, "text",
                                                          value,
                                                          sizeof(value)),
                                      value);

    // the old crlf attribute, when text says nothing
    if (action == D_INTERNAL_CRLF_UNDEFINED)
    {
        const enum d_git_text_state state = d_internal_state_of(_values,
                                                                "crlf",
                                                                value,
                                                                sizeof(value));

        action = d_internal_crlf_of(state, value);
    }

    const enum d_git_text_state eol = d_internal_state_of(_values, "eol",
                                                          value,
                                                          sizeof(value));
    const bool lf   = ( (eol == D_GIT_TEXT_VALUE) &&
                        (strcmp(value, "lf") == 0) );
    const bool crlf = ( (eol == D_GIT_TEXT_VALUE) &&
                        (strcmp(value, "crlf") == 0) );
    const int  auto_crlf = d_internal_autocrlf(_repo);

    // eol's choice, but not for a binary file
    if ( (action != D_INTERNAL_CRLF_BINARY) &&
         ( (lf) || (crlf) ) )
    {
        action = (action == D_INTERNAL_CRLF_AUTO)
                 ? ( (lf) ? D_INTERNAL_CRLF_AUTO_INPUT
                          : D_INTERNAL_CRLF_AUTO_CRLF )
                 : ( (lf) ? D_INTERNAL_CRLF_TEXT_INPUT
                          : D_INTERNAL_CRLF_TEXT_CRLF );
    }

    // text by autocrlf and core.eol; undefined by autocrlf alone
    if (action == D_INTERNAL_CRLF_TEXT)
    {
        bool              set = false;
        const char* const eol_value = d_git_repo_config(_repo, "core.eol",
                                                        &set);
        const bool        to_crlf   = ( (auto_crlf > 0) ||
                                        ( (auto_crlf == 0) && (set) &&
                                          (eol_value) &&
                                          (strcmp(eol_value, "crlf") == 0) ) );

        action = (to_crlf) ? D_INTERNAL_CRLF_TEXT_CRLF
                           : D_INTERNAL_CRLF_TEXT_INPUT;
    }
    else if (action == D_INTERNAL_CRLF_UNDEFINED)
    {
        action = (auto_crlf > 0) ? D_INTERNAL_CRLF_AUTO_CRLF
               : (auto_crlf < 0) ? D_INTERNAL_CRLF_AUTO_INPUT
               : D_INTERNAL_CRLF_BINARY;
    }

    return action;
}


/*
d_internal_crlf_in_index
  Whether the index's version of a path is text with a `\r\n` in it, which
an automatic conversion leaves alone, as git's has_crlf_in_index finds it.
*/
static bool
d_internal_crlf_in_index(
    struct d_git_repo* _repo,
    const char*        _path
)
{
    const size_t        at = d_git_repo_index_find(_repo, _path, 0u);
    struct d_git_object blob;

    // an entry, its blob read
    if ( (at == D_GIT_REPO_NONE) ||
         (!d_git_repo_read(_repo, &_repo->entries[at].oid, &blob)) )
    {
        _repo->problem[0] = '\0';

        return false;
    }

    struct d_internal_stats stats;
    bool                    has = false;

    // a `\r` at all, then the counts
    if (memchr(blob.data, '\r', blob.size))
    {
        d_internal_gather(blob.data, blob.size, &stats);
        has = ( (!d_internal_is_binary(&stats)) &&
                (stats.crlf > 0u) );
    }

    d_git_object_release(&blob);

    return has;
}


/*
d_internal_crlf_to_git
  Line endings made clean, as convert.c's crlf_to_git makes them: none for
binary content, or content with no `\r\n`; for an automatic conversion,
none for content that looks binary or whose index version has `\r\n`,
else every `\r` gone; otherwise each `\r` before a `\n` gone.
*/
static void
d_internal_crlf_to_git(
    struct d_git_repo*   _repo,
    const char*          _path,
    enum d_internal_crlf _action,
    uint8_t*             _data,
    size_t*              _size
)
{
    struct d_internal_stats stats;

    // binary, or nothing
    if ( (_action == D_INTERNAL_CRLF_BINARY) ||
         (*_size == 0u) )
    {
        return;
    }

    d_internal_gather(_data, *_size, &stats);

    const bool automatic = ( (_action == D_INTERNAL_CRLF_AUTO) ||
                             (_action == D_INTERNAL_CRLF_AUTO_INPUT) ||
                             (_action == D_INTERNAL_CRLF_AUTO_CRLF) );

    // nothing to change, or an automatic change git does not make
    if ( (stats.crlf == 0u) ||
         ( (automatic) &&
           ( (d_internal_is_binary(&stats)) ||
             (d_internal_crlf_in_index(_repo, _path)) ) ) )
    {
        return;
    }

    size_t kept = 0u;

    // each byte, the `\r`s that go dropped
    for (size_t at = 0u; at < *_size; ++at)
    {
        const bool drop = ( (_data[at] == '\r') &&
                            ( (automatic) ||
                              ( ((at + 1u) < *_size) &&
                                (_data[at + 1u] == '\n') ) ) );

        // kept
        if (!drop)
        {
            _data[kept++] = _data[at];
        }
    }

    *_size = kept;

    return;
}


/*
d_internal_ident_to_git
  $Id: ...$ made $Id$ again, as convert.c's ident_to_git makes it, for a
path with the ident attribute: each from its `$Id:` to the next `$` on the
same line.
*/
static void
d_internal_ident_to_git(
    uint8_t* _data,
    size_t*  _size
)
{
    size_t from = 0u;
    size_t to   = 0u;

    // each byte, an $Id: ...$ cut to $Id$
    while (from < *_size)
    {
        const uint8_t c = _data[from];

        _data[to++] = c;
        ++from;

        // a `$` that begins an $Id: ...$
        if ( (c != '$') ||
             ((from + 3u) >= *_size) ||
             (memcmp(&_data[from], "Id:", 3u) != 0) )
        {
            continue;
        }

        const uint8_t* const close = memchr(&_data[from + 3u], '$',
                                            *_size - from - 3u);

        // a closing `$` on the same line
        if ( (!close) ||
             (memchr(&_data[from + 3u], '\n',
                     (size_t)(close - &_data[from + 3u]))) )
        {
            continue;
        }

        memcpy(&_data[to], "Id$", 3u);
        to  += 3u;
        from = (size_t)(close - _data) + 1u;
    }

    *_size = to;

    return;
}


/*
d_internal_filtered
  Whether a path's filter attribute names a driver with a command git
would clean it with -- filter.<name>.clean or .process -- or a
working-tree-encoding is set: content git would change by a program, or
re-encode, which is not done here.
*/
static bool
d_internal_filtered(
    const struct d_git_repo*        _repo,
    const struct d_git_text_values* _values
)
{
    char value[128];

    // an encoding git would undo
    if (d_internal_state_of(_values, "working-tree-encoding", value,
                            sizeof(value)) == D_GIT_TEXT_VALUE)
    {
        return true;
    }

    // no filter named
    if (d_internal_state_of(_values, "filter", value, sizeof(value))
            != D_GIT_TEXT_VALUE)
    {
        return false;
    }

    struct d_git_buffer key;
    bool                set   = false;
    bool                found = false;

    memset(&key, 0, sizeof(key));
    (void)d_git_buffer_add_text(&key, "filter.");
    (void)d_git_buffer_add_text(&key, value);
    (void)d_git_buffer_add_text(&key, ".clean");
    (void)d_git_repo_config(_repo, key.text, &set);
    found = set;
    key.length -= 5u;
    (void)d_git_buffer_add_text(&key, "process");
    (void)d_git_repo_config(_repo, key.text, &set);
    found = ( (found) || (set) );
    d_git_buffer_free(&key);

    return found;
}


/*
d_internal_clean
  Content made clean as git add would make it: its line endings, then its
$Id$; false when a filter or an encoding would clean it, which this does
not run.
*/
static bool
d_internal_clean(
    struct d_git_repo* _repo,
    const char*        _path,
    uint8_t*           _data,
    size_t*            _size
)
{
    struct d_git_text_values values;

    // the path's attributes
    if (!d_git_repo_attributes(_repo, _path, &values))
    {
        d_git_text_values_release(&values);

        return true;
    }

    char value[8];

    // a filter, or an encoding, cleans it otherwise
    if (d_internal_filtered(_repo, &values))
    {
        d_git_text_values_release(&values);

        return false;
    }

    d_internal_crlf_to_git(_repo, _path,
                           d_internal_crlf_action(_repo, &values),
                           _data, _size);

    // $Id$, for ident
    if (d_internal_state_of(&values, "ident", value, sizeof(value))
            == D_GIT_TEXT_SET)
    {
        d_internal_ident_to_git(_data, _size);
    }

    d_git_text_values_release(&values);

    return true;
}


//==============================================================================
// 4.  STATUS
//==============================================================================


// D_INTERNAL_TYPE_MASK ... D_INTERNAL_GITLINK
//   constant: the type bits of a mode, and the types git keeps.
#define D_INTERNAL_TYPE_MASK 0170000u
#define D_INTERNAL_REGULAR   0100000u
#define D_INTERNAL_SYMLINK   0120000u
#define D_INTERNAL_GITLINK   0160000u

// d_internal_tree_file
//   struct: one file of HEAD's tree, flattened: its path, its mode and
//   object.
struct d_internal_tree_file
{
    char*            path;
    uint32_t         mode;
    struct d_git_oid oid;
};

// d_internal_tree_files
//   struct: HEAD's tree, flattened, in the index's order.
struct d_internal_tree_files
{
    struct d_internal_tree_file* items;
    size_t                       count;
    size_t                       capacity;
};


/*
d_internal_tree_files_free
  Lets go of a flattened tree.
*/
static void
d_internal_tree_files_free(
    struct d_internal_tree_files* _files
)
{
    // each path
    for (size_t at = 0u; at < _files->count; ++at)
    {
        free(_files->items[at].path);
    }

    free(_files->items);
    memset(_files, 0, sizeof(*_files));

    return;
}


/*
d_internal_tree_add
  One file more of a flattened tree.
*/
static bool
d_internal_tree_add(
    struct d_internal_tree_files* _files,
    const char*                   _prefix,
    const struct d_git_tree_entry* _entry
)
{
    // room for one more
    if (_files->count == _files->capacity)
    {
        const size_t                       wanted = (_files->capacity > 0u)
                                                    ? (_files->capacity * 2u)
                                                    : 256u;
        struct d_internal_tree_file* const grown  =
            realloc(_files->items, wanted * sizeof(*grown));

        // memory that ran out
        if (!grown)
        {
            return false;
        }

        _files->items    = grown;
        _files->capacity = wanted;
    }

    struct d_git_buffer path;

    memset(&path, 0, sizeof(path));
    (void)d_git_buffer_add_text(&path, _prefix);
    (void)d_git_buffer_add(&path, _entry->name, _entry->name_length);
    _files->items[_files->count].path = d_git_buffer_take(&path);
    _files->items[_files->count].mode = _entry->mode;
    _files->items[_files->count].oid  = _entry->oid;

    return (_files->items[_files->count++].path != NULL);
}


/*
d_internal_flatten
  A tree's files, every subtree walked, each path from the top.
*/
static bool
d_internal_flatten(
    struct d_git_repo*            _repo,
    const struct d_git_oid*       _tree,
    const char*                   _prefix,
    struct d_internal_tree_files* _out
)
{
    struct d_git_object     tree;
    struct d_git_tree_entry entry;
    size_t                  at   = 0u;
    bool                    went = true;

    // the tree
    if ( (!d_git_repo_read(_repo, _tree, &tree)) ||
         (tree.type != D_GIT_OBJECT_TREE) )
    {
        d_git_object_release(&tree);

        return false;
    }

    // each entry: a subtree walked, a file kept
    while ( (went) &&
            (d_git_repo_tree_next(_repo, &tree, &at, &entry)) )
    {
        // a subtree
        if (entry.mode == 040000u)
        {
            struct d_git_buffer prefix;

            memset(&prefix, 0, sizeof(prefix));
            (void)d_git_buffer_add_text(&prefix, _prefix);
            (void)d_git_buffer_add(&prefix, entry.name, entry.name_length);
            (void)d_git_buffer_add_char(&prefix, '/');
            went = ( (!prefix.failed) &&
                     (d_internal_flatten(_repo, &entry.oid, prefix.text,
                                         _out)) );
            d_git_buffer_free(&prefix);

            continue;
        }

        went = d_internal_tree_add(_out, _prefix, &entry);
    }

    d_git_object_release(&tree);

    return went;
}


/*
d_internal_compare_files
  Orders a flattened tree's files by path, as the index orders them.
*/
static int
d_internal_compare_files(
    const void* _left,
    const void* _right
)
{
    const struct d_internal_tree_file* const left  = _left;
    const struct d_internal_tree_file* const right = _right;

    return strcmp(left->path, right->path);
}


/*
d_internal_head_files
  HEAD's tree, flattened and ordered; none for a branch with no commit
yet.
*/
static bool
d_internal_head_files(
    struct d_git_repo*            _repo,
    struct d_internal_tree_files* _out
)
{
    struct d_git_oid    head;
    struct d_git_object object;
    struct d_git_commit commit;

    memset(_out, 0, sizeof(*_out));

    // no commit yet: an empty tree
    if (!d_git_repo_resolve(_repo, "HEAD", &head, NULL))
    {
        _repo->problem[0] = '\0';

        return true;
    }

    // the commit's tree
    if ( (!d_git_repo_read(_repo, &head, &object)) ||
         (!d_git_repo_parse_commit(_repo, &object, &commit)) )
    {
        d_git_object_release(&object);

        return d_git_repo_fail(_repo, "cannot read HEAD's commit", NULL);
    }

    const struct d_git_oid tree = commit.tree;

    d_git_commit_release(&commit);
    d_git_object_release(&object);

    // flattened, ordered
    if (!d_internal_flatten(_repo, &tree, "", _out))
    {
        return d_git_repo_fail(_repo, "cannot read HEAD's tree", NULL);
    }

    // in the index's order
    if (_out->count > 1u)
    {
        qsort(_out->items, _out->count, sizeof(*_out->items),
              d_internal_compare_files);
    }

    return true;
}


/*
d_internal_status_add
  One line more of status, its path copied.
*/
static bool
d_internal_status_add(
    struct d_git_status* _status,
    char                 _index,
    char                 _work,
    const char*          _path,
    size_t               _length
)
{
    // room for one more
    if (_status->count == _status->capacity)
    {
        const size_t                     wanted = (_status->capacity > 0u)
                                                  ? (_status->capacity * 2u)
                                                  : 64u;
        struct d_git_status_entry* const grown  =
            realloc(_status->items, wanted * sizeof(*grown));

        // memory that ran out
        if (!grown)
        {
            return false;
        }

        _status->items    = grown;
        _status->capacity = wanted;
    }

    char* const path = malloc(_length + 1u);

    // the path kept
    if (!path)
    {
        return false;
    }

    memcpy(path, _path, _length);
    path[_length] = '\0';
    _status->items[_status->count].index = _index;
    _status->items[_status->count].work  = _work;
    _status->items[_status->count].path  = path;
    ++_status->count;

    return true;
}


void
d_git_status_release(
    struct d_git_status* _status
)
{
    // nothing to release
    if (!_status)
    {
        return;
    }

    // each path
    for (size_t at = 0u; at < _status->count; ++at)
    {
        free(_status->items[at].path);
    }

    free(_status->items);
    memset(_status, 0, sizeof(*_status));

    return;
}


/*
d_internal_unmerged_code
  A conflicted path's two letters, by the stages it has, as git's short
status gives them.
*/
static const char*
d_internal_unmerged_code(
    unsigned _mask
)
{
    static const char* const codes[8] = { "  ", "DD", "AU", "UD", "UA", "DU",
                                          "AA", "UU" };

    return codes[_mask & 7u];
}


/*
d_internal_against_head
  The index's letter for an entry against HEAD's file of its path: added,
deleted, a type changed, modified, or the same.
*/
static char
d_internal_against_head(
    const struct d_git_repo*           _repo,
    const struct d_git_index_entry*    _entry,
    const struct d_internal_tree_file* _head
)
{
    // in the index alone
    if (!_head)
    {
        return (_entry->intent_to_add) ? ' ' : 'A';
    }

    // a type changed, or anything else
    if ((_entry->mode & D_INTERNAL_TYPE_MASK)
            != (_head->mode & D_INTERNAL_TYPE_MASK))
    {
        return 'T';
    }

    return ( (_entry->mode != _head->mode) ||
             (!d_git_repo_same_oid(_repo, &_entry->oid, &_head->oid)) )
           ? 'M'
           : ' ';
}


/*
d_internal_leading_link
  Whether a directory a path is in is a link, or is not a directory at all:
a file under it is not there, as git's has_symlink_leading_path finds.
*/
static bool
d_internal_leading_link(
    const struct d_git_repo* _repo,
    const char*              _path
)
{
    struct d_git_buffer leading;
    bool                link = false;

    memset(&leading, 0, sizeof(leading));

    // each directory above the path
    for (const char* slash = strchr(_path, '/');
         ( (slash) && (!link) );
         slash = strchr(slash + 1, '/'))
    {
        struct d_git_file_info info;

        leading.length = 0u;
        (void)d_git_buffer_add_text(&leading, _repo->work_tree);
        (void)d_git_buffer_add_char(&leading, '/');
        (void)d_git_buffer_add(&leading, _path, (size_t)(slash - _path));
        link = ( (!leading.failed) &&
                 (d_git_file_info(leading.text, &info)) &&
                 (info.kind != D_GIT_FILE_DIRECTORY) );
    }

    d_git_buffer_free(&leading);

    return link;
}


// d_internal_stat
//   enum: what comparing a file's stat data with the index's finds, as
//   git's ie_match_stat and ie_modified weigh it: the same; a size that
//   differs, which is a change unless the index kept no size; or anything
//   else, which only the content can decide.
enum d_internal_stat
{
    D_INTERNAL_STAT_SAME = 0,
    D_INTERNAL_STAT_SIZE,
    D_INTERNAL_STAT_CHECK
};


/*
d_internal_stat_compare
  A file's stat data against the index's, as git's match_stat_data compares
them -- the times in seconds, the size, and, unless core.checkStat is
minimal, the inode and owner; the change time unless core.trustctime is
false. An entry that matches still needs its content asked when it is
racily clean -- changed in the second the index was written, or after --
or smudged: of no size, but not of the empty blob.
*/
static enum d_internal_stat
d_internal_stat_compare(
    const struct d_git_repo*        _repo,
    const struct d_git_index_entry* _entry,
    const struct d_git_file_info*   _info
)
{
    bool              set     = false;
    const char* const check   = d_git_repo_config(_repo, "core.checkstat",
                                                  &set);
    const bool        minimal = ( (set) && (check) &&
                                  (strcmp(check, "minimal") == 0) );
    const bool        ctime   = d_git_repo_flag(_repo, "core.trustctime",
                                                true);
    struct d_git_oid  empty;

    // a size that differs
    if (_entry->size != (uint32_t)_info->size)
    {
        return D_INTERNAL_STAT_SIZE;
    }

    d_git_repo_hash_object(_repo, D_GIT_OBJECT_BLOB, "", 0u, &empty);

    const bool same = ( (_entry->mtime == _info->mtime)                    &&
                        ( (minimal) || (!ctime) ||
                          (_entry->ctime == _info->ctime) )                &&
                        ( (minimal) ||
                          ( (_entry->ino == _info->ino) &&
                            (_entry->uid == _info->uid) &&
                            (_entry->gid == _info->gid) ) ) );
    const bool racy = ( (_repo->index_mtime != 0u) &&
                        (_repo->index_mtime <= _entry->mtime) );
    const bool smudged = ( (_entry->size == 0u) &&
                           (!d_git_repo_same_oid(_repo, &_entry->oid,
                                                 &empty)) );

    return ( (same) && (!racy) && (!smudged) ) ? D_INTERNAL_STAT_SAME
                                               : D_INTERNAL_STAT_CHECK;
}


/*
d_internal_content_same
  Whether a file's content, made clean, is the entry's object: a link's
target, or a file's bytes; a file a filter would clean is not read, and is
taken to have changed.
*/
static bool
d_internal_content_same(
    struct d_git_repo*              _repo,
    const struct d_git_index_entry* _entry,
    const char*                     _where,
    bool                            _link
)
{
    size_t           size = 0u;
    uint8_t* const   data = (_link)
                            ? (uint8_t*)d_git_file_link(_where, &size)
                            : (uint8_t*)d_git_file_read(_where, &size);
    struct d_git_oid oid;

    // unreadable is changed
    if (!data)
    {
        return false;
    }

    const bool clean = ( (_link) ||
                         (d_internal_clean(_repo, _entry->path, data, &size)) );

    // a filter's file is taken to have changed
    if (!clean)
    {
        free(data);

        return false;
    }

    d_git_repo_hash_object(_repo, D_GIT_OBJECT_BLOB, data, size, &oid);
    free(data);

    return d_git_repo_same_oid(_repo, &oid, &_entry->oid);
}


// d_internal_ignoring
//   enum: what of a submodule's changes status passes over, as
//   --ignore-submodules names them.
enum d_internal_ignoring
{
    D_INTERNAL_IGNORE_NONE = 0,   // nothing
    D_INTERNAL_IGNORE_UNTRACKED,  // files it does not track
    D_INTERNAL_IGNORE_DIRTY,      // all but another commit checked out
    D_INTERNAL_IGNORE_ALL,        // everything
    D_INTERNAL_IGNORE_UNKNOWN     // a value that is none of these
};


/*
d_internal_ignoring_of
  What a value of submodule.<name>.ignore or diff.ignoreSubmodules says.
*/
static enum d_internal_ignoring
d_internal_ignoring_of(
    const char* _value
)
{
    static const char* const names[4] = { "none", "untracked", "dirty",
                                          "all" };

    // each name in turn
    for (size_t at = 0u; ( (_value) && (at < 4u) ); ++at)
    {
        // the value's
        if (strcmp(_value, names[at]) == 0)
        {
            return (enum d_internal_ignoring)at;
        }
    }

    return D_INTERNAL_IGNORE_UNKNOWN;
}


/*
d_internal_module_ignore
  What a submodule at a path ignores by its name in .gitmodules' records:
the configuration's submodule.<name>.ignore, else .gitmodules' own;
D_INTERNAL_IGNORE_UNKNOWN for no submodule there, or no value git takes.
*/
static enum d_internal_ignoring
d_internal_module_ignore(
    const struct d_git_repo* _repo,
    const struct d_git_text* _records,
    const char*              _path
)
{
    // each entry, for a submodule's path that is the one
    for (size_t at = 0u; at < _records->entry_count; ++at)
    {
        const struct d_git_text_entry* const entry = &_records->entries[at];
        const char* const key   = d_git_text_string(_records, entry->key);
        const char* const value = d_git_text_string(_records, entry->value);
        const size_t      size  = strlen(key);

        // submodule.<name>.path, of this path
        if ( (size > 15u) &&
             (strncmp(key, "submodule.", 10u) == 0) &&
             (strcmp(&key[size - 5u], ".path") == 0) &&
             (value) &&
             (strcmp(value, _path) == 0) )
        {
            struct d_git_buffer ignore;
            bool                set = false;

            memset(&ignore, 0, sizeof(ignore));
            (void)d_git_buffer_add(&ignore, key, size - 5u);
            (void)d_git_buffer_add_text(&ignore, ".ignore");

            const char* const own   = d_git_repo_config(_repo, ignore.text,
                                                        &set);
            size_t            found = (ignore.failed)
                                      ? D_GIT_TEXT_NONE
                                      : d_git_text_find_entry(_records,
                                                              ignore.text,
                                                              D_GIT_TEXT_NONE);
            size_t            last  = found;

            // .gitmodules' last value of it
            while (found != D_GIT_TEXT_NONE)
            {
                last  = found;
                found = d_git_text_find_entry(_records, ignore.text, last);
            }

            d_git_buffer_free(&ignore);

            const enum d_internal_ignoring said =
                d_internal_ignoring_of(
                    (last != D_GIT_TEXT_NONE)
                    ? d_git_text_string(_records,
                                        _records->entries[last].value)
                    : NULL);

            return ( (set) && (own) )
                   ? d_internal_ignoring_of(own)
                   : said;
        }
    }

    return D_INTERNAL_IGNORE_UNKNOWN;
}


/*
d_internal_gitmodules_ignore
  What .gitmodules says a submodule at a path ignores -- the work tree's
.gitmodules, or the index's -- and the configuration's
submodule.<name>.ignore before it; D_INTERNAL_IGNORE_UNKNOWN for neither,
or a value git passes over.
*/
static enum d_internal_ignoring
d_internal_gitmodules_ignore(
    struct d_git_repo* _repo,
    const char*        _path
)
{
    char* const              where  = d_git_path_join(_repo->work_tree,
                                                      ".gitmodules");
    size_t                   length = 0u;
    char*                    text   = (where) ? d_git_file_read(where, &length)
                                              : NULL;
    const size_t             at     = (text) ? D_GIT_REPO_NONE
                                             : d_git_repo_index_find(
                                                   _repo, ".gitmodules", 0u);
    struct d_git_object      object;
    struct d_git_text        records;
    enum d_internal_ignoring found  = D_INTERNAL_IGNORE_UNKNOWN;
    char                     why[64];

    free(where);
    memset(&object, 0, sizeof(object));

    // none in the work tree: the index's
    if ( (!text) &&
         (at != D_GIT_REPO_NONE) &&
         (d_git_repo_read(_repo, &_repo->entries[at].oid, &object)) )
    {
        text   = (char*)object.data;
        length = object.size;
    }

    // each submodule at the path, its ignore read
    if ( (text) &&
         (d_git_text_read(&records, text, length, D_GIT_TEXT_CONFIG, why,
                          sizeof(why))) )
    {
        found = d_internal_module_ignore(_repo, &records, _path);
    }

    d_git_text_release(&records);

    // the text let go of: the file's, or the object's
    if (object.data)
    {
        d_git_object_release(&object);
    }
    else
    {
        free(text);
    }

    return found;
}


/*
d_internal_submodule_changed
  Whether a submodule's work tree has another commit checked out than the
entry records, or changes of its own -- its tracked files modified, or
files it does not track -- as git status reports a submodule; one not
checked out has none. What counts is as git's match_stat_with_submodule
has it: diff.ignoreSubmodules, untracked files passed over when status
shows none, and then submodule.<name>.ignore, of the configuration or
.gitmodules, in place of both.
  Long as git's match_stat_with_submodule and the settings it reads are,
which it follows.
*/
static bool
d_internal_submodule_changed(
    struct d_git_repo*              _repo,
    const struct d_git_index_entry* _entry,
    const char*                     _where,
    bool                            _untracked
)
{
    char* const dot = d_git_path_join(_where, ".git");
    struct d_git_file_info info;
    const bool  there = ( (dot) && (d_git_file_info(dot, &info)) );

    free(dot);

    // not checked out
    if (!there)
    {
        return false;
    }

    char               problem[64];
    struct d_git_repo* sub = d_git_repo_open(_where, problem,
                                             sizeof(problem));
    struct d_git_oid   head;

    // a submodule that is not one is changed
    if (!sub)
    {
        return true;
    }

    bool                     set     = false;
    const char* const        value   = d_git_repo_config(
                                           _repo, "diff.ignoresubmodules",
                                           &set);
    enum d_internal_ignoring ignoring = (set)
                                        ? d_internal_ignoring_of(value)
                                        : D_INTERNAL_IGNORE_NONE;
    const enum d_internal_ignoring own =
        d_internal_gitmodules_ignore(_repo, _entry->path);

    // untracked files passed over when status shows none; the
    // submodule's own word in place of all
    ignoring = ( (!_untracked) &&
                 ( (ignoring == D_INTERNAL_IGNORE_NONE) ||
                   (ignoring == D_INTERNAL_IGNORE_UNKNOWN) ) )
               ? D_INTERNAL_IGNORE_UNTRACKED
               : ignoring;
    ignoring = (own != D_INTERNAL_IGNORE_UNKNOWN) ? own : ignoring;

    const struct d_git_status_options options = {
        (ignoring == D_INTERNAL_IGNORE_UNTRACKED) ? D_GIT_UNTRACKED_NO
                                                  : D_GIT_UNTRACKED_NORMAL,
        false };
    struct d_git_status               status;

    memset(&status, 0, sizeof(status));

    const bool moved   = ( (!d_git_repo_resolve(sub, "HEAD", &head, NULL)) ||
                           (!d_git_repo_same_oid(_repo, &head,
                                                 &_entry->oid)) );
    const bool changed = (ignoring == D_INTERNAL_IGNORE_ALL) ? false
                         : (moved) ? true
                         : (ignoring == D_INTERNAL_IGNORE_DIRTY) ? false
                         : ( (!d_git_repo_status(sub, &options, &status)) ||
                             (status.count > 0u) );


    d_git_status_release(&status);
    d_git_repo_free(sub);

    return changed;
}


/*
d_internal_against_work
  The work tree's letter for an entry: deleted, a type changed, modified,
or the same, as git's run_diff_files decides it.
  Long as run_diff_files' order of tests is, which it follows: gone, a
submodule, a type changed, the mode, the content.
*/
static char
d_internal_against_work(
    struct d_git_repo*              _repo,
    const struct d_git_index_entry* _entry,
    bool                            _untracked
)
{
    char* const            where = d_git_path_join(_repo->work_tree,
                                                   _entry->path);
    struct d_git_file_info info;
    const uint32_t         type  = _entry->mode & D_INTERNAL_TYPE_MASK;

    // not there, or under a link
    if ( (!where) ||
         (!d_git_file_info(where, &info)) ||
         (d_internal_leading_link(_repo, _entry->path)) )
    {
        free(where);

        return 'D';
    }

    char work = ' ';

    // a submodule, a directory where a file was, a type changed, the mode,
    // or the content
    if (type == D_INTERNAL_GITLINK)
    {
        work = (info.kind != D_GIT_FILE_DIRECTORY) ? 'T'
             : (d_internal_submodule_changed(_repo, _entry, where,
                                             _untracked)) ? 'M'
             : ' ';
    }
    else if (info.kind == D_GIT_FILE_DIRECTORY)
    {
        work = 'D';
    }
    else if ( ( (type == D_INTERNAL_REGULAR) &&
                (info.kind != D_GIT_FILE_REGULAR) ) ||
              ( (type == D_INTERNAL_SYMLINK) &&
                (info.kind != D_GIT_FILE_LINK) &&
                (d_git_repo_flag(_repo, "core.symlinks", true)) ) )
    {
        work = 'T';
    }
    else if ( (type == D_INTERNAL_REGULAR) &&
              (d_git_repo_flag(_repo, "core.filemode", true)) &&
              (info.executable != ((_entry->mode & 0100u) != 0u)) )
    {
        work = 'M';
    }
    else if (!_entry->assume_valid)
    {
        const enum d_internal_stat stat = d_internal_stat_compare(_repo,
                                                                  _entry,
                                                                  &info);

        work = ( ( (stat == D_INTERNAL_STAT_SIZE) && (_entry->size != 0u) ) ||
                 ( (stat != D_INTERNAL_STAT_SAME) &&
                   (!d_internal_content_same(_repo, _entry, where,
                                             (info.kind
                                                  == D_GIT_FILE_LINK))) ) )
               ? 'M'
               : ' ';
    }

    free(where);

    return ( (_entry->intent_to_add) && (work != 'D') ) ? 'A' : work;
}


/*
d_internal_tracked_under
  Whether the index holds a path below a directory: whether its first path
not before "directory/" begins with it.
*/
static bool
d_internal_tracked_under(
    const struct d_git_repo* _repo,
    const char*              _directory,
    size_t                   _length
)
{
    struct d_git_buffer prefix;
    bool                found = false;

    memset(&prefix, 0, sizeof(prefix));
    (void)d_git_buffer_add(&prefix, _directory, _length);
    (void)d_git_buffer_add_char(&prefix, '/');

    // the first entry not before the prefix
    if (!prefix.failed)
    {
        const size_t at = d_internal_index_position(_repo, prefix.text,
                                                    prefix.length, 0u,
                                                    &found);

        found = ( (at < _repo->entry_count) &&
                  (_repo->entries[at].path_length >= prefix.length) &&
                  (memcmp(_repo->entries[at].path, prefix.text,
                          prefix.length) == 0) );
    }

    d_git_buffer_free(&prefix);

    return found;
}


/*
d_internal_tracked_at
  Whether the index holds a path itself, at any stage.
*/
static bool
d_internal_tracked_at(
    const struct d_git_repo* _repo,
    const char*              _path,
    size_t                   _length
)
{
    bool         found = false;
    const size_t at    = d_internal_index_position(_repo, _path, _length, 0u,
                                                   &found);

    return ( (found) ||
             ( (at < _repo->entry_count) &&
               (_repo->entries[at].path_length == _length) &&
               (memcmp(_repo->entries[at].path, _path, _length) == 0) ) );
}


/*
d_internal_is_ignored
  Whether git ignores a path of the work tree.
*/
static bool
d_internal_is_ignored(
    struct d_git_repo* _repo,
    const char*        _path
)
{
    struct d_git_ignore_match match;

    return ( (d_git_repo_ignored(_repo, _path, &match)) &&
             (match.ignored) );
}


/*
d_internal_nested_repository
  Whether a directory of the work tree is a repository of its own: a .git
in it, which git shows untracked as a whole.
*/
static bool
d_internal_nested_repository(
    const struct d_git_repo* _repo,
    const char*              _path
)
{
    char* const            where = d_git_path_join(_repo->work_tree, _path);
    char* const            dot   = (where) ? d_git_path_join(where, ".git")
                                           : NULL;
    struct d_git_file_info info;
    const bool             is    = ( (dot) && (d_git_file_info(dot, &info)) );

    free(where);
    free(dot);

    return is;
}


/*
d_internal_compare_status
  Orders status's lines by path.
*/
static int
d_internal_compare_status(
    const void* _left,
    const void* _right
)
{
    const struct d_git_status_entry* const left  = _left;
    const struct d_git_status_entry* const right = _right;

    return strcmp(left->path, right->path);
}


/*
d_internal_status_append
  The lines of one list after another's, ordered by path.
*/
static bool
d_internal_status_append(
    struct d_git_status* _out,
    struct d_git_status* _more
)
{
    bool went = true;

    // ordered
    if (_more->count > 1u)
    {
        qsort(_more->items, _more->count, sizeof(*_more->items),
              d_internal_compare_status);
    }

    // each line moved over
    for (size_t at = 0u; ( (went) && (at < _more->count) ); ++at)
    {
        went = d_internal_status_add(_out, _more->items[at].index,
                                     _more->items[at].work,
                                     _more->items[at].path,
                                     strlen(_more->items[at].path));
    }

    d_git_status_release(_more);

    return went;
}


// d_internal_untracked
//   struct: the walk for untracked files: the repository, how they are
//   shown, and the lines found -- untracked, and ignored.
struct d_internal_untracked
{
    struct d_git_repo*                 repo;
    const struct d_git_status_options* options;
    struct d_git_status*               untracked;
    struct d_git_status*               ignored;
};

// d_internal_name
//   struct: one name of a directory being walked: its path from the top,
//   as status shows it -- a directory's with its `/` -- and what it is.
struct d_internal_name
{
    struct d_git_buffer shown;
    size_t              length;
    bool                dir;
    bool                tracked;
    bool                tracked_under;
    bool                ignored;
    bool                nested;
};


/*
d_internal_names_of
  A directory's names, ordered, .git not among them.
*/
static bool
d_internal_names_of(
    const struct d_git_repo* _repo,
    const char*              _directory,
    char***                  _names,
    size_t*                  _count
)
{
    char* const where = d_git_path_join(_repo->work_tree, _directory);
    const bool  read  = ( (where) && (d_fs_names(where, _names, _count)) );

    free(where);

    // ordered
    if ( (read) &&
         (*_count > 1u) )
    {
        qsort(*_names, *_count, sizeof(**_names), d_internal_compare_files);
    }

    return read;
}


/*
d_internal_name_of
  What a name of a directory is: there or not, a directory, tracked or with
tracked paths below it, ignored, a repository of its own.
*/
static bool
d_internal_name_of(
    struct d_git_repo*      _repo,
    const char*             _directory,
    const char*             _name,
    struct d_internal_name* _out
)
{
    memset(_out, 0, sizeof(*_out));
    (void)d_git_buffer_add_text(&_out->shown, _directory);
    (void)d_git_buffer_add_text(&_out->shown, (_directory[0] != '\0') ? "/"
                                                                      : "");
    (void)d_git_buffer_add_text(&_out->shown, _name);
    _out->length = _out->shown.length;

    char* const            where = (_out->shown.text)
                                   ? d_git_path_join(_repo->work_tree,
                                                     _out->shown.text)
                                   : NULL;
    struct d_git_file_info info;
    const bool             there = ( (where) &&
                                     (d_git_file_info(where, &info)) );

    free(where);

    // gone since it was named
    if ( (!there) ||
         (_out->shown.failed) )
    {
        return false;
    }

    _out->dir           = (info.kind == D_GIT_FILE_DIRECTORY);
    _out->tracked       = d_internal_tracked_at(_repo, _out->shown.text,
                                                _out->length);
    _out->tracked_under = ( (_out->dir) &&
                            (d_internal_tracked_under(_repo, _out->shown.text,
                                                      _out->length)) );
    _out->nested        = ( (_out->dir) &&
                            (d_internal_nested_repository(_repo,
                                                          _out->shown.text)) );

    // a directory is shown with its slash, and asked of as one
    if (_out->dir)
    {
        (void)d_git_buffer_add_char(&_out->shown, '/');
    }

    _out->ignored = ( (!_out->tracked) &&
                      (!_out->tracked_under) &&
                      (d_internal_is_ignored(_repo, _out->shown.text)) );

    return !_out->shown.failed;
}


static bool d_internal_scan_all(struct d_internal_untracked* _walk,
                                const char*                  _directory,
                                bool                         _ignored);


/*
d_internal_scan_all_name
  One name of a directory shown file by file, as -uall shows one: a
repository of its own shown whole; a directory walked -- an ignored one's
files all ignored, walked only when the ignored are shown; a file shown
untracked or ignored.
*/
static bool
d_internal_scan_all_name(
    struct d_internal_untracked*  _walk,
    const struct d_internal_name* _name,
    bool                          _ignored
)
{
    const bool ignored = ( (_ignored) || (_name->ignored) );

    // a repository, a directory, or a file
    if ( (_name->nested) &&
         (!ignored) )
    {
        return d_internal_status_add(_walk->untracked, '?', '?',
                                     _name->shown.text, _name->shown.length);
    }
    else if (_name->dir)
    {
        return ( ( (ignored) && (!_walk->options->ignored) ) ||
                 (d_internal_scan_all(_walk, _name->shown.text, ignored)) );
    }
    else if (ignored)
    {
        return ( (!_walk->options->ignored) ||
                 (d_internal_status_add(_walk->ignored, '!', '!',
                                        _name->shown.text,
                                        _name->shown.length)) );
    }

    return d_internal_status_add(_walk->untracked, '?', '?',
                                 _name->shown.text, _name->shown.length);
}


/*
d_internal_scan_all
  An untracked directory shown file by file.
*/
static bool
d_internal_scan_all(
    struct d_internal_untracked* _walk,
    const char*                  _directory,
    bool                         _ignored
)
{
    struct d_git_buffer directory;
    char**              names = NULL;
    size_t              count = 0u;
    bool                went  = true;

    memset(&directory, 0, sizeof(directory));
    (void)d_git_buffer_add_text(&directory, _directory);

    // the directory's own path, without its slash
    while ( (directory.length > 0u) &&
            (directory.text[directory.length - 1u] == '/') )
    {
        directory.text[--directory.length] = '\0';
    }

    // the names, if it can be read
    if ( (directory.failed) ||
         (!d_internal_names_of(_walk->repo, directory.text, &names, &count)) )
    {
        d_git_buffer_free(&directory);

        return !directory.failed;
    }

    // each name but .git
    for (size_t at = 0u; ( (went) && (at < count) ); ++at)
    {
        struct d_internal_name name;

        memset(&name, 0, sizeof(name));

        // .git, or a name gone
        if ( (strcmp(names[at], ".git") == 0) ||
             (!d_internal_name_of(_walk->repo, directory.text, names[at],
                                  &name)) )
        {
            d_git_buffer_free(&name.shown);

            continue;
        }

        went = d_internal_scan_all_name(_walk, &name, _ignored);
        d_git_buffer_free(&name.shown);
    }

    d_fs_names_free(names, count);
    d_git_buffer_free(&directory);

    return went;
}


static bool d_internal_scan_collapsed(struct d_internal_untracked* _walk,
                                      const char*                  _directory,
                                      struct d_git_status*         _ignored,
                                      bool*                        _out_found);


/*
d_internal_scan_collapsed_name
  One name of an untracked directory status shows whole: an ignored one
noted, an untracked file or repository found, a directory scanned the
same way -- what it holds untracked found with its ignored noted, what it
holds only ignored noted as the directory.
*/
static bool
d_internal_scan_collapsed_name(
    struct d_internal_untracked*  _walk,
    const struct d_internal_name* _name,
    struct d_git_status*          _ignored,
    bool*                         _out_found
)
{
    // ignored, a repository, a file
    if (_name->ignored)
    {
        return d_internal_status_add(_ignored, '!', '!', _name->shown.text,
                                     _name->shown.length);
    }
    else if ( (_name->nested) ||
              (!_name->dir) )
    {
        *_out_found = true;

        return true;
    }

    struct d_git_status inner;
    bool                found = false;

    memset(&inner, 0, sizeof(inner));

    bool went = d_internal_scan_collapsed(_walk, _name->shown.text, &inner,
                                          &found);

    // its ignored, if it holds untracked; itself, if it holds only ignored
    if ( (went) &&
         (found) )
    {
        *_out_found = true;
        went        = d_internal_status_append(_ignored, &inner);
    }
    else if ( (went) &&
              (inner.count > 0u) )
    {
        went = d_internal_status_add(_ignored, '!', '!', _name->shown.text,
                                     _name->shown.length);
    }

    d_git_status_release(&inner);

    return went;
}


/*
d_internal_scan_collapsed
  An untracked directory status shows whole: whether it holds anything
untracked, and its ignored -- collapsed as git collapses them -- noted;
with no ignored to show, the first untracked found is enough.
*/
static bool
d_internal_scan_collapsed(
    struct d_internal_untracked* _walk,
    const char*                  _directory,
    struct d_git_status*         _ignored,
    bool*                        _out_found
)
{
    struct d_git_buffer directory;
    char**              names = NULL;
    size_t              count = 0u;
    bool                went  = true;

    memset(&directory, 0, sizeof(directory));
    (void)d_git_buffer_add_text(&directory, _directory);

    // the directory's own path, without its slash
    while ( (directory.length > 0u) &&
            (directory.text[directory.length - 1u] == '/') )
    {
        directory.text[--directory.length] = '\0';
    }

    // the names, if it can be read
    if ( (directory.failed) ||
         (!d_internal_names_of(_walk->repo, directory.text, &names, &count)) )
    {
        d_git_buffer_free(&directory);

        return !directory.failed;
    }

    // each name, until enough is known
    for (size_t at = 0u;
         ( (went) &&
           (at < count) &&
           ( (_walk->options->ignored) || (!*_out_found) ) );
         ++at)
    {
        struct d_internal_name name;

        memset(&name, 0, sizeof(name));

        // .git, or a name gone
        if ( (strcmp(names[at], ".git") == 0) ||
             (!d_internal_name_of(_walk->repo, directory.text, names[at],
                                  &name)) )
        {
            d_git_buffer_free(&name.shown);

            continue;
        }

        went = d_internal_scan_collapsed_name(_walk, &name, _ignored,
                                              _out_found);
        d_git_buffer_free(&name.shown);
    }

    d_fs_names_free(names, count);
    d_git_buffer_free(&directory);

    return went;
}


/*
d_internal_untracked_dir
  An untracked directory below a tracked one: ignored, shown as ignored
-- whole, or file by file for -uall; a repository of its own shown whole;
else file by file for -uall, or whole: untracked when it holds anything
untracked, its ignored with it, or ignored when it holds only ignored.
*/
static bool
d_internal_untracked_dir(
    struct d_internal_untracked*  _walk,
    const struct d_internal_name* _name
)
{
    const bool all = (_walk->options->untracked == D_GIT_UNTRACKED_ALL);

    // ignored, a repository, or file by file
    if (_name->ignored)
    {
        return ( (!_walk->options->ignored) ||
                 ( (all)
                   ? d_internal_scan_all(_walk, _name->shown.text, true)
                   : d_internal_status_add(_walk->ignored, '!', '!',
                                           _name->shown.text,
                                           _name->shown.length) ) );
    }
    else if (_name->nested)
    {
        return d_internal_status_add(_walk->untracked, '?', '?',
                                     _name->shown.text, _name->shown.length);
    }
    else if (all)
    {
        return d_internal_scan_all(_walk, _name->shown.text, false);
    }

    struct d_git_status inner;
    bool                found = false;

    memset(&inner, 0, sizeof(inner));

    bool went = d_internal_scan_collapsed(_walk, _name->shown.text, &inner,
                                          &found);

    // untracked, with its ignored; or ignored as a whole
    if ( (went) &&
         (found) )
    {
        went = ( (d_internal_status_add(_walk->untracked, '?', '?',
                                        _name->shown.text,
                                        _name->shown.length)) &&
                 ( (!_walk->options->ignored) ||
                   (d_internal_status_append(_walk->ignored, &inner)) ) );
    }
    else if ( (went) &&
              (inner.count > 0u) &&
              (_walk->options->ignored) )
    {
        went = d_internal_status_add(_walk->ignored, '!', '!',
                                     _name->shown.text, _name->shown.length);
    }

    d_git_status_release(&inner);

    return went;
}


/*
d_internal_walk_tracked
  A directory that holds tracked paths -- the top among them -- name by
name: tracked files passed, directories with tracked paths walked the same
way, untracked directories as d_internal_untracked_dir shows them, and
untracked files shown, as untracked or ignored.
*/
static bool
d_internal_walk_tracked(
    struct d_internal_untracked* _walk,
    const char*                  _directory
)
{
    char** names = NULL;
    size_t count = 0u;
    bool   went  = true;

    // the names, if it can be read
    if (!d_internal_names_of(_walk->repo, _directory, &names, &count))
    {
        return true;
    }

    // each name but .git
    for (size_t at = 0u; ( (went) && (at < count) ); ++at)
    {
        struct d_internal_name name;

        memset(&name, 0, sizeof(name));

        // .git, or a name gone, or tracked
        if ( (strcmp(names[at], ".git") == 0) ||
             (!d_internal_name_of(_walk->repo, _directory, names[at],
                                  &name)) ||
             (name.tracked) )
        {
            d_git_buffer_free(&name.shown);

            continue;
        }

        // a directory with tracked paths, an untracked one, or a file
        if (name.tracked_under)
        {
            name.shown.text[name.length] = '\0';
            went = d_internal_walk_tracked(_walk, name.shown.text);
        }
        else if (name.dir)
        {
            went = d_internal_untracked_dir(_walk, &name);
        }
        else if (name.ignored)
        {
            went = ( (!_walk->options->ignored) ||
                     (d_internal_status_add(_walk->ignored, '!', '!',
                                            name.shown.text,
                                            name.shown.length)) );
        }
        else
        {
            went = d_internal_status_add(_walk->untracked, '?', '?',
                                         name.shown.text, name.shown.length);
        }

        d_git_buffer_free(&name.shown);
    }

    d_fs_names_free(names, count);

    return went;
}


/*
d_internal_status_tracked
  The tracked lines: each path of the index, or of HEAD alone, with its
letters -- a conflict's from its stages; else the index's against HEAD
and the work tree's against the index -- if either is not a space.
  Long as the index and HEAD are merged by path here, every stage of a path
gathered.
*/
static bool
d_internal_status_tracked(
    struct d_git_repo*                  _repo,
    const struct d_internal_tree_files* _head,
    bool                                _untracked,
    struct d_git_status*                _out
)
{
    size_t at   = 0u;
    size_t file = 0u;
    bool   went = true;

    // the index and HEAD, merged by path
    while ( (went) &&
            ( (at < _repo->entry_count) || (file < _head->count) ) )
    {
        const int order = (at >= _repo->entry_count) ? 1
                        : (file >= _head->count) ? -1
                        : strcmp(_repo->entries[at].path,
                                 _head->items[file].path);

        // HEAD's alone: deleted from the index
        if (order > 0)
        {
            went = d_internal_status_add(_out, 'D', ' ',
                                         _head->items[file].path,
                                         strlen(_head->items[file].path));
            ++file;

            continue;
        }

        const struct d_git_index_entry* const entry = &_repo->entries[at];
        unsigned                              mask  = 0u;
        size_t                                next  = at;

        // every stage of the path
        while ( (next < _repo->entry_count) &&
                (strcmp(_repo->entries[next].path, entry->path) == 0) )
        {
            mask |= (_repo->entries[next].stage > 0u)
                    ? (1u << (_repo->entries[next].stage - 1u))
                    : 0u;
            ++next;
        }

        const struct d_internal_tree_file* const head =
            (order == 0) ? &_head->items[file] : NULL;
        const char* const code  = d_internal_unmerged_code(mask);
        const char        index = (mask != 0u) ? code[0]
                                : d_internal_against_head(_repo, entry, head);
        const char        work  = (mask != 0u) ? code[1]
                                : ( (entry->skip_worktree)
                                    ? ' '
                                    : d_internal_against_work(_repo, entry,
                                                              _untracked) );

        // a line when anything changed
        if ( (index != ' ') ||
             (work != ' ') )
        {
            went = d_internal_status_add(_out, index, work, entry->path,
                                         entry->path_length);
        }

        file += (order == 0) ? 1u : 0u;
        at    = next;
    }

    return went;
}


bool
d_git_repo_status(
    struct d_git_repo*                 _repo,
    const struct d_git_status_options* _options,
    struct d_git_status*               _out
)
{
    struct d_internal_tree_files head;
    struct d_git_status          untracked;
    struct d_git_status          ignored;

    memset(_out, 0, sizeof(*_out));
    memset(&untracked, 0, sizeof(untracked));
    memset(&ignored, 0, sizeof(ignored));

    // a work tree, its index, and HEAD's files
    if ( (!_repo) ||
         (!_options) ||
         (!_repo->work_tree) )
    {
        return (_repo) ? d_git_repo_fail(_repo, "status needs a work tree",
                                         NULL)
                       : false;
    }
    else if ( (d_git_repo_index_count(_repo) == D_GIT_REPO_NONE) ||
              (!d_internal_head_files(_repo, &head)) )
    {
        return false;
    }

    struct d_internal_untracked walk = { _repo, _options, &untracked,
                                         &ignored };
    bool went = ( (d_internal_status_tracked(
                       _repo, &head,
                       (_options->untracked != D_GIT_UNTRACKED_NO), _out)) &&
                  ( (_options->untracked == D_GIT_UNTRACKED_NO) ||
                    (d_internal_walk_tracked(&walk, "")) ) );

    d_internal_tree_files_free(&head);
    went = ( (went) &&
             (d_internal_status_append(_out, &untracked)) &&
             (d_internal_status_append(_out, &ignored)) );
    d_git_status_release(&untracked);
    d_git_status_release(&ignored);

    return (went) ? true : d_git_repo_fail(_repo, "out of memory", NULL);
}
