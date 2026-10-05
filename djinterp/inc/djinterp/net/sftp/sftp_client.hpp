/*******************************************************************************
* djinterp [net]                                                 sftp_client.hpp
*
* The C++ interface to the SFTP client.
*   A zero-cost view of sftp_client.h: `client` holds a d_sftp_client, and
* each member forwards to one C function, inline. Callbacks are templates,
* so any functor or, from C++11, any lambda serves as a sink, a source, or a
* directory walker with no virtual call and no allocation.
*   The interface is tiered. C++98 is the floor: this header needs only the
* C headers there. From C++11 the error is an enum class, copying is deleted
* rather than hidden, and the members are noexcept; from C++14, where
* re_std's expected is valid, every operation that produces a value also
* returns it as re_std::expected, beside the form that writes it through a
* reference. The spelling is the same in both tiers:
* sftp::error::no_such_file names the same value either way.
*   Errors are values, never exceptions. A callback must not throw; one that
* does terminates the program rather than unwind through the C client.
*   Below C++17, the SSH headers this one includes compile only once the
* framework root's D_NODISCARD stops using [[nodiscard]] where the language
* has no attribute syntax; the fix is with its owner.
*
*
* path:      /inc/djinterp/net/sftp/sftp_client.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LANGUAGE TIER
    -------------
    1.  Qualifiers
         1.  D_SFTP_NOEXCEPT
2.  VOCABULARY
    ----------
    1.  Errors
         1.  error
         2.  Conversions
    2.  Records
         1.  attributes
         2.  handle
         3.  name
    3.  Constants
         1.  Open flags
         2.  Attribute flags
3.  CLIENT
    ------
    1.  Callbacks
         1.  internal::string_sink
         2.  internal::string_source
         3.  internal::context_of
         4.  internal::sink_call
         5.  internal::source_call
         6.  internal::entry_call
    2.  Sessions
         1.  client
*/

#ifndef DJINTERP_NET_SFTP_SFTP_CLIENT_HPP
#define DJINTERP_NET_SFTP_SFTP_CLIENT_HPP 1

// std
#include <cstddef>   // std::size_t
#include <cstring>   // std::memcpy
#include <stdint.h>  // uint32_t, uint64_t
#include <string>    // std::string
// djinterp
#include "../../c/djinterp.h"    // framework root
#include "../ssh/ssh_common.h"   // d_ssh_status
#include "../ssh/ssh_session.h"  // d_ssh_session
#include "./sftp_client.h"       // the C client
#include "./sftp_common.h"       // d_sftp_error, records, flags

// the value-returning forms, from C++14: re_std's expected.hpp is not valid
// C++11 on every compiler, its expected<void, E> declaring constexpr
// functions that return void
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #include <utility>                             // std::move
    #include "../../../re_std/expected/expected.hpp"  // re_std::expected
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


//==============================================================================
// 1.  LANGUAGE TIER
//==============================================================================
// The one spelling that differs between the tiers.


// 1.1    Qualifiers
//------------------------------------------------------------------------------
// 1.1.1
// D_SFTP_NOEXCEPT
//   qualifier: `noexcept` from C++11, and `throw()` below it, whose check
// table-driven unwinding makes free on the path that does not throw. Either
// way, an exception reaching it terminates the program instead of unwinding
// through the C client. Pre-definable.
#ifndef D_SFTP_NOEXCEPT
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_SFTP_NOEXCEPT noexcept
    #else
        #define D_SFTP_NOEXCEPT throw()
    #endif
#endif  // D_SFTP_NOEXCEPT


namespace djinterp {
namespace net {
namespace sftp {


//==============================================================================
// 2.  VOCABULARY
//==============================================================================


// 2.1    Errors
//------------------------------------------------------------------------------
// 2.1.1
// error
//   enum: why an operation failed; `none` is success. An enum class from
// C++11, and below it a class scoping a plain enumeration, which converts to
// it for `switch` and comparison. The values are d_sftp_error's, whose
// documentation in sftp_common.h describes each.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

enum class error : int
{
    none              = D_SFTP_OK,
    eof               = D_SFTP_ERROR_EOF,
    no_such_file      = D_SFTP_ERROR_NO_SUCH_FILE,
    permission_denied = D_SFTP_ERROR_PERMISSION_DENIED,
    failure           = D_SFTP_ERROR_FAILURE,
    bad_message       = D_SFTP_ERROR_BAD_MESSAGE,
    no_connection     = D_SFTP_ERROR_NO_CONNECTION,
    connection_lost   = D_SFTP_ERROR_CONNECTION_LOST,
    unsupported       = D_SFTP_ERROR_UNSUPPORTED,
    invalid_argument  = D_SFTP_ERROR_INVALID_ARGUMENT,
    memory            = D_SFTP_ERROR_MEMORY,
    protocol          = D_SFTP_ERROR_PROTOCOL,
    version           = D_SFTP_ERROR_VERSION,
    transport         = D_SFTP_ERROR_TRANSPORT,
    bad_sequence      = D_SFTP_ERROR_BAD_SEQUENCE,
    too_large         = D_SFTP_ERROR_TOO_LARGE
};

#else

class error
{
public:
    enum value
    {
        none              = D_SFTP_OK,
        eof               = D_SFTP_ERROR_EOF,
        no_such_file      = D_SFTP_ERROR_NO_SUCH_FILE,
        permission_denied = D_SFTP_ERROR_PERMISSION_DENIED,
        failure           = D_SFTP_ERROR_FAILURE,
        bad_message       = D_SFTP_ERROR_BAD_MESSAGE,
        no_connection     = D_SFTP_ERROR_NO_CONNECTION,
        connection_lost   = D_SFTP_ERROR_CONNECTION_LOST,
        unsupported       = D_SFTP_ERROR_UNSUPPORTED,
        invalid_argument  = D_SFTP_ERROR_INVALID_ARGUMENT,
        memory            = D_SFTP_ERROR_MEMORY,
        protocol          = D_SFTP_ERROR_PROTOCOL,
        version           = D_SFTP_ERROR_VERSION,
        transport         = D_SFTP_ERROR_TRANSPORT,
        bad_sequence      = D_SFTP_ERROR_BAD_SEQUENCE,
        too_large         = D_SFTP_ERROR_TOO_LARGE
    };

    error()
        : m_value(none)
    {}

    error(
        value _value
    )
        : m_value(_value)
    {}

    explicit error(
        ::d_sftp_error _value
    )
        : m_value(static_cast<value>(_value))
    {}

    // value -- the enumerator, for switch and comparison
    operator value() const
    {
        return m_value;
    }

private:
    value m_value;
};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 2.1.2
// Conversions
//   to_c and from_c cross to and from d_sftp_error; error_string describes
// an error in a short static English phrase.
inline ::d_sftp_error
to_c(
    error _error
) D_SFTP_NOEXCEPT
{
    return static_cast< ::d_sftp_error>(static_cast<int>(_error));
}

inline error
from_c(
    ::d_sftp_error _error
) D_SFTP_NOEXCEPT
{
    return error(_error);
}

inline const char*
error_string(
    error _error
) D_SFTP_NOEXCEPT
{
    return ::d_sftp_error_string(to_c(_error));
}

// 2.2    Records
//------------------------------------------------------------------------------
// The C records themselves: plain data, with nothing a wrapper would add.

// 2.2.1
// attributes
//   type: a file's attributes; `flags` says which fields are present.
typedef ::d_sftp_attributes attributes;

// 2.2.2
// handle
//   type: an open file or directory, as the server names it.
typedef ::d_sftp_handle handle;

// 2.2.3
// name
//   type: a directory entry: its name, a display line, and its attributes;
// its text is borrowed, valid only during the callback that receives it.
typedef ::d_sftp_name name;

// 2.3    Constants
//------------------------------------------------------------------------------
// 2.3.1
// Open flags
//   constants: the pflags of open_file, combined with `|`.
const uint32_t OPEN_READ      = D_SFTP_OPEN_READ;
const uint32_t OPEN_WRITE     = D_SFTP_OPEN_WRITE;
const uint32_t OPEN_APPEND    = D_SFTP_OPEN_APPEND;
const uint32_t OPEN_CREATE    = D_SFTP_OPEN_CREATE;
const uint32_t OPEN_TRUNCATE  = D_SFTP_OPEN_TRUNCATE;
const uint32_t OPEN_EXCLUSIVE = D_SFTP_OPEN_EXCLUSIVE;

// 2.3.2
// Attribute flags
//   constants: the fields present in an attributes record's `flags`.
const uint32_t ATTR_SIZE        = D_SFTP_ATTR_SIZE;
const uint32_t ATTR_UIDGID      = D_SFTP_ATTR_UIDGID;
const uint32_t ATTR_PERMISSIONS = D_SFTP_ATTR_PERMISSIONS;
const uint32_t ATTR_ACMODTIME   = D_SFTP_ATTR_ACMODTIME;


//==============================================================================
// 3.  CLIENT
//==============================================================================


// 3.1    Callbacks
//------------------------------------------------------------------------------
// A sink is called as `error sink(const char* data, std::size_t size)` for
// each piece of a download, in file order; a source as `error source(char*
// buffer, std::size_t capacity, std::size_t& size)`, setting `size` to the
// bytes written and 0 at the end; a directory walker as `error walk(const
// sftp::name& entry)`. Each returns error::none to go on. Every member
// taking one has a `const` overload too, so a callback written in place
// binds as well as a named one.
namespace internal {

    // 3.1.1
    // internal::string_sink
    //   class: a sink that appends to a string.
    class string_sink
    {
    public:
        explicit string_sink(
            std::string& _out
        ) D_SFTP_NOEXCEPT
            : m_out(&_out)
        {}

        // operator() -- appends the data
        error
        operator()(
            const char* _data,
            std::size_t _size
        )
        {
            m_out->append(_data,
                          _size);

            return error::none;
        }

    private:
        std::string* m_out;
    };

    // 3.1.2
    // internal::string_source
    //   class: a source that supplies a string's bytes, in order.
    class string_source
    {
    public:
        explicit string_source(
            const std::string& _data
        ) D_SFTP_NOEXCEPT
            : m_data(&_data),
              m_offset(0u)
        {}

        // operator() -- supplies the next bytes, and 0 at the end
        error
        operator()(
            char*        _buffer,
            std::size_t  _capacity,
            std::size_t& _size
        ) D_SFTP_NOEXCEPT
        {
            const std::size_t left = m_data->size() - m_offset;

            _size = (left < _capacity) ? left : _capacity;

            // a round with bytes left copies them
            if (_size > 0u)
            {
                std::memcpy(_buffer,
                            m_data->data() + m_offset,
                            _size);
            }

            m_offset += _size;

            return error::none;
        }

    private:
        const std::string* m_data;
        std::size_t        m_offset;
    };

    // 3.1.3
    // internal::context_of
    //   function: a callback's address as the C client's context, which is
    // never written through; const is restored by the trampoline's type.
    template<typename Callback>
    void*
    context_of(
        Callback& _callback
    ) D_SFTP_NOEXCEPT
    {
        return const_cast<void*>(static_cast<const void*>(&_callback));
    }

    // 3.1.4
    // internal::sink_call
    //   function: the d_sftp_sink_fn for a Sink, whose address is the
    // context; Sink may be const-qualified.
    template<typename Sink>
    ::d_sftp_error
    sink_call(
        void*       _context,
        const void* _data,
        std::size_t _size
    ) D_SFTP_NOEXCEPT
    {
        Sink& sink = *static_cast<Sink*>(_context);

        return to_c(sink(static_cast<const char*>(_data),
                         _size));
    }

    // 3.1.5
    // internal::source_call
    //   function: the d_sftp_source_fn for a Source, whose address is the
    // context; Source may be const-qualified.
    template<typename Source>
    ::d_sftp_error
    source_call(
        void*        _context,
        void*        _buffer,
        std::size_t  _capacity,
        std::size_t* _out_size
    ) D_SFTP_NOEXCEPT
    {
        Source& source = *static_cast<Source*>(_context);

        return to_c(source(static_cast<char*>(_buffer),
                           _capacity,
                           *_out_size));
    }

    // 3.1.6
    // internal::entry_call
    //   function: the d_sftp_entry_fn for a Walker, whose address is the
    // context; Walker may be const-qualified.
    template<typename Walker>
    ::d_sftp_error
    entry_call(
        void*                 _context,
        const ::d_sftp_name*  _entry
    ) D_SFTP_NOEXCEPT
    {
        Walker& walker = *static_cast<Walker*>(_context);

        return to_c(walker(*_entry));
    }

}  // namespace internal

// 3.2    Sessions
//------------------------------------------------------------------------------
// 3.2.1
// client
//   class: an SFTP session over an SSH session, or over any byte stream a
// d_sftp_transport describes. Every member blocks, as the C client does,
// and reports its outcome as an error; the destructor closes the session.
// The C client holds pointers into itself, so a client can be neither
// copied nor moved: hold it where it is made, or on the heap.
class client
{
public:
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    client(const client&)            = delete;
    client& operator=(const client&) = delete;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    client() D_SFTP_NOEXCEPT
    {
        ::d_sftp_client_init(&m_client);
    }

    ~client()
    {
        ::d_sftp_client_close(&m_client);
    }

    // open -- starts SFTP on an authenticated SSH session, which must
    // outlive the client
    error
    open(
        ::d_ssh_session& _session
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_open(&m_client,
                                           &_session));
    }

    // open -- starts SFTP over a stream already carrying it
    error
    open(
        const ::d_sftp_transport& _transport
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_open_transport(&m_client,
                                                     _transport));
    }

    // close -- ends the session, closing a channel it opened
    void
    close() D_SFTP_NOEXCEPT
    {
        ::d_sftp_client_close(&m_client);

        return;
    }

    // realpath -- the canonical absolute form of a path
    error
    realpath(
        const char*  _path,
        std::string& _out
    )
    {
        return text_of(&::d_sftp_client_realpath,
                       _path,
                       _out);
    }

    // stat -- a path's attributes, following a final link
    error
    stat(
        const char* _path,
        attributes& _out
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_stat(&m_client,
                                           _path,
                                           &_out));
    }

    // lstat -- a path's attributes, not following a final link
    error
    lstat(
        const char* _path,
        attributes& _out
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_lstat(&m_client,
                                            _path,
                                            &_out));
    }

    // setstat -- sets the attributes whose flags are present
    error
    setstat(
        const char*       _path,
        const attributes& _attributes
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_setstat(&m_client,
                                              _path,
                                              &_attributes));
    }

    // remove -- deletes a file
    error
    remove(
        const char* _path
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_remove(&m_client,
                                             _path));
    }

    // rename -- moves a file; _replace overwrites an existing target where
    // the server offers posix-rename@openssh.com
    error
    rename(
        const char* _from,
        const char* _to,
        bool        _replace = false
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_rename(&m_client,
                                             _from,
                                             _to,
                                             _replace));
    }

    // readlink -- a symbolic link's target
    error
    readlink(
        const char*  _path,
        std::string& _out
    )
    {
        return text_of(&::d_sftp_client_readlink,
                       _path,
                       _out);
    }

    // symlink -- makes _link a symbolic link to _target
    error
    symlink(
        const char* _target,
        const char* _link
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_symlink(&m_client,
                                              _target,
                                              _link));
    }

    // mkdir -- makes a directory, with the attributes given, if any
    error
    mkdir(
        const char*       _path,
        const attributes* _attributes = NULL
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_mkdir(&m_client,
                                            _path,
                                            _attributes));
    }

    // rmdir -- removes an empty directory
    error
    rmdir(
        const char* _path
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_rmdir(&m_client,
                                            _path));
    }

    // list -- calls a walker for each entry of a directory, "." and ".."
    // included, as the server sends them
    template<typename Walker>
    error
    list(
        const char* _path,
        Walker&     _walker
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_list(&m_client,
                                           _path,
                                           &internal::entry_call<Walker>,
                                           internal::context_of(_walker)));
    }

    template<typename Walker>
    error
    list(
        const char*   _path,
        const Walker& _walker
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_list(
                          &m_client,
                          _path,
                          &internal::entry_call<const Walker>,
                          internal::context_of(_walker)));
    }

    // open_file -- opens a file with OPEN_* flags; _attributes, if any,
    // apply to a file OPEN_CREATE makes
    error
    open_file(
        const char*       _path,
        uint32_t          _flags,
        const attributes* _attributes,
        handle&           _out
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_open_file(&m_client,
                                                _path,
                                                _flags,
                                                _attributes,
                                                &_out));
    }

    // close_file -- closes an open file
    error
    close_file(
        const handle& _handle
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_close_file(&m_client,
                                                 &_handle));
    }

    // read -- reads up to _capacity bytes at an offset; error::eof past
    // the end
    error
    read(
        const handle& _handle,
        uint64_t      _offset,
        void*         _buffer,
        std::size_t   _capacity,
        std::size_t&  _out_read
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_read(&m_client,
                                           &_handle,
                                           _offset,
                                           _buffer,
                                           _capacity,
                                           &_out_read));
    }

    // write -- writes all of the data at an offset, pipelined
    error
    write(
        const handle& _handle,
        uint64_t      _offset,
        const void*   _data,
        std::size_t   _size
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_write(&m_client,
                                            &_handle,
                                            _offset,
                                            _data,
                                            _size));
    }

    // fstat -- an open file's attributes
    error
    fstat(
        const handle& _handle,
        attributes&   _out
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_fstat(&m_client,
                                            &_handle,
                                            &_out));
    }

    // fsync -- flushes an open file to stable storage, where the server
    // offers fsync@openssh.com
    error
    fsync(
        const handle& _handle
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_fsync(&m_client,
                                            &_handle));
    }

    // get -- downloads a whole file into a sink, pipelined
    template<typename Sink>
    error
    get(
        const char* _path,
        Sink&       _sink
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_get(&m_client,
                                          _path,
                                          &internal::sink_call<Sink>,
                                          internal::context_of(_sink)));
    }

    template<typename Sink>
    error
    get(
        const char* _path,
        const Sink& _sink
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_get(&m_client,
                                          _path,
                                          &internal::sink_call<const Sink>,
                                          internal::context_of(_sink)));
    }

    // get -- downloads a whole file onto the end of a string
    error
    get(
        const char*  _path,
        std::string& _out
    ) D_SFTP_NOEXCEPT
    {
        internal::string_sink sink(_out);

        return get(_path,
                   sink);
    }

    // put -- uploads a whole file from a source, pipelined, creating or
    // truncating it; _attributes, if any, apply to a file it creates
    template<typename Source>
    error
    put(
        const char*       _path,
        Source&           _source,
        const attributes* _attributes = NULL
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_put(&m_client,
                                          _path,
                                          _attributes,
                                          &internal::source_call<Source>,
                                          internal::context_of(_source)));
    }

    template<typename Source>
    error
    put(
        const char*       _path,
        const Source&     _source,
        const attributes* _attributes = NULL
    ) D_SFTP_NOEXCEPT
    {
        return from_c(::d_sftp_client_put(
                          &m_client,
                          _path,
                          _attributes,
                          &internal::source_call<const Source>,
                          internal::context_of(_source)));
    }

    // put -- uploads a string's bytes as a whole file
    error
    put(
        const char*        _path,
        const std::string& _data,
        const attributes*  _attributes = NULL
    ) D_SFTP_NOEXCEPT
    {
        internal::string_source source(_data);

        return put(_path,
                   source,
                   _attributes);
    }

    // a string that is not const binds here: to the template above it would
    // bind more tightly, as a callback
    error
    put(
        const char*       _path,
        std::string&      _data,
        const attributes* _attributes = NULL
    ) D_SFTP_NOEXCEPT
    {
        return put(_path,
                   static_cast<const std::string&>(_data),
                   _attributes);
    }

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    // value-returning forms -- each returns what the reference form writes,
    // or the error
    re_std::expected<std::string, error>
    realpath(
        const char* _path
    )
    {
        std::string out;

        return value_or_error(realpath(_path,
                                       out),
                              out);
    }

    re_std::expected<attributes, error>
    stat(
        const char* _path
    ) noexcept
    {
        attributes out = attributes();

        return value_or_error(stat(_path,
                                   out),
                              out);
    }

    re_std::expected<attributes, error>
    lstat(
        const char* _path
    ) noexcept
    {
        attributes out = attributes();

        return value_or_error(lstat(_path,
                                    out),
                              out);
    }

    re_std::expected<std::string, error>
    readlink(
        const char* _path
    )
    {
        std::string out;

        return value_or_error(readlink(_path,
                                       out),
                              out);
    }

    re_std::expected<handle, error>
    open_file(
        const char*       _path,
        uint32_t          _flags,
        const attributes* _attributes = nullptr
    ) noexcept
    {
        handle out = handle();

        return value_or_error(open_file(_path,
                                        _flags,
                                        _attributes,
                                        out),
                              out);
    }

    re_std::expected<std::size_t, error>
    read(
        const handle& _handle,
        uint64_t      _offset,
        void*         _buffer,
        std::size_t   _capacity
    ) noexcept
    {
        std::size_t count = 0u;

        return value_or_error(read(_handle,
                                   _offset,
                                   _buffer,
                                   _capacity,
                                   count),
                              count);
    }

    re_std::expected<attributes, error>
    fstat(
        const handle& _handle
    ) noexcept
    {
        attributes out = attributes();

        return value_or_error(fstat(_handle,
                                    out),
                              out);
    }

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

    // state -- O(1) reads of the session
    bool
    is_open() const D_SFTP_NOEXCEPT
    {
        return m_client.open;
    }

    uint32_t
    version() const D_SFTP_NOEXCEPT
    {
        return m_client.version;
    }

    uint32_t
    extensions() const D_SFTP_NOEXCEPT
    {
        return m_client.extensions;
    }

    uint32_t
    status_code() const D_SFTP_NOEXCEPT
    {
        return m_client.status_code;
    }

    const char*
    status_message() const D_SFTP_NOEXCEPT
    {
        return m_client.status_message;
    }

    ::d_ssh_status
    ssh_status() const D_SFTP_NOEXCEPT
    {
        return m_client.ssh_status;
    }

    // c_client -- the d_sftp_client beneath, for what has no member here
    ::d_sftp_client&
    c_client() D_SFTP_NOEXCEPT
    {
        return m_client;
    }

    const ::d_sftp_client&
    c_client() const D_SFTP_NOEXCEPT
    {
        return m_client;
    }

private:
#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    client(const client&);
    client& operator=(const client&);
#endif  // !D_ENV_LANG_IS_CPP11_OR_HIGHER

    // text_fn -- the C form of realpath and readlink
    typedef ::d_sftp_error (*text_fn)(::d_sftp_client*,
                                      const char*,
                                      char*,
                                      size_t);

    // text_of -- runs a text-producing call into a string, doubling the
    // room from 4 KiB while the answer does not fit, up to 1 MiB
    error
    text_of(
        text_fn      _call,
        const char*  _path,
        std::string& _out
    )
    {
        std::string    room(4096u,
                            '\0');
        ::d_sftp_error result = _call(&m_client,
                                      _path,
                                      &room[0],
                                      room.size());

        // a longer answer gets a larger room
        while ( (result == D_SFTP_ERROR_TOO_LARGE) &&
                (room.size() < 1048576u) )
        {
            room.resize(room.size() * 2u);
            result = _call(&m_client,
                           _path,
                           &room[0],
                           room.size());
        }

        // the answer, up to its terminator
        if (result == D_SFTP_OK)
        {
            _out.assign(room.c_str());
        }

        return from_c(result);
    }

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    // value_or_error -- a value on success, the error otherwise
    template<typename Value>
    static re_std::expected<Value, error>
    value_or_error(
        error  _error,
        Value& _value
    )
    {
        // a failure carries only its error
        if (_error != error::none)
        {
            return re_std::unexpected<error>(_error);
        }

        return re_std::expected<Value, error>(std::move(_value));
    }

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

    ::d_sftp_client m_client;
};


}  // namespace sftp
}  // namespace net
}  // namespace djinterp


#endif  // DJINTERP_NET_SFTP_SFTP_CLIENT_HPP
