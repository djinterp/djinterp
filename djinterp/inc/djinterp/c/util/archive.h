/******************************************************************************
* djinterp [utility]                                                 archive.h
*
*   The C face of the archive kernel: tier 1a.  Ergonomics ONLY, on the same
* terms as compress.h -- every macro expands to a call into archive_common.h,
* and nothing here decides anything.  If a change to this file would change an
* output byte, it belongs in the core.
*
*   THE THREE-FILE SPLIT:
*     archive_common.h / .c   the kernel, compiled by BOTH languages.
*     archive.h       (this)  the C face.
*     archive.hpp / .cpp      the C++ face.
*
*   C++ DOES NOT INCLUDE THIS FILE, for the same reason it does not include
* compress.h: the builders are C99 compound literals.  The C++ face wraps
* archive_common.h directly.
*
*   NO archive.c -- everything here is a macro or a D_INLINE (`static inline`)
* function, so this header exports no external symbol.
*
*   THE PROBLEM THIS FACE ACTUALLY SOLVES:
*   The core's entry model is a borrowed pointer pair, which is the right
* representation and a poor thing to type.  Building a three-file archive by
* hand is fifteen assignments across three named locals, and the failure mode
* is silent -- a forgotten `is_directory` or an uninitialised `mtime` produces a
* valid archive with wrong metadata, which no round-trip test catches because
* the round trip preserves the wrong value faithfully.
*
*   D_ARCHIVE_FILE and its siblings collapse each entry to one expression with
* every field accounted for, so an omission is a compile error rather than a
* wrong byte.  That is the whole justification for the section; the rest of this
* header is the same four categories compress.h has.
*
*   LIFETIME, STATED ONCE:
*   An entry BORROWS.  The builders below produce compound literals whose
* storage lasts to the end of the enclosing block, and the name and payload
* they point at must outlive the d_archive_create call, not merely the builder.
* Both conditions hold for the ordinary shape -- a literal array of entries over
* string literals and caller buffers, passed to a create call in the same block.
* Storing an entry list beyond its block, or over a buffer that is then reused,
* is the one way to misuse this file, and no macro can catch it.
*
*   TIERING:
*   As compress.h: _Generic is C11 and degrades to the enumerator spelling
* below it.  Everything else is C99.
*
*
* TABLE OF CONTENTS
* =================
* I.    SPAN BUILDERS             (text / bytes as expressions)
* II.   ENTRY BUILDERS            (file / text / dir / entry lists)
* III.  OPTION BUILDERS           (container option sets)
* IV.   FORMAT NAMING             (_Generic over enumerator or string)
* V.    SHORTHAND CALLS
* VI.   EXTRACTION PATTERN        (the measure / supply / extract shape)
* VII.  ITERATION                 (entries, formats)
*
*
* path:      /inc/djinterp/core/util/archive.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.29
******************************************************************************/

#ifndef DJINTERP_UTIL_ARCHIVE_
#define DJINTERP_UTIL_ARCHIVE_ 1

#ifdef __cplusplus
    #error "archive.h is the C face; C++ includes archive.hpp"
#endif

// c
#include <stddef.h>
#include <stdint.h>
#include <string.h>
// djinterp
#include "./archive_common.h"       // the kernel this is notation over
#include "./compress.h"             // D_PACK_OK / D_PACK_TRY / D_CODEC


// =============================================================================
// I.   SPAN BUILDERS
// =============================================================================
//   A d_pack_text or d_pack_bytes in two forms, and the distinction is not
// cosmetic -- it is forced by C and getting it wrong is a compile error at
// every static call site.
//
//     *_INIT   a BRACE INITIALISER.  Valid wherever an initialiser is, which
//              includes an object with static storage duration.
//     (plain)  a COMPOUND LITERAL, i.e. an expression.  Valid as a function
//              argument or in an assignment; NOT valid initialising a static,
//              because C99 6.7.8p4 requires a constant expression there and a
//              compound literal is not one.
//
//   So a file-scope entry table takes the _INIT forms and an inline argument
// takes the plain ones.  The plain forms are DEFINED as a cast over the _INIT
// forms, so the two cannot drift apart.
//
//   The literal builders take their length from the literal rather than calling
// strlen, which matters twice: it is constant-folded, and it is correct for a
// literal containing an interior NUL, where strlen is not.

// D_TEXT_INIT
//   macro: brace initialiser for a d_pack_text over the string LITERAL _lit.
// Only valid for a literal -- for a runtime pointer use D_TEXT_CSTR or
// D_TEXT_SPAN_INIT.
#define D_TEXT_INIT(_lit)           { (_lit), (sizeof(_lit) - 1u) }

// D_TEXT_SPAN_INIT
//   macro: brace initialiser for a d_pack_text over an explicit pointer and
// length.
#define D_TEXT_SPAN_INIT(_p, _n)    { (_p), (size_t)(_n) }

// D_BYTES_INIT
//   macro: brace initialiser for a d_pack_bytes over a pointer and size.
#define D_BYTES_INIT(_p, _n)        { (_p), (size_t)(_n) }

// D_BYTES_LITERAL_INIT
//   macro: brace initialiser for a d_pack_bytes over a string literal's
// characters, excluding its terminator.
#define D_BYTES_LITERAL_INIT(_lit)  { (_lit), (sizeof(_lit) - 1u) }

// D_TEXT
//   macro: a d_pack_text over the string literal _lit, as an EXPRESSION.
#define D_TEXT(_lit)                                                           \
    ((struct d_pack_text)D_TEXT_INIT(_lit))

// D_TEXT_SPAN
//   macro: a d_pack_text over an explicit pointer and length, as an expression.
#define D_TEXT_SPAN(_p, _n)                                                    \
    ((struct d_pack_text)D_TEXT_SPAN_INIT((_p), (_n)))

// D_TEXT_CSTR
//   macro: a d_pack_text over a NUL-terminated runtime pointer, as an
// expression.  This is the only span builder that consults a terminator, and
// it has no _INIT form: strlen is not a constant expression, so it can never
// initialise a static.
#define D_TEXT_CSTR(_p)                                                        \
    ((struct d_pack_text){ (_p), ((_p) ? strlen(_p) : (size_t)0) })

// D_BYTES
//   macro: a d_pack_bytes over a pointer and size, as an expression.
#define D_BYTES(_p, _n)                                                        \
    ((struct d_pack_bytes)D_BYTES_INIT((_p), (_n)))

// D_BYTES_LITERAL
//   macro: a d_pack_bytes over a string literal's characters, as an expression.
#define D_BYTES_LITERAL(_lit)                                                  \
    ((struct d_pack_bytes)D_BYTES_LITERAL_INIT(_lit))

// D_BYTES_NONE
//   macro: the empty byte span as an expression.  The counterpart of
// archive_common.h's D_PACK_BYTES_NONE, which is the brace form.
#define D_BYTES_NONE                                                           \
    ((struct d_pack_bytes)D_PACK_BYTES_NONE)

// D_BYTES_ARRAY
//   macro: a d_pack_bytes over the whole of array _arr, size taken from its own
// declaration.  Passing a pointer where an array was meant is caught by
// D_ARRAY_STATIC_SIZE rather than silently yielding a size of 1.
#define D_BYTES_ARRAY(_arr)                                                    \
    ((struct d_pack_bytes)D_BYTES_INIT((_arr), D_ARRAY_STATIC_SIZE(_arr)))


// =============================================================================
// II.  ENTRY BUILDERS
// =============================================================================
//   One expression per entry, every field accounted for -- and, as in section
// I, an _INIT form for the static case.  A file-scope entry table is the normal
// shape for a fixture, so the _INIT forms are not an edge case.
//
//   Mode and mtime default to 0, which the core reads as "writer chooses" and
// resolves to D_ARCHIVE_MODE_FILE_DEFAULT / _DIR_DEFAULT and a pinned
// timestamp.  That is what makes the default output reproducible: a 0 here
// means a constant, never the process umask or the wall clock.

// D_ARCHIVE_FILE_INIT
//   macro: brace initialiser for a regular-file entry over a pointer and size,
// with the writer choosing mode and mtime.
#define D_ARCHIVE_FILE_INIT(_name, _data, _size)                               \
    {                                                                          \
        D_TEXT_INIT(_name), D_BYTES_INIT((_data), (_size)),                    \
        0, (uint32_t)0, (int64_t)0                                             \
    }

// D_ARCHIVE_TEXT_INIT
//   macro: brace initialiser for a regular-file entry whose payload is the
// string literal _body, excluding its terminator.
#define D_ARCHIVE_TEXT_INIT(_name, _body)                                      \
    {                                                                          \
        D_TEXT_INIT(_name), D_BYTES_LITERAL_INIT(_body),                       \
        0, (uint32_t)0, (int64_t)0                                             \
    }

// D_ARCHIVE_DIR_INIT
//   macro: brace initialiser for a directory entry -- no payload,
// is_directory raised.
#define D_ARCHIVE_DIR_INIT(_name)                                              \
    {                                                                          \
        D_TEXT_INIT(_name), D_PACK_BYTES_NONE,                                 \
        1, (uint32_t)0, (int64_t)0                                             \
    }

// D_ARCHIVE_ENTRY_FULL_INIT
//   macro: brace initialiser for the general entry -- explicit mode and mtime.
// Use when reproducing a source tree's metadata rather than normalising it.
#define D_ARCHIVE_ENTRY_FULL_INIT(_name, _data, _size, _is_dir, _mode, _mtime) \
    {                                                                          \
        D_TEXT_INIT(_name), D_BYTES_INIT((_data), (_size)),                    \
        (int32_t)(_is_dir), (uint32_t)(_mode), (int64_t)(_mtime)               \
    }

// D_ARCHIVE_FILE
//   macro: a regular-file entry over a pointer and size, as an expression.
#define D_ARCHIVE_FILE(_name, _data, _size)                                    \
    ((struct d_archive_entry)D_ARCHIVE_FILE_INIT((_name), (_data), (_size)))

// D_ARCHIVE_TEXT
//   macro: a regular-file entry with a string-literal payload, as an
// expression.  The readable form for a test fixture.
#define D_ARCHIVE_TEXT(_name, _body)                                           \
    ((struct d_archive_entry)D_ARCHIVE_TEXT_INIT((_name), (_body)))

// D_ARCHIVE_DIR
//   macro: a directory entry, as an expression.
#define D_ARCHIVE_DIR(_name)                                                   \
    ((struct d_archive_entry)D_ARCHIVE_DIR_INIT(_name))

// D_ARCHIVE_ENTRY_FULL
//   macro: the general entry builder, as an expression.
#define D_ARCHIVE_ENTRY_FULL(_name, _data, _size, _is_dir, _mode, _mtime)      \
    ((struct d_archive_entry)D_ARCHIVE_ENTRY_FULL_INIT(                        \
        (_name), (_data), (_size), (_is_dir), (_mode), (_mtime)))

// D_ARCHIVE_ENTRIES
//   macro: an entry array as an expression, for passing a whole list inline:
//     d_archive_create(D_FORMAT_ID_ZIP,
//                      D_ARCHIVE_ENTRIES(D_ARCHIVE_TEXT("a.txt", "alpha"),
//                                        D_ARCHIVE_TEXT("b.txt", "beta")),
//                      2, NULL, buf, sizeof buf, &n);
//   The count must still be passed separately, because an array decays to a
// pointer as an argument and its length cannot survive the call.  Prefer a
// named array with the _INIT builders and D_ARCHIVE_CREATE_LIST below, which
// derives the count from the declaration and cannot drift.
#define D_ARCHIVE_ENTRIES(...)                                                 \
    ((const struct d_archive_entry[]){ __VA_ARGS__ })

// D_ARCHIVE_ENTRIES
//   macro: an entry array as an expression, for passing a whole list inline:
//     d_archive_create(D_FORMAT_ID_ZIP,
//                      D_ARCHIVE_ENTRIES(D_ARCHIVE_TEXT("a.txt", "alpha"),
//                                        D_ARCHIVE_TEXT("b.txt", "beta")),
//                      2, NULL, buf, sizeof buf, &n);
//   The count must still be passed separately, because an array decays to a
// pointer as an argument and its length cannot survive the call.  Prefer
// D_ARCHIVE_CREATE_LIST below, which declares a named array and derives the
// count from it.
#define D_ARCHIVE_ENTRIES(...)                                                 \
    ((const struct d_archive_entry[]){ __VA_ARGS__ })


// =============================================================================
// III. OPTION BUILDERS
// =============================================================================
//   As compress.h section II, including the reason these are functions rather
// than compound literals with designated initialisers: C zero-initialises every
// member an initialiser does not name, so a macro form would quietly set the
// unnamed knobs to 0 instead of leaving them UNSET, and 0 is meaningful for
// most of them.  See that section for the full argument.
//
//   For tuning a knob with no named builder, declare and assign:
//       struct d_archive_options opt = D_ARCHIVE_OPTIONS_INIT;
//       opt.zip.method = D_ZIP_METHOD_DEFLATE;

// d_archive_options_of
//   function: a pristine container option set as an expression.
D_INLINE struct d_archive_options
d_archive_options_of(void)
{
    struct d_archive_options opt = D_ARCHIVE_OPTIONS_INIT;

    return opt;
}

// d_archive_at_level
//   function: a pristine set with the CONTAINER-level effort at _level.
// Distinct from the embedded codec's level -- see the note on
// d_archive_options.
D_INLINE struct d_archive_options
d_archive_at_level(
    int32_t _level
)
{
    struct d_archive_options opt = D_ARCHIVE_OPTIONS_INIT;

    opt.level = _level;

    return opt;
}

// d_archive_at_codec_level
//   function: a pristine set with the EMBEDDED codec's level set and the
// container-level effort left UNSET.  Pairs with d_archive_at_level for
// testing which of the two a call site consults, as codec_level_options does
// on the C++ side.
D_INLINE struct d_archive_options
d_archive_at_codec_level(
    int32_t _level
)
{
    struct d_archive_options opt = D_ARCHIVE_OPTIONS_INIT;

    opt.codec.level = _level;

    return opt;
}

// d_archive_store_only
//   function: a pristine set that stores every member uncompressed.  The mode
// a container uses for already-compressed payloads, and the one that always
// works on a build with no codec at all.
D_INLINE struct d_archive_options
d_archive_store_only(void)
{
    struct d_archive_options opt = D_ARCHIVE_OPTIONS_INIT;

    opt.store_only = 1;

    return opt;
}

// d_archive_reproducible
//   function: a pristine set tuned so two runs over identical entries produce
// identical bytes.  Clears the gzip header's name and timestamp, drops owner
// names in tar in favour of numeric ids, and stops recording permissions and
// mtimes -- the four places a container otherwise absorbs the environment.
//   Worth a named builder because every one of those is silent: the archive is
// valid either way, and only a byte comparison across two machines reveals it.
D_INLINE struct d_archive_options
d_archive_reproducible(void)
{
    struct d_archive_options opt = D_ARCHIVE_OPTIONS_INIT;

    opt.preserve_permissions = 0;
    opt.preserve_mtime       = 0;
    opt.tar.numeric_owner    = 1;
    opt.gz.store_name        = 0;
    opt.gz.store_mtime       = 0;

    return opt;
}

// d_archive_encrypted_zip
//   function: a pristine set requesting AES-256 ZIP encryption with _password.
// Fixed at AES rather than parameterised over the scheme because the legacy
// PKWARE scheme is broken and should not be reachable by a one-word edit; a
// caller needing it for read compatibility sets zip.encryption directly.
//   _password is BORROWED, as every d_pack_text is: it must outlive the create
// call, not merely this builder.
D_INLINE struct d_archive_options
d_archive_encrypted_zip(
    const char* _password
)
{
    struct d_archive_options opt = D_ARCHIVE_OPTIONS_INIT;

    opt.zip.encryption = D_ZIP_ENCRYPTION_AES_256;
    opt.zip.password   = d_pack_text_from_cstr(_password);

    return opt;
}


// =============================================================================
// IV.  FORMAT NAMING
// =============================================================================
//   One argument, two spellings, exactly as D_CODEC.  D_FORMAT(zip) is the
// enumerator; D_FORMAT("7z") is the string.  The string arm exists for the same
// reason: a format that arrives from a config file or a command line is text,
// and without it every such call site writes its own switch.

#if D_ENV_LANG_IS_C11_OR_HIGHER

// d_internal_format_identity
//   function: the enumerator arm of D_FORMAT -- the identity, so both arms are
// function calls and the macro has one shape.
D_INLINE enum d_format_id
d_internal_format_identity(
    enum d_format_id _format
)
{
    return _format;
}

// d_internal_format_from_any
//   function: the string arm of D_FORMAT -- resolves a format name, falling
// back to zip when unknown.  A caller needing to DETECT a bad name calls
// d_format_id_from_name, which reports it.
D_INLINE enum d_format_id
d_internal_format_from_any(
    const char* _name
)
{
    enum d_format_id format = D_FORMAT_ID_ZIP;

    (void)d_format_id_from_name(_name, &format);

    return format;
}

// D_FORMAT
//   macro: an enum d_format_id from either an enumerator or a format name.
#define D_FORMAT(_x)                                                           \
    _Generic((_x),                                                             \
             char*:       d_internal_format_from_any,                          \
             const char*: d_internal_format_from_any,                          \
             default:     d_internal_format_identity)(_x)

#else   // C99: no _Generic

// D_FORMAT
//   macro: degraded form -- forwards an enumerator unchanged.  Call
// d_format_id_from_name for the string spelling below C11.
#define D_FORMAT(_x)    (_x)

#endif  // D_ENV_LANG_IS_C11_OR_HIGHER


// =============================================================================
// V.   SHORTHAND CALLS
// =============================================================================
//   The core's transforms with the options argument omitted.  As in compress.h,
// a NULL options pointer means the core's pinned defaults, resolved in exactly
// one place; these are shorthands and not a second default policy.

// D_ARCHIVE_MEASURE
//   macro: the measure call for creation -- sets *_out_size to the exact byte
// count the container would occupy.
#define D_ARCHIVE_MEASURE(_fmt, _entries, _count, _out_size)                   \
    d_archive_create(D_FORMAT(_fmt), (_entries), (_count), NULL,               \
                     NULL, (size_t)0, (_out_size))

// D_ARCHIVE_CREATE_INTO
//   macro: create into a caller-owned buffer with default tuning.
#define D_ARCHIVE_CREATE_INTO(_fmt, _entries, _count, _out, _cap, _out_size)   \
    d_archive_create(D_FORMAT(_fmt), (_entries), (_count), NULL,               \
                     (_out), (_cap), (_out_size))

// D_ARCHIVE_CREATE_LIST
//   macro: create from a NAMED entry array, deriving the count from the array's
// own declaration.  The preferred spelling: the count cannot drift out of step
// with the list, which is the mistake D_ARCHIVE_ENTRIES cannot prevent.
#define D_ARCHIVE_CREATE_LIST(_fmt, _list, _out, _cap, _out_size)              \
    d_archive_create(D_FORMAT(_fmt),                                           \
                     (_list), D_ARRAY_STATIC_SIZE(_list), NULL,                \
                     (_out), (_cap), (_out_size))

// D_ARCHIVE_CREATE_TO_ARRAY
//   macro: create from a named entry array into a named output array, both
// capacities taken from their declarations.  The whole call in one line, with
// nothing for a call site to get out of step.
#define D_ARCHIVE_CREATE_TO_ARRAY(_fmt, _list, _arr, _out_size)                \
    d_archive_create(D_FORMAT(_fmt),                                           \
                     (_list), D_ARRAY_STATIC_SIZE(_list), NULL,                \
                     (_arr), D_ARRAY_STATIC_SIZE(_arr), (_out_size))


// =============================================================================
// VI.  EXTRACTION PATTERN
// =============================================================================
//   Extraction is the three-step shape described in archive_common.h section X:
// measure both regions, supply both, extract.  A caller writing it by hand gets
// it wrong in one specific way -- sizing the entry array from the arena size or
// the reverse -- so the two regions are named separately here and never mixed.
//
//   These macros do NOT allocate.  D_ARCHIVE_EXTRACT_INTO takes both regions
// from the caller; D_ARCHIVE_EXTRACT_TO_ARRAYS takes them as named arrays and
// derives both capacities.  A caller with a heap grows its own regions between
// the measure and the extract.

// D_ARCHIVE_MEASURE_EXTRACT
//   macro: fill _layout with the entry count and arena size _in would require.
#define D_ARCHIVE_MEASURE_EXTRACT(_fmt, _in, _in_size, _layout)                \
    d_archive_measure(D_FORMAT(_fmt), (_in), (_in_size), NULL, (_layout))

// D_ARCHIVE_EXTRACT_INTO
//   macro: extract into caller-owned regions with default options.
#define D_ARCHIVE_EXTRACT_INTO(_fmt, _in, _in_size,                            \
                               _entries, _ecap, _arena, _acap, _used)          \
    d_archive_extract(D_FORMAT(_fmt), (_in), (_in_size), NULL,                 \
                      (_entries), (_ecap), (_arena), (_acap), (_used))

// D_ARCHIVE_EXTRACT_TO_ARRAYS
//   macro: extract into a named entry array and a named byte arena, both
// capacities taken from their declarations.  Yields
// D_PACK_STATUS_BUFFER_TOO_SMALL with *_used carrying BOTH requirements when
// either region is short, so one retry suffices.
#define D_ARCHIVE_EXTRACT_TO_ARRAYS(_fmt, _in, _in_size,                       \
                                    _entry_arr, _arena_arr, _used)             \
    d_archive_extract(D_FORMAT(_fmt), (_in), (_in_size), NULL,                 \
                      (_entry_arr), D_ARRAY_STATIC_SIZE(_entry_arr),           \
                      (_arena_arr), D_ARRAY_STATIC_SIZE(_arena_arr),           \
                      (_used))

// D_ARCHIVE_READ_ONE
//   macro: pull a single member out by name without materialising the rest.
// The common case -- reading one file from a container -- and the one that
// needs no arena at all.
#define D_ARCHIVE_READ_ONE(_fmt, _in, _in_size, _name, _out, _cap, _out_size)  \
    d_archive_extract_entry(D_FORMAT(_fmt), (_in), (_in_size),                 \
                            D_TEXT(_name), NULL,                               \
                            (_out), (_cap), (_out_size))


// =============================================================================
// VII. ITERATION
// =============================================================================
//   The loops a capability banner, a listing, and a differential sweep all
// write.  D_FOR_EACH_FORMAT visits every format the framework NAMES;
// D_FOR_EACH_WRITABLE_FORMAT visits only those this build can create.
//
//   The distinction is load-bearing here in a way it is not for codecs: RAR is
// readable by four backends and writable by none of them, so a sweep that
// iterated "supported" formats and tried to create each would report a formal
// failure for RAR on every correctly configured build.

// D_FOR_EACH_FORMAT
//   macro: iterate _var over every format the framework names.
#define D_FOR_EACH_FORMAT(_var)                                                \
    for (enum d_format_id _var = D_FORMAT_ID_ZIP;                              \
         (int)_var < D_FORMAT_ID_COUNT;                                        \
         _var = (enum d_format_id)((int)_var + 1))

// D_FOR_EACH_WRITABLE_FORMAT
//   macro: iterate _var over only the formats this build can create.
#define D_FOR_EACH_WRITABLE_FORMAT(_var)                                       \
    D_FOR_EACH_FORMAT(_var)                                                    \
        if (!d_format_is_writable(_var)) { continue; } else

// D_FOR_EACH_READABLE_FORMAT
//   macro: iterate _var over only the formats this build can read.
#define D_FOR_EACH_READABLE_FORMAT(_var)                                       \
    D_FOR_EACH_FORMAT(_var)                                                    \
        if (!d_format_is_readable(_var)) { continue; } else

// D_FOR_EACH_ENTRY
//   macro: iterate _var over an extracted entry list, given the layout the
// extract call filled in.  Reads the count from the layout rather than from a
// separate variable, which is where a hand-written loop drifts.
#define D_FOR_EACH_ENTRY(_var, _entries, _layout)                              \
    for (const struct d_archive_entry* _var = (_entries);                      \
         _var < ((_entries) + (_layout)->entry_count);                         \
         ++_var)


#endif  // DJINTERP_UTIL_ARCHIVE_
