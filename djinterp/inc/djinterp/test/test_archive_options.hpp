/******************************************************************************
* djinterp [test]                                     test_archive_options.hpp
*
*   Shared, DEPENDENCY-LIGHT comparison helpers for the archive option
* vocabulary (core/util/archive_options.hpp).  archive_options is plain data
* with no operator==, and it nests deeply (five per-format blocks plus an
* embedded compress_options carrying six per-codec blocks), so every suite that
* passes options across a boundary otherwise re-derives the same field-by-field
* walk.  It is collected once here instead.
*
*   WHY IT LIVES HERE (not inside a single suite):
*   Several modules accept an archive_options and forward it (bundle writers,
* report packaging, the archive facade itself).  Their tests share one question
* -- "did the options I handed over arrive unchanged, and did ONLY the field I
* set actually move?" -- which needs a deep comparator and a readable diff, not
* a hand-rolled chain of == per call site.  Both live in djinterp::test
* alongside the other shared test utilities (e.g. test_archive.hpp).
*
*   WHAT IT PROVIDES:
*   Deep equality at every level (whole aggregate, each per-format block, and
* the embedded codec), a "is this still pristine?" predicate against a
* default-constructed baseline, a human-readable diff naming the fields that
* moved, and a few small builders for the option sets suites reach for most.
*
*   LAYERING:
*   The embedded codec's comparators are NOT defined here.  archive_options
* HAS-A compress_options, so this header includes test_compress_options.hpp and
* reuses its per-codec comparators rather than keeping a second copy that could
* drift.  Both headers are link-free, so the layering costs a caller nothing;
* including this one gives you both surfaces.
*
*   WHAT IT IS NOT:
*   Not an assertion framework and not a test itself -- it returns plain bool /
* std::string that a suite feeds to its own D_xx_CHECK macros.  It asserts
* nothing about whether a backend HONOURS a knob; that is behaviour, and it
* belongs to the suite exercising the facade.  This header only answers what
* the option data itself says.
*
*   DIFF GRANULARITY:
*   describe_option_diff names archive-level and per-format fields
* individually, because those are the knobs call sites actually set.  The
* embedded codec's per-codec blocks are reported at BLOCK granularity
* ("codec.zstd") with codec.level called out by name -- naming all fifty codec
* fields would bury the signal for a caller tuning a container.  That coarseness
* is deliberate, not a limitation: when the individual knob matters, call
* describe_compress_option_diff from test_compress_options.hpp on the .codec
* member, which names every field.
*
*   PORTABILITY:
*   C++98 - C++23, header-only, no third-party include.  Note that
* archive_options.hpp is NOT self-contained (it uses NS_DJINTERP without
* including the header that defines it), so djinterp.hpp is included first
* here; that ordering is deliberate, not incidental.
*
*
* TABLE OF CONTENTS
* =================
* I.    PER-FORMAT BLOCK COMPARISON
* II.   AGGREGATE COMPARISON
* III.  PRISTINE-DEFAULT PREDICATES
* IV.   DIFF DESCRIPTION
* V.    OPTION BUILDERS
*
*
* path:      /inc/djinterp/test/test_archive_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

#ifndef DJINTERP_TEST_ARCHIVE_OPTIONS_
#define DJINTERP_TEST_ARCHIVE_OPTIONS_ 1

#ifndef __cplusplus
    #error "test_archive_options.hpp requires C++ compilation"
#endif

// std
#include <string>
// djinterp
#include "../core/djinterp.hpp"              // namespace / qualifier macros
#include "../core/util/archive_options.hpp"  // the vocabulary under comparison
#include "./test_compress_options.hpp"       // the embedded codec's comparators


NS_DJINTERP
NS_TEST

// =============================================================================
// I.   PER-FORMAT BLOCK COMPARISON
// =============================================================================

// zip_options_equal
//   function: deep equality for the ZIP container block.
//
// Parameter(s):
//   _a: the first block.
//   _b: the second block.
// Return:
//   true when every field matches.
D_INLINE bool
zip_options_equal(
    const zip_options& _a,
    const zip_options& _b
)
{
    return (_a.method     == _b.method)     &&
           (_a.encryption == _b.encryption) &&
           (_a.password   == _b.password)   &&
           (_a.zip64      == _b.zip64)      &&
           (_a.utf8_names == _b.utf8_names);
}

// tar_options_equal
//   function: deep equality for the tar container block.
//
// Parameter(s):
//   _a: the first block.
//   _b: the second block.
// Return:
//   true when every field matches.
D_INLINE bool
tar_options_equal(
    const tar_options& _a,
    const tar_options& _b
)
{
    return (_a.format        == _b.format) &&
           (_a.numeric_owner == _b.numeric_owner);
}

// gz_options_equal
//   function: deep equality for the gzip header block.
//
// Parameter(s):
//   _a: the first block.
//   _b: the second block.
// Return:
//   true when every field matches.
D_INLINE bool
gz_options_equal(
    const gz_options& _a,
    const gz_options& _b
)
{
    return (_a.store_name    == _b.store_name)  &&
           (_a.store_mtime   == _b.store_mtime) &&
           (_a.original_name == _b.original_name);
}

// sevenzip_options_equal
//   function: deep equality for the 7z container block.
//
// Parameter(s):
//   _a: the first block.
//   _b: the second block.
// Return:
//   true when every field matches.
D_INLINE bool
sevenzip_options_equal(
    const sevenzip_options& _a,
    const sevenzip_options& _b
)
{
    return (_a.method             == _b.method)             &&
           (_a.solid              == _b.solid)              &&
           (_a.header_compression == _b.header_compression) &&
           (_a.header_encryption  == _b.header_encryption)  &&
           (_a.password           == _b.password)           &&
           (_a.threads            == _b.threads);
}

// rar_options_equal
//   function: deep equality for the RAR creation block.
//
// Parameter(s):
//   _a: the first block.
//   _b: the second block.
// Return:
//   true when every field matches.
D_INLINE bool
rar_options_equal(
    const rar_options& _a,
    const rar_options& _b
)
{
    return (_a.level           == _b.level)           &&
           (_a.solid           == _b.solid)           &&
           (_a.recovery_record == _b.recovery_record) &&
           (_a.password        == _b.password);
}


// =============================================================================
// II.  AGGREGATE COMPARISON
// =============================================================================

// archive_options_equal
//   function: deep equality across the whole aggregate -- the archive-level
// fields, every per-format block, and the embedded codec.
//
// Parameter(s):
//   _a: the first option set.
//   _b: the second option set.
// Return:
//   true when the two sets are field-for-field identical.
D_INLINE bool
archive_options_equal(
    const archive_options& _a,
    const archive_options& _b
)
{
    return (_a.level                == _b.level)                &&
           (_a.store_only           == _b.store_only)           &&
           (_a.comment              == _b.comment)              &&
           (_a.preserve_permissions == _b.preserve_permissions) &&
           (_a.preserve_mtime       == _b.preserve_mtime)       &&
           compress_options_equal(_a.codec, _b.codec)           &&
           zip_options_equal(_a.zip, _b.zip)                    &&
           tar_options_equal(_a.tar, _b.tar)                    &&
           gz_options_equal(_a.gz, _b.gz)                       &&
           sevenzip_options_equal(_a.sevenzip, _b.sevenzip)     &&
           rar_options_equal(_a.rar, _b.rar);
}


// =============================================================================
// III. PRISTINE-DEFAULT PREDICATES
// =============================================================================

// default_archive_options
//   function: a freshly default-constructed option set -- the baseline every
// "did anything move?" check compares against.
//
// Parameter(s):
//   none.
// Return:
//   a default-constructed archive_options.
D_INLINE archive_options
default_archive_options()
{
    return archive_options();
}

// options_are_default
//   function: whether an option set still matches a freshly constructed one,
// i.e. nothing has been touched anywhere in the tree.
//
// Parameter(s):
//   _opt: the option set to inspect.
// Return:
//   true when the set is pristine.
D_INLINE bool
options_are_default(
    const archive_options& _opt
)
{
    const archive_options fresh;

    return archive_options_equal(_opt, fresh);
}

// codec_is_default
//   function: whether the embedded codec block is still pristine.  Useful for
// the common assertion that a call touched the container knobs but left the
// stream tuning alone.
//
// Parameter(s):
//   _opt: the option set to inspect.
// Return:
//   true when the codec block is untouched.
D_INLINE bool
codec_is_default(
    const archive_options& _opt
)
{
    const compress_options fresh;

    return compress_options_equal(_opt.codec, fresh);
}


// =============================================================================
// IV.  DIFF DESCRIPTION
// =============================================================================

// describe_option_diff
//   function: a human-readable, comma-separated list of the fields that differ
// between two option sets, for use in a failing check's message.  Archive-level
// and per-format fields are named individually; the codec's per-codec blocks
// are reported at block granularity with codec.level called out by name.
//
// Parameter(s):
//   _a: the first option set.
//   _b: the second option set.
// Return:
//   the field list, or an empty string when the two sets are identical.
D_INLINE std::string
describe_option_diff(
    const archive_options& _a,
    const archive_options& _b
)
{
    std::string out;

    // append
    //   a tiny local accumulator; kept as a lambda-free helper for C++98.
    struct add
    {
        static void
        field(
            std::string& _out,
            const char*  _name
        )
        {
            if (!_out.empty())
            {
                _out += ", ";
            }
            _out += _name;

            return;
        }
    };

    // archive-level
    if (_a.level                != _b.level)                { add::field(out, "level"); }
    if (_a.store_only           != _b.store_only)           { add::field(out, "store_only"); }
    if (_a.comment              != _b.comment)              { add::field(out, "comment"); }
    if (_a.preserve_permissions != _b.preserve_permissions) { add::field(out, "preserve_permissions"); }
    if (_a.preserve_mtime       != _b.preserve_mtime)       { add::field(out, "preserve_mtime"); }

    // embedded codec: level by name, the rest by block
    if (_a.codec.level != _b.codec.level)
    {
        add::field(out, "codec.level");
    }
    if (!deflate_options_equal(_a.codec.deflate, _b.codec.deflate)) { add::field(out, "codec.deflate"); }
    if (!bzip2_options_equal(_a.codec.bzip2, _b.codec.bzip2))       { add::field(out, "codec.bzip2"); }
    if (!lzma_options_equal(_a.codec.lzma, _b.codec.lzma))          { add::field(out, "codec.lzma"); }
    if (!zstd_options_equal(_a.codec.zstd, _b.codec.zstd))          { add::field(out, "codec.zstd"); }
    if (!lz4_options_equal(_a.codec.lz4, _b.codec.lz4))             { add::field(out, "codec.lz4"); }
    if (!brotli_options_equal(_a.codec.brotli, _b.codec.brotli))    { add::field(out, "codec.brotli"); }

    // zip block
    if (_a.zip.method     != _b.zip.method)     { add::field(out, "zip.method"); }
    if (_a.zip.encryption != _b.zip.encryption) { add::field(out, "zip.encryption"); }
    if (_a.zip.password   != _b.zip.password)   { add::field(out, "zip.password"); }
    if (_a.zip.zip64      != _b.zip.zip64)      { add::field(out, "zip.zip64"); }
    if (_a.zip.utf8_names != _b.zip.utf8_names) { add::field(out, "zip.utf8_names"); }

    // tar block
    if (_a.tar.format        != _b.tar.format)        { add::field(out, "tar.format"); }
    if (_a.tar.numeric_owner != _b.tar.numeric_owner) { add::field(out, "tar.numeric_owner"); }

    // gz block
    if (_a.gz.store_name    != _b.gz.store_name)    { add::field(out, "gz.store_name"); }
    if (_a.gz.store_mtime   != _b.gz.store_mtime)   { add::field(out, "gz.store_mtime"); }
    if (_a.gz.original_name != _b.gz.original_name) { add::field(out, "gz.original_name"); }

    // sevenzip block
    if (_a.sevenzip.method             != _b.sevenzip.method)             { add::field(out, "sevenzip.method"); }
    if (_a.sevenzip.solid              != _b.sevenzip.solid)              { add::field(out, "sevenzip.solid"); }
    if (_a.sevenzip.header_compression != _b.sevenzip.header_compression) { add::field(out, "sevenzip.header_compression"); }
    if (_a.sevenzip.header_encryption  != _b.sevenzip.header_encryption)  { add::field(out, "sevenzip.header_encryption"); }
    if (_a.sevenzip.password           != _b.sevenzip.password)           { add::field(out, "sevenzip.password"); }
    if (_a.sevenzip.threads            != _b.sevenzip.threads)            { add::field(out, "sevenzip.threads"); }

    // rar block
    if (_a.rar.level           != _b.rar.level)           { add::field(out, "rar.level"); }
    if (_a.rar.solid           != _b.rar.solid)           { add::field(out, "rar.solid"); }
    if (_a.rar.recovery_record != _b.rar.recovery_record) { add::field(out, "rar.recovery_record"); }
    if (_a.rar.password        != _b.rar.password)        { add::field(out, "rar.password"); }

    return out;
}

// describe_diff_from_default
//   function: the same field list, taken against a freshly constructed option
// set -- "what has this set changed from pristine?".
//
// Parameter(s):
//   _opt: the option set to inspect.
// Return:
//   the field list, or an empty string when the set is pristine.
D_INLINE std::string
describe_diff_from_default(
    const archive_options& _opt
)
{
    const archive_options fresh;

    return describe_option_diff(_opt, fresh);
}


// =============================================================================
// V.   OPTION BUILDERS
// =============================================================================
// Small presets for the option sets suites reach for most.  Each starts from a
// pristine set and moves exactly one thing, so a caller can assert both the
// intended change and that nothing else drifted.

// store_only_options
//   function: a pristine set with the archive-level store_only flag raised.
//
// Parameter(s):
//   none.
// Return:
//   the constructed option set.
D_INLINE archive_options
store_only_options()
{
    archive_options opt;

    opt.store_only = true;

    return opt;
}

// level_options
//   function: a pristine set with the generic effort set.
//
// Parameter(s):
//   _level: the effort to record in the archive-level `level` field.
// Return:
//   the constructed option set.
D_INLINE archive_options
level_options(
    int _level
)
{
    archive_options opt;

    opt.level = _level;

    return opt;
}

// codec_level_options
//   function: a pristine set with the EMBEDDED codec's level set and the
// archive-level `level` left at its default.  Pairs with level_options for
// testing which of the two a call site actually consults.
//
// Parameter(s):
//   _level: the effort to record in codec.level.
// Return:
//   the constructed option set.
D_INLINE archive_options
codec_level_options(
    int _level
)
{
    archive_options opt;

    opt.codec.level = _level;

    return opt;
}

// zip_method_options
//   function: a pristine set with the ZIP container method selected.
//
// Parameter(s):
//   _method: the per-entry ZIP method to request.
// Return:
//   the constructed option set.
D_INLINE archive_options
zip_method_options(
    zip_method _method
)
{
    archive_options opt;

    opt.zip.method = _method;

    return opt;
}

// zip_encrypted_options
//   function: a pristine set requesting ZIP encryption with a passphrase.
//
// Parameter(s):
//   _scheme:   the encryption scheme to request.
//   _password: the passphrase to carry.
// Return:
//   the constructed option set.
D_INLINE archive_options
zip_encrypted_options(
    zip_encryption    _scheme,
    const std::string& _password
)
{
    archive_options opt;

    opt.zip.encryption = _scheme;
    opt.zip.password   = _password;

    return opt;
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_ARCHIVE_OPTIONS_
