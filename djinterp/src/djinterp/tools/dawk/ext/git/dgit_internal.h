/*******************************************************************************
* djinterp [tools]                                               dgit_internal.h
*
* What the git extension's units share, and no other unit sees.
*   Files: a file read whole, written beside its target and renamed over it,
* asked about without following a link, as git's lstat asks; a link read; a
* path's real name; the working directory. Each is ISO C where ISO C has an
* answer, and otherwise POSIX or Windows, chosen a whole function at a time.
*   Hashes: SHA-1, and SHA-256 for a repository of that format, each fed in
* pieces. Inflating: zlib's stream, which git's objects and packs are
* written in, read without zlib. Both are written here for the git
* extension alone, which shares them with no other extension.
*
* path:      /src/djinterp/tools/dawk/ext/git/dgit_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Files
         1.  d_git_file_kind
         2.  d_git_file_info
    2.  Hashes
         1.  D_GIT_HASH_MOST
         2.  d_git_hash_kind
         3.  d_git_hash
    3.  Inflating
         1.  d_git_inflated
    4.  Repository
         1.  D_GIT_CACHE_SLOTS
         2.  d_git_pack
         3.  d_git_store
         4.  d_git_cached
         5.  d_git_packed_ref
         6.  d_git_rules
         7.  d_git_repo
2.  OPERATIONS
    ----------
    1.  Files
    2.  Hashes
    3.  Inflating
    4.  Repository
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_GIT_DGIT_INTERNAL_H
#define DJINTERP_TOOLS_DAWK_EXT_GIT_DGIT_INTERNAL_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_repo.h"  // d_git_oid,
                                                          // d_git_setting
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_text.h"  // d_git_text
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, uint32_t,
                                                     // uint64_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Files
//------------------------------------------------------------------------------
// 1.1.1
// d_git_file_kind
//   enum: what a path names, a link not followed.
enum d_git_file_kind
{
    D_GIT_FILE_NONE = 0,   // nothing there, or nothing that can be asked of
    D_GIT_FILE_REGULAR,
    D_GIT_FILE_DIRECTORY,
    D_GIT_FILE_LINK,
    D_GIT_FILE_OTHER       // a device, a pipe, a socket
};

// 1.1.2
// d_git_file_info
//   struct: what git's index keeps of a file, as lstat gives it: its
//   kind, whether its owner may run it, its size, the seconds of its
//   change times, its device and inode, its owner and group -- each cut
//   to 32 bits, as the index cuts them. What a system does not give is 0.
struct d_git_file_info
{
    enum d_git_file_kind kind;
    bool                 executable;
    uint64_t             size;
    uint32_t             mtime;
    uint32_t             ctime;
    uint32_t             dev;
    uint32_t             ino;
    uint32_t             uid;
    uint32_t             gid;
};

// 1.2    Hashes
//------------------------------------------------------------------------------
// 1.2.1
// D_GIT_HASH_MOST
//   constant: the most bytes a hash has: SHA-256's 32.
#define D_GIT_HASH_MOST 32u

// 1.2.2
// d_git_hash_kind
//   enum: the hash a repository names its objects by.
enum d_git_hash_kind
{
    D_GIT_HASH_SHA1 = 0,   // 20 bytes
    D_GIT_HASH_SHA256      // 32 bytes
};

// 1.2.3
// d_git_hash
//   struct: a hash being fed: its kind, its state, the bytes waiting to
//   fill a block, and how many bytes it has been fed.
struct d_git_hash
{
    enum d_git_hash_kind kind;
    uint32_t             state[8];
    uint8_t              block[64];
    size_t               waiting;
    uint64_t             fed;
};

// 1.3    Inflating
//------------------------------------------------------------------------------
// 1.3.1
// d_git_inflated
//   struct: what inflating gave: `length` bytes, allocated and
//   terminated, and `used`, how many bytes of the stream it took, its
//   trailer among them.
struct d_git_inflated
{
    uint8_t* bytes;
    size_t   length;
    size_t   used;
};

// 1.4    Repository
//------------------------------------------------------------------------------
// 1.4.1
// D_GIT_CACHE_SLOTS
//   constant: how many objects a delta's bases are kept in, by place.
#define D_GIT_CACHE_SLOTS 256u

// 1.4.2
// d_git_pack
//   struct: one pack: its index, whole, read -- version 1 or 2, `count`
//   names in order, and where each object is -- and the pack itself, open,
//   with every object's place in order, made when an object's end is first
//   wanted.
struct d_git_pack
{
    char*          path;
    uint8_t*       idx;
    size_t         idx_length;
    unsigned       version;
    uint32_t       count;
    const uint8_t* oids;
    size_t         stride;
    const uint8_t* offsets;
    const uint8_t* large;
    void*          file;
    uint64_t       file_size;
    uint64_t*      sorted;
};

// 1.4.3
// d_git_store
//   struct: a directory of objects -- the repository's, or an alternate's
//   -- and its packs, read when first wanted.
struct d_git_store
{
    char*              path;
    struct d_git_pack* packs;
    size_t             pack_count;
    bool               packs_read;
};

// 1.4.4
// d_git_cached
//   struct: an object a pack holds at a place, kept as a delta's base.
struct d_git_cached
{
    size_t                 store;
    size_t                 pack;
    uint64_t               offset;
    enum d_git_object_type type;
    uint8_t*               data;
    size_t                 size;
};

// 1.4.5
// d_git_packed_ref
//   struct: a ref packed-refs holds, and what its tag peels to.
struct d_git_packed_ref
{
    char*            name;
    struct d_git_oid oid;
    bool             has_peeled;
    struct d_git_oid peeled;
};

// 1.4.6
// d_git_rules
//   struct: an ignore or attribute file the work tree's rules read, kept:
//   its path as git names it, the base its patterns are relative to, its
//   text and records -- `present` false for one not there.
struct d_git_rules
{
    char*             name;
    char*             base;
    char*             text;
    struct d_git_text records;
    bool              present;
    bool              macros;
};

// 1.4.7
// d_git_repo
//   struct: an open repository: its paths and format; its settings, and
//   the strings they hold; its stores of objects, the bases kept and how
//   many bytes they hold; its packed refs; its index; the work tree's
//   rules, read as wanted; and the tree attributes are read from instead,
//   when there is one.
struct d_git_repo
{
    char*                    git_dir;
    char*                    common_dir;
    char*                    work_tree;
    enum d_git_hash_kind     hash;
    char                     problem[512];
    struct d_git_setting*    settings;
    size_t                   setting_count;
    size_t                   setting_capacity;
    char**                   strings;
    size_t                   string_count;
    size_t                   string_capacity;
    struct d_git_store*      stores;
    size_t                   store_count;
    struct d_git_cached      cache[D_GIT_CACHE_SLOTS];
    size_t                   cache_bytes;
    struct d_git_packed_ref* packed;
    size_t                   packed_count;
    bool                     packed_read;
    struct d_git_index_entry* entries;
    size_t                   entry_count;
    char*                    index_names;
    bool                     index_read;
    bool                     index_failed;
    uint32_t                 index_mtime;
    struct d_git_rules*      rules;
    size_t                   rule_count;
    size_t                   rule_capacity;
    bool                     attr_source_read;
    bool                     attr_source_bad;
    bool                     attr_from_tree;
    struct d_git_oid         attr_tree;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Files
//------------------------------------------------------------------------------
// d_git_file_read gives a file's bytes, allocated and terminated, `NULL`
// when it cannot be read; `_out_length` receives their length.
// d_git_file_write writes a file beside its path and renames it over the
// path, the directories it is in made first; false, with why in
// `_problem`, when it cannot.
// d_git_file_info asks what a path names without following a link; false
// when nothing is there.
// d_git_file_link gives where a link leads, allocated, `NULL` for none;
// d_git_file_real a path's full name, every link followed, allocated, NULL
// for a path that names nothing; d_git_file_cwd the working directory,
// allocated.
char*  d_git_file_read(const char* _path,
                       size_t*     _out_length);
bool   d_git_file_write(const char* _path,
                        const char* _text,
                        size_t      _length,
                        char*       _problem,
                        size_t      _capacity);
bool   d_git_file_info(const char*             _path,
                       struct d_git_file_info* _out);
char*  d_git_file_link(const char* _path,
                       size_t*     _out_length);
char*  d_git_file_real(const char* _path);
char*  d_git_file_cwd(void);
// d_git_file_open opens a file to read at places, giving its size;
// d_git_file_read_at reads `_length` bytes at a place, false unless every
// one was there; and d_git_file_close closes it. Places are 64 bits on any
// system.
void*  d_git_file_open(const char* _path,
                       uint64_t*   _out_size);
bool   d_git_file_read_at(void*    _file,
                          uint64_t _offset,
                          void*    _buffer,
                          size_t   _length);
void   d_git_file_close(void* _file);

// 2.2    Hashes
//------------------------------------------------------------------------------
// d_git_hash_size is a kind's length in bytes; d_git_hash_begin starts a
// hash, d_git_hash_feed feeds it bytes, and d_git_hash_end writes it, its
// size in bytes, and leaves it to be begun again.
size_t d_git_hash_size(enum d_git_hash_kind _kind);
void   d_git_hash_begin(struct d_git_hash*   _hash,
                        enum d_git_hash_kind _kind);
void   d_git_hash_feed(struct d_git_hash* _hash,
                       const void*        _bytes,
                       size_t             _length);
void   d_git_hash_end(struct d_git_hash* _hash,
                      uint8_t*           _out);

// 2.3    Inflating
//------------------------------------------------------------------------------
/**
 * @brief Inflates a zlib stream: its header, its deflated blocks, and its
 *        Adler-32 trailer, checked.
 *
 * @param[in]  _stream    the stream's bytes; more may follow it.
 * @param[in]  _length    how many there are.
 * @param[in]  _expected  how many bytes it inflates to, when that is
 *                        known, as a pack's header says; 0 when it is not.
 * @param[out] _out       receives the bytes and how many were used; free
 *                        its bytes, whatever is returned.
 * @return `true` if the stream inflated whole and sound -- to `_expected`
 *         bytes when that was given; `false` otherwise.
 */
bool   d_git_inflate(const uint8_t*         _stream,
                     size_t                 _length,
                     size_t                 _expected,
                     struct d_git_inflated* _out);

// 2.4    Repository
//------------------------------------------------------------------------------
// d_git_repo_fail says why something asked of a repository failed --
// `_what`, then `_detail` after a colon when it is not NULL -- and is
// false.
// d_git_repo_keep keeps a string the repository's settings hold, copied,
// and gives the copy; NULL when memory runs out.
// d_git_repo_flag is a boolean setting, or `_default` when it is not set
// or is not a boolean.
bool        d_git_repo_fail(struct d_git_repo* _repo,
                            const char*        _what,
                            const char*        _detail);
const char* d_git_repo_keep(struct d_git_repo* _repo,
                            const char*        _text,
                            size_t             _length);
bool        d_git_repo_flag(const struct d_git_repo* _repo,
                            const char*              _key,
                            bool                     _default);
// d_git_path_join joins a directory and a name with a `/`, allocated;
// either may be "".
char*       d_git_path_join(const char* _directory,
                            const char* _name);
// d_git_stores_release lets go of the object stores and the bases kept;
// d_git_rules_release of the work tree's rules; d_git_index_release of
// the index.
void        d_git_stores_release(struct d_git_repo* _repo);
void        d_git_rules_release(struct d_git_repo* _repo);
void        d_git_index_release(struct d_git_repo* _repo);
// d_git_tree_find gives what a tree holds at a path, `/` between its
// names: the object and its mode; false when the path is not there.
bool        d_git_tree_find(struct d_git_repo*      _repo,
                            const struct d_git_oid* _tree,
                            const char*             _path,
                            struct d_git_oid*       _out,
                            uint32_t*               _out_mode);
// d_git_repo_prefix_find finds the one object whose name begins with
// `_length` hex digits, in either case; false, said, for none or several.
bool        d_git_repo_prefix_find(struct d_git_repo* _repo,
                                   const char*        _hex,
                                   size_t             _length,
                                   struct d_git_oid*  _out);


#endif  // DJINTERP_TOOLS_DAWK_EXT_GIT_DGIT_INTERNAL_H
