/*******************************************************************************
* djinterp [core]                                                       path.hpp
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
* THE SPEC (Addressability).  A path string is a RENDERING of an ADDRESS -- the
* word of labels along a path -- into a character alphabet, with a separator as
* the framing.  Everything here is that one idea:
*
*     path_split           parse   : text  -> word of labels
*     path_from_components render  : word of labels -> text
*     path_meet            the MEET of two addresses in the prefix order
*     path_relative_to     the (k, v) pair: k ascents, then a descent
*     path_starts_with     the PREFIX ORDER, which is the ancestor relation
*     path_level           lambda: the number of labels, NOT a height
*
*   THE ROOT CONTRIBUTES NO LABEL.  path_split("/a/b") is ["a","b"], not
* ["", "a", "b"] and not ["/", "a", "b"]: the leading separator says WHERE the
* address is anchored, it is not a step taken.  path_from_components is the
* faithful inverse, and it takes that anchoring as an explicit argument, because
* a word of labels alone cannot say whether it is absolute.
*
*   FAITHFULNESS.  A rendering round-trips if and only if the label rendering is
* injective and NO LABEL CONTAINS THE SEPARATOR (nor is "." or "..", which the
* parse reads as navigation).  path_label_is_renderable decides this.  Nothing
* here escapes on your behalf; the right escape belongs to the format.
*
* WHAT CHANGED, AND WHY IT MATTERS:
*   - path_common_prefix("/a/x", "/b/y") returned "".  But the MEET always
*     exists: Lambda* under the prefix order is a meet-semilattice, and two
*     absolute paths that share no component meet AT THE ROOT.  The answer is
*     "/", and "" now means only "no meet exists".
*   - path_join_components(path_split_strings("/a/b")) was "a/b" -- absoluteness
*     was dropped, so render o parse was not the identity.  path_from_components
*     takes the anchor and restores the round trip.
*   - path_normalize("/../a") was "/../a".  An address cannot ascend above its
*     root (k <= |addr(base)|), so a leading ".." on an ABSOLUTE path is not an
*     address at all.  It is now dropped: "/a".
*   - path_relative_to and path_starts_with compared an absolute path against a
*     relative one as though they were addresses in one space.  They are not,
*     and both now say so.
*   - path_depth counts labels, which is the LEVEL, not the height.  Renamed;
*     the old spelling is retained.
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
*   - path_from_components   render a label word (the faithful inverse of split)
*   - path_meet              the meet of two addresses (was path_common_prefix)
*   - path_relative_to       compute a relative path
*   - path_starts_with       component-wise prefix test
*   - path_level             count labels (was path_depth)
*   - path_label_is_renderable   can a label survive render o parse?
*
*
* path:      /inc/djinterp/core/paradigm/path/path.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.22
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_PARADIGM_PATH_PATH_HPP
#define DJINTERP_PARADIGM_PATH_PATH_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // fixed-width integers


// D_KEYWORD_PATH
//   keyword: resolves to `path`.
#define D_KEYWORD_PATH              path


NS_DJINTERP

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


// path_from_components
//   renders a word of labels back into a path string.  This is the FAITHFUL
// INVERSE of path_split_strings, and the reason it needs _absolute is that a
// word of labels cannot say where it is anchored: path_split drops the leading
// separator (correctly -- the root contributes no label), so something must
// carry that one bit back.
//
//   path_from_components(path_split_strings(p), path_has_root_separator(p)) ==
// p
//
// for any p that is already normalized and whose labels are renderable.
// path_join_components is the same rendering with _absolute = false, and is
// kept
// for callers building relative fragments.
//
//   An EMPTY word renders to "/" when absolute -- the root, whose address is
// the
// empty word -- and to "" when relative.
inline std::string
path_from_components
(
    const std::vector<std::string>& _components,
    bool                            _absolute,
    char                            _sep = path_separator
)
{
    std::string result;

    // an absolute address opens at its root separator
    if (_absolute)
    {
        result += _sep;
    }

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
//  path_label_is_renderable
// ================================================================

// path_label_is_renderable
//   returns true if _label survives the round trip through _sep: it is
// non-empty, contains no separator, and is neither "." nor "..".
//
//   A label containing the separator is torn in two by the parse; an empty
// label
// is dropped; "." and ".." are read as navigation rather than as names.  In
// each
// case the rendering is LOSSY and path_split(path_from_components(...)) is not
// the identity.  Where a label may contain the separator, the format must
// escape
// it -- exactly as a text format escapes its own delimiters.
inline bool
path_label_is_renderable
(
    const std::string& _label,
    char               _sep = path_separator
)
{
    if (_label.empty())
    {
        return false;
    }

    // "." and ".." are navigation, not names
    if ( (_label == ".") ||
         (_label == "..") )
    {
        return false;
    }

    for (std::size_t i = 0; i < _label.size(); ++i)
    {
        if ( (_label[i] == _sep) ||
             (path_is_separator(_label[i])) )
        {
            return false;
        }
    }

    return true;
}

// path_labels_are_renderable
//   returns true if every label of _components is renderable -- the condition
// under which path_from_components and path_split are mutually inverse.
inline bool
path_labels_are_renderable
(
    const std::vector<std::string>& _components,
    char                            _sep = path_separator
)
{
    for (std::size_t i = 0; i < _components.size(); ++i)
    {
        if (!path_label_is_renderable(_components[i], _sep))
        {
            return false;
        }
    }

    return true;
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
    // an ABSOLUTE path is anchored at a root, and an address cannot ascend
    // above
    // its root: k <= |addr(base)|.  A RELATIVE path has no root to be stopped
    // by,
    // so its leading ".." are part of the (k, v) address and must survive.
    const bool absolute =
        ( (!_path.empty()) &&
          (path_is_separator(_path[0])) );

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
            else if (!absolute)
            {
                // relative: the leading ".." is the ascent count k, and is kept
                stack.push_back(comp);
            }

            // absolute: an ascent above the root is not an address.  Drop it --
            // "/../a" is "/a", as it must be.

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
//  path_level
// ================================================================

// path_has_root_separator
//   returns true if _path opens with a separator -- the only ROOT the component
// algebra of this header models.
//
//   A Windows drive letter or a UNC share is also "absolute" to
// path_is_absolute, but it carries a ROOT NAME, which path_split has no notion
// of: it drops "C:" into an ordinary component like any other.  So wherever the
// question is "are these two paths ANCHORED ALIKE" -- which is the question the
// meet, the relative address, and the prefix order all rest on -- ask this, not
// path_is_absolute.
inline bool
path_has_root_separator
(
    const std::string& _path
)
{
    return ( (!_path.empty()) &&
             (path_is_separator(_path[0])) );
}

// path_have_common_anchor
//   returns true if _a and _b are anchored alike, and so are addresses in ONE
// space.  An absolute path and a relative one are not: there is no tree in
// which
// both name a component, so they have no meet, no relative address, and stand
// in
// no prefix relation.  Every operation below that compares two paths asks this
// first.
inline bool
path_have_common_anchor
(
    const std::string& _a,
    const std::string& _b
)
{
    return (path_has_root_separator(_a) == path_has_root_separator(_b));
}

// path_level
//   returns the LEVEL (lambda) of _path: the number of labels in it, which is
// the length of its address and the number of descents it makes from its
// anchor.
//
//   It is NOT the spec's depth, which is a node's HEIGHT -- measured downward,
// to
// its deepest leaf.  Level counts labels; height counts levels BELOW a node.
// The
// two are different numbers and this function only ever computed the first.
inline std::size_t
path_level
(
    const std::string& _path
)
{
    return path_split(_path).size();
}

// path_depth
//   the retained spelling of path_level.  It counts labels, which is the LEVEL;
// prefer path_level.
inline std::size_t
path_depth
(
    const std::string& _path
)
{
    return path_level(_path);
}


// ================================================================
//  path_meet
// ================================================================

// path_meet
//   returns the MEET of _a and _b in the prefix order: their longest common
// prefix, rendered.
//
//   THE MEET ALWAYS EXISTS when the two are anchored alike.  Lambda* under the
// prefix order is a MEET-SEMILATTICE, and the worst case is the empty word.  So
// two ABSOLUTE paths sharing no component meet AT THE ROOT, and the answer is
// "/" -- the rendering of the empty address at an absolute anchor -- not "".
// The old path_common_prefix returned "" there, conflating "they meet at the
// root" with "they do not meet", which for two absolute paths are never the
// same
// thing.  It is the meet that makes the lowest common ancestor total, and that
// totality is exactly what was being thrown away.
//
//   Two paths anchored DIFFERENTLY have no meet at all; the result is "", and
// path_have_common_anchor says so in advance.
inline std::string
path_meet
(
    const std::string& _a,
    const std::string& _b,
    char               _sep = path_separator
)
{
    // paths anchored differently are not addresses in one space
    if (!path_have_common_anchor(_a, _b))
    {
        return std::string();
    }

    auto ca = path_split_strings(_a);
    auto cb = path_split_strings(_b);

    std::size_t limit = (ca.size() < cb.size())
                      ? ca.size() : cb.size();

    std::vector<std::string> common;

    // the longest common prefix
    for (std::size_t i = 0; i < limit; ++i)
    {
        if (ca[i] != cb[i])
        {
            break;
        }

        common.push_back(ca[i]);
    }

    // an EMPTY meet is still a meet: for two absolute paths it is the root
    return path_from_components(
        common,
        path_has_root_separator(_a),
        _sep);
}

// path_common_prefix
//   the retained spelling of path_meet.  Note that its behaviour has CHANGED
// for two absolute paths with nothing in common: the answer is now "/", the
// root, and no longer "".
inline std::string
path_common_prefix
(
    const std::string& _a,
    const std::string& _b,
    char               _sep = path_separator
)
{
    return path_meet(_a, _b, _sep);
}


// ================================================================
//  path_relative_to
// ================================================================

// path_relative_to
//   computes the relative path from _base to _target -- the (k, v) pair of the
// spec, rendered: k copies of ".." for the ascents to the meet, then the labels
// descending from the meet to the target.
//
//   Both paths should be normalized first for best results.
//
//   Returns "." for the same path -- the (0, <>) address -- and the EMPTY
// string
// when the two are NOT ANCHORED ALIKE, since an absolute base and a relative
// target are not addresses in one space and no (k, v) relates them.  The old
// version happily produced "../../c/d" for that pair, which named nothing.
inline std::string
path_relative_to
(
    const std::string& _base,
    const std::string& _target,
    char               _sep = path_separator
)
{
    // (k, v) relates two addresses in ONE space, or it relates nothing
    if (!path_have_common_anchor(_base, _target))
    {
        return std::string();
    }

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
//   returns true if _path starts with _prefix, compared component-wise (not
// character-wise).
//
//   This is the PREFIX ORDER, and so -- by the spec -- it is the ANCESTOR
// relation: _prefix names an ancestor of what _path names exactly when it is a
// prefix of it.  The order relates addresses in ONE space, so two paths
// anchored
// differently stand in no prefix relation at all, however their labels line up:
// "a" is not an ancestor of "/a/b", because "a" names nothing in the tree
// "/a/b" is addressed in.
inline bool
path_starts_with
(
    const std::string& _path,
    const std::string& _prefix
)
{
    // the prefix order relates addresses anchored alike
    if (!path_have_common_anchor(_path, _prefix))
    {
        return false;
    }

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


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARADIGM_PATH_PATH_HPP
