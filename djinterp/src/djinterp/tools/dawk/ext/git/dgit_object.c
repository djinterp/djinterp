/*******************************************************************************
* djinterp [tools]                                                 dgit_object.c
*
* Definitions for the objects of dgit_repo.h: reading, naming and parsing.
*   An object is looked for in each store in turn -- the repository's own
* directory of objects, then its alternates, as objects/info/alternates and
* GIT_ALTERNATE_OBJECT_DIRECTORIES name them -- loose first, then in each
* pack. A loose object is a zlib stream of its header and content, checked
* against its name. A pack's index is read whole, and an object found in it
* by its first byte's range and a binary search; the pack itself is read at
* places, an object's bytes from its place to the next object's. A delta is
* applied to its base, found back along the pack or by name, a chain of
* them walked to its first base and applied outward; bases are kept, a few
* hundred by place, so the deltas of a walk through history do not inflate
* the same base again and again.
*   Parsing is git's, and as lenient: a commit's tree, parents, author and
* committer, encoding and message; a tag's object, type, name and tagger;
* a tree's entries in order.
*
* path:      /src/djinterp/tools/dawk/ext/git/dgit_object.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./dgit_internal.h"  // corresponding header: d_git_repo layout
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, calloc, realloc, free, getenv, qsort
#include <string.h>  // memchr, memcmp, memcpy, memmove, memset, strchr,
                     // strcmp, strlen, strncmp
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"                // d_fs_names
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_repo.h"  // d_git_object
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, uint32_t,
                                                     // uint64_t, int64_t


// D_INTERNAL_ALTERNATE_DEPTH
//   constant: how deep alternates of alternates go, as git allows them.
#define D_INTERNAL_ALTERNATE_DEPTH 5u

// D_INTERNAL_CACHE_MOST, D_INTERNAL_CACHE_TOTAL
//   constant: the largest object kept as a delta's base, and the most
//   bytes all those kept may hold.
#define D_INTERNAL_CACHE_MOST  (1u << 20)
#define D_INTERNAL_CACHE_TOTAL (64u << 20)

// D_INTERNAL_OFS_DELTA, D_INTERNAL_REF_DELTA
//   constant: a pack's types of delta: one whose base is back along the
//   pack, and one whose base is named.
#define D_INTERNAL_OFS_DELTA 6u
#define D_INTERNAL_REF_DELTA 7u

//==============================================================================
// 1.  TYPES
//==============================================================================


// d_internal_type_names
//   constant: the types' names, by git's numbers.
static const char* const d_internal_type_names[5] =
{
    "", "commit", "tree", "blob", "tag"
};


const char*
d_git_object_type_name(
    enum d_git_object_type _type
)
{
    return ( (_type >= D_GIT_OBJECT_COMMIT) &&
             (_type <= D_GIT_OBJECT_TAG) )
           ? d_internal_type_names[_type]
           : "";
}


enum d_git_object_type
d_git_object_type_named(
    const char* _name
)
{
    // each type's name
    for (unsigned at = D_GIT_OBJECT_COMMIT; at <= D_GIT_OBJECT_TAG; ++at)
    {
        // the name, whole
        if ( (_name) &&
             (strcmp(_name, d_internal_type_names[at]) == 0) )
        {
            return (enum d_git_object_type)at;
        }
    }

    return D_GIT_OBJECT_NONE;
}


void
d_git_object_release(
    struct d_git_object* _object
)
{
    // nothing to release
    if (!_object)
    {
        return;
    }

    free(_object->data);
    memset(_object, 0, sizeof(*_object));

    return;
}


/*
d_internal_header
  An object's header as git hashes it: its type's name, a space, its size
in decimal, and a NUL; its length.
*/
static size_t
d_internal_header(
    enum d_git_object_type _type,
    size_t                 _size,
    char*                  _out,
    size_t                 _capacity
)
{
    const int written = snprintf(_out,
                                 _capacity,
                                 "%s %zu",
                                 d_git_object_type_name(_type),
                                 _size);

    return (written > 0) ? ((size_t)written + 1u) : 0u;
}


void
d_git_repo_hash_object(
    const struct d_git_repo* _repo,
    enum d_git_object_type   _type,
    const void*              _data,
    size_t                   _size,
    struct d_git_oid*        _out
)
{
    char              header[64];
    const size_t      length = d_internal_header(_type,
                                                 _size,
                                                 header,
                                                 sizeof(header));
    struct d_git_hash hash;

    memset(_out, 0, sizeof(*_out));
    d_git_hash_begin(&hash, (_repo) ? _repo->hash : D_GIT_HASH_SHA1);
    d_git_hash_feed(&hash, header, length);
    d_git_hash_feed(&hash, _data, _size);
    d_git_hash_end(&hash, _out->bytes);

    return;
}


//==============================================================================
// 2.  STORES
//==============================================================================


/*
d_internal_hex
  A name in hex, terminated, of the repository's length.
*/
static void
d_internal_hex(
    const struct d_git_repo* _repo,
    const struct d_git_oid*  _oid,
    char*                    _out
)
{
    d_git_repo_hex(_repo, _oid, _out);

    return;
}


/*
d_internal_store_add
  A store of objects more, unless one of the same path is there already.
*/
static bool
d_internal_store_add(
    struct d_git_repo* _repo,
    const char*        _path
)
{
    // one of the same path, which an alternate may name again
    for (size_t at = 0u; at < _repo->store_count; ++at)
    {
        // the same
        if (strcmp(_repo->stores[at].path, _path) == 0)
        {
            return true;
        }
    }

    struct d_git_store* const grown = realloc(_repo->stores,
                                              (_repo->store_count + 1u)
                                                  * sizeof(*grown));

    // memory that ran out
    if (!grown)
    {
        return false;
    }

    _repo->stores = grown;

    struct d_git_store* const store = &_repo->stores[_repo->store_count];
    const size_t              length = strlen(_path);

    memset(store, 0, sizeof(*store));
    store->path = malloc(length + 1u);

    // the path kept
    if (!store->path)
    {
        return false;
    }

    memcpy(store->path, _path, length + 1u);
    ++_repo->store_count;

    return true;
}


/*
d_internal_resolve_path
  A path relative to a directory, made whole: as it is when absolute.
*/
static char*
d_internal_resolve_path(
    const char* _directory,
    const char* _path
)
{
    // absolute: as it is
    if ( (_path[0] == '/') ||
         ( (_path[0] != '\0') && (_path[1] == ':') ) )
    {
        return d_git_path_join("", _path);
    }

    return d_git_path_join(_directory, _path);
}


static bool d_internal_alternates(struct d_git_repo* _repo,
                                  const char*        _objects,
                                  unsigned           _depth);


/*
d_internal_alternate_line
  One line of an alternates file: a comment, a path in C's quotes, or a
path; relative to the objects directory that names it.
*/
static bool
d_internal_alternate_line(
    struct d_git_repo* _repo,
    const char*        _objects,
    const char*        _line,
    size_t             _length,
    unsigned           _depth
)
{
    struct d_git_buffer path;
    size_t              used = 0u;

    memset(&path, 0, sizeof(path));

    // a comment, or nothing
    if ( (_length == 0u) ||
         (_line[0] == '#') )
    {
        return true;
    }

    // a quoted path, unquoted; any other as it is
    if ( (_line[0] != '"') ||
         (!d_git_text_unquote(&path, _line, _length, &used)) )
    {
        d_git_buffer_free(&path);
        (void)d_git_buffer_add(&path, _line, _length);
    }

    char* const whole = (path.text) ? d_internal_resolve_path(_objects,
                                                              path.text)
                                    : NULL;
    const bool  added = ( (whole) &&
                          (d_internal_store_add(_repo, whole)) &&
                          (d_internal_alternates(_repo, whole, _depth + 1u)) );

    free(whole);
    d_git_buffer_free(&path);

    return added;
}


/*
d_internal_alternates
  The stores an objects directory's info/alternates names, and theirs, no
deeper than git goes.
*/
static bool
d_internal_alternates(
    struct d_git_repo* _repo,
    const char*        _objects,
    unsigned           _depth
)
{
    // deep enough
    if (_depth > D_INTERNAL_ALTERNATE_DEPTH)
    {
        return true;
    }

    char* const path   = d_git_path_join(_objects, "info/alternates");
    size_t      length = 0u;
    char* const text   = (path) ? d_git_file_read(path, &length) : NULL;
    bool        added  = true;

    free(path);

    // each line
    for (size_t at = 0u; ( (text) && (added) && (at < length) );)
    {
        const char* const end  = memchr(&text[at], '\n', length - at);
        const size_t      stop = (end) ? (size_t)(end - text) : length;
        size_t            size = stop - at;

        // a `\r` before the `\n` is the ending's
        if ( (size > 0u) &&
             (text[at + size - 1u] == '\r') )
        {
            --size;
        }

        added = d_internal_alternate_line(_repo, _objects, &text[at], size,
                                          _depth);
        at    = stop + 1u;
    }

    free(text);

    return added;
}


/*
d_internal_environment_alternates
  The stores GIT_ALTERNATE_OBJECT_DIRECTORIES names, apart by colons --
semicolons on Windows.
*/
static bool
d_internal_environment_alternates(
    struct d_git_repo* _repo,
    char               _separator
)
{
    const char* at    = getenv("GIT_ALTERNATE_OBJECT_DIRECTORIES");
    bool        added = true;

    // each path
    while ( (at) &&
            (*at != '\0') &&
            (added) )
    {
        const char* const end  = strchr(at, _separator);
        const size_t      size = (end) ? (size_t)(end - at) : strlen(at);
        char* const       path = malloc(size + 1u);

        // the path alone
        if (path)
        {
            memcpy(path, at, size);
            path[size] = '\0';
        }

        added = ( (path) &&
                  ( (size == 0u) ||
                    ( (d_internal_store_add(_repo, path)) &&
                      (d_internal_alternates(_repo, path, 1u)) ) ) );
        free(path);
        at = (end) ? (end + 1) : NULL;
    }

    return added;
}


#if defined(_WIN32)

/*
d_internal_list_separator
  What separates the paths of a list in the environment: `;` on Windows.
*/
static char
d_internal_list_separator(void)
{
    return ';';
}

#else

/*
d_internal_list_separator
  What separates the paths of a list in the environment: `:` on POSIX.
*/
static char
d_internal_list_separator(void)
{
    return ':';
}

#endif  // _WIN32


/*
d_internal_stores
  The stores, found the first time an object is wanted: the repository's
own -- GIT_OBJECT_DIRECTORY's, when it is set -- then its alternates.
*/
static bool
d_internal_stores(
    struct d_git_repo* _repo
)
{
    // found already
    if (_repo->store_count > 0u)
    {
        return true;
    }

    const char* const named = getenv("GIT_OBJECT_DIRECTORY");
    char* const       own   = ( (named) && (named[0] != '\0') )
                              ? d_git_path_join("", named)
                              : d_git_path_join(_repo->common_dir, "objects");
    const char        sep   = d_internal_list_separator();
    const bool        found = ( (own) &&
                                (d_internal_store_add(_repo, own)) &&
                                (d_internal_alternates(_repo, own, 1u)) &&
                                (d_internal_environment_alternates(_repo,
                                                                   sep)) );

    free(own);

    return (found) ? true : d_git_repo_fail(_repo, "out of memory", NULL);
}


void
d_git_stores_release(
    struct d_git_repo* _repo
)
{
    // each store's packs
    for (size_t at = 0u; at < _repo->store_count; ++at)
    {
        struct d_git_store* const store = &_repo->stores[at];

        // each pack
        for (size_t pack = 0u; pack < store->pack_count; ++pack)
        {
            d_git_file_close(store->packs[pack].file);
            free(store->packs[pack].path);
            free(store->packs[pack].idx);
            free(store->packs[pack].sorted);
        }

        free(store->packs);
        free(store->path);
    }

    // the bases kept
    for (size_t at = 0u; at < D_GIT_CACHE_SLOTS; ++at)
    {
        free(_repo->cache[at].data);
    }

    free(_repo->stores);
    _repo->stores      = NULL;
    _repo->store_count = 0u;
    _repo->cache_bytes = 0u;
    memset(_repo->cache, 0, sizeof(_repo->cache));

    return;
}


//==============================================================================
// 3.  PACK INDEXES
//==============================================================================


/*
d_internal_be32, d_internal_be64
  Four and eight bytes read big-endian.
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


static uint64_t
d_internal_be64(
    const uint8_t* _bytes
)
{
    return ((uint64_t)d_internal_be32(_bytes) << 32) |
           (uint64_t)d_internal_be32(&_bytes[4]);
}


/*
d_internal_index_layout
  Where an index's tables are, by its version: 2, after its magic, with
names, checksums, places and places past 4 GiB in tables of their own; 1,
a place before each name. False for one too short for what it says.
*/
static bool
d_internal_index_layout(
    struct d_git_pack* _pack,
    size_t             _hash
)
{
    static const uint8_t magic[4] = { 0xFFu, 't', 'O', 'c' };
    const bool           two      = ( (_pack->idx_length >= 8u) &&
                                      (memcmp(_pack->idx, magic, 4u) == 0) );
    const uint8_t* const fanout   = &_pack->idx[(two) ? 8u : 0u];
    const size_t         head     = ((two) ? 8u : 0u) + 1024u;

    // the fanout's room, and version 2's number
    if ( (_pack->idx_length < (head + (2u * _hash))) ||
         ( (two) && (d_internal_be32(&_pack->idx[4]) != 2u) ) )
    {
        return false;
    }

    _pack->version = (two) ? 2u : 1u;
    _pack->count   = d_internal_be32(&fanout[255u * 4u]);

    const size_t count = _pack->count;

    // version 1: a place and a name, together
    if (!two)
    {
        _pack->oids   = &_pack->idx[head + 4u];
        _pack->stride = 4u + _hash;

        return (_pack->idx_length >= (head + (count * (4u + _hash))
                                      + (2u * _hash)));
    }

    _pack->oids    = &_pack->idx[head];
    _pack->stride  = _hash;
    _pack->offsets = &_pack->idx[head + (count * _hash) + (count * 4u)];
    _pack->large   = &_pack->offsets[count * 4u];

    return (_pack->idx_length >= (head + (count * (_hash + 8u))
                                  + (2u * _hash)));
}


/*
d_internal_pack_load
  One pack: its index read and laid out, the pack's path beside it, ".pack"
for ".idx"; the pack opened when an object is first read from it. An index
that does not read is passed over, as git passes one over.
*/
static bool
d_internal_pack_load(
    struct d_git_repo*  _repo,
    struct d_git_store* _store,
    const char*         _pack_dir,
    const char*         _idx_name
)
{
    char* const         idx_path = d_git_path_join(_pack_dir, _idx_name);
    struct d_git_pack   pack;
    struct d_git_buffer path;

    memset(&pack, 0, sizeof(pack));
    memset(&path, 0, sizeof(path));

    // the index's path
    if (!idx_path)
    {
        return false;
    }

    (void)d_git_buffer_add(&path, idx_path, strlen(idx_path) - 4u);
    (void)d_git_buffer_add_text(&path, ".pack");
    pack.path = d_git_buffer_take(&path);
    pack.idx  = (uint8_t*)d_git_file_read(idx_path, &pack.idx_length);
    free(idx_path);

    // an index that reads, and is laid out as it says
    if ( (!pack.path) ||
         (!pack.idx)  ||
         (!d_internal_index_layout(&pack, d_git_repo_hash_size(_repo))) )
    {
        const bool memory = (!pack.path);

        free(pack.idx);
        free(pack.path);

        return !memory;
    }

    struct d_git_pack* const grown = realloc(_store->packs,
                                             (_store->pack_count + 1u)
                                                 * sizeof(*grown));

    // room for one more
    if (!grown)
    {
        free(pack.idx);
        free(pack.path);

        return false;
    }

    _store->packs                     = grown;
    _store->packs[_store->pack_count] = pack;
    ++_store->pack_count;

    return true;
}


/*
d_internal_compare_names
  Orders names, so packs are searched in the same order each time.
*/
static int
d_internal_compare_names(
    const void* _left,
    const void* _right
)
{
    return strcmp(*(const char* const*)_left, *(const char* const*)_right);
}


/*
d_internal_packs
  A store's packs, read the first time an object is looked for in it: each
.idx in its pack directory.
*/
static bool
d_internal_packs(
    struct d_git_repo*  _repo,
    struct d_git_store* _store
)
{
    // read already
    if (_store->packs_read)
    {
        return true;
    }

    char* const pack_dir = d_git_path_join(_store->path, "pack");
    char**      names    = NULL;
    size_t      count    = 0u;
    bool        loaded   = true;

    _store->packs_read = true;

    // the directory's names, ordered
    if ( (pack_dir) &&
         (d_fs_names(pack_dir, &names, &count)) &&
         (count > 1u) )
    {
        qsort(names, count, sizeof(*names), d_internal_compare_names);
    }

    // each index
    for (size_t at = 0u; ( (loaded) && (at < count) ); ++at)
    {
        const size_t length = strlen(names[at]);

        // a name of an index
        if ( (length > 4u) &&
             (strcmp(&names[at][length - 4u], ".idx") == 0) )
        {
            loaded = d_internal_pack_load(_repo, _store, pack_dir, names[at]);
        }
    }

    d_fs_names_free(names, count);
    free(pack_dir);

    return (loaded) ? true : d_git_repo_fail(_repo, "out of memory", NULL);
}


/*
d_internal_pack_find
  The index of a name among a pack's, by the range its first byte's
fanout gives and a binary search within it; D_GIT_REPO_NONE for none.
*/
static size_t
d_internal_pack_find(
    const struct d_git_pack* _pack,
    const struct d_git_oid*  _oid,
    size_t                   _hash
)
{
    const uint8_t* const fanout = &_pack->idx[(_pack->version == 2u) ? 8u : 0u];
    const unsigned       first  = _oid->bytes[0];
    size_t               low    = (first == 0u)
                                  ? 0u
                                  : d_internal_be32(&fanout[(first - 1u) * 4u]);
    size_t               high   = d_internal_be32(&fanout[first * 4u]);

    // halving the range
    while ( (low < high) &&
            (high <= _pack->count) )
    {
        const size_t middle = low + ((high - low) / 2u);
        const int    order  = memcmp(&_pack->oids[middle * _pack->stride],
                                     _oid->bytes,
                                     _hash);

        // the name, or a half
        if (order == 0)
        {
            return middle;
        }
        else if (order < 0)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    return D_GIT_REPO_NONE;
}


/*
d_internal_pack_offset
  Where in the pack an object of its index begins: a place of 4 bytes, or,
its top bit set, one of 8 from the table of large places.
*/
static uint64_t
d_internal_pack_offset(
    const struct d_git_pack* _pack,
    size_t                   _index
)
{
    // version 1 keeps the place before the name
    if (_pack->version == 1u)
    {
        return d_internal_be32(&_pack->oids[(_index * _pack->stride) - 4u]);
    }

    const uint32_t small = d_internal_be32(&_pack->offsets[_index * 4u]);

    // a large place
    if ((small & 0x80000000u) != 0u)
    {
        return d_internal_be64(&_pack->large[(size_t)(small & 0x7FFFFFFFu)
                                             * 8u]);
    }

    return small;
}


/*
d_internal_compare_offsets
  Orders places in a pack.
*/
static int
d_internal_compare_offsets(
    const void* _left,
    const void* _right
)
{
    const uint64_t left  = *(const uint64_t*)_left;
    const uint64_t right = *(const uint64_t*)_right;

    return (left < right) ? -1 : ( (left > right) ? 1 : 0 );
}


/*
d_internal_pack_open
  The pack opened, its header checked against its index, and every
object's place in order, so each object's end is the next one's start.
*/
static bool
d_internal_pack_open(
    struct d_git_repo* _repo,
    struct d_git_pack* _pack
)
{
    // opened already
    if (_pack->file)
    {
        return true;
    }

    uint8_t header[12];

    _pack->file   = d_git_file_open(_pack->path, &_pack->file_size);
    _pack->sorted = malloc(((size_t)_pack->count + 1u)
                           * sizeof(*_pack->sorted));

    // the pack, its header, and room for its places
    if ( (!_pack->file)                                           ||
         (!_pack->sorted)                                         ||
         (!d_git_file_read_at(_pack->file, 0u, header, 12u))      ||
         (memcmp(header, "PACK", 4u) != 0)                        ||
         (d_internal_be32(&header[8]) != _pack->count) )
    {
        d_git_file_close(_pack->file);
        free(_pack->sorted);
        _pack->file   = NULL;
        _pack->sorted = NULL;

        return d_git_repo_fail(_repo, "cannot read pack", _pack->path);
    }

    // each place
    for (size_t at = 0u; at < _pack->count; ++at)
    {
        _pack->sorted[at] = d_internal_pack_offset(_pack, at);
    }

    qsort(_pack->sorted,
          _pack->count,
          sizeof(*_pack->sorted),
          d_internal_compare_offsets);

    return true;
}


/*
d_internal_pack_end
  Where the object at a place ends: at the next object's place, or the
pack's trailer.
*/
static uint64_t
d_internal_pack_end(
    const struct d_git_pack* _pack,
    uint64_t                 _offset,
    size_t                   _hash
)
{
    size_t low  = 0u;
    size_t high = _pack->count;

    // the first place past this one
    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        // past it, or not
        if (_pack->sorted[middle] > _offset)
        {
            high = middle;
        }
        else
        {
            low = middle + 1u;
        }
    }

    return (low < _pack->count) ? _pack->sorted[low]
                                : (_pack->file_size - _hash);
}


//==============================================================================
// 4.  PACKED OBJECTS
//==============================================================================


// d_internal_entry
//   struct: one object of a pack, its header read: its type -- a delta's
//   among them -- the size it inflates to, where its stream begins and
//   ends, and a delta's base: a place back along the pack, or a name.
struct d_internal_entry
{
    unsigned         type;
    uint64_t         size;
    uint64_t         offset;
    uint64_t         stream;
    uint64_t         end;
    uint64_t         base_offset;
    struct d_git_oid base_oid;
};

// d_internal_raw_object
//   struct: an object's type and bytes, allocated.
struct d_internal_raw_object
{
    enum d_git_object_type type;
    uint8_t*               data;
    size_t                 size;
};


/*
d_internal_entry_header
  An object's header at a place in a pack: its type and size, the size
seven bits a byte after the first four; then a delta's base -- a place
back, in git's offset encoding, or a name.
  Long as a pack entry's header -- its type, its size, and its base, by
offset or by name -- is read in one pass.
*/
static bool
d_internal_entry_header(
    const struct d_git_repo* _repo,
    const struct d_git_pack* _pack,
    uint64_t                 _offset,
    struct d_internal_entry* _out
)
{
    const size_t hash  = d_git_repo_hash_size(_repo);
    uint8_t      head[64];
    const uint64_t end = d_internal_pack_end(_pack, _offset, hash);
    const size_t want  = ((end - _offset) < sizeof(head))
                         ? (size_t)(end - _offset)
                         : sizeof(head);
    size_t       at    = 0u;

    memset(_out, 0, sizeof(*_out));

    // the header's bytes
    if ( (end <= _offset) ||
         (!d_git_file_read_at(_pack->file, _offset, head, want)) )
    {
        return false;
    }

    _out->offset = _offset;
    _out->end    = end;
    _out->type   = (head[0] >> 4) & 7u;
    _out->size   = head[0] & 15u;

    // the size's further bytes
    for (unsigned shift = 4u; ( (head[at] & 0x80u) != 0u ); shift += 7u)
    {
        // a header that runs on too long
        if ( (++at >= want) ||
             (shift > 57u) )
        {
            return false;
        }

        _out->size |= (uint64_t)(head[at] & 0x7Fu) << shift;
    }

    ++at;

    // a delta's base: a place back, or a name
    if (_out->type == D_INTERNAL_OFS_DELTA)
    {
        uint64_t back = head[at] & 0x7Fu;

        // each further byte, one more added before the shift
        while ( (head[at] & 0x80u) != 0u )
        {
            // a place that runs on too long
            if (++at >= want)
            {
                return false;
            }

            back = ((back + 1u) << 7) | (head[at] & 0x7Fu);
        }

        ++at;

        // a base before this object
        if ( (back == 0u) ||
             (back > _offset) )
        {
            return false;
        }

        _out->base_offset = _offset - back;
    }
    else if (_out->type == D_INTERNAL_REF_DELTA)
    {
        // the name, whole
        if ((at + hash) > want)
        {
            return false;
        }

        memcpy(_out->base_oid.bytes, &head[at], hash);
        at += hash;
    }

    _out->stream = _offset + at;

    return ( (_out->type != 0u) &&
             (_out->type != 5u) &&
             (_out->stream < end) );
}


/*
d_internal_entry_inflate
  An object's stream, read and inflated to the size its header says.
*/
static bool
d_internal_entry_inflate(
    const struct d_git_pack*       _pack,
    const struct d_internal_entry* _entry,
    struct d_git_inflated*         _out
)
{
    const uint64_t length = _entry->end - _entry->stream;
    uint8_t* const bytes  = ( (length > 0u) &&
                              (length <= (uint64_t)((size_t)-1 / 2u)) )
                            ? malloc((size_t)length)
                            : NULL;

    memset(_out, 0, sizeof(*_out));

    // the stream's bytes
    if ( (!bytes) ||
         (!d_git_file_read_at(_pack->file, _entry->stream, bytes,
                              (size_t)length)) )
    {
        free(bytes);

        return false;
    }

    const bool inflated = d_git_inflate(bytes,
                                        (size_t)length,
                                        (size_t)_entry->size,
                                        _out);

    free(bytes);

    // an object of nothing inflates to nothing
    if ( (inflated) &&
         (!_out->bytes) )
    {
        _out->bytes = malloc(1u);
    }

    return ( (inflated) &&
             (_out->bytes) );
}


/*
d_internal_varint
  A delta's size, seven bits a byte, least significant first.
*/
static bool
d_internal_varint(
    const uint8_t* _delta,
    size_t         _length,
    size_t*        _at,
    uint64_t*      _out
)
{
    *_out = 0u;

    // seven bits a byte, until a byte without its top bit
    for (unsigned shift = 0u; shift < 64u; shift += 7u)
    {
        // a delta that ends within it
        if (*_at >= _length)
        {
            return false;
        }

        const uint8_t byte = _delta[(*_at)++];

        *_out |= (uint64_t)(byte & 0x7Fu) << shift;

        // the last byte
        if ((byte & 0x80u) == 0u)
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_delta_copy
  A delta's copy: which of up to four bytes of place and three of size
follow, by the command's bits; a size of 0 is 64 KiB.
*/
static bool
d_internal_delta_copy(
    const uint8_t* _delta,
    size_t         _length,
    size_t*        _at,
    uint8_t        _command,
    uint64_t*      _out_offset,
    uint64_t*      _out_size
)
{
    *_out_offset = 0u;
    *_out_size   = 0u;

    // the place's bytes, then the size's, each present when its bit is
    for (unsigned bit = 0u; bit < 7u; ++bit)
    {
        // a byte not present
        if ((_command & (1u << bit)) == 0u)
        {
            continue;
        }

        // a delta that ends within it
        if (*_at >= _length)
        {
            return false;
        }

        const uint64_t byte = _delta[(*_at)++];

        // a place's byte, or a size's
        if (bit < 4u)
        {
            *_out_offset |= byte << (bit * 8u);
        }
        else
        {
            *_out_size |= byte << ((bit - 4u) * 8u);
        }
    }

    *_out_size = (*_out_size == 0u) ? 0x10000u : *_out_size;

    return true;
}


/*
d_internal_delta_apply
  A delta applied to its base, as git's patch_delta applies one: the base's
size and the result's, then copies from the base and inserts of the
delta's own bytes, until the result is its size.
  Long as a delta's two sizes and its copies and inserts are read in one
loop, each checked.
*/
static bool
d_internal_delta_apply(
    const uint8_t*                _base,
    size_t                        _base_size,
    const uint8_t*                _delta,
    size_t                        _length,
    struct d_internal_raw_object* _out
)
{
    size_t   at          = 0u;
    uint64_t base_size   = 0u;
    uint64_t result_size = 0u;

    // the sizes, the base's as it is
    if ( (!d_internal_varint(_delta, _length, &at, &base_size)) ||
         (!d_internal_varint(_delta, _length, &at, &result_size)) ||
         (base_size != _base_size) ||
         (result_size > (uint64_t)((size_t)-1 / 2u)) )
    {
        return false;
    }

    uint8_t* const out    = malloc((size_t)result_size + 1u);
    size_t         filled = 0u;

    // room for the result
    if (!out)
    {
        return false;
    }

    // each command
    while (at < _length)
    {
        const uint8_t command = _delta[at++];
        uint64_t      offset  = 0u;
        uint64_t      size    = command;

        // a copy from the base, an insert, or a command that is none
        if ( (command & 0x80u) != 0u )
        {
            // its place and size, within the base and the result
            if ( (!d_internal_delta_copy(_delta, _length, &at, command,
                                         &offset, &size)) ||
                 ((offset + size) > _base_size) ||
                 ((filled + size) > result_size) )
            {
                free(out);

                return false;
            }

            memcpy(&out[filled], &_base[offset], (size_t)size);
        }
        else if ( (command == 0u) ||
                  ((at + size) > _length) ||
                  ((filled + size) > result_size) )
        {
            free(out);

            return false;
        }
        else
        {
            memcpy(&out[filled], &_delta[at], (size_t)size);
            at += (size_t)size;
        }

        filled += (size_t)size;
    }

    // the result its size
    if (filled != result_size)
    {
        free(out);

        return false;
    }

    out[filled] = '\0';
    _out->data  = out;
    _out->size  = filled;

    return true;
}


/*
d_internal_cache_slot
  The slot a place's base is kept in.
*/
static size_t
d_internal_cache_slot(
    size_t   _store,
    size_t   _pack,
    uint64_t _offset
)
{
    const uint64_t mixed = (_offset * 0x9E3779B97F4A7C15u)
                           ^ ((uint64_t)_store << 40)
                           ^ ((uint64_t)_pack << 20);

    return (size_t)(mixed >> 56) % D_GIT_CACHE_SLOTS;
}


/*
d_internal_cache_find
  A base kept for a place, copied; false for none.
*/
static bool
d_internal_cache_find(
    const struct d_git_repo*      _repo,
    size_t                        _store,
    size_t                        _pack,
    uint64_t                      _offset,
    struct d_internal_raw_object* _out
)
{
    const struct d_git_cached* const cached =
        &_repo->cache[d_internal_cache_slot(_store, _pack, _offset)];

    // the place's
    if ( (!cached->data)           ||
         (cached->store != _store) ||
         (cached->pack != _pack)   ||
         (cached->offset != _offset) )
    {
        return false;
    }

    _out->data = malloc(cached->size + 1u);

    // room for the copy
    if (!_out->data)
    {
        return false;
    }

    memcpy(_out->data, cached->data, cached->size + 1u);
    _out->type = cached->type;
    _out->size = cached->size;

    return true;
}


/*
d_internal_cache_keep
  Keeps a base for its place, in place of what its slot held, unless it is
too large to keep.
*/
static void
d_internal_cache_keep(
    struct d_git_repo*                  _repo,
    size_t                              _store,
    size_t                              _pack,
    uint64_t                            _offset,
    const struct d_internal_raw_object* _object
)
{
    struct d_git_cached* const cached =
        &_repo->cache[d_internal_cache_slot(_store, _pack, _offset)];

    const size_t after = _repo->cache_bytes - ( (cached->data) ? cached->size
                                                               : 0u )
                         + _object->size;

    // too large to keep, alone or with the rest
    if ( (_object->size > D_INTERNAL_CACHE_MOST) ||
         (after > D_INTERNAL_CACHE_TOTAL) )
    {
        return;
    }

    uint8_t* const copy = malloc(_object->size + 1u);

    // memory that ran out keeps nothing
    if (!copy)
    {
        return;
    }

    memcpy(copy, _object->data, _object->size + 1u);
    free(cached->data);
    _repo->cache_bytes = after;
    cached->store  = _store;
    cached->pack   = _pack;
    cached->offset = _offset;
    cached->type   = _object->type;
    cached->data   = copy;
    cached->size   = _object->size;

    return;
}


static bool d_internal_read_any(struct d_git_repo*            _repo,
                                const struct d_git_oid*       _oid,
                                struct d_internal_raw_object* _out);


static bool d_internal_packed_at(struct d_git_repo*            _repo,
                                 size_t                        _store,
                                 size_t                        _pack,
                                 uint64_t                      _offset,
                                 struct d_internal_raw_object* _out);


/*
d_internal_delta_base
  The base a delta needs: back along its pack, or named anywhere.
*/
static bool
d_internal_delta_base(
    struct d_git_repo*             _repo,
    size_t                         _store,
    size_t                         _pack,
    const struct d_internal_entry* _entry,
    struct d_internal_raw_object*  _out
)
{
    // a place back along the pack, or a name
    if (_entry->type == D_INTERNAL_OFS_DELTA)
    {
        return d_internal_packed_at(_repo, _store, _pack,
                                    _entry->base_offset, _out);
    }

    return d_internal_read_any(_repo, &_entry->base_oid, _out);
}


/*
d_internal_chain_apply
  A chain of deltas applied outward from their base: each one's delta
inflated and applied to what the one before it made, each result kept as
a base for what comes after.
*/
static bool
d_internal_chain_apply(
    struct d_git_repo*             _repo,
    size_t                         _store,
    size_t                         _pack,
    const struct d_internal_entry* _chain,
    size_t                         _count,
    struct d_internal_raw_object*  _object
)
{
    struct d_git_pack* const pack = &_repo->stores[_store].packs[_pack];

    // from the base outward
    for (size_t at = _count; at-- > 0u;)
    {
        struct d_git_inflated        delta;
        struct d_internal_raw_object result;

        memset(&result, 0, sizeof(result));

        // the delta, inflated and applied
        if ( (!d_internal_entry_inflate(pack, &_chain[at], &delta)) ||
             (!d_internal_delta_apply(_object->data,
                                      _object->size,
                                      delta.bytes,
                                      delta.length,
                                      &result)) )
        {
            free(delta.bytes);

            return d_git_repo_fail(_repo, "bad delta in pack", pack->path);
        }

        free(delta.bytes);
        free(_object->data);
        result.type = _object->type;
        *_object    = result;
        d_internal_cache_keep(_repo, _store, _pack, _chain[at].offset,
                              _object);
    }

    return true;
}


/*
d_internal_chain_grow
  Room for one more delta in a chain.
*/
static bool
d_internal_chain_grow(
    struct d_internal_entry** _chain,
    size_t                    _count,
    size_t*                   _capacity
)
{
    // room left
    if (_count < *_capacity)
    {
        return true;
    }

    const size_t                   wanted = (*_capacity > 0u)
                                            ? (*_capacity * 2u)
                                            : 16u;
    struct d_internal_entry* const grown  = realloc(*_chain,
                                                    wanted * sizeof(*grown));

    // memory that ran out
    if (!grown)
    {
        return false;
    }

    *_chain    = grown;
    *_capacity = wanted;

    return true;
}


/*
d_internal_packed_at
  The object at a place in a pack: its deltas walked back, place by place,
to a base -- an object of its own, one kept, or one named -- then applied
outward. A base named is read by name, which may lead to another pack.
  Long as a chain of deltas is followed to its base, from the cache or the
pack, before it is applied.
*/
static bool
d_internal_packed_at(
    struct d_git_repo*            _repo,
    size_t                        _store,
    size_t                        _pack,
    uint64_t                      _offset,
    struct d_internal_raw_object* _out
)
{
    struct d_git_pack* const pack     = &_repo->stores[_store].packs[_pack];
    struct d_internal_entry* chain    = NULL;
    size_t                   count    = 0u;
    size_t                   capacity = 0u;
    uint64_t                 at       = _offset;
    bool                     based    = false;

    memset(_out, 0, sizeof(*_out));

    // back along the deltas, until a base
    while (!based)
    {
        struct d_internal_entry entry;

        // one kept, or the entry's header
        if (d_internal_cache_find(_repo, _store, _pack, at, _out))
        {
            break;
        }
        else if ( (!d_internal_entry_header(_repo, pack, at, &entry)) ||
                  (!d_internal_chain_grow(&chain, count, &capacity)) )
        {
            free(chain);

            return d_git_repo_fail(_repo, "bad object in pack", pack->path);
        }

        // a delta goes back; an object of its own is the base
        if (entry.type >= D_INTERNAL_OFS_DELTA)
        {
            chain[count++] = entry;

            // a place back goes on back; a named base ends the walk
            if (entry.type == D_INTERNAL_OFS_DELTA)
            {
                at = entry.base_offset;

                continue;
            }

            // the named base, read
            if (!d_internal_delta_base(_repo, _store, _pack, &entry, _out))
            {
                free(chain);

                return false;
            }

            break;
        }

        struct d_git_inflated inflated;

        // the base itself
        if (!d_internal_entry_inflate(pack, &entry, &inflated))
        {
            free(chain);

            return d_git_repo_fail(_repo, "bad object in pack", pack->path);
        }

        _out->type = (enum d_git_object_type)entry.type;
        _out->data = inflated.bytes;
        _out->size = inflated.length;
        d_internal_cache_keep(_repo, _store, _pack, at, _out);
        based      = true;
    }

    const bool applied = d_internal_chain_apply(_repo, _store, _pack, chain,
                                                count, _out);

    free(chain);

    // a chain that would not apply leaves nothing
    if (!applied)
    {
        free(_out->data);
        memset(_out, 0, sizeof(*_out));
    }

    return applied;
}


//==============================================================================
// 5.  LOOSE OBJECTS
//==============================================================================


/*
d_internal_loose_path
  Where a loose object would be: its name's first two digits a directory,
the rest the file.
*/
static char*
d_internal_loose_path(
    const struct d_git_repo* _repo,
    const char*              _objects,
    const struct d_git_oid*  _oid
)
{
    char hex[(2u * D_GIT_OID_MOST) + 2u];

    d_internal_hex(_repo, _oid, hex);
    memmove(&hex[3], &hex[2], strlen(&hex[2]) + 1u);
    hex[2] = '/';

    return d_git_path_join(_objects, hex);
}


/*
d_internal_loose_header
  A loose object's header, "type size" and a NUL: its type, its size, and
how long it is; false for one git would not read.
*/
static bool
d_internal_loose_header(
    const uint8_t*          _bytes,
    size_t                  _length,
    enum d_git_object_type* _out_type,
    size_t*                 _out_size,
    size_t*                 _out_used
)
{
    const uint8_t* const space = memchr(_bytes, ' ', (_length < 16u)
                                                     ? _length
                                                     : 16u);
    const uint8_t* const nul   = memchr(_bytes, '\0', (_length < 64u)
                                                      ? _length
                                                      : 64u);
    char                 type[16];

    // a type, a space, digits, and a NUL
    if ( (!space) ||
         (!nul)   ||
         (nul < space) ||
         ((size_t)(space - _bytes) >= sizeof(type)) )
    {
        return false;
    }

    memcpy(type, _bytes, (size_t)(space - _bytes));
    type[space - _bytes] = '\0';
    *_out_type = d_git_object_type_named(type);
    *_out_size = 0u;

    // the size, in decimal
    for (const uint8_t* at = space + 1; at < nul; ++at)
    {
        // a digit
        if ( (*at < '0') ||
             (*at > '9') ||
             (*_out_size > ((size_t)-1 / 10u)) )
        {
            return false;
        }

        *_out_size = (*_out_size * 10u) + (size_t)(*at - '0');
    }

    *_out_used = (size_t)(nul - _bytes) + 1u;

    return ( (*_out_type != D_GIT_OBJECT_NONE) &&
             (nul > (space + 1)) );
}


/*
d_internal_loose_read
  A loose object of a store: inflated, its header read, its size as the
header says, its name the hash of all it is. False with nothing said when
it is not there; said when it is, but is unsound.
*/
static bool
d_internal_loose_read(
    struct d_git_repo*            _repo,
    const char*                   _objects,
    const struct d_git_oid*       _oid,
    struct d_internal_raw_object* _out,
    bool*                         _out_there
)
{
    char* const    path   = d_internal_loose_path(_repo, _objects, _oid);
    size_t         length = 0u;
    uint8_t* const bytes  = (path) ? (uint8_t*)d_git_file_read(path, &length)
                                   : NULL;
    struct d_git_inflated inflated;

    free(path);
    *_out_there = (bytes != NULL);

    // not there
    if (!bytes)
    {
        return false;
    }

    const bool read = d_git_inflate(bytes, length, 0u, &inflated);
    size_t     size = 0u;
    size_t     used = 0u;
    struct d_git_oid named;

    free(bytes);

    // a stream, a header, and the size it says
    if ( (!read) ||
         (!d_internal_loose_header(inflated.bytes, inflated.length,
                                   &_out->type, &size, &used)) ||
         ((used + size) != inflated.length) )
    {
        free(inflated.bytes);

        return d_git_repo_fail(_repo, "loose object is corrupt", NULL);
    }

    d_git_repo_hash_object(_repo, _out->type, &inflated.bytes[used], size,
                           &named);

    // the name it hashes to
    if (!d_git_repo_same_oid(_repo, &named, _oid))
    {
        free(inflated.bytes);

        return d_git_repo_fail(_repo, "loose object does not hash to its "
                                      "name", NULL);
    }

    memmove(inflated.bytes, &inflated.bytes[used], size);

    uint8_t* const data = realloc(inflated.bytes, size + 1u);

    _out->data = (data) ? data : inflated.bytes;
    _out->data[size] = '\0';
    _out->size = size;

    return true;
}


//==============================================================================
// 6.  READING
//==============================================================================


/*
d_internal_read_any
  An object, by name, from the first store that has it: loose there, then
in each of its packs.
*/
static bool
d_internal_read_any(
    struct d_git_repo*            _repo,
    const struct d_git_oid*       _oid,
    struct d_internal_raw_object* _out
)
{
    const size_t hash = d_git_repo_hash_size(_repo);

    memset(_out, 0, sizeof(*_out));

    // the stores found
    if (!d_internal_stores(_repo))
    {
        return false;
    }

    // each store in turn
    for (size_t store = 0u; store < _repo->store_count; ++store)
    {
        bool there = false;

        // loose
        if ( (d_internal_loose_read(_repo, _repo->stores[store].path, _oid,
                                    _out, &there)) ||
             (there) )
        {
            return ( (there) && (_out->data != NULL) );
        }

        // the store's packs
        if (!d_internal_packs(_repo, &_repo->stores[store]))
        {
            return false;
        }

        // each pack
        for (size_t pack = 0u; pack < _repo->stores[store].pack_count; ++pack)
        {
            struct d_git_pack* const record = &_repo->stores[store].packs[pack];
            const size_t             index  = d_internal_pack_find(record,
                                                                   _oid,
                                                                   hash);

            // the object's place, read
            if (index != D_GIT_REPO_NONE)
            {
                return ( (d_internal_pack_open(_repo, record)) &&
                         (d_internal_packed_at(_repo, store, pack,
                                               d_internal_pack_offset(record,
                                                                      index),
                                               _out)) );
            }
        }
    }

    return d_git_repo_fail(_repo, "no such object", NULL);
}


bool
d_git_repo_read(
    struct d_git_repo*      _repo,
    const struct d_git_oid* _oid,
    struct d_git_object*    _out
)
{
    struct d_internal_raw_object raw;

    memset(_out, 0, sizeof(*_out));

    // a repository and a name
    if ( (!_repo) ||
         (!_oid) )
    {
        return false;
    }

    // the object, of a type there is
    if ( (!d_internal_read_any(_repo, _oid, &raw)) ||
         (raw.type < D_GIT_OBJECT_COMMIT) ||
         (raw.type > D_GIT_OBJECT_TAG) )
    {
        free(raw.data);

        // a failure says why
        if (_repo->problem[0] == '\0')
        {
            (void)d_git_repo_fail(_repo, "no such object", NULL);
        }

        return false;
    }

    _out->type = raw.type;
    _out->data = raw.data;
    _out->size = raw.size;

    return true;
}


bool
d_git_repo_has(
    struct d_git_repo*      _repo,
    const struct d_git_oid* _oid
)
{
    const size_t hash = d_git_repo_hash_size(_repo);

    // a repository, a name, and the stores
    if ( (!_repo) ||
         (!_oid) ||
         (!d_internal_stores(_repo)) )
    {
        return false;
    }

    // each store, loose then packed
    for (size_t store = 0u; store < _repo->store_count; ++store)
    {
        char* const path  = d_internal_loose_path(_repo,
                                                  _repo->stores[store].path,
                                                  _oid);
        struct d_git_file_info info;
        const bool  loose = ( (path) && (d_git_file_info(path, &info)) );

        free(path);

        // loose, or the store's packs read
        if (loose)
        {
            return true;
        }
        else if (!d_internal_packs(_repo, &_repo->stores[store]))
        {
            return false;
        }

        // each pack's index
        for (size_t pack = 0u; pack < _repo->stores[store].pack_count; ++pack)
        {
            // the name among it
            if (d_internal_pack_find(&_repo->stores[store].packs[pack], _oid,
                                     hash) != D_GIT_REPO_NONE)
            {
                return true;
            }
        }
    }

    return false;
}


//==============================================================================
// 7.  PARSING
//==============================================================================


/*
d_internal_parse_name
  A name in hex at the start of text, of the repository's length.
*/
static bool
d_internal_parse_name(
    const struct d_git_repo* _repo,
    const char*              _text,
    size_t                   _length,
    struct d_git_oid*        _out
)
{
    const size_t size = d_git_repo_hash_size(_repo);
    char         hex[(2u * D_GIT_OID_MOST) + 1u];

    // the digits there
    if (_length < (size * 2u))
    {
        return false;
    }

    memcpy(hex, _text, size * 2u);
    hex[size * 2u] = '\0';

    return d_git_repo_parse_hex(_repo, hex, _out);
}


/*
d_internal_parse_person
  An author, committer or tagger, as git's split_ident_line reads one:
the name before ` <`, the address between the angles, then the time and
the zone.
  Long as git's split_ident_line is, which it follows: the name, the
address, the time and the zone.
*/
static void
d_internal_parse_person(
    const char*          _text,
    size_t               _length,
    struct d_git_person* _out
)
{
    const char* const open  = memchr(_text, '<', _length);
    const char* const close = (open) ? memchr(open, '>',
                                              _length
                                                  - (size_t)(open - _text))
                                     : NULL;

    memset(_out, 0, sizeof(*_out));

    // an address in angles
    if (!close)
    {
        _out->name        = _text;
        _out->name_length = _length;

        return;
    }

    size_t name = (size_t)(open - _text);

    // the spaces before the angle are not the name's
    while ( (name > 0u) &&
            (_text[name - 1u] == ' ') )
    {
        --name;
    }

    _out->name         = _text;
    _out->name_length  = name;
    _out->email        = open + 1;
    _out->email_length = (size_t)(close - open) - 1u;

    const char* at  = close + 1;
    const char* end = _text + _length;

    // the spaces before the time
    while ( (at < end) &&
            (*at == ' ') )
    {
        ++at;
    }

    // the time's digits
    while ( (at < end) &&
            (*at >= '0') &&
            (*at <= '9') )
    {
        _out->time = (_out->time * 10) + (*at - '0');
        ++at;
    }

    // the spaces before the zone
    while ( (at < end) &&
            (*at == ' ') )
    {
        ++at;
    }

    // a zone: a sign and four digits
    if ( ((end - at) >= 5) &&
         ( (*at == '+') || (*at == '-') ) )
    {
        const int hours   = ((at[1] - '0') * 10) + (at[2] - '0');
        const int minutes = ((at[3] - '0') * 10) + (at[4] - '0');

        _out->offset = ((*at == '-') ? -1 : 1) * ((hours * 60) + minutes);
        memcpy(_out->zone, at, 5u);
        _out->zone[5] = '\0';
    }

    return;
}


/*
d_internal_header_line
  The next header line of a commit or tag from `*_at`: its key and value,
`*_at` moved past it and the lines that continue it, which begin with a
space. False at the blank line that ends the headers, or the content's end.
*/
static bool
d_internal_header_line(
    const struct d_git_object* _object,
    size_t*                    _at,
    const char**               _out_value,
    size_t*                    _out_value_length,
    char*                      _out_key,
    size_t                     _key_capacity
)
{
    const char* const text   = (const char*)_object->data;
    const size_t      length = _object->size;

    // the headers' end
    if ( (*_at >= length) ||
         (text[*_at] == '\n') )
    {
        return false;
    }

    const char* const line  = &text[*_at];
    const char* const end   = memchr(line, '\n', length - *_at);
    const size_t      size  = (end) ? (size_t)(end - line)
                                    : (length - *_at);
    const char* const space = memchr(line, ' ', size);
    const size_t      key   = (space) ? (size_t)(space - line) : size;

    // the key, as much as fits
    memcpy(_out_key, line, (key < _key_capacity) ? key : (_key_capacity - 1u));
    _out_key[(key < _key_capacity) ? key : (_key_capacity - 1u)] = '\0';
    *_out_value        = (space) ? (space + 1) : (line + size);
    *_out_value_length = (space) ? (size - key - 1u) : 0u;
    *_at              += size + ((end) ? 1u : 0u);

    // the lines that continue it
    while ( (*_at < length) &&
            (text[*_at] == ' ') )
    {
        const char* const next = memchr(&text[*_at], '\n', length - *_at);

        *_at = (next) ? ((size_t)(next - text) + 1u) : length;
    }

    return true;
}


/*
d_internal_commit_header
  One header of a commit kept: its tree, a parent, its author, committer
and encoding; any other passed over.
*/
static bool
d_internal_commit_header(
    const struct d_git_repo* _repo,
    const char*              _key,
    const char*              _value,
    size_t                   _length,
    struct d_git_commit*     _out
)
{
    // each header git reads
    if (strcmp(_key, "tree") == 0)
    {
        return d_internal_parse_name(_repo, _value, _length, &_out->tree);
    }
    else if (strcmp(_key, "parent") == 0)
    {
        struct d_git_oid* const grown = realloc(_out->parents,
                                                (_out->parent_count + 1u)
                                                    * sizeof(*grown));

        // room, and the name
        if (!grown)
        {
            return false;
        }

        _out->parents = grown;

        return ( (d_internal_parse_name(_repo, _value, _length,
                                        &_out->parents[_out->parent_count]))
                 && (++_out->parent_count > 0u) );
    }
    else if (strcmp(_key, "author") == 0)
    {
        d_internal_parse_person(_value, _length, &_out->author);
    }
    else if (strcmp(_key, "committer") == 0)
    {
        d_internal_parse_person(_value, _length, &_out->committer);
    }
    else if (strcmp(_key, "encoding") == 0)
    {
        _out->encoding        = _value;
        _out->encoding_length = _length;
    }

    return true;
}


bool
d_git_repo_parse_commit(
    const struct d_git_repo*   _repo,
    const struct d_git_object* _object,
    struct d_git_commit*       _out
)
{
    size_t      at     = 0u;
    const char* value  = NULL;
    size_t      length = 0u;
    char        key[16];
    bool        tree   = false;

    memset(_out, 0, sizeof(*_out));

    // a commit's content
    if ( (!_object) ||
         (_object->type != D_GIT_OBJECT_COMMIT) ||
         (strncmp((const char*)_object->data, "tree ", 5u) != 0) )
    {
        return false;
    }

    // each header
    while (d_internal_header_line(_object, &at, &value, &length, key,
                                  sizeof(key)))
    {
        tree = ( (tree) || (strcmp(key, "tree") == 0) );

        // one that will not parse
        if (!d_internal_commit_header(_repo, key, value, length, _out))
        {
            d_git_commit_release(_out);

            return false;
        }
    }

    // the message, after the blank line
    at                   += (at < _object->size) ? 1u : 0u;
    _out->message         = (const char*)&_object->data[at];
    _out->message_length  = _object->size - at;

    return tree;
}


void
d_git_commit_release(
    struct d_git_commit* _commit
)
{
    // nothing to release
    if (!_commit)
    {
        return;
    }

    free(_commit->parents);
    memset(_commit, 0, sizeof(*_commit));

    return;
}


bool
d_git_repo_parse_tag(
    const struct d_git_repo*   _repo,
    const struct d_git_object* _object,
    struct d_git_tag*          _out
)
{
    size_t      at     = 0u;
    const char* value  = NULL;
    size_t      length = 0u;
    char        key[16];
    char        type[16];
    bool        object = false;

    memset(_out, 0, sizeof(*_out));

    // a tag's content
    if ( (!_object) ||
         (_object->type != D_GIT_OBJECT_TAG) )
    {
        return false;
    }

    // each header
    while (d_internal_header_line(_object, &at, &value, &length, key,
                                  sizeof(key)))
    {
        // the object, its type, the name, the tagger
        if (strcmp(key, "object") == 0)
        {
            object = d_internal_parse_name(_repo, value, length,
                                           &_out->object);
        }
        else if ( (strcmp(key, "type") == 0) &&
                  (length < sizeof(type)) )
        {
            memcpy(type, value, length);
            type[length] = '\0';
            _out->type   = d_git_object_type_named(type);
        }
        else if (strcmp(key, "tag") == 0)
        {
            _out->name        = value;
            _out->name_length = length;
        }
        else if (strcmp(key, "tagger") == 0)
        {
            _out->has_tagger = true;
            d_internal_parse_person(value, length, &_out->tagger);
        }
    }

    at                   += (at < _object->size) ? 1u : 0u;
    _out->message         = (const char*)&_object->data[at];
    _out->message_length  = _object->size - at;

    return ( (object) &&
             (_out->type != D_GIT_OBJECT_NONE) );
}


bool
d_git_repo_tree_next(
    const struct d_git_repo*   _repo,
    const struct d_git_object* _tree,
    size_t*                    _at,
    struct d_git_tree_entry*   _out
)
{
    const size_t      hash   = d_git_repo_hash_size(_repo);
    const char* const text   = (_tree) ? (const char*)_tree->data : NULL;
    const size_t      length = (_tree) ? _tree->size : 0u;

    memset(_out, 0, sizeof(*_out));

    // an entry left
    if ( (!text) ||
         (*_at >= length) )
    {
        return false;
    }

    size_t at = *_at;

    // the mode, in octal, to its space
    while ( (at < length) &&
            (text[at] >= '0') &&
            (text[at] <= '7') )
    {
        _out->mode = (_out->mode << 3) | (uint32_t)(text[at] - '0');
        ++at;
    }

    // a space, a name, its NUL, and the name's bytes
    if ( (at == *_at) ||
         (at >= length) ||
         (text[at] != ' ') )
    {
        return false;
    }

    const char* const name = &text[at + 1u];
    const char* const nul  = memchr(name, '\0', length - at - 1u);

    // a name of something, and a whole name after it
    if ( (!nul) ||
         (nul == name) ||
         ((size_t)(nul - text) + 1u + hash > length) )
    {
        return false;
    }

    _out->name        = name;
    _out->name_length = (size_t)(nul - name);
    memcpy(_out->oid.bytes, nul + 1, hash);
    *_at = (size_t)(nul - text) + 1u + hash;

    return true;
}


bool
d_git_tree_find(
    struct d_git_repo*      _repo,
    const struct d_git_oid* _tree,
    const char*             _path,
    struct d_git_oid*       _out,
    uint32_t*               _out_mode
)
{
    struct d_git_oid at   = *_tree;
    const char*      name = _path;

    // each name of the path
    while (*name != '\0')
    {
        const char* const slash  = strchr(name, '/');
        const size_t      length = (slash) ? (size_t)(slash - name)
                                           : strlen(name);
        struct d_git_object     tree;
        struct d_git_tree_entry entry;
        size_t                  offset = 0u;
        bool                    found  = false;

        // an empty name is passed over
        if (length == 0u)
        {
            name = (slash) ? (slash + 1) : (name + length);

            continue;
        }

        // the tree there
        if ( (!d_git_repo_read(_repo, &at, &tree)) ||
             (tree.type != D_GIT_OBJECT_TREE) )
        {
            d_git_object_release(&tree);

            return false;
        }

        // its entry of the name
        while ( (!found) &&
                (d_git_repo_tree_next(_repo, &tree, &offset, &entry)) )
        {
            found = ( (entry.name_length == length) &&
                      (memcmp(entry.name, name, length) == 0) );
        }

        d_git_object_release(&tree);

        // the entry, a tree when more of the path follows
        if ( (!found) ||
             ( (slash) && (slash[1] != '\0') &&
               (entry.mode != 040000u) ) )
        {
            return false;
        }

        at        = entry.oid;
        *_out_mode = entry.mode;
        name       = (slash) ? (slash + 1) : (name + length);
    }

    *_out = at;

    return true;
}


//==============================================================================
// 8.  PREFIXES
//==============================================================================


// d_internal_prefix
//   struct: a prefix being looked for: its bytes, the length in hex
//   digits, and what has been found -- the first name, and whether
//   another differs from it.
struct d_internal_prefix
{
    struct d_git_oid bytes;
    size_t           digits;
    struct d_git_oid first;
    size_t           found;
};


/*
d_internal_prefix_matches
  Whether a name begins with the prefix's digits.
*/
static bool
d_internal_prefix_matches(
    const struct d_internal_prefix* _prefix,
    const uint8_t*                  _name
)
{
    const size_t whole = _prefix->digits / 2u;

    // the whole bytes, then a last half
    if (memcmp(_name, _prefix->bytes.bytes, whole) != 0)
    {
        return false;
    }

    return ( ((_prefix->digits % 2u) == 0u) ||
             ((_name[whole] & 0xF0u)
                  == (_prefix->bytes.bytes[whole] & 0xF0u)) );
}


/*
d_internal_prefix_found
  A name the prefix matches, counted once however many places hold it.
*/
static void
d_internal_prefix_found(
    const struct d_git_repo*  _repo,
    struct d_internal_prefix* _prefix,
    const struct d_git_oid*   _oid
)
{
    // the first, or one that differs from it
    if (_prefix->found == 0u)
    {
        _prefix->first = *_oid;
        _prefix->found = 1u;
    }
    else if (!d_git_repo_same_oid(_repo, &_prefix->first, _oid))
    {
        _prefix->found = 2u;
    }

    return;
}


/*
d_internal_prefix_loose
  The loose names of a store the prefix matches: its first two digits a
directory, the rest each file's name.
*/
static void
d_internal_prefix_loose(
    const struct d_git_repo*  _repo,
    const char*               _objects,
    struct d_internal_prefix* _prefix
)
{
    static const char digits[] = "0123456789abcdef";
    char              directory[3];

    directory[0] = digits[_prefix->bytes.bytes[0] >> 4];
    directory[1] = digits[_prefix->bytes.bytes[0] & 15u];
    directory[2] = '\0';

    char* const path  = d_git_path_join(_objects, directory);
    char**      names = NULL;
    size_t      count = 0u;

    // the directory's names
    if ( (!path) ||
         (!d_fs_names(path, &names, &count)) )
    {
        free(path);

        return;
    }

    // each a name in hex, the rest of an object's
    for (size_t at = 0u; ( (_prefix->found < 2u) && (at < count) ); ++at)
    {
        char             hex[(2u * D_GIT_OID_MOST) + 1u];
        struct d_git_oid oid;

        (void)snprintf(hex, sizeof(hex), "%s%s", directory, names[at]);

        // a name, which the prefix matches
        if ( (d_git_repo_parse_hex(_repo, hex, &oid)) &&
             (d_internal_prefix_matches(_prefix, oid.bytes)) )
        {
            d_internal_prefix_found(_repo, _prefix, &oid);
        }
    }

    d_fs_names_free(names, count);
    free(path);

    return;
}


/*
d_internal_prefix_pack
  The names of a pack the prefix matches: from the first not before the
prefix, by binary search, while they match.
*/
static void
d_internal_prefix_pack(
    const struct d_git_repo*  _repo,
    const struct d_git_pack*  _pack,
    struct d_internal_prefix* _prefix
)
{
    const size_t hash = d_git_repo_hash_size(_repo);
    size_t       low  = 0u;
    size_t       high = _pack->count;

    // the first name not before the prefix's bytes
    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        // before it, or not
        if (memcmp(&_pack->oids[middle * _pack->stride],
                   _prefix->bytes.bytes,
                   hash) < 0)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    // each that matches
    for (size_t at = low;
         ( (_prefix->found < 2u) &&
           (at < _pack->count) &&
           (d_internal_prefix_matches(_prefix,
                                      &_pack->oids[at * _pack->stride])) );
         ++at)
    {
        struct d_git_oid oid;

        memset(&oid, 0, sizeof(oid));
        memcpy(oid.bytes, &_pack->oids[at * _pack->stride], hash);
        d_internal_prefix_found(_repo, _prefix, &oid);
    }

    return;
}


/*
d_internal_prefix_search
  Every store searched for the prefix, loose and packed, until two names
are found.
*/
static bool
d_internal_prefix_search(
    struct d_git_repo*        _repo,
    struct d_internal_prefix* _prefix
)
{
    // the stores
    if (!d_internal_stores(_repo))
    {
        return false;
    }

    // each store
    for (size_t store = 0u;
         ( (_prefix->found < 2u) && (store < _repo->store_count) );
         ++store)
    {
        d_internal_prefix_loose(_repo, _repo->stores[store].path, _prefix);

        // its packs
        if (!d_internal_packs(_repo, &_repo->stores[store]))
        {
            return false;
        }

        // each pack
        for (size_t pack = 0u;
             ( (_prefix->found < 2u) &&
               (pack < _repo->stores[store].pack_count) );
             ++pack)
        {
            d_internal_prefix_pack(_repo, &_repo->stores[store].packs[pack],
                                   _prefix);
        }
    }

    return true;
}


/*
d_internal_prefix_of
  A prefix read from hex digits: whole bytes, and a last half; the rest of
its bytes 0, so a search begins at the first name it could match.
*/
static bool
d_internal_prefix_of(
    const char*               _hex,
    size_t                    _length,
    struct d_internal_prefix* _out
)
{
    memset(_out, 0, sizeof(*_out));
    _out->digits = _length;

    // each digit, in either case
    for (size_t at = 0u; at < _length; ++at)
    {
        const char c     = _hex[at];
        const int  worth = ( (c >= '0') && (c <= '9') ) ? (c - '0')
                         : ( (c >= 'a') && (c <= 'f') ) ? (c - 'a' + 10)
                         : ( (c >= 'A') && (c <= 'F') ) ? (c - 'A' + 10)
                         : -1;

        // a digit that is not one
        if (worth < 0)
        {
            return false;
        }

        _out->bytes.bytes[at / 2u] |= (uint8_t)(((at % 2u) == 0u)
                                                ? (worth << 4)
                                                : worth);
    }

    return true;
}


bool
d_git_repo_prefix_find(
    struct d_git_repo* _repo,
    const char*        _hex,
    size_t             _length,
    struct d_git_oid*  _out
)
{
    struct d_internal_prefix prefix;

    // a prefix in hex, searched for
    if ( (!d_internal_prefix_of(_hex, _length, &prefix)) ||
         (!d_internal_prefix_search(_repo, &prefix)) )
    {
        return false;
    }

    // one name alone
    if (prefix.found != 1u)
    {
        return d_git_repo_fail(_repo,
                               (prefix.found == 0u)
                               ? "no object of that prefix"
                               : "short object ID is ambiguous",
                               NULL);
    }

    *_out = prefix.first;

    return true;
}


/*
d_internal_auto_abbrev
  The length core.abbrev's auto gives, as git's find_unique_abbrev counts
it: half the bits of how many objects the packs hold, rounded up, but no
fewer than 7.
*/
static size_t
d_internal_auto_abbrev(
    struct d_git_repo* _repo
)
{
    uint64_t count = 0u;

    // every pack's objects
    for (size_t store = 0u;
         ( (d_internal_stores(_repo)) && (store < _repo->store_count) );
         ++store)
    {
        // the store's packs
        if (!d_internal_packs(_repo, &_repo->stores[store]))
        {
            break;
        }

        // each pack's count
        for (size_t pack = 0u; pack < _repo->stores[store].pack_count; ++pack)
        {
            count += _repo->stores[store].packs[pack].count;
        }
    }

    unsigned bits = 0u;

    // the highest bit set, from 0
    while ( (count >> (bits + 1u)) != 0u )
    {
        ++bits;
    }

    const size_t length = ((size_t)bits + 2u) / 2u;

    return (length < 7u) ? 7u : length;
}


/*
d_internal_least_abbrev
  The length to begin abbreviating at: the one asked for, or core.abbrev's
-- a number, "auto", or "no" for the whole name.
*/
static size_t
d_internal_least_abbrev(
    struct d_git_repo* _repo,
    size_t             _least
)
{
    bool              set   = false;
    const char* const value = d_git_repo_config(_repo, "core.abbrev", &set);
    int64_t           count = 0;
    const size_t      most  = d_git_repo_hash_size(_repo) * 2u;

    // asked for, or a setting
    if (_least > 0u)
    {
        return (_least > most) ? most : _least;
    }
    else if ( (set) &&
              (value) &&
              (strcmp(value, "no") == 0) )
    {
        return most;
    }
    else if ( (set) &&
              (value) &&
              (d_git_text_int(value, &count)) &&
              (count >= 4) )
    {
        return ((size_t)count > most) ? most : (size_t)count;
    }

    return d_internal_auto_abbrev(_repo);
}


bool
d_git_repo_abbreviate(
    struct d_git_repo*      _repo,
    const struct d_git_oid* _oid,
    size_t                  _least,
    char*                   _out
)
{
    const size_t most = d_git_repo_hash_size(_repo) * 2u;
    char         hex[(2u * D_GIT_OID_MOST) + 1u];

    d_git_repo_hex(_repo, _oid, hex);

    // longer, until it names this object alone
    for (size_t length = d_internal_least_abbrev(_repo, _least);
         length <= most;
         ++length)
    {
        struct d_internal_prefix prefix;

        // a prefix that names one object, or none yet
        if ( (!d_internal_prefix_of(hex, length, &prefix)) ||
             (!d_internal_prefix_search(_repo, &prefix)) )
        {
            return false;
        }

        // this one alone, or nothing at all of it
        if (prefix.found <= 1u)
        {
            memcpy(_out, hex, length);
            _out[length] = '\0';

            return true;
        }
    }

    memcpy(_out, hex, most + 1u);

    return true;
}
