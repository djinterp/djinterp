/******************************************************************************
* djinterp [utility]                                         archive_common.c
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
* trailing data descriptor.  With no allocator there is nowhere to stage the
* compressed bytes, and data descriptors are handled poorly by several readers,
* so this writer takes the third option: it runs the codec twice per entry --
* once into a counting sink to learn the compressed size, once into the real
* sink -- and computes the CRC directly over the uncompressed input.
*
*   That is a deliberate, tiered cost: one extra encode per entry, paid to keep
* tier 0 allocation-free and the output maximally compatible.  The store
* method pays nothing, because its compressed size is its input size.  When
* d_string exists, staging becomes possible and the second pass can go.
*
*   REPRODUCIBILITY IS THE DEFAULT:
*   Nothing here reads the clock, the umask, the environment, or the locale.
* An entry with mtime 0 is written with D_ARCHIVE_EPOCH_DEFAULT, not "now", and
* mode 0 becomes a pinned constant, not the process umask.  Two runs over
* identical entries produce identical bytes on every machine -- which is the
* precondition for the differential test comparing archives at all.
*
*
* TABLE OF CONTENTS
* =================
* II.   FORMAT IDENTITY
* III.  THE KNOB TABLE
* IV.   OPTION OPERATIONS
* V.    CAPABILITY AND ENTRY VALIDITY
* VI.   SIGNATURES
* VII.  BYTE-ORDER PRIMITIVES
* VIII. THE BUILT-IN USTAR WRITER / READER
* IX.   THE BUILT-IN ZIP WRITER / READER
* X.    TRANSFORMS
*
*
* path:      /src/djinterp/core/util/archive_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.30
******************************************************************************/

// c
#include <string.h>
// djinterp
#include "./archive_common.h"


// D_ARCHIVE_EPOCH_DEFAULT
//   constant: the timestamp written for an entry whose mtime is 0.
// 1980-01-01T00:00:00Z, which is the ZIP format's own epoch and therefore the
// earliest instant both tar and zip can represent without a special case.
// Pinned rather than taken from the clock: a container that embeds "now" is a
// container two identical runs cannot be compared byte for byte.
#define D_ARCHIVE_EPOCH_DEFAULT     315532800L

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
//   table: the format names, indexed by d_format_id.
static const char* const d_internal_format_names[D_FORMAT_ID_COUNT] =
{
    "zip", "tar", "gz", "tar_gz", "sevenzip", "rar"
};

// d_internal_format_suffixes
//   table: the conventional file suffix for each format, indexed by
// d_format_id.
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
    enum d_format_id _format
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
    enum d_format_id _format
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
    enum d_format_id* _out_format
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
            *_out_format = (enum d_format_id)i;

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
    enum d_format_id* _out_format
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
            *_out_format = (enum d_format_id)i;

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
enum d_codec_id
d_format_default_codec(
    enum d_format_id _format
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
// d_archive_knob.  These are the strings a diff report prints and a config
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
    enum d_archive_knob _knob
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
    enum d_archive_knob _knob
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

/*
d_archive_knob_format
  Which format a knob configures.

Parameter(s):
  _knob: the knob to classify.
Return:
  The owning format.  A container-level knob applies to all of them and is
reported as ZIP, the first enumerator; a caller distinguishing the two cases
tests the index against D_ARCHIVE_KNOB_ZIP_METHOD.
*/
enum d_format_id
d_archive_knob_format(
    enum d_archive_knob _knob
)
{
    if ((int)_knob < (int)D_ARCHIVE_KNOB_ZIP_METHOD)
    {
        return D_FORMAT_ID_ZIP;
    }
    if ((int)_knob <= (int)D_ARCHIVE_KNOB_ZIP_PASSWORD)
    {
        return D_FORMAT_ID_ZIP;
    }
    if ((int)_knob <= (int)D_ARCHIVE_KNOB_TAR_NUMERIC_OWNER)
    {
        return D_FORMAT_ID_TAR;
    }
    if ((int)_knob <= (int)D_ARCHIVE_KNOB_GZ_ORIGINAL_NAME)
    {
        return D_FORMAT_ID_GZ;
    }
    if ((int)_knob <= (int)D_ARCHIVE_KNOB_SEVENZIP_PASSWORD)
    {
        return D_FORMAT_ID_SEVENZIP;
    }

    return D_FORMAT_ID_RAR;
}


// =============================================================================
// IV.  OPTION OPERATIONS
// =============================================================================

/*
d_internal_archive_knob_value
  Read a numeric container knob by index.

  A switch rather than an offset table, because d_archive_options is NOT
densely packed -- it mixes int32_t knobs with pointer-bearing spans, so the
index-to-offset arithmetic that works for d_compress_options does not apply
here.  The switch is the honest form; the header's assertions say why.

Parameter(s):
  _options: the set to read.
  _knob:    the knob to read.
Return:
  The value, or D_ARCHIVE_KNOB_UNSET for a text knob or unknown index.
*/
static int32_t
d_internal_archive_knob_value(
    const struct d_archive_options* _options,
    enum d_archive_knob             _knob
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
static struct d_pack_text
d_internal_archive_knob_text(
    const struct d_archive_options* _options,
    enum d_archive_knob             _knob
)
{
    struct d_pack_text empty;

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
void
d_archive_options_init(
    struct d_archive_options* _options
)
{
    struct d_archive_options fresh = D_ARCHIVE_OPTIONS_INIT;

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
int
d_archive_options_equal(
    const struct d_archive_options* _a,
    const struct d_archive_options* _b
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
        enum d_archive_knob k = (enum d_archive_knob)i;

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
int
d_archive_options_are_default(
    const struct d_archive_options* _options
)
{
    struct d_archive_options fresh = D_ARCHIVE_OPTIONS_INIT;

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
int
d_archive_options_codec_is_default(
    const struct d_archive_options* _options
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
    const struct d_archive_options* _a,
    const struct d_archive_options* _b,
    enum d_archive_knob*            _out_knobs,
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
        enum d_archive_knob k       = (enum d_archive_knob)i;
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
d_archive_options_resolve
  Produce a fully specified container option set.

  As with compression, the defaults are pinned here rather than left to a
backend, so that C and C++ hand any backend byte-identical parameters and two
builds with different libraries produce the same container.  Only knobs
belonging to _format are resolved; the rest pass through untouched.

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
enum d_pack_status
d_archive_options_resolve(
    enum d_format_id                _format,
    const struct d_archive_options* _in,
    struct d_archive_options*       _out
)
{
    struct d_archive_options work;
    enum d_codec_id          codec;
    enum d_pack_status       status;

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

    if (work.level == D_ARCHIVE_KNOB_UNSET)
    {
        work.level = D_COMPRESS_LEVEL_DEFAULT;
    }
    if (work.store_only == D_ARCHIVE_KNOB_UNSET)
    {
        work.store_only = 0;
    }
    if (work.preserve_permissions == D_ARCHIVE_KNOB_UNSET)
    {
        work.preserve_permissions = 1;
    }
    if (work.preserve_mtime == D_ARCHIVE_KNOB_UNSET)
    {
        work.preserve_mtime = 1;
    }

    switch (_format)
    {
        case D_FORMAT_ID_ZIP:
        {
            if (work.zip.method == D_ARCHIVE_KNOB_UNSET)
            {
                // fall back to store when no DEFLATE provider exists, so a
                // zero-dependency build still writes a valid, readable ZIP.
                work.zip.method = d_codec_can_compress(D_CODEC_ID_DEFLATE)
                                ? D_ZIP_METHOD_DEFLATE
                                : D_ZIP_METHOD_STORE;
            }
            if (work.store_only)
            {
                work.zip.method = D_ZIP_METHOD_STORE;
            }
            if (work.zip.encryption == D_ARCHIVE_KNOB_UNSET)
            {
                work.zip.encryption = D_ZIP_ENCRYPTION_NONE;
            }
            if (work.zip.zip64 == D_ARCHIVE_KNOB_UNSET)
            {
                work.zip.zip64 = 0;
            }
            if (work.zip.utf8_names == D_ARCHIVE_KNOB_UNSET)
            {
                work.zip.utf8_names = 1;
            }
            break;
        }
        case D_FORMAT_ID_TAR:
        case D_FORMAT_ID_TAR_GZ:
        {
            if (work.tar.format == D_ARCHIVE_KNOB_UNSET)
            {
                work.tar.format = D_TAR_FORMAT_USTAR;
            }
            if (work.tar.numeric_owner == D_ARCHIVE_KNOB_UNSET)
            {
                work.tar.numeric_owner = 1;
            }
            break;
        }
        case D_FORMAT_ID_GZ:
        {
            if (work.gz.store_name == D_ARCHIVE_KNOB_UNSET)
            {
                work.gz.store_name = 0;
            }
            if (work.gz.store_mtime == D_ARCHIVE_KNOB_UNSET)
            {
                work.gz.store_mtime = 0;
            }
            break;
        }
        case D_FORMAT_ID_SEVENZIP:
        {
            if (work.sevenzip.method == D_ARCHIVE_KNOB_UNSET)
            {
                work.sevenzip.method = D_SEVENZIP_METHOD_LZMA2;
            }
            if (work.sevenzip.solid == D_ARCHIVE_KNOB_UNSET)
            {
                work.sevenzip.solid = 1;
            }
            if (work.sevenzip.header_compression == D_ARCHIVE_KNOB_UNSET)
            {
                work.sevenzip.header_compression = 1;
            }
            if (work.sevenzip.header_encryption == D_ARCHIVE_KNOB_UNSET)
            {
                work.sevenzip.header_encryption = 0;
            }
            if (work.sevenzip.threads == D_ARCHIVE_KNOB_UNSET)
            {
                work.sevenzip.threads = 1;
            }
            break;
        }
        case D_FORMAT_ID_RAR:
        {
            if (work.rar.level == D_ARCHIVE_KNOB_UNSET)
            {
                work.rar.level = work.level;
            }
            if (work.rar.solid == D_ARCHIVE_KNOB_UNSET)
            {
                work.rar.solid = 0;
            }
            if (work.rar.recovery_record == D_ARCHIVE_KNOB_UNSET)
            {
                work.rar.recovery_record = 0;
            }
            break;
        }
    }

    // the embedded codec inherits the container's effort unless it was tuned
    // in its own right, so a caller who set only the container level gets the
    // effect they asked for all the way down.
    codec = d_format_default_codec(_format);

    if (work.codec.level == D_COMPRESS_KNOB_UNSET)
    {
        work.codec.level = work.level;
    }

    status = d_compress_options_resolve(codec, &work.codec, &work.codec);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    *_out = work;

    return D_PACK_STATUS_OK;
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
enum d_pack_status
d_archive_options_validate(
    enum d_format_id                _format,
    const struct d_archive_options* _options,
    enum d_archive_knob*            _out_offender
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

    return D_PACK_STATUS_OK;
}


// =============================================================================
// V.   CAPABILITY AND ENTRY VALIDITY
// =============================================================================

/*
d_format_is_readable
  Whether this build can read a format.

  Answers from env_archive.h's roll-ups rather than from its own knowledge of
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
    enum d_format_id _format
)
{
    switch (_format)
    {
        case D_FORMAT_ID_ZIP:
        {
            return D_ENV_ARCHIVE_CAN_READ_ZIP ? 1 : 0;
        }
        case D_FORMAT_ID_TAR:
        {
            return D_ENV_ARCHIVE_CAN_READ_TAR ? 1 : 0;
        }
        case D_FORMAT_ID_GZ:
        case D_FORMAT_ID_TAR_GZ:
        {
            return D_ENV_ARCHIVE_CAN_READ_GZ ? 1 : 0;
        }
        case D_FORMAT_ID_SEVENZIP:
        {
            return D_ENV_ARCHIVE_CAN_READ_7Z ? 1 : 0;
        }
        case D_FORMAT_ID_RAR:
        {
            return D_ENV_ARCHIVE_CAN_READ_RAR ? 1 : 0;
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
    enum d_format_id _format
)
{
    switch (_format)
    {
        case D_FORMAT_ID_ZIP:
        {
            return D_ENV_ARCHIVE_CAN_WRITE_ZIP ? 1 : 0;
        }
        case D_FORMAT_ID_TAR:
        {
            return D_ENV_ARCHIVE_CAN_WRITE_TAR ? 1 : 0;
        }
        case D_FORMAT_ID_GZ:
        {
            return D_ENV_ARCHIVE_CAN_WRITE_GZ ? 1 : 0;
        }
        case D_FORMAT_ID_TAR_GZ:
        {
            return D_ENV_ARCHIVE_CAN_WRITE_TGZ ? 1 : 0;
        }
        case D_FORMAT_ID_SEVENZIP:
        {
            return D_ENV_ARCHIVE_CAN_WRITE_7Z ? 1 : 0;
        }
        case D_FORMAT_ID_RAR:
        {
            return D_ENV_ARCHIVE_CAN_WRITE_RAR ? 1 : 0;
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
    enum d_format_id _format,
    enum d_codec_id  _codec
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
    enum d_format_id _format
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
    enum d_format_id _format
)
{
    switch (_format)
    {
        case D_FORMAT_ID_ZIP:      { return D_ENV_ARCHIVE_PREFERRED_ZIP; }
        case D_FORMAT_ID_TAR:
        case D_FORMAT_ID_TAR_GZ:   { return D_ENV_ARCHIVE_PREFERRED_TAR; }
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
    enum d_format_id _format
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
    enum d_format_id* _out_formats,
    size_t            _out_capacity
)
{
    size_t found = 0;
    int    i;

    for (i = 0; i < D_FORMAT_ID_COUNT; ++i)
    {
        if (!d_format_is_writable((enum d_format_id)i))
        {
            continue;
        }
        if ( (_out_formats != NULL) &&
             (found < _out_capacity) )
        {
            _out_formats[found] = (enum d_format_id)i;
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
int
d_archive_entry_name_is_valid(
    struct d_pack_text _name
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
int
d_archive_entry_is_valid(
    const struct d_archive_entry* _entry
)
{
    if (_entry == NULL)
    {
        return 0;
    }
    if (!d_archive_entry_name_is_valid(_entry->name))
    {
        return 0;
    }
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
    enum d_format_id _format
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
    enum d_format_id _format,
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
    enum d_format_id* _out_format
)
{
    static const enum d_format_id order[] =
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
static void
d_internal_put_le16(
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
static void
d_internal_put_le32(
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
static uint32_t
d_internal_get_le16(
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
static uint32_t
d_internal_get_le32(
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
static void
d_internal_put_octal(
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
static uint64_t
d_internal_get_octal(
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
static void
d_internal_dos_datetime(
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

    if (_unix < D_ARCHIVE_EPOCH_DEFAULT)
    {
        _unix = D_ARCHIVE_EPOCH_DEFAULT;
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
  The timestamp.  D_ARCHIVE_EPOCH_DEFAULT when the entry carries none or when
the caller asked not to preserve it -- never the current time, which is what
keeps two runs byte-identical.
*/
static int64_t
d_internal_entry_mtime(
    const struct d_archive_entry*   _entry,
    const struct d_archive_options* _options
)
{
    if ( (_options != NULL) &&
         (_options->preserve_mtime == 0) )
    {
        return (int64_t)D_ARCHIVE_EPOCH_DEFAULT;
    }
    if ( (_entry == NULL) ||
         (_entry->mtime == 0) )
    {
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
static uint32_t
d_internal_entry_mode(
    const struct d_archive_entry*   _entry,
    const struct d_archive_options* _options
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
// 255 bytes is representable provided it can be SPLIT at a separator.  A
// single component longer than 100 bytes cannot be, and that is a formal
// limitation of ustar rather than a defect here -- it is what pax exists to
// fix, and the reason d_tar_format enumerates pax.

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
static enum d_pack_status
d_internal_emit(
    struct d_pack_sink _sink,
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
static enum d_pack_status
d_internal_emit_zeros(
    struct d_pack_sink _sink,
    size_t             _size
)
{
    unsigned char zeros[D_INTERNAL_TAR_BLOCK];
    size_t        left = _size;

    memset(zeros, 0, sizeof(zeros));

    while (left > 0u)
    {
        size_t             take = (left < sizeof(zeros)) ? left : sizeof(zeros);
        enum d_pack_status s    = d_internal_emit(_sink, zeros, take);

        if (s != D_PACK_STATUS_OK)
        {
            return s;
        }

        left -= take;
    }

    return D_PACK_STATUS_OK;
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
static size_t
d_internal_field_len(
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
cannot be represented at all.  Reporting that as UNSUPPORTED rather than
silently truncating is the whole point: a truncated path produces an archive
that extracts to the wrong place and looks correct doing it.

Parameter(s):
  _name:           the entry name.
  _out_name_off:   receives the offset of the `name` portion.
  _out_name_len:   receives its length.
  _out_prefix_len: receives the `prefix` length, 0 when unused.
Return:
  1 when the name is representable in ustar, 0 otherwise.
*/
static int
d_internal_tar_split_name(
    struct d_pack_text _name,
    size_t*            _out_name_off,
    size_t*            _out_name_len,
    size_t*            _out_prefix_len
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
d_internal_tar_header
  Build one 512-byte ustar header.

  The checksum is computed with its own field held at eight spaces, which is
the convention the format mandates and the single most commonly mis-implemented
detail in a hand-written tar writer.

Parameter(s):
  _block:   the 512-byte record to fill.
  _entry:   the entry to describe.
  _options: a resolved option set.
Return:
  D_PACK_STATUS_OK, or D_PACK_STATUS_UNSUPPORTED for an unrepresentable name.
*/
static enum d_pack_status
d_internal_tar_header(
    unsigned char*                  _block,
    const struct d_archive_entry*   _entry,
    const struct d_archive_options* _options
)
{
    size_t   name_off   = 0;
    size_t   name_len   = 0;
    size_t   prefix_len = 0;
    uint32_t sum        = 0;
    size_t   i;

    memset(_block, 0, D_INTERNAL_TAR_BLOCK);

    if (!d_internal_tar_split_name(_entry->name,
                                   &name_off, &name_len, &prefix_len))
    {
        return D_PACK_STATUS_UNSUPPORTED;
    }

    memcpy(_block + D_INTERNAL_TAR_OFF_NAME,
           _entry->name.data + name_off, name_len);

    if (prefix_len != 0u)
    {
        memcpy(_block + D_INTERNAL_TAR_OFF_PREFIX,
               _entry->name.data, prefix_len);
    }

    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_MODE, 8u,
                         (uint64_t)(d_internal_entry_mode(_entry, _options) &
                                    07777u));
    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_UID,   8u, 0u);
    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_GID,   8u, 0u);
    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_SIZE, 12u,
                         (uint64_t)_entry->data.size);
    d_internal_put_octal(_block + D_INTERNAL_TAR_OFF_MTIME, 12u,
                         (uint64_t)d_internal_entry_mtime(_entry, _options));

    _block[D_INTERNAL_TAR_OFF_TYPE] =
        (unsigned char)(_entry->is_directory ? '5' : '0');

    memcpy(_block + D_INTERNAL_TAR_OFF_MAGIC,   "ustar", 5);
    memcpy(_block + D_INTERNAL_TAR_OFF_VERSION, "00",    2);

    // owner NAMES are deliberately left empty.  They are the one field a tar
    // writer normally fills from the host, and doing so makes the same entry
    // produce different bytes on different machines.
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

    return D_PACK_STATUS_OK;
}

/*
d_internal_tar_create
  Write a complete ustar archive to a sink.

Parameter(s):
  _entries:     the members, in order; may be null when _count is 0.
  _count:       how many.
  _options:     a resolved option set.
  _sink:        the destination.
Return:
  D_PACK_STATUS_OK, or the first failure encountered.
*/
static enum d_pack_status
d_internal_tar_create(
    const struct d_archive_entry*   _entries,
    size_t                          _count,
    const struct d_archive_options* _options,
    struct d_pack_sink              _sink
)
{
    unsigned char      block[D_INTERNAL_TAR_BLOCK];
    enum d_pack_status status;
    size_t             i;

    for (i = 0; i < _count; ++i)
    {
        size_t pad;

        if (!d_archive_entry_is_valid(&_entries[i]))
        {
            return D_PACK_STATUS_INVALID_ARGUMENT;
        }

        status = d_internal_tar_header(block, &_entries[i], _options);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }

        status = d_internal_emit(_sink, block, D_INTERNAL_TAR_BLOCK);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }
        if (_entries[i].data.size == 0u)
        {
            continue;
        }

        status = d_internal_emit(_sink,
                                 _entries[i].data.data,
                                 _entries[i].data.size);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }

        pad = _entries[i].data.size % D_INTERNAL_TAR_BLOCK;

        if (pad != 0u)
        {
            status = d_internal_emit_zeros(_sink,
                                           D_INTERNAL_TAR_BLOCK - pad);

            if (status != D_PACK_STATUS_OK)
            {
                return status;
            }
        }
    }

    // the end-of-archive marker: two zero records.
    return d_internal_emit_zeros(_sink, 2u * D_INTERNAL_TAR_BLOCK);
}

/*
d_internal_tar_walk
  Walk a ustar archive, reporting each member.

  Serves BOTH the measure pass and the extract pass, which is what guarantees
they agree about the entry count and the arena size.  When _entries is null the
walk only counts; otherwise it fills the entry array and copies names and
payloads into the arena.

Parameter(s):
  _in:        the archive bytes.
  _in_size:   how many.
  _entries:   receives the members; may be null to measure only.
  _entry_cap: how many entries _entries holds.
  _arena:     receives names and payloads; may be null to measure only.
  _arena_size: how many bytes _arena holds.
  _out:       receives the required entry count and arena size.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL when a region was short, or
a formal status when the archive is malformed.
*/
static enum d_pack_status
d_internal_tar_walk(
    const void*              _in,
    size_t                   _in_size,
    struct d_archive_entry*  _entries,
    size_t                   _entry_cap,
    void*                    _arena,
    size_t                   _arena_size,
    struct d_archive_layout* _out
)
{
    const unsigned char* p        = (const unsigned char*)_in;
    unsigned char*       arena    = (unsigned char*)_arena;
    size_t               pos      = 0;
    size_t               count    = 0;
    size_t               used     = 0;
    int                  overflow = 0;

    _out->entry_count = 0u;
    _out->arena_size  = 0u;

    while ((pos + D_INTERNAL_TAR_BLOCK) <= _in_size)
    {
        const unsigned char* h = p + pos;
        size_t               name_len;
        size_t               prefix_len;
        size_t               total_name;
        uint64_t             size;
        size_t               data_off;

        // two zero records end the archive; one is enough to recognise it.
        if (h[0] == 0u)
        {
            break;
        }
        if (memcmp(h + D_INTERNAL_TAR_OFF_MAGIC, "ustar", 5) != 0)
        {
            return D_PACK_STATUS_CORRUPT_INPUT;
        }

        name_len = d_internal_field_len(h + D_INTERNAL_TAR_OFF_NAME, 100u);
        prefix_len = d_internal_field_len(h + D_INTERNAL_TAR_OFF_PREFIX, 155u);
        total_name = (prefix_len != 0u)
                   ? (prefix_len + 1u + name_len)
                   : name_len;

        size     = d_internal_get_octal(h + D_INTERNAL_TAR_OFF_SIZE, 12u);
        data_off = pos + D_INTERNAL_TAR_BLOCK;

        if ((data_off + (size_t)size) > _in_size)
        {
            return D_PACK_STATUS_TRUNCATED_INPUT;
        }

        if ( (_entries != NULL) &&
             (arena != NULL)    &&
             (count < _entry_cap) &&
             ((used + total_name + (size_t)size) <= _arena_size) )
        {
            struct d_archive_entry* e = &_entries[count];
            unsigned char*          n = arena + used;

            if (prefix_len != 0u)
            {
                memcpy(n, h + D_INTERNAL_TAR_OFF_PREFIX, prefix_len);
                n[prefix_len] = (unsigned char)'/';
                memcpy(n + prefix_len + 1u,
                       h + D_INTERNAL_TAR_OFF_NAME, name_len);
            }
            else
            {
                memcpy(n, h + D_INTERNAL_TAR_OFF_NAME, name_len);
            }

            e->name.data   = (const char*)n;
            e->name.length = total_name;
            used          += total_name;

            if (size != 0u)
            {
                memcpy(arena + used, p + data_off, (size_t)size);
                e->data.data = arena + used;
                e->data.size = (size_t)size;
                used        += (size_t)size;
            }
            else
            {
                e->data.data = NULL;
                e->data.size = 0u;
            }

            e->is_directory =
                (h[D_INTERNAL_TAR_OFF_TYPE] == '5') ? 1 : 0;
            e->mode  = (uint32_t)d_internal_get_octal(
                           h + D_INTERNAL_TAR_OFF_MODE, 8u);
            e->mtime = (int64_t)d_internal_get_octal(
                           h + D_INTERNAL_TAR_OFF_MTIME, 12u);
        }
        else
        {
            if ( (_entries != NULL) ||
                 (arena != NULL) )
            {
                overflow = 1;
            }

            used += total_name + (size_t)size;
        }

        ++count;

        pos = data_off +
              (((size_t)size + D_INTERNAL_TAR_BLOCK - 1u) /
               D_INTERNAL_TAR_BLOCK) * D_INTERNAL_TAR_BLOCK;
    }

    _out->entry_count = count;
    _out->arena_size  = used;

    return overflow ? D_PACK_STATUS_BUFFER_TOO_SMALL : D_PACK_STATUS_OK;
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
d_internal_zip_count_write
  A sink write that discards its bytes and counts them, for the sizing pass.

Parameter(s):
  _context: a size_t total.
  _data:    ignored.
  _size:    how many bytes to add.
Return:
  _size always.
*/
static size_t
d_internal_zip_count_write(
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
d_internal_zip_codec_for_method
  The codec that performs a ZIP method.

Parameter(s):
  _method: a d_zip_method value.
Return:
  The codec, or D_CODEC_ID_STORE for an unrecognised method.
*/
static enum d_codec_id
d_internal_zip_codec_for_method(
    int32_t _method
)
{
    switch (_method)
    {
        case D_ZIP_METHOD_DEFLATE: { return D_CODEC_ID_DEFLATE; }
        case D_ZIP_METHOD_BZIP2:   { return D_CODEC_ID_BZIP2; }
        case D_ZIP_METHOD_LZMA:    { return D_CODEC_ID_XZ; }
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
  A d_zip_method value.
*/
static int32_t
d_internal_zip_entry_method(
    const struct d_archive_entry*   _entry,
    const struct d_archive_options* _options
)
{
    enum d_codec_id codec;

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
d_internal_zip_entry_sizes
  Determine an entry's compressed size and CRC.

  The CRC is always over the UNCOMPRESSED bytes, which is what the format
requires and an easy thing to get backwards.

Parameter(s):
  _entry:     the entry.
  _options:   a resolved option set.
  _method:    the method chosen for this entry.
  _out_csize: receives the compressed size.
  _out_crc:   receives the CRC-32.
Return:
  D_PACK_STATUS_OK, or a status from the codec.
*/
static enum d_pack_status
d_internal_zip_entry_sizes(
    const struct d_archive_entry*   _entry,
    const struct d_archive_options* _options,
    int32_t                         _method,
    size_t*                         _out_csize,
    uint32_t*                       _out_crc
)
{
    struct d_pack_sink counter;
    size_t             total = 0;
    enum d_codec_id    codec;
    enum d_pack_status status;

    *_out_crc = d_pack_crc32(0u, _entry->data.data, _entry->data.size);

    if (_method == D_ZIP_METHOD_STORE)
    {
        *_out_csize = _entry->data.size;

        return D_PACK_STATUS_OK;
    }

    codec           = d_internal_zip_codec_for_method(_method);
    counter.write   = &d_internal_zip_count_write;
    counter.context = &total;

    status = d_compress_to_sink(codec,
                                _entry->data.data,
                                _entry->data.size,
                                &_options->codec,
                                counter,
                                NULL);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    *_out_csize = total;

    return D_PACK_STATUS_OK;
}

/*
d_internal_zip_create
  Write a complete ZIP archive to a sink.

Parameter(s):
  _entries: the members, in order; may be null when _count is 0.
  _count:   how many.
  _options: a resolved option set.
  _sink:    the destination.
Return:
  D_PACK_STATUS_OK, or the first failure encountered.
*/
static enum d_pack_status
d_internal_zip_create(
    const struct d_archive_entry*   _entries,
    size_t                          _count,
    const struct d_archive_options* _options,
    struct d_pack_sink              _sink
)
{
    unsigned char      hdr[D_INTERNAL_ZIP_CENTRAL_SIZE];
    enum d_pack_status status;
    size_t             i;
    size_t             offset    = 0;
    size_t             cd_offset = 0;
    size_t             cd_size   = 0;

    // -- pass 1: local headers and member data ------------------------------
    for (i = 0; i < _count; ++i)
    {
        const struct d_archive_entry* e = &_entries[i];
        int32_t                       method;
        size_t                        csize = 0;
        uint32_t                      crc   = 0;
        uint32_t                      date  = 0;
        uint32_t                      time  = 0;

        if (!d_archive_entry_is_valid(e))
        {
            return D_PACK_STATUS_INVALID_ARGUMENT;
        }

        method = d_internal_zip_entry_method(e, _options);
        status = d_internal_zip_entry_sizes(e, _options, method,
                                            &csize, &crc);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }

        d_internal_dos_datetime(d_internal_entry_mtime(e, _options),
                                &date, &time);

        memset(hdr, 0, D_INTERNAL_ZIP_LOCAL_SIZE);
        memcpy(hdr, "PK\003\004", 4);
        d_internal_put_le16(hdr + 4,  20u);         // version needed
        d_internal_put_le16(hdr + 6,
            (uint32_t)(_options->zip.utf8_names ? 0x0800u : 0u));
        d_internal_put_le16(hdr + 8,  (uint32_t)method);
        d_internal_put_le16(hdr + 10, time);
        d_internal_put_le16(hdr + 12, date);
        d_internal_put_le32(hdr + 14, crc);
        d_internal_put_le32(hdr + 18, (uint32_t)csize);
        d_internal_put_le32(hdr + 22, (uint32_t)e->data.size);
        d_internal_put_le16(hdr + 26, (uint32_t)e->name.length);
        d_internal_put_le16(hdr + 28, 0u);          // extra field length

        status = d_internal_emit(_sink, hdr, D_INTERNAL_ZIP_LOCAL_SIZE);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }

        status = d_internal_emit(_sink, e->name.data, e->name.length);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }

        if (e->data.size != 0u)
        {
            if (method == D_ZIP_METHOD_STORE)
            {
                status = d_internal_emit(_sink, e->data.data, e->data.size);
            }
            else
            {
                status = d_compress_to_sink(
                    d_internal_zip_codec_for_method(method),
                    e->data.data, e->data.size,
                    &_options->codec, _sink, NULL);
            }

            if (status != D_PACK_STATUS_OK)
            {
                return status;
            }
        }

        offset += D_INTERNAL_ZIP_LOCAL_SIZE + e->name.length + csize;
    }

    // -- pass 2: the central directory ---------------------------------------
    cd_offset = offset;
    offset    = 0;

    for (i = 0; i < _count; ++i)
    {
        const struct d_archive_entry* e = &_entries[i];
        int32_t                       method;
        size_t                        csize = 0;
        uint32_t                      crc   = 0;
        uint32_t                      date  = 0;
        uint32_t                      time  = 0;
        uint32_t                      attr;

        method = d_internal_zip_entry_method(e, _options);
        status = d_internal_zip_entry_sizes(e, _options, method,
                                            &csize, &crc);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }

        d_internal_dos_datetime(d_internal_entry_mtime(e, _options),
                                &date, &time);

        // external attributes: the unix mode in the high 16 bits, plus the
        // MS-DOS directory bit in the low byte so that both worlds agree
        // about what is a directory.
        attr = (d_internal_entry_mode(e, _options) & 07777u) << 16;

        if (e->is_directory)
        {
            attr |= 0x40000000u | 0x10u;
        }

        memset(hdr, 0, D_INTERNAL_ZIP_CENTRAL_SIZE);
        memcpy(hdr, "PK\001\002", 4);
        d_internal_put_le16(hdr + 4,  (uint32_t)(3u << 8 | 20u)); // unix
        d_internal_put_le16(hdr + 6,  20u);
        d_internal_put_le16(hdr + 8,
            (uint32_t)(_options->zip.utf8_names ? 0x0800u : 0u));
        d_internal_put_le16(hdr + 10, (uint32_t)method);
        d_internal_put_le16(hdr + 12, time);
        d_internal_put_le16(hdr + 14, date);
        d_internal_put_le32(hdr + 16, crc);
        d_internal_put_le32(hdr + 20, (uint32_t)csize);
        d_internal_put_le32(hdr + 24, (uint32_t)e->data.size);
        d_internal_put_le16(hdr + 28, (uint32_t)e->name.length);
        d_internal_put_le16(hdr + 30, 0u);          // extra
        d_internal_put_le16(hdr + 32, 0u);          // comment
        d_internal_put_le16(hdr + 34, 0u);          // disk number
        d_internal_put_le16(hdr + 36, 0u);          // internal attributes
        d_internal_put_le32(hdr + 38, attr);
        d_internal_put_le32(hdr + 42, (uint32_t)offset);

        status = d_internal_emit(_sink, hdr, D_INTERNAL_ZIP_CENTRAL_SIZE);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }

        status = d_internal_emit(_sink, e->name.data, e->name.length);

        if (status != D_PACK_STATUS_OK)
        {
            return status;
        }

        cd_size += D_INTERNAL_ZIP_CENTRAL_SIZE + e->name.length;
        offset  += D_INTERNAL_ZIP_LOCAL_SIZE + e->name.length + csize;
    }

    // -- the end-of-central-directory record --------------------------------
    memset(hdr, 0, D_INTERNAL_ZIP_EOCD_SIZE);
    memcpy(hdr, "PK\005\006", 4);
    d_internal_put_le16(hdr + 8,  (uint32_t)_count);
    d_internal_put_le16(hdr + 10, (uint32_t)_count);
    d_internal_put_le32(hdr + 12, (uint32_t)cd_size);
    d_internal_put_le32(hdr + 16, (uint32_t)cd_offset);
    d_internal_put_le16(hdr + 20, (uint32_t)_options->comment.length);

    status = d_internal_emit(_sink, hdr, D_INTERNAL_ZIP_EOCD_SIZE);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }
    if (_options->comment.length != 0u)
    {
        return d_internal_emit(_sink,
                               _options->comment.data,
                               _options->comment.length);
    }

    return D_PACK_STATUS_OK;
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
static int
d_internal_zip_find_eocd(
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

/*
d_internal_zip_walk
  Walk a ZIP's central directory, reporting each member.

  Serves both the measure pass and the extract pass, exactly as the tar walk
does, so the two cannot disagree.  Members are read through the CENTRAL
directory rather than by scanning local headers: the directory is the archive's
authoritative index, and a local-header scan cannot distinguish a real member
from the bytes of a stored member that happens to contain a ZIP.

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
static enum d_pack_status
d_internal_zip_walk(
    const void*              _in,
    size_t                   _in_size,
    struct d_archive_entry*  _entries,
    size_t                   _entry_cap,
    void*                    _arena,
    size_t                   _arena_size,
    struct d_archive_layout* _out
)
{
    const unsigned char* p        = (const unsigned char*)_in;
    unsigned char*       arena    = (unsigned char*)_arena;
    size_t               eocd     = 0;
    size_t               cd_off;
    size_t               total;
    size_t               pos;
    size_t               count    = 0;
    size_t               used     = 0;
    int                  overflow = 0;
    size_t               i;

    _out->entry_count = 0u;
    _out->arena_size  = 0u;

    if (!d_internal_zip_find_eocd(p, _in_size, &eocd))
    {
        return D_PACK_STATUS_CORRUPT_INPUT;
    }

    total  = (size_t)d_internal_get_le16(p + eocd + 10);
    cd_off = (size_t)d_internal_get_le32(p + eocd + 16);

    if (cd_off > _in_size)
    {
        return D_PACK_STATUS_CORRUPT_INPUT;
    }

    pos = cd_off;

    for (i = 0; i < total; ++i)
    {
        const unsigned char* c = p + pos;
        size_t               name_len;
        size_t               extra_len;
        size_t               comment_len;
        size_t               local_off;
        size_t               csize;
        size_t               usize;
        uint32_t             method;
        size_t               data_off;

        if ( ((pos + D_INTERNAL_ZIP_CENTRAL_SIZE) > _in_size) ||
             (memcmp(c, "PK\001\002", 4) != 0) )
        {
            return D_PACK_STATUS_CORRUPT_INPUT;
        }

        method      = d_internal_get_le16(c + 10);
        csize       = (size_t)d_internal_get_le32(c + 20);
        usize       = (size_t)d_internal_get_le32(c + 24);
        name_len    = (size_t)d_internal_get_le16(c + 28);
        extra_len   = (size_t)d_internal_get_le16(c + 30);
        comment_len = (size_t)d_internal_get_le16(c + 32);
        local_off   = (size_t)d_internal_get_le32(c + 42);

        if ((local_off + D_INTERNAL_ZIP_LOCAL_SIZE) > _in_size)
        {
            return D_PACK_STATUS_CORRUPT_INPUT;
        }

        // the local header's own name and extra lengths are authoritative for
        // where the data starts; they may differ from the directory's.
        data_off = local_off + D_INTERNAL_ZIP_LOCAL_SIZE +
                   (size_t)d_internal_get_le16(p + local_off + 26) +
                   (size_t)d_internal_get_le16(p + local_off + 28);

        if ((data_off + csize) > _in_size)
        {
            return D_PACK_STATUS_TRUNCATED_INPUT;
        }

        if ( (_entries != NULL) &&
             (arena != NULL)    &&
             (count < _entry_cap) &&
             ((used + name_len + usize) <= _arena_size) )
        {
            struct d_archive_entry* e = &_entries[count];
            unsigned char*          n = arena + used;

            memcpy(n, c + D_INTERNAL_ZIP_CENTRAL_SIZE, name_len);
            e->name.data   = (const char*)n;
            e->name.length = name_len;
            used          += name_len;

            e->data.data = NULL;
            e->data.size = 0u;

            if (usize != 0u)
            {
                size_t got = 0;

                if (method == (uint32_t)D_ZIP_METHOD_STORE)
                {
                    memcpy(arena + used, p + data_off, csize);
                    got = csize;
                }
                else
                {
                    enum d_pack_status s = d_decompress(
                        d_internal_zip_codec_for_method((int32_t)method),
                        p + data_off, csize,
                        arena + used, _arena_size - used, &got);

                    if (s != D_PACK_STATUS_OK)
                    {
                        return s;
                    }
                }

                e->data.data = arena + used;
                e->data.size = got;
                used        += got;
            }

            // a name ending in '/' is the format's way of saying "directory";
            // there is no type flag in a ZIP record.
            e->is_directory = ( (name_len != 0u) &&
                                (n[name_len - 1u] == '/') ) ? 1 : 0;
            e->mode  = (uint32_t)((d_internal_get_le32(c + 38) >> 16) & 07777u);
            e->mtime = 0;
        }
        else
        {
            if ( (_entries != NULL) ||
                 (arena != NULL) )
            {
                overflow = 1;
            }

            used += name_len + usize;
        }

        ++count;
        pos += D_INTERNAL_ZIP_CENTRAL_SIZE + name_len + extra_len +
               comment_len;
    }

    _out->entry_count = count;
    _out->arena_size  = used;

    return overflow ? D_PACK_STATUS_BUFFER_TOO_SMALL : D_PACK_STATUS_OK;
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
  _context: a struct d_pack_buffer_sink.
  _data:    the bytes offered.
  _size:    how many.
Return:
  _size always; the caller checks `overflow`.
*/
static size_t
d_internal_archive_buffer_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct d_pack_buffer_sink* b = (struct d_pack_buffer_sink*)_context;

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

// d_internal_tee
//   struct: the context of a sink that counts the bytes passing through it and
// forwards them to an inner sink.
struct d_internal_tee
{
    struct d_pack_sink  inner;
    size_t              total;
};

/*
d_internal_tee_write
  A sink write that counts the bytes and forwards them to an inner sink.

  This is how a writer reports the size it produced without requiring the
caller's sink to be countable -- a file, a socket and a growable buffer are all
legitimate destinations and none of them is obliged to keep a total.

Parameter(s):
  _context: a struct d_internal_tee.
  _data:    the bytes offered.
  _size:    how many.
Return:
  What the inner sink accepted, so a short write still propagates.
*/
static size_t
d_internal_tee_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct d_internal_tee* t = (struct d_internal_tee*)_context;
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
static enum d_pack_status
d_internal_archive_prepare(
    enum d_format_id                _format,
    int                             _writing,
    const struct d_archive_options* _in,
    struct d_archive_options*       _out
)
{
    enum d_pack_status status;

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
d_archive_create_to_sink
  Write a container to a sink.

  An empty entry list is legal for every multi-member format and produces a
valid empty container -- document_bundle already relies on that, passing a null
base rather than indexing an empty vector.  It is a formal error only for gz,
which has no directory to be empty.

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
enum d_pack_status
d_archive_create_to_sink(
    enum d_format_id                _format,
    const struct d_archive_entry*   _entries,
    size_t                          _entry_count,
    const struct d_archive_options* _options,
    struct d_pack_sink              _sink,
    size_t*                         _out_size
)
{
    struct d_archive_options resolved;
    struct d_pack_sink       counted;
    struct d_internal_tee    tally;
    enum d_pack_status       status;

    if ( (_sink.write == NULL) ||
         ((_entries == NULL) && (_entry_count != 0u)) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    status = d_internal_archive_prepare(_format, 1, _options, &resolved);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    // gz carries exactly one member and has no directory, so anything other
    // than a single entry is a formal error rather than a truncation.
    if ( D_FORMAT_ID_IS_SINGLE_STREAM(_format) &&
         (_entry_count != 1u) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    // interpose a tee so the produced size is known whatever the caller's
    // sink does with the bytes.
    tally.inner     = _sink;
    tally.total     = 0u;
    counted.write   = &d_internal_tee_write;
    counted.context = &tally;

    switch (_format)
    {
        case D_FORMAT_ID_TAR:
        {
            status = d_internal_tar_create(_entries, _entry_count,
                                           &resolved, counted);
            break;
        }
        case D_FORMAT_ID_ZIP:
        {
            status = d_internal_zip_create(_entries, _entry_count,
                                           &resolved, counted);
            break;
        }
        case D_FORMAT_ID_GZ:
        {
            // a single-member container IS the codec, with the entry's payload
            // as the whole stream.  There is no directory to write.
            if (!d_archive_entry_is_valid(&_entries[0]))
            {
                return D_PACK_STATUS_INVALID_ARGUMENT;
            }

            status = d_compress_to_sink(D_CODEC_ID_GZIP,
                                        _entries[0].data.data,
                                        _entries[0].data.size,
                                        &resolved.codec,
                                        counted,
                                        NULL);
            break;
        }
        case D_FORMAT_ID_TAR_GZ:
        {
            // tar then gzip is a COMPOSITION of two transforms, and composing
            // them without an intermediate buffer needs a streaming tar whose
            // output feeds the codec's input.  The codec leaves consume a whole
            // buffer rather than a stream, so this path needs staging that
            // tier 0 has nowhere to put.  UNSUPPORTED is the honest answer
            // until d_string exists; a caller wanting a tarball today creates
            // the tar, then compresses the result.
            status = D_PACK_STATUS_UNSUPPORTED;
            break;
        }
        default:
        {
            status = D_PACK_STATUS_UNAVAILABLE;
            break;
        }
    }

    if ( (status == D_PACK_STATUS_OK) &&
         (_out_size != NULL) )
    {
        *_out_size = tally.total;
    }

    return status;
}

/*
d_archive_create
  Write a container into a caller-owned buffer under the two-call protocol.

Parameter(s):
  _format:       the container format.
  _entries:      the members, in order.
  _entry_count:  how many.
  _options:      the tuning; null requests the core's pinned defaults.
  _out:          the destination; null with _out_capacity 0 measures.
  _out_capacity: how many bytes _out holds.
  _out_size:     receives the produced or required size.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL with the requirement in
*_out_size, or a status describing the failure.
*/
enum d_pack_status
d_archive_create(
    enum d_format_id                _format,
    const struct d_archive_entry*   _entries,
    size_t                          _entry_count,
    const struct d_archive_options* _options,
    void*                           _out,
    size_t                          _out_capacity,
    size_t*                         _out_size
)
{
    struct d_pack_buffer_sink dest;
    struct d_pack_sink        sink;
    enum d_pack_status        status;

    if (_out_size == NULL)
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    d_pack_buffer_sink_init(&dest, _out, _out_capacity);
    sink.write   = &d_internal_archive_buffer_write;
    sink.context = &dest;

    status = d_archive_create_to_sink(_format, _entries, _entry_count,
                                      _options, sink, NULL);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    *_out_size = dest.needed;

    // a MEASURE call asked only for that number and has succeeded; an overflow
    // is a failure only when the caller offered somewhere to put the bytes.
    if ( (_out == NULL) &&
         (_out_capacity == 0u) )
    {
        return D_PACK_STATUS_OK;
    }

    return dest.overflow
         ? D_PACK_STATUS_BUFFER_TOO_SMALL
         : D_PACK_STATUS_OK;
}

/*
d_archive_bound
  An upper bound on the size of a container built from these entries.

  Cheap and always safe.  Every format's per-entry overhead is a constant plus
the name, and the payload's worst case is the codec's compress bound -- which
exceeds the input for incompressible data, the case a caller sizing from the
payload total alone gets wrong.

Parameter(s):
  _format:      the container format.
  _entries:     the members.
  _entry_count: how many.
  _options:     the tuning; may be null.
  _out_bound:   receives the bound; must not be null.
Return:
  D_PACK_STATUS_OK, or D_PACK_STATUS_INVALID_ARGUMENT.
*/
enum d_pack_status
d_archive_bound(
    enum d_format_id                _format,
    const struct d_archive_entry*   _entries,
    size_t                          _entry_count,
    const struct d_archive_options* _options,
    size_t*                         _out_bound
)
{
    size_t total = 0;
    size_t i;

    (void)_options;

    if ( (_out_bound == NULL) ||
         (!D_FORMAT_ID_IS_VALID(_format)) ||
         ((_entries == NULL) && (_entry_count != 0u)) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    for (i = 0; i < _entry_count; ++i)
    {
        size_t             payload = 0;
        enum d_pack_status s       = d_compress_bound(
            d_format_default_codec(_format),
            _entries[i].data.size, NULL, &payload);

        if (s != D_PACK_STATUS_OK)
        {
            return s;
        }

        switch (_format)
        {
            case D_FORMAT_ID_TAR:
            {
                // one header record, then the payload rounded up to a record
                total += D_INTERNAL_TAR_BLOCK +
                         ((_entries[i].data.size + D_INTERNAL_TAR_BLOCK - 1u) /
                          D_INTERNAL_TAR_BLOCK) * D_INTERNAL_TAR_BLOCK;
                break;
            }
            case D_FORMAT_ID_ZIP:
            {
                // a local header and a directory record, each carrying the name
                total += D_INTERNAL_ZIP_LOCAL_SIZE +
                         D_INTERNAL_ZIP_CENTRAL_SIZE +
                         (2u * _entries[i].name.length) +
                         payload;
                break;
            }
            default:
            {
                total += payload + _entries[i].name.length + 512u;
                break;
            }
        }
    }

    switch (_format)
    {
        case D_FORMAT_ID_TAR:
        {
            total += 2u * D_INTERNAL_TAR_BLOCK;
            break;
        }
        case D_FORMAT_ID_ZIP:
        {
            total += D_INTERNAL_ZIP_EOCD_SIZE + 65535u;
            break;
        }
        default:
        {
            total += 1024u;
            break;
        }
    }

    *_out_bound = total;

    return D_PACK_STATUS_OK;
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
enum d_pack_status
d_archive_measure(
    enum d_format_id                _format,
    const void*                     _in,
    size_t                          _in_size,
    const struct d_archive_options* _options,
    struct d_archive_layout*        _out_layout
)
{
    struct d_archive_options resolved;
    enum d_pack_status       status;

    if ( (_out_layout == NULL) ||
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
        {
            return d_internal_tar_walk(_in, _in_size, NULL, 0u, NULL, 0u,
                                       _out_layout);
        }
        case D_FORMAT_ID_ZIP:
        {
            return d_internal_zip_walk(_in, _in_size, NULL, 0u, NULL, 0u,
                                       _out_layout);
        }
        case D_FORMAT_ID_GZ:
        {
            size_t             need = 0;
            enum d_pack_status s    = d_decompress(D_CODEC_ID_GZIP,
                                                   _in, _in_size,
                                                   NULL, 0u, &need);

            if (s != D_PACK_STATUS_OK)
            {
                return s;
            }

            // one member, whose name the gzip header may or may not carry;
            // the arena holds only the payload.
            _out_layout->entry_count = 1u;
            _out_layout->arena_size  = need;

            return D_PACK_STATUS_OK;
        }
        default:
        {
            return D_PACK_STATUS_UNAVAILABLE;
        }
    }
}

/*
d_archive_extract
  Read a container's members into caller-owned regions.

  Each filled entry's name and data spans point INTO _arena, and are valid for
exactly as long as it is.  Either region being too small yields
D_PACK_STATUS_BUFFER_TOO_SMALL with *_out_used carrying BOTH requirements, so
one retry suffices.

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
enum d_pack_status
d_archive_extract(
    enum d_format_id                _format,
    const void*                     _in,
    size_t                          _in_size,
    const struct d_archive_options* _options,
    struct d_archive_entry*         _entries,
    size_t                          _entry_cap,
    void*                           _arena,
    size_t                          _arena_size,
    struct d_archive_layout*        _out_used
)
{
    struct d_archive_options resolved;
    enum d_pack_status       status;

    if ( (_out_used == NULL) ||
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
        {
            return d_internal_tar_walk(_in, _in_size, _entries, _entry_cap,
                                       _arena, _arena_size, _out_used);
        }
        case D_FORMAT_ID_ZIP:
        {
            return d_internal_zip_walk(_in, _in_size, _entries, _entry_cap,
                                       _arena, _arena_size, _out_used);
        }
        case D_FORMAT_ID_GZ:
        {
            size_t got = 0;

            status = d_decompress(D_CODEC_ID_GZIP, _in, _in_size,
                                  _arena, _arena_size, &got);

            _out_used->entry_count = 1u;
            _out_used->arena_size  = got;

            if (status != D_PACK_STATUS_OK)
            {
                return status;
            }
            if (_entry_cap < 1u)
            {
                return D_PACK_STATUS_BUFFER_TOO_SMALL;
            }

            _entries[0].name.data     = NULL;
            _entries[0].name.length   = 0u;
            _entries[0].data.data     = _arena;
            _entries[0].data.size     = got;
            _entries[0].is_directory  = 0;
            _entries[0].mode          = (uint32_t)D_ARCHIVE_MODE_FILE_DEFAULT;
            _entries[0].mtime         = 0;

            return D_PACK_STATUS_OK;
        }
        default:
        {
            return D_PACK_STATUS_UNAVAILABLE;
        }
    }
}

/*
d_archive_extract_entry
  Pull one member out by name without materialising the rest.

  The common case -- reading a single file from a container -- and the one that
needs no arena.  Implemented by walking to the named member and decoding only
it, so the cost is the directory walk plus that member, not the whole archive.

Parameter(s):
  _format:       the container format.
  _in:           the archive bytes.
  _in_size:      how many.
  _name:         the member's name, compared byte for byte.
  _options:      the tuning; may be null.
  _out:          the destination; null with _out_capacity 0 measures.
  _out_capacity: how many bytes _out holds.
  _out_size:     receives the produced or required size.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL, or a formal status.
D_PACK_STATUS_INVALID_ARGUMENT when no member carries that name.
*/
enum d_pack_status
d_archive_extract_entry(
    enum d_format_id                _format,
    const void*                     _in,
    size_t                          _in_size,
    struct d_pack_text              _name,
    const struct d_archive_options* _options,
    void*                           _out,
    size_t                          _out_capacity,
    size_t*                         _out_size
)
{
    const unsigned char*     p = (const unsigned char*)_in;
    struct d_archive_options resolved;
    enum d_pack_status       status;

    if ( (_out_size == NULL) ||
         ((_in == NULL) && (_in_size != 0u)) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    status = d_internal_archive_prepare(_format, 0, _options, &resolved);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    if (_format == D_FORMAT_ID_TAR)
    {
        size_t pos = 0;

        while ((pos + D_INTERNAL_TAR_BLOCK) <= _in_size)
        {
            const unsigned char* h = p + pos;
            size_t               nl;
            size_t               pl;
            uint64_t             size;
            struct d_pack_text   here;
            char                 joined[256];

            if (h[0] == 0u)
            {
                break;
            }

            nl   = d_internal_field_len(h + D_INTERNAL_TAR_OFF_NAME, 100u);
            pl   = d_internal_field_len(h + D_INTERNAL_TAR_OFF_PREFIX, 155u);
            size = d_internal_get_octal(h + D_INTERNAL_TAR_OFF_SIZE, 12u);

            if (pl != 0u)
            {
                memcpy(joined, h + D_INTERNAL_TAR_OFF_PREFIX, pl);
                joined[pl] = '/';
                memcpy(joined + pl + 1u, h + D_INTERNAL_TAR_OFF_NAME, nl);
                here.data   = joined;
                here.length = pl + 1u + nl;
            }
            else
            {
                here.data   = (const char*)(h + D_INTERNAL_TAR_OFF_NAME);
                here.length = nl;
            }

            if (d_pack_text_equal(here, _name))
            {
                *_out_size = (size_t)size;

                if ( (_out == NULL) &&
                     (_out_capacity == 0u) )
                {
                    return D_PACK_STATUS_OK;
                }
                if (_out_capacity < (size_t)size)
                {
                    return D_PACK_STATUS_BUFFER_TOO_SMALL;
                }

                memcpy(_out, h + D_INTERNAL_TAR_BLOCK, (size_t)size);

                return D_PACK_STATUS_OK;
            }

            pos += D_INTERNAL_TAR_BLOCK +
                   (((size_t)size + D_INTERNAL_TAR_BLOCK - 1u) /
                    D_INTERNAL_TAR_BLOCK) * D_INTERNAL_TAR_BLOCK;
        }

        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    if (_format == D_FORMAT_ID_ZIP)
    {
        size_t eocd  = 0;
        size_t total;
        size_t pos;
        size_t i;

        if (!d_internal_zip_find_eocd(p, _in_size, &eocd))
        {
            return D_PACK_STATUS_CORRUPT_INPUT;
        }

        total = (size_t)d_internal_get_le16(p + eocd + 10);
        pos   = (size_t)d_internal_get_le32(p + eocd + 16);

        for (i = 0; i < total; ++i)
        {
            const unsigned char* c = p + pos;
            size_t               nl;
            size_t               csize;
            size_t               usize;
            uint32_t             method;
            size_t               data_off;
            struct d_pack_text   here;

            if ( ((pos + D_INTERNAL_ZIP_CENTRAL_SIZE) > _in_size) ||
                 (memcmp(c, "PK\001\002", 4) != 0) )
            {
                return D_PACK_STATUS_CORRUPT_INPUT;
            }

            method = d_internal_get_le16(c + 10);
            csize  = (size_t)d_internal_get_le32(c + 20);
            usize  = (size_t)d_internal_get_le32(c + 24);
            nl     = (size_t)d_internal_get_le16(c + 28);

            here.data   = (const char*)(c + D_INTERNAL_ZIP_CENTRAL_SIZE);
            here.length = nl;

            if (d_pack_text_equal(here, _name))
            {
                size_t local_off = (size_t)d_internal_get_le32(c + 42);

                data_off = local_off + D_INTERNAL_ZIP_LOCAL_SIZE +
                           (size_t)d_internal_get_le16(p + local_off + 26) +
                           (size_t)d_internal_get_le16(p + local_off + 28);

                if ((data_off + csize) > _in_size)
                {
                    return D_PACK_STATUS_TRUNCATED_INPUT;
                }

                *_out_size = usize;

                if ( (_out == NULL) &&
                     (_out_capacity == 0u) )
                {
                    return D_PACK_STATUS_OK;
                }
                if (_out_capacity < usize)
                {
                    return D_PACK_STATUS_BUFFER_TOO_SMALL;
                }

                if (method == (uint32_t)D_ZIP_METHOD_STORE)
                {
                    memcpy(_out, p + data_off, csize);

                    return D_PACK_STATUS_OK;
                }

                return d_decompress(
                    d_internal_zip_codec_for_method((int32_t)method),
                    p + data_off, csize, _out, _out_capacity, _out_size);
            }

            pos += D_INTERNAL_ZIP_CENTRAL_SIZE + nl +
                   (size_t)d_internal_get_le16(c + 30) +
                   (size_t)d_internal_get_le16(c + 32);
        }

        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    return D_PACK_STATUS_UNAVAILABLE;
}
