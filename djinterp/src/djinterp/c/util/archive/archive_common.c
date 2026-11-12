/*******************************************************************************
* djinterp [c]                                                  archive_common.c
*
*   The archive kernel.  Every declaration in archive_common.h is defined here.
*
*   THIS FILE CLOSES THE `BUILTIN` DEFECT:
*   env_archive.h has always advertised D_ENV_ARCHIVE_HAVE_BUILTIN_TAR and
* _BUILTIN_ZIP as 1, and D_ENV_ARCHIVE_BACKEND_BUILTIN has always been
* enumerated -- but no built-in writer existed, so tar and zip support in
* practice depended on which third-party headers happened to be installed.
* That is parity failing for ENVIRONMENTAL reasons, which is the one way it is
* not allowed to fail.  Sections VIII and IX are dependency-free ustar and ZIP
* writers and readers; with them, every build can create and read those two
* formats, and a detected library becomes an optimisation rather than a
* precondition.
*
*   STORE IS ALWAYS REACHABLE:
*   The built-in ZIP writer's DEFLATE method needs a DEFLATE codec, which a
* build may lack.  It falls back to the store method rather than failing, so a
* zero-dependency build still produces a valid, universally readable ZIP.  The
* archive is bigger; it is not absent.
*
*   NO ALLOCATION, AND WHAT THAT COSTS:
*   A ZIP local header records the compressed size and CRC of the entry that
* FOLLOWS it, so a streaming writer must either know them in advance or emit a
* trailing data descriptor.  Data descriptors are handled poorly by several
* readers, so this writer knows them in advance.  With no memory of its own,
* it runs the codec three times per entry -- into a counting sink to learn the
* size, into the real sink, and into a counting sink again for the central
* directory.  Given a STAGE (d_archive_create_staged), it runs the codec once,
* into the stage, and keeps each entry's plan there for the directory.
*
*   tar.gz needs a stage outright: the tar has to exist before gzip can take
* it, and the caller names where.  Reading needs none in any format: a tar.gz
* is inflated straight through the tar reader, which takes its input in
* pieces.  The store method pays nothing anywhere, because its compressed size
* is its input size.
*
*   REPRODUCIBILITY IS THE DEFAULT:
*   Nothing here reads the clock, the umask, the environment, or the locale.
* An entry with mtime 0 is written with D_ARCHIVE_EPOCH_DEFAULT, not "now", and
* mode 0 becomes a pinned constant, not the process umask.  Two runs over
* identical entries produce identical bytes on every machine -- which is the
* precondition for the differential test comparing archives at all.
*
*
* path:      /src/djinterp/c/util/archive/archive_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
II.   FORMAT IDENTITY
      ---------------

III.  THE KNOB TABLE
      --------------

IV.   OPTION OPERATIONS
      -----------------

V.    CAPABILITY AND ENTRY VALIDITY
      -----------------------------

VI.   SIGNATURES
      ----------

VII.  BYTE-ORDER PRIMITIVES
      ---------------------

VIII. THE BUILT-IN USTAR WRITER / READER
      ----------------------------------

IX.   THE BUILT-IN ZIP WRITER / READER
      --------------------------------

X.    TRANSFORMS
      ----------
*/

// std
// c
#include <string.h>
// djinterp
#include "../../../../../inc/djinterp/c/util/archive/archive_common.h"
#include "../../../../../inc/djinterp/env/util/archive/env_archive.h"
                                        // D_ENV_ARCHIVE_BACKEND_*
// std, once the configuration has said whether the clock is read
#if D_INTERNAL_ARCHIVE_USE_CLOCK
    #include <time.h>  // time, for an entry with no mtime
#endif


// D_ARCHIVE_ZIP_EPOCH
//   constant: 1980-01-01T00:00:00Z, the ZIP format's own epoch and therefore
// the earliest instant both tar and zip can represent without a special case.
// A ZIP timestamp before it is clamped to it.
#define D_ARCHIVE_ZIP_EPOCH         315532800L

// D_ARCHIVE_EPOCH_DEFAULT
//   constant: the timestamp written for an entry whose mtime is 0:
// D_CFG_ARCHIVE_EPOCH, resolved in cfg_archive.h, which may not precede the
// ZIP epoch and defaults to it. Pinned rather than taken from the clock under
// D_CFG_ARCHIVE_REPRODUCIBLE (the default): a container that embeds "now" is a
// container two identical runs cannot be compared byte for byte.
#define D_ARCHIVE_EPOCH_DEFAULT     D_INTERNAL_ARCHIVE_EPOCH

// D_INTERNAL_TAR_BLOCK
//   constant: the ustar record size.  Everything in a tar is a multiple of it.
#define D_INTERNAL_TAR_BLOCK        512u

// D_INTERNAL_ZIP_LOCAL_SIZE / _CENTRAL_SIZE / _EOCD_SIZE
//   constant: the fixed portion of each ZIP structure, name and extra fields
// excluded.
#define D_INTERNAL_ZIP_LOCAL_SIZE   30u
#define D_INTERNAL_ZIP_CENTRAL_SIZE 46u
#define D_INTERNAL_ZIP_EOCD_SIZE    22u


// =============================================================================
// II.  FORMAT IDENTITY
// =============================================================================

// d_internal_format_names
//   table: the format names, indexed by dj_format_id.
static const char* const d_internal_format_names[D_FORMAT_ID_COUNT] =
{
    "zip", "tar", "gz", "tar_gz", "sevenzip", "rar"
};

// d_internal_format_suffixes
//   table: the conventional file suffix for each format, indexed by
// dj_format_id.
static const char* const d_internal_format_suffixes[D_FORMAT_ID_COUNT] =
{
    ".zip", ".tar", ".gz", ".tar.gz", ".7z", ".rar"
};

/*
d_format_id_name
  The format's canonical name.

Parameter(s):
  _format: the format to name.
Return:
  A static, NUL-terminated name, or "unknown".
*/
const char*
d_format_id_name(
    enum dj_format_id _format
)
{
    if (!D_FORMAT_ID_IS_VALID(_format))
    {
        return "unknown";
    }

    return d_internal_format_names[(int)_format];
}

/*
d_format_suffix
  The conventional file suffix, leading dot included.

Parameter(s):
  _format: the format to describe.
Return:
  A static, NUL-terminated suffix.
*/
const char*
d_format_suffix(
    enum dj_format_id _format
)
{
    if (!D_FORMAT_ID_IS_VALID(_format))
    {
        return "";
    }

    return d_internal_format_suffixes[(int)_format];
}

/*
d_format_id_from_name
  Resolve a format name, accepting the canonical spelling and the aliases a
command line is likely to carry.

Parameter(s):
  _name:       the name; may be null, which never matches.
  _out_format: receives the format on success; untouched on failure.
Return:
  1 when the name resolved, 0 otherwise.
*/
int
d_format_id_from_name(
    const char*       _name,
    enum dj_format_id* _out_format
)
{
    int i;

    if ( (_name == NULL) ||
         (_out_format == NULL) )
    {
        return 0;
    }

    for (i = 0; i < D_FORMAT_ID_COUNT; ++i)
    {
        if (strcmp(_name, d_internal_format_names[i]) == 0)
        {
            *_out_format = (enum dj_format_id)i;

            return 1;
        }
    }

    if ( (strcmp(_name, "7z")   == 0) ||
         (strcmp(_name, "7zip") == 0) )
    {
        *_out_format = D_FORMAT_ID_SEVENZIP;

        return 1;
    }
    if ( (strcmp(_name, "tgz")    == 0) ||
         (strcmp(_name, "tar.gz") == 0) )
    {
        *_out_format = D_FORMAT_ID_TAR_GZ;

        return 1;
    }
    if (strcmp(_name, "gzip") == 0)
    {
        *_out_format = D_FORMAT_ID_GZ;

        return 1;
    }

    return 0;
}

/*
d_format_id_from_suffix
  Infer a format from a path's extension.

  The compound suffixes are tested BEFORE the simple ones, because ".tar.gz"
also ends in ".gz" and a shortest-match walk would classify every tarball as a
bare gzip stream -- silently producing a single-member container where a
multi-member one was meant.

Parameter(s):
  _path:       the path to inspect; may be null, which never matches.
  _out_format: receives the format on success; untouched on failure.
Return:
  1 when a suffix was recognised, 0 otherwise.
*/
int
d_format_id_from_suffix(
    const char*       _path,
    enum dj_format_id* _out_format
)
{
    static const char* const compound[]  = { ".tar.gz", ".tgz" };
    static const int         compound_n  = 2;

    size_t len;
    int    i;

    if ( (_path == NULL) ||
         (_out_format == NULL) )
    {
        return 0;
    }

    len = strlen(_path);

    for (i = 0; i < compound_n; ++i)
    {
        size_t sl = strlen(compound[i]);

        if ( (len >= sl) &&
             (strcmp(_path + (len - sl), compound[i]) == 0) )
        {
            *_out_format = D_FORMAT_ID_TAR_GZ;

            return 1;
        }
    }

    for (i = 0; i < D_FORMAT_ID_COUNT; ++i)
    {
        size_t sl = strlen(d_internal_format_suffixes[i]);

        if ( (len >= sl) &&
             (strcmp(_path + (len - sl), d_internal_format_suffixes[i]) == 0) )
        {
            *_out_format = (enum dj_format_id)i;

            return 1;
        }
    }

    return 0;
}

/*
d_format_default_codec
  The codec a format uses for its members when nothing else is requested.

Parameter(s):
  _format: the format to query.
Return:
  The codec.  tar is STORE because tar itself does not compress -- compression
is the outer layer, which is precisely what distinguishes tar from tar_gz.
*/
enum dj_codec_id
d_format_default_codec(
    enum dj_format_id _format
)
{
    switch (_format)
    {
        case D_FORMAT_ID_ZIP:      { return D_CODEC_ID_DEFLATE; }
        case D_FORMAT_ID_TAR:      { return D_CODEC_ID_STORE; }
        case D_FORMAT_ID_GZ:
        case D_FORMAT_ID_TAR_GZ:   { return D_CODEC_ID_GZIP; }
        case D_FORMAT_ID_SEVENZIP: { return D_CODEC_ID_XZ; }
        case D_FORMAT_ID_RAR:      { return D_CODEC_ID_STORE; }
    }

    return D_CODEC_ID_STORE;
}


// =============================================================================
// III. THE KNOB TABLE
// =============================================================================

// d_internal_archive_knob_names
//   table: the qualified name of each container knob, indexed by
// dj_archive_knob.  These are the strings a diff report prints and a config
// file binds against.
static const char* const
d_internal_archive_knob_names[D_ARCHIVE_KNOB_COUNT] =
{
    "level", "store_only", "preserve_permissions", "preserve_mtime",
    "comment", "codec",
    "zip.method", "zip.encryption", "zip.zip64", "zip.utf8_names",
    "zip.password",
    "tar.format", "tar.numeric_owner",
    "gz.store_name", "gz.store_mtime", "gz.original_name",
    "sevenzip.method", "sevenzip.solid", "sevenzip.header_compression",
    "sevenzip.header_encryption", "sevenzip.threads", "sevenzip.password",
    "rar.level", "rar.solid", "rar.recovery_record", "rar.password"
};

/*
d_archive_knob_name
  The knob's qualified name.

Parameter(s):
  _knob: the knob to name.
Return:
  A static, NUL-terminated name, or "unknown".
*/
const char*
d_archive_knob_name(
    enum dj_archive_knob _knob
)
{
    if ( ((int)_knob < 0) ||
         ((int)_knob >= D_ARCHIVE_KNOB_COUNT) )
    {
        return "unknown";
    }

    return d_internal_archive_knob_names[(int)_knob];
}

/*
d_archive_knob_is_text
  Whether a knob carries a text span rather than a number.

  Callers need this because the two kinds compare differently -- a numeric knob
by value, a text knob by d_pack_text_equal -- and a generic walk over the knob
set has to branch somewhere.  Branching here keeps that decision in one place.

Parameter(s):
  _knob: the knob to classify.
Return:
  1 for a text knob, 0 for a numeric one or the embedded codec.
*/
int
d_archive_knob_is_text(
    enum dj_archive_knob _knob
)
{
    switch (_knob)
    {
        case D_ARCHIVE_KNOB_COMMENT:
        case D_ARCHIVE_KNOB_ZIP_PASSWORD:
        case D_ARCHIVE_KNOB_GZ_ORIGINAL_NAME:
        case D_ARCHIVE_KNOB_SEVENZIP_PASSWORD:
        case D_ARCHIVE_KNOB_RAR_PASSWORD:
        {
            return 1;
        }
        default:
        {
            return 0;
        }
    }
}



// =============================================================================
// IV.  OPTION OPERATIONS
// =============================================================================

/*
d_internal_archive_knob_value
  Read a numeric container knob by index.

  A switch rather than an offset table, because dj_archive_options is NOT
densely packed -- it mixes int32_t knobs with pointer-bearing spans, so the
index-to-offset arithmetic that works for dj_compress_options does not apply
here.  The switch is the honest form; the header's assertions say why.

Parameter(s):
  _options: the set to read.
  _knob:    the knob to read.
Return:
  The value, or D_ARCHIVE_KNOB_UNSET for a text knob or unknown index.
*/
static int32_t
d_internal_archive_knob_value(
    const struct dj_archive_options* _options,
    enum dj_archive_knob             _knob
)
{
    if (_options == NULL)
    {
        return D_ARCHIVE_KNOB_UNSET;
    }

    switch (_knob)
    {
        case D_ARCHIVE_KNOB_LEVEL:      { return _options->level; }
        case D_ARCHIVE_KNOB_STORE_ONLY: { return _options->store_only; }
        case D_ARCHIVE_KNOB_PRESERVE_PERMISSIONS:
        {
            return _options->preserve_permissions;
        }
        case D_ARCHIVE_KNOB_PRESERVE_MTIME:
        {
            return _options->preserve_mtime;
        }
        case D_ARCHIVE_KNOB_ZIP_METHOD:     { return _options->zip.method; }
        case D_ARCHIVE_KNOB_ZIP_ENCRYPTION: { return _options->zip.encryption; }
        case D_ARCHIVE_KNOB_ZIP_ZIP64:      { return _options->zip.zip64; }
        case D_ARCHIVE_KNOB_ZIP_UTF8_NAMES: { return _options->zip.utf8_names; }
        case D_ARCHIVE_KNOB_TAR_FORMAT:     { return _options->tar.format; }
        case D_ARCHIVE_KNOB_TAR_NUMERIC_OWNER:
        {
            return _options->tar.numeric_owner;
        }
        case D_ARCHIVE_KNOB_GZ_STORE_NAME:  { return _options->gz.store_name; }
        case D_ARCHIVE_KNOB_GZ_STORE_MTIME: { return _options->gz.store_mtime; }
        case D_ARCHIVE_KNOB_SEVENZIP_METHOD:
        {
            return _options->sevenzip.method;
        }
        case D_ARCHIVE_KNOB_SEVENZIP_SOLID:
        {
            return _options->sevenzip.solid;
        }
        case D_ARCHIVE_KNOB_SEVENZIP_HEADER_COMP:
        {
            return _options->sevenzip.header_compression;
        }
        case D_ARCHIVE_KNOB_SEVENZIP_HEADER_ENCRYPT:
        {
            return _options->sevenzip.header_encryption;
        }
        case D_ARCHIVE_KNOB_SEVENZIP_THREADS:
        {
            return _options->sevenzip.threads;
        }
        case D_ARCHIVE_KNOB_RAR_LEVEL: { return _options->rar.level; }
        case D_ARCHIVE_KNOB_RAR_SOLID: { return _options->rar.solid; }
        case D_ARCHIVE_KNOB_RAR_RECOVERY_RECORD:
        {
            return _options->rar.recovery_record;
        }
        default:
        {
            return D_ARCHIVE_KNOB_UNSET;
        }
    }
}

/*
d_internal_archive_knob_text
  Read a text container knob by index.

Parameter(s):
  _options: the set to read.
  _knob:    the knob to read.
Return:
  The span, or an empty span for a numeric knob or unknown index.
*/
static struct dj_pack_text
d_internal_archive_knob_text(
    const struct dj_archive_options* _options,
    enum dj_archive_knob             _knob
)
{
    struct dj_pack_text empty;

    empty.data   = NULL;
    empty.length = 0u;

    if (_options == NULL)
    {
        return empty;
    }

    switch (_knob)
    {
        case D_ARCHIVE_KNOB_COMMENT:      { return _options->comment; }
        case D_ARCHIVE_KNOB_ZIP_PASSWORD: { return _options->zip.password; }
        case D_ARCHIVE_KNOB_GZ_ORIGINAL_NAME:
        {
            return _options->gz.original_name;
        }
        case D_ARCHIVE_KNOB_SEVENZIP_PASSWORD:
        {
            return _options->sevenzip.password;
        }
        case D_ARCHIVE_KNOB_RAR_PASSWORD: { return _options->rar.password; }
        default:                          { return empty; }
    }
}

/*
d_archive_options_init
  Set every numeric knob to UNSET, every text knob empty, and the embedded
codec block to pristine.

Parameter(s):
  _options: the set to reset; may be null, which does nothing.
Return:
  none.
*/
void d_archive_options_init(
    struct dj_archive_options* _options
)
{
    struct dj_archive_options fresh = D_ARCHIVE_OPTIONS_INIT;

    if (_options != NULL)
    {
        *_options = fresh;
    }

    return;
}

/*
d_archive_options_equal
  Whether two sets are knob-for-knob identical, embedded codec included.

Parameter(s):
  _a: the first set; may be null.
  _b: the second set; may be null.
Return:
  1 when both are null or every knob matches, 0 otherwise.
*/
int d_archive_options_equal(
    const struct dj_archive_options* _a,
    const struct dj_archive_options* _b
)
{
    int i;

    if ( (_a == NULL) ||
         (_b == NULL) )
    {
        return (_a == _b) ? 1 : 0;
    }

    for (i = 0; i < D_ARCHIVE_KNOB_COUNT; ++i)
    {
        enum dj_archive_knob k = (enum dj_archive_knob)i;

        if (k == D_ARCHIVE_KNOB_CODEC)
        {
            if (!d_compress_options_equal(&_a->codec, &_b->codec))
            {
                return 0;
            }

            continue;
        }
        if (d_archive_knob_is_text(k))
        {
            if (!d_pack_text_equal(d_internal_archive_knob_text(_a, k),
                                   d_internal_archive_knob_text(_b, k)))
            {
                return 0;
            }

            continue;
        }
        if (d_internal_archive_knob_value(_a, k) !=
            d_internal_archive_knob_value(_b, k))
        {
            return 0;
        }
    }

    return 1;
}

/*
d_archive_options_are_default
  Whether nothing anywhere in the set, codec included, has been touched.

Parameter(s):
  _options: the set to inspect; may be null, which is not pristine.
Return:
  1 when the set is pristine, 0 otherwise.
*/
int d_archive_options_are_default(
    const struct dj_archive_options* _options
)
{
    struct dj_archive_options fresh = D_ARCHIVE_OPTIONS_INIT;

    if (_options == NULL)
    {
        return 0;
    }

    return d_archive_options_equal(_options, &fresh);
}

/*
d_archive_options_codec_is_default
  Whether only the embedded codec block is pristine.

  Separate from the whole-set predicate because the common assertion is that a
call touched the container knobs and left the stream tuning alone, and that is
not expressible by comparing the whole set.

Parameter(s):
  _options: the set to inspect; may be null, which is not pristine.
Return:
  1 when the codec block is untouched, 0 otherwise.
*/
int d_archive_options_codec_is_default(
    const struct dj_archive_options* _options
)
{
    if (_options == NULL)
    {
        return 0;
    }

    return d_compress_options_are_default(&_options->codec);
}

/*
d_archive_options_diff
  The container knobs that differ between two sets.

  The embedded codec is reported COARSELY, as the single index
D_ARCHIVE_KNOB_CODEC, however many of its fifty knobs moved.  A call site
tuning a container rarely cares which of nineteen zstd knobs changed, and a
caller that does care passes the two codec blocks to d_compress_options_diff
directly.  Two granularities, one per question, neither reimplementing the
other.

Parameter(s):
  _a:            the first set.
  _b:            the second set.
  _out_knobs:    receives the differing indices; may be null to count only.
  _out_capacity: how many indices _out_knobs holds.
Return:
  The total number of differing knobs, which may exceed _out_capacity.
*/
size_t
d_archive_options_diff(
    const struct dj_archive_options* _a,
    const struct dj_archive_options* _b,
    enum dj_archive_knob*            _out_knobs,
    size_t                          _out_capacity
)
{
    size_t found = 0;
    int    i;

    if ( (_a == NULL) ||
         (_b == NULL) )
    {
        return 0;
    }

    for (i = 0; i < D_ARCHIVE_KNOB_COUNT; ++i)
    {
        enum dj_archive_knob k       = (enum dj_archive_knob)i;
        int                 differs = 0;

        if (k == D_ARCHIVE_KNOB_CODEC)
        {
            differs = !d_compress_options_equal(&_a->codec, &_b->codec);
        }
        else if (d_archive_knob_is_text(k))
        {
            differs = !d_pack_text_equal(d_internal_archive_knob_text(_a, k),
                                         d_internal_archive_knob_text(_b, k));
        }
        else
        {
            differs = (d_internal_archive_knob_value(_a, k) !=
                       d_internal_archive_knob_value(_b, k));
        }

        if (!differs)
        {
            continue;
        }
        if ( (_out_knobs != NULL) &&
             (found < _out_capacity) )
        {
            _out_knobs[found] = k;
        }

        ++found;
    }

    return found;
}

/*
d_internal_knob_default
  Give a numeric knob its default when the caller left it unset.  A knob the
caller set is never replaced.

Parameter(s):
  _knob:  the knob.
  _value: its default.
Return:
  none.
*/
static
void d_internal_knob_default(
    int32_t* _knob,
    int32_t  _value
)
{
    if (*_knob == D_ARCHIVE_KNOB_UNSET)
    {
        *_knob = _value;
    }

    return;
}

/*
d_internal_resolve_zip
  The ZIP block's defaults.  With no build choice of method, DEFLATE when
this build has it and store when not, so a zero-dependency build still writes
a valid, readable ZIP; an entry whose codec is missing is stored anyway
(d_internal_zip_entry_method).  store_only overrides the method.

Parameter(s):
  _work: the set being resolved.
Return:
  none.
*/
static
void d_internal_resolve_zip(
    struct dj_archive_options* _work
)
{
#if ( D_INTERNAL_ARCHIVE_ZIP_METHOD < 0 )
    d_internal_knob_default(&_work->zip.method,
                            d_codec_can_compress(D_CODEC_ID_DEFLATE)
                            ? D_ZIP_METHOD_DEFLATE
                            : D_ZIP_METHOD_STORE);
#else
    d_internal_knob_default(&_work->zip.method,
                            D_INTERNAL_ARCHIVE_ZIP_METHOD);
#endif

    if (_work->store_only)
    {
        _work->zip.method = D_ZIP_METHOD_STORE;
    }

    d_internal_knob_default(&_work->zip.encryption, D_ZIP_ENCRYPTION_NONE);
    d_internal_knob_default(&_work->zip.zip64,      0);
    d_internal_knob_default(&_work->zip.utf8_names, 1);

    return;
}

/*
d_internal_resolve_blocks
  The defaults of _format's own block; the other blocks pass through
untouched.  tar's format defaults to pax: ustar for every member that fits,
with a pax record for a name, size or timestamp that does not, so a long name
is written rather than refused.  7z's store is its copy method.

Parameter(s):
  _format: the format.
  _work:   the set being resolved.
Return:
  none.
*/
static
void d_internal_resolve_blocks(
    enum dj_format_id          _format,
    struct dj_archive_options* _work
)
{
    switch (_format)
    {
        case D_FORMAT_ID_ZIP:
        {
            d_internal_resolve_zip(_work);
            break;
        }
        case D_FORMAT_ID_TAR:
        case D_FORMAT_ID_TAR_GZ:
        {
            d_internal_knob_default(&_work->tar.format, D_TAR_FORMAT_PAX);
            d_internal_knob_default(&_work->tar.numeric_owner, 1);
            break;
        }
        case D_FORMAT_ID_GZ:
        {
            d_internal_knob_default(&_work->gz.store_name,  0);
            d_internal_knob_default(&_work->gz.store_mtime, 0);
            break;
        }
        case D_FORMAT_ID_SEVENZIP:
        {
            d_internal_knob_default(&_work->sevenzip.method,
                                    D_INTERNAL_ARCHIVE_SEVENZIP_METHOD);

            if (_work->store_only)
            {
                _work->sevenzip.method = D_SEVENZIP_METHOD_COPY;
            }

            d_internal_knob_default(&_work->sevenzip.solid,              1);
            d_internal_knob_default(&_work->sevenzip.header_compression, 1);
            d_internal_knob_default(&_work->sevenzip.header_encryption,  0);
            d_internal_knob_default(&_work->sevenzip.threads,            1);
            break;
        }
        case D_FORMAT_ID_RAR:
        {
            d_internal_knob_default(&_work->rar.level, _work->level);
            d_internal_knob_default(&_work->rar.solid, 0);
            d_internal_knob_default(&_work->rar.recovery_record, 0);
            break;
        }
    }

    return;
}

/*
d_archive_options_resolve
  Produce a fully specified container option set.

  As with compression, the defaults are pinned here rather than left to a
backend, so that C and C++ hand any backend byte-identical parameters and two
builds with different libraries produce the same container.  Only knobs
belonging to _format are resolved; the rest pass through untouched.

  The entry-compression defaults -- store_only, level, and the ZIP and 7z
methods -- come from cfg_archive.h's knobs (D_CFG_ARCHIVE_STORE_ONLY and its
kin), so a build chooses what an unset knob means; a knob the caller set is
never replaced.  store_only, once resolved, overrides the method of both ZIP
and 7z, so "store" means stored in either container.

  *** The VALUES below are provisional pending an archive chapter in the .tex
  *** corpus, exactly as the compression default table is.  What is settled is
  *** that they are decided in one place and that both faces read them.

Parameter(s):
  _format: the format the set is being resolved for.
  _in:     the set to resolve; may be null, which resolves as pristine.
  _out:    receives the resolved set; must not be null and may alias _in.
Return:
  D_PACK_STATUS_OK, or D_PACK_STATUS_INVALID_ARGUMENT.
*/
enum dj_pack_status
d_archive_options_resolve(
    enum dj_format_id                _format,
    const struct dj_archive_options* _in,
    struct dj_archive_options*       _out
)
{
    struct dj_archive_options work;
    enum dj_pack_status       status;

    if ( (_out == NULL) ||
         (!D_FORMAT_ID_IS_VALID(_format)) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    if (_in == NULL)
    {
        d_archive_options_init(&work);
    }
    else
    {
        work = *_in;
    }

    d_internal_knob_default(&work.level,      D_INTERNAL_ARCHIVE_LEVEL);
    d_internal_knob_default(&work.store_only, D_INTERNAL_ARCHIVE_STORE_ONLY);
    d_internal_knob_default(&work.preserve_permissions, 1);
    d_internal_knob_default(&work.preserve_mtime,       1);
    d_internal_resolve_blocks(_format, &work);

    // the embedded codec inherits the container's effort unless it was tuned
    // in its own right, so a caller who set only the container level gets the
    // effect they asked for all the way down.
    d_internal_knob_default(&work.codec.level, work.level);

    status = d_compress_options_resolve(d_format_default_codec(_format),
                                        &work.codec, &work.codec);

    if (status == D_PACK_STATUS_OK)
    {
        *_out = work;
    }

    return status;
}

/*
d_internal_zip_method_check
  Whether the ZIP method knob names a method the built-in writer writes:
unset, store, deflate, bzip2, zstd or xz.  LZMA is a ZIP method, but its raw
LZMA1 stream is not one the codec set makes, so D_PACK_STATUS_UNSUPPORTED;
any other value is not a method at all.

Parameter(s):
  _method:       the knob's value.
  _out_offender: receives D_ARCHIVE_KNOB_ZIP_METHOD on failure; may be null.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_UNSUPPORTED or
D_PACK_STATUS_INVALID_ARGUMENT.
*/
static enum dj_pack_status
d_internal_zip_method_check(
    int32_t               _method,
    enum dj_archive_knob* _out_offender
)
{
    enum dj_pack_status status = D_PACK_STATUS_INVALID_ARGUMENT;

    switch (_method)
    {
        case D_ARCHIVE_KNOB_UNSET:
        case D_ZIP_METHOD_STORE:
        case D_ZIP_METHOD_DEFLATE:
        case D_ZIP_METHOD_BZIP2:
        case D_ZIP_METHOD_ZSTD:
        case D_ZIP_METHOD_XZ:
        {
            return D_PACK_STATUS_OK;
        }
        case D_ZIP_METHOD_LZMA:
        {
            status = D_PACK_STATUS_UNSUPPORTED;
            break;
        }
        default:
        {
            break;
        }
    }

    if (_out_offender != NULL)
    {
        *_out_offender = D_ARCHIVE_KNOB_ZIP_METHOD;
    }

    return status;
}

/*
d_internal_tar_format_check
  Whether the tar format knob names a format the built-in writer writes:
unset, pax or ustar.  gnu and v7 are formats, but not ones this writer
emits, so D_PACK_STATUS_UNSUPPORTED; anything else is not a format at all.

Parameter(s):
  _format:       the knob's value.
  _out_offender: receives D_ARCHIVE_KNOB_TAR_FORMAT on failure; may be null.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_UNSUPPORTED or
D_PACK_STATUS_INVALID_ARGUMENT.
*/
static enum dj_pack_status
d_internal_tar_format_check(
    int32_t               _format,
    enum dj_archive_knob* _out_offender
)
{
    enum dj_pack_status status = D_PACK_STATUS_OK;

    if ( (_format == D_TAR_FORMAT_GNU) ||
         (_format == D_TAR_FORMAT_V7) )
    {
        status = D_PACK_STATUS_UNSUPPORTED;
    }
    else if ( (_format != D_ARCHIVE_KNOB_UNSET) &&
              (_format != D_TAR_FORMAT_USTAR) &&
              (_format != D_TAR_FORMAT_PAX) )
    {
        status = D_PACK_STATUS_INVALID_ARGUMENT;
    }
    if ( (status != D_PACK_STATUS_OK) &&
         (_out_offender != NULL) )
    {
        *_out_offender = D_ARCHIVE_KNOB_TAR_FORMAT;
    }

    return status;
}

/*
d_archive_options_validate
  Whether the knobs relevant to a format are coherent.

  Checks cross-knob CONSISTENCY, not just ranges: requesting encryption with no
passphrase, or a ZIP method this build cannot perform, are both sets that pass
a per-knob range check and still cannot be honoured.  A writer that discovered
either only at the point of emitting bytes would report a formal error as a
backend failure.

Parameter(s):
  _format:       the format whose rules apply.
  _options:      the set to check; may be null, which is trivially valid.
  _out_offender: receives the first offending knob; may be null.
Return:
  D_PACK_STATUS_OK, or a formal status naming the offender.
*/
enum dj_pack_status
d_archive_options_validate(
    enum dj_format_id                _format,
    const struct dj_archive_options* _options,
    enum dj_archive_knob*            _out_offender
)
{
    if (_options == NULL)
    {
        return D_PACK_STATUS_OK;
    }
    if (!D_FORMAT_ID_IS_VALID(_format))
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    if ( (_options->level != D_ARCHIVE_KNOB_UNSET) &&
         ( (_options->level < 0) ||
           (_options->level > 9) ) )
    {
        if (_out_offender != NULL)
        {
            *_out_offender = D_ARCHIVE_KNOB_LEVEL;
        }

        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    if (_format == D_FORMAT_ID_ZIP)
    {
        int32_t enc = _options->zip.encryption;

        if ( (enc != D_ARCHIVE_KNOB_UNSET)  &&
             (enc != D_ZIP_ENCRYPTION_NONE) &&
             d_pack_text_is_empty(_options->zip.password) )
        {
            if (_out_offender != NULL)
            {
                *_out_offender = D_ARCHIVE_KNOB_ZIP_PASSWORD;
            }

            return D_PACK_STATUS_INVALID_ARGUMENT;
        }
        if ( (enc != D_ARCHIVE_KNOB_UNSET) &&
             (enc != D_ZIP_ENCRYPTION_NONE) )
        {
            // the built-in writer does not encrypt.  UNSUPPORTED, not
            // INVALID_ARGUMENT: the request is well formed and this build
            // simply cannot honour it.
            if (_out_offender != NULL)
            {
                *_out_offender = D_ARCHIVE_KNOB_ZIP_ENCRYPTION;
            }

            return D_PACK_STATUS_UNSUPPORTED;
        }
    }

    if (_format == D_FORMAT_ID_ZIP)
    {
        return d_internal_zip_method_check(_options->zip.method,
                                           _out_offender);
    }
    if ( (_format == D_FORMAT_ID_TAR) ||
         (_format == D_FORMAT_ID_TAR_GZ) )
    {
        return d_internal_tar_format_check(_options->tar.format,
                                           _out_offender);
    }

    return D_PACK_STATUS_OK;
}


// =============================================================================
// V.   CAPABILITY AND ENTRY VALIDITY
// =============================================================================

/*
d_format_is_readable
  Whether this build can read a format.

  Answers from cfg_archive.h's D_INTERNAL_ARCHIVE_CAN_* -- env_archive.h's
roll-ups, ANDed with the format knobs -- rather than from its own knowledge of
which writers exist here.  That matters: the roll-ups already fold in
D_ENV_ARCHIVE_HAVE_BUILTIN_TAR / _ZIP, so they are 1 by default now that the
built-in writers are real -- and a project that sets those knobs to 0 is saying
"do not use the builtin", which this function must honour.  Reading the macro
also keeps the runtime answer and format_traits<>'s compile-time answer to one
source, instead of two that agree until they do not.

Parameter(s):
  _format: the format to query.
Return:
  1 when readable, 0 otherwise.
*/
int
d_format_is_readable(
    enum dj_format_id _format
)
{
    switch (_format)
    {
        case D_FORMAT_ID_ZIP:
        {
            return D_INTERNAL_ARCHIVE_CAN_READ_ZIP ? 1 : 0;
        }
        case D_FORMAT_ID_TAR:
        {
            return D_INTERNAL_ARCHIVE_CAN_READ_TAR ? 1 : 0;
        }
        case D_FORMAT_ID_GZ:
        {
            return D_INTERNAL_ARCHIVE_CAN_READ_GZ ? 1 : 0;
        }
        case D_FORMAT_ID_TAR_GZ:
        {
            return ( D_INTERNAL_ARCHIVE_CAN_READ_GZ &&
                     D_INTERNAL_ARCHIVE_CAN_READ_TAR ) ? 1 : 0;
        }
        case D_FORMAT_ID_SEVENZIP:
        {
            return D_INTERNAL_ARCHIVE_CAN_READ_7Z ? 1 : 0;
        }
        case D_FORMAT_ID_RAR:
        {
            return D_INTERNAL_ARCHIVE_CAN_READ_RAR ? 1 : 0;
        }
    }

    return 0;
}

/*
d_format_is_writable
  Whether this build can create a format.

  Kept separate from readability on purpose: RAR is readable by four backends
and writable by none of them, because no library can create it.  A single
"supported" predicate would make a writer report that formal fact as though the
format were unknown.

Parameter(s):
  _format: the format to query.
Return:
  1 when writable, 0 otherwise.
*/
int
d_format_is_writable(
    enum dj_format_id _format
)
{
    switch (_format)
    {
        case D_FORMAT_ID_ZIP:
        {
            return D_INTERNAL_ARCHIVE_CAN_WRITE_ZIP ? 1 : 0;
        }
        case D_FORMAT_ID_TAR:
        {
            return D_INTERNAL_ARCHIVE_CAN_WRITE_TAR ? 1 : 0;
        }
        case D_FORMAT_ID_GZ:
        {
            return D_INTERNAL_ARCHIVE_CAN_WRITE_GZ ? 1 : 0;
        }
        case D_FORMAT_ID_TAR_GZ:
        {
            return D_INTERNAL_ARCHIVE_CAN_WRITE_TGZ ? 1 : 0;
        }
        case D_FORMAT_ID_SEVENZIP:
        {
            return D_INTERNAL_ARCHIVE_CAN_WRITE_7Z ? 1 : 0;
        }
        case D_FORMAT_ID_RAR:
        {
            return D_INTERNAL_ARCHIVE_CAN_WRITE_RAR ? 1 : 0;
        }
    }

    return 0;
}

/*
d_format_supports_codec
  Whether a format can carry members encoded with a codec.

Parameter(s):
  _format: the container format.
  _codec:  the codec in question.
Return:
  1 when the pairing is legal AND this build can perform the codec.
*/
int
d_format_supports_codec(
    enum dj_format_id _format,
    enum dj_codec_id  _codec
)
{
    if ( (!D_FORMAT_ID_IS_VALID(_format)) ||
         (!D_CODEC_ID_IS_VALID(_codec)) )
    {
        return 0;
    }
    if (!d_codec_can_compress(_codec))
    {
        return 0;
    }

    switch (_format)
    {
        case D_FORMAT_ID_ZIP:
        {
            return ( (_codec == D_CODEC_ID_STORE)   ||
                     (_codec == D_CODEC_ID_DEFLATE) ||
                     (_codec == D_CODEC_ID_BZIP2)   ||
                     (_codec == D_CODEC_ID_XZ)      ||
                     (_codec == D_CODEC_ID_ZSTD) ) ? 1 : 0;
        }
        case D_FORMAT_ID_TAR:
        {
            // tar does not compress its members; the codec is the outer layer.
            return (_codec == D_CODEC_ID_STORE) ? 1 : 0;
        }
        case D_FORMAT_ID_GZ:
        case D_FORMAT_ID_TAR_GZ:
        {
            return (_codec == D_CODEC_ID_GZIP) ? 1 : 0;
        }
        case D_FORMAT_ID_SEVENZIP:
        {
            return ( (_codec == D_CODEC_ID_STORE)   ||
                     (_codec == D_CODEC_ID_XZ)      ||
                     (_codec == D_CODEC_ID_BZIP2)   ||
                     (_codec == D_CODEC_ID_DEFLATE) ||
                     (_codec == D_CODEC_ID_ZSTD) ) ? 1 : 0;
        }
        case D_FORMAT_ID_RAR:
        {
            return 0;
        }
    }

    return 0;
}

/*
d_format_supports_encryption
  Whether a format defines member or header encryption at all.

  A format-level fact, independent of whether this build can perform it -- the
built-in writer encrypts nothing, and d_archive_options_validate is what
reports that.

Parameter(s):
  _format: the format to query.
Return:
  1 when the format defines encryption, 0 otherwise.
*/
int
d_format_supports_encryption(
    enum dj_format_id _format
)
{
    return ( (_format == D_FORMAT_ID_ZIP)      ||
             (_format == D_FORMAT_ID_SEVENZIP) ||
             (_format == D_FORMAT_ID_RAR) ) ? 1 : 0;
}

/*
d_format_backend_id
  Which backend serves a format on this build.

Parameter(s):
  _format: the format to query.
Return:
  A D_ENV_ARCHIVE_BACKEND_* constant.  tar and zip report BACKEND_BUILTIN when
no library was detected, which is now a true statement rather than an
aspiration.
*/
int
d_format_backend_id(
    enum dj_format_id _format
)
{
    switch (_format)
    {
        // D_CFG_ARCHIVE_PREFER_BUILTIN: the built-in writers even where a
        // library was detected, which is what this kernel writes with
        case D_FORMAT_ID_ZIP:
        {
            return ( (D_INTERNAL_ARCHIVE_PREFER_BUILTIN) &&
                     (D_ENV_ARCHIVE_HAVE_BUILTIN_ZIP) )
                 ? D_ENV_ARCHIVE_BACKEND_BUILTIN
                 : D_ENV_ARCHIVE_PREFERRED_ZIP;
        }
        case D_FORMAT_ID_TAR:
        case D_FORMAT_ID_TAR_GZ:
        {
            return ( (D_INTERNAL_ARCHIVE_PREFER_BUILTIN) &&
                     (D_ENV_ARCHIVE_HAVE_BUILTIN_TAR) )
                 ? D_ENV_ARCHIVE_BACKEND_BUILTIN
                 : D_ENV_ARCHIVE_PREFERRED_TAR;
        }
        case D_FORMAT_ID_GZ:       { return D_ENV_ARCHIVE_BACKEND_BUILTIN; }
        case D_FORMAT_ID_SEVENZIP: { return D_ENV_ARCHIVE_PREFERRED_7Z; }
        case D_FORMAT_ID_RAR:      { return D_ENV_ARCHIVE_PREFERRED_RAR_WRITE; }
    }

    return D_ENV_ARCHIVE_BACKEND_NONE;
}

/*
d_format_backend_name
  The name of the backend serving a format.

Parameter(s):
  _format: the format to query.
Return:
  A static, NUL-terminated backend name.
*/
const char*
d_format_backend_name(
    enum dj_format_id _format
)
{
    return d_env_archive_backend_name(d_format_backend_id(_format));
}

/*
d_format_writable_list
  The formats this build can create, in enumeration order.

Parameter(s):
  _out_formats:  receives the formats; may be null to count only.
  _out_capacity: how many entries _out_formats holds.
Return:
  The total number writable, which may exceed _out_capacity.
*/
size_t
d_format_writable_list(
    enum dj_format_id* _out_formats,
    size_t            _out_capacity
)
{
    size_t found = 0;
    int    i;

    for (i = 0; i < D_FORMAT_ID_COUNT; ++i)
    {
        if (!d_format_is_writable((enum dj_format_id)i))
        {
            continue;
        }
        if ( (_out_formats != NULL) &&
             (found < _out_capacity) )
        {
            _out_formats[found] = (enum dj_format_id)i;
        }

        ++found;
    }

    return found;
}

/*
d_archive_entry_name_is_valid
  Whether an entry name is safe to write and safe to extract.

  Three rules, and all three are security properties rather than style ones.  A
name may not be empty, may not be absolute, and may not contain a `..`
component -- the last being the path-traversal hole that lets a crafted archive
write outside the directory it is extracted into.  Backslashes are treated as
separators too, because a name written on Windows and extracted on POSIX would
otherwise smuggle `..\` past a check that only looked for `../`.

  This lives in the vocabulary rather than in a writer so that a caller can
sanitise before building an entry list and get exactly the answer the writer
will give.

Parameter(s):
  _name: the name to check.
Return:
  1 when the name is acceptable, 0 otherwise.
*/
int d_archive_entry_name_is_valid(
    struct dj_pack_text _name
)
{
    size_t i;
    size_t seg_start = 0;

    if ( (_name.data == NULL) ||
         (_name.length == 0u) )
    {
        return 0;
    }
    if ( (_name.data[0] == '/') ||
         (_name.data[0] == '\\') )
    {
        return 0;
    }
    // a Windows drive prefix ("C:...") is absolute too
    if ( (_name.length >= 2u) &&
         (_name.data[1] == ':') )
    {
        return 0;
    }

    for (i = 0; i <= _name.length; ++i)
    {
        int at_end = (i == _name.length);

        if ( at_end ||
             (_name.data[i] == '/') ||
             (_name.data[i] == '\\') )
        {
            size_t seg_len = i - seg_start;

            if ( (seg_len == 2u) &&
                 (_name.data[seg_start] == '.') &&
                 (_name.data[seg_start + 1u] == '.') )
            {
                return 0;
            }

            seg_start = i + 1u;
        }
    }

    return 1;
}

/*
d_archive_entry_is_valid
  Whether an entry is internally coherent.

Parameter(s):
  _entry: the entry to check; may be null, which is not valid.
Return:
  1 when the name passes and a directory carries no payload, 0 otherwise.
*/
int d_archive_entry_is_valid(
    const struct dj_archive_entry* _entry
)
{
    if (_entry == NULL)
    {
        return 0;
    }
#if D_INTERNAL_ARCHIVE_CHECK_ENTRY_NAMES
    // D_CFG_ARCHIVE_CHECK_ENTRY_NAMES: no empty, absolute or `..` name
    if (!d_archive_entry_name_is_valid(_entry->name))
    {
        return 0;
    }
#else
    // the traversal guard is off; a name must still be there
    if ( (_entry->name.data == NULL) ||
         (_entry->name.length == 0u) )
    {
        return 0;
    }
#endif
    if ( (_entry->is_directory) &&
         (_entry->data.size != 0u) )
    {
        return 0;
    }
    if ( (_entry->data.data == NULL) &&
         (_entry->data.size != 0u) )
    {
        return 0;
    }

    return 1;
}


// =============================================================================
// VI.  SIGNATURES
// =============================================================================

/*
d_format_signature_length
  How many leading bytes are needed to recognise a container.

  tar is the awkward one: its `ustar` magic sits at offset 257, not 0, so
recognising a tar needs a whole 512-byte record rather than the four or six
bytes every other format needs.  Reporting a nominal small number here would
make a caller pass too little data and get a false negative.

Parameter(s):
  _format: the format to query.
Return:
  The byte count needed.
*/
int
d_format_signature_length(
    enum dj_format_id _format
)
{
    switch (_format)
    {
        case D_FORMAT_ID_ZIP:      { return 4; }
        case D_FORMAT_ID_TAR:      { return (int)D_INTERNAL_TAR_BLOCK; }
        case D_FORMAT_ID_GZ:
        case D_FORMAT_ID_TAR_GZ:   { return 3; }
        case D_FORMAT_ID_SEVENZIP: { return 6; }
        case D_FORMAT_ID_RAR:      { return 7; }
    }

    return 0;
}

/*
d_format_signature_matches
  Whether a buffer opens with a container's signature.

Parameter(s):
  _format: the container to test for.
  _data:   the buffer; may be null, which never matches.
  _size:   the buffer's length.
Return:
  1 when the buffer opens with that container, 0 otherwise.
*/
int
d_format_signature_matches(
    enum dj_format_id _format,
    const void*      _data,
    size_t           _size
)
{
    const unsigned char* p   = (const unsigned char*)_data;
    int                  len = d_format_signature_length(_format);

    if ( (p == NULL) ||
         (len == 0)  ||
         (_size < (size_t)len) )
    {
        return 0;
    }

    switch (_format)
    {
        case D_FORMAT_ID_ZIP:
        {
            // "PK\003\004" is a local header; "PK\005\006" is an empty
            // archive, which has no local header at all and would otherwise
            // fail to be recognised as a ZIP.
            return ( (p[0] == 'P') && (p[1] == 'K') &&
                     ( ((p[2] == 0x03u) && (p[3] == 0x04u)) ||
                       ((p[2] == 0x05u) && (p[3] == 0x06u)) ) ) ? 1 : 0;
        }
        case D_FORMAT_ID_TAR:
        {
            return (memcmp(p + 257, "ustar", 5) == 0) ? 1 : 0;
        }
        case D_FORMAT_ID_GZ:
        case D_FORMAT_ID_TAR_GZ:
        {
            return d_codec_signature_matches(D_CODEC_ID_GZIP, _data, _size);
        }
        case D_FORMAT_ID_SEVENZIP:
        {
            return ( (p[0] == '7')   && (p[1] == 'z')   &&
                     (p[2] == 0xBCu) && (p[3] == 0xAFu) &&
                     (p[4] == 0x27u) && (p[5] == 0x1Cu) ) ? 1 : 0;
        }
        case D_FORMAT_ID_RAR:
        {
            return ( (p[0] == 'R') && (p[1] == 'a') && (p[2] == 'r') &&
                     (p[3] == '!') && (p[4] == 0x1Au) &&
                     (p[5] == 0x07u) ) ? 1 : 0;
        }
    }

    return 0;
}

/*
d_format_detect
  Identify a container from its opening bytes.

  gz is tested before tar_gz and both after the unambiguous magics.  A gzip
stream that turns out to contain a tar cannot be told apart from one that does
not without decompressing it, so this reports GZ and a caller that cares
decompresses and asks again -- which is honest, where guessing would not be.

Parameter(s):
  _data:       the buffer to inspect.
  _size:       its length.
  _out_format: receives the format on success; untouched on failure.
Return:
  1 when a container was recognised, 0 otherwise.
*/
int
d_format_detect(
    const void*       _data,
    size_t            _size,
    enum dj_format_id* _out_format
)
{
    static const enum dj_format_id order[] =
    {
        D_FORMAT_ID_SEVENZIP, D_FORMAT_ID_RAR, D_FORMAT_ID_ZIP,
        D_FORMAT_ID_TAR,      D_FORMAT_ID_GZ
    };

    size_t n = sizeof(order) / sizeof(order[0]);
    size_t i;

    if ( (_data == NULL) ||
         (_out_format == NULL) )
    {
        return 0;
    }

    for (i = 0; i < n; ++i)
    {
        if (d_format_signature_matches(order[i], _data, _size))
        {
            *_out_format = order[i];

            return 1;
        }
    }

    return 0;
}


// =============================================================================
// VII. BYTE-ORDER PRIMITIVES
// =============================================================================
//   ZIP is little-endian on the wire regardless of the host, and tar writes its
// numbers as ASCII octal.  Both are done by explicit byte arithmetic rather
// than by casting a struct over the buffer: a packed-struct overlay would
// depend on the host's endianness and on a non-standard packing pragma, which
// is two portability holes to save nothing.

/*
d_internal_put_le16
  Write a 16-bit value little-endian.

Parameter(s):
  _dst:   the destination, at least 2 bytes.
  _value: the value.
Return:
  none.
*/
static
void d_internal_put_le16(
    unsigned char* _dst,
    uint32_t       _value
)
{
    _dst[0] = (unsigned char)( _value        & 0xFFu);
    _dst[1] = (unsigned char)((_value >> 8)  & 0xFFu);

    return;
}

/*
d_internal_put_le32
  Write a 32-bit value little-endian.

Parameter(s):
  _dst:   the destination, at least 4 bytes.
  _value: the value.
Return:
  none.
*/
static
void d_internal_put_le32(
    unsigned char* _dst,
    uint32_t       _value
)
{
    _dst[0] = (unsigned char)( _value        & 0xFFu);
    _dst[1] = (unsigned char)((_value >> 8)  & 0xFFu);
    _dst[2] = (unsigned char)((_value >> 16) & 0xFFu);
    _dst[3] = (unsigned char)((_value >> 24) & 0xFFu);

    return;
}

/*
d_internal_get_le16
  Read a 16-bit little-endian value.

Parameter(s):
  _src: the source, at least 2 bytes.
Return:
  The value.
*/
static
uint32_t d_internal_get_le16(
    const unsigned char* _src
)
{
    return ((uint32_t)_src[0]) | (((uint32_t)_src[1]) << 8);
}

/*
d_internal_get_le32
  Read a 32-bit little-endian value.

Parameter(s):
  _src: the source, at least 4 bytes.
Return:
  The value.
*/
static
uint32_t d_internal_get_le32(
    const unsigned char* _src
)
{
    return ((uint32_t)_src[0])          |
           (((uint32_t)_src[1]) << 8)   |
           (((uint32_t)_src[2]) << 16)  |
           (((uint32_t)_src[3]) << 24);
}

/*
d_internal_put_octal
  Write a value as zero-padded ASCII octal with a trailing NUL, which is how
every numeric field of a ustar header is encoded.

Parameter(s):
  _dst:   the field.
  _width: the field's total width, terminator included.
  _value: the value.
Return:
  none.
*/
static
void d_internal_put_octal(
    unsigned char* _dst,
    size_t         _width,
    uint64_t       _value
)
{
    size_t i = _width - 1u;

    _dst[i] = (unsigned char)'\0';

    while (i > 0u)
    {
        --i;
        _dst[i] = (unsigned char)('0' + (unsigned char)(_value & 7u));
        _value >>= 3;
    }

    return;
}

/*
d_internal_get_octal
  Read a ustar numeric field.  Leading spaces and NULs are skipped and parsing
stops at the first byte that is not an octal digit, which is what real tars
require: the padding conventions vary between implementations.

Parameter(s):
  _src:   the field.
  _width: the field's width.
Return:
  The value.
*/
static
uint64_t d_internal_get_octal(
    const unsigned char* _src,
    size_t               _width
)
{
    uint64_t v = 0u;
    size_t   i = 0;

    while ( (i < _width) &&
            ( (_src[i] == ' ') ||
              (_src[i] == '\0') ) )
    {
        ++i;
    }

    while ( (i < _width) &&
            (_src[i] >= '0') &&
            (_src[i] <= '7') )
    {
        v = (v << 3) | (uint64_t)(_src[i] - '0');
        ++i;
    }

    return v;
}

/*
d_internal_dos_datetime
  Convert a Unix timestamp to the packed DOS date and time a ZIP record uses.

  The ZIP format's epoch is 1980 and its resolution is two seconds, so a
timestamp before 1980 cannot be represented and is clamped rather than
wrapped -- wrapping would put a 1970 file in 2050-something and no reader would
flag it.

  The calendar arithmetic is the standard days-to-civil algorithm, done here
rather than through gmtime because gmtime consults the C library's timezone
state and is not required to be thread-safe.  A container's contents must not
depend on either.

Parameter(s):
  _unix:     seconds since the Unix epoch.
  _out_date: receives the packed date.
  _out_time: receives the packed time.
Return:
  none.
*/
static
void d_internal_dos_datetime(
    int64_t   _unix,
    uint32_t* _out_date,
    uint32_t* _out_time
)
{
    int64_t  days;
    int64_t  secs;
    int64_t  era;
    int64_t  doe;
    int64_t  yoe;
    int64_t  y;
    int64_t  doy;
    int64_t  mp;
    int64_t  d;
    int64_t  m;

    if (_unix < D_ARCHIVE_ZIP_EPOCH)
    {
        _unix = D_ARCHIVE_ZIP_EPOCH;
    }

    days = _unix / 86400;
    secs = _unix % 86400;

    // days-to-civil, shifted so the era begins on 0000-03-01
    days += 719468;
    era   = ((days >= 0) ? days : (days - 146096)) / 146097;
    doe   = days - (era * 146097);
    yoe   = (doe - (doe / 1460) + (doe / 36524) - (doe / 146096)) / 365;
    y     = yoe + (era * 400);
    doy   = doe - ((365 * yoe) + (yoe / 4) - (yoe / 100));
    mp    = ((5 * doy) + 2) / 153;
    d     = doy - (((153 * mp) + 2) / 5) + 1;
    m     = (mp < 10) ? (mp + 3) : (mp - 9);
    y    += (m <= 2) ? 1 : 0;

    *_out_date = (uint32_t)(((y - 1980) << 9) | (m << 5) | d);
    *_out_time = (uint32_t)(((secs / 3600) << 11)          |
                            (((secs / 60) % 60) << 5)      |
                            ((secs % 60) / 2));

    return;
}

/*
d_internal_entry_mtime
  The timestamp to record for an entry, honouring the preserve_mtime knob.

Parameter(s):
  _entry:   the entry.
  _options: a resolved option set.
Return:
  The timestamp.  When the entry carries none or the caller asked not to
preserve it: D_ARCHIVE_EPOCH_DEFAULT under D_CFG_ARCHIVE_REPRODUCIBLE, the
default, which is what keeps two runs byte-identical; the current time when a
build turns it off.
*/
static
int64_t d_internal_entry_mtime(
    const struct dj_archive_entry*   _entry,
    const struct dj_archive_options* _options
)
{
    if ( ( (_options != NULL) &&
           (_options->preserve_mtime == 0) ) ||
         (_entry == NULL) ||
         (_entry->mtime == 0) )
    {
#if D_INTERNAL_ARCHIVE_USE_CLOCK
        time_t now = time(NULL);

        if (now != (time_t)-1)
        {
            return (int64_t)now;
        }
#endif
        return (int64_t)D_ARCHIVE_EPOCH_DEFAULT;
    }

    return _entry->mtime;
}

/*
d_internal_entry_mode
  The permission bits to record for an entry, honouring
preserve_permissions.

Parameter(s):
  _entry:   the entry.
  _options: a resolved option set.
Return:
  The mode.  A pinned default when the entry carries none or when the caller
asked not to preserve permissions -- never the process umask.
*/
static
uint32_t d_internal_entry_mode(
    const struct dj_archive_entry*   _entry,
    const struct dj_archive_options* _options
)
{
    int is_dir = ((_entry != NULL) && _entry->is_directory);

    if ( (_options != NULL) &&
         (_options->preserve_permissions == 0) )
    {
        return is_dir
             ? (uint32_t)D_ARCHIVE_MODE_DIR_DEFAULT
             : (uint32_t)D_ARCHIVE_MODE_FILE_DEFAULT;
    }
    if ( (_entry == NULL) ||
         (_entry->mode == 0u) )
    {
        return is_dir
             ? (uint32_t)D_ARCHIVE_MODE_DIR_DEFAULT
             : (uint32_t)D_ARCHIVE_MODE_FILE_DEFAULT;
    }

    return _entry->mode;
}


// =============================================================================
// VIII. THE BUILT-IN USTAR WRITER / READER
// =============================================================================
//   POSIX ustar, written from the specification with no dependency.  Every
// structure is a multiple of 512 bytes; numbers are ASCII octal; the archive
// ends with two zero records.
//
//   The 100-byte name field is the format's one real limit.  ustar extends it
// with a 155-byte `prefix` that is logically joined with a '/', so a path up to
// 255 bytes is representable provided it can be SPLIT at a separator.  What
// ustar cannot hold -- a name that does not split, a size of 8 GiB, a
// timestamp before 1970 -- goes in a pax extended header before the member,
// under the default format (pax); strict ustar refuses it.  A member that fits
// gets no extended header, so a pax archive of short names is a ustar archive.
//
//   The reader is a push parser: it is fed bytes in pieces of any size and
// keeps its place as a state, which is what lets a tar.gz be read through
// the inflater with nothing staged.  It reads pax extended headers (path,
// size, mtime), skips pax global headers, reads GNU long names, and checks
// each header's checksum.

// d_internal_tar_field
//   constant: the byte offsets of each ustar header field, so the writer and
// the reader address them through one set of names instead of two sets of
// literals that can drift apart.
#define D_INTERNAL_TAR_OFF_NAME       0u
#define D_INTERNAL_TAR_OFF_MODE     100u
#define D_INTERNAL_TAR_OFF_UID      108u
#define D_INTERNAL_TAR_OFF_GID      116u
#define D_INTERNAL_TAR_OFF_SIZE     124u
#define D_INTERNAL_TAR_OFF_MTIME    136u
#define D_INTERNAL_TAR_OFF_CHKSUM   148u
#define D_INTERNAL_TAR_OFF_TYPE     156u
#define D_INTERNAL_TAR_OFF_MAGIC    257u
#define D_INTERNAL_TAR_OFF_VERSION  263u
#define D_INTERNAL_TAR_OFF_UNAME    265u
#define D_INTERNAL_TAR_OFF_GNAME    297u
#define D_INTERNAL_TAR_OFF_DEVMAJ   329u
#define D_INTERNAL_TAR_OFF_DEVMIN   337u
#define D_INTERNAL_TAR_OFF_PREFIX   345u

// D_INTERNAL_TAR_OCTAL_MAX
//   constant: the largest value an 11-digit ustar numeric field holds: a size
// of 8 GiB - 1, a timestamp in the year 2242.  Past it, a pax record carries
// the value.
#define D_INTERNAL_TAR_OCTAL_MAX    ((uint64_t)077777777777u)

/*
d_internal_emit
  Push bytes to a sink, mapping a short write onto the sink-error status.

Parameter(s):
  _sink: the destination.
  _data: the bytes.
  _size: how many.
Return:
  D_PACK_STATUS_OK, or D_PACK_STATUS_SINK_ERROR.
*/
static enum dj_pack_status
d_internal_emit(
    struct dj_pack_sink _sink,
    const void*        _data,
    size_t             _size
)
{
    return d_sink_emit(_sink, _data, _size)
         ? D_PACK_STATUS_OK
         : D_PACK_STATUS_SINK_ERROR;
}

/*
d_internal_emit_zeros
  Push _size zero bytes to a sink, in chunks from a small stack buffer so that
padding a large record needs no allocation and no large stack frame.

Parameter(s):
  _sink: the destination.
  _size: how many zero bytes.
Return:
  D_PACK_STATUS_OK, or D_PACK_STATUS_SINK_ERROR.
*/
static enum dj_pack_status
d_internal_emit_zeros(
    struct dj_pack_sink _sink,
    size_t             _size
)
{
    unsigned char zeros[D_INTERNAL_TAR_BLOCK];
    size_t        left = _size;

    memset(zeros, 0, sizeof(zeros));

    while (left > 0u)
    {
        size_t             take = (left < sizeof(zeros)) ? left : sizeof(zeros);
        enum dj_pack_status s    = d_internal_emit(_sink, zeros, take);

        if (s != D_PACK_STATUS_OK)
        {
            return s;
        }

        left -= take;
    }

    return D_PACK_STATUS_OK;
}

/*
d_internal_count_write
  A sink write that discards its bytes and counts them, for a sizing pass.

Parameter(s):
  _context: a size_t total.
  _data:    ignored.
  _size:    how many bytes to add.
Return:
  _size always.
*/
static
size_t d_internal_count_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    (void)_data;

    if (_context != NULL)
    {
        *((size_t*)_context) += _size;
    }

    return _size;
}

/*
d_internal_field_len
  The length of a fixed-width character field.

  A ustar field is NUL-padded but NOT NUL-terminated when the value exactly
fills it: a 100-byte name occupies all 100 bytes with no terminator anywhere.
strlen would therefore run off the end of the field and into the next one, and
the bounded library call that would be correct is POSIX rather than C99, so it
sits below the framework's portability floor.  This is the bounded scan both
facts require.

Parameter(s):
  _field: the field's first byte.
  _width: the field's width.
Return:
  The number of bytes before the first NUL, or _width when there is none.
*/
static
size_t d_internal_field_len(
    const unsigned char* _field,
    size_t               _width
)
{
    size_t i = 0;

    while ( (i < _width) &&
            (_field[i] != (unsigned char)'\0') )
    {
        ++i;
    }

    return i;
}

/*
d_internal_tar_split_name
  Split an entry name across ustar's `name` and `prefix` fields.

  The split must fall on a '/' -- the reader rejoins the two with one -- so a
name that fits in neither field alone and has no separator in the right window
cannot be represented in the header.  The caller decides what that means: a
pax record under D_TAR_FORMAT_PAX, D_PACK_STATUS_UNSUPPORTED under strict
ustar.  Never truncation: a truncated path produces an archive that extracts
to the wrong place and looks correct doing it.

Parameter(s):
  _name:           the entry name.
  _out_name_off:   receives the offset of the `name` portion.
  _out_name_len:   receives its length.
  _out_prefix_len: receives the `prefix` length, 0 when unused.
Return:
  1 when the name is representable in ustar, 0 otherwise.
*/
static
int d_internal_tar_split_name(
    struct dj_pack_text _name,
    size_t*             _out_name_off,
    size_t*             _out_name_len,
    size_t*             _out_prefix_len
)
{
    size_t split;

    if (_name.length <= 100u)
    {
        *_out_name_off   = 0u;
        *_out_name_len   = _name.length;
        *_out_prefix_len = 0u;

        return 1;
    }
    if (_name.length > 255u)
    {
        return 0;
    }

    // walk backwards for the last '/' that leaves <= 100 bytes after it and
    // <= 155 before it
    split = _name.length;

    while (split > 0u)
    {
        --split;

        if (_name.data[split] != '/')
        {
            continue;
        }
        if ( ((_name.length - split - 1u) <= 100u) &&
             (split <= 155u) )
        {
            *_out_prefix_len = split;
            *_out_name_off   = split + 1u;
            *_out_name_len   = _name.length - split - 1u;

            return 1;
        }
    }

    return 0;
}

/*
d_internal_tar_tail
  The tail of a name that fits in _width bytes, starting at a component when
one starts inside the window.

  Used where a pax record carries the real name and the ustar header holds
only a stand-in: the name a pax-unaware reader would extract, and the name of
the extended header itself.  A trailing '/' is not taken as a component
start, so a directory's stand-in is never empty.

Parameter(s):
  _name:    the full name.
  _width:   the most bytes the stand-in may take.
  _out_off: receives where the stand-in starts in _name.
Return:
  The stand-in's length.
*/
static
size_t d_internal_tar_tail(
    struct dj_pack_text _name,
    size_t              _width,
    size_t*             _out_off
)
{
    size_t start = (_name.length > _width) ? (_name.length - _width) : 0u;
    size_t i;

    for (i = start; (i + 1u) < _name.length; ++i)
    {
        if ( (_name.data[i] == '/') &&
             ((i + 2u) < _name.length) )
        {
            start = i + 1u;

            break;
        }
    }

    *_out_off = start;

    return _name.length - start;
}

/*
d_internal_decimal
  Write a value in decimal, most significant digit first, with no terminator.

Parameter(s):
  _dst:   at least 20 bytes.
  _value: the value.
Return:
  The number of digits written.
*/
static
size_t d_internal_decimal(
    char*    _dst,
    uint64_t _value
)
{
    char   rev[20];
    size_t n = 0;
    size_t i;

    do
    {
        rev[n++] = (char)('0' + (int)(_value % 10u));
        _value  /= 10u;
    } while (_value != 0u);

    for (i = 0; i < n; ++i)
    {
        _dst[i] = rev[n - 1u - i];
    }

    return n;
}

/*
d_internal_signed_decimal
  Write a signed value in decimal, with a leading '-' when negative.

Parameter(s):
  _dst:   at least 21 bytes.
  _value: the value.
Return:
  The number of characters written.
*/
static
size_t d_internal_signed_decimal(
    char*   _dst,
    int64_t _value
)
{
    if (_value < 0)
    {
        _dst[0] = '-';

        // negate in unsigned arithmetic, which is defined for INT64_MIN
        return 1u + d_internal_decimal(_dst + 1,
                                       (uint64_t)0 - (uint64_t)_value);
    }

    return d_internal_decimal(_dst, (uint64_t)_value);
}

/*
d_internal_pax_record_length
  The length of the pax record "LEN KEY=VALUE\n", whose LEN counts its own
digits.

  Adding the digits can carry the total into one more digit (a 98-byte body
needs 101 bytes, not 100), so the count is taken twice: once over the body,
once over the total that produced.

Parameter(s):
  _key_len:   the keyword's length.
  _value_len: the value's length.
Return:
  The record's length.
*/
static
size_t d_internal_pax_record_length(
    size_t _key_len,
    size_t _value_len
)
{
    char   digits[20];
    size_t body  = _key_len + _value_len + 3u;     // ' ', '=' and '\n'
    size_t width = d_internal_decimal(digits, (uint64_t)body);
    size_t total = body + width;

    if (d_internal_decimal(digits, (uint64_t)total) != width)
    {
        total = body + d_internal_decimal(digits, (uint64_t)total);
    }

    return total;
}

// dj_internal_tar_plan
//   struct: what the writer decides about one member before writing it --
// where its name sits in the ustar header, and which values the header cannot
// hold and a pax record carries instead.
struct dj_internal_tar_plan
{
    size_t   name_off;
    size_t   name_len;
    size_t   prefix_len;
    int      pax_path;
    int      pax_size;
    int      pax_mtime;
    int64_t  mtime;
    uint32_t mode;
    size_t   pax_len;
};

/*
d_internal_tar_plan_member
  Decide how one member is written.

  A value the ustar header cannot hold -- a name that does not split, a size
of 8 GiB or more, a timestamp before 1970 or after 2242 -- goes in a pax
record when the format is pax, the default.  Strict ustar refuses the member
with D_PACK_STATUS_UNSUPPORTED instead.  A member that needs no record is
written exactly as strict ustar writes it, so a pax archive with short names
is byte for byte a ustar archive.

Parameter(s):
  _entry:   the member.
  _options: a resolved option set.
  _plan:    receives the decisions.
Return:
  D_PACK_STATUS_OK, or D_PACK_STATUS_UNSUPPORTED.
*/
static enum dj_pack_status
d_internal_tar_plan_member(
    const struct dj_archive_entry*   _entry,
    const struct dj_archive_options* _options,
    struct dj_internal_tar_plan*     _plan
)
{
    char digits[21];

    _plan->mode       = d_internal_entry_mode(_entry, _options) & 07777u;
    _plan->mtime      = d_internal_entry_mtime(_entry, _options);
    _plan->prefix_len = 0u;
    _plan->pax_len    = 0u;
    _plan->pax_path   = !d_internal_tar_split_name(_entry->name,
                                                   &_plan->name_off,
                                                   &_plan->name_len,
                                                   &_plan->prefix_len);
    _plan->pax_size   = ((uint64_t)_entry->data.size >
                         D_INTERNAL_TAR_OCTAL_MAX);
    _plan->pax_mtime  = ( (_plan->mtime < 0) ||
                          ((uint64_t)_plan->mtime >
                           D_INTERNAL_TAR_OCTAL_MAX) );

    if (_plan->pax_path)
    {
        _plan->prefix_len = 0u;
        _plan->name_len   = d_internal_tar_tail(_entry->name, 100u,
                                                &_plan->name_off);
        _plan->pax_len   += d_internal_pax_record_length(
                                4u, _entry->name.length);
    }
    if (_plan->pax_size)
    {
        _plan->pax_len += d_internal_pax_record_length(
            4u, d_internal_decimal(digits, (uint64_t)_entry->data.size));
    }
    if (_plan->pax_mtime)
    {
        _plan->pax_len += d_internal_pax_record_length(
            5u, d_internal_signed_decimal(digits, _plan->mtime));
    }

    if ( (_plan->pax_len != 0u) &&
         (_options->tar.format != D_TAR_FORMAT_PAX) )
    {
        return D_PACK_STATUS_UNSUPPORTED;
    }

    return D_PACK_STATUS_OK;
}

/*
d_internal_tar_finish_header
  Fill every field of a ustar header but the name and prefix, which the
caller has written, and then the checksum.

  The checksum is computed with its own field held at eight spaces, which is
the convention the format mandates and the single most commonly mis-implemented
detail in a hand-written tar writer.  Owner NAMES are left empty: they are
the one field a tar writer normally fills from the host, and doing so makes
the same entry produce different bytes on different machines.

Parameter(s):
  _block: the 512-byte record, zeroed, with its name fields written.
  _mode:  the permission bits.
  _size:  the size field's value; 0 when a pax record carries the size.
  _mtime: the timestamp, clamped to what the field holds.
  _type:  the typeflag.
Return:
  none.
*/
static
void d_internal_tar_finish_header(
    unsigned char* _block,
    uint32_t       _mode,
    uint64_t       _size,
    int64_t        _mtime,
    char           _type
)
{
    uint64_t when = 0u;
    uint32_t sum  = 0u;
    size_t   i;

    if (_mtime > 0)
    {
        when = ((uint64_t)_mtime > D_INTERNAL_TAR_OCTAL_MAX)
             ? D_INTERNAL_TAR_OCTAL_MAX
             : (uint64_t)_mtime;
    }

    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_MODE,   8u,
                         (uint64_t)_mode);
    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_UID,    8u, 0u);
    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_GID,    8u, 0u);
    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_SIZE,  12u, _size);
    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_MTIME, 12u, when);

    _block[D_INTERNAL_TAR_OFF_TYPE] = (unsigned char)_type;

    memcpy(_block + D_INTERNAL_TAR_OFF_MAGIC,   "ustar", 5);
    memcpy(_block + D_INTERNAL_TAR_OFF_VERSION, "00",    2);

    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_DEVMAJ, 8u, 0u);
    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_DEVMIN, 8u, 0u);

    memset(_block + D_INTERNAL_TAR_OFF_CHKSUM, ' ', 8);

    for (i = 0; i < D_INTERNAL_TAR_BLOCK; ++i)
    {
        sum += (uint32_t)_block[i];
    }

    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_CHKSUM, 7u,
                         (uint64_t)sum);
    _block[D_INTERNAL_TAR_OFF_CHKSUM + 7u] = (unsigned char)' ';

    return;
}

/*
d_internal_pax_emit_record
  Write one pax record, "LEN KEY=VALUE\n".

Parameter(s):
  _sink:      the destination.
  _key:       the keyword.
  _value:     the value's bytes.
  _value_len: how many.
Return:
  D_PACK_STATUS_OK, or the sink's failure.
*/
static enum dj_pack_status
d_internal_pax_emit_record(
    struct dj_pack_sink _sink,
    const char*         _key,
    const char*         _value,
    size_t              _value_len
)
{
    char               head[48];
    size_t             key_len = strlen(_key);
    size_t             n;
    enum dj_pack_status status;

    n           = d_internal_decimal(head, (uint64_t)
                      d_internal_pax_record_length(key_len, _value_len));
    head[n++]   = ' ';
    memcpy(head + n, _key, key_len);
    n          += key_len;
    head[n++]   = '=';

    status = d_internal_emit(_sink, head, n);

    if (status == D_PACK_STATUS_OK)
    {
        status = d_internal_emit(_sink, _value, _value_len);
    }
    if (status == D_PACK_STATUS_OK)
    {
        status = d_internal_emit(_sink, "\n", 1u);
    }

    return status;
}

/*
d_internal_tar_emit_pax
  Write the pax extended header that precedes a member: its own ustar header
(typeflag 'x'), the records, and the padding to the next record.

  The extended header is named "PaxHeader/" and the tail of the member's
name, so a reader that does not know pax extracts it as a small, plainly
labelled file beside the member rather than failing.

Parameter(s):
  _sink:  the destination.
  _entry: the member.
  _plan:  its plan, with pax_len nonzero.
Return:
  D_PACK_STATUS_OK, or the sink's failure.
*/
static enum dj_pack_status
d_internal_tar_emit_pax(
    struct dj_pack_sink                _sink,
    const struct dj_archive_entry*     _entry,
    const struct dj_internal_tar_plan* _plan
)
{
    unsigned char      block[D_INTERNAL_TAR_BLOCK];
    char               digits[21];
    size_t             off = 0;
    size_t             len = d_internal_tar_tail(_entry->name, 90u, &off);
    size_t             pad = _plan->pax_len % D_INTERNAL_TAR_BLOCK;
    enum dj_pack_status status;

    memset(block, 0, sizeof(block));
    memcpy(block, "PaxHeader/", 10);
    memcpy(block + 10, _entry->name.data + off, len);
    d_internal_tar_finish_header(block, 0644u, (uint64_t)_plan->pax_len,
                                 _plan->mtime, 'x');

    status = d_internal_emit(_sink, block, D_INTERNAL_TAR_BLOCK);

    if ( (status == D_PACK_STATUS_OK) &&
         _plan->pax_path )
    {
        status = d_internal_pax_emit_record(_sink, "path",
                                            _entry->name.data,
                                            _entry->name.length);
    }
    if ( (status == D_PACK_STATUS_OK) &&
         _plan->pax_size )
    {
        len    = d_internal_decimal(digits, (uint64_t)_entry->data.size);
        status = d_internal_pax_emit_record(_sink, "size", digits, len);
    }
    if ( (status == D_PACK_STATUS_OK) &&
         _plan->pax_mtime )
    {
        len    = d_internal_signed_decimal(digits, _plan->mtime);
        status = d_internal_pax_emit_record(_sink, "mtime", digits, len);
    }
    if ( (status == D_PACK_STATUS_OK) &&
         (pad != 0u) )
    {
        status = d_internal_emit_zeros(_sink, D_INTERNAL_TAR_BLOCK - pad);
    }

    return status;
}

/*
d_internal_tar_emit_member
  Write one member: its pax extended header when it needs one, its ustar
header, its payload, and the payload's padding.

Parameter(s):
  _sink:    the destination.
  _entry:   the member, already validated.
  _options: a resolved option set.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_UNSUPPORTED for a member strict ustar
cannot hold, or the sink's failure.
*/
static enum dj_pack_status
d_internal_tar_emit_member(
    struct dj_pack_sink              _sink,
    const struct dj_archive_entry*   _entry,
    const struct dj_archive_options* _options
)
{
    unsigned char               block[D_INTERNAL_TAR_BLOCK];
    struct dj_internal_tar_plan plan;
    size_t                      pad;
    enum dj_pack_status          status;

    status = d_internal_tar_plan_member(_entry, _options, &plan);

    if ( (status == D_PACK_STATUS_OK) &&
         (plan.pax_len != 0u) )
    {
        status = d_internal_tar_emit_pax(_sink, _entry, &plan);
    }
    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    memset(block, 0, sizeof(block));
    memcpy(block + D_INTERNAL_TAR_OFF_NAME,
           _entry->name.data + plan.name_off, plan.name_len);

    if (plan.prefix_len != 0u)
    {
        memcpy(block + D_INTERNAL_TAR_OFF_PREFIX,
               _entry->name.data, plan.prefix_len);
    }

    d_internal_tar_finish_header(
        block, plan.mode,
        plan.pax_size ? (uint64_t)0u : (uint64_t)_entry->data.size,
        plan.mtime, _entry->is_directory ? '5' : '0');

    status = d_internal_emit(_sink, block, D_INTERNAL_TAR_BLOCK);

    if ( (status != D_PACK_STATUS_OK) ||
         (_entry->data.size == 0u) )
    {
        return status;
    }

    status = d_internal_emit(_sink, _entry->data.data, _entry->data.size);
    pad    = _entry->data.size % D_INTERNAL_TAR_BLOCK;

    if ( (status == D_PACK_STATUS_OK) &&
         (pad != 0u) )
    {
        status = d_internal_emit_zeros(_sink, D_INTERNAL_TAR_BLOCK - pad);
    }

    return status;
}

/*
d_internal_tar_create
  Write a complete tar archive to a sink: ustar, with pax extended headers
for the members that need them (see d_internal_tar_plan_member).

Parameter(s):
  _entries: the members, in order; may be null when _count is 0.
  _count:   how many.
  _options: a resolved option set.
  _sink:    the destination.
Return:
  D_PACK_STATUS_OK, or the first failure encountered.
*/
static enum dj_pack_status
d_internal_tar_create(
    const struct dj_archive_entry*   _entries,
    size_t                          _count,
    const struct dj_archive_options* _options,
    struct dj_pack_sink              _sink
)
{
    enum dj_pack_status status;
    size_t             i;

    for (i = 0; i < _count; ++i)
    {
        if (!d_archive_entry_is_valid(&_entries[i]))
        {
            return D_PACK_STATUS_INVALID_ARGUMENT;
        }

        status = d_internal_tar_emit_member(_sink, &_entries[i], _options);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }
    }

    // the end-of-archive marker: two zero records.
    return d_internal_emit_zeros(_sink, 2u * D_INTERNAL_TAR_BLOCK);
}

/*
d_internal_tar_size
  The exact size of the tar archive these members make, by writing it to a
counting sink.  No codec runs, so this costs a walk over the entry list.

Parameter(s):
  _entries:  the members; may be null when _count is 0.
  _count:    how many.
  _options:  a resolved option set.
  _out_size: receives the size.
Return:
  D_PACK_STATUS_OK, or the writer's formal failure.
*/
static enum dj_pack_status
d_internal_tar_size(
    const struct dj_archive_entry*   _entries,
    size_t                          _count,
    const struct dj_archive_options* _options,
    size_t*                         _out_size
)
{
    struct dj_pack_sink counter;

    *_out_size      = 0u;
    counter.write   = &d_internal_count_write;
    counter.context = _out_size;

    return d_internal_tar_create(_entries, _count, _options, counter);
}

// dj_internal_tar_state
//   enum: where the tar reader is in its input.  The reader is pushed bytes
// in pieces of any size -- the whole archive at once for a tar, the inflater's
// output a window at a time for a tar.gz -- so its position is a state and a
// count, never an offset into one buffer.
enum dj_internal_tar_state
{
    D_INTERNAL_TAR_HEAD = 0,    // filling a 512-byte header
    D_INTERNAL_TAR_DATA = 1,    // a member's payload
    D_INTERNAL_TAR_PAX  = 2,    // a pax extended header's records
    D_INTERNAL_TAR_LONG = 3,    // a GNU long name ('L')
    D_INTERNAL_TAR_SKIP = 4,    // a region the reader does not use
    D_INTERNAL_TAR_PAD  = 5,    // padding to the next record
    D_INTERNAL_TAR_END  = 6     // past the end-of-archive marker
};

// dj_internal_pax_parse
//   struct: the pax record parser's state.  Records are "LEN KEY=VALUE\n"
// and may be split anywhere, so the parser is fed a byte at a time.  Only
// three keys matter here -- path, size and mtime -- so a key longer than the
// buffer is simply some other key.
struct dj_internal_pax_parse
{
    int      phase;         // 0 the length, 1 the key, 2 the value
    uint64_t len;           // the record's declared length
    uint64_t pos;           // bytes of it read
    char     key[8];
    size_t   key_len;
    int      which;         // 0 other, 1 path, 2 size, 3 mtime
    int      neg;
    int      frac;
    size_t   digits;
    uint64_t num;
};

// dj_internal_tar_reader
//   struct: a tar reader in one of three modes.  MEASURE: no destination;
// it counts members and the arena bytes they need.  EXTRACT: an entry array
// and an arena; it fills both, and stops writing at the first member that
// does not fit, while still counting.  FIND: a target name and an output
// buffer; it copies the first member of that name.  The three share one
// walk, which is what keeps measure and extract agreeing.
struct dj_internal_tar_reader
{
    struct dj_archive_entry*     entries;
    size_t                       entry_cap;
    unsigned char*               arena;
    size_t                       arena_size;
    int                          finding;
    struct dj_pack_text          target;
    unsigned char*               out;
    size_t                       out_cap;
    enum dj_pack_status          status;
    int                          state;
    unsigned char                block[D_INTERNAL_TAR_BLOCK];
    size_t                       have;
    uint64_t                     region;
    uint64_t                     left;
    size_t                       count;
    size_t                       used;
    int                          writing;
    int                          overflow;
    unsigned char*               dst;
    int                          long_name;
    size_t                       name_at;
    size_t                       name_len;
    int                          name_match;
    int                          name_done;
    int                          has_size;
    uint64_t                     size;
    int                          has_mtime;
    int64_t                      mtime;
    struct dj_internal_pax_parse pax;
    int                          found;
    uint64_t                     found_size;
};

/*
d_internal_tar_reader_init
  Prepare a reader to measure (both regions null) or to extract.

Parameter(s):
  _r:          the reader.
  _entries:    receives the members; may be null.
  _entry_cap:  how many _entries holds.
  _arena:      receives names and payloads; may be null.
  _arena_size: how many bytes _arena holds.
Return:
  none.
*/
static
void d_internal_tar_reader_init(
    struct dj_internal_tar_reader* _r,
    struct dj_archive_entry*       _entries,
    size_t                         _entry_cap,
    void*                          _arena,
    size_t                         _arena_size
)
{
    memset(_r, 0, sizeof(*_r));

    _r->entries    = _entries;
    _r->entry_cap  = _entry_cap;
    _r->arena      = (unsigned char*)_arena;
    _r->arena_size = _arena_size;
    _r->status     = D_PACK_STATUS_OK;
    _r->state      = D_INTERNAL_TAR_HEAD;
    _r->writing    = ( (_entries != NULL) &&
                       (_arena != NULL) );
    _r->overflow   = ( (_entries != NULL) ||
                       (_arena != NULL) ) && !_r->writing;

    return;
}

/*
d_internal_tar_header_ok
  Whether a 512-byte record is a ustar header: the magic at 257, and a
checksum that matches.  Both the unsigned and the signed byte sum are
accepted, since early writers summed signed chars.

Parameter(s):
  _h: the record.
Return:
  1 when it is a header, 0 otherwise.
*/
static
int d_internal_tar_header_ok(
    const unsigned char* _h
)
{
    uint64_t stored = d_internal_get_octal(_h + D_INTERNAL_TAR_OFF_CHKSUM, 8u);
    uint32_t sum    = 0u;
    int32_t  ssum   = 0;
    size_t   i;

    if (memcmp(_h + D_INTERNAL_TAR_OFF_MAGIC, "ustar", 5) != 0)
    {
        return 0;
    }

    for (i = 0; i < D_INTERNAL_TAR_BLOCK; ++i)
    {
        int in_field = ( (i >= D_INTERNAL_TAR_OFF_CHKSUM) &&
                         (i < (D_INTERNAL_TAR_OFF_CHKSUM + 8u)) );
        unsigned char b = in_field ? (unsigned char)' ' : _h[i];

        sum  += (uint32_t)b;
        ssum += (b < 128u) ? (int32_t)b : ((int32_t)b - 256);
    }

    return ( (stored == (uint64_t)sum) ||
             ( (ssum >= 0) &&
               (stored == (uint64_t)ssum) ) ) ? 1 : 0;
}

/*
d_internal_tar_take
  Account for _n arena bytes; while extracting, report whether they fit and
stop writing, for good, at the first that do not.

Parameter(s):
  _r: the reader.
  _n: how many bytes.
Return:
  1 when the bytes are to be written at _r->used, 0 otherwise.
*/
static
int d_internal_tar_take(
    struct dj_internal_tar_reader* _r,
    size_t                         _n
)
{
    if ( _r->writing &&
         (_n > (_r->arena_size - _r->used)) )
    {
        _r->writing  = 0;
        _r->overflow = 1;
    }

    return _r->writing;
}

/*
d_internal_tar_name_byte
  One byte of a name carried outside the header (a pax path or a GNU long
name): counted, stored in the arena when extracting, compared with the target
when finding.

Parameter(s):
  _r: the reader.
  _b: the byte.
Return:
  none.
*/
static
void d_internal_tar_name_byte(
    struct dj_internal_tar_reader* _r,
    unsigned char                  _b
)
{
    if (d_internal_tar_take(_r, 1u))
    {
        _r->arena[_r->used] = _b;
    }
    if ( _r->finding &&
         ( (_r->name_len >= _r->target.length) ||
           ((unsigned char)_r->target.data[_r->name_len] != _b) ) )
    {
        _r->name_match = 0;
    }

    ++_r->used;
    ++_r->name_len;

    return;
}

/*
d_internal_tar_name_start
  Begin a name carried outside the header, at the arena's current end.

Parameter(s):
  _r: the reader.
Return:
  none.
*/
static
void d_internal_tar_name_start(
    struct dj_internal_tar_reader* _r
)
{
    _r->long_name  = 1;
    _r->name_at    = _r->used;
    _r->name_len   = 0u;
    _r->name_match = 1;
    _r->name_done  = 0;

    return;
}

/*
d_internal_tar_header_name
  The member's name from its header -- prefix, '/', name -- placed in the
arena when extracting and compared with the target when finding.

Parameter(s):
  _r:         the reader, its block holding the header.
  _out_match: receives 1 when the name is the target.
Return:
  The name's length.
*/
static
size_t d_internal_tar_header_name(
    struct dj_internal_tar_reader* _r,
    int*                           _out_match
)
{
    const unsigned char* h  = _r->block;
    size_t               nl = d_internal_field_len(h + D_INTERNAL_TAR_OFF_NAME,
                                                   100u);
    size_t               pl = d_internal_field_len(
                                  h + D_INTERNAL_TAR_OFF_PREFIX, 155u);
    size_t               total = (pl != 0u) ? (pl + 1u + nl) : nl;
    struct dj_pack_text   t     = _r->target;

    *_out_match = 0;

    if (_r->finding)
    {
        *_out_match = (pl != 0u)
            ? ( (t.length == total) &&
                (memcmp(t.data, h + D_INTERNAL_TAR_OFF_PREFIX, pl) == 0) &&
                (t.data[pl] == '/') &&
                (memcmp(t.data + pl + 1u, h + D_INTERNAL_TAR_OFF_NAME,
                        nl) == 0) )
            : ( (t.length == nl) &&
                (memcmp(t.data, h + D_INTERNAL_TAR_OFF_NAME, nl) == 0) );
    }

    if (d_internal_tar_take(_r, total))
    {
        unsigned char* n = _r->arena + _r->used;

        if (pl != 0u)
        {
            memcpy(n, h + D_INTERNAL_TAR_OFF_PREFIX, pl);
            n[pl] = (unsigned char)'/';
            memcpy(n + pl + 1u, h + D_INTERNAL_TAR_OFF_NAME, nl);
        }
        else
        {
            memcpy(n, h + D_INTERNAL_TAR_OFF_NAME, nl);
        }
    }

    _r->used += total;

    return total;
}

/*
d_internal_tar_region_end
  Finish the region just read and move to its padding, then the next header.

  A pax region must end on a record boundary; a GNU long name becomes the next
member's name; a member's payload, when finding and it was the target, ends
the walk.

Parameter(s):
  _r: the reader.
Return:
  none.
*/
static
void d_internal_tar_region_end(
    struct dj_internal_tar_reader* _r
)
{
    size_t pad = (size_t)(_r->region % D_INTERNAL_TAR_BLOCK);

    if ( (_r->state == D_INTERNAL_TAR_PAX) &&
         ( (_r->pax.phase != 0) ||
           (_r->pax.pos != 0u) ) )
    {
        _r->status = D_PACK_STATUS_CORRUPT_INPUT;

        return;
    }
    if ( (_r->state == D_INTERNAL_TAR_LONG) &&
         (_r->name_len == 0u) )
    {
        _r->long_name = 0;
    }
    if ( (_r->state == D_INTERNAL_TAR_DATA) &&
         _r->finding &&
         _r->found )
    {
        _r->state = D_INTERNAL_TAR_END;

        return;
    }

    _r->dst   = NULL;
    _r->left  = (pad != 0u) ? (D_INTERNAL_TAR_BLOCK - pad) : 0u;
    _r->state = (pad != 0u) ? D_INTERNAL_TAR_PAD : D_INTERNAL_TAR_HEAD;

    return;
}

/*
d_internal_tar_region
  Enter a region of _size bytes in _state, finishing it at once when empty.

Parameter(s):
  _r:     the reader.
  _state: the region's state.
  _size:  its size.
Return:
  none.
*/
static
void d_internal_tar_region(
    struct dj_internal_tar_reader* _r,
    int                            _state,
    uint64_t                       _size
)
{
    _r->state  = _state;
    _r->region = _size;
    _r->left   = _size;

    if (_size == 0u)
    {
        d_internal_tar_region_end(_r);
    }

    return;
}

/*
d_internal_tar_begin_member
  Start a member from the header in the block: its name (from the header or
from a preceding extension), its size and timestamp (likewise), its slot in
the entry array, and where its payload goes.

Parameter(s):
  _r: the reader.
Return:
  none.
*/
static
void d_internal_tar_begin_member(
    struct dj_internal_tar_reader* _r
)
{
    const unsigned char* h     = _r->block;
    uint64_t             size  = _r->has_size ? _r->size
                               : d_internal_get_octal(
                                     h + D_INTERNAL_TAR_OFF_SIZE, 12u);
    size_t               at    = _r->name_at;
    size_t               len   = _r->name_len;
    int                  match = _r->name_match &&
                                 (len == _r->target.length);

    if (!_r->long_name)
    {
        at  = _r->used;
        len = d_internal_tar_header_name(_r, &match);
    }

    // a payload the address space cannot hold is not in this input
    if (size > (uint64_t)((size_t)-1 - _r->used))
    {
        _r->status = D_PACK_STATUS_TRUNCATED_INPUT;

        return;
    }

    _r->dst = NULL;

    if ( _r->writing &&
         (_r->count < _r->entry_cap) &&
         d_internal_tar_take(_r, (size_t)size) )
    {
        struct dj_archive_entry* e = &_r->entries[_r->count];

        e->name.data    = (const char*)(_r->arena + at);
        e->name.length  = len;
        e->data.data    = (size != 0u) ? (_r->arena + _r->used) : NULL;
        e->data.size    = (size_t)size;
        e->is_directory = (h[D_INTERNAL_TAR_OFF_TYPE] == '5') ? 1 : 0;
        e->mode         = (uint32_t)d_internal_get_octal(
                              h + D_INTERNAL_TAR_OFF_MODE, 8u);
        e->mtime        = _r->has_mtime ? _r->mtime
                        : (int64_t)d_internal_get_octal(
                              h + D_INTERNAL_TAR_OFF_MTIME, 12u);
        _r->dst         = _r->arena + _r->used;
    }
    else if (_r->writing)
    {
        _r->writing  = 0;
        _r->overflow = 1;
    }

    if ( _r->finding &&
         match &&
         !_r->found )
    {
        _r->found      = 1;
        _r->found_size = size;
        _r->dst        = ( (_r->out != NULL) &&
                           (size <= (uint64_t)_r->out_cap) ) ? _r->out : NULL;
    }

    _r->used     += (size_t)size;
    _r->count    += 1u;
    _r->long_name = 0;
    _r->has_size  = 0;
    _r->has_mtime = 0;

    d_internal_tar_region(_r, D_INTERNAL_TAR_DATA, size);

    return;
}

/*
d_internal_tar_on_header
  Act on a complete 512-byte record: the end marker, an extension (pax 'x',
pax global 'g', GNU long name 'L' or long link 'K'), or a member.

  A pax global header's records apply to every later member; none of the
three keys this reader honours is meaningful globally, so its records are
skipped.  A typeflag the reader does not know is read as a member, as POSIX
asks of a reader.

Parameter(s):
  _r: the reader.
Return:
  none.
*/
static
void d_internal_tar_on_header(
    struct dj_internal_tar_reader* _r
)
{
    const unsigned char* h = _r->block;
    uint64_t             size;

    // two zero records end the archive; one is enough to recognise it.
    if (h[0] == 0u)
    {
        _r->state = D_INTERNAL_TAR_END;

        return;
    }
    if (!d_internal_tar_header_ok(h))
    {
        _r->status = D_PACK_STATUS_CORRUPT_INPUT;

        return;
    }

    size = d_internal_get_octal(h + D_INTERNAL_TAR_OFF_SIZE, 12u);

    switch (h[D_INTERNAL_TAR_OFF_TYPE])
    {
        case 'x':
        {
            memset(&_r->pax, 0, sizeof(_r->pax));
            d_internal_tar_region(_r, D_INTERNAL_TAR_PAX, size);
            break;
        }
        case 'L':
        {
            d_internal_tar_name_start(_r);
            d_internal_tar_region(_r, D_INTERNAL_TAR_LONG, size);
            break;
        }
        case 'g':
        case 'K':
        {
            d_internal_tar_region(_r, D_INTERNAL_TAR_SKIP, size);
            break;
        }
        default:
        {
            d_internal_tar_begin_member(_r);
            break;
        }
    }

    return;
}

/*
d_internal_pax_key
  Classify the key just read, and start its value.

Parameter(s):
  _r: the reader.
Return:
  none.
*/
static
void d_internal_pax_key(
    struct dj_internal_tar_reader* _r
)
{
    struct dj_internal_pax_parse* p = &_r->pax;

    p->which  = 0;
    p->neg    = 0;
    p->frac   = 0;
    p->digits = 0u;
    p->num    = 0u;

    if ( (p->key_len == 4u) &&
         (memcmp(p->key, "path", 4) == 0) )
    {
        p->which = 1;
        d_internal_tar_name_start(_r);
    }
    else if ( (p->key_len == 4u) &&
              (memcmp(p->key, "size", 4) == 0) )
    {
        p->which = 2;
    }
    else if ( (p->key_len == 5u) &&
              (memcmp(p->key, "mtime", 5) == 0) )
    {
        p->which = 3;
    }

    return;
}

/*
d_internal_pax_value_byte
  One byte of a value (never the record's closing '\n').  A path byte goes to
the name; a size must be digits; an mtime may carry a sign and a fraction,
which is dropped.

Parameter(s):
  _r: the reader.
  _b: the byte.
Return:
  none.
*/
static
void d_internal_pax_value_byte(
    struct dj_internal_tar_reader* _r,
    unsigned char                  _b
)
{
    struct dj_internal_pax_parse* p = &_r->pax;

    if (p->which == 1)
    {
        d_internal_tar_name_byte(_r, _b);

        return;
    }
    if (p->which == 0)
    {
        return;
    }
    if ( (p->which == 3) &&
         (_b == '-') &&
         (p->digits == 0u) &&
         !p->neg )
    {
        p->neg = 1;
    }
    else if ( (p->which == 3) &&
              (_b == '.') &&
              (p->digits != 0u) &&
              !p->frac )
    {
        p->frac = 1;
    }
    else if ( (_b >= '0') &&
              (_b <= '9') &&
              (p->num <= (((uint64_t)-1) / 10u) - 1u) )
    {
        if (!p->frac)
        {
            p->num = (p->num * 10u) + (uint64_t)(_b - '0');
        }

        ++p->digits;
    }
    else
    {
        _r->status = D_PACK_STATUS_CORRUPT_INPUT;
    }

    return;
}

/*
d_internal_pax_record_end
  The record's closing '\n' has arrived: apply its value.  An empty path
deletes the keyword, as POSIX defines, so the header's own name stands.

Parameter(s):
  _r: the reader.
Return:
  none.
*/
static
void d_internal_pax_record_end(
    struct dj_internal_tar_reader* _r
)
{
    struct dj_internal_pax_parse* p = &_r->pax;

    if ( (p->which == 1) &&
         (_r->name_len == 0u) )
    {
        _r->long_name = 0;
    }
    else if ( (p->which >= 2) &&
              (p->digits == 0u) )
    {
        _r->status = D_PACK_STATUS_CORRUPT_INPUT;
    }
    else if (p->which == 2)
    {
        _r->has_size = 1;
        _r->size     = p->num;
    }
    else if ( (p->which == 3) &&
              (p->num <= (uint64_t)INT64_MAX) )
    {
        _r->has_mtime = 1;
        _r->mtime     = p->neg ? -(int64_t)p->num : (int64_t)p->num;
    }

    memset(p, 0, sizeof(*p));

    return;
}

/*
d_internal_pax_byte
  Feed one byte of a pax extended header's data to the record parser.

Parameter(s):
  _r: the reader.
  _b: the byte.
Return:
  none.
*/
static
void d_internal_pax_byte(
    struct dj_internal_tar_reader* _r,
    unsigned char                  _b
)
{
    struct dj_internal_pax_parse* p = &_r->pax;

    ++p->pos;

    if (p->phase == 0)
    {
        if ( (_b >= '0') &&
             (_b <= '9') &&
             (p->len < ((uint64_t)1 << 40)) )
        {
            p->len = (p->len * 10u) + (uint64_t)(_b - '0');
        }
        else if ( (_b == ' ') &&
                  (p->len > p->pos) )
        {
            p->phase = 1;
        }
        else
        {
            _r->status = D_PACK_STATUS_CORRUPT_INPUT;
        }
    }
    else if (p->phase == 1)
    {
        if (_b == '=')
        {
            p->phase = 2;
            d_internal_pax_key(_r);
        }
        else if (p->key_len < sizeof(p->key))
        {
            p->key[p->key_len++] = (char)_b;
        }
        else
        {
            p->key_len = sizeof(p->key) + 1u;
        }
    }
    else if (p->pos < p->len)
    {
        d_internal_pax_value_byte(_r, _b);
    }
    else if (_b == '\n')
    {
        d_internal_pax_record_end(_r);
    }

    if ( (_r->status == D_PACK_STATUS_OK) &&
         (p->phase != 0) &&
         ( (p->pos >= p->len) ||
           ( (p->phase == 1) &&
             ((p->pos + 1u) >= p->len) ) ) )
    {
        _r->status = D_PACK_STATUS_CORRUPT_INPUT;
    }

    return;
}

/*
d_internal_tar_feed_region
  Consume up to _size bytes of the current region.

Parameter(s):
  _r:    the reader, in a region state.
  _data: the bytes.
  _size: how many are offered.
Return:
  How many were consumed.
*/
static
size_t d_internal_tar_feed_region(
    struct dj_internal_tar_reader* _r,
    const unsigned char*           _data,
    size_t                         _size
)
{
    size_t take = ((uint64_t)_size < _r->left) ? _size : (size_t)_r->left;
    size_t i;

    if ( (_r->state == D_INTERNAL_TAR_DATA) &&
         (_r->dst != NULL) )
    {
        memcpy(_r->dst, _data, take);
        _r->dst += take;
    }
    else if (_r->state == D_INTERNAL_TAR_PAX)
    {
        for (i = 0; (i < take) && (_r->status == D_PACK_STATUS_OK); ++i)
        {
            d_internal_pax_byte(_r, _data[i]);
        }
    }
    else if (_r->state == D_INTERNAL_TAR_LONG)
    {
        // the name runs to its NUL; the rest of the region is padding
        for (i = 0; (i < take) && !_r->name_done; ++i)
        {
            if (_data[i] == 0u)
            {
                _r->name_done = 1;
            }
            else
            {
                d_internal_tar_name_byte(_r, _data[i]);
            }
        }
    }

    _r->left -= take;

    if ( (_r->left == 0u) &&
         (_r->status == D_PACK_STATUS_OK) )
    {
        if (_r->state == D_INTERNAL_TAR_PAD)
        {
            _r->state = D_INTERNAL_TAR_HEAD;
        }
        else
        {
            d_internal_tar_region_end(_r);
        }
    }

    return take;
}

/*
d_internal_tar_feed
  Push bytes through the reader.  Pieces may be any size and split anything:
a header, a pax record, a payload.

Parameter(s):
  _r:    the reader.
  _data: the bytes.
  _size: how many.
Return:
  none; a formal failure is recorded in _r->status and stops the walk.
*/
static
void d_internal_tar_feed(
    struct dj_internal_tar_reader* _r,
    const void*                    _data,
    size_t                         _size
)
{
    const unsigned char* p = (const unsigned char*)_data;

    while ( (_size > 0u) &&
            (_r->status == D_PACK_STATUS_OK) &&
            (_r->state != D_INTERNAL_TAR_END) )
    {
        size_t take;

        if (_r->state == D_INTERNAL_TAR_HEAD)
        {
            take = D_INTERNAL_TAR_BLOCK - _r->have;
            take = (take < _size) ? take : _size;

            memcpy(_r->block + _r->have, p, take);
            _r->have += take;

            if (_r->have == D_INTERNAL_TAR_BLOCK)
            {
                _r->have = 0u;
                d_internal_tar_on_header(_r);
            }
        }
        else
        {
            take = d_internal_tar_feed_region(_r, p, _size);
        }

        p     += take;
        _size -= take;
    }

    return;
}

/*
d_internal_tar_reader_write
  The reader as a sink: how a tar.gz is read, the inflater pushing its output
straight into the walk with nothing staged.  Everything past the end marker
is accepted and ignored, so the inflater runs to its trailer and checks the
stream's CRC.

Parameter(s):
  _context: a struct dj_internal_tar_reader.
  _data:    the bytes.
  _size:    how many.
Return:
  _size, or 0 once the walk has failed, which stops the inflater.
*/
static
size_t d_internal_tar_reader_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct dj_internal_tar_reader* r =
        (struct dj_internal_tar_reader*)_context;

    d_internal_tar_feed(r, _data, _size);

    return (r->status == D_PACK_STATUS_OK) ? _size : 0u;
}

/*
d_internal_all_zero
  Whether _size bytes are all zero; true of none.

Parameter(s):
  _p:    the bytes.
  _size: how many.
Return:
  1 or 0.
*/
static
int d_internal_all_zero(
    const unsigned char* _p,
    size_t               _size
)
{
    size_t i;

    for (i = 0; i < _size; ++i)
    {
        if (_p[i] != 0u)
        {
            return 0;
        }
    }

    return 1;
}

/*
d_internal_tar_reader_finish
  The walk's verdict once the input is exhausted.

  Input that stops inside a header, a payload or an extension is truncated.
Input that stops in the padding after the last payload, inside the zero
records that end the archive, or with no end marker at all, is accepted, as
every mainstream reader accepts it.

Parameter(s):
  _r:   the reader.
  _out: receives the entry count and the arena size; may be null.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL when extracting into
regions that were short, or the formal failure.
*/
static enum dj_pack_status
d_internal_tar_reader_finish(
    struct dj_internal_tar_reader* _r,
    struct dj_archive_layout*      _out
)
{
    if (_out != NULL)
    {
        _out->entry_count = _r->count;
        _out->arena_size  = _r->used;
    }
    if (_r->status != D_PACK_STATUS_OK)
    {
        return _r->status;
    }
    if ( ( (_r->state == D_INTERNAL_TAR_HEAD) &&
           !d_internal_all_zero(_r->block, _r->have) ) ||
         ( (_r->state != D_INTERNAL_TAR_HEAD) &&
           (_r->state != D_INTERNAL_TAR_PAD) &&
           (_r->state != D_INTERNAL_TAR_END) ) )
    {
        return D_PACK_STATUS_TRUNCATED_INPUT;
    }

    return _r->overflow ? D_PACK_STATUS_BUFFER_TOO_SMALL : D_PACK_STATUS_OK;
}

/*
d_internal_tar_read
  Run a reader over a tar, or over a tar.gz through the inflater.

Parameter(s):
  _format:  D_FORMAT_ID_TAR or D_FORMAT_ID_TAR_GZ.
  _in:      the archive bytes.
  _in_size: how many.
  _r:       the reader, initialised.
  _out:     receives the layout; may be null.
Return:
  As d_internal_tar_reader_finish, or the inflater's failure.
*/
static enum dj_pack_status
d_internal_tar_read(
    enum dj_format_id              _format,
    const void*                    _in,
    size_t                         _in_size,
    struct dj_internal_tar_reader* _r,
    struct dj_archive_layout*      _out
)
{
    struct dj_pack_sink sink;
    enum dj_pack_status status;

    if (_format == D_FORMAT_ID_TAR)
    {
        d_internal_tar_feed(_r, _in, _in_size);

        return d_internal_tar_reader_finish(_r, _out);
    }

    sink.write   = &d_internal_tar_reader_write;
    sink.context = _r;
    status       = d_decompress_to_sink(D_CODEC_ID_GZIP, _in, _in_size,
                                        sink, NULL);

    // the walk's own failure is the cause when it stopped the inflater
    if (_r->status != D_PACK_STATUS_OK)
    {
        return d_internal_tar_reader_finish(_r, _out);
    }
    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    return d_internal_tar_reader_finish(_r, _out);
}

/*
d_internal_tar_find
  Copy the first member named _name out of a tar or tar.gz, or measure it.

Parameter(s):
  _format:       D_FORMAT_ID_TAR or D_FORMAT_ID_TAR_GZ.
  _in:           the archive bytes.
  _in_size:      how many.
  _name:         the member's name, nonempty.
  _out:          the destination; null with _out_capacity 0 measures.
  _out_capacity: how many bytes _out holds.
  _out_size:     receives the member's size.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL, a formal failure, or
D_PACK_STATUS_INVALID_ARGUMENT when no member has that name.
*/
static enum dj_pack_status
d_internal_tar_find(
    enum dj_format_id   _format,
    const void*         _in,
    size_t              _in_size,
    struct dj_pack_text _name,
    void*               _out,
    size_t              _out_capacity,
    size_t*             _out_size
)
{
    struct dj_internal_tar_reader r;
    enum dj_pack_status           status;

    d_internal_tar_reader_init(&r, NULL, 0u, NULL, 0u);

    r.finding = 1;
    r.target  = _name;
    r.out     = (unsigned char*)_out;
    r.out_cap = _out_capacity;
    status    = d_internal_tar_read(_format, _in, _in_size, &r, NULL);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }
    if (!r.found)
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    *_out_size = (size_t)r.found_size;

    if ( (_out == NULL) &&
         (_out_capacity == 0u) )
    {
        return D_PACK_STATUS_OK;
    }

    return (r.found_size > (uint64_t)_out_capacity)
         ? D_PACK_STATUS_BUFFER_TOO_SMALL
         : D_PACK_STATUS_OK;
}


// =============================================================================
// IX.  THE BUILT-IN ZIP WRITER / READER
// =============================================================================
//   The ZIP appnote's classic 32-bit layout, written from the specification
// with no dependency.  Each member is a local header followed by its data; a
// central directory repeats the headers at the end, and an end-of-central-
// directory record points at it.
//
//   THE TWO-PASS COST, RESTATED WHERE IT HAPPENS:
//   A local header records the compressed size of the data that FOLLOWS it, so
// the size must be known before the header is written.  With no allocator
// there is nowhere to stage the compressed bytes, so a compressed entry is
// encoded twice: once into a counting sink to learn its size, once into the
// real sink.  A stored entry is encoded zero extra times, because its
// compressed size is its input size.  The alternative -- a trailing data
// descriptor -- avoids the second pass but is mishandled by enough readers
// that it is not worth the compatibility.

/*
d_internal_zip_codec_for_method
  The codec that performs a ZIP method.

Parameter(s):
  _method: a dj_zip_method value.
Return:
  The codec, or D_CODEC_ID_STORE for an unrecognised method.
*/
static enum dj_codec_id
d_internal_zip_codec_for_method(
    int32_t _method
)
{
    // LZMA (14) is not here: ZIP's LZMA is a raw LZMA1 stream behind a
    // four-byte header, and the codec set has no raw LZMA1, only xz.  An xz
    // stream labelled 14 is an archive no other reader opens.
    switch (_method)
    {
        case D_ZIP_METHOD_DEFLATE: { return D_CODEC_ID_DEFLATE; }
        case D_ZIP_METHOD_BZIP2:   { return D_CODEC_ID_BZIP2; }
        case D_ZIP_METHOD_ZSTD:    { return D_CODEC_ID_ZSTD; }
        case D_ZIP_METHOD_XZ:      { return D_CODEC_ID_XZ; }
        default:                   { return D_CODEC_ID_STORE; }
    }
}

/*
d_internal_zip_entry_method
  The method actually used for one entry, after every fallback.

  A directory has no payload and is always stored.  An empty file is stored
too: compressing zero bytes produces a non-empty DEFLATE stream, which is
larger than the input and confuses several readers.  Anything else uses the
requested method if this build can perform it, and falls back to store if not
-- which is what keeps a zero-dependency build producing valid archives.

Parameter(s):
  _entry:   the entry.
  _options: a resolved option set.
Return:
  A dj_zip_method value.
*/
static
int32_t d_internal_zip_entry_method(
    const struct dj_archive_entry*   _entry,
    const struct dj_archive_options* _options
)
{
    enum dj_codec_id codec;

    if ( _entry->is_directory ||
         (_entry->data.size == 0u) ||
         (_options->store_only) )
    {
        return D_ZIP_METHOD_STORE;
    }

    codec = d_internal_zip_codec_for_method(_options->zip.method);

    if (!d_codec_can_compress(codec))
    {
        return D_ZIP_METHOD_STORE;
    }

    return _options->zip.method;
}

/*
d_internal_zip_emit_name
  Write an entry's name, and the directory separator the plan added.

Parameter(s):
  _sink:  the destination.
  _entry: the entry.
  _slash: 1 when a '/' follows the name.
Return:
  D_PACK_STATUS_OK, or the sink's failure.
*/
static enum dj_pack_status
d_internal_zip_emit_name(
    struct dj_pack_sink            _sink,
    const struct dj_archive_entry* _entry,
    size_t                         _slash
)
{
    enum dj_pack_status status;

    status = d_internal_emit(_sink, _entry->name.data, _entry->name.length);

    if ( (status == D_PACK_STATUS_OK) &&
         (_slash != 0u) )
    {
        status = d_internal_emit(_sink, "/", 1u);
    }

    return status;
}

// dj_internal_zip_plan
//   struct: everything the writer decides about one member, decided once so
// the local header and the central directory cannot disagree.  With a stage,
// the plans are kept there between the two passes; without one, the central
// pass decides them again.
struct dj_internal_zip_plan
{
    uint32_t crc;
    int32_t  method;
    size_t   csize;
    size_t   slash;
};

/*
d_internal_zip_slash
  Whether a '/' must follow a member's name: a directory whose name lacks it.
The separator is how a ZIP reader -- this one included -- knows a directory;
there is no type flag in the record.

Parameter(s):
  _entry: the member.
Return:
  1 or 0.
*/
static
size_t d_internal_zip_slash(
    const struct dj_archive_entry* _entry
)
{
    return ( (_entry->is_directory) &&
             ( (_entry->name.length == 0u) ||
               (_entry->name.data[_entry->name.length - 1u] != '/') ) )
         ? 1u : 0u;
}

/*
d_internal_zip_entry_plan
  Plan one member without a stage: the compressed size comes from a run of
the codec into a counting sink, and the writer runs it again to emit.

  The method is d_internal_zip_entry_method's, except that a compressed
stream no smaller than the payload is not worth its method: the member is
stored instead, as every mainstream writer does (a two-byte file deflates to
four).

Parameter(s):
  _entry:   the member.
  _options: a resolved option set.
  _plan:    receives the plan.
Return:
  D_PACK_STATUS_OK, or a status from the codec.
*/
static enum dj_pack_status
d_internal_zip_entry_plan(
    const struct dj_archive_entry*   _entry,
    const struct dj_archive_options* _options,
    struct dj_internal_zip_plan*     _plan
)
{
    struct dj_pack_sink counter;
    enum dj_pack_status status = D_PACK_STATUS_OK;

    _plan->method = d_internal_zip_entry_method(_entry, _options);
    _plan->crc    = d_pack_crc32(0u, _entry->data.data, _entry->data.size);
    _plan->csize  = _entry->data.size;
    _plan->slash  = d_internal_zip_slash(_entry);

    if (_plan->method != D_ZIP_METHOD_STORE)
    {
        _plan->csize    = 0u;
        counter.write   = &d_internal_count_write;
        counter.context = &_plan->csize;
        status          = d_compress_to_sink(
            d_internal_zip_codec_for_method(_plan->method),
            _entry->data.data, _entry->data.size,
            &_options->codec, counter, NULL);
    }

    // no gain: store (the CRC is over the payload either way)
    if ( (status == D_PACK_STATUS_OK) &&
         (_plan->csize >= _entry->data.size) )
    {
        _plan->method = D_ZIP_METHOD_STORE;
        _plan->csize  = _entry->data.size;
    }

    return status;
}

/*
d_internal_zip_entry_plan_staged
  Plan one member with a stage: the codec runs once, into the stage's
scratch, and the writer emits those bytes.

Parameter(s):
  _entry:       the member.
  _options:     a resolved option set.
  _scratch:     at least the member's compress bound.
  _scratch_cap: its size.
  _plan:        receives the plan.
Return:
  D_PACK_STATUS_OK, or a status from the codec.
*/
static enum dj_pack_status
d_internal_zip_entry_plan_staged(
    const struct dj_archive_entry*   _entry,
    const struct dj_archive_options* _options,
    unsigned char*                   _scratch,
    size_t                           _scratch_cap,
    struct dj_internal_zip_plan*     _plan
)
{
    enum dj_pack_status status = D_PACK_STATUS_OK;

    _plan->method = d_internal_zip_entry_method(_entry, _options);
    _plan->crc    = d_pack_crc32(0u, _entry->data.data, _entry->data.size);
    _plan->csize  = _entry->data.size;
    _plan->slash  = d_internal_zip_slash(_entry);

    if (_plan->method != D_ZIP_METHOD_STORE)
    {
        status = d_compress(d_internal_zip_codec_for_method(_plan->method),
                            _entry->data.data, _entry->data.size,
                            &_options->codec,
                            _scratch, _scratch_cap, &_plan->csize);
    }
    if ( (status == D_PACK_STATUS_OK) &&
         (_plan->csize >= _entry->data.size) )
    {
        _plan->method = D_ZIP_METHOD_STORE;
        _plan->csize  = _entry->data.size;
    }

    return status;
}

/*
d_internal_zip_fits
  Whether the classic (non-zip64) records can hold a member: 16-bit name
lengths and 32-bit sizes and offsets.  A member that does not fit is refused
with D_PACK_STATUS_UNSUPPORTED rather than written with its fields cut short,
which makes an archive no reader can open.

Parameter(s):
  _entry:  the member.
  _plan:   its plan.
  _offset: where its local header starts.
Return:
  1 when it fits, 0 otherwise.
*/
static
int d_internal_zip_fits(
    const struct dj_archive_entry*     _entry,
    const struct dj_internal_zip_plan* _plan,
    size_t                             _offset
)
{
    return ( ((uint64_t)_entry->name.length + _plan->slash <= 0xFFFFu) &&
             ((uint64_t)_entry->data.size <= 0xFFFFFFFFu) &&
             ((uint64_t)_plan->csize <= 0xFFFFFFFFu) &&
             ((uint64_t)_offset <= 0xFFFFFFFFu) ) ? 1 : 0;
}

/*
d_internal_zip_put_common
  The fields the local header and the central record share, written at their
offsets in each: version needed, flags, method, time, date, CRC, sizes and
the name's length.

Parameter(s):
  _at:      the field block (local + 4, central + 6).
  _entry:   the member.
  _options: a resolved option set.
  _plan:    its plan.
Return:
  none.
*/
static
void d_internal_zip_put_common(
    unsigned char*                     _at,
    const struct dj_archive_entry*     _entry,
    const struct dj_archive_options*   _options,
    const struct dj_internal_zip_plan* _plan
)
{
    uint32_t date = 0;
    uint32_t time = 0;

    d_internal_dos_datetime(d_internal_entry_mtime(_entry, _options),
                            &date, &time);

    d_internal_put_le16(_at + 0,  20u);         // version needed
    d_internal_put_le16(_at + 2,
        (uint32_t)(_options->zip.utf8_names ? 0x0800u : 0u));
    d_internal_put_le16(_at + 4,  (uint32_t)_plan->method);
    d_internal_put_le16(_at + 6,  time);
    d_internal_put_le16(_at + 8,  date);
    d_internal_put_le32(_at + 10, _plan->crc);
    d_internal_put_le32(_at + 14, (uint32_t)_plan->csize);
    d_internal_put_le32(_at + 18, (uint32_t)_entry->data.size);
    d_internal_put_le16(_at + 22,
                        (uint32_t)(_entry->name.length + _plan->slash));

    return;
}

/*
d_internal_zip_emit_local
  Write one member's local header, its name, and its data.

Parameter(s):
  _sink:    the destination.
  _entry:   the member.
  _options: a resolved option set.
  _plan:    its plan.
  _scratch: the compressed bytes when the plan was staged, else null.
Return:
  D_PACK_STATUS_OK, or the first failure.
*/
static enum dj_pack_status
d_internal_zip_emit_local(
    struct dj_pack_sink                _sink,
    const struct dj_archive_entry*     _entry,
    const struct dj_archive_options*   _options,
    const struct dj_internal_zip_plan* _plan,
    const unsigned char*               _scratch
)
{
    unsigned char      hdr[D_INTERNAL_ZIP_LOCAL_SIZE];
    enum dj_pack_status status;

    memset(hdr, 0, sizeof(hdr));
    memcpy(hdr, "PK\003\004", 4);
    d_internal_zip_put_common(hdr + 4, _entry, _options, _plan);

    status = d_internal_emit(_sink, hdr, D_INTERNAL_ZIP_LOCAL_SIZE);

    if (status == D_PACK_STATUS_OK)
    {
        status = d_internal_zip_emit_name(_sink, _entry, _plan->slash);
    }
    if ( (status != D_PACK_STATUS_OK) ||
         (_entry->data.size == 0u) )
    {
        return status;
    }
    if (_plan->method == D_ZIP_METHOD_STORE)
    {
        return d_internal_emit(_sink, _entry->data.data, _entry->data.size);
    }
    if (_scratch != NULL)
    {
        return d_internal_emit(_sink, _scratch, _plan->csize);
    }

    return d_compress_to_sink(d_internal_zip_codec_for_method(_plan->method),
                              _entry->data.data, _entry->data.size,
                              &_options->codec, _sink, NULL);
}

/*
d_internal_zip_emit_central
  Write one member's central directory record and its name.

  The external attributes carry the unix mode in the high 16 bits and the
MS-DOS directory bit in the low byte, so that both worlds agree about what is
a directory.

Parameter(s):
  _sink:    the destination.
  _entry:   the member.
  _options: a resolved option set.
  _plan:    its plan.
  _offset:  where its local header starts.
Return:
  D_PACK_STATUS_OK, or the sink's failure.
*/
static enum dj_pack_status
d_internal_zip_emit_central(
    struct dj_pack_sink                _sink,
    const struct dj_archive_entry*     _entry,
    const struct dj_archive_options*   _options,
    const struct dj_internal_zip_plan* _plan,
    size_t                             _offset
)
{
    unsigned char      hdr[D_INTERNAL_ZIP_CENTRAL_SIZE];
    uint32_t           attr;
    enum dj_pack_status status;

    attr = (d_internal_entry_mode(_entry, _options) & 07777u) << 16;

    if (_entry->is_directory)
    {
        attr |= 0x40000000u | 0x10u;
    }

    memset(hdr, 0, sizeof(hdr));
    memcpy(hdr, "PK\001\002", 4);
    d_internal_put_le16(hdr + 4, (uint32_t)(3u << 8 | 20u));   // unix
    d_internal_zip_put_common(hdr + 6, _entry, _options, _plan);
    d_internal_put_le32(hdr + 38, attr);
    d_internal_put_le32(hdr + 42, (uint32_t)_offset);

    status = d_internal_emit(_sink, hdr, D_INTERNAL_ZIP_CENTRAL_SIZE);

    if (status == D_PACK_STATUS_OK)
    {
        status = d_internal_zip_emit_name(_sink, _entry, _plan->slash);
    }

    return status;
}

// D_INTERNAL_ZIP_PLAN_SIZE
//   constant: the stage bytes one kept plan takes.  Plans are copied in and
// out with memcpy, so the stage needs no particular alignment.
#define D_INTERNAL_ZIP_PLAN_SIZE    sizeof(struct dj_internal_zip_plan)

/*
d_internal_zip_stage_size
  The stage a ZIP of these members can use: a kept plan per member, and
scratch for the largest member's compressed bytes (its compress bound).  With
it, each member is compressed once; without one, three times.

Parameter(s):
  _entries:  the members; may be null when _count is 0.
  _count:    how many.
  _options:  a resolved option set.
  _out_size: receives the size.
Return:
  D_PACK_STATUS_OK, or the codec's failure to bound.
*/
static enum dj_pack_status
d_internal_zip_stage_size(
    const struct dj_archive_entry*   _entries,
    size_t                          _count,
    const struct dj_archive_options* _options,
    size_t*                         _out_size
)
{
    size_t largest = 0u;
    size_t i;

    for (i = 0; i < _count; ++i)
    {
        int32_t            method = d_internal_zip_entry_method(&_entries[i],
                                                                _options);
        size_t             bound  = 0u;
        enum dj_pack_status status;

        if (method == D_ZIP_METHOD_STORE)
        {
            continue;
        }

        status = d_compress_bound(d_internal_zip_codec_for_method(method),
                                  _entries[i].data.size, &_options->codec,
                                  &bound);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }

        largest = (bound > largest) ? bound : largest;
    }

    *_out_size = (_count * D_INTERNAL_ZIP_PLAN_SIZE) + largest;

    return D_PACK_STATUS_OK;
}

/*
d_internal_zip_local_pass
  The first pass: each member's local header and data.  With a stage, each
member's plan is kept there for the central pass.

Parameter(s):
  _entries:   the members.
  _count:     how many.
  _options:   a resolved option set.
  _stage:     the stage, or null.
  _stage_cap: its size; at least d_internal_zip_stage_size when given.
  _sink:      the destination.
  _out_end:   receives the offset after the last member.
Return:
  D_PACK_STATUS_OK, or the first failure.
*/
static enum dj_pack_status
d_internal_zip_local_pass(
    const struct dj_archive_entry*   _entries,
    size_t                          _count,
    const struct dj_archive_options* _options,
    unsigned char*                  _stage,
    size_t                          _stage_cap,
    struct dj_pack_sink              _sink,
    size_t*                         _out_end
)
{
    unsigned char*             scratch = NULL;
    size_t                     offset  = 0u;
    struct dj_internal_zip_plan plan;
    enum dj_pack_status         status;
    size_t                     i;

    if (_stage != NULL)
    {
        scratch = _stage + (_count * D_INTERNAL_ZIP_PLAN_SIZE);
    }

    for (i = 0; i < _count; ++i)
    {
        const struct dj_archive_entry* e = &_entries[i];

        if (!d_archive_entry_is_valid(e))
        {
            return D_PACK_STATUS_INVALID_ARGUMENT;
        }

        status = (scratch != NULL)
            ? d_internal_zip_entry_plan_staged(
                  e, _options, scratch,
                  _stage_cap - (_count * D_INTERNAL_ZIP_PLAN_SIZE), &plan)
            : d_internal_zip_entry_plan(e, _options, &plan);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }
        if (!d_internal_zip_fits(e, &plan, offset))
        {
            return D_PACK_STATUS_UNSUPPORTED;
        }
        if (_stage != NULL)
        {
            memcpy(_stage + (i * D_INTERNAL_ZIP_PLAN_SIZE), &plan,
                   D_INTERNAL_ZIP_PLAN_SIZE);
        }

        status = d_internal_zip_emit_local(_sink, e, _options, &plan,
                                           scratch);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }

        offset += D_INTERNAL_ZIP_LOCAL_SIZE + e->name.length + plan.slash +
                  plan.csize;
    }

    *_out_end = offset;

    return D_PACK_STATUS_OK;
}

/*
d_internal_zip_central_pass
  The second pass: the central directory, one record per member, from the
plans kept in the stage or decided again.

Parameter(s):
  _entries:  the members.
  _count:    how many.
  _options:  a resolved option set.
  _stage:    the stage the first pass kept plans in, or null.
  _sink:     the destination.
  _out_size: receives the directory's size.
Return:
  D_PACK_STATUS_OK, or the first failure.
*/
static enum dj_pack_status
d_internal_zip_central_pass(
    const struct dj_archive_entry*   _entries,
    size_t                          _count,
    const struct dj_archive_options* _options,
    const unsigned char*            _stage,
    struct dj_pack_sink              _sink,
    size_t*                         _out_size
)
{
    size_t                     offset = 0u;
    size_t                     size   = 0u;
    struct dj_internal_zip_plan plan;
    enum dj_pack_status         status = D_PACK_STATUS_OK;
    size_t                     i;

    for (i = 0; i < _count; ++i)
    {
        const struct dj_archive_entry* e = &_entries[i];

        if (_stage != NULL)
        {
            memcpy(&plan, _stage + (i * D_INTERNAL_ZIP_PLAN_SIZE),
                   D_INTERNAL_ZIP_PLAN_SIZE);
        }
        else
        {
            status = d_internal_zip_entry_plan(e, _options, &plan);
        }
        if (status == D_PACK_STATUS_OK)
        {
            status = d_internal_zip_emit_central(_sink, e, _options, &plan,
                                                 offset);
        }
        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }

        size   += D_INTERNAL_ZIP_CENTRAL_SIZE + e->name.length + plan.slash;
        offset += D_INTERNAL_ZIP_LOCAL_SIZE + e->name.length + plan.slash +
                  plan.csize;
    }

    *_out_size = size;

    return D_PACK_STATUS_OK;
}

/*
d_internal_zip_create
  Write a complete ZIP archive to a sink: the members' local headers and
data, the central directory, and the end-of-central-directory record.

Parameter(s):
  _entries:   the members, in order; may be null when _count is 0.
  _count:     how many.
  _options:   a resolved option set.
  _stage:     a stage of at least d_internal_zip_stage_size, or null.
  _stage_cap: its size.
  _sink:      the destination.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_UNSUPPORTED for what the classic records
cannot hold, or the first failure encountered.
*/
static enum dj_pack_status
d_internal_zip_create(
    const struct dj_archive_entry*   _entries,
    size_t                          _count,
    const struct dj_archive_options* _options,
    void*                           _stage,
    size_t                          _stage_cap,
    struct dj_pack_sink              _sink
)
{
    unsigned char      hdr[D_INTERNAL_ZIP_EOCD_SIZE];
    size_t             cd_offset = 0u;
    size_t             cd_size   = 0u;
    enum dj_pack_status status;

    if ( (_count > 0xFFFFu) ||
         (_options->comment.length > 0xFFFFu) )
    {
        return D_PACK_STATUS_UNSUPPORTED;
    }

    status = d_internal_zip_local_pass(_entries, _count, _options,
                                       (unsigned char*)_stage, _stage_cap,
                                       _sink, &cd_offset);

    if (status == D_PACK_STATUS_OK)
    {
        status = d_internal_zip_central_pass(_entries, _count, _options,
                                             (const unsigned char*)_stage,
                                             _sink, &cd_size);
    }
    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }
    if ( ((uint64_t)cd_offset > 0xFFFFFFFFu) ||
         ((uint64_t)cd_size > 0xFFFFFFFFu) )
    {
        return D_PACK_STATUS_UNSUPPORTED;
    }

    memset(hdr, 0, sizeof(hdr));
    memcpy(hdr, "PK\005\006", 4);
    d_internal_put_le16(hdr + 8,  (uint32_t)_count);
    d_internal_put_le16(hdr + 10, (uint32_t)_count);
    d_internal_put_le32(hdr + 12, (uint32_t)cd_size);
    d_internal_put_le32(hdr + 16, (uint32_t)cd_offset);
    d_internal_put_le16(hdr + 20, (uint32_t)_options->comment.length);

    status = d_internal_emit(_sink, hdr, D_INTERNAL_ZIP_EOCD_SIZE);

    if ( (status == D_PACK_STATUS_OK) &&
         (_options->comment.length != 0u) )
    {
        status = d_internal_emit(_sink, _options->comment.data,
                                 _options->comment.length);
    }

    return status;
}

/*
d_internal_zip_find_eocd
  Locate the end-of-central-directory record.

  Scanned BACKWARDS from the end, because the record is last and its trailing
comment is variable-length -- there is no forward path to it.  The scan is
bounded by the maximum comment length (65535) plus the record itself, which is
what stops a corrupt file turning the search into a walk of the whole archive.

Parameter(s):
  _in:      the archive bytes.
  _in_size: how many.
  _out_off: receives the record's offset.
Return:
  1 when the record was found, 0 otherwise.
*/
static
int d_internal_zip_find_eocd(
    const unsigned char* _in,
    size_t               _in_size,
    size_t*              _out_off
)
{
    size_t limit;
    size_t i;

    if (_in_size < D_INTERNAL_ZIP_EOCD_SIZE)
    {
        return 0;
    }

    limit = D_INTERNAL_ZIP_EOCD_SIZE + 65535u;

    if (limit > _in_size)
    {
        limit = _in_size;
    }

    for (i = D_INTERNAL_ZIP_EOCD_SIZE; i <= limit; ++i)
    {
        const unsigned char* p = _in + (_in_size - i);

        if ( (p[0] == 'P')   && (p[1] == 'K') &&
             (p[2] == 0x05u) && (p[3] == 0x06u) )
        {
            *_out_off = _in_size - i;

            return 1;
        }
    }

    return 0;
}

// dj_internal_zip_record
//   struct: one central directory record, read and checked: where its name
// is, where its data starts, its sizes, method, CRC and attributes, and how
// long the record is.
struct dj_internal_zip_record
{
    const unsigned char* name;
    size_t               name_len;
    size_t               length;
    size_t               data_off;
    size_t               csize;
    size_t               usize;
    uint32_t             method;
    uint32_t             crc;
    uint32_t             attr;
};

/*
d_internal_zip_directory
  Find the central directory: its record count and where it starts.

Parameter(s):
  _p:         the archive bytes.
  _in_size:   how many.
  _out_total: receives the record count.
  _out_pos:   receives the directory's offset.
Return:
  D_PACK_STATUS_OK, or D_PACK_STATUS_CORRUPT_INPUT.
*/
static enum dj_pack_status
d_internal_zip_directory(
    const unsigned char* _p,
    size_t               _in_size,
    size_t*              _out_total,
    size_t*              _out_pos
)
{
    size_t eocd = 0;

    if (!d_internal_zip_find_eocd(_p, _in_size, &eocd))
    {
        return D_PACK_STATUS_CORRUPT_INPUT;
    }

    *_out_total = (size_t)d_internal_get_le16(_p + eocd + 10);
    *_out_pos   = (size_t)d_internal_get_le32(_p + eocd + 16);

    return (*_out_pos > _in_size) ? D_PACK_STATUS_CORRUPT_INPUT
                                  : D_PACK_STATUS_OK;
}

/*
d_internal_zip_record_read
  Read and check the central record at _pos, and the local header it points
at.

  Every offset is checked against the input before it is followed.  The local
header's own name and extra lengths decide where the data starts; they may
differ from the directory's.  A method this reader does not know is refused
here, in the measure pass as in the extract pass, rather than let the codec
lookup's fallback (store) copy compressed bytes out as though they were the
payload.

Parameter(s):
  _p:       the archive bytes.
  _in_size: how many.
  _pos:     the record's offset.
  _rec:     receives the record.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_UNSUPPORTED, or a formal failure.
*/
static enum dj_pack_status
d_internal_zip_record_read(
    const unsigned char*           _p,
    size_t                         _in_size,
    size_t                         _pos,
    struct dj_internal_zip_record* _rec
)
{
    const unsigned char* c = _p + _pos;
    size_t               local;

    if ( (_pos > _in_size) ||
         ((_in_size - _pos) < D_INTERNAL_ZIP_CENTRAL_SIZE) ||
         (memcmp(c, "PK\001\002", 4) != 0) )
    {
        return D_PACK_STATUS_CORRUPT_INPUT;
    }

    _rec->method   = d_internal_get_le16(c + 10);
    _rec->crc      = d_internal_get_le32(c + 16);
    _rec->csize    = (size_t)d_internal_get_le32(c + 20);
    _rec->usize    = (size_t)d_internal_get_le32(c + 24);
    _rec->name_len = (size_t)d_internal_get_le16(c + 28);
    _rec->attr     = d_internal_get_le32(c + 38);
    _rec->name     = c + D_INTERNAL_ZIP_CENTRAL_SIZE;
    _rec->length   = D_INTERNAL_ZIP_CENTRAL_SIZE + _rec->name_len +
                     (size_t)d_internal_get_le16(c + 30) +
                     (size_t)d_internal_get_le16(c + 32);
    local          = (size_t)d_internal_get_le32(c + 42);

    if ( ((_in_size - _pos) < _rec->length) ||
         (local > _in_size) ||
         ((_in_size - local) < D_INTERNAL_ZIP_LOCAL_SIZE) )
    {
        return D_PACK_STATUS_CORRUPT_INPUT;
    }
    if ( (_rec->method != (uint32_t)D_ZIP_METHOD_STORE) &&
         (d_internal_zip_codec_for_method((int32_t)_rec->method) ==
          D_CODEC_ID_STORE) )
    {
        return D_PACK_STATUS_UNSUPPORTED;
    }

    _rec->data_off = local + D_INTERNAL_ZIP_LOCAL_SIZE +
                     (size_t)d_internal_get_le16(_p + local + 26) +
                     (size_t)d_internal_get_le16(_p + local + 28);

    if ( (_rec->data_off > _in_size) ||
         ((_in_size - _rec->data_off) < _rec->csize) )
    {
        return D_PACK_STATUS_TRUNCATED_INPUT;
    }

    return D_PACK_STATUS_OK;
}

/*
d_internal_zip_unpack
  Decode one member's data into _dst, and check it against the record: the
size it declares and the CRC of the uncompressed bytes.  A mismatch is
CORRUPT_INPUT, so damage the codec cannot see -- a stored member's bytes, a
flipped bit a codec decodes without complaint -- is still reported.

Parameter(s):
  _p:   the archive bytes.
  _rec: the member's record.
  _dst: at least _rec->usize bytes.
Return:
  D_PACK_STATUS_OK, or the failure.
*/
static enum dj_pack_status
d_internal_zip_unpack(
    const unsigned char*                 _p,
    const struct dj_internal_zip_record* _rec,
    unsigned char*                       _dst
)
{
    size_t             got    = _rec->csize;
    enum dj_pack_status status = D_PACK_STATUS_OK;

    if (_rec->usize == 0u)
    {
        return (_rec->crc == 0u) ? D_PACK_STATUS_OK
                                 : D_PACK_STATUS_CORRUPT_INPUT;
    }
    if (_rec->method == (uint32_t)D_ZIP_METHOD_STORE)
    {
        if (_rec->csize != _rec->usize)
        {
            return D_PACK_STATUS_CORRUPT_INPUT;
        }

        memcpy(_dst, _p + _rec->data_off, _rec->csize);
    }
    else
    {
        status = d_decompress(
            d_internal_zip_codec_for_method((int32_t)_rec->method),
            _p + _rec->data_off, _rec->csize, _dst, _rec->usize, &got);

        // more output than the record declares is damage, not a short buffer
        if (status == D_PACK_STATUS_BUFFER_TOO_SMALL)
        {
            status = D_PACK_STATUS_CORRUPT_INPUT;
        }
    }
    if ( (status == D_PACK_STATUS_OK) &&
         ( (got != _rec->usize) ||
           (d_pack_crc32(0u, _dst, got) != _rec->crc) ) )
    {
        status = D_PACK_STATUS_CORRUPT_INPUT;
    }

    return status;
}

/*
d_internal_zip_walk
  Walk a ZIP's central directory, reporting each member.

  Serves both the measure pass and the extract pass, exactly as the tar walk
does, so the two cannot disagree.  Members are read through the CENTRAL
directory rather than by scanning local headers: the directory is the
archive's authoritative index, and a local-header scan cannot distinguish a
real member from the bytes of a stored member that happens to contain a ZIP.

Parameter(s):
  _in:         the archive bytes.
  _in_size:    how many.
  _entries:    receives the members; may be null to measure only.
  _entry_cap:  how many entries _entries holds.
  _arena:      receives names and payloads; may be null to measure only.
  _arena_size: how many bytes _arena holds.
  _out:        receives the required entry count and arena size.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL, or a formal status.
*/
static enum dj_pack_status
d_internal_zip_walk(
    const void*              _in,
    size_t                   _in_size,
    struct dj_archive_entry*  _entries,
    size_t                   _entry_cap,
    void*                    _arena,
    size_t                   _arena_size,
    struct dj_archive_layout* _out
)
{
    const unsigned char*         p        = (const unsigned char*)_in;
    unsigned char*               arena    = (unsigned char*)_arena;
    int                          writing  = (_entries != NULL) &&
                                            (arena != NULL);
    int                          overflow = !writing &&
                                            ( (_entries != NULL) ||
                                              (arena != NULL) );
    size_t                       total    = 0u;
    size_t                       pos      = 0u;
    size_t                       used     = 0u;
    size_t                       i;
    struct dj_internal_zip_record rec;
    enum dj_pack_status           status;

    _out->entry_count = 0u;
    _out->arena_size  = 0u;
    status            = d_internal_zip_directory(p, _in_size, &total, &pos);

    for (i = 0; (i < total) && (status == D_PACK_STATUS_OK); ++i)
    {
        status = d_internal_zip_record_read(p, _in_size, pos, &rec);

        if (status != D_PACK_STATUS_OK)
        {
            break;
        }
        // a member the address space cannot hold is not in this input
        if ( (rec.usize > ((size_t)-1 - used)) ||
             (rec.name_len > ((size_t)-1 - used - rec.usize)) )
        {
            status = D_PACK_STATUS_CORRUPT_INPUT;

            break;
        }
        if ( writing &&
             ( (i >= _entry_cap) ||
               ((rec.name_len + rec.usize) > (_arena_size - used)) ) )
        {
            writing  = 0;
            overflow = 1;
        }
        if (writing)
        {
            struct dj_archive_entry* e = &_entries[i];

            memcpy(arena + used, rec.name, rec.name_len);
            e->name.data    = (const char*)(arena + used);
            e->name.length  = rec.name_len;
            e->data.data    = (rec.usize != 0u)
                            ? (arena + used + rec.name_len) : NULL;
            e->data.size    = rec.usize;
            // a name ending in '/' is the format's way of saying
            // "directory"; there is no type flag in a ZIP record.
            e->is_directory = ( (rec.name_len != 0u) &&
                                (rec.name[rec.name_len - 1u] == '/') );
            e->mode         = (rec.attr >> 16) & 07777u;
            e->mtime        = 0;
            status          = d_internal_zip_unpack(
                                  p, &rec, arena + used + rec.name_len);
        }

        used += rec.name_len + rec.usize;
        pos  += rec.length;
    }

    _out->entry_count = i;
    _out->arena_size  = used;

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    return overflow ? D_PACK_STATUS_BUFFER_TOO_SMALL : D_PACK_STATUS_OK;
}

/*
d_internal_zip_find
  Copy the first member named _name out of a ZIP, or measure it.

Parameter(s):
  _in:           the archive bytes.
  _in_size:      how many.
  _name:         the member's name.
  _out:          the destination; null with _out_capacity 0 measures.
  _out_capacity: how many bytes _out holds.
  _out_size:     receives the member's size.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL, a formal failure, or
D_PACK_STATUS_INVALID_ARGUMENT when no member has that name.
*/
static enum dj_pack_status
d_internal_zip_find(
    const void*         _in,
    size_t              _in_size,
    struct dj_pack_text _name,
    void*               _out,
    size_t              _out_capacity,
    size_t*             _out_size
)
{
    const unsigned char*         p     = (const unsigned char*)_in;
    size_t                       total = 0u;
    size_t                       pos   = 0u;
    size_t                       i;
    struct dj_internal_zip_record rec;
    enum dj_pack_status           status;

    status = d_internal_zip_directory(p, _in_size, &total, &pos);

    for (i = 0; (i < total) && (status == D_PACK_STATUS_OK); ++i)
    {
        status = d_internal_zip_record_read(p, _in_size, pos, &rec);

        if (status != D_PACK_STATUS_OK)
        {
            break;
        }
        if ( (rec.name_len != _name.length) ||
             (memcmp(rec.name, _name.data, rec.name_len) != 0) )
        {
            pos += rec.length;

            continue;
        }

        *_out_size = rec.usize;

        if ( (_out == NULL) &&
             (_out_capacity == 0u) )
        {
            return D_PACK_STATUS_OK;
        }
        if (_out_capacity < rec.usize)
        {
            return D_PACK_STATUS_BUFFER_TOO_SMALL;
        }

        return d_internal_zip_unpack(p, &rec, (unsigned char*)_out);
    }

    return (status != D_PACK_STATUS_OK) ? status
                                        : D_PACK_STATUS_INVALID_ARGUMENT;
}

// =============================================================================
// X.   TRANSFORMS
// =============================================================================
//   The public leaves.  Each validates, resolves options once, dispatches to a
// writer or a walker, and adapts between the sink and buffer forms.  No format
// knowledge lives here; no buffer-protocol knowledge lives in a writer.

/*
d_internal_archive_buffer_write
  A sink write over a fixed caller buffer, recording the full requirement even
after it overflows.  The archive counterpart of compression's buffer sink; it
is duplicated rather than shared because the compression one is static to its
own translation unit, and exporting it would put a sink implementation in the
public surface where a caller could bind to it.

Parameter(s):
  _context: a struct dj_pack_buffer_sink.
  _data:    the bytes offered.
  _size:    how many.
Return:
  _size always; the caller checks `overflow`.
*/
static
size_t d_internal_archive_buffer_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct dj_pack_buffer_sink* b = (struct dj_pack_buffer_sink*)_context;

    if (b == NULL)
    {
        return 0;
    }

    b->needed += _size;

    if ( (b->buffer != NULL) &&
         ((b->written + _size) <= b->capacity) )
    {
        if (_size != 0u)
        {
            memcpy(b->buffer + b->written, _data, _size);
        }

        b->written += _size;
    }
    else if (_size != 0u)
    {
        b->overflow = 1;
    }

    return _size;
}

// dj_internal_tee
//   struct: the context of a sink that counts the bytes passing through it and
// forwards them to an inner sink.
struct dj_internal_tee
{
    struct dj_pack_sink  inner;
    size_t              total;
};

/*
d_internal_tee_write
  A sink write that counts the bytes and forwards them to an inner sink.

  This is how a writer reports the size it produced without requiring the
caller's sink to be countable -- a file, a socket and a growable buffer are all
legitimate destinations and none of them is obliged to keep a total.

Parameter(s):
  _context: a struct dj_internal_tee.
  _data:    the bytes offered.
  _size:    how many.
Return:
  What the inner sink accepted, so a short write still propagates.
*/
static
size_t d_internal_tee_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct dj_internal_tee* t = (struct dj_internal_tee*)_context;
    size_t                 taken;

    if ( (t == NULL) ||
         (t->inner.write == NULL) )
    {
        return 0;
    }

    taken     = t->inner.write(t->inner.context, _data, _size);
    t->total += taken;

    return taken;
}

/*
d_internal_archive_prepare
  The checks every create and extract call runs before doing anything.

Parameter(s):
  _format:  the format requested.
  _writing: 1 for a create call, 0 for a read call.
  _in:      the caller's options; may be null.
  _out:     receives the resolved options.
Return:
  D_PACK_STATUS_OK, or the formal status that stops the call.
*/
static enum dj_pack_status
d_internal_archive_prepare(
    enum dj_format_id                _format,
    int                             _writing,
    const struct dj_archive_options* _in,
    struct dj_archive_options*       _out
)
{
    enum dj_pack_status status;

    if (!D_FORMAT_ID_IS_VALID(_format))
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }
    if (_writing && !d_format_is_writable(_format))
    {
        return D_PACK_STATUS_UNAVAILABLE;
    }
    if (!_writing && !d_format_is_readable(_format))
    {
        return D_PACK_STATUS_UNAVAILABLE;
    }

    status = d_archive_options_resolve(_format, _in, _out);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    return d_archive_options_validate(_format, _out, NULL);
}

/*
d_internal_tar_gz_create
  Write a tarball through a stage: the tar into the stage, then gzip from the
stage into the sink.

  The two transforms cannot be fused in one pass -- that needs a streaming
tar feeding a streaming codec input, and the codec leaves consume a whole
buffer -- so the tar has to live somewhere between them.  The stage is that
place, and the caller names it, which is what lets the C face write tar.gz
with no allocator.

Parameter(s):
  _entries:   the members; may be null when _count is 0.
  _count:     how many.
  _options:   a resolved option set.
  _stage:     at least the tar's size (d_archive_stage_size).
  _stage_cap: its size.
  _sink:      the destination.
Return:
  D_PACK_STATUS_OK, or the first failure.
*/
static enum dj_pack_status
d_internal_tar_gz_create(
    const struct dj_archive_entry*   _entries,
    size_t                          _count,
    const struct dj_archive_options* _options,
    void*                           _stage,
    size_t                          _stage_cap,
    struct dj_pack_sink              _sink
)
{
    struct dj_pack_buffer_sink tar;
    struct dj_pack_sink        sink;
    enum dj_pack_status        status;

    d_pack_buffer_sink_init(&tar, _stage, _stage_cap);
    sink.write   = &d_internal_archive_buffer_write;
    sink.context = &tar;
    status       = d_internal_tar_create(_entries, _count, _options, sink);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }
    if (tar.overflow)
    {
        // the stage was sized for this tar; a writer that disagrees with
        // its own measure is a fault, not a caller's mistake
        return D_PACK_STATUS_BACKEND_ERROR;
    }

    return d_compress_to_sink(D_CODEC_ID_GZIP, _stage, tar.written,
                              &_options->codec, _sink, NULL);
}

/*
d_internal_archive_stage_need
  The stage a create call in this format needs (tar.gz) or can use (ZIP); 0
for the formats that use none.

Parameter(s):
  _format:   the format.
  _entries:  the members; may be null when _count is 0.
  _count:    how many.
  _options:  a resolved option set.
  _out_need: receives the size.
Return:
  D_PACK_STATUS_OK, or a formal failure found while sizing.
*/
static enum dj_pack_status
d_internal_archive_stage_need(
    enum dj_format_id                _format,
    const struct dj_archive_entry*   _entries,
    size_t                          _count,
    const struct dj_archive_options* _options,
    size_t*                         _out_need
)
{
    *_out_need = 0u;

    switch (_format)
    {
        case D_FORMAT_ID_TAR_GZ:
        {
            return d_internal_tar_size(_entries, _count, _options,
                                       _out_need);
        }
        case D_FORMAT_ID_ZIP:
        {
            if (_count > 0xFFFFu)
            {
                return D_PACK_STATUS_UNSUPPORTED;
            }

            return d_internal_zip_stage_size(_entries, _count, _options,
                                             _out_need);
        }
        default:
        {
            return D_PACK_STATUS_OK;
        }
    }
}

/*
d_internal_archive_write
  Dispatch a create to its writer.  No format knowledge lives above this;
no buffer-protocol knowledge lives below it.

Parameter(s):
  _format:    the format.
  _entries:   the members; may be null when _count is 0.
  _count:     how many.
  _options:   a resolved option set.
  _stage:     a stage of at least the format's need, or null.
  _stage_cap: its size.
  _sink:      the destination.
Return:
  D_PACK_STATUS_OK, or the writer's failure.  tar.gz with no stage is
D_PACK_STATUS_UNSUPPORTED: d_archive_create_staged is the call for it.
*/
static enum dj_pack_status
d_internal_archive_write(
    enum dj_format_id                _format,
    const struct dj_archive_entry*   _entries,
    size_t                          _count,
    const struct dj_archive_options* _options,
    void*                           _stage,
    size_t                          _stage_cap,
    struct dj_pack_sink              _sink
)
{
    switch (_format)
    {
        case D_FORMAT_ID_TAR:
        {
            return d_internal_tar_create(_entries, _count, _options, _sink);
        }
        case D_FORMAT_ID_ZIP:
        {
            return d_internal_zip_create(_entries, _count, _options,
                                         _stage, _stage_cap, _sink);
        }
        case D_FORMAT_ID_GZ:
        {
            // a single-member container IS the codec, with the entry's
            // payload as the whole stream.  There is no directory to write.
            if (!d_archive_entry_is_valid(&_entries[0]))
            {
                return D_PACK_STATUS_INVALID_ARGUMENT;
            }

            return d_compress_to_sink(D_CODEC_ID_GZIP,
                                      _entries[0].data.data,
                                      _entries[0].data.size,
                                      &_options->codec, _sink, NULL);
        }
        case D_FORMAT_ID_TAR_GZ:
        {
            return (_stage == NULL)
                ? D_PACK_STATUS_UNSUPPORTED
                : d_internal_tar_gz_create(_entries, _count, _options,
                                           _stage, _stage_cap, _sink);
        }
        default:
        {
            return D_PACK_STATUS_UNAVAILABLE;
        }
    }
}

/*
d_internal_create_checked
  Every create call's path, sink form: check the arguments, resolve and
validate the options, check the stage, and write through a tee that counts.

Parameter(s):
  _format:    the container format.
  _entries:   the members; may be null when _count is 0.
  _count:     how many.
  _options:   the tuning; null requests the core's pinned defaults.
  _staged:    1 for the staged calls, which check the stage.
  _stage:     the stage; may be null when _stage_cap is 0.
  _stage_cap: its size.
  _sink:      the destination.
  _out_size:  receives the bytes produced; may be null.
Return:
  D_PACK_STATUS_OK, or a status describing the failure.
*/
static enum dj_pack_status
d_internal_create_checked(
    enum dj_format_id                _format,
    const struct dj_archive_entry*   _entries,
    size_t                          _count,
    const struct dj_archive_options* _options,
    int                             _staged,
    void*                           _stage,
    size_t                          _stage_cap,
    struct dj_pack_sink              _sink,
    size_t*                         _out_size
)
{
    struct dj_archive_options resolved;
    struct dj_pack_sink       counted;
    struct dj_internal_tee    tally;
    size_t                    need = 0u;
    enum dj_pack_status       status;

    if ( (_sink.write == NULL) ||
         ((_entries == NULL) && (_count != 0u)) ||
         ((_stage == NULL) && (_stage_cap != 0u)) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    status = d_internal_archive_prepare(_format, 1, _options, &resolved);

    // gz carries exactly one member and has no directory, so anything other
    // than a single entry is a formal error rather than a truncation.
    if ( (status == D_PACK_STATUS_OK) &&
         D_FORMAT_ID_IS_SINGLE_STREAM(_format) &&
         (_count != 1u) )
    {
        status = D_PACK_STATUS_INVALID_ARGUMENT;
    }
    if ( (status == D_PACK_STATUS_OK) &&
         _staged )
    {
        status = d_internal_archive_stage_need(_format, _entries, _count,
                                               &resolved, &need);
    }
    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }
    // a stage shorter than d_archive_stage_size reported is a malformed
    // call, not a short output: BUFFER_TOO_SMALL keeps its one meaning
    if (_stage_cap < need)
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    tally.inner     = _sink;
    tally.total     = 0u;
    counted.write   = &d_internal_tee_write;
    counted.context = &tally;
    status          = d_internal_archive_write(
                          _format, _entries, _count, &resolved,
                          (need != 0u) ? _stage : NULL, _stage_cap, counted);

    if ( (status == D_PACK_STATUS_OK) &&
         (_out_size != NULL) )
    {
        *_out_size = tally.total;
    }

    return status;
}

/*
d_internal_create_buffer
  Every create call's path, buffer form: the sink form over a caller buffer,
under the two-call protocol.

Parameter(s):
  As d_internal_create_checked, with _out, _out_capacity and _out_size (which
must not be null) in place of the sink.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL with the requirement in
*_out_size, or a status describing the failure.
*/
static enum dj_pack_status
d_internal_create_buffer(
    enum dj_format_id                _format,
    const struct dj_archive_entry*   _entries,
    size_t                          _count,
    const struct dj_archive_options* _options,
    int                             _staged,
    void*                           _stage,
    size_t                          _stage_cap,
    void*                           _out,
    size_t                          _out_capacity,
    size_t*                         _out_size
)
{
    struct dj_pack_buffer_sink dest;
    struct dj_pack_sink        sink;
    enum dj_pack_status        status;

    if (_out_size == NULL)
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    d_pack_buffer_sink_init(&dest, _out, _out_capacity);
    sink.write   = &d_internal_archive_buffer_write;
    sink.context = &dest;
    status       = d_internal_create_checked(_format, _entries, _count,
                                             _options, _staged, _stage,
                                             _stage_cap, sink, NULL);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    *_out_size = dest.needed;

    // a MEASURE call asked only for that number and has succeeded; an
    // overflow is a failure only when the caller offered somewhere to put
    // the bytes.
    if ( (_out == NULL) &&
         (_out_capacity == 0u) )
    {
        return D_PACK_STATUS_OK;
    }

    return dest.overflow ? D_PACK_STATUS_BUFFER_TOO_SMALL : D_PACK_STATUS_OK;
}

/*
d_archive_stage_size
  How big a stage the staged create calls need for these members.

  tar.gz needs one the size of its tar: the tar is written there and
compressed from there.  ZIP can use one -- a kept plan per member and room
for the largest member's compressed bytes -- and then compresses each member
once instead of three times.  Every other format needs none, and reports 0.
Sizing runs no codec.

Parameter(s):
  _format:         the container format.
  _entries:        the members; may be null when _entry_count is 0.
  _entry_count:    how many.
  _options:        the tuning; null requests the core's pinned defaults.
  _out_stage_size: receives the size.
Return:
  D_PACK_STATUS_OK, or a status describing the failure.
*/
enum dj_pack_status
d_archive_stage_size(
    enum dj_format_id                _format,
    const struct dj_archive_entry*   _entries,
    size_t                          _entry_count,
    const struct dj_archive_options* _options,
    size_t*                         _out_stage_size
)
{
    struct dj_archive_options resolved;
    enum dj_pack_status       status;

    if ( (_out_stage_size == NULL) ||
         ((_entries == NULL) && (_entry_count != 0u)) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    status = d_internal_archive_prepare(_format, 1, _options, &resolved);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    return d_internal_archive_stage_need(_format, _entries, _entry_count,
                                         &resolved, _out_stage_size);
}

/*
d_archive_create_to_sink
  Write a container to a sink.

  An empty entry list is legal for every multi-member format and produces a
valid empty container -- document_bundle already relies on that, passing a
null base rather than indexing an empty vector.  It is a formal error only
for gz, which has no directory to be empty.  tar.gz needs a stage, and so
d_archive_create_staged_to_sink; here it is D_PACK_STATUS_UNSUPPORTED.

Parameter(s):
  _format:      the container format.
  _entries:     the members, in order; may be null when _entry_count is 0.
  _entry_count: how many.
  _options:     the tuning; null requests the core's pinned defaults.
  _sink:        the destination.
  _out_size:    receives the number of bytes produced; may be null.
Return:
  D_PACK_STATUS_OK, or a status describing the failure.
*/
enum dj_pack_status
d_archive_create_to_sink(
    enum dj_format_id                _format,
    const struct dj_archive_entry*   _entries,
    size_t                          _entry_count,
    const struct dj_archive_options* _options,
    struct dj_pack_sink              _sink,
    size_t*                         _out_size
)
{
    return d_internal_create_checked(_format, _entries, _entry_count,
                                     _options, 0, NULL, 0u, _sink, _out_size);
}

/*
d_archive_create_staged_to_sink
  Write a container to a sink, using a caller's stage: the call for tar.gz,
and for a ZIP that compresses each member once.

Parameter(s):
  _format:      the container format.
  _entries:     the members, in order; may be null when _entry_count is 0.
  _entry_count: how many.
  _options:     the tuning; null requests the core's pinned defaults.
  _stage:       at least d_archive_stage_size bytes; may be null when that
                is 0.
  _stage_cap:   its size.
  _sink:        the destination.
  _out_size:    receives the number of bytes produced; may be null.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_INVALID_ARGUMENT for a stage that is too
small, or a status describing the failure.
*/
enum dj_pack_status
d_archive_create_staged_to_sink(
    enum dj_format_id                _format,
    const struct dj_archive_entry*   _entries,
    size_t                          _entry_count,
    const struct dj_archive_options* _options,
    void*                           _stage,
    size_t                          _stage_cap,
    struct dj_pack_sink              _sink,
    size_t*                         _out_size
)
{
    return d_internal_create_checked(_format, _entries, _entry_count,
                                     _options, 1, _stage, _stage_cap,
                                     _sink, _out_size);
}

/*
d_archive_create
  Write a container into a caller-owned buffer under the two-call protocol.

Parameter(s):
  _format:       the container format.
  _entries:      the members; may be null when _entry_count is 0.
  _entry_count:  how many.
  _options:      the tuning; null requests the core's pinned defaults.
  _out:          the destination; null with _out_capacity 0 measures.
  _out_capacity: how many bytes _out holds.
  _out_size:     receives the produced or required size.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL with the requirement in
*_out_size, or a status describing the failure.
*/
enum dj_pack_status
d_archive_create(
    enum dj_format_id                _format,
    const struct dj_archive_entry*   _entries,
    size_t                          _entry_count,
    const struct dj_archive_options* _options,
    void*                           _out,
    size_t                          _out_capacity,
    size_t*                         _out_size
)
{
    return d_internal_create_buffer(_format, _entries, _entry_count,
                                    _options, 0, NULL, 0u,
                                    _out, _out_capacity, _out_size);
}

/*
d_archive_create_staged
  Write a container into a caller-owned buffer, using a caller's stage.

  The two regions are separate questions with separate answers: the stage
is sized beforehand by d_archive_stage_size, and one shorter than that is
D_PACK_STATUS_INVALID_ARGUMENT; the output follows the two-call protocol, so
D_PACK_STATUS_BUFFER_TOO_SMALL always means the output.

Parameter(s):
  _format:        the container format.
  _entries:       the members; may be null when _entry_count is 0.
  _entry_count:   how many.
  _options:       the tuning; null requests the core's pinned defaults.
  _stage:         the stage; may be null when its size is 0.
  _stage_cap:     its size.
  _out:           the destination; null with _out_capacity 0 measures.
  _out_capacity:  how many bytes _out holds.
  _out_size:      receives the produced or required size.
Return:
  As d_archive_create, and D_PACK_STATUS_INVALID_ARGUMENT for a short stage.
*/
enum dj_pack_status
d_archive_create_staged(
    enum dj_format_id                _format,
    const struct dj_archive_entry*   _entries,
    size_t                          _entry_count,
    const struct dj_archive_options* _options,
    void*                           _stage,
    size_t                          _stage_cap,
    void*                           _out,
    size_t                          _out_capacity,
    size_t*                         _out_size
)
{
    return d_internal_create_buffer(_format, _entries, _entry_count,
                                    _options, 1, _stage, _stage_cap,
                                    _out, _out_capacity, _out_size);
}

/*
d_internal_tar_bound
  An upper bound on a tar of these members: per member a header, a pax
extended header when the name may need one (any name over 100 bytes), and
the payload rounded up to a record; then the end marker.

Parameter(s):
  _entries: the members.
  _count:   how many.
Return:
  The bound.
*/
static
size_t d_internal_tar_bound(
    const struct dj_archive_entry* _entries,
    size_t                         _count
)
{
    size_t total = 2u * D_INTERNAL_TAR_BLOCK;
    size_t i;

    for (i = 0; i < _count; ++i)
    {
        size_t size = _entries[i].data.size;

        total += D_INTERNAL_TAR_BLOCK +
                 ((size + D_INTERNAL_TAR_BLOCK - 1u) / D_INTERNAL_TAR_BLOCK) *
                 D_INTERNAL_TAR_BLOCK;

        // a pax header (one record), then path, size and mtime records
        // padded to records: the path's length plus 64 covers the rest
        if (_entries[i].name.length > 100u)
        {
            total += 2u * D_INTERNAL_TAR_BLOCK + _entries[i].name.length;
        }
    }

    return total;
}

/*
d_archive_bound
  An upper bound on the size of a container built from these entries.

  Cheap and always safe.  Every format's per-entry overhead is a constant plus
the name, and the payload's worst case is the codec's compress bound -- which
exceeds the input for incompressible data, the case a caller sizing from the
payload total alone gets wrong.  A tar.gz is bounded as the codec's bound on
the tar's bound: the tar's headers and padding are inside the stream.

Parameter(s):
  _format:      the container format.
  _entries:     the members; may be null when _entry_count is 0.
  _entry_count: how many.
  _options:     the tuning; may be null.
  _out_bound:   receives the bound.
Return:
  D_PACK_STATUS_OK, or D_PACK_STATUS_INVALID_ARGUMENT.
*/
enum dj_pack_status
d_archive_bound(
    enum dj_format_id                _format,
    const struct dj_archive_entry*   _entries,
    size_t                          _entry_count,
    const struct dj_archive_options* _options,
    size_t*                         _out_bound
)
{
    size_t total = 0u;
    size_t i;

    (void)_options;

    if ( (_out_bound == NULL) ||
         (!D_FORMAT_ID_IS_VALID(_format)) ||
         ((_entries == NULL) && (_entry_count != 0u)) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }
    if ( (_format == D_FORMAT_ID_TAR) ||
         (_format == D_FORMAT_ID_TAR_GZ) )
    {
        total = d_internal_tar_bound(_entries, _entry_count);

        if (_format == D_FORMAT_ID_TAR_GZ)
        {
            return d_compress_bound(D_CODEC_ID_GZIP, total, NULL,
                                    _out_bound);
        }

        *_out_bound = total;

        return D_PACK_STATUS_OK;
    }

    for (i = 0; i < _entry_count; ++i)
    {
        size_t             payload = 0;
        enum dj_pack_status s       = d_compress_bound(
            d_format_default_codec(_format),
            _entries[i].data.size, NULL, &payload);

        if (s != D_PACK_STATUS_OK)
        {
            return s;
        }

        // ZIP: a local header and a directory record, each with the name
        total += (_format == D_FORMAT_ID_ZIP)
               ? (D_INTERNAL_ZIP_LOCAL_SIZE + D_INTERNAL_ZIP_CENTRAL_SIZE +
                  (2u * (_entries[i].name.length + 1u)) + payload)
               : (payload + _entries[i].name.length + 512u);
    }

    *_out_bound = total + ( (_format == D_FORMAT_ID_ZIP)
                            ? (D_INTERNAL_ZIP_EOCD_SIZE + 65535u)
                            : 1024u );

    return D_PACK_STATUS_OK;
}

/*
d_internal_gz_read
  Measure or extract a gz: one member, the whole stream, with no name.

Parameter(s):
  As d_internal_archive_read.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL, or the codec's failure.
*/
static enum dj_pack_status
d_internal_gz_read(
    const void*              _in,
    size_t                   _in_size,
    struct dj_archive_entry*  _entries,
    size_t                   _entry_cap,
    void*                    _arena,
    size_t                   _arena_size,
    struct dj_archive_layout* _out
)
{
    size_t             got = 0;
    enum dj_pack_status status;

    status            = d_decompress(D_CODEC_ID_GZIP, _in, _in_size,
                                     _arena, _arena_size, &got);
    _out->entry_count = 1u;
    _out->arena_size  = got;

    if ( (status != D_PACK_STATUS_OK) ||
         ( (_entries == NULL) &&
           (_arena == NULL) ) )
    {
        return status;
    }
    if ( (_entries == NULL) ||
         (_entry_cap < 1u) )
    {
        return D_PACK_STATUS_BUFFER_TOO_SMALL;
    }

    _entries[0].name.data    = NULL;
    _entries[0].name.length  = 0u;
    _entries[0].data.data    = _arena;
    _entries[0].data.size    = got;
    _entries[0].is_directory = 0;
    _entries[0].mode         = (uint32_t)D_ARCHIVE_MODE_FILE_DEFAULT;
    _entries[0].mtime        = 0;

    return D_PACK_STATUS_OK;
}

/*
d_internal_archive_read
  The measure and extract calls' shared path: check the arguments, prepare,
and walk the container in the format's reader.  Measure passes both regions
null.

Parameter(s):
  _format:     the container format.
  _in:         the archive bytes.
  _in_size:    how many.
  _options:    the tuning; may be null.
  _entries:    receives the members; may be null.
  _entry_cap:  how many _entries holds.
  _arena:      receives names and payloads; may be null.
  _arena_size: how many bytes _arena holds.
  _out:        receives the entry count and arena size; must not be null.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL, or a formal status.
*/
static enum dj_pack_status
d_internal_archive_read(
    enum dj_format_id                _format,
    const void*                     _in,
    size_t                          _in_size,
    const struct dj_archive_options* _options,
    struct dj_archive_entry*         _entries,
    size_t                          _entry_cap,
    void*                           _arena,
    size_t                          _arena_size,
    struct dj_archive_layout*        _out
)
{
    struct dj_archive_options     resolved;
    struct dj_internal_tar_reader reader;
    enum dj_pack_status           status;

    if ( (_out == NULL) ||
         ((_in == NULL) && (_in_size != 0u)) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    status = d_internal_archive_prepare(_format, 0, _options, &resolved);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    switch (_format)
    {
        case D_FORMAT_ID_TAR:
        case D_FORMAT_ID_TAR_GZ:
        {
            d_internal_tar_reader_init(&reader, _entries, _entry_cap,
                                       _arena, _arena_size);

            return d_internal_tar_read(_format, _in, _in_size, &reader, _out);
        }
        case D_FORMAT_ID_ZIP:
        {
            return d_internal_zip_walk(_in, _in_size, _entries, _entry_cap,
                                       _arena, _arena_size, _out);
        }
        case D_FORMAT_ID_GZ:
        {
            return d_internal_gz_read(_in, _in_size, _entries, _entry_cap,
                                      _arena, _arena_size, _out);
        }
        default:
        {
            return D_PACK_STATUS_UNAVAILABLE;
        }
    }
}

/*
d_archive_measure
  Report the two region sizes an extraction of this container needs.

Parameter(s):
  _format:     the container format.
  _in:         the archive bytes.
  _in_size:    how many.
  _options:    the tuning; may be null.
  _out_layout: receives the entry count and arena size; must not be null.
Return:
  D_PACK_STATUS_OK, or a formal status when the archive is malformed.
*/
enum dj_pack_status
d_archive_measure(
    enum dj_format_id                _format,
    const void*                     _in,
    size_t                          _in_size,
    const struct dj_archive_options* _options,
    struct dj_archive_layout*        _out_layout
)
{
    return d_internal_archive_read(_format, _in, _in_size, _options,
                                   NULL, 0u, NULL, 0u, _out_layout);
}

/*
d_archive_extract
  Read a container's members into caller-owned regions.

  Each filled entry's name and data spans point INTO _arena, and are valid
for exactly as long as it is.  Either region being too small yields
D_PACK_STATUS_BUFFER_TOO_SMALL with *_out_used carrying BOTH requirements, so
one retry suffices.  A tar.gz is read with nothing staged: the inflater's
output is pushed straight through the tar reader.

Parameter(s):
  _format:     the container format.
  _in:         the archive bytes.
  _in_size:    how many.
  _options:    the tuning; may be null.
  _entries:    receives the members.
  _entry_cap:  how many entries _entries holds.
  _arena:      receives names and payloads.
  _arena_size: how many bytes _arena holds.
  _out_used:   receives the required entry count and arena size.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL, or a formal status.
*/
enum dj_pack_status
d_archive_extract(
    enum dj_format_id                _format,
    const void*                     _in,
    size_t                          _in_size,
    const struct dj_archive_options* _options,
    struct dj_archive_entry*         _entries,
    size_t                          _entry_cap,
    void*                           _arena,
    size_t                          _arena_size,
    struct dj_archive_layout*        _out_used
)
{
    return d_internal_archive_read(_format, _in, _in_size, _options,
                                   _entries, _entry_cap,
                                   _arena, _arena_size, _out_used);
}

/*
d_archive_extract_entry
  Pull one member out by name without materialising the rest.

  The common case -- reading a single file from a container -- and the one
that needs no arena.  Implemented by walking to the named member and decoding
only it, so the cost is the directory walk plus that member, not the whole
archive (a tar.gz is inflated in full, to its trailer, so its CRC is
checked).

Parameter(s):
  _format:       the container format.
  _in:           the archive bytes.
  _in_size:      how many.
  _name:         the member's name, compared byte for byte; nonempty.
  _options:      the tuning; may be null.
  _out:          the destination; null with _out_capacity 0 measures.
  _out_capacity: how many bytes _out holds.
  _out_size:     receives the produced or required size.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL, or a formal status.
D_PACK_STATUS_INVALID_ARGUMENT when no member carries that name.
*/
enum dj_pack_status
d_archive_extract_entry(
    enum dj_format_id                _format,
    const void*                     _in,
    size_t                          _in_size,
    struct dj_pack_text              _name,
    const struct dj_archive_options* _options,
    void*                           _out,
    size_t                          _out_capacity,
    size_t*                         _out_size
)
{
    struct dj_archive_options resolved;
    enum dj_pack_status       status;

    if ( (_out_size == NULL) ||
         (_name.data == NULL) ||
         (_name.length == 0u) ||
         ((_in == NULL) && (_in_size != 0u)) ||
         ((_out == NULL) && (_out_capacity != 0u)) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    status = d_internal_archive_prepare(_format, 0, _options, &resolved);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    switch (_format)
    {
        case D_FORMAT_ID_TAR:
        case D_FORMAT_ID_TAR_GZ:
        {
            return d_internal_tar_find(_format, _in, _in_size, _name,
                                       _out, _out_capacity, _out_size);
        }
        case D_FORMAT_ID_ZIP:
        {
            return d_internal_zip_find(_in, _in_size, _name,
                                       _out, _out_capacity, _out_size);
        }
        default:
        {
            return D_PACK_STATUS_UNAVAILABLE;
        }
    }
}
