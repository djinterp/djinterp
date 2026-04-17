/******************************************************************************
* djinterp [core]                                                     path.hpp
*
* Generic path manipulation utilities:
*   This header provides container-agnostic, string-based path operations.
* It knows nothing about arenas, trees, or filesystems — only about the
* structure of hierarchical path strings: components separated by '/' or
* '\', with optional roots and extensions.
*
*   All functions are pure: they operate on string data and return new
* strings or view-like descriptors.  No OS calls, no allocations beyond
* the returned strings.
*
* Contents:
*   - path_separator         platform default separator
*   - path_component         lightweight view into a path string
*   - path_split             decompose a path into components
*   - path_join              combine components into a path
*   - path_normalize         collapse separators, resolve . and ..
*   - path_parent            remove the last component
*   - path_filename          extract the last component
*   - path_stem              filename without extension
*   - path_extension         extract the extension (with dot)
*   - path_is_absolute       detect absolute paths
*   - path_is_separator      test a character
*   - path_common_prefix     longest shared ancestor path
*   - path_relative_to       compute a relative path
*   - path_starts_with       component-wise prefix test
*   - path_depth             count components
*
*
* path:      /inc/cpp/core/path.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_CORE_PATH_
#define DJINTERP_CORE_PATH_ 1

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../djinterp.hpp"


// D_KEYWORD_PATH
//   keyword: resolves to `path`.
#define D_KEYWORD_PATH              path

// NS_PATH
//   namespace: the path utilities namespace.
#define NS_PATH                     D_NAMESPACE(D_KEYWORD_PATH)


NS_DJINTERP
NS_PATH


// ================================================================
//  path_separator
// ================================================================

// path_separator
//   constant: the platform-preferred path separator character.
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    static D_CONSTEXPR char path_separator = '\\';
#else
    static D_CONSTEXPR char path_separator = '/';
#endif

// path_is_separator
//   returns true if _c is a path separator on any platform.
// Both '/' and '\\' are always recognized for portability.
D_STATIC_INLINE
bool
path_is_separator
(
    char _c
)
{
    return (_c == '/' || _c == '\\');
}


// ================================================================
//  path_component
// ================================================================

// path_component
//   struct: a lightweight, non-owning view into a path string.
// Represents a single component (directory name, filename, etc.)
// by offset and length into the original string.
struct path_component
{
    std::size_t offset;
    std::size_t length;

    // path_component (default)
    path_component
    ()
        : offset(0),
          length(0)
    {}

    // path_component (parameterized)
    path_component
    (
        std::size_t _offset,
        std::size_t _length
    )
        : offset(_offset),
          length(_length)
    {}

    // empty
    //   returns true if the component has zero length.
    bool
    empty() const
    {
        return (length == 0);
    }

    // extract
    //   copies the component text from the source string.
    std::string
    extract
    (
        const std::string& _source
    ) const
    {
        return _source.substr(offset, length);
    }

    // extract (const char* overload)
    std::string
    extract
    (
        const char* _source
    ) const
    {
        return std::string(_source + offset, length);
    }
};


// ================================================================
//  path_split
// ================================================================

// path_split
//   decomposes _path into its individual components.
// Leading separators are consumed but not emitted.
// Empty components (from double separators) are skipped.
inline std::vector<path_component>
path_split
(
    const char* _path,
    std::size_t _len
)
{
    std::vector<path_component> result;

    std::size_t i = 0;

    while (i < _len)
    {
        // skip separators.
        while (i < _len && path_is_separator(_path[i]))
        {
            ++i;
        }

        if (i >= _len)
        {
            break;
        }

        // find end of component.
        std::size_t start = i;

        while (i < _len && !path_is_separator(_path[i]))
        {
            ++i;
        }

        result.push_back(path_component(start, i - start));
    }

    return result;
}

// path_split (std::string overload)
inline std::vector<path_component>
path_split
(
    const std::string& _path
)
{
    return path_split(_path.c_str(), _path.size());
}

// path_split_strings
//   convenience: returns component strings directly.
inline std::vector<std::string>
path_split_strings
(
    const std::string& _path
)
{
    auto components = path_split(_path);
    std::vector<std::string> result;

    result.reserve(components.size());

    for (const auto& comp : components)
    {
        result.push_back(comp.extract(_path));
    }

    return result;
}


// ================================================================
//  path_join
// ================================================================

// path_join
//   combines two path strings with a single separator between
// them.  Does not normalize.
inline std::string
path_join
(
    const std::string& _left,
    const std::string& _right,
    char               _sep = path_separator
)
{
    if (_left.empty())
    {
        return _right;
    }

    if (_right.empty())
    {
        return _left;
    }

    bool left_has_sep  = path_is_separator(_left.back());
    bool right_has_sep = path_is_separator(_right.front());

    if (left_has_sep && right_has_sep)
    {
        return _left + _right.substr(1);
    }

    if (left_has_sep || right_has_sep)
    {
        return _left + _right;
    }

    return _left + _sep + _right;
}

// path_join (variadic — three or more)
inline std::string
path_join
(
    const std::string& _a,
    const std::string& _b,
    const std::string& _c,
    char               _sep = path_separator
)
{
    return path_join(path_join(_a, _b, _sep), _c, _sep);
}

// path_join (vector of components)
inline std::string
path_join_components
(
    const std::vector<std::string>& _components,
    char                            _sep = path_separator
)
{
    std::string result;

    for (std::size_t i = 0; i < _components.size(); ++i)
    {
        if (i > 0)
        {
            result += _sep;
        }

        result += _components[i];
    }

    return result;
}


// ================================================================
//  path_normalize
// ================================================================

// path_normalize
//   collapses consecutive separators, resolves "." (current dir)
// and ".." (parent dir) components, and normalizes all separators
// to _sep.  Does not touch the filesystem.
inline std::string
path_normalize
(
    const std::string& _path,
    char               _sep = path_separator
)
{
    auto components = path_split_strings(_path);
    std::vector<std::string> stack;

    for (const auto& comp : components)
    {
        if (comp == ".")
        {
            continue;
        }

        if (comp == "..")
        {
            if (!stack.empty() && stack.back() != "..")
            {
                stack.pop_back();
            }
            else
            {
                // preserve leading ".." for relative paths.
                stack.push_back(comp);
            }

            continue;
        }

        stack.push_back(comp);
    }

    std::string result;

    // preserve leading separator for absolute paths.
    if (!_path.empty() && path_is_separator(_path[0]))
    {
        result += _sep;
    }

    for (std::size_t i = 0; i < stack.size(); ++i)
    {
        if (i > 0)
        {
            result += _sep;
        }

        result += stack[i];
    }

    if (result.empty())
    {
        result = ".";
    }

    return result;
}


// ================================================================
//  path_parent
// ================================================================

// path_parent
//   removes the last component from _path.
// Returns "." for a path with no parent.
inline std::string
path_parent
(
    const std::string& _path
)
{
    if (_path.empty())
    {
        return ".";
    }

    // find the last separator, ignoring trailing separators.
    std::size_t end = _path.size();

    while (end > 0 && path_is_separator(_path[end - 1]))
    {
        --end;
    }

    if (end == 0)
    {
        // all separators — root.
        return _path.substr(0, 1);
    }

    std::size_t pos = end;

    while (pos > 0 && !path_is_separator(_path[pos - 1]))
    {
        --pos;
    }

    if (pos == 0)
    {
        return ".";
    }

    // skip trailing separator of parent, but keep root "/".
    std::size_t parent_end = pos;

    while (parent_end > 1 &&
           path_is_separator(_path[parent_end - 1]))
    {
        --parent_end;
    }

    return _path.substr(0, parent_end);
}


// ================================================================
//  path_filename
// ================================================================

// path_filename
//   extracts the last component of _path (the filename or
// leaf directory name).
inline std::string
path_filename
(
    const std::string& _path
)
{
    if (_path.empty())
    {
        return std::string();
    }

    std::size_t end = _path.size();

    // skip trailing separators.
    while (end > 0 && path_is_separator(_path[end - 1]))
    {
        --end;
    }

    if (end == 0)
    {
        return std::string();
    }

    std::size_t start = end;

    while (start > 0 && !path_is_separator(_path[start - 1]))
    {
        --start;
    }

    return _path.substr(start, end - start);
}


// ================================================================
//  path_stem / path_extension
// ================================================================

// path_extension
//   extracts the extension from the filename, including the
// leading dot.  Returns empty string if no extension.
inline std::string
path_extension
(
    const std::string& _path
)
{
    std::string fname = path_filename(_path);

    if (fname.empty())
    {
        return std::string();
    }

    // find last dot, but not if it's the first character
    // (hidden file, e.g. ".gitignore" has no extension).
    std::size_t dot = fname.rfind('.');

    if (dot == std::string::npos || dot == 0)
    {
        return std::string();
    }

    return fname.substr(dot);
}

// path_stem
//   returns the filename without its extension.
inline std::string
path_stem
(
    const std::string& _path
)
{
    std::string fname = path_filename(_path);

    if (fname.empty())
    {
        return std::string();
    }

    std::size_t dot = fname.rfind('.');

    if (dot == std::string::npos || dot == 0)
    {
        return fname;
    }

    return fname.substr(0, dot);
}

// path_replace_extension
//   returns _path with the extension replaced by _new_ext.
// _new_ext should include the leading dot.
inline std::string
path_replace_extension
(
    const std::string& _path,
    const std::string& _new_ext
)
{
    std::string parent = path_parent(_path);
    std::string stem   = path_stem(_path);

    if (parent == ".")
    {
        return stem + _new_ext;
    }

    return path_join(parent, stem + _new_ext);
}


// ================================================================
//  path_is_absolute
// ================================================================

// path_is_absolute
//   returns true if _path is an absolute path.
// Recognizes:
//   /path         (POSIX)
//   C:\path       (Windows drive letter)
//   \\server      (UNC)
inline bool
path_is_absolute
(
    const std::string& _path
)
{
    if (_path.empty())
    {
        return false;
    }

    // POSIX absolute.
    if (_path[0] == '/')
    {
        return true;
    }

    // Windows drive letter: C:\ or C:/
    if (_path.size() >= 3 &&
        ((_path[0] >= 'A' && _path[0] <= 'Z') ||
         (_path[0] >= 'a' && _path[0] <= 'z')) &&
        _path[1] == ':' &&
        path_is_separator(_path[2]))
    {
        return true;
    }

    // UNC path: \\server or //server
    if (_path.size() >= 2 &&
        path_is_separator(_path[0]) &&
        path_is_separator(_path[1]))
    {
        return true;
    }

    return false;
}

// path_is_relative
//   returns true if _path is not absolute.
inline bool
path_is_relative
(
    const std::string& _path
)
{
    return !path_is_absolute(_path);
}


// ================================================================
//  path_depth
// ================================================================

// path_depth
//   returns the number of components in _path.
inline std::size_t
path_depth
(
    const std::string& _path
)
{
    return path_split(_path).size();
}


// ================================================================
//  path_common_prefix
// ================================================================

// path_common_prefix
//   returns the longest component-wise common prefix of _a
// and _b as a path string.  Returns empty string if no
// components match.
inline std::string
path_common_prefix
(
    const std::string& _a,
    const std::string& _b,
    char               _sep = path_separator
)
{
    auto ca = path_split_strings(_a);
    auto cb = path_split_strings(_b);

    std::size_t limit = (ca.size() < cb.size())
                      ? ca.size() : cb.size();

    std::vector<std::string> common;

    for (std::size_t i = 0; i < limit; ++i)
    {
        if (ca[i] != cb[i])
        {
            break;
        }

        common.push_back(ca[i]);
    }

    if (common.empty())
    {
        return std::string();
    }

    std::string result;

    // preserve leading separator.
    if (!_a.empty() && path_is_separator(_a[0]) &&
        !_b.empty() && path_is_separator(_b[0]))
    {
        result += _sep;
    }

    result += path_join_components(common, _sep);

    return result;
}


// ================================================================
//  path_relative_to
// ================================================================

// path_relative_to
//   computes the relative path from _base to _target.
// Both paths should be normalized first for best results.
inline std::string
path_relative_to
(
    const std::string& _base,
    const std::string& _target,
    char               _sep = path_separator
)
{
    auto cb = path_split_strings(_base);
    auto ct = path_split_strings(_target);

    // find common prefix length.
    std::size_t common = 0;
    std::size_t limit  = (cb.size() < ct.size())
                       ? cb.size() : ct.size();

    while (common < limit && cb[common] == ct[common])
    {
        ++common;
    }

    // climb up from base to common ancestor.
    std::vector<std::string> parts;

    for (std::size_t i = common; i < cb.size(); ++i)
    {
        parts.push_back("..");
    }

    // descend from common ancestor to target.
    for (std::size_t i = common; i < ct.size(); ++i)
    {
        parts.push_back(ct[i]);
    }

    if (parts.empty())
    {
        return ".";
    }

    return path_join_components(parts, _sep);
}


// ================================================================
//  path_starts_with / path_ends_with
// ================================================================

// path_starts_with
//   returns true if _path starts with _prefix, compared
// component-wise (not character-wise).
inline bool
path_starts_with
(
    const std::string& _path,
    const std::string& _prefix
)
{
    auto cp = path_split_strings(_path);
    auto cx = path_split_strings(_prefix);

    if (cx.size() > cp.size())
    {
        return false;
    }

    for (std::size_t i = 0; i < cx.size(); ++i)
    {
        if (cp[i] != cx[i])
        {
            return false;
        }
    }

    return true;
}

// path_ends_with
//   returns true if _path ends with _suffix, compared
// component-wise.
inline bool
path_ends_with
(
    const std::string& _path,
    const std::string& _suffix
)
{
    auto cp = path_split_strings(_path);
    auto cx = path_split_strings(_suffix);

    if (cx.size() > cp.size())
    {
        return false;
    }

    std::size_t off = cp.size() - cx.size();

    for (std::size_t i = 0; i < cx.size(); ++i)
    {
        if (cp[off + i] != cx[i])
        {
            return false;
        }
    }

    return true;
}


// ================================================================
//  path_to_posix / path_to_windows
// ================================================================

// path_to_posix
//   replaces all backslashes with forward slashes.
inline std::string
path_to_posix
(
    const std::string& _path
)
{
    std::string result = _path;

    for (char& c : result)
    {
        if (c == '\\')
        {
            c = '/';
        }
    }

    return result;
}

// path_to_windows
//   replaces all forward slashes with backslashes.
inline std::string
path_to_windows
(
    const std::string& _path
)
{
    std::string result = _path;

    for (char& c : result)
    {
        if (c == '/')
        {
            c = '\\';
        }
    }

    return result;
}


NS_END  // path
NS_END  // djinterp


#endif  // DJINTERP_CORE_PATH_
