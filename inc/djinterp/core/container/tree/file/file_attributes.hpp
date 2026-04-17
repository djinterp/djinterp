/******************************************************************************
* djinterp [core]                                       file_attributes.hpp
*
* Portable file attribute and metadata types:
*   This header provides platform-independent representations of file
* attributes, timestamps, text encoding, and extended metadata.  On
* Windows it maps to NTFS file attributes, WIN32_FIND_DATA, and the
* Shell property system.  On POSIX it maps to struct stat, extended
* attributes (xattr), and MIME detection.
*
*   The design is enum-centric — attribute flags are bitfields,
* metadata categories are strongly-typed enumerations, and text
* encodings are a closed enum.  All types are trivially copyable
* and fixed-size, suitable for storage in arena_node payloads.
*
* Contents:
*   - file_encoding          text encoding identification
*   - file_attr_flag         NTFS/POSIX attribute bitfield
*   - file_detail            extended metadata category enum
*   - file_detail_group      grouping of detail categories
*   - file_timestamps        creation, modification, access times
*   - file_metadata          composite descriptor
*   - file_attributes        population from the OS
*
* Platform support:
*   - Win32    GetFileAttributesW, FindFirstFileW, IPropertyStore
*   - POSIX    stat/lstat, getxattr (Linux), listxattr (macOS)
*
*
* path:      /inc/cpp/fs/file_attributes.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_FS_FILE_ATTRIBUTES_
#define DJINTERP_FS_FILE_ATTRIBUTES_ 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "../djinterp.hpp"


// ================================================================
//  platform headers
// ================================================================

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
    #include <sys/stat.h>
    #include <unistd.h>
    #if defined(__linux__)
        #include <sys/xattr.h>
    #elif defined(__APPLE__)
        #include <sys/xattr.h>
    #endif
#else
    #include <sys/stat.h>
    #include <unistd.h>
#endif


NS_DJINTERP
NS_FS


// ================================================================
//  file_encoding
// ================================================================

// file_encoding
//   enum: identifies the text encoding of a file's content.
// Detection is heuristic — these values represent the best
// guess from a BOM or byte-pattern scan.
enum file_encoding : std::uint8_t
{
    file_encoding_unknown       = 0,

    // unicode family
    file_encoding_utf8          = 1,
    file_encoding_utf8_bom      = 2,
    file_encoding_utf16_le      = 3,
    file_encoding_utf16_be      = 4,
    file_encoding_utf32_le      = 5,
    file_encoding_utf32_be      = 6,

    // single-byte
    file_encoding_ascii         = 10,
    file_encoding_latin1        = 11,   // ISO 8859-1
    file_encoding_windows_1252  = 12,

    // east asian
    file_encoding_shift_jis     = 20,
    file_encoding_euc_jp        = 21,
    file_encoding_gb2312        = 22,
    file_encoding_big5          = 23,
    file_encoding_euc_kr        = 24,

    // binary / non-text
    file_encoding_binary        = 255
};


// ================================================================
//  file_encoding utilities
// ================================================================

// file_encoding_name
//   returns a human-readable name for the given encoding.
D_STATIC_INLINE
const char*
file_encoding_name
(
    file_encoding _enc
)
{
    switch (_enc)
    {
        case file_encoding_utf8:         return "UTF-8";
        case file_encoding_utf8_bom:     return "UTF-8 (BOM)";
        case file_encoding_utf16_le:     return "UTF-16 LE";
        case file_encoding_utf16_be:     return "UTF-16 BE";
        case file_encoding_utf32_le:     return "UTF-32 LE";
        case file_encoding_utf32_be:     return "UTF-32 BE";
        case file_encoding_ascii:        return "ASCII";
        case file_encoding_latin1:       return "ISO 8859-1";
        case file_encoding_windows_1252: return "Windows-1252";
        case file_encoding_shift_jis:    return "Shift-JIS";
        case file_encoding_euc_jp:       return "EUC-JP";
        case file_encoding_gb2312:       return "GB2312";
        case file_encoding_big5:         return "Big5";
        case file_encoding_euc_kr:       return "EUC-KR";
        case file_encoding_binary:       return "Binary";
        default:                         return "Unknown";
    }
}

// file_encoding_is_unicode
//   returns true if the encoding is a Unicode variant.
D_STATIC_INLINE
bool
file_encoding_is_unicode
(
    file_encoding _enc
)
{
    return (_enc >= file_encoding_utf8 &&
            _enc <= file_encoding_utf32_be);
}

// detect_bom
//   examines the first bytes of a buffer and returns the
// encoding indicated by a byte-order mark, or
// file_encoding_unknown if no BOM is found.
D_STATIC_INLINE
file_encoding
detect_bom
(
    const unsigned char* _data,
    std::size_t          _len
)
{
    if (_len >= 4)
    {
        // UTF-32 LE: FF FE 00 00
        if (_data[0] == 0xFF && _data[1] == 0xFE &&
            _data[2] == 0x00 && _data[3] == 0x00)
        {
            return file_encoding_utf32_le;
        }

        // UTF-32 BE: 00 00 FE FF
        if (_data[0] == 0x00 && _data[1] == 0x00 &&
            _data[2] == 0xFE && _data[3] == 0xFF)
        {
            return file_encoding_utf32_be;
        }
    }

    if (_len >= 3)
    {
        // UTF-8 BOM: EF BB BF
        if (_data[0] == 0xEF && _data[1] == 0xBB &&
            _data[2] == 0xBF)
        {
            return file_encoding_utf8_bom;
        }
    }

    if (_len >= 2)
    {
        // UTF-16 LE: FF FE
        if (_data[0] == 0xFF && _data[1] == 0xFE)
        {
            return file_encoding_utf16_le;
        }

        // UTF-16 BE: FE FF
        if (_data[0] == 0xFE && _data[1] == 0xFF)
        {
            return file_encoding_utf16_be;
        }
    }

    return file_encoding_unknown;
}


// ================================================================
//  file_attr_flag
// ================================================================

// file_attr_flag
//   enum: bitfield of file attributes.  The values are chosen
// to map directly to NTFS FILE_ATTRIBUTE_* constants where
// applicable, with POSIX-only flags occupying the upper bits.
//
// On Windows these come from GetFileAttributesW or
// WIN32_FIND_DATA::dwFileAttributes.
//
// On POSIX they are synthesized from struct stat::st_mode.
enum file_attr_flag : std::uint32_t
{
    // --------------------------------------------------------
    //  NTFS-mapped flags (low 16 bits)
    // --------------------------------------------------------

    // file_attr_readonly
    //   flag: file is read-only.
    // NTFS: FILE_ATTRIBUTE_READONLY (0x1)
    // POSIX: !(st_mode & S_IWUSR)
    file_attr_readonly          = 0x00000001,

    // file_attr_hidden
    //   flag: file is hidden.
    // NTFS: FILE_ATTRIBUTE_HIDDEN (0x2)
    // POSIX: name starts with '.'
    file_attr_hidden            = 0x00000002,

    // file_attr_system
    //   flag: file is a system file.
    // NTFS: FILE_ATTRIBUTE_SYSTEM (0x4)
    // POSIX: not applicable (never set)
    file_attr_system            = 0x00000004,

    // file_attr_directory
    //   flag: entry is a directory.
    // NTFS: FILE_ATTRIBUTE_DIRECTORY (0x10)
    // POSIX: S_ISDIR(st_mode)
    file_attr_directory         = 0x00000010,

    // file_attr_archive
    //   flag: file has been modified since last up.
    // NTFS: FILE_ATTRIBUTE_ARCHIVE (0x20)
    // POSIX: not applicable (never set)
    file_attr_archive           = 0x00000020,

    // file_attr_device
    //   flag: reserved for system use.
    // NTFS: FILE_ATTRIBUTE_DEVICE (0x40)
    // POSIX: S_ISBLK || S_ISCHR
    file_attr_device            = 0x00000040,

    // file_attr_normal
    //   flag: file has no other attributes set.
    // NTFS: FILE_ATTRIBUTE_NORMAL (0x80)
    file_attr_normal            = 0x00000080,

    // file_attr_temporary
    //   flag: file is being used for temporary storage.
    // NTFS: FILE_ATTRIBUTE_TEMPORARY (0x100)
    file_attr_temporary         = 0x00000100,

    // file_attr_sparse
    //   flag: file is sparse.
    // NTFS: FILE_ATTRIBUTE_SPARSE_FILE (0x200)
    file_attr_sparse            = 0x00000200,

    // file_attr_reparse_point
    //   flag: file has a reparse point (symlink, junction).
    // NTFS: FILE_ATTRIBUTE_REPARSE_POINT (0x400)
    // POSIX: S_ISLNK(st_mode)
    file_attr_reparse_point     = 0x00000400,

    // file_attr_compressed
    //   flag: file or directory is compressed.
    // NTFS: FILE_ATTRIBUTE_COMPRESSED (0x800)
    file_attr_compressed        = 0x00000800,

    // file_attr_offline
    //   flag: file data is not immediately available.
    // NTFS: FILE_ATTRIBUTE_OFFLINE (0x1000)
    file_attr_offline           = 0x00001000,

    // file_attr_not_indexed
    //   flag: file will not be indexed by content indexer.
    // NTFS: FILE_ATTRIBUTE_NOT_CONTENT_INDEXED (0x2000)
    file_attr_not_indexed       = 0x00002000,

    // file_attr_encrypted
    //   flag: file is encrypted (EFS).
    // NTFS: FILE_ATTRIBUTE_ENCRYPTED (0x4000)
    file_attr_encrypted         = 0x00004000,

    // --------------------------------------------------------
    //  POSIX-only flags (bits 16–23)
    // --------------------------------------------------------

    // file_attr_setuid
    //   flag: set-user-ID bit is set.
    // POSIX: st_mode & S_ISUID
    file_attr_setuid            = 0x00010000,

    // file_attr_setgid
    //   flag: set-group-ID bit is set.
    // POSIX: st_mode & S_ISGID
    file_attr_setgid            = 0x00020000,

    // file_attr_sticky
    //   flag: sticky bit is set.
    // POSIX: st_mode & S_ISVTX
    file_attr_sticky            = 0x00040000,

    // file_attr_symlink
    //   flag: entry is a symbolic link.
    // POSIX: S_ISLNK(st_mode)
    // Win32: reparse point (also sets file_attr_reparse_point)
    file_attr_symlink           = 0x00080000,

    // file_attr_pipe
    //   flag: entry is a named pipe (FIFO).
    // POSIX: S_ISFIFO(st_mode)
    file_attr_pipe              = 0x00100000,

    // file_attr_socket
    //   flag: entry is a Unix domain socket.
    // POSIX: S_ISSOCK(st_mode)
    file_attr_socket            = 0x00200000,

    // --------------------------------------------------------
    //  synthetic / derived (bits 24–31)
    // --------------------------------------------------------

    // file_attr_executable
    //   flag: file is executable.
    // POSIX: st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)
    // Win32: heuristic from extension (.exe, .bat, .cmd, etc.)
    file_attr_executable        = 0x01000000,

    // file_attr_has_xattr
    //   flag: file has extended attributes.
    // POSIX: listxattr returns > 0
    // Win32: file has alternate data streams
    file_attr_has_xattr         = 0x02000000,

    // file_attr_none
    //   flag: no flags set (sentinel).
    file_attr_none              = 0x00000000
};

// D_FILE_ATTR_HAS
//   macro: tests whether a flag set contains a specific flag.
#define D_FILE_ATTR_HAS(_flags, _flag)  \
    ( ((_flags) & (_flag)) != 0 )


// ================================================================
//  file_detail
// ================================================================

// file_detail
//   enum: identifies a category of extended file metadata from
// the Windows Shell property system.  On POSIX, a subset of
// these can be populated from xattr, EXIF, ID3, or similar
// sources — most will simply report as unavailable.
//
// Values are grouped by domain.  The grouping enum
// file_detail_group provides the domain classification.
//
// The enumerators below correspond to the columns available
// in the Windows Explorer "Choose Details" dialog.
enum file_detail : std::uint16_t
{
    // --------------------------------------------------------
    //  core / filesystem (0x00–)
    // --------------------------------------------------------
    file_detail_name                    = 0x0001,
    file_detail_size                    = 0x0002,
    file_detail_type                    = 0x0003,
    file_detail_date_modified           = 0x0004,
    file_detail_date_created            = 0x0005,
    file_detail_date_accessed           = 0x0006,
    file_detail_attributes              = 0x0007,
    file_detail_owner                   = 0x0008,
    file_detail_computer                = 0x0009,
    file_detail_number                  = 0x000A,   // "#" column
    file_detail_folder_path             = 0x000B,
    file_detail_folder_name             = 0x000C,
    file_detail_file_extension          = 0x000D,
    file_detail_filename                = 0x000E,
    file_detail_content_status          = 0x000F,
    file_detail_perceived_type          = 0x0010,
    file_detail_kind                    = 0x0011,

    // --------------------------------------------------------
    //  document (0x01–)
    // --------------------------------------------------------
    file_detail_title                   = 0x0100,
    file_detail_subject                 = 0x0101,
    file_detail_categories              = 0x0102,
    file_detail_tags                    = 0x0103,   // keywords
    file_detail_comments                = 0x0104,
    file_detail_authors                 = 0x0105,
    file_detail_last_saved_by           = 0x0106,
    file_detail_revision_number         = 0x0107,
    file_detail_content_created         = 0x0108,
    file_detail_date_last_saved         = 0x0109,
    file_detail_program_name            = 0x010A,
    file_detail_pages                   = 0x010B,
    file_detail_word_count              = 0x010C,
    file_detail_line_count              = 0x010D,
    file_detail_total_editing_time      = 0x010E,
    file_detail_template                = 0x010F,

    // --------------------------------------------------------
    //  image / photo (0x02–)
    // --------------------------------------------------------
    file_detail_dimensions              = 0x0200,
    file_detail_width                   = 0x0201,
    file_detail_height                  = 0x0202,
    file_detail_horizontal_resolution   = 0x0203,
    file_detail_vertical_resolution     = 0x0204,
    file_detail_bit_depth               = 0x0205,
    file_detail_camera_maker            = 0x0206,
    file_detail_camera_model            = 0x0207,
    file_detail_date_taken              = 0x0208,
    file_detail_exposure_time           = 0x0209,
    file_detail_f_stop                  = 0x020A,
    file_detail_flash_mode              = 0x020B,
    file_detail_focal_length            = 0x020C,
    file_detail_focal_length_35mm       = 0x020D,   // "35mm focal length"
    file_detail_iso_speed               = 0x020E,
    file_detail_metering_mode           = 0x020F,
    file_detail_orientation             = 0x0210,
    file_detail_white_balance           = 0x0211,
    file_detail_exposure_bias           = 0x0212,
    file_detail_exposure_program        = 0x0213,
    file_detail_lens_maker              = 0x0214,
    file_detail_lens_model              = 0x0215,
    file_detail_subject_distance        = 0x0216,
    file_detail_light_source            = 0x0217,
    file_detail_max_aperture            = 0x0218,

    // --------------------------------------------------------
    //  audio / music (0x03–)
    // --------------------------------------------------------
    file_detail_album                   = 0x0300,
    file_detail_album_artist            = 0x0301,
    file_detail_album_id                = 0x0302,
    file_detail_contributing_artists    = 0x0303,
    file_detail_genre                   = 0x0304,
    file_detail_track_number            = 0x0305,
    file_detail_disc_number             = 0x0306,
    file_detail_year                    = 0x0307,
    file_detail_duration                = 0x0308,
    file_detail_bit_rate                = 0x0309,
    file_detail_sample_rate             = 0x030A,
    file_detail_sample_size             = 0x030B,
    file_detail_channel_count           = 0x030C,
    file_detail_beats_per_minute        = 0x030D,
    file_detail_composers               = 0x030E,
    file_detail_conductors              = 0x030F,
    file_detail_initial_key             = 0x0310,
    file_detail_lyrics                  = 0x0311,
    file_detail_mood                    = 0x0312,
    file_detail_publisher               = 0x0313,
    file_detail_subtitle                = 0x0314,
    file_detail_audio_compression       = 0x0315,
    file_detail_is_protected            = 0x0316,
    file_detail_encoded_by              = 0x0317,

    // --------------------------------------------------------
    //  video (0x04–)
    // --------------------------------------------------------
    file_detail_frame_width             = 0x0400,
    file_detail_frame_height            = 0x0401,
    file_detail_frame_rate              = 0x0402,
    file_detail_total_bitrate           = 0x0403,
    file_detail_video_compression       = 0x0404,
    file_detail_data_rate               = 0x0405,
    file_detail_director                = 0x0406,
    file_detail_producer                = 0x0407,
    file_detail_writer                  = 0x0408,

    // --------------------------------------------------------
    //  contact / communication (0x05–)
    // --------------------------------------------------------
    file_detail_account_name            = 0x0500,
    file_detail_anniversary             = 0x0501,
    file_detail_assistant_name          = 0x0502,
    file_detail_assistant_phone         = 0x0503,
    file_detail_birthday                = 0x0504,
    file_detail_business_address        = 0x0505,
    file_detail_business_phone          = 0x0506,
    file_detail_call_number         = 0x0507,
    file_detail_car_phone               = 0x0508,
    file_detail_city                    = 0x0509,
    file_detail_company                 = 0x050A,
    file_detail_country                 = 0x050B,
    file_detail_department              = 0x050C,
    file_detail_email_address           = 0x050D,
    file_detail_first_name              = 0x050E,
    file_detail_full_name               = 0x050F,
    file_detail_gender                  = 0x0510,
    file_detail_home_address            = 0x0511,
    file_detail_home_phone              = 0x0512,
    file_detail_im_address              = 0x0513,
    file_detail_job_title               = 0x0514,
    file_detail_last_name               = 0x0515,
    file_detail_mailing_address         = 0x0516,
    file_detail_middle_name             = 0x0517,
    file_detail_mobile_phone            = 0x0518,
    file_detail_nickname                = 0x0519,
    file_detail_office_location         = 0x051A,
    file_detail_pager                   = 0x051B,
    file_detail_po_box                  = 0x051C,
    file_detail_postal_code             = 0x051D,
    file_detail_state_province          = 0x051E,
    file_detail_suffix                  = 0x051F,
    file_detail_web_page                = 0x0520,

    // --------------------------------------------------------
    //  email / message (0x06–)
    // --------------------------------------------------------
    file_detail_attachments             = 0x0600,
    file_detail_bcc_address             = 0x0601,
    file_detail_cc_address              = 0x0602,
    file_detail_conversation_id         = 0x0603,
    file_detail_date_received           = 0x0604,
    file_detail_date_sent               = 0x0605,
    file_detail_flag_status             = 0x0606,
    file_detail_from_address            = 0x0607,
    file_detail_has_attachments         = 0x0608,
    file_detail_importance              = 0x0609,
    file_detail_is_read                 = 0x060A,
    file_detail_to_address              = 0x060B,

    // --------------------------------------------------------
    //  GPS / location (0x07–)
    // --------------------------------------------------------
    file_detail_gps_latitude            = 0x0700,
    file_detail_gps_longitude           = 0x0701,
    file_detail_gps_altitude            = 0x0702,

    // --------------------------------------------------------
    //  version / executable (0x08–)
    // --------------------------------------------------------
    file_detail_file_version            = 0x0800,
    file_detail_product_name            = 0x0801,
    file_detail_product_version         = 0x0802,
    file_detail_file_description        = 0x0803,
    file_detail_copyright               = 0x0804,
    file_detail_company_name            = 0x0805,
    file_detail_language                = 0x0806,
    file_detail_link_target             = 0x0807,

    // --------------------------------------------------------
    //  sentinel
    // --------------------------------------------------------
    file_detail_none                    = 0x0000
};


// ================================================================
//  file_detail_group
// ================================================================

// file_detail_group
//   enum: the domain group that a file_detail belongs to.
// Derived by masking the high byte of the file_detail value.
enum file_detail_group : std::uint8_t
{
    file_detail_group_core          = 0x00,
    file_detail_group_document      = 0x01,
    file_detail_group_image         = 0x02,
    file_detail_group_audio         = 0x03,
    file_detail_group_video         = 0x04,
    file_detail_group_contact       = 0x05,
    file_detail_group_email         = 0x06,
    file_detail_group_gps           = 0x07,
    file_detail_group_version       = 0x08,
    file_detail_group_unknown       = 0xFF
};

// file_detail_to_group
//   returns the group of a file_detail value.
D_STATIC_INLINE
file_detail_group
file_detail_to_group
(
    file_detail _detail
)
{
    std::uint8_t high = static_cast<std::uint8_t>(
        (static_cast<std::uint16_t>(_detail) >> 8) & 0xFF
    );

    if (high <= 0x08)
    {
        return static_cast<file_detail_group>(high);
    }

    return file_detail_group_unknown;
}

// file_detail_group_name
//   returns a human-readable name for the given group.
D_STATIC_INLINE
const char*
file_detail_group_name
(
    file_detail_group _group
)
{
    switch (_group)
    {
        case file_detail_group_core:     return "Core";
        case file_detail_group_document: return "Document";
        case file_detail_group_image:    return "Image";
        case file_detail_group_audio:    return "Audio";
        case file_detail_group_video:    return "Video";
        case file_detail_group_contact:  return "Contact";
        case file_detail_group_email:    return "Email";
        case file_detail_group_gps:      return "GPS";
        case file_detail_group_version:  return "Version";
        default:                         return "Unknown";
    }
}


// ================================================================
//  file_timestamps
// ================================================================

// file_timestamps
//   struct: platform-independent timestamp representation.
// All values are in nanoseconds since the Unix epoch
// (1970-01-01T00:00:00Z).  A value of 0 indicates
// "not available".
struct file_timestamps
{
    std::uint64_t   created;        // birth time (btime)
    std::uint64_t   modified;       // last data modification
    std::uint64_t   accessed;       // last access
    std::uint64_t   changed;        // last metadata change (POSIX ctime)

    // file_timestamps (default)
    file_timestamps
    ()
        : created   (0),
          modified  (0),
          accessed  (0),
          changed   (0)
    {}
};


// ================================================================
//  file_metadata
// ================================================================

// file_metadata
//   struct: composite descriptor holding all portable
// file attributes.  Fixed-size and trivially copyable —
// suitable for storage in arena_node payloads alongside
// or replacing file_entry.
struct file_metadata
{
    // --------------------------------------------------------
    //  core
    // --------------------------------------------------------
    std::uint64_t       size;
    file_timestamps     timestamps;
    std::uint32_t       attr_flags;     // OR'd file_attr_flag
    file_encoding       encoding;

    // --------------------------------------------------------
    //  permissions (POSIX)
    // --------------------------------------------------------
    std::uint16_t       mode;           // raw st_mode (0 on Win)
    std::uint32_t       uid;            // owner user id
    std::uint32_t       gid;            // owner group id

    // --------------------------------------------------------
    //  identity
    // --------------------------------------------------------
    std::uint64_t       inode;          // st_ino (POSIX) / file index (Win)
    std::uint32_t       link_count;     // st_nlink / number of hard links
    std::uint64_t       device;         // st_dev (POSIX) / volume serial (Win)

    // --------------------------------------------------------
    //  padding / reserved
    // --------------------------------------------------------
    std::uint8_t        _pad[3];

    // file_metadata (default)
    file_metadata
    ()
        : size          (0),
          timestamps    (),
          attr_flags    (file_attr_none),
          encoding      (file_encoding_unknown),
          mode          (0),
          uid           (0),
          gid           (0),
          inode         (0),
          link_count    (0),
          device        (0),
          _pad          {}
    {}

    // --------------------------------------------------------
    //  flag queries
    // --------------------------------------------------------

    // has_flag
    //   returns true if the specified attribute flag is set.
    bool
    has_flag
    (
        file_attr_flag _flag
    ) const
    {
        return D_FILE_ATTR_HAS(attr_flags, _flag);
    }

    // is_readonly
    bool is_readonly()  const { return has_flag(file_attr_readonly);  }

    // is_hidden
    bool is_hidden()    const { return has_flag(file_attr_hidden);    }

    // is_system
    bool is_system()    const { return has_flag(file_attr_system);    }

    // is_directory
    bool is_directory()  const { return has_flag(file_attr_directory);  }

    // is_symlink
    bool is_symlink()   const { return has_flag(file_attr_symlink);   }

    // is_compressed
    bool is_compressed() const { return has_flag(file_attr_compressed); }

    // is_encrypted
    bool is_encrypted() const { return has_flag(file_attr_encrypted); }

    // is_executable
    bool is_executable() const { return has_flag(file_attr_executable); }
};


// ================================================================
//  file_attributes (population)
// ================================================================

// file_attributes
//   class: static methods for populating file_metadata from
// OS-level queries.
class file_attributes
{
public:

    // --------------------------------------------------------
    //  populate
    // --------------------------------------------------------

    // populate
    //   fills _out with metadata from the file at _path.
    // Returns true on success, false on failure.
    static bool
    populate
    (
        const char*    _path,
        file_metadata& _out
    )
    {
        _out = file_metadata();

        return populate_impl(_path, _out);
    }

    // populate (std::string overload)
    static bool
    populate
    (
        const std::string& _path,
        file_metadata&     _out
    )
    {
        return populate(_path.c_str(), _out);
    }

    // --------------------------------------------------------
    //  encoding detection
    // --------------------------------------------------------

    // detect_encoding
    //   performs BOM detection on the first _len bytes of
    // _data.  Returns the detected encoding, or
    // file_encoding_unknown if no BOM is found.
    // A more thorough heuristic (byte-frequency, UTF-8
    // validation) can be layered on top.
    static file_encoding
    detect_encoding
    (
        const unsigned char* _data,
        std::size_t          _len
    )
    {
        // try BOM first.
        file_encoding bom = detect_bom(_data, _len);

        if (bom != file_encoding_unknown)
        {
            return bom;
        }

        // heuristic: check for null bytes (binary indicator).
        bool has_null  = false;
        bool has_high  = false;
        bool valid_utf8 = true;

        std::size_t scan_len = (_len > 8192) ? 8192 : _len;
        std::size_t i = 0;

        while (i < scan_len)
        {
            unsigned char c = _data[i];

            if (c == 0x00)
            {
                has_null = true;
                break;
            }

            if (c > 0x7F)
            {
                has_high = true;

                // validate UTF-8 sequence.
                std::size_t seq_len = 0;

                if ((c & 0xE0) == 0xC0)      { seq_len = 2; }
                else if ((c & 0xF0) == 0xE0)  { seq_len = 3; }
                else if ((c & 0xF8) == 0xF0)  { seq_len = 4; }
                else { valid_utf8 = false; ++i; continue; }

                if (i + seq_len > scan_len)
                {
                    break;
                }

                for (std::size_t j = 1; j < seq_len; ++j)
                {
                    if ((_data[i + j] & 0xC0) != 0x80)
                    {
                        valid_utf8 = false;
                        break;
                    }
                }

                i += seq_len;
                continue;
            }

            ++i;
        }

        if (has_null)
        {
            return file_encoding_binary;
        }

        if (!has_high)
        {
            return file_encoding_ascii;
        }

        if (valid_utf8)
        {
            return file_encoding_utf8;
        }

        // could be Latin-1, Windows-1252, or another
        // single-byte encoding — caller may refine.
        return file_encoding_unknown;
    }


private:

    // --------------------------------------------------------
    //  platform implementation
    // --------------------------------------------------------

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)

    // ============================================
    //  Win32 constants
    // ============================================

    // WINDOWS_TICK
    //   constant: number of 100-ns intervals per second.
    static D_CONSTEXPR std::uint64_t WINDOWS_TICK
        = UINT64_C(10000000);

    // EPOCH_DIFFERENCE
    //   constant: 100-ns intervals between the Windows epoch
    // (1601-01-01) and the Unix epoch (1970-01-01).
    static D_CONSTEXPR std::uint64_t EPOCH_DIFFERENCE
        = UINT64_C(116444736000000000);

    // filetime_to_ns
    //   converts a FILETIME to nanoseconds since Unix epoch.
    D_STATIC_INLINE
    std::uint64_t
    filetime_to_ns
    (
        const FILETIME& _ft
    )
    {
        std::uint64_t ticks =
            (static_cast<std::uint64_t>(_ft.dwHighDateTime) << 32) |
            static_cast<std::uint64_t>(_ft.dwLowDateTime);

        if (ticks < EPOCH_DIFFERENCE)
        {
            return 0;
        }

        // convert 100-ns ticks to nanoseconds.
        return (ticks - EPOCH_DIFFERENCE) * 100;
    }

    // widen
    //   converts a UTF-8 string to a wide string for Win32 APIs.
    D_STATIC_INLINE
    std::wstring
    widen
    (
        const char* _utf8
    )
    {
        if (_utf8 == nullptr || _utf8[0] == '\0')
        {
            return std::wstring();
        }

        int len = static_cast<int>(std::strlen(_utf8));

        int needed = MultiByteToWideChar(
            CP_UTF8, 0, _utf8, len, nullptr, 0
        );

        std::wstring out(
            static_cast<std::size_t>(needed), L'\0'
        );

        MultiByteToWideChar(
            CP_UTF8, 0, _utf8, len, &out[0], needed
        );

        return out;
    }

    // is_executable_extension
    //   returns true if _path ends with a known executable
    // extension.
    D_STATIC_INLINE
    bool
    is_executable_extension
    (
        const char* _path
    )
    {
        const char* dot = nullptr;
        const char* p   = _path;

        while (*p != '\0')
        {
            if (*p == '.')
            {
                dot = p;
            }

            ++p;
        }

        if (dot == nullptr)
        {
            return false;
        }

        ++dot;  // skip the dot

        // case-insensitive compare for common extensions.
        auto eq3 = [](const char* a, const char* b) -> bool
        {
            return ((a[0] | 0x20) == b[0] &&
                    (a[1] | 0x20) == b[1] &&
                    (a[2] | 0x20) == b[2] &&
                    a[3] == '\0');
        };

        if (eq3(dot, "exe")) return true;
        if (eq3(dot, "com")) return true;
        if (eq3(dot, "bat")) return true;
        if (eq3(dot, "cmd")) return true;
        if (eq3(dot, "msi")) return true;
        if (eq3(dot, "scr")) return true;
        if (eq3(dot, "ps1")) return true;

        return false;
    }

    // populate_impl (Win32)
    D_STATIC_INLINE
    bool
    populate_impl
    (
        const char*    _path,
        file_metadata& _out
    )
    {
        std::wstring wide = widen(_path);

        WIN32_FILE_ATTRIBUTE_DATA fad;

        if (!GetFileAttributesExW(
                wide.c_str(),
                GetFileExInfoStandard,
                &fad))
        {
            return false;
        }

        // size
        _out.size =
            (static_cast<std::uint64_t>(fad.nFileSizeHigh) << 32) |
            static_cast<std::uint64_t>(fad.nFileSizeLow);

        // timestamps
        _out.timestamps.created  =
            filetime_to_ns(fad.ftCreationTime);
        _out.timestamps.modified =
            filetime_to_ns(fad.ftLastWriteTime);
        _out.timestamps.accessed =
            filetime_to_ns(fad.ftLastAccessTime);
        _out.timestamps.changed  =
            _out.timestamps.modified; // Win32 has no ctime

        // NTFS attribute flags (direct mapping).
        _out.attr_flags = fad.dwFileAttributes;

        // synthesize portable flags.
        if (fad.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            _out.attr_flags |= file_attr_symlink;
        }

        if (is_executable_extension(_path))
        {
            _out.attr_flags |= file_attr_executable;
        }

        // identity — requires a handle for file index.
        HANDLE hFile = CreateFileW(
            wide.c_str(),
            0,                          // no access needed
            FILE_SHARE_READ |
            FILE_SHARE_WRITE |
            FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_UP_SEMANTICS, // required for dirs
            nullptr
        );

        if (hFile != INVALID_HANDLE_VALUE)
        {
            BY_HANDLE_FILE_INFORMATION bhfi;

            if (GetFileInformationByHandle(hFile, &bhfi))
            {
                _out.inode =
                    (static_cast<std::uint64_t>(
                        bhfi.nFileIndexHigh) << 32) |
                    static_cast<std::uint64_t>(
                        bhfi.nFileIndexLow);

                _out.link_count = bhfi.nNumberOfLinks;
                _out.device     = bhfi.dwVolumeSerialNumber;
            }

            CloseHandle(hFile);
        }

        return true;
    }

#else

    // ============================================
    //  POSIX implementation
    // ============================================

    // timespec_to_ns
    //   converts a struct timespec to nanoseconds since epoch.
    D_STATIC_INLINE
    std::uint64_t
    timespec_to_ns
    (
        const struct timespec& _ts
    )
    {
        return static_cast<std::uint64_t>(_ts.tv_sec)
               * UINT64_C(1000000000)
             + static_cast<std::uint64_t>(_ts.tv_nsec);
    }

    // populate_impl (POSIX)
    D_STATIC_INLINE
    bool
    populate_impl
    (
        const char*    _path,
        file_metadata& _out
    )
    {
        struct stat st;

        if (lstat(_path, &st) != 0)
        {
            return false;
        }

        // size
        _out.size = static_cast<std::uint64_t>(st.st_size);

        // timestamps
        //   platforms vary in which timespec fields they expose.
        #if defined(__APPLE__)
            _out.timestamps.modified = timespec_to_ns(st.st_mtimespec);
            _out.timestamps.accessed = timespec_to_ns(st.st_atimespec);
            _out.timestamps.changed  = timespec_to_ns(st.st_ctimespec);
            _out.timestamps.created  = timespec_to_ns(st.st_birthtimespec);
        #elif defined(__linux__)
            _out.timestamps.modified = timespec_to_ns(st.st_mtim);
            _out.timestamps.accessed = timespec_to_ns(st.st_atim);
            _out.timestamps.changed  = timespec_to_ns(st.st_ctim);
            _out.timestamps.created  = 0;  // no birth time on most Linux fs
            // statx() could provide stx_btime on Linux 4.11+ / ext4/btrfs;
            // layered on separately if needed.
        #else
            // generic POSIX fall (seconds only)
            _out.timestamps.modified =
                static_cast<std::uint64_t>(st.st_mtime)
                * UINT64_C(1000000000);
            _out.timestamps.accessed =
                static_cast<std::uint64_t>(st.st_atime)
                * UINT64_C(1000000000);
            _out.timestamps.changed  =
                static_cast<std::uint64_t>(st.st_ctime)
                * UINT64_C(1000000000);
            _out.timestamps.created  = 0;
        #endif

        // raw mode and ownership
        _out.mode = static_cast<std::uint16_t>(st.st_mode & 0xFFFF);
        _out.uid  = static_cast<std::uint32_t>(st.st_uid);
        _out.gid  = static_cast<std::uint32_t>(st.st_gid);

        // identity
        _out.inode      = static_cast<std::uint64_t>(st.st_ino);
        _out.link_count = static_cast<std::uint32_t>(st.st_nlink);
        _out.device     = static_cast<std::uint64_t>(st.st_dev);

        // synthesize attribute flags from st_mode.
        std::uint32_t flags = file_attr_none;

        if (S_ISDIR(st.st_mode))  { flags |= file_attr_directory;  }
        if (S_ISLNK(st.st_mode))  { flags |= file_attr_symlink |
                                              file_attr_reparse_point; }
        if (S_ISBLK(st.st_mode) ||
            S_ISCHR(st.st_mode))  { flags |= file_attr_device;     }
        #ifdef S_ISFIFO
        if (S_ISFIFO(st.st_mode)) { flags |= file_attr_pipe;       }
        #endif
        #ifdef S_ISSOCK
        if (S_ISSOCK(st.st_mode)) { flags |= file_attr_socket;     }
        #endif

        if (!(st.st_mode & S_IWUSR))
        {
            flags |= file_attr_readonly;
        }

        if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        {
            flags |= file_attr_executable;
        }

        if (st.st_mode & S_ISUID) { flags |= file_attr_setuid; }
        if (st.st_mode & S_ISGID) { flags |= file_attr_setgid; }
        #ifdef S_ISVTX
        if (st.st_mode & S_ISVTX) { flags |= file_attr_sticky; }
        #endif

        // hidden: POSIX convention — name starts with '.'.
        // Caller must check this separately as we only have
        // the path here, not a guaranteed leaf name.
        {
            const char* leaf = _path;
            const char* p    = _path;

            while (*p != '\0')
            {
                if (*p == '/')
                {
                    leaf = p + 1;
                }

                ++p;
            }

            if (leaf[0] == '.' && leaf[1] != '\0')
            {
                // skip "." and ".." (already filtered upstream)
                if (!(leaf[1] == '.' && leaf[2] == '\0'))
                {
                    flags |= file_attr_hidden;
                }
            }
        }

        // extended attributes
        #if defined(__linux__)
        {
            ssize_t xlen = listxattr(_path, nullptr, 0);

            if (xlen > 0)
            {
                flags |= file_attr_has_xattr;
            }
        }
        #elif defined(__APPLE__)
        {
            ssize_t xlen = listxattr(_path, nullptr, 0, XATTR_NOFOLLOW);

            if (xlen > 0)
            {
                flags |= file_attr_has_xattr;
            }
        }
        #endif

        _out.attr_flags = flags;

        return true;
    }

#endif  // platform

};


NS_END  // fs
NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_ATTRIBUTES_
