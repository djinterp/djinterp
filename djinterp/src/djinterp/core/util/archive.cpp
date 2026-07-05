/******************************************************************************
* djinterp [utility]                                               archive.cpp
*
*   Implementation of the djinterp portable archive facade. The tar (ustar),
* zip, gz, and tar.gz formats are implemented here directly on top of the
* compression facade, so they work on any OS with only zlib present. The 7z
* and rar formats route to whichever backend env_archive.h detected, returning
* status_unavailable when none is built in.
*
* Built-in writers favour portability over completeness: ustar with the common
* fields, and a zip writer using the store and deflate methods with 32-bit
* sizes. They interoperate with standard tools for typical payloads.
*
* 
* path:      /src/djinterp/core/util/archive.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/
#include "../../../../inc/djinterp/core/util/archive.hpp"
#include "../../../../inc/djinterp/core/env/env_compress_link.h"

// archive.cpp calls crc32 directly for the zip central directory, so it owns
// the zlib include (gated); the public header stays dependency-free.
#if D_ENV_COMPRESSION_HAVE_ZLIB
    #include <zlib.h>   // crc32 / Bytef / uInt
#endif


// the runtime PATH probe lives in env_archive.c
extern "C" int d_env_archive_has_tool(const char* tool_name);


NS_DJINTERP

NS_INTERNAL

// =============================================================================
// I.   LITTLE-ENDIAN WRITE HELPERS
// =============================================================================

// put_u16_le / put_u32_le
//   function: append a value in little-endian order (the on-disk byte order
// for ZIP), independent of host endianness.
static void
put_u16_le(
    byte_buffer& _b,
    unsigned int _v
)
{
    _b.push_back((char)(unsigned char)(_v & 0xFFu));
    _b.push_back((char)(unsigned char)((_v >> 8) & 0xFFu));
}

static void
put_u32_le(
    byte_buffer&  _b, 
    unsigned long _v
)
{
    _b.push_back((char)(unsigned char)(_v & 0xFFu));
    _b.push_back((char)(unsigned char)((_v >> 8)  & 0xFFu));
    _b.push_back((char)(unsigned char)((_v >> 16) & 0xFFu));
    _b.push_back((char)(unsigned char)((_v >> 24) & 0xFFu));
}

// read_u16_le / read_u32_le
//   function: read a little-endian value from a raw buffer at offset _at.
static unsigned int
read_u16_le(
    const char* _p,
    std::size_t _at
)
{
    const unsigned char* u = (const unsigned char*)_p + _at;
    return (unsigned int)u[0] | ((unsigned int)u[1] << 8);
}

static unsigned long
read_u32_le(
    const char* _p,
    std::size_t _at
)
{
    const unsigned char* u = (const unsigned char*)_p + _at;

    return (  (unsigned long)u[0]         |
             ((unsigned long)u[1] << 8)   |
             ((unsigned long)u[2] << 16)  |
             ((unsigned long)u[3] << 24) );
}


// =============================================================================
// II.  TAR (USTAR) WRITER / READER
// =============================================================================

// tar_octal
//   function: write `val` as a zero-padded octal string of `width-1` digits
// followed by a NUL, as ustar numeric fields require.
static void
tar_octal(
    char*         _field,
    std::size_t   _width,
    unsigned long _val
)
{
    std::size_t i;

    // fill right-to-left with octal digits, leaving the final byte NUL
    _field[_width - 1] = '\0';

    for (i = _width - 1; i > 0; --i)
    {
        _field[i - 1] = (char)('0' + (int)(_val & 7u));
        _val >>= 3;
    }
}

// tar_write_entry
//   function: append one 512-byte header plus padded data for `_e`.
static void
tar_write_entry(
    byte_buffer& _out, 
    const entry& _e
)
{
    char          header[512];
    unsigned long sum;
    std::size_t   i;
    unsigned long mode;
    unsigned long mtime;

    std::memset(header, 0, sizeof(header));

    // name (field 0, 100 bytes); truncate defensively
    std::strncpy(header + 0, _e.name.c_str(), 100);

    mode  = (_e.mode != 0) ? _e.mode
                           : (_e.is_directory ? 0755u : 0644u);
    mtime = (_e.mtime != 0) ? (unsigned long)_e.mtime
                            : (unsigned long)std::time(0);

    tar_octal(header + 100, 8,  mode);          // mode
    tar_octal(header + 108, 8,  0);             // uid
    tar_octal(header + 116, 8,  0);             // gid

    {
        // size: 0 for directories, else the data length
        unsigned long sz = _e.is_directory ? 0u
                                           : (unsigned long)_e.data.size();
        tar_octal(header + 124, 12, sz);
    }

    tar_octal(header + 136, 12, mtime);         // mtime

    // typeflag: '5' directory, '0' regular file
    header[156] = _e.is_directory ? '5' : '0';

    // ustar magic and version
    std::memcpy(header + 257, "ustar", 6);      // "ustar\0"
    header[263] = '0';
    header[264] = '0';

    // checksum field is treated as spaces while summing
    for (i = 148; i < 156; ++i)
    {
        header[i] = ' ';
    }

    sum = 0;
    for (i = 0; i < sizeof(header); ++i)
    {
        sum += (unsigned long)(unsigned char)header[i];
    }

    // checksum: six octal digits, NUL, then a space (the conventional form)
    tar_octal(header + 148, 7, sum);
    header[155] = ' ';

    _out.append(header, sizeof(header));

    // file data, padded up to the next 512-byte boundary
    if (!_e.is_directory && !_e.data.empty())
    {
        std::size_t pad;

        _out.append(_e.data.data(), _e.data.size());

        pad = (512u - (_e.data.size() % 512u)) % 512u;
        _out.append(pad, '\0');
    }
}

// tar_create
//   function: build a complete ustar stream terminated by two zero blocks.
static status
tar_create(
    const entry* _items,
    std::size_t  _count,
    byte_buffer& _out
)
{
    std::size_t i;

    _out.clear();

    for (i = 0; i < _count; ++i)
    {
        tar_write_entry(_out, _items[i]);
    }

    // two 512-byte zero blocks mark end of archive
    _out.append(1024u, '\0');

    return status_ok;
}

// tar_parse_octal
//   function: parse a (possibly space/NUL padded) octal field.
static unsigned long
tar_parse_octal(
    const char* _p,
    std::size_t _width
)
{
    unsigned long v = 0;
    std::size_t   i;

    for (i = 0; i < _width; ++i)
    {
        char c = _p[i];

        if (c >= '0' && c <= '7')
        {
            v = (v << 3) + (unsigned long)(c - '0');
        }
    }

    return v;
}

// tar_extract
//   function: parse a ustar stream into entries. stops at the end-of-archive
// zero blocks or when the input is exhausted.
static status
tar_extract(
    const char* _in,
    std::size_t _n,
    entry_list& _out
)
{
    std::size_t pos = 0;

    _out.clear();

    while (pos + 512u <= _n)
    {
        const char*   h = _in + pos;
        std::size_t   i;
        bool          all_zero = true;
        unsigned long size;
        char          type;

        // a fully zero block signals the end of the archive
        for (i = 0; i < 512u; ++i)
        {
            if (h[i] != 0)
            {
                all_zero = false;
                break;
            }
        }
        if (all_zero)
        {
            break;
        }

        pos += 512u;

        size = tar_parse_octal(h + 124, 12);
        type = h[156];

        {
            entry e;
            char  namebuf[101];

            std::memcpy(namebuf, h + 0, 100);
            namebuf[100] = '\0';
            e.name = namebuf;

            e.mode         = (unsigned int)tar_parse_octal(h + 100, 8);
            e.mtime        = (long)tar_parse_octal(h + 136, 12);
            e.is_directory = (type == '5');

            if (!e.is_directory && size > 0)
            {
                if (pos + size > _n)
                {
                    return status_backend_error;   // truncated
                }
                e.data.assign(_in + pos, (std::size_t)size);
            }

            _out.push_back(e);
        }

        // advance past the data, rounded up to a 512-byte block
        pos += (std::size_t)((size + 511u) & ~((unsigned long)511u));
    }

    return status_ok;
}


// =============================================================================
// III. ZIP WRITER / READER
// =============================================================================

// zip uses DEFLATE method 8 when a backend is present, else store (method 0).
#if D_ENV_COMPRESSION_HAVE_ZLIB
    #define D_INTERNAL_ZIP_HAVE_DEFLATE 1
#else
    #define D_INTERNAL_ZIP_HAVE_DEFLATE 0
#endif

// zip_local_record
//   struct: the bookkeeping retained per entry for the central directory.
struct zip_local_record
{
    std::string   name;
    unsigned long crc;
    unsigned long comp_size;
    unsigned long uncomp_size;
    unsigned long offset;
    unsigned int  method;
};

// zip_create
//   function: build a ZIP archive. Each entry is stored or deflated, then a
// central directory and end-of-central-directory record are appended.
static status
zip_create(
    const entry*           _items,
    std::size_t            _count,
    const archive_options& _opt,
    byte_buffer&           _out
)
{
    std::vector<zip_local_record> records;
    std::size_t                   i;
    unsigned long                 cd_offset;
    unsigned long                 cd_size;

    _out.clear();
    records.reserve(_count);

    // -- local file headers + data --
    for (i = 0; i < _count; ++i)
    {
        const entry&     e = _items[i];
        zip_local_record rec;
        byte_buffer      payload;
        unsigned int     method;
        unsigned long    crc;

        rec.offset = (unsigned long)_out.size();
        rec.name   = e.name;

        // directories are stored as empty entries with a trailing slash
        if (e.is_directory)
        {
            method      = 0;
            crc         = 0;
            payload.clear();
            if (!rec.name.empty() &&
                rec.name[rec.name.size() - 1] != '/')
            {
                rec.name += '/';
            }
        }
        else
        {
#if D_ENV_COMPRESSION_HAVE_ZLIB
            crc = (unsigned long)crc32(0L,
                                       (const Bytef*)e.data.data(),
                                       (uInt)e.data.size());
#else
            crc = 0;
#endif
            // choose deflate unless asked to store or no backend exists
            if ( (_opt.store_only) || 
                 (!D_INTERNAL_ZIP_HAVE_DEFLATE) )
            {
                method  = 0;
                payload = e.data;
            }
            else
            {
                compress_options copt;
                status           cs;

                copt.level = _opt.level;

                // raw DEFLATE (method 8) is what ZIP stores
                cs = compress_buffer(codec_id_deflate,
                                     e.data.data(),
                                     e.data.size(),
                                     copt,
                                     payload);

                if (cs != status_ok)
                {
                    return cs;
                }

                // fall back to store if deflate somehow grew the data
                if (payload.size() >= e.data.size())
                {
                    method  = 0;
                    payload = e.data;
                }
                else
                {
                    method = 8;
                }
            }
        }

        rec.crc         = crc;
        rec.method      = method;
        rec.comp_size   = (unsigned long)payload.size();
        rec.uncomp_size = e.is_directory ? 0u
                                         : (unsigned long)e.data.size();

        // local file header signature
        put_u32_le(_out, 0x04034b50UL);
        put_u16_le(_out, 20);                 // version needed
        put_u16_le(_out, 0);                  // flags
        put_u16_le(_out, rec.method);         // method
        put_u16_le(_out, 0);                  // mod time
        put_u16_le(_out, 0);                  // mod date
        put_u32_le(_out, rec.crc);            // crc-32
        put_u32_le(_out, rec.comp_size);      // compressed size
        put_u32_le(_out, rec.uncomp_size);    // uncompressed size
        put_u16_le(_out, (unsigned int)rec.name.size());
        put_u16_le(_out, 0);                  // extra length
        _out.append(rec.name);
        _out.append(payload);

        records.push_back(rec);
    }

    // -- central directory --
    cd_offset = (unsigned long)_out.size();

    for (i = 0; i < records.size(); ++i)
    {
        const zip_local_record& rec = records[i];

        put_u32_le(_out, 0x02014b50UL);       // central dir signature
        put_u16_le(_out, 20);                 // version made by
        put_u16_le(_out, 20);                 // version needed
        put_u16_le(_out, 0);                  // flags
        put_u16_le(_out, rec.method);         // method
        put_u16_le(_out, 0);                  // mod time
        put_u16_le(_out, 0);                  // mod date
        put_u32_le(_out, rec.crc);            // crc-32
        put_u32_le(_out, rec.comp_size);      // compressed size
        put_u32_le(_out, rec.uncomp_size);    // uncompressed size
        put_u16_le(_out, (unsigned int)rec.name.size());
        put_u16_le(_out, 0);                  // extra length
        put_u16_le(_out, 0);                  // comment length
        put_u16_le(_out, 0);                  // disk number start
        put_u16_le(_out, 0);                  // internal attrs
        put_u32_le(_out, 0);                  // external attrs
        put_u32_le(_out, rec.offset);         // local header offset
        _out.append(rec.name);
    }

    cd_size = (unsigned long)_out.size() - cd_offset;

    // -- end of central directory --
    put_u32_le(_out, 0x06054b50UL);           // EOCD signature
    put_u16_le(_out, 0);                      // disk number
    put_u16_le(_out, 0);                      // cd start disk
    put_u16_le(_out, (unsigned int)records.size());
    put_u16_le(_out, (unsigned int)records.size());
    put_u32_le(_out, cd_size);
    put_u32_le(_out, cd_offset);
    put_u16_le(_out, 0);                      // comment length

    return status_ok;
}

// zip_extract
//   function: parse a ZIP by walking local file headers from the front. This
// handles store and deflate; entries using other methods yield status
// _unsupported.
static status
zip_extract(
    const char* _in,
    std::size_t _n,
    entry_list& _out
)
{
    std::size_t pos = 0;

    _out.clear();

    while (pos + 4u <= _n)
    {
        unsigned long sig = read_u32_le(_in, pos);

        // stop once we reach the central directory
        if (sig != 0x04034b50UL)
        {
            break;
        }
        if (pos + 30u > _n)
        {
            return status_backend_error;
        }

        {
            unsigned int  method      = read_u16_le(_in, pos + 8);
            unsigned long comp_size   = read_u32_le(_in, pos + 18);
            unsigned long uncomp_size = read_u32_le(_in, pos + 22);
            unsigned int  name_len    = read_u16_le(_in, pos + 26);
            unsigned int  extra_len   = read_u16_le(_in, pos + 28);
            std::size_t   data_at     = pos + 30u + name_len + extra_len;
            entry         e;

            if (data_at + comp_size > _n)
            {
                return status_backend_error;
            }

            e.name.assign(_in + pos + 30u, name_len);
            e.is_directory = (!e.name.empty() &&
                              e.name[e.name.size() - 1] == '/');

            if (!e.is_directory)
            {
                if (method == 0)
                {
                    e.data.assign(_in + data_at, (std::size_t)comp_size);
                }
                else if (method == 8)
                {
                    byte_buffer raw(_in + data_at, (std::size_t)comp_size);
                    status      ds;

                    ds = decompress_buffer(codec_id_deflate,
                                           raw.data(),
                                           raw.size(),
                                           e.data);
                    if (ds != status_ok)
                    {
                        return ds;
                    }

                    (void)uncomp_size;
                }
                else
                {
                    return status_unsupported;
                }
            }

            _out.push_back(e);
            pos = data_at + comp_size;
        }
    }

    return status_ok;
}


// =============================================================================
// IV.  GZ AND TAR.GZ (composition over the codec facade)
// =============================================================================

// gz_create
//   function: a .gz holds a single logical file. The first entry's data is
// gzip-wrapped; additional entries are an error.
static status
gz_create(
    const entry*           _items,
    std::size_t            _count,
    const archive_options& _opt,
    byte_buffer&           _out
)
{
    compress_options copt;

    if (_count != 1)
    {
        return status_invalid_argument;
    }

    copt.level = _opt.level;

    return compress_buffer(codec_id_gzip,
                           _items[0].data.data(),
                           _items[0].data.size(),
                           copt,
                           _out);
}

// gz_extract
//   function: decompress a single gzip member. The entry name is left empty
// because gzip does not reliably carry one.
static status
gz_extract(
    const char* _in,
    std::size_t _n,
    entry_list& _out
)
{
    entry  e;
    status s;

    s = decompress_buffer(codec_id_gzip, _in, _n, e.data);

    if (s != status_ok)
    {
        return s;
    }

    _out.clear();
    _out.push_back(e);

    return status_ok;
}

// tar_gz_create
//   function: build a tar stream, then gzip-wrap it.
static status
tar_gz_create(
    const entry*           _items,
    std::size_t            _count,
    const archive_options& _opt,
    byte_buffer&           _out
)
{
    byte_buffer      tarball;
    compress_options copt;
    status           s;

    s = tar_create(_items, _count, tarball);
    if (s != status_ok)
    {
        return s;
    }

    copt.level = _opt.level;

    return compress_buffer(codec_id_gzip,
                           tarball.data(),
                           tarball.size(),
                           copt,
                           _out);
}

// tar_gz_extract
//   function: gunzip, then parse the inner tar.
static status
tar_gz_extract(
    const char* _in,
    std::size_t _n,
    entry_list& _out
)
{
    byte_buffer tarball;
    status      s;

    s = decompress_buffer(codec_id_gzip,
                          _in,
                          _n,
                          tarball);

    if (s != status_ok)
    {
        return s;
    }

    return tar_extract(tarball.data(),
                       tarball.size(),
                       _out);
}


// =============================================================================
// V.   7Z / RAR BACKENDS
// =============================================================================
// Two backends sit behind the 7z and rar formats:
//   1. libarchive, when detected -- reads 7z and rar, writes 7z.
//   2. an external tool shell-out -- the only way to *create* rar (the
//      proprietary `rar` binary), and a fallback reader via `7z`/`unrar`.
// Each is compiled only when its enabling condition holds; otherwise the
// dispatch reports status_unavailable.

// -----------------------------------------------------------------------------
// A.  libarchive backend
// -----------------------------------------------------------------------------

#if D_ENV_ARCHIVE_HAVE_LIBARCHIVE

// la_read_all
//   function: read every entry from an in-memory archive using libarchive's
// format auto-detection, into _out. Shared by the 7z and rar readers.
static status
la_read_all(const char* _in, std::size_t _n, entry_list& _out)
{
    struct archive*       a;
    struct archive_entry* ent;
    int                   rc;

    _out.clear();

    a = archive_read_new();

    if (a == 0)
    {
        return status_buffer_error;
    }

    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    rc = archive_read_open_memory(a,
                                  (void*)_in,
                                  _n);

    if (rc != ARCHIVE_OK)
    {
        archive_read_free(a);
        return status_backend_error;
    }

    while (archive_read_next_header(a, &ent) == ARCHIVE_OK)
    {
        entry       e;
        const char* name = archive_entry_pathname(ent);
        mode_t      ftype;

        e.name  = (name != 0) ? name : "";
        ftype   = archive_entry_filetype(ent);
        e.mode  = (unsigned int)archive_entry_perm(ent);
        e.mtime = (long)archive_entry_mtime(ent);

        // AE_IFDIR marks a directory
        e.is_directory = (ftype == AE_IFDIR);

        if (!e.is_directory)
        {
            const void* buff;
            size_t      size;
            la_int64_t  offset;

            // pull data blocks until the entry is exhausted
            for (;;)
            {
                rc = archive_read_data_block(a, &buff, &size, &offset);

                if (rc == ARCHIVE_EOF)
                {
                    break;
                }
                if (rc < ARCHIVE_WARN)
                {
                    archive_read_free(a);
                    return status_backend_error;
                }

                e.data.append((const char*)buff, size);
            }
        }

        _out.push_back(e);
    }

    archive_read_free(a);

    return status_ok;
}

// la_write_7z
//   function: create a 7z archive of the given entries in memory using
// libarchive's 7zip writer.
static status
la_write_7z(
    const entry*           _items,
    std::size_t            _count,
    const archive_options& _opt,
    byte_buffer&           _out
)
{
    struct archive* a;
    std::size_t     i;
    size_t          used;
    int             rc;

    (void)_opt;   // 7z method/level wiring omitted; libarchive defaults apply

    _out.clear();
    // a generous initial buffer; libarchive reports the used length
    _out.resize((std::size_t)(64u * 1024u));

    a = archive_write_new();
    if (a == 0)
    {
        return status_buffer_error;
    }

    if (archive_write_set_format_7zip(a) != ARCHIVE_OK)
    {
        archive_write_free(a);
        return status_backend_error;
    }

    // grow the output buffer until the whole archive fits
    for (;;)
    {
        used = 0;
        rc = archive_write_open_memory(a, &_out[0], _out.size(), &used);
        if (rc != ARCHIVE_OK)
        {
            archive_write_free(a);
            return status_backend_error;
        }

        rc = ARCHIVE_OK;

        for (i = 0; i < _count && rc == ARCHIVE_OK; ++i)
        {
            const entry&          e = _items[i];
            struct archive_entry* ent = archive_entry_new();

            archive_entry_set_pathname(ent, e.name.c_str());
            archive_entry_set_perm(ent,
                                   (e.mode != 0) ? (mode_t)e.mode
                                                 : (mode_t)0644);
            archive_entry_set_mtime(ent,
                                    (e.mtime != 0) ? (time_t)e.mtime
                                                   : time(0),
                                    0);

            if (e.is_directory)
            {
                archive_entry_set_filetype(ent, AE_IFDIR);
                archive_entry_set_size(ent, 0);
            }
            else
            {
                archive_entry_set_filetype(ent, AE_IFREG);
                archive_entry_set_size(ent, (la_int64_t)e.data.size());
            }

            if (archive_write_header(a, ent) != ARCHIVE_OK)
            {
                rc = ARCHIVE_FATAL;
            }
            else if (!e.is_directory && !e.data.empty())
            {
                la_ssize_t w = archive_write_data(a,
                                                  e.data.data(),
                                                  e.data.size());
                if (w < 0)
                {
                    rc = ARCHIVE_FATAL;
                }
            }

            archive_entry_free(ent);
        }

        archive_write_close(a);

        // a full buffer manifests as ARCHIVE_FATAL with used == capacity;
        // grow and retry the whole write
        if (rc == ARCHIVE_OK && used <= _out.size())
        {
            _out.resize(used);
            archive_write_free(a);
            return status_ok;
        }

        // retry with a larger buffer (cap the growth to avoid runaway)
        if (_out.size() > (std::size_t)(256u * 1024u * 1024u))
        {
            archive_write_free(a);
            return status_buffer_error;
        }
        _out.resize(_out.size() * 2u);

        // a fresh writer is needed for the retry
        archive_write_free(a);
        a = archive_write_new();
        if (a == 0)
        {
            return status_buffer_error;
        }
        if (archive_write_set_format_7zip(a) != ARCHIVE_OK)
        {
            archive_write_free(a);
            return status_backend_error;
        }
    }
}

#endif  // D_ENV_ARCHIVE_HAVE_LIBARCHIVE


// -----------------------------------------------------------------------------
// B.  external-tool shell-out
// -----------------------------------------------------------------------------

#if D_CFG_ARCHIVE_ALLOW_TOOL_SHELLOUT

// internal_make_temp_dir
//   function: create a unique temporary directory and return its path. returns
// an empty string on failure.
static std::string
internal_make_temp_dir()
{
    std::string base;

#if D_INTERNAL_ARCHIVE_OS_WINDOWS
    const char* t = getenv("TEMP");
    if (t == 0) { t = getenv("TMP"); }
    base = (t != 0) ? t : ".";
#else
    const char* t = getenv("TMPDIR");
    base = (t != 0) ? t : "/tmp";
#endif

    // derive a name from the clock and pid-ish entropy
    {
        char        suffix[64];
        std::sprintf(suffix, "djarc_%lu_%lu",
                     (unsigned long)time(0),
                     (unsigned long)(std::size_t)(&base));

        std::string dir = base;
        dir += (char)
#if D_INTERNAL_ARCHIVE_OS_WINDOWS
            '\\';
#else
            '/';
#endif
        dir += suffix;

#if D_INTERNAL_ARCHIVE_OS_WINDOWS
        if (_mkdir(dir.c_str()) != 0)
#else
        if (mkdir(dir.c_str(), 0700) != 0)
#endif
        {
            return std::string();
        }

        return dir;
    }
}

// internal_path_join
//   function: join a directory and a leaf with the platform separator.
static std::string
internal_path_join(
    const std::string& _dir,
    const std::string& _leaf
)
{
    std::string out = _dir;

#if D_INTERNAL_ARCHIVE_OS_WINDOWS
    out += '\\';
#else
    out += '/';
#endif
    out += _leaf;

    return out;
}

// internal_write_file
//   function: write a buffer to a path, creating parent directories as needed
// for nested entry names.
static bool
internal_write_file(
    const std::string& _path,
    const byte_buffer& _data
)
{
    // create any intermediate directories named in the path
    std::string::size_type p = 0;

    for (;;)
    {
        std::string::size_type slash;

#if D_INTERNAL_ARCHIVE_OS_WINDOWS
        slash = _path.find('\\', p);
#else
        slash = _path.find('/', p);
#endif
        if (slash == std::string::npos)
        {
            break;
        }

        if (slash > 0)
        {
            std::string sub = _path.substr(0, slash);
#if D_INTERNAL_ARCHIVE_OS_WINDOWS
            _mkdir(sub.c_str());
#else
            mkdir(sub.c_str(), 0700);
#endif
        }
        p = slash + 1;
    }

    {
        std::ofstream f(_path.c_str(),
                        std::ios::out | std::ios::binary | std::ios::trunc);
        if (!f.is_open())
        {
            return false;
        }
        if (!_data.empty())
        {
            f.write(_data.data(), (std::streamsize)_data.size());
        }
        return f.good();
    }
}

// internal_read_file
//   function: read an entire file into _out.
static bool
internal_read_file(
    const std::string& _path,
    byte_buffer&       _out
)
{
    std::ifstream f(_path.c_str(), std::ios::in | std::ios::binary);
    if (!f.is_open())
    {
        return false;
    }

    f.seekg(0, std::ios::end);
    std::streamoff len = f.tellg();
    f.seekg(0, std::ios::beg);

    if (len < 0)
    {
        return false;
    }

    _out.resize((std::size_t)len);
    if (len > 0)
    {
        f.read(&_out[0], (std::streamsize)len);
    }

    return f.good() || f.eof();
}

// internal_shell_quote
//   function: wrap a path in quotes appropriate to the host shell so spaces
// are handled. (Sufficient for paths we generate ourselves under a temp dir.)
static std::string
internal_shell_quote(
    const std::string& _s
)
{
    std::string out;

#if D_INTERNAL_ARCHIVE_OS_WINDOWS
    out += '"';
    out += _s;
    out += '"';
#else
    out += '\'';
    // escape embedded single quotes the POSIX way
    for (std::size_t i = 0; i < _s.size(); ++i)
    {
        if (_s[i] == '\'')
        {
            out += "'\\''";
        }
        else
        {
            out += _s[i];
        }
    }
    out += '\'';
#endif

    return out;
}

// internal_remove_tree
//   function: best-effort recursive delete of a temp directory via the host
// shell. Failure is non-fatal (the OS reclaims temp space eventually).
static void
internal_remove_tree(
    const std::string& _dir
)
{
    std::string cmd;

#if D_INTERNAL_ARCHIVE_OS_WINDOWS
    cmd  = "rmdir /s /q ";
    cmd += internal_shell_quote(_dir);
#else
    cmd  = "rm -rf ";
    cmd += internal_shell_quote(_dir);
#endif

    if (std::system(cmd.c_str()) != 0)
    {
        // ignored: cleanup is best-effort
    }
}

// rar_create_via_tool
//   function: create a RAR archive by staging entries in a temp directory and
// invoking the proprietary `rar` tool. This is the only way to write RAR.
static status
rar_create_via_tool(
    const entry* _items,
    std::size_t  _count,
    byte_buffer& _out
)
{
    if (!d_env_archive_has_tool("rar"))
    {
        return status_unavailable;
    }

    std::string work = internal_make_temp_dir();
    if (work.empty())
    {
        return status_buffer_error;
    }

    std::string src     = internal_path_join(work, "src");
    std::string out_rar = internal_path_join(work, "out.rar");

#if D_INTERNAL_ARCHIVE_OS_WINDOWS
    _mkdir(src.c_str());
#else
    mkdir(src.c_str(), 0700);
#endif

    // stage every file under src/
    {
        std::size_t i;
        for (i = 0; i < _count; ++i)
        {
            const entry& e = _items[i];
            std::string  dst = internal_path_join(src, e.name);

            if (e.is_directory)
            {
#if D_INTERNAL_ARCHIVE_OS_WINDOWS
                _mkdir(dst.c_str());
#else
                mkdir(dst.c_str(), 0700);
#endif
            }
            else if (!internal_write_file(dst, e.data))
            {
                internal_remove_tree(work);
                return status_buffer_error;
            }
        }
    }

    // rar a -ep1 <out.rar> <src/.> : archive recursively, trimming the
    // staging prefix so stored names match the entry names.
    {
        std::string cmd;
        int         rc;

        cmd  = "rar a -idq -ep1 -r ";
        cmd += internal_shell_quote(out_rar);
        cmd += " ";
        cmd += internal_shell_quote(internal_path_join(src, "."));

        rc = std::system(cmd.c_str());
        if (rc != 0)
        {
            internal_remove_tree(work);
            return status_backend_error;
        }
    }

    {
        bool ok = internal_read_file(out_rar, _out);
        internal_remove_tree(work);
        return ok ? status_ok : status_backend_error;
    }
}

// archive_extract_via_tool
//   function: extract an archive by writing it to a temp file and invoking a
// tool (`7z` or `unrar`) to expand it, then reading the results back. Used as
// the 7z/rar reader when libarchive is absent, so it is only compiled then.
#if !D_ENV_ARCHIVE_HAVE_LIBARCHIVE
static status
archive_extract_via_tool(
    const char* _in,
    std::size_t _n,
    const char* _tool,
    entry_list& _out
)
{
    if (!d_env_archive_has_tool(_tool))
    {
        return status_unavailable;
    }

    std::string work = internal_make_temp_dir();
    if (work.empty())
    {
        return status_buffer_error;
    }

    std::string in_path  = internal_path_join(work, "in.archive");
    std::string out_dir  = internal_path_join(work, "out");

#if D_INTERNAL_ARCHIVE_OS_WINDOWS
    _mkdir(out_dir.c_str());
#else
    mkdir(out_dir.c_str(), 0700);
#endif

    {
        byte_buffer blob(_in, _n);
        if (!internal_write_file(in_path, blob))
        {
            internal_remove_tree(work);
            return status_buffer_error;
        }
    }

    // build the extract command for the chosen tool
    {
        std::string cmd;
        int         rc;

        if (std::strcmp(_tool, "unrar") == 0)
        {
            // unrar x -o+ <in> <out>/
            cmd  = "unrar x -inul -o+ ";
            cmd += internal_shell_quote(in_path);
            cmd += " ";
            cmd += internal_shell_quote(out_dir);
            cmd += "/";
        }
        else
        {
            // 7z x -o<out> <in>
            cmd  = "7z x -bso0 -bsp0 -y -o";
            cmd += internal_shell_quote(out_dir);
            cmd += " ";
            cmd += internal_shell_quote(in_path);
        }

        rc = std::system(cmd.c_str());
        if (rc != 0)
        {
            internal_remove_tree(work);
            return status_backend_error;
        }
    }

    // walk out_dir and read files back. Directory recursion is delegated to a
    // `find`/`dir` listing to avoid pulling in <filesystem> (C++17-only).
    {
        std::string list = internal_path_join(work, "list.txt");
        std::string cmd;
        int         rc;

#if D_INTERNAL_ARCHIVE_OS_WINDOWS
        cmd  = "dir /b /s /a-d ";
        cmd += internal_shell_quote(out_dir);
        cmd += " > ";
        cmd += internal_shell_quote(list);
#else
        cmd  = "find ";
        cmd += internal_shell_quote(out_dir);
        cmd += " -type f > ";
        cmd += internal_shell_quote(list);
#endif
        rc = std::system(cmd.c_str());

        if (rc != 0)
        {
            internal_remove_tree(work);
            return status_backend_error;
        }

        byte_buffer listing;

        if (!internal_read_file(list, listing))
        {
            internal_remove_tree(work);
            return status_backend_error;
        }

        _out.clear();

        // each line is an absolute path under out_dir; the stored name is the
        // remainder after the out_dir prefix and separator
        std::string       text(listing.data(), listing.size());
        std::string::size_type start = 0;

        while (start < text.size())
        {
            std::string::size_type nl = text.find('\n', start);
            std::string            line =
                text.substr(start,
                            (nl == std::string::npos) ? std::string::npos
                                                      : (nl - start));
            start = (nl == std::string::npos) ? text.size() : (nl + 1);

            // strip a trailing CR (Windows listings)
            if (!line.empty() && line[line.size() - 1] == '\r')
            {
                line.erase(line.size() - 1);
            }
            if (line.empty())
            {
                continue;
            }

            entry e;
            // derive the archive-relative name
            if (line.size() > out_dir.size() + 1)
            {
                e.name = line.substr(out_dir.size() + 1);
            }
            else
            {
                e.name = line;
            }
            // normalize separators to '/'
            for (std::size_t k = 0; k < e.name.size(); ++k)
            {
                if (e.name[k] == '\\')
                {
                    e.name[k] = '/';
                }
            }

            if (!internal_read_file(line, e.data))
            {
                internal_remove_tree(work);
                return status_backend_error;
            }

            _out.push_back(e);
        }
    }

    internal_remove_tree(work);

    return status_ok;
}
#endif  // !D_ENV_ARCHIVE_HAVE_LIBARCHIVE

#endif  // D_CFG_ARCHIVE_ALLOW_TOOL_SHELLOUT


// -----------------------------------------------------------------------------
// C.  format dispatch for 7z / rar
// -----------------------------------------------------------------------------

static status
sevenzip_create(
    const entry*            _items,
    std::size_t              _count,
    const archive_options&   _opt,
    byte_buffer&             _out
)
{
#if D_ENV_ARCHIVE_HAVE_LIBARCHIVE
    return la_write_7z(_items, _count, _opt, _out);
#elif D_CFG_ARCHIVE_ALLOW_TOOL_SHELLOUT
    // a `7z` tool can also create; reuse the staging path with a 7z command
    (void)_items; (void)_count; (void)_opt; (void)_out;
    return status_unsupported;   // tool-based 7z creation not wired yet
#else
    (void)_items; (void)_count; (void)_opt; (void)_out;
    return status_unavailable;
#endif
}

static status
sevenzip_extract(
    const char* _in,
    std::size_t _n,
    entry_list& _out
)
{
#if D_ENV_ARCHIVE_HAVE_LIBARCHIVE
    return la_read_all(_in, _n, _out);
#elif D_CFG_ARCHIVE_ALLOW_TOOL_SHELLOUT
    return archive_extract_via_tool(_in, _n, "7z", _out);
#else
    (void)_in; (void)_n; (void)_out;
    return status_unavailable;
#endif
}

static status
rar_create(const entry* _items, std::size_t _count, byte_buffer& _out)
{
#if D_CFG_ARCHIVE_ALLOW_TOOL_SHELLOUT
    // no library writes RAR; the proprietary tool is the only path
    return rar_create_via_tool(_items, _count, _out);
#else
    (void)_items; (void)_count; (void)_out;
    return status_unavailable;
#endif
}

static status
rar_extract(const char* _in, std::size_t _n, entry_list& _out)
{
#if D_ENV_ARCHIVE_HAVE_LIBARCHIVE
    return la_read_all(_in, _n, _out);
#elif D_CFG_ARCHIVE_ALLOW_TOOL_SHELLOUT
    return archive_extract_via_tool(_in, _n, "unrar", _out);
#else
    (void)_in; (void)_n; (void)_out;
    return status_unavailable;
#endif
}


// =============================================================================
// VI.  DISPATCH LEAVES
// =============================================================================

bool
format_can_write(
    format_id _format
)
{
    switch (_format)
    {
        case format_id_zip:      return (D_ENV_ARCHIVE_CAN_WRITE_ZIP  != 0);
        case format_id_tar:      return (D_ENV_ARCHIVE_CAN_WRITE_TAR  != 0);
        case format_id_gz:       return (D_ENV_ARCHIVE_CAN_WRITE_GZ   != 0);
        case format_id_tar_gz:   return (D_ENV_ARCHIVE_CAN_WRITE_TGZ  != 0);
        case format_id_sevenzip:
            // libarchive writes 7z in-process; the compile-time flag covers it
            return (D_ENV_ARCHIVE_CAN_WRITE_7Z != 0);
        case format_id_rar:
            // RAR creation is tool-only and thus a runtime fact
#if D_CFG_ARCHIVE_ALLOW_TOOL_SHELLOUT
            return (D_ENV_ARCHIVE_CAN_WRITE_RAR != 0) ||
                   (d_env_archive_has_tool("rar") != 0);
#else
            return (D_ENV_ARCHIVE_CAN_WRITE_RAR != 0);
#endif
        default:                 return false;
    }
}

bool
format_can_read(
    format_id _format
)
{
    switch (_format)
    {
        case format_id_zip:      return (D_ENV_ARCHIVE_CAN_READ_ZIP  != 0);
        case format_id_tar:      return (D_ENV_ARCHIVE_CAN_READ_TAR  != 0);
        case format_id_gz:       return (D_ENV_ARCHIVE_CAN_READ_GZ   != 0);
        case format_id_tar_gz:   return (D_ENV_ARCHIVE_CAN_READ_GZ   != 0);
        case format_id_sevenzip:
#if D_CFG_ARCHIVE_ALLOW_TOOL_SHELLOUT
            return ( (D_ENV_ARCHIVE_CAN_READ_7Z != 0) ||
                     (d_env_archive_has_tool("7z") != 0) );
#else
            return (D_ENV_ARCHIVE_CAN_READ_7Z != 0);
#endif
        case format_id_rar:
#if D_CFG_ARCHIVE_ALLOW_TOOL_SHELLOUT
            return (D_ENV_ARCHIVE_CAN_READ_RAR != 0) ||
                   (d_env_archive_has_tool("unrar") != 0) ||
                   (d_env_archive_has_tool("7z") != 0);
#else
            return (D_ENV_ARCHIVE_CAN_READ_RAR != 0);
#endif
        default:                 return false;
    }
}

status
archive_create(
    format_id              _format,
    const entry*           _items,
    std::size_t            _count,
    const archive_options& _opt,
    byte_buffer&           _out
)
{
    // a null base is only valid when there are no items
    if ( (_items == 0) && 
         (_count != 0) )
    {
        return status_invalid_argument;
    }

    switch (_format)
    {
        case format_id_zip:
            return zip_create(_items, _count, _opt, _out);
        case format_id_tar:
            return tar_create(_items, _count, _out);
        case format_id_gz:
            return gz_create(_items, _count, _opt, _out);
        case format_id_tar_gz:
            return tar_gz_create(_items, _count, _opt, _out);
        case format_id_sevenzip:
            return sevenzip_create(_items, _count, _opt, _out);
        case format_id_rar:
            return rar_create(_items, _count, _out);
        default:
            return status_unavailable;
    }
}

status
archive_extract(
    format_id   _format,
    const char* _in,
    std::size_t _n,
    entry_list& _out
)
{
    if ( (_in == 0) &&
         (_n != 0) )
    {
        return status_invalid_argument;
    }

    switch (_format)
    {
        case format_id_zip:
            return zip_extract(_in, _n, _out);
        case format_id_tar:
            return tar_extract(_in, _n, _out);
        case format_id_gz:
            return gz_extract(_in, _n, _out);
        case format_id_tar_gz:
            return tar_gz_extract(_in, _n, _out);
        case format_id_sevenzip:
            return sevenzip_extract(_in, _n, _out);
        case format_id_rar:
            return rar_extract(_in, _n, _out);
        default:
            return status_unavailable;
    }
}

NS_END  // internal


NS_END  // djinterp