/******************************************************************************
* djinterp [core]                                            document_bundle.hpp
*
*   The assembly half of the document-output stack: a bundle collects the
* documents a run produced - each a (logical name, extension, DEFERRED
* producer) - and write() drives them through the output_packaging vocabulary
* to a sink.  Content production (state -> byte_buffer, in binding_env /
* the format skeletons / test_document) and assembly (bytes -> files / archive,
* here) are the two composing halves of the pipeline:
*
*     DTest state
*        -> [ binding_env + format skeleton ]   one byte_buffer per document
*        -> [ document_bundle: collate + name ]  THIS LAYER
*        -> [ output_packaging: none|compress|archive ]
*        -> sink (disk or in-memory buffer)
*
*   DEFERRED PRODUCERS:
*   A bundle item holds a producer_type (std::function<byte_buffer()>), not a
* rendered buffer.  Rendering is deferred to write() time, so a producer may
* close over a stack-built template / binding_env that has gone out of scope by
* the time the bytes are wanted, and an item that is never written is never
* rendered.  The carrier is std::function for parity with the rest of the stack
* (binding_env's projection_type, section's count_fn / refocus_fn are all
* std::function); producer.hpp's pull-based stream type is a different tool, for
* sequences of values rather than a single deferred document.
*
*   THE WRITE ORCHESTRATOR:
*   write(bundle, cfg, sink) renders each item once, applies cfg's packaging
* policy, and hands the result(s) to the sink.  In `none` / `compress` mode each
* document is an independent (name, bytes) write; in `archive` mode every
* document becomes an entry of one container and the sink receives a single
* (container name, archive bytes) write.  The codec / container runtime
* selectors are dispatched through the compression.hpp / archive.hpp internal
* leaves, so any codec or container the build detected is reachable.
*
*   NON-THROWING ORCHESTRATION:
*   write() returns bool (true iff every document rendered, packaged, and wrote)
* in keeping with the facades' status-returning style; it raises nothing of its
* own.  A producer is an arbitrary host closure: one that itself throws will
* propagate (write() does not wrap it), exactly as a binding_env projection
* would - producers are expected to be as well-behaved as projections.
*
*   DTEST-AGNOSTIC:
*   document_bundle names no format, no test_report, no test_options.  A custom
* test_kind that adds documents adds bundle items; a custom destination is an
* output_sink subclass.  Both touch this layer not at all.
*
*   PORTABILITY:
*   C++17 (it composes output_packaging); self-suppresses below the floor.
*
*
* TABLE OF CONTENTS
* =================
* I.    BUNDLE ITEM             (bundle_item: name + ext + deferred producer)
* II.   DOCUMENT BUNDLE         (document_bundle: the fluent collection)
* III.  WRITE ORCHESTRATOR      (write: render + package + sink)
* IV.   CONVENIENCE WRITES      (write_to_disk / write_to_buffer / write_custom)
*
*
* path:      /inc/djinterp/core/document_bundle.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.27
******************************************************************************/

#ifndef DJINTERP_CORE_DOCUMENT_BUNDLE_
#define DJINTERP_CORE_DOCUMENT_BUNDLE_ 1

// std
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "./djinterp.hpp"                  // NS_*, D_NODISCARD, D_NOEXCEPT, gates
#include "./compression.hpp"               // byte_buffer, status, internal leaves
#include "./archive.hpp"                   // entry, entry_list, internal leaves
#include "./output/output_packaging.hpp"   // pack_mode, output_config, sinks, ...


// document_bundle composes output_packaging (C++17); below the floor it
// contributes nothing rather than failing to compile.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   BUNDLE ITEM                                          ///
///////////////////////////////////////////////////////////////////////////////

// bundle_item
//   struct: one document of a run, named for output and rendered on demand.
// `name` + `ext` feed the naming policy (output_config::naming); `produce` is
// the DEFERRED render that yields the document's bytes when write() asks for
// them.
struct bundle_item
{
    // producer_type
    //   type: the deferred render - a nullary thunk yielding the document's
    // bytes.  std::function for parity with the rest of the document stack.
    using producer_type = std::function<byte_buffer()>;

    // name: the logical name handed to the naming policy (e.g. "report").
    std::string    name;

    // ext: the extension handed to the naming policy (e.g. ".pdf").
    std::string    ext;

    // produce: the deferred render; invoked exactly once, at write() time.
    producer_type  produce;

    // bundle_item
    //   constructor: an empty item (no name, no extension, no producer).
    bundle_item()
        : name(),
          ext(),
          produce()
    {}

    // bundle_item
    //   constructor: an item named _name with extension _ext, rendered by
    // _produce.
    bundle_item(
        std::string    _name,
        std::string    _ext,
        producer_type  _produce
    )
        : name(static_cast<std::string&&>(_name)),
          ext(static_cast<std::string&&>(_ext)),
          produce(static_cast<producer_type&&>(_produce))
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                II.  DOCUMENT BUNDLE                                      ///
///////////////////////////////////////////////////////////////////////////////

// document_bundle
//   class: an ordered, fluently-assembled collection of bundle_items.  The
// bundle carries only logical names and deferred producers; packaging, naming,
// and the destination are decided at write() time by an output_config and a
// sink, so one bundle can be written several ways.
class document_bundle
{
public:
    // -- public type aliases -------------------------------------------------

    // item_type
    //   type: one collected document.
    using item_type = bundle_item;

    // producer_type
    //   type: a document's deferred render (passed through from bundle_item).
    using producer_type = bundle_item::producer_type;

    // item_list
    //   type: the ordered item storage.
    using item_list = std::vector<bundle_item>;

    // size_type
    //   type: the item-count type.
    using size_type = std::size_t;

    // document_bundle
    //   constructor: an empty bundle (no documents).
    document_bundle() = default;

    // add
    //   appends a document named _name with extension _ext, rendered by
    // _produce.  Returns *this so a whole bundle can be assembled in one fluent
    // statement.
    document_bundle&
    add(
        std::string    _name,
        std::string    _ext,
        producer_type  _produce
    )
    {
        m_items.emplace_back(static_cast<std::string&&>(_name),
                             static_cast<std::string&&>(_ext),
                             static_cast<producer_type&&>(_produce));

        return *this;
    }

    // add
    //   appends an already-built item.  Returns *this for fluent assembly.
    document_bundle&
    add(
        bundle_item  _item
    )
    {
        m_items.push_back(static_cast<bundle_item&&>(_item));

        return *this;
    }

    // items
    //   the collected documents, in insertion order.
    D_NODISCARD const item_list&
    items() const D_NOEXCEPT
    {
        return m_items;
    }

    // size
    //   the number of documents in the bundle.
    D_NODISCARD size_type
    size() const D_NOEXCEPT
    {
        return m_items.size();
    }

    // empty
    //   true iff no document has been added.
    D_NODISCARD bool
    empty() const D_NOEXCEPT
    {
        return m_items.empty();
    }

private:
    item_list m_items;
};


///////////////////////////////////////////////////////////////////////////////
///                III. WRITE ORCHESTRATOR                                   ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL


// dispatch_compress
//   helper: compress _in into _out with the runtime codec _codec through
// compression.hpp's dispatch leaf.  Returns the facade status.
D_NODISCARD inline status
dispatch_compress(
    codec_id                  _codec,
    const byte_buffer&        _in,
    const compress_options&   _opt,
    byte_buffer&              _out
)
{
    return internal::compress_buffer(_codec,
                                     _in.data(),
                                     _in.size(),
                                     _opt,
                                     _out);
}


// dispatch_archive
//   helper: build a container of _entries in the runtime format _format into
// _out through archive.hpp's dispatch leaf.  An empty entry list passes a null
// base with count 0 (an empty vector must not be indexed).  Returns the facade
// status.
D_NODISCARD inline status
dispatch_archive(
    format_id                 _format,
    const entry_list&         _entries,
    const archive_options&    _opt,
    byte_buffer&              _out
)
{
    const entry* _first =
        _entries.empty() ? static_cast<const entry*>(0) : &_entries[0];

    return internal::archive_create(_format,
                                    _first,
                                    _entries.size(),
                                    _opt,
                                    _out);
}


// resolve_name
//   helper: the filename for item _index of _total under _cfg's naming policy,
// falling back to default_base_name when the policy is unset.
D_NODISCARD inline std::string
resolve_name(
    const output_config&  _cfg,
    const std::string&    _name,
    const std::string&    _ext,
    std::size_t           _index,
    std::size_t           _total
)
{
    if (!_cfg.naming)
    {
        return default_base_name(_name, _ext, _index, _total);
    }

    return _cfg.naming(_name, _ext, _index, _total);
}


NS_END  // internal


// write
//   function: render _bundle, package it per _cfg, and write the result to
// _sink.  Returns true iff every document rendered, packaged, and wrote
// successfully; false on the first sink rejection or facade error.  An empty
// bundle is a successful no-op.
//
//   The three modes:
//     none      - for each item: render, name, sink.write_item(name, bytes).
//     compress  - as none, but each item's bytes are run through cfg.codec and
//                 cfg's codec suffix is appended to its name.
//     archive   - each item becomes an entry (named by the policy) of one
//                 container built in cfg.format; the sink receives a single
//                 write of (archive_name + format extension, container bytes).
//
//   A producer is invoked exactly once.  write() raises nothing itself; a
// producer that throws propagates.
D_NODISCARD inline bool
write(
    const document_bundle&  _bundle,
    const output_config&    _cfg,
    output_sink&            _sink
)
{
    const document_bundle::item_list& _items = _bundle.items();
    const std::size_t                 _total = _items.size();
    std::size_t                       _i     = 0;

    // an empty bundle writes nothing and succeeds
    if (_total == std::size_t(0))
    {
        return true;
    }

    // ---- archive: every document is an entry of one container --------------
    if (_cfg.pack == pack_mode::archive)
    {
        entry_list  _entries;
        byte_buffer _blob;
        status      _s = status_ok;

        _entries.reserve(_total);

        for (_i = 0; _i < _total; ++_i)
        {
            entry _e;
            _e.name = internal::resolve_name(_cfg,
                                             _items[_i].name,
                                             _items[_i].ext,
                                             _i,
                                             _total);
            _e.data = _items[_i].produce();   // deferred render (once)

            _entries.push_back(static_cast<entry&&>(_e));
        }

        _s = internal::dispatch_archive(_cfg.format,
                                        _entries,
                                        _cfg.archive_opts,
                                        _blob);

        // a backend / availability failure aborts the run's write
        if (_s != status_ok)
        {
            return false;
        }

        // the lone container is named from cfg, extension by its format
        return _sink.write_item(_cfg.archive_name
                                    + format_extension(_cfg.format),
                                _blob);
    }

    // ---- none / compress: each document is an independent write -----------
    for (_i = 0; _i < _total; ++_i)
    {
        std::string _name = internal::resolve_name(_cfg,
                                                   _items[_i].name,
                                                   _items[_i].ext,
                                                   _i,
                                                   _total);
        byte_buffer _bytes = _items[_i].produce();   // deferred render (once)

        if (_cfg.pack == pack_mode::compress)
        {
            byte_buffer _packed;
            status      _s = internal::dispatch_compress(_cfg.codec,
                                                         _bytes,
                                                         _cfg.compress_opts,
                                                         _packed);

            // a backend / availability failure aborts the run's write
            if (_s != status_ok)
            {
                return false;
            }

            _name += codec_suffix(_cfg.codec);
            _bytes = static_cast<byte_buffer&&>(_packed);
        }

        // a sink rejection aborts the run's write
        if (!_sink.write_item(_name, _bytes))
        {
            return false;
        }
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  CONVENIENCE WRITES                                   ///
///////////////////////////////////////////////////////////////////////////////

// write_to_disk
//   function: write _bundle per _cfg to disk, naming each file through _path
// (filename -> destination path).  Builds a disk_output_sink and forwards to
// write(); returns its result.
D_NODISCARD inline bool
write_to_disk(
    const document_bundle&          _bundle,
    const output_config&            _cfg,
    disk_output_sink::path_fn       _path
)
{
    disk_output_sink _sink(static_cast<disk_output_sink::path_fn&&>(_path));

    return write(_bundle, _cfg, _sink);
}


// write_to_buffer
//   function: write _bundle per _cfg into the in-memory buffer _out, with
// _separator placed between documents (relevant only in `none` / `compress`
// mode; `archive` mode yields a single container).  Builds a buffer_output_sink
// and forwards to write(); returns its result.
D_NODISCARD inline bool
write_to_buffer(
    const document_bundle&  _bundle,
    const output_config&    _cfg,
    byte_buffer&            _out,
    const std::string&      _separator = std::string()
)
{
    buffer_output_sink _sink(_out, _separator);

    return write(_bundle, _cfg, _sink);
}


// write_custom
//   function: write _bundle per _cfg to a caller-supplied _sink.  A thin alias
// over write() for symmetry with the disk / buffer helpers when the destination
// is a custom output_sink subclass.
D_NODISCARD inline bool
write_custom(
    const document_bundle&  _bundle,
    const output_config&    _cfg,
    output_sink&            _sink
)
{
    return write(_bundle, _cfg, _sink);
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_CORE_DOCUMENT_BUNDLE_
