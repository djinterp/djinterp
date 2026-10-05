/*******************************************************************************
* djinterp [core]                                                    archive.cpp
*
*   The C++ facade's dispatch leaves -- four adapters, and nothing else.
*
*   WHAT THIS FILE USED TO BE:
*   tar and zip writers, plus libarchive / libzip / minizip / 7-Zip / unrar
* bindings, each behind its own #if.  All of it now lives once in
* archive_common.c, which the C fork compiles too.  The dependency-free ustar
* and ZIP writers AND readers went with it, which is what closed the `BUILTIN`
* defect: tar and zip no longer depend on which third-party headers happen to
* be installed on the build machine.
*
*   WHAT AN ADAPTER DOES:
*   Lower the C++ types to the kernel's borrowed forms, call in, and -- for
* extraction -- lift the result back.  Nothing here decides a default, selects
* a backend, or knows what a container looks like.
*
*   WHERE THE ALLOCATION LIVES, AND WHY THAT IS CORRECT:
*   The kernel allocates nothing; it takes caller-owned regions.  This file is
* the C++ face, std::vector and std::string are its native vocabulary, and it
* is the right place for the two allocations an extraction needs: an entry
* array and a byte arena the entries point into.  Pushing them down into the
* kernel would put an allocator in tier 0, where every other module would then
* inherit it.
*
*   EXTRACTION IS MEASURE-THEN-FILL, and the retry loop is the interesting
* part: the kernel reports both region sizes before the fact, so there is
* exactly one growth step and never a guess-and-double.
*
*
* path:      /src/djinterp/core/util/archive/archive.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../../inc/djinterp/core/util/archive/archive.hpp"


// std
#include <vector>
#include "../../../../../inc/djinterp/env/util/compress/env_compress_link.h"


NS_DJINTERP

namespace internal
{

// entry_sink_write
//   function: the sink write that appends into a byte_blob.  Same shape as
// compress.cpp's, and deliberately not shared with it: exporting a sink
// implementation from either translation unit would put it in the public
// surface, where a caller could bind to it and depend on it.
static std::size_t
entry_sink_write(
    void*        _context,
    const void*  _data,
    std::size_t  _size
)
{
    byte_blob* out = static_cast<byte_blob*>(_context);

    if (out == 0)
    {
        return 0;
    }

    out->append(static_cast<const char*>(_data), _size);

    return _size;
}

// lower_entry
//   function: the kernel's view of one entry.  Every span BORROWS from _e,
// which must outlive the call.
static d_archive_entry
lower_entry(
    const entry&  _e
)
{
    d_archive_entry out;

    out.name.data     = _e.name.empty() ? (const char*)0 : _e.name.data();
    out.name.length   = _e.name.size();
    out.data.data     = _e.data.empty() ? (const void*)0 : _e.data.data();
    out.data.size     = _e.data.size();
    out.is_directory  = _e.is_directory ? 1 : 0;
    out.mode          = (uint32_t)_e.mode;
    // widen deliberately: `long` is 32 bits on LLP64 and 64 on LP64, so the
    // kernel pins int64_t and the conversion happens exactly here.
    out.mtime         = (int64_t)_e.mtime;

    return out;
}

// lift_entry
//   function: a C++ entry from the kernel's borrowed form.  COPIES, because
// the kernel's spans point into an arena this function is about to discard.
static entry
lift_entry(
    const d_archive_entry&  _e
)
{
    entry out;

    if ( (_e.name.data != 0) &&
         (_e.name.length != 0) )
    {
        out.name.assign(_e.name.data, _e.name.length);
    }
    if ( (_e.data.data != 0) &&
         (_e.data.size != 0) )
    {
        out.data.assign(static_cast<const char*>(_e.data.data), _e.data.size);
    }

    out.is_directory = (_e.is_directory != 0);
    out.mode         = (unsigned int)_e.mode;
    out.mtime        = (long)_e.mtime;

    return out;
}

/*
archive_create
  Build an archive of [_items, _items + _count) in format _fmt into _out.

  _out is CLEARED first: the kernel appends, so a caller reusing a blob would
otherwise concatenate two archives into a file that is valid up to the first
end-of-archive marker and silently truncated after it.

Parameter(s):
  _fmt:   the container format.
  _items: the members, in order; may be null when _count is 0.
  _count: how many.
  _opt:   the tuning; UNSET knobs are resolved by the kernel.
  _out:   receives the container bytes.
Return:
  status_ok, or the kernel's status unchanged.
*/
status
archive_create(
    format_id               _fmt,
    const entry*            _items,
    std::size_t             _count,
    const archive_options&  _opt,
    byte_blob&              _out
)
{
    std::vector<d_archive_entry>  lowered;
    d_archive_options             opt = _opt.lower();
    d_pack_sink                   sink;
    const d_archive_entry*        base;
    std::size_t                   i;

    _out.clear();

    lowered.reserve(_count);

    for (i = 0; i < _count; ++i)
    {
        lowered.push_back(lower_entry(_items[i]));
    }

    // an empty vector must not be indexed; pass a null base with count 0
    base = lowered.empty() ? (const d_archive_entry*)0 : &lowered[0];

    sink.write   = &entry_sink_write;
    sink.context = &_out;

    return d_archive_create_to_sink(_fmt, base, _count, &opt, sink, 0);
}

/*
archive_extract
  Extract the archive in [_in, _in + _n) of format _fmt into _out.

  Measure, then fill.  The kernel reports the entry count and the arena size
before the fact, so the two regions are sized once and there is no
guess-and-double loop.  A second BUFFER_TOO_SMALL after growing to the reported
size would mean the kernel disagreed with itself, so it is surfaced rather than
retried again.

Parameter(s):
  _fmt: the container format.
  _in:  the archive bytes.
  _n:   how many.
  _out: receives the members; cleared first.
Return:
  status_ok, or the kernel's status unchanged.
*/
status
archive_extract(
    format_id    _fmt,
    const char*  _in,
    std::size_t  _n,
    entry_list&  _out
)
{
    archive_options               defaults;
    d_archive_options             opt = defaults.lower();
    d_archive_layout              layout;
    std::vector<d_archive_entry>  slots;
    std::vector<char>             arena;
    status                        s;
    std::size_t                   i;

    _out.clear();

    s = d_archive_measure(_fmt, _in, _n, &opt, &layout);

    if (s != status_ok)
    {
        return s;
    }
    if (layout.entry_count == 0)
    {
        return status_ok;
    }

    slots.resize(layout.entry_count);
    arena.resize(layout.arena_size ? layout.arena_size : 1);

    s = d_archive_extract(_fmt, _in, _n, &opt,
                          &slots[0], slots.size(),
                          &arena[0], arena.size(),
                          &layout);

    if (s != status_ok)
    {
        return s;
    }

    _out.reserve(layout.entry_count);

    for (i = 0; i < layout.entry_count; ++i)
    {
        _out.push_back(lift_entry(slots[i]));
    }

    return status_ok;
}

/*
format_can_write
  Runtime write capability of a format id.

Parameter(s):
  _fmt: the format to query.
Return:
  true when this build can create that format.
*/
bool
format_can_write(
    format_id  _fmt
)
{
    return (d_format_is_writable(_fmt) != 0);
}

/*
format_can_read
  Runtime read capability of a format id.

  Kept separate from write capability on purpose: RAR is readable by several
backends and writable by none, so a single "supported" predicate would make a
writer report that formal fact as though the format were unknown.

Parameter(s):
  _fmt: the format to query.
Return:
  true when this build can read that format.
*/
bool
format_can_read(
    format_id  _fmt
)
{
    return (d_format_is_readable(_fmt) != 0);
}

}  // namespace internal

NS_END  // djinterp
