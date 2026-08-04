/******************************************************************************
* djinterp [utility]                                         archive_common.h
*
*   The shared archive kernel: tier 0.  One definition of every container
* format, every entry, every container option, and the two transforms (create,
* extract), compiled by BOTH C and C++.  archive.h and archive.hpp are notation
* over this file.
*
*   THE THREE-FILE SPLIT:
*     archive_common.h / .c   this kernel, compiled by BOTH languages.
*     archive.h               the C face      -- entry/option builders, macros.
*     archive.hpp / .cpp      the C++ face    -- entry, entry_list, try_archive.
*   A header that must compile in both languages includes THIS file.
*
*   LAYERED ON COMPRESSION, AS THE ENV HEADERS ALREADY ARE:
*   env_archive.h includes env_compress.h because a container is a directory
* plus a codec.  This header follows the same direction: it includes
* compress_common.h and reuses d_pack_status, d_pack_sink, and
* d_compress_options wholesale rather than restating any of them.  A container
* option set EMBEDS a codec option set; it does not parallel one.
*
*   AN ENTRY BORROWS:
*   struct d_archive_entry holds a name and a payload it does not own.  This is
* the transient-value case the decision log carves out -- payload bytes use
* offsets rather than pointers where a record must be relocatable, and an entry
* handed to a writer is not such a record: it lives for the duration of one
* call.  So an entry is a pointer pair, it allocates nothing, and building an
* entry list is filling a stack array.
*
*   Every string is a struct d_pack_text -- pointer plus length, never a bare
* const char*.  A ZIP comment may contain a NUL, and test_compress.hpp already
* records the round-trip requirement that the payload path never treat a buffer
* as a C string; a length-carrying text type is the same discipline applied to
* metadata.
*
*   MTIME IS int64_t, NOT long:
*   The C++ face takes `long _mtime`.  `long` is 32 bits on LLP64 (Windows) and
* 64 bits on LP64, so one declaration would give the two platforms different
* representations AND put the 2038 boundary inside the framework.  The core
* pins int64_t; the C++ face's `long` parameter widens on the way in, which is
* lossless in both directions for every representable timestamp.
*
*   THE ASYMMETRY BETWEEN CREATE AND EXTRACT:
*   Creation writes one byte stream, so it uses the same two-call buffer
* protocol as compression.  Extraction produces a VARIABLE NUMBER of entries
* whose names and payloads must live somewhere, so it takes two caller-owned
* regions -- an entry array and a byte arena that the entries point into -- and
* d_archive_measure reports the size of both before the fact.  Still no
* allocation anywhere in tier 0.
*
*   WHAT IS NOT HERE:
*   `entry_list` (a std::vector), the format tag types, and try_archive<> are
* C++ notation.  The archive-format ordering is NOT notation and is pinned
* below: test_packaging.hpp records that DTest's own format enum orders `gz`
* and `tar_gz` differently from this one, which is exactly why the bridge
* between them must stay a switch and never become a cast.
*
*   PORTABILITY:
*   C99 / C++11 and upward.  Presence-only backend detection through
* env_archive.h; no third-party header is included and no link dependency is
* added.
*
*
* TABLE OF CONTENTS
* =================
* II.   FORMAT IDENTITY           (enum d_format_id, names, suffixes)
* III.  KNOB CONSTANT SETS        (zip methods, encryption, tar flavours)
* IV.   OPTION LAYOUT             (the five blocks + d_archive_options)
* V.    THE ENTRY
* VI.   LAYOUT ASSERTIONS
* VII.  OPTION OPERATIONS         (init, resolve, compare, diff)
* VIII. CAPABILITY
* IX.   CONTAINER SIGNATURES
* X.    TRANSFORM LEAVES          (create / measure / extract)
*
*
* path:      /inc/djinterp/core/util/archive_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.29
******************************************************************************/

#ifndef DJINTERP_UTIL_ARCHIVE_COMMON_
#define DJINTERP_UTIL_ARCHIVE_COMMON_ 1

// c
#include <stddef.h>
#include <stdint.h>
// djinterp
#include "../../c/djinterp.h"       // D_STATIC_ASSERT, D_EXTERN_C_*
#include "./compress_common.h"       // status, sink, d_compress_options
#include "../env/env_archive.h"      // D_ENV_ARCHIVE_CAN_*


//   cfg_qualifiers.h lets a project take ownership of the D_EXTERN_C family by
// setting D_CFG_DEFINE_EXTERN_C to 0.  This header needs it -- every
// declaration below must carry C linkage so one compiled object serves both
// languages -- so say so in one sentence rather than failing as a cascade of
// syntax errors fifty lines further down.
#if !defined(D_EXTERN_C_BEGIN)
    #error "archive_common.h needs D_EXTERN_C_BEGIN; \
define it or set D_CFG_DEFINE_EXTERN_C to 1"
#endif

D_EXTERN_C_BEGIN


// =============================================================================
// II.  FORMAT IDENTITY
// =============================================================================
//   Values are pinned explicitly and MUST NOT be reordered.  This ordering --
// zip, tar, gz, tar_gz, sevenzip, rar -- is the one output_packaging and
// document_bundle already pass by value, and it deliberately differs from
// DTest's test_archive_format, which orders tar_gz before gz.  A static_cast
// between the two produces the wrong container silently; the bridge is three
// switches for that reason and must stay so.

// d_format_id
//   enum: the container formats this framework names.  `gz` is a single
// compressed stream rather than a directory of members, and `tar_gz` is a tar
// container inside one, which is why both appear here alongside the true
// multi-member containers.
enum d_format_id
{
    D_FORMAT_ID_ZIP         = 0,
    D_FORMAT_ID_TAR         = 1,
    D_FORMAT_ID_GZ          = 2,
    D_FORMAT_ID_TAR_GZ      = 3,
    D_FORMAT_ID_SEVENZIP    = 4,
    D_FORMAT_ID_RAR         = 5
};

// D_FORMAT_ID_COUNT
//   constant: the number of enumerators in d_format_id.
#define D_FORMAT_ID_COUNT   6

// D_FORMAT_ID_IS_VALID
//   macro: 1 when _f names a format this framework knows.  Says nothing about
// whether the build can read or write it.
#define D_FORMAT_ID_IS_VALID(_f)                                               \
    ( ((int)(_f) >= 0) && ((int)(_f) < D_FORMAT_ID_COUNT) )

// D_FORMAT_ID_IS_SINGLE_STREAM
//   macro: 1 for a format that carries exactly one member and no directory.
// `gz` is the only such format; a create call naming it with more than one
// entry is a formal error, not a truncation.
#define D_FORMAT_ID_IS_SINGLE_STREAM(_f)                                       \
    ( (int)(_f) == (int)D_FORMAT_ID_GZ )


const char*        d_format_id_name(enum d_format_id _format);
const char*        d_format_suffix(enum d_format_id _format);
int                d_format_id_from_name(const char*       _name,
                                         enum d_format_id* _out_format);
int                d_format_id_from_suffix(const char*       _path,
                                           enum d_format_id* _out_format);
enum d_codec_id    d_format_default_codec(enum d_format_id _format);


// =============================================================================
// III. KNOB CONSTANT SETS
// =============================================================================
//   As in compress_common.h, these are named constant sets and not member
// types: every numeric option member below is an int32_t, because a struct
// member declared with an enum type has an implementation-defined size that C
// and C++ may resolve differently for one declaration.

// d_zip_method
//   enum: the per-entry ZIP compression method; values are the method codes
// the ZIP specification puts in the local header, so the mapping to the wire
// is the identity.
enum d_zip_method
{
    D_ZIP_METHOD_STORE   = 0,
    D_ZIP_METHOD_DEFLATE = 8,
    D_ZIP_METHOD_BZIP2   = 12,
    D_ZIP_METHOD_LZMA    = 14,
    D_ZIP_METHOD_ZSTD    = 93,
    D_ZIP_METHOD_XZ      = 95
};

// d_zip_encryption
//   enum: the ZIP encryption scheme.  `legacy` is PKWARE ZipCrypto, which is
// broken and is offered only for reading compatibility; a writer should
// prefer aes_256.
enum d_zip_encryption
{
    D_ZIP_ENCRYPTION_NONE    = 0,
    D_ZIP_ENCRYPTION_LEGACY  = 1,
    D_ZIP_ENCRYPTION_AES_128 = 2,
    D_ZIP_ENCRYPTION_AES_192 = 3,
    D_ZIP_ENCRYPTION_AES_256 = 4
};

// d_tar_format
//   enum: the tar flavour a writer emits.  `ustar` is the dependency-free
// built-in and the portability floor; `pax` is required for a path longer than
// 255 bytes or a timestamp outside the ustar octal range.
enum d_tar_format
{
    D_TAR_FORMAT_USTAR = 0,
    D_TAR_FORMAT_GNU   = 1,
    D_TAR_FORMAT_PAX   = 2,
    D_TAR_FORMAT_V7    = 3
};

// d_sevenzip_method
//   enum: the 7z stream method.
enum d_sevenzip_method
{
    D_SEVENZIP_METHOD_LZMA2   = 0,
    D_SEVENZIP_METHOD_LZMA    = 1,
    D_SEVENZIP_METHOD_BZIP2   = 2,
    D_SEVENZIP_METHOD_DEFLATE = 3,
    D_SEVENZIP_METHOD_ZSTD    = 4,
    D_SEVENZIP_METHOD_COPY    = 5
};

// D_ARCHIVE_KNOB_UNSET
//   constant: the pristine value of every numeric container knob.  The same
// sentinel compress_common.h uses, for the same reason and resolved the same
// way -- by the core, never by the backend.
#define D_ARCHIVE_KNOB_UNSET    D_COMPRESS_KNOB_UNSET


// =============================================================================
// IV.  OPTION LAYOUT
// =============================================================================
//   Declaration order is normative and matches the C++ face member for member,
// so that a diff report names the same knob in the same position in both
// languages.
//
//   Text knobs are grouped LAST within each block.  A block whose numeric
// knobs come first has its numeric prefix densely packed regardless of pointer
// width, which keeps the numeric offset table simple on every data model.

// d_zip_options
//   struct: the ZIP container block.  `method` takes a d_zip_method value and
// `encryption` a d_zip_encryption; `zip64` and `utf8_names` are flags.
struct d_zip_options
{
    int32_t             method;
    int32_t             encryption;
    int32_t             zip64;
    int32_t             utf8_names;
    struct d_pack_text  password;
};

// d_tar_options
//   struct: the tar container block.  `format` takes a d_tar_format value;
// `numeric_owner` is a flag that suppresses owner names in favour of ids,
// which is what makes a tar reproducible across machines.
struct d_tar_options
{
    int32_t     format;
    int32_t     numeric_owner;
};

// d_gz_options
//   struct: the gzip header block.  `store_name` and `store_mtime` are flags;
// clearing both is what produces a byte-reproducible .gz, since the RFC 1952
// header otherwise records the source filename and the current time.
struct d_gz_options
{
    int32_t             store_name;
    int32_t             store_mtime;
    struct d_pack_text  original_name;
};

// d_sevenzip_options
//   struct: the 7z container block.  `method` takes a d_sevenzip_method value;
// `solid`, `header_compression` and `header_encryption` are flags.
struct d_sevenzip_options
{
    int32_t             method;
    int32_t             solid;
    int32_t             header_compression;
    int32_t             header_encryption;
    int32_t             threads;
    struct d_pack_text  password;
};

// d_rar_options
//   struct: the RAR creation block.  Present for completeness of the option
// vocabulary; no library can create RAR, so a create call naming this format
// resolves to D_PACK_STATUS_UNAVAILABLE unless the proprietary tool backend is
// present.  `solid` and `recovery_record` are flags.
struct d_rar_options
{
    int32_t             level;
    int32_t             solid;
    int32_t             recovery_record;
    struct d_pack_text  password;
};

// d_archive_options
//   struct: the whole container surface -- the archive-level knobs, the
// embedded codec tuning, and one block per format.  `codec` is a full
// d_compress_options: a container's stream tuning IS codec tuning, and
// embedding it rather than restating it is what keeps one definition of the
// fifty codec knobs.
//
//   `level` and `codec.level` are deliberately distinct.  The archive-level
// value is the container's effort; the codec-level value tunes the stream
// inside it.  A call site that consults one and not the other is testable
// precisely because they are separate knobs.
struct d_archive_options
{
    int32_t                     level;
    int32_t                     store_only;
    int32_t                     preserve_permissions;
    int32_t                     preserve_mtime;
    struct d_pack_text          comment;
    struct d_compress_options   codec;
    struct d_zip_options        zip;
    struct d_tar_options        tar;
    struct d_gz_options         gz;
    struct d_sevenzip_options   sevenzip;
    struct d_rar_options        rar;
};

// D_ARCHIVE_OPTIONS_INIT
//   macro: the pristine initialiser -- every numeric knob UNSET and every text
// knob empty.  The `codec` member expands D_COMPRESS_OPTIONS_INIT, so the two
// vocabularies cannot drift out of step.
#define D_ARCHIVE_OPTIONS_INIT                                                 \
{                                                                              \
    D_ARCHIVE_KNOB_UNSET,                                                      \
    D_ARCHIVE_KNOB_UNSET,                                                      \
    D_ARCHIVE_KNOB_UNSET,                                                      \
    D_ARCHIVE_KNOB_UNSET,                                                      \
    D_PACK_TEXT_NONE,                                                          \
    D_COMPRESS_OPTIONS_INIT,                                                   \
    { D_ARCHIVE_KNOB_UNSET, D_ARCHIVE_KNOB_UNSET, D_ARCHIVE_KNOB_UNSET,        \
      D_ARCHIVE_KNOB_UNSET, D_PACK_TEXT_NONE },                                \
    { D_ARCHIVE_KNOB_UNSET, D_ARCHIVE_KNOB_UNSET },                            \
    { D_ARCHIVE_KNOB_UNSET, D_ARCHIVE_KNOB_UNSET, D_PACK_TEXT_NONE },          \
    { D_ARCHIVE_KNOB_UNSET, D_ARCHIVE_KNOB_UNSET, D_ARCHIVE_KNOB_UNSET,        \
      D_ARCHIVE_KNOB_UNSET, D_ARCHIVE_KNOB_UNSET, D_PACK_TEXT_NONE },          \
    { D_ARCHIVE_KNOB_UNSET, D_ARCHIVE_KNOB_UNSET, D_ARCHIVE_KNOB_UNSET,        \
      D_PACK_TEXT_NONE }                                                       \
}


// =============================================================================
// V.   THE ENTRY
// =============================================================================
//   One member of a container.  Borrowed throughout: `name` and `data` point
// at memory the caller owns for the duration of the call, and an entry list is
// a plain array of these.
//
//   The same type serves creation and extraction.  On the way in the caller
// fills the spans; on the way out d_archive_extract fills them to point into
// the arena the caller supplied.  One type, two directions -- a second
// "extracted entry" type would be a second definition of one object.

// d_archive_entry
//   struct: a container member -- its path, its bytes, and the metadata a
// writer records.  A directory member carries an empty payload; `mode` and
// `mtime` of 0 mean "let the writer choose", which is how a reproducible
// archive is requested.
struct d_archive_entry
{
    struct d_pack_text      name;
    struct d_pack_bytes     data;
    int32_t                 is_directory;
    uint32_t                mode;
    int64_t                 mtime;
};

// D_ARCHIVE_ENTRY_INIT
//   macro: an empty entry -- no name, no payload, not a directory, and both
// metadata fields left for the writer to choose.
#define D_ARCHIVE_ENTRY_INIT                                                   \
{                                                                              \
    D_PACK_TEXT_NONE,                                                          \
    D_PACK_BYTES_NONE,                                                         \
    0,                                                                         \
    (uint32_t)0,                                                               \
    (int64_t)0                                                                 \
}

// D_ARCHIVE_MODE_FILE_DEFAULT
//   constant: the permission bits a writer records for a regular file when
// `mode` is 0 -- 0644.  Pinned rather than taken from the process umask, which
// would make the output depend on the environment and break parity.
#define D_ARCHIVE_MODE_FILE_DEFAULT     0644u

// D_ARCHIVE_MODE_DIR_DEFAULT
//   constant: the permission bits a writer records for a directory when `mode`
// is 0 -- 0755.  Pinned for the same reason.
#define D_ARCHIVE_MODE_DIR_DEFAULT      0755u

// d_archive_layout
//   struct: the two region sizes an extraction needs -- how many entries the
// container holds, and how many bytes their names and payloads occupy in
// total.  Returned by d_archive_measure and, filled in, by d_archive_extract.
struct d_archive_layout
{
    size_t  entry_count;
    size_t  arena_size;
};


// =============================================================================
// VI.  LAYOUT ASSERTIONS
// =============================================================================
//   The Layout law, with one difference from compress_common.h that is worth
// stating because getting it wrong is the obvious mistake here.
//
//   d_archive_options CANNOT be densely packed and must not be asserted as
// though it were.  It mixes int32_t knobs with pointer-bearing text spans, and
// a span's alignment is the pointer's: a block of five int32_t followed by a
// d_pack_text puts the span at offset 24 on LP64, not 20, because four bytes of
// padding are inserted to align it.  An assertion that pinned the span to 20
// would hold on no data model at all, and one that pinned it to 24 would hold
// on LP64 and fail on ILP32 -- a per-tier assertion is not an invariant.
//
//   So the assertions below pin exactly what the code depends on and nothing
// more.  The NUMERIC prefix of each block is dense, because int32_t is four
// bytes on every data model and nothing may be interleaved with it -- that
// density is what lets the numeric knob table be an offset table.  Each TEXT
// member is asserted only to FOLLOW its block's numeric prefix, which is an
// ordering fact rather than a padding fact.  Where the padding lands is not
// asserted because nothing reads it: knob access goes through an offset table
// and memcpy, which is indifferent to it.

D_STATIC_ASSERT(sizeof(struct d_pack_text) == sizeof(struct d_pack_bytes),
                "d_pack_text / d_pack_bytes: spans must share a shape");
D_STATIC_ASSERT(offsetof(struct d_pack_text, data) == 0,
                "d_pack_text: field drift at data");
D_STATIC_ASSERT(offsetof(struct d_pack_bytes, data) == 0,
                "d_pack_bytes: field drift at data");

// -- zip: 4 dense numeric knobs, then password --------------------------------
D_STATIC_ASSERT(offsetof(struct d_zip_options, method) == 0,
                "d_zip_options: field drift at method");
D_STATIC_ASSERT(offsetof(struct d_zip_options, encryption) ==
                (1 * sizeof(int32_t)),
                "d_zip_options: field drift at encryption");
D_STATIC_ASSERT(offsetof(struct d_zip_options, zip64) ==
                (2 * sizeof(int32_t)),
                "d_zip_options: field drift at zip64");
D_STATIC_ASSERT(offsetof(struct d_zip_options, utf8_names) ==
                (3 * sizeof(int32_t)),
                "d_zip_options: field drift at utf8_names");
D_STATIC_ASSERT(offsetof(struct d_zip_options, password) >=
                (4 * sizeof(int32_t)),
                "d_zip_options: password must follow the numeric prefix");

// -- tar: 2 dense numeric knobs, no text -------------------------------------
D_STATIC_ASSERT(sizeof(struct d_tar_options) == (2 * sizeof(int32_t)),
                "d_tar_options: layout drift");
D_STATIC_ASSERT(offsetof(struct d_tar_options, numeric_owner) ==
                (1 * sizeof(int32_t)),
                "d_tar_options: field drift at numeric_owner");

// -- gz: 2 dense numeric knobs, then original_name ---------------------------
D_STATIC_ASSERT(offsetof(struct d_gz_options, store_mtime) ==
                (1 * sizeof(int32_t)),
                "d_gz_options: field drift at store_mtime");
D_STATIC_ASSERT(offsetof(struct d_gz_options, original_name) >=
                (2 * sizeof(int32_t)),
                "d_gz_options: original_name must follow the numeric prefix");

// -- 7z: 5 dense numeric knobs, then password --------------------------------
D_STATIC_ASSERT(offsetof(struct d_sevenzip_options, solid) ==
                (1 * sizeof(int32_t)),
                "d_sevenzip_options: field drift at solid");
D_STATIC_ASSERT(offsetof(struct d_sevenzip_options, header_compression) ==
                (2 * sizeof(int32_t)),
                "d_sevenzip_options: field drift at header_compression");
D_STATIC_ASSERT(offsetof(struct d_sevenzip_options, header_encryption) ==
                (3 * sizeof(int32_t)),
                "d_sevenzip_options: field drift at header_encryption");
D_STATIC_ASSERT(offsetof(struct d_sevenzip_options, threads) ==
                (4 * sizeof(int32_t)),
                "d_sevenzip_options: field drift at threads");
D_STATIC_ASSERT(offsetof(struct d_sevenzip_options, password) >=
                (5 * sizeof(int32_t)),
                "d_sevenzip_options: password must follow the numeric prefix");

// -- rar: 3 dense numeric knobs, then password -------------------------------
D_STATIC_ASSERT(offsetof(struct d_rar_options, solid) ==
                (1 * sizeof(int32_t)),
                "d_rar_options: field drift at solid");
D_STATIC_ASSERT(offsetof(struct d_rar_options, recovery_record) ==
                (2 * sizeof(int32_t)),
                "d_rar_options: field drift at recovery_record");
D_STATIC_ASSERT(offsetof(struct d_rar_options, password) >=
                (3 * sizeof(int32_t)),
                "d_rar_options: password must follow the numeric prefix");

// -- the aggregate: 4 dense archive-level knobs, then comment, then codec ----
D_STATIC_ASSERT(offsetof(struct d_archive_options, level) == 0,
                "d_archive_options: field drift at level");
D_STATIC_ASSERT(offsetof(struct d_archive_options, store_only) ==
                (1 * sizeof(int32_t)),
                "d_archive_options: field drift at store_only");
D_STATIC_ASSERT(offsetof(struct d_archive_options, preserve_permissions) ==
                (2 * sizeof(int32_t)),
                "d_archive_options: field drift at preserve_permissions");
D_STATIC_ASSERT(offsetof(struct d_archive_options, preserve_mtime) ==
                (3 * sizeof(int32_t)),
                "d_archive_options: field drift at preserve_mtime");
D_STATIC_ASSERT(offsetof(struct d_archive_options, comment) >=
                (4 * sizeof(int32_t)),
                "d_archive_options: comment must follow the numeric prefix");
D_STATIC_ASSERT(offsetof(struct d_archive_options, codec) >
                offsetof(struct d_archive_options, comment),
                "d_archive_options: codec must follow comment");

// -- the entry ---------------------------------------------------------------
D_STATIC_ASSERT(offsetof(struct d_archive_entry, name) == 0,
                "d_archive_entry: field drift at name");
D_STATIC_ASSERT(offsetof(struct d_archive_entry, data) ==
                sizeof(struct d_pack_text),
                "d_archive_entry: field drift at data");
D_STATIC_ASSERT(sizeof(struct d_archive_layout) == (2 * sizeof(size_t)),
                "d_archive_layout: layout drift");


// =============================================================================
// VII. OPTION OPERATIONS
// =============================================================================
//   The archive counterparts of compress_common.h's option operations, and the
// same division of labour: the core reports, the caller presents.
//
//   THE DIFF IS COARSE ON THE CODEC, ON PURPOSE.  d_archive_options_diff
// reports the embedded codec block as a SINGLE difference
// (D_ARCHIVE_KNOB_CODEC) rather than expanding it into fifty codec knobs.  A
// call site tuning a container rarely cares which of nineteen zstd knobs
// moved, and a caller that does care passes _a->codec and _b->codec to
// d_compress_options_diff directly.  Two granularities, one for each question,
// neither reimplementing the other.

// d_archive_knob
//   enum: a dense index over the numeric and text knobs of a
// d_archive_options.  The embedded codec occupies one index; the per-format
// blocks follow in declaration order.
enum d_archive_knob
{
    D_ARCHIVE_KNOB_LEVEL                    = 0,
    D_ARCHIVE_KNOB_STORE_ONLY               = 1,
    D_ARCHIVE_KNOB_PRESERVE_PERMISSIONS     = 2,
    D_ARCHIVE_KNOB_PRESERVE_MTIME           = 3,
    D_ARCHIVE_KNOB_COMMENT                  = 4,
    D_ARCHIVE_KNOB_CODEC                    = 5,

    D_ARCHIVE_KNOB_ZIP_METHOD               = 6,
    D_ARCHIVE_KNOB_ZIP_ENCRYPTION           = 7,
    D_ARCHIVE_KNOB_ZIP_ZIP64                = 8,
    D_ARCHIVE_KNOB_ZIP_UTF8_NAMES           = 9,
    D_ARCHIVE_KNOB_ZIP_PASSWORD             = 10,

    D_ARCHIVE_KNOB_TAR_FORMAT               = 11,
    D_ARCHIVE_KNOB_TAR_NUMERIC_OWNER        = 12,

    D_ARCHIVE_KNOB_GZ_STORE_NAME            = 13,
    D_ARCHIVE_KNOB_GZ_STORE_MTIME           = 14,
    D_ARCHIVE_KNOB_GZ_ORIGINAL_NAME         = 15,

    D_ARCHIVE_KNOB_SEVENZIP_METHOD          = 16,
    D_ARCHIVE_KNOB_SEVENZIP_SOLID           = 17,
    D_ARCHIVE_KNOB_SEVENZIP_HEADER_COMP     = 18,
    D_ARCHIVE_KNOB_SEVENZIP_HEADER_ENCRYPT  = 19,
    D_ARCHIVE_KNOB_SEVENZIP_THREADS         = 20,
    D_ARCHIVE_KNOB_SEVENZIP_PASSWORD        = 21,

    D_ARCHIVE_KNOB_RAR_LEVEL                = 22,
    D_ARCHIVE_KNOB_RAR_SOLID                = 23,
    D_ARCHIVE_KNOB_RAR_RECOVERY_RECORD      = 24,
    D_ARCHIVE_KNOB_RAR_PASSWORD             = 25
};

// D_ARCHIVE_KNOB_COUNT
//   constant: the number of knobs in a d_archive_options, counting the
// embedded codec as one.
#define D_ARCHIVE_KNOB_COUNT    26

D_STATIC_ASSERT((D_ARCHIVE_KNOB_RAR_PASSWORD + 1) == D_ARCHIVE_KNOB_COUNT,
                "d_archive_knob: index set is not dense");


const char*        d_archive_knob_name(enum d_archive_knob _knob);
int                d_archive_knob_is_text(enum d_archive_knob _knob);
enum d_format_id   d_archive_knob_format(enum d_archive_knob _knob);
void               d_archive_options_init(struct d_archive_options* _options);
int                d_archive_options_equal(const struct d_archive_options* _a,
                                           const struct d_archive_options* _b);
int                d_archive_options_are_default(
                    const struct d_archive_options* _options);
int                d_archive_options_codec_is_default(
                    const struct d_archive_options* _options);
size_t             d_archive_options_diff(
                    const struct d_archive_options* _a,
                    const struct d_archive_options* _b,
                    enum d_archive_knob*            _out_knobs,
                    size_t                          _out_capacity);
enum d_pack_status d_archive_options_resolve(
                    enum d_format_id                _format,
                    const struct d_archive_options* _in,
                    struct d_archive_options*       _out);
enum d_pack_status d_archive_options_validate(
                    enum d_format_id                _format,
                    const struct d_archive_options* _options,
                    enum d_archive_knob*            _out_offender);


// =============================================================================
// VIII. CAPABILITY
// =============================================================================
//   Read and write capability are separate questions and must stay so: RAR is
// readable by four backends and writable by none of them, and 7z write support
// depends on the backend rather than on the format.  Collapsing the two into
// one "supported" predicate is what makes a writer report a formal failure as
// though the format were unknown.
//
//   d_archive_entry_name_is_valid is here rather than in a writer because the
// rule is a property of the container vocabulary: a name is rejected for
// absolute paths, for `..` traversal, and for a directory entry whose payload
// is non-empty.  A caller that sanitises before building an entry list gets
// the same answer as the writer would, which is what lets a test assert the
// rule without invoking a backend.

int                d_format_is_readable(enum d_format_id _format);
int                d_format_is_writable(enum d_format_id _format);
int                d_format_supports_codec(enum d_format_id _format,
                                           enum d_codec_id  _codec);
int                d_format_supports_encryption(enum d_format_id _format);
int                d_format_backend_id(enum d_format_id _format);
const char*        d_format_backend_name(enum d_format_id _format);
size_t             d_format_writable_list(enum d_format_id* _out_formats,
                                          size_t            _out_capacity);
int                d_archive_entry_name_is_valid(struct d_pack_text _name);
int                d_archive_entry_is_valid(
                    const struct d_archive_entry* _entry);


// =============================================================================
// IX.  CONTAINER SIGNATURES
// =============================================================================
//   Container recognition, the archive counterpart of d_codec_signature_matches
// and the core home for the checks test_archive.hpp currently performs by
// hand against emitted bytes (the ZIP end-of-central-directory scan, the ustar
// magic at offset 257).
//
//   tar has no magic at offset 0 -- `ustar` sits at byte 257 -- so recognising
// a tar requires at least a full 512-byte record, and d_format_signature_length
// reports that rather than a nominal small number.

int                d_format_signature_matches(enum d_format_id _format,
                                              const void*      _data,
                                              size_t           _size);
int                d_format_signature_length(enum d_format_id _format);
int                d_format_detect(const void*       _data,
                                   size_t            _size,
                                   enum d_format_id* _out_format);


// =============================================================================
// X.   TRANSFORM LEAVES
// =============================================================================
//   CREATE follows compress_common.h's two-call buffer protocol exactly:
// _out == NULL with _out_capacity == 0 measures, a too-small buffer returns
// D_PACK_STATUS_BUFFER_TOO_SMALL with *_out_size set to the requirement.  The
// sink form streams and pays one pass.
//
//   An empty entry list is legal for every multi-member format and produces a
// valid empty container; it is a formal error only for gz, which has no
// directory to be empty.  Passing count 0 with a null entry base is therefore
// the normal way to ask for an empty archive, and document_bundle already
// relies on that -- it passes a null base rather than indexing an empty
// vector.
//
//   EXTRACT is the two-region form described in the header banner:
//     1. d_archive_measure reports entry_count and arena_size.
//     2. The caller supplies an entry array of at least entry_count and an
//        arena of at least arena_size.
//     3. d_archive_extract fills both, and each entry's name and data spans
//        point INTO the arena.  The entries are valid for exactly as long as
//        the arena is.
//   Either region being too small yields D_PACK_STATUS_BUFFER_TOO_SMALL with
// *_out_used carrying both requirements, so one retry suffices.
//
//   d_archive_extract_entry pulls a single member by name without materialising
// the rest, for the common case of reading one file out of a container.

enum d_pack_status d_archive_bound(enum d_format_id                _format,
                                   const struct d_archive_entry*   _entries,
                                   size_t                          _entry_count,
                                   const struct d_archive_options* _options,
                                   size_t*                         _out_bound);
enum d_pack_status d_archive_create(
                    enum d_format_id                _format,
                    const struct d_archive_entry*   _entries,
                    size_t                          _entry_count,
                    const struct d_archive_options* _options,
                    void*                           _out,
                    size_t                          _out_capacity,
                    size_t*                         _out_size);
enum d_pack_status d_archive_create_to_sink(
                    enum d_format_id                _format,
                    const struct d_archive_entry*   _entries,
                    size_t                          _entry_count,
                    const struct d_archive_options* _options,
                    struct d_pack_sink              _sink,
                    size_t*                         _out_size);
enum d_pack_status d_archive_measure(
                    enum d_format_id                _format,
                    const void*                     _in,
                    size_t                          _in_size,
                    const struct d_archive_options* _options,
                    struct d_archive_layout*        _out_layout);
enum d_pack_status d_archive_extract(
                    enum d_format_id                _format,
                    const void*                     _in,
                    size_t                          _in_size,
                    const struct d_archive_options* _options,
                    struct d_archive_entry*         _entries,
                    size_t                          _entry_cap,
                    void*                           _arena,
                    size_t                          _arena_size,
                    struct d_archive_layout*        _out_used);
enum d_pack_status d_archive_extract_entry(
                    enum d_format_id                _format,
                    const void*                     _in,
                    size_t                          _in_size,
                    struct d_pack_text              _name,
                    const struct d_archive_options* _options,
                    void*                           _out,
                    size_t                          _out_capacity,
                    size_t*                         _out_size);


D_EXTERN_C_END


#endif  // DJINTERP_UTIL_ARCHIVE_COMMON_
