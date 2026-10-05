/*******************************************************************************
* djinterp [core]                                            archive_options.hpp
*
*   Container tuning knobs for the C++ facade -- RE-BASED onto the shared
* kernel, but NOT the same way compress_options was, and the difference is
* worth understanding before editing either.
*
*   THE ASYMMETRY
*   -------------
*   compress_options DERIVES from d_compress_options at zero cost, because
* every one of its fifty knobs is numeric.  archive_options cannot: five of its
* knobs are text -- the archive comment, three passwords, and the gzip original
* filename -- and the C++ vocabulary for text is std::string while the kernel's
* is a borrowed span (pointer plus length, owning nothing).  A type holding
* std::string cannot also be a type holding d_pack_text.
*
*   So this one is a PARALLEL SHAPE with an explicit lowering.  That is a real
* cost -- a second field list that can drift from the kernel's, and a copy at
* every dispatch -- and it is accepted because the alternative is worse: making
* the C++ facade speak borrowed spans would push lifetime management onto every
* caller of a convenience API, to save one struct copy per archive.  Goals §4
* permits a cost demanded by another goal; it forbids an unnecessary one.  This
* is the former, and recording which is the point of this paragraph.
*
*   LIFETIME, STATED ONCE
*   ---------------------
*   lower() returns a d_archive_options whose text spans POINT INTO this object.
* It is valid for exactly as long as this object lives unmodified.  Every use
* in archive.cpp takes the lowering as a local, uses it within one call, and
* discards it -- which is the only pattern that is obviously correct, and the
* only one this header endorses.
*
*   WHAT IS SHARED ANYWAY
*   ---------------------
*   The embedded codec block is a compress_options, which IS a
* d_compress_options, so the fifty codec knobs have one definition even here.
* Only the container layer is duplicated, and only because of the five strings.
*
*   PORTABILITY:
*   Target floor is C++98.  The EFFECTIVE floor today is C++11, and not because
* of anything in this header -- djinterp.hpp gates below it outright ("djinterp
* requires C++11 or later"), so no header that includes it can compile at C++98
* regardless of what it uses.  Nothing here needs C++11 in its own right, so
* this returns to the target floor the day the gate moves.  Verified building
* at C++11, C++17 and C++20.
*
*
* path:      /inc/djinterp/core/util/archive/archive_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_UTIL_ARCHIVE_ARCHIVE_OPTIONS_HPP
#define DJINTERP_UTIL_ARCHIVE_ARCHIVE_OPTIONS_HPP 1

// std
#include <string>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../c/util/archive/archive_common.h"
#include "../compress/compress_options.hpp"


NS_DJINTERP

// =============================================================================
// I.   KNOB CONSTANT SETS
// =============================================================================
//   Aliases of the kernel's enums plus named constants, exactly as
// compress_options.hpp does: one declaration, two vocabularies.

typedef enum d_zip_method zip_method;
D_STATIC const zip_method zip_method_store   = D_ZIP_METHOD_STORE;
D_STATIC const zip_method zip_method_deflate = D_ZIP_METHOD_DEFLATE;
D_STATIC const zip_method zip_method_bzip2   = D_ZIP_METHOD_BZIP2;
D_STATIC const zip_method zip_method_lzma    = D_ZIP_METHOD_LZMA;
D_STATIC const zip_method zip_method_zstd    = D_ZIP_METHOD_ZSTD;
D_STATIC const zip_method zip_method_xz      = D_ZIP_METHOD_XZ;

typedef enum d_zip_encryption zip_encryption;
D_STATIC const zip_encryption zip_encryption_none    = D_ZIP_ENCRYPTION_NONE;
D_STATIC const zip_encryption zip_encryption_legacy  = D_ZIP_ENCRYPTION_LEGACY;
D_STATIC const zip_encryption zip_encryption_aes_128 = D_ZIP_ENCRYPTION_AES_128;
D_STATIC const zip_encryption zip_encryption_aes_192 = D_ZIP_ENCRYPTION_AES_192;
D_STATIC const zip_encryption zip_encryption_aes_256 = D_ZIP_ENCRYPTION_AES_256;

typedef enum d_tar_format tar_format;
D_STATIC const tar_format tar_format_ustar = D_TAR_FORMAT_USTAR;
D_STATIC const tar_format tar_format_gnu   = D_TAR_FORMAT_GNU;
D_STATIC const tar_format tar_format_pax   = D_TAR_FORMAT_PAX;
D_STATIC const tar_format tar_format_v7    = D_TAR_FORMAT_V7;

typedef enum d_sevenzip_method sevenzip_method;
D_STATIC const sevenzip_method sevenzip_method_lzma2   =
    D_SEVENZIP_METHOD_LZMA2;
D_STATIC const sevenzip_method sevenzip_method_lzma    =
    D_SEVENZIP_METHOD_LZMA;
D_STATIC const sevenzip_method sevenzip_method_bzip2   =
    D_SEVENZIP_METHOD_BZIP2;
D_STATIC const sevenzip_method sevenzip_method_deflate =
    D_SEVENZIP_METHOD_DEFLATE;
D_STATIC const sevenzip_method sevenzip_method_zstd    =
    D_SEVENZIP_METHOD_ZSTD;
D_STATIC const sevenzip_method sevenzip_method_copy    =
    D_SEVENZIP_METHOD_COPY;

// archive_knob_unset
//   constant: the pristine value of a numeric container knob, re-exported so a
// C++ caller can test "was this touched?" without naming a D_ prefixed macro.
// Aliases D_ARCHIVE_KNOB_UNSET.
D_STATIC const int archive_knob_unset = D_ARCHIVE_KNOB_UNSET;


// =============================================================================
// II.  PER-FORMAT BLOCKS
// =============================================================================
//   Declaration order MIRRORS the kernel's, member for member, so a reader can
// diff the two field lists.  The numeric members are int rather than the
// kernel's int32_t only where C++ callers already wrote int; lower() performs
// the (lossless, range-checked-by-construction) narrowing.

// zip_options
//   struct: the ZIP container block.
struct zip_options
{
    int          method;        // a zip_method value
    int          encryption;    // a zip_encryption value
    int          zip64;
    int          utf8_names;
    std::string  password;

    zip_options()
        : method(D_ARCHIVE_KNOB_UNSET),
          encryption(D_ARCHIVE_KNOB_UNSET),
          zip64(D_ARCHIVE_KNOB_UNSET),
          utf8_names(D_ARCHIVE_KNOB_UNSET)
    {}
};

// tar_options
//   struct: the tar container block.
struct tar_options
{
    int  format;                // a tar_format value
    int  numeric_owner;

    tar_options()
        : format(D_ARCHIVE_KNOB_UNSET),
          numeric_owner(D_ARCHIVE_KNOB_UNSET)
    {}
};

// gz_options
//   struct: the gzip header block.  Clearing store_name and store_mtime is
// what produces a byte-reproducible .gz, since RFC 1952 otherwise records the
// source filename and the current time.
struct gz_options
{
    int          store_name;
    int          store_mtime;
    std::string  original_name;

    gz_options()
        : store_name(D_ARCHIVE_KNOB_UNSET),
          store_mtime(D_ARCHIVE_KNOB_UNSET)
    {}
};

// sevenzip_options
//   struct: the 7z container block.
struct sevenzip_options
{
    int          method;        // a sevenzip_method value
    int          solid;
    int          header_compression;
    int          header_encryption;
    int          threads;
    std::string  password;

    sevenzip_options()
        : method(D_ARCHIVE_KNOB_UNSET),
          solid(D_ARCHIVE_KNOB_UNSET),
          header_compression(D_ARCHIVE_KNOB_UNSET),
          header_encryption(D_ARCHIVE_KNOB_UNSET),
          threads(D_ARCHIVE_KNOB_UNSET)
    {}
};

// rar_options
//   struct: the RAR creation block.  Present for completeness of the option
// vocabulary; no library can create RAR, so a create call naming that format
// resolves to status_unavailable unless the proprietary tool backend is there.
struct rar_options
{
    int          level;
    int          solid;
    int          recovery_record;
    std::string  password;

    rar_options()
        : level(D_ARCHIVE_KNOB_UNSET),
          solid(D_ARCHIVE_KNOB_UNSET),
          recovery_record(D_ARCHIVE_KNOB_UNSET)
    {}
};


// =============================================================================
// III. THE OPTION SET
// =============================================================================

// archive_options
//   struct: the whole container surface -- the archive-level knobs, the
// embedded codec tuning, and one block per format.
//
//   `level` and `codec.level` are deliberately distinct: the first is the
// container's effort, the second tunes the stream inside it.  A call site that
// consults one and not the other is testable precisely because they are
// separate knobs.
struct archive_options
{
    int                level;
    int                store_only;
    int                preserve_permissions;
    int                preserve_mtime;
    std::string        comment;
    compress_options   codec;
    zip_options        zip;
    tar_options        tar;
    gz_options         gz;
    sevenzip_options   sevenzip;
    rar_options        rar;

    archive_options()
        : level(D_ARCHIVE_KNOB_UNSET),
          store_only(D_ARCHIVE_KNOB_UNSET),
          preserve_permissions(D_ARCHIVE_KNOB_UNSET),
          preserve_mtime(D_ARCHIVE_KNOB_UNSET)
    {}

    // lower
    //   function: the kernel's view of this option set.  Text spans BORROW
    // from this object and are valid only while it lives unmodified -- see the
    // header banner.  Use the result within a single call and discard it.
    d_archive_options lower() const
    {
        d_archive_options out;

        d_archive_options_init(&out);

        out.level                = (int32_t)level;
        out.store_only           = (int32_t)store_only;
        out.preserve_permissions = (int32_t)preserve_permissions;
        out.preserve_mtime       = (int32_t)preserve_mtime;
        out.comment              = span(comment);

        // the codec block IS a d_compress_options; this is a base-class copy,
        // not a field-by-field translation, so it cannot drift.
        out.codec = codec;

        out.zip.method             = (int32_t)zip.method;
        out.zip.encryption         = (int32_t)zip.encryption;
        out.zip.zip64              = (int32_t)zip.zip64;
        out.zip.utf8_names         = (int32_t)zip.utf8_names;
        out.zip.password           = span(zip.password);

        out.tar.format             = (int32_t)tar.format;
        out.tar.numeric_owner      = (int32_t)tar.numeric_owner;

        out.gz.store_name          = (int32_t)gz.store_name;
        out.gz.store_mtime         = (int32_t)gz.store_mtime;
        out.gz.original_name       = span(gz.original_name);

        out.sevenzip.method             = (int32_t)sevenzip.method;
        out.sevenzip.solid              = (int32_t)sevenzip.solid;
        out.sevenzip.header_compression = (int32_t)sevenzip.header_compression;
        out.sevenzip.header_encryption  = (int32_t)sevenzip.header_encryption;
        out.sevenzip.threads            = (int32_t)sevenzip.threads;
        out.sevenzip.password           = span(sevenzip.password);

        out.rar.level              = (int32_t)rar.level;
        out.rar.solid              = (int32_t)rar.solid;
        out.rar.recovery_record    = (int32_t)rar.recovery_record;
        out.rar.password           = span(rar.password);

        return out;
    }

private:
    // span
    //   function: a borrowed view of _s.  An empty string yields the null span
    // rather than a pointer to a terminator, so "unset" and "set to empty" are
    // the same answer -- which is what the kernel's knob comparison expects.
    static d_pack_text span(const std::string& _s)
    {
        d_pack_text t;

        t.data   = _s.empty() ? (const char*)0 : _s.data();
        t.length = _s.size();

        return t;
    }
};


// =============================================================================
// IV.  COMPARISON
// =============================================================================
//   Forwards to the kernel's knob walk over the lowered forms, so "equal"
// means the same thing in both languages.  The lowerings are locals and die at
// the end of the expression, which is exactly the endorsed lifetime.

inline bool
operator==(const archive_options& _a, const archive_options& _b)
{
    d_archive_options la = _a.lower();
    d_archive_options lb = _b.lower();

    return (d_archive_options_equal(&la, &lb) != 0);
}

inline bool
operator!=(const archive_options& _a, const archive_options& _b)
{
    return !(_a == _b);
}



// =============================================================================
// V.   PER-FORMAT BLOCK COMPARISON
// =============================================================================
//   One comparator per format block, for a caller that wants to prove a single
// block moved (or did not) rather than asking about the whole aggregate.
//
//   Each embeds the two blocks in otherwise-pristine option sets and asks the
// kernel, so there is still exactly ONE knob walk in the codebase and a knob
// added to d_archive_options is covered here the day it lands.  It also means
// the text knobs (passwords, gz.original_name) compare by span rather than by
// pointer, which a hand-written field chain gets wrong quietly.
//
//   These do not partition the knob table by format, and there is no kernel
// function that would let them.  d_format_id has no "belongs to no format"
// enumerator, so any knob-to-format map has to put the six archive-level knobs
// (level, store_only, preserve_permissions, preserve_mtime, comment, codec)
// somewhere untrue.  d_archive_knob_format used to answer ZIP for them and was
// deleted for that reason -- it had no callers, and its one plausible use was
// the wrong one.  Embedding sidesteps the question entirely.

// zip_options_equal / tar_options_equal / ...
//   function: whether two per-format blocks carry the same knob values.
inline bool
zip_options_equal(
    const zip_options& _a,
    const zip_options& _b
)
{
    archive_options   oa;
    archive_options   ob;

    oa.zip = _a;
    ob.zip = _b;

    d_archive_options la = oa.lower();
    d_archive_options lb = ob.lower();

    return (d_archive_options_equal(&la, &lb) != 0);
}

inline bool
tar_options_equal(
    const tar_options& _a,
    const tar_options& _b
)
{
    archive_options   oa;
    archive_options   ob;

    oa.tar = _a;
    ob.tar = _b;

    d_archive_options la = oa.lower();
    d_archive_options lb = ob.lower();

    return (d_archive_options_equal(&la, &lb) != 0);
}

inline bool
gz_options_equal(
    const gz_options& _a,
    const gz_options& _b
)
{
    archive_options   oa;
    archive_options   ob;

    oa.gz = _a;
    ob.gz = _b;

    d_archive_options la = oa.lower();
    d_archive_options lb = ob.lower();

    return (d_archive_options_equal(&la, &lb) != 0);
}

inline bool
sevenzip_options_equal(
    const sevenzip_options& _a,
    const sevenzip_options& _b
)
{
    archive_options   oa;
    archive_options   ob;

    oa.sevenzip = _a;
    ob.sevenzip = _b;

    d_archive_options la = oa.lower();
    d_archive_options lb = ob.lower();

    return (d_archive_options_equal(&la, &lb) != 0);
}

inline bool
rar_options_equal(
    const rar_options& _a,
    const rar_options& _b
)
{
    archive_options   oa;
    archive_options   ob;

    oa.rar = _a;
    ob.rar = _b;

    d_archive_options la = oa.lower();
    d_archive_options lb = ob.lower();

    return (d_archive_options_equal(&la, &lb) != 0);
}

// archive_options_equal
//   function: the named spelling of operator==, kept because a caller reading
// a chain of block comparisons wants the aggregate to read the same way.
inline bool
archive_options_equal(
    const archive_options& _a,
    const archive_options& _b
)
{
    return (_a == _b);
}


// =============================================================================
// VI.  PRISTINE-DEFAULT PREDICATES
// =============================================================================
//   "Did this call leave the tuning alone?"  Both forward to the kernel, which
// owns the pristine value of every knob, rather than comparing against a
// locally constructed baseline.

// default_archive_options
//   function: a pristine set.  Named rather than left to the default
// constructor so that "compare against pristine" reads as one expression.
inline archive_options
default_archive_options()
{
    return archive_options();
}

// options_are_default
//   function: whether nothing anywhere -- archive level, embedded codec, or
// any format block -- has been touched.
inline bool
options_are_default(
    const archive_options& _opt
)
{
    d_archive_options lowered = _opt.lower();

    return (d_archive_options_are_default(&lowered) != 0);
}

// codec_is_default
//   function: whether the EMBEDDED CODEC alone is pristine, saying nothing
// about the archive-level or per-format knobs.  The question a caller asks
// when it wants to prove a container knob moved without disturbing the stream
// tuning inside it.
inline bool
codec_is_default(
    const archive_options& _opt
)
{
    d_archive_options lowered = _opt.lower();

    return (d_archive_options_codec_is_default(&lowered) != 0);
}


// =============================================================================
// VII. DIFF DESCRIPTION
// =============================================================================
//   The kernel answers WHICH knobs moved as a dense index list; naming them is
// d_archive_knob_name's job and formatting is the caller's.  This is that
// caller.
//
//   THE CODEC IS REPORTED COARSELY, ON PURPOSE.  Archive-level and per-format
// knobs are named individually, because those are the knobs call sites
// actually set.  The embedded codec is reported at BLOCK granularity
// ("codec.zstd") with codec.level called out by name -- naming all fifty codec
// fields would bury the signal for a caller tuning a container.  When the
// individual knob matters, call describe_compress_option_diff on the .codec
// member, which names every field.

// describe_option_diff
//   function: a comma-separated list of every knob that differs between two
// option sets, or an empty string when the two are identical.
inline std::string
describe_option_diff(
    const archive_options& _a,
    const archive_options& _b
)
{
    enum d_archive_knob moved[D_ARCHIVE_KNOB_COUNT];
    d_archive_options   la = _a.lower();
    d_archive_options   lb = _b.lower();
    std::string         out;
    size_t              n;
    size_t              i;

    n = d_archive_options_diff(&la, &lb, moved, (size_t)D_ARCHIVE_KNOB_COUNT);

    for (i = 0; i < n; ++i)
    {
        if (moved[i] != D_ARCHIVE_KNOB_CODEC)
        {
            if (!out.empty())
            {
                out += ", ";
            }
            out += d_archive_knob_name(moved[i]);

            continue;
        }

        // the codec block: expand to block granularity, level by name.  The
        // block set is walked in knob order and de-duplicated, so a codec
        // whose knobs are spread across the table still names it once.
        {
            enum d_compress_knob cmoved[D_COMPRESS_KNOB_COUNT];
            size_t               cn;
            size_t               j;
            int                  seen[D_CODEC_ID_COUNT];
            int                  k;
            int                  level_named = 0;

            for (k = 0; k < D_CODEC_ID_COUNT; ++k)
            {
                seen[k] = 0;
            }

            cn = d_compress_options_diff(&_a.codec,
                                         &_b.codec,
                                         cmoved,
                                         (size_t)D_COMPRESS_KNOB_COUNT);

            for (j = 0; j < cn; ++j)
            {
                if (cmoved[j] == D_COMPRESS_KNOB_LEVEL)
                {
                    if (level_named)
                    {
                        continue;
                    }
                    level_named = 1;

                    if (!out.empty())
                    {
                        out += ", ";
                    }
                    out += "codec.level";

                    continue;
                }

                {
                    enum d_codec_id c = d_compress_knob_codec(cmoved[j]);

                    if (!D_CODEC_ID_IS_VALID(c) || seen[(int)c])
                    {
                        continue;
                    }
                    seen[(int)c] = 1;

                    if (!out.empty())
                    {
                        out += ", ";
                    }
                    out += "codec.";
                    out += d_codec_id_name(c);
                }
            }
        }
    }

    return out;
}

// describe_diff_from_default
//   function: the same list taken against a freshly constructed set -- "what
// has this option set changed from pristine?".
inline std::string
describe_diff_from_default(
    const archive_options& _opt
)
{
    const archive_options fresh;

    return describe_option_diff(_opt, fresh);
}


// =============================================================================
// VIII. OPTION BUILDERS
// =============================================================================
//   Small presets.  Each starts from a pristine set and moves exactly one
// thing, so a caller can assert both the intended change and that nothing else
// drifted.

// store_only_options
//   function: a pristine set that asks for no compression at all.
inline archive_options
store_only_options()
{
    archive_options opt;

    opt.store_only = 1;

    return opt;
}

// level_options
//   function: a pristine set with the CONTAINER's effort set, leaving the
// embedded codec's own level untouched.
inline archive_options
level_options(
    int _level
)
{
    archive_options opt;

    opt.level = _level;

    return opt;
}

// codec_level_options
//   function: a pristine set with the EMBEDDED CODEC's level set, leaving the
// container's effort untouched.  Pairs with level_options for testing which of
// the two a call site actually consults.
inline archive_options
codec_level_options(
    int _level
)
{
    archive_options opt;

    opt.codec.level = _level;

    return opt;
}

// zip_method_options
//   function: a pristine set with the zip block's method selected.
inline archive_options
zip_method_options(
    zip_method _method
)
{
    archive_options opt;

    opt.zip.method = _method;

    return opt;
}

// zip_encrypted_options
//   function: a pristine set with the zip block's encryption scheme and
// password set together, since neither is meaningful alone.
inline archive_options
zip_encrypted_options(
    zip_encryption     _scheme,
    const std::string& _password
)
{
    archive_options opt;

    opt.zip.encryption = _scheme;
    opt.zip.password   = _password;

    return opt;
}



NS_END  // djinterp


#endif  // DJINTERP_UTIL_ARCHIVE_ARCHIVE_OPTIONS_HPP
