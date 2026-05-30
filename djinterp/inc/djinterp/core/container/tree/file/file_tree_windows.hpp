/******************************************************************************
* djinterp [fs]                                           file_tree_windows.hpp
*
* Windows file tree scanner:
*   Defines windows_scanner and windows10_scanner.  The Win32 directory
* APIs return size, type, and timestamps inline with enumeration, so -
* unlike the POSIX baseline - no second metadata call per entry is
* needed.  windows_scanner uses FindFirstFileExW / FindNextFileW;
* windows10_scanner additionally passes FindExInfoBasic and
* FIND_FIRST_EX_LARGE_FETCH to skip 8.3 short-name generation and pull
* larger batches per call, which measurably speeds enumeration on
* modern Windows.
*
*   All names are converted from UTF-16 to UTF-8 at the API boundary
* before being interned, matching the core's UTF-8 storage contract.
*
*
* path:      /inc/cpp/fs/file_tree_windows.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_FS_FILE_TREE_WINDOWS_
#define DJINTERP_FS_FILE_TREE_WINDOWS_ 1

#include "./file_tree_common.hpp"

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif

#include <string>


NS_DJINTERP
NS_FS


// ================================================================
//  windows_scanner
// ================================================================

// windows_scanner
//   policy: Win32 directory walk via FindFirstFileExW.  Pulls type
// and size from WIN32_FIND_DATAW with no per-entry stat.
//
//   The _LargeFetch template parameter selects the modern
// enumeration hints; windows10_scanner is the specialization with
// it enabled.
template<bool _LargeFetch>
struct basic_windows_scanner
{
#if defined(_WIN32)

    // narrow
    //   converts a UTF-16 buffer to UTF-8.
    static std::string
    narrow(
        const wchar_t* _wide
    )
    {
        if (_wide == nullptr || _wide[0] == L'\0')
        {
            return std::string();
        }

        int needed = ::WideCharToMultiByte(
            CP_UTF8, 0, _wide, -1,
            nullptr, 0, nullptr, nullptr);

        if (needed <= 1)
        {
            return std::string();
        }

        std::string out(static_cast<std::size_t>(needed - 1), '\0');

        ::WideCharToMultiByte(
            CP_UTF8, 0, _wide, -1,
            &out[0], needed, nullptr, nullptr);

        return out;
    }

    // widen
    //   converts a UTF-8 string to UTF-16 for Win32 APIs.
    static std::wstring
    widen(
        const std::string& _utf8
    )
    {
        if (_utf8.empty())
        {
            return std::wstring();
        }

        int needed = ::MultiByteToWideChar(
            CP_UTF8, 0, _utf8.data(),
            static_cast<int>(_utf8.size()), nullptr, 0);

        std::wstring out(static_cast<std::size_t>(needed), L'\0');

        ::MultiByteToWideChar(
            CP_UTF8, 0, _utf8.data(),
            static_cast<int>(_utf8.size()), &out[0], needed);

        return out;
    }

    // classify
    static file_type
    classify(
        DWORD _attrs
    )
    {
        if (_attrs & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            return file_type_symlink;
        }

        if (_attrs & FILE_ATTRIBUTE_DIRECTORY)
        {
            return file_type_directory;
        }

        return file_type_regular;
    }

    // is_dot_entry
    static bool
    is_dot_entry(
        const wchar_t* _name
    )
    {
        if (_name[0] != L'.')
        {
            return false;
        }

        if (_name[1] == L'\0')
        {
            return true;
        }

        return (_name[1] == L'.' && _name[2] == L'\0');
    }

    // scan
    template<typename _Ctx>
    static void
    scan(
        _Ctx&              _ctx,
        const std::string& _dir_path,
        node_id            _parent
    )
    {
        std::wstring pattern = widen(_dir_path + "\\*");

        WIN32_FIND_DATAW fd;

        HANDLE h;

        if (_LargeFetch)
        {
            h = ::FindFirstFileExW(
                pattern.c_str(),
                FindExInfoBasic,            // skip 8.3 names
                &fd,
                FindExSearchNameMatch,
                nullptr,
                FIND_FIRST_EX_LARGE_FETCH); // bigger batches
        }
        else
        {
            h = ::FindFirstFileExW(
                pattern.c_str(),
                FindExInfoStandard,
                &fd,
                FindExSearchNameMatch,
                nullptr,
                0);
        }

        if (h == INVALID_HANDLE_VALUE)
        {
            return;
        }

        do
        {
            if (is_dot_entry(fd.cFileName))
            {
                continue;
            }

            std::string child_name = narrow(fd.cFileName);

            file_type type = classify(fd.dwFileAttributes);

            std::uint64_t sz =
                (static_cast<std::uint64_t>(fd.nFileSizeHigh) << 32) |
                static_cast<std::uint64_t>(fd.nFileSizeLow);

            node_id id = _ctx.intern_child(
                _parent,
                child_name.c_str(),
                child_name.size(),
                type,
                sz);

            if (type == file_type_directory)
            {
                _ctx.recurse(_dir_path + "\\" + child_name, id);
            }

        } while (::FindNextFileW(h, &fd));

        ::FindClose(h);

        return;
    }

#else  // !_WIN32

    // off-platform stub: compiles, scans nothing, so the umbrella
    // can name this policy on any host.
    template<typename _Ctx>
    static void
    scan(_Ctx&, const std::string&, node_id)
    {
        return;
    }

#endif  // _WIN32
};


// windows_scanner
//   policy: standard Win32 enumeration.
using windows_scanner = basic_windows_scanner<false>;

// windows10_scanner
//   policy: Win32 enumeration with FindExInfoBasic +
// FIND_FIRST_EX_LARGE_FETCH.  Also the Windows 11 backend.
using windows10_scanner = basic_windows_scanner<true>;


NS_END  // fs
NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_TREE_WINDOWS_
