/******************************************************************************
* djinterp [test]                                            test_zip_store.hpp
*
*   A tiny, DEPENDENCY-FREE archive writer: it bundles a set of entries into a
* single ZIP container using the STORED method (no compression), emitting the
* bytes into a byte_blob.  It exists so the report packaging path
* (test_report_runner.hpp, write_archived_report) can always produce ONE
* container file even on a build with no third-party archive backend
* (libarchive / minizip / the LZMA SDK / bit7z).  The core archive facade
* (archive.hpp) writes every format through such a backend; when none is
* present, format_is_writable<> is false and NOTHING is written - this writer
* is the fallback that closes that gap.
*
*   WHY STORED (not compressed):
*   The stored (method 0) ZIP layout is small, self-contained, and needs no
* codec - only a CRC-32, computed here.  The resulting file is a fully standard
* .zip: it opens by double-click in Windows Explorer, and with 7-Zip, unzip,
* Python's zipfile, etc.  Because the payloads here are already-rendered PDF
* reports (which are themselves compressed streams), the size cost of storing
* rather than deflating is negligible.
*
*   WHY NOT 7z:
*   Writing a valid .7z requires a real 7z encoder (the LZMA SDK, bit7z, or
* libarchive); there is no small, correct, from-scratch 7z writer.  Rather than
* emit a mislabeled container that will not open, the fallback produces a real
* .zip.  For an actual .7z, make a 7z-capable library visible to the build so
* env_archive.h detects it (D_ENV_ARCHIVE_CAN_WRITE_7Z), and the existing
* sevenzip path in archive.cpp lights up with no change to the runners.
*
*   LIMITS:
*   32-bit sizes/offsets (no zip64) - each entry and the whole container must be
* under 4 GiB, which test reports always are.  No directory entries, no
* per-entry timestamps beyond a fixed valid DOS date, no encryption.  Entry
* names are written verbatim as UTF-8 bytes (the report layer already reduces
* them to basenames).
*
*
* path:      /inc/djinterp/test/output/test_zip_store.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.04
******************************************************************************/

#ifndef DJINTERP_TEST_ZIP_STORE_
#define DJINTERP_TEST_ZIP_STORE_ 1

#ifndef __cplusplus
    #error "test_zip_store.hpp requires C++ compilation"
#endif

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/util/archive.hpp"   // entry, entry_list, byte_blob


NS_DJINTERP
NS_TEST

///////////////////////////////////////////////////////////////////////////////
///                I.   INTERNAL: BYTE / CRC HELPERS                         ///
///////////////////////////////////////////////////////////////////////////////

namespace zip_store_internal
{
    // put_u16_le / put_u32_le
    //   append a little-endian 16- / 32-bit value to a byte string.
    D_INLINE void
    put_u16_le(
        std::string&   _s,
        std::uint16_t  _v
    )
    {
        _s.push_back(static_cast<char>( _v        & 0xFFu));
        _s.push_back(static_cast<char>((_v >> 8)  & 0xFFu));

        return;
    }

    D_INLINE void
    put_u32_le(
        std::string&   _s,
        std::uint32_t  _v
    )
    {
        _s.push_back(static_cast<char>( _v        & 0xFFu));
        _s.push_back(static_cast<char>((_v >> 8)  & 0xFFu));
        _s.push_back(static_cast<char>((_v >> 16) & 0xFFu));
        _s.push_back(static_cast<char>((_v >> 24) & 0xFFu));

        return;
    }

    // crc32
    //   the IEEE / zlib CRC-32 (reflected, polynomial 0xEDB88320) of a byte
    // range, matching what ZIP readers verify.  The lookup table is built once
    // on first use.
    D_INLINE std::uint32_t
    crc32(
        const char*  _data,
        std::size_t  _size
    )
    {
        static std::uint32_t table[256];
        static bool          ready = false;

        if (!ready)
        {
            std::uint32_t i = 0;

            for (i = 0; i < 256u; ++i)
            {
                std::uint32_t c = i;
                int           k = 0;

                for (k = 0; k < 8; ++k)
                {
                    c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
                }

                table[i] = c;
            }

            ready = true;
        }

        std::uint32_t c = 0xFFFFFFFFu;
        std::size_t   i = 0;

        for (i = 0; i < _size; ++i)
        {
            const unsigned char b = static_cast<unsigned char>(_data[i]);

            c = table[(c ^ b) & 0xFFu] ^ (c >> 8);
        }

        return c ^ 0xFFFFFFFFu;
    }
}   // namespace zip_store_internal


///////////////////////////////////////////////////////////////////////////////
///                II.  STORED-ZIP WRITER                                    ///
///////////////////////////////////////////////////////////////////////////////

// zip_store_archive
//   function: bundle _entries into a single STORED (uncompressed) ZIP, writing
// the container bytes into _out.  Each entry's `name` becomes the archive path
// and its `data` the file contents.  Needs no third-party library.
//
// Parameter(s):
//   _entries: the members to store (name + data each).
//   _out:     receives the complete .zip byte stream (overwritten).
// Return:
//   true always (the stored path cannot fail short of allocation); the bool
// return keeps the call site uniform with the backend writers.
D_INLINE bool
zip_store_archive(
    const entry_list& _entries,
    byte_blob&      _out
)
{
    using namespace zip_store_internal;

    // A fixed, valid DOS timestamp keeps output deterministic and avoids a
    // clock dependency: 2026-01-01 00:00:00.
    const std::uint16_t dos_time = 0x0000u;
    const std::uint16_t dos_date = 0x5C21u;   // ((2026-1980)<<9)|(1<<5)|1

    std::string central;                       // central directory, built alongside
    std::size_t i = 0;

    _out.clear();

    // 1) local file header + data for each entry; mirror a central record.
    for (i = 0; i < _entries.size(); ++i)
    {
        const std::string&  name = _entries[i].name;
        const byte_blob&  data = _entries[i].data;

        const std::uint32_t crc    = crc32(data.data(), data.size());
        const std::uint32_t size   = static_cast<std::uint32_t>(data.size());
        const std::uint16_t nlen   = static_cast<std::uint16_t>(name.size());
        const std::uint32_t offset = static_cast<std::uint32_t>(_out.size());

        // -- local file header --
        put_u32_le(_out, 0x04034B50u);   // signature "PK\003\004"
        put_u16_le(_out, 20u);           // version needed to extract (2.0)
        put_u16_le(_out, 0u);            // general purpose flags
        put_u16_le(_out, 0u);            // method: 0 = stored
        put_u16_le(_out, dos_time);
        put_u16_le(_out, dos_date);
        put_u32_le(_out, crc);
        put_u32_le(_out, size);          // compressed size   (== stored size)
        put_u32_le(_out, size);          // uncompressed size
        put_u16_le(_out, nlen);          // file name length
        put_u16_le(_out, 0u);            // extra field length
        _out += name;
        _out += data;

        // -- central directory record (accumulated, appended in step 2) --
        put_u32_le(central, 0x02014B50u);   // signature "PK\001\002"
        put_u16_le(central, 20u);           // version made by
        put_u16_le(central, 20u);           // version needed to extract
        put_u16_le(central, 0u);            // flags
        put_u16_le(central, 0u);            // method: stored
        put_u16_le(central, dos_time);
        put_u16_le(central, dos_date);
        put_u32_le(central, crc);
        put_u32_le(central, size);
        put_u32_le(central, size);
        put_u16_le(central, nlen);
        put_u16_le(central, 0u);            // extra field length
        put_u16_le(central, 0u);            // file comment length
        put_u16_le(central, 0u);            // disk number start
        put_u16_le(central, 0u);            // internal attributes
        put_u32_le(central, 0u);            // external attributes
        put_u32_le(central, offset);        // offset of local header
        central += name;
    }

    // 2) central directory, then the end-of-central-directory record.
    const std::uint32_t cd_offset = static_cast<std::uint32_t>(_out.size());
    _out += central;
    const std::uint32_t cd_size =
        static_cast<std::uint32_t>(_out.size()) - cd_offset;

    const std::uint16_t count = static_cast<std::uint16_t>(_entries.size());

    put_u32_le(_out, 0x06054B50u);   // signature "PK\005\006"
    put_u16_le(_out, 0u);            // this disk number
    put_u16_le(_out, 0u);            // disk with central directory
    put_u16_le(_out, count);         // central records on this disk
    put_u16_le(_out, count);         // total central records
    put_u32_le(_out, cd_size);       // size of central directory
    put_u32_le(_out, cd_offset);     // offset of central directory
    put_u16_le(_out, 0u);            // archive comment length

    return true;
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_ZIP_STORE_
