/*******************************************************************************
* djinterp [cpp/parse]                                            arg_config.hpp
*
* The argument parser's C++ face: configuration formats.
*   `cli::config_format` is CLI11's ConfigBase -- its ConfigTOML -- and
* `cli::DConfigIni` its ConfigINI: a struct dj_arg_config_format (the
* characters, section and index of arg_config.h) whose three operations are
* CLI11's virtual Config functions: reading text into items (from_config),
* writing a command's values (to_config), and reading an item as a flag
* (to_flag). It derives from the C struct, so that it is given to a command
* (DCommandInterface::config_formatter) as the C format it is, and its
* table calls the virtual functions, so that a class deriving from it reads
* or writes its own way wherever the parser does. Each default is the C
* library's.
*   As help's steps do, an operation appends to a cli::buffer and returns a
* cli::status, and reads into a view of the C item list, appending items it
* builds as DConfigItem; what it throws ends the operation as a conversion
* error, or as out_of_memory for std::bad_alloc. Reading and writing text
* exist only with D_INTERNAL_ARG_PARSER_CONFIG_FILES; without it their
* defaults answer status_code::unsupported, and the class is otherwise the
* same.
*   A command borrows its format, which must outlive it.
*
*
* path:      /inc/djinterp/cpp/modules/parse/cli/arg_parser/arg_config.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  ITEMS
    -----
    1.  Reading
         1.  DConfigEntry
    2.  Building
         1.  DConfigItem
    3.  Lists
         1.  DConfigItems
         2.  DConfigItemList
2.  FORMATS
    -------
    1.  The format
         1.  config_format
         2.  DConfigToml
    2.  INI
         1.  DConfigIni
3.  INTERNALS
    ---------
    1.  Operations
         1.  DFromConfigCall / DToConfigCall / DToFlagCall
         2.  config_table
         3.  The operation functions
4.  DEFINITIONS
    -----------
    1.  DConfigEntry
    2.  DConfigItem
    3.  DConfigItems
    4.  DConfigItemList
    5.  config_format
    6.  DConfigIni
    7.  Internals
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_CONFIG_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_CONFIG_HPP 1

// std
#include <cstddef>    // std::size_t
#include <string>     // std::string
// djinterp
#include "../../../../djinterp.hpp"          // framework root
#include "../../../../../c/util/sink_common.h"  // dj_pack_text
#include "../../../../../config/c/modules/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../../../../env/env.h"             // language levels
#include "./arg_command.hpp"              // command, raise_out_of_memory
#include "./arg_common.hpp"               // status, text, lists, buffer,
                                          // guard, scratch
#include "./arg_type.hpp"                 // conversion_failure
#include "../../../../../c/modules/parse/cli/arg_parser/arg_config.h"               // dj_arg_config_format,
                                          // dj_arg_config_item
// re_std
#include "../../../../../../re_std/c/cstdint/dstdint.h"  // uint32_t


namespace djinterp
{
namespace parse
{
namespace cli
{

//==============================================================================
// 1.  ITEMS
//==============================================================================


// 1.1    Reading
//------------------------------------------------------------------------------
// 1.1.1
// DConfigEntry
//   class: CLI11's ConfigItem, read: a view of an item a format read -- its
// sections, outermost first, its name and its values, as views valid while
// the item is -- or of none, which reads as an item with nothing in it.
class DConfigEntry
{
public:
    DConfigEntry() D_NOEXCEPT;
    explicit
    DConfigEntry(
        const ::dj_arg_config_item* _item
    ) D_NOEXCEPT;

    const ::dj_arg_config_item* get() const D_NOEXCEPT;
    bool valid() const D_NOEXCEPT;

    // the item, as CLI11's fields read it, and CLI11's fullname(): the
    // sections and the name joined by `.`
    DTextList parents() const D_NOEXCEPT;
    text name() const D_NOEXCEPT;
    DTextList inputs() const D_NOEXCEPT;
    bool multiline() const D_NOEXCEPT;
    std::string fullname() const;

private:
    const ::dj_arg_config_item* m_item;
};

// 1.2    Building
//------------------------------------------------------------------------------
// 1.2.1
// DConfigItem
//   class: an item a format builds as it reads -- CLI11's ConfigItem, made --
// owning its copies until a list adopts it (DConfigItems::adopt), which
// leaves it empty. Each setting copies its text and returns
// status_code::ok, or out_of_memory with the item as it was. Not copied.
class DConfigItem : public ::dj_arg_config_item
{
public:
    DConfigItem() D_NOEXCEPT;
    ~DConfigItem();

    status set_name(
        const text& _name
    ) D_NOEXCEPT;
    status add_parent(
        const text& _section
    ) D_NOEXCEPT;
    status add_input(
        const text& _value
    ) D_NOEXCEPT;
    void set_multiline(
        bool _value = true
    ) D_NOEXCEPT;

    // the item, read
    DConfigEntry entry() const D_NOEXCEPT;

private:
    D_DELETED_FN(DConfigItem(const DConfigItem& _other))
    D_DELETED_FN(DConfigItem& operator=(const DConfigItem& _other))
};

// 1.3    Lists
//------------------------------------------------------------------------------
// 1.3.1
// DConfigItems
//   class: a view of the list of items a read appends to, which owns them:
// what from_config is given. Valid while the list is.
class DConfigItems
{
public:
    explicit
    DConfigItems(
        ::dj_arg_config_item_list& _list
    ) D_NOEXCEPT;

    // reading
    std::size_t size() const D_NOEXCEPT;
    bool empty() const D_NOEXCEPT;
    DConfigEntry operator[](
        std::size_t _index
    ) const D_NOEXCEPT;

    // appending: the list takes everything the item holds, leaving it
    // empty; status_code::ok, or out_of_memory with both as they were
    status adopt(
        DConfigItem& _item
    ) D_NOEXCEPT;

    // the C list, for the C calls
    ::dj_arg_config_item_list& get() const D_NOEXCEPT;

private:
    ::dj_arg_config_item_list* m_list;
};

// 1.3.2
// DConfigItemList
//   class: a list of items owned by the caller -- CLI11's
// std::vector<ConfigItem> -- for reading text or a file directly
// (config_format::from_text and from_file); released with it. Not copied.
class DConfigItemList : public ::dj_arg_config_item_list
{
public:
    DConfigItemList() D_NOEXCEPT;
    ~DConfigItemList();

    std::size_t size() const D_NOEXCEPT;
    bool empty() const D_NOEXCEPT;
    DConfigEntry operator[](
        std::size_t _index
    ) const D_NOEXCEPT;
    DConfigItems items() D_NOEXCEPT;
    void clear() D_NOEXCEPT;

private:
    D_DELETED_FN(DConfigItemList(const DConfigItemList& _other))
    D_DELETED_FN(DConfigItemList& operator=(const DConfigItemList& _other))
};


//==============================================================================
// 2.  FORMATS
//==============================================================================


// 2.1    The format
//------------------------------------------------------------------------------
// 2.1.1
// config_format
//   class: CLI11's ConfigBase, as ConfigTOML makes it, over a
// dj_arg_config_format made by its constructor and released by its
// destructor, whose table calls the virtual operations with the object as
// its context. The settings are ConfigBase's, in this face's spelling, and
// return the format. A copy has the original's settings and its own table;
// copying throws std::bad_alloc, as CLI11's std::string section does, when
// the section cannot be copied. A member function hides the field of its
// name.
class config_format : public ::dj_arg_config_format
{
public:
    // making, copying and releasing
    config_format() D_NOEXCEPT;
    config_format(
        const config_format& _other
    );
    config_format& operator=(
        const config_format& _other
    );
    virtual
    ~config_format();

    // settings: CLI11's comment, arrayBounds, arraySeparator,
    // valueSeparator, quoteCharacter, maxLayers (0 to 255), parentSeparator,
    // commentDefaults and allowDuplicateFields; '\0' disables what a
    // character marks
    config_format& comment(
        char _character
    ) D_NOEXCEPT;
    config_format& array_bounds(
        char _start,
        char _end
    ) D_NOEXCEPT;
    config_format& array_separator(
        char _character
    ) D_NOEXCEPT;
    config_format& value_separator(
        char _character
    ) D_NOEXCEPT;
    config_format& quote_character(
        char _string_quote,
        char _literal_quote
    ) D_NOEXCEPT;
    config_format& max_layers(
        int _layers
    ) D_NOEXCEPT;
    config_format& parent_separator(
        char _character
    ) D_NOEXCEPT;
    config_format& comment_defaults(
        bool _value = true
    ) D_NOEXCEPT;
    config_format& allow_duplicate_fields(
        bool _value = true
    ) D_NOEXCEPT;

    // the characters, read, which the settings' names hide as fields
    char get_comment() const D_NOEXCEPT;
    char get_array_start() const D_NOEXCEPT;
    char get_array_end() const D_NOEXCEPT;
    char get_array_separator() const D_NOEXCEPT;
    char get_value_separator() const D_NOEXCEPT;
    char get_string_quote() const D_NOEXCEPT;
    char get_literal_quote() const D_NOEXCEPT;
    char get_parent_separator() const D_NOEXCEPT;

    // the section and index read: CLI11's section(name) and index(n); a
    // section that cannot be copied is not set, and std::bad_alloc is
    // thrown, as by CLI11's std::string; none -- or "" -- reads them all,
    // and an index of -1 every occurrence
    config_format& section(
        DCString _section
    );
    config_format& index(
        int _index
    ) D_NOEXCEPT;
    const char* get_section() const D_NOEXCEPT;
    int get_index() const D_NOEXCEPT;
    int get_max_layers() const D_NOEXCEPT;
    bool get_comment_defaults() const D_NOEXCEPT;
    bool get_allow_duplicate_fields() const D_NOEXCEPT;

    // the operations: CLI11's virtual from_config, to_config and to_flag,
    // each appending -- items to `_out`, text to `_out`, a failure's
    // description to `_error` -- and returning status_code::ok or the status
    // that stopped it. The defaults are the C library's (arg_config.h).
    virtual
    cli::status from_config(
        const text&   _input,
        DConfigItems& _out,
        buffer&       _error
    ) const;
    virtual
    cli::status to_config(
        const cli::command&       _command,
        config_output_mode::value _mode,
        bool                      _write_description,
        const text&               _prefix,
        buffer&                   _out
    ) const;
    virtual
    cli::status to_flag(
        const DConfigEntry& _item,
        buffer&             _out
    ) const;

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    // reading directly: CLI11's from_config(std::istream&) and
    // from_file(name), through this format's operations, the items
    // appended to `_out` and a failure's description to `_error`
    cli::status from_text(
        const text&      _input,
        DConfigItemList& _out,
        std::string&     _error
    ) const;
    cli::status from_file(
        DCString         _path,
        DConfigItemList& _out,
        std::string&     _error
    ) const;
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

protected:
    // the table, and this object as its context, given again after a C
    // call that makes the format afresh
    void own_table() D_NOEXCEPT;
};

// 2.1.2
// DConfigToml
//   type: CLI11's ConfigTOML, which config_format is.
typedef config_format DConfigToml;

// 2.2    INI
//------------------------------------------------------------------------------
// 2.2.1
// DConfigIni
//   class: CLI11's ConfigINI: config_format with INI's characters -- `;`
// comments, no array brackets, values separated by spaces.
class DConfigIni : public config_format
{
public:
    DConfigIni() D_NOEXCEPT;
};


//==============================================================================
// 3.  INTERNALS
//==============================================================================


// 3.1    Operations
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.1.1
    // DFromConfigCall / DToConfigCall / DToFlagCall
    //   classes: the work guard runs for an operation: the virtual function
    //   called on the format with the arguments given, its status as a
    //   dj_arg_status.
    class DFromConfigCall
    {
    public:
        typedef int result_type;

        DFromConfigCall(
            const config_format& _self,
            const text&          _input,
            DConfigItems&        _out,
            buffer&              _error
        ) D_NOEXCEPT;

        int operator()() const;

    private:
        const config_format& m_self;
        const text&          m_input;
        DConfigItems&        m_out;
        buffer&              m_error;
    };

    class DToConfigCall
    {
    public:
        typedef int result_type;

        DToConfigCall(
            const config_format&      _self,
            const cli::command&       _command,
            config_output_mode::value _mode,
            bool                      _write_description,
            const text&               _prefix,
            buffer&                   _out
        ) D_NOEXCEPT;

        int operator()() const;

    private:
        const config_format&      m_self;
        const cli::command&       m_command;
        config_output_mode::value m_mode;
        bool                      m_write_description;
        const text&               m_prefix;
        buffer&                   m_out;
    };

    class DToFlagCall
    {
    public:
        typedef int result_type;

        DToFlagCall(
            const config_format& _self,
            const DConfigEntry&  _item,
            buffer&              _out
        ) D_NOEXCEPT;

        int operator()() const;

    private:
        const config_format& m_self;
        const DConfigEntry&  m_item;
        buffer&              m_out;
    };

    // 3.1.2
    // config_table
    //   function: the table of operations every cli::config_format holds, one
    //   for the program, each entry the operation function of 3.1.3.
    const ::dj_arg_config_format_vtable& config_table() D_NOEXCEPT;

    // 3.1.3
    // The operation functions
    //   functions: the table's entries, with the C operations' parameters:
    //   each reads the format object from the C format's context, makes the
    //   views and handles the virtual operation takes, and calls it under
    //   guard, an exception answered as conversion_failure answers it.
    int operation_from_config(
        const ::dj_arg_config_format* _format,
        ::dj_pack_text                _input,
        ::dj_arg_config_item_list*    _out,
        ::dj_arg_buffer*              _error
    );
    int operation_to_config(
        const ::dj_arg_config_format* _format,
        const ::dj_arg_command*       _command,
        int                           _mode,
        bool                          _write_description,
        ::dj_pack_text                _prefix,
        ::dj_arg_buffer*              _out
    );
    int operation_to_flag(
        const ::dj_arg_config_format* _format,
        const ::dj_arg_config_item*   _item,
        ::dj_arg_buffer*              _out
    );

NS_END  // internal


//==============================================================================
// 4.  DEFINITIONS
//==============================================================================


// 4.1    DConfigEntry
//------------------------------------------------------------------------------
// DConfigEntry
//   The item's fields, read in place; a view of none reads empty.
inline
DConfigEntry::DConfigEntry() D_NOEXCEPT
    : m_item(D_NULLPTR)
{}

inline
DConfigEntry::DConfigEntry(
    const ::dj_arg_config_item* _item
) D_NOEXCEPT
    : m_item(_item)
{}

inline
const ::dj_arg_config_item* DConfigEntry::get() const D_NOEXCEPT
{
    return m_item;
}

inline
bool DConfigEntry::valid() const D_NOEXCEPT
{
    return (m_item != D_NULLPTR);
}

inline
DTextList DConfigEntry::parents() const D_NOEXCEPT
{
    return (m_item != D_NULLPTR)
           ? DTextList(m_item->parents.items,
                       m_item->parents.count)
           : DTextList();
}

inline
text DConfigEntry::name() const D_NOEXCEPT
{
    return (m_item != D_NULLPTR) ? text(m_item->name) : text();
}

inline
DTextList DConfigEntry::inputs() const D_NOEXCEPT
{
    return (m_item != D_NULLPTR)
           ? DTextList(m_item->inputs.items,
                       m_item->inputs.count)
           : DTextList();
}

inline
bool DConfigEntry::multiline() const D_NOEXCEPT
{
    return ( (m_item != D_NULLPTR) &&
             (m_item->multiline) );
}

inline
std::string DConfigEntry::fullname() const
{
    internal::scratch written;
    const int         result = d_arg_config_item_fullname(m_item,
                                                          written.get());

    internal::raise_out_of_memory(result);

    return (result == D_ARG_OK) ? written.str() : std::string();
}

// 4.2    DConfigItem
//------------------------------------------------------------------------------
// DConfigItem
//   An item made empty, as D_ARG_CONFIG_ITEM_INIT makes one; the C calls.
inline
DConfigItem::DConfigItem() D_NOEXCEPT
{
    const ::dj_arg_config_item empty = D_ARG_CONFIG_ITEM_INIT;

    *static_cast< ::dj_arg_config_item*>(this) = empty;
}

inline
DConfigItem::~DConfigItem()
{
    d_arg_config_item_release(this);
}

inline
status DConfigItem::set_name(
    const text& _name
) D_NOEXCEPT
{
    return status(d_arg_config_item_set_name(this,
                                             _name));
}

inline
status DConfigItem::add_parent(
    const text& _section
) D_NOEXCEPT
{
    return status(d_arg_config_item_add_parent(this,
                                               _section));
}

inline
status DConfigItem::add_input(
    const text& _value
) D_NOEXCEPT
{
    return status(d_arg_config_item_add_input(this,
                                              _value));
}

inline
void DConfigItem::set_multiline(
    bool _value
) D_NOEXCEPT
{
    this->dj_arg_config_item::multiline = _value;

    return;
}

inline
DConfigEntry DConfigItem::entry() const D_NOEXCEPT
{
    return DConfigEntry(this);
}

// 4.3    DConfigItems
//------------------------------------------------------------------------------
// DConfigItems
//   The list's array, read; d_arg_config_item_list_adopt.
inline
DConfigItems::DConfigItems(
    ::dj_arg_config_item_list& _list
) D_NOEXCEPT
    : m_list(&_list)
{}

inline
std::size_t DConfigItems::size() const D_NOEXCEPT
{
    return m_list->count;
}

inline
bool DConfigItems::empty() const D_NOEXCEPT
{
    return (m_list->count == 0u);
}

inline
DConfigEntry DConfigItems::operator[](
    std::size_t _index
) const D_NOEXCEPT
{
    return DConfigEntry(&m_list->items[_index]);
}

inline
status DConfigItems::adopt(
    DConfigItem& _item
) D_NOEXCEPT
{
    return status(d_arg_config_item_list_adopt(m_list,
                                               &_item));
}

inline
::dj_arg_config_item_list& DConfigItems::get() const D_NOEXCEPT
{
    return *m_list;
}

// 4.4    DConfigItemList
//------------------------------------------------------------------------------
// DConfigItemList
//   A list made empty; d_arg_config_item_list_release, which leaves it
// empty, for clear() and the destructor.
inline
DConfigItemList::DConfigItemList() D_NOEXCEPT
{
    this->dj_arg_config_item_list::items    = D_NULLPTR;
    this->dj_arg_config_item_list::count    = 0u;
    this->dj_arg_config_item_list::capacity = 0u;
}

inline
DConfigItemList::~DConfigItemList()
{
    d_arg_config_item_list_release(this);
}

inline
std::size_t DConfigItemList::size() const D_NOEXCEPT
{
    return this->dj_arg_config_item_list::count;
}

inline
bool DConfigItemList::empty() const D_NOEXCEPT
{
    return (this->dj_arg_config_item_list::count == 0u);
}

inline
DConfigEntry DConfigItemList::operator[](
    std::size_t _index
) const D_NOEXCEPT
{
    return DConfigEntry(&this->dj_arg_config_item_list::items[_index]);
}

inline
DConfigItems DConfigItemList::items() D_NOEXCEPT
{
    return DConfigItems(*this);
}

inline
void DConfigItemList::clear() D_NOEXCEPT
{
    d_arg_config_item_list_release(this);

    return;
}

// 4.5    config_format
//------------------------------------------------------------------------------
// config_format::config_format / operator= / ~config_format / own_table
//   d_arg_config_format_init over the base, which is plain data and so
// already there to be made, then the table and this object as its context;
// a copy's fields copied, its section through the setter, whose failure is
// std::bad_alloc; d_arg_config_format_release.
inline
config_format::config_format() D_NOEXCEPT
{
    d_arg_config_format_init(this);
    own_table();
}

inline
config_format::config_format(
    const config_format& _other
)
    : ::dj_arg_config_format()
{
    d_arg_config_format_init(this);
    own_table();
    (void)operator=(_other);
}

inline config_format&
config_format::operator=(
    const config_format& _other
)
{
    // a format given itself is left as it is
    if (this != &_other)
    {
        internal::raise_out_of_memory(
            d_arg_config_format_set_section(this,
                                            _other.get_section()));

        this->dj_arg_config_format::index            = _other.get_index();
        this->dj_arg_config_format::max_layers       = _other.get_max_layers();
        this->dj_arg_config_format::flags            =
            _other.dj_arg_config_format::flags;
        this->dj_arg_config_format::comment          =
            _other.dj_arg_config_format::comment;
        this->dj_arg_config_format::array_start      =
            _other.dj_arg_config_format::array_start;
        this->dj_arg_config_format::array_end        =
            _other.dj_arg_config_format::array_end;
        this->dj_arg_config_format::array_separator  =
            _other.dj_arg_config_format::array_separator;
        this->dj_arg_config_format::value_delimiter  =
            _other.dj_arg_config_format::value_delimiter;
        this->dj_arg_config_format::string_quote     =
            _other.dj_arg_config_format::string_quote;
        this->dj_arg_config_format::literal_quote    =
            _other.dj_arg_config_format::literal_quote;
        this->dj_arg_config_format::parent_separator =
            _other.dj_arg_config_format::parent_separator;
    }

    return *this;
}

inline
config_format::~config_format()
{
    d_arg_config_format_release(this);
}

inline
void config_format::own_table() D_NOEXCEPT
{
    this->dj_arg_config_format::vtable  = &internal::config_table();
    this->dj_arg_config_format::context = this;

    return;
}

// config_format::comment and kin
//   The characters written to their fields, which the format reads as it
// reads them; the layers, index and switches through the C settings.
inline
config_format& config_format::comment(
    char _character
) D_NOEXCEPT
{
    this->dj_arg_config_format::comment = _character;

    return *this;
}

inline
config_format& config_format::array_bounds(
    char _start,
    char _end
) D_NOEXCEPT
{
    this->dj_arg_config_format::array_start = _start;
    this->dj_arg_config_format::array_end   = _end;

    return *this;
}

inline
config_format& config_format::array_separator(
    char _character
) D_NOEXCEPT
{
    this->dj_arg_config_format::array_separator = _character;

    return *this;
}

inline
config_format& config_format::value_separator(
    char _character
) D_NOEXCEPT
{
    this->dj_arg_config_format::value_delimiter = _character;

    return *this;
}

inline
config_format& config_format::quote_character(
    char _string_quote,
    char _literal_quote
) D_NOEXCEPT
{
    this->dj_arg_config_format::string_quote  = _string_quote;
    this->dj_arg_config_format::literal_quote = _literal_quote;

    return *this;
}

inline
config_format& config_format::max_layers(
    int _layers
) D_NOEXCEPT
{
    d_arg_config_format_set_max_layers(this,
                                       _layers);

    return *this;
}

inline
config_format& config_format::parent_separator(
    char _character
) D_NOEXCEPT
{
    this->dj_arg_config_format::parent_separator = _character;

    return *this;
}

inline
config_format& config_format::comment_defaults(
    bool _value
) D_NOEXCEPT
{
    d_arg_config_format_set_flag(this,
                                 D_ARG_CONFIG_COMMENT_DEFAULTS,
                                 _value);

    return *this;
}

inline
config_format& config_format::allow_duplicate_fields(
    bool _value
) D_NOEXCEPT
{
    d_arg_config_format_set_flag(this,
                                 D_ARG_CONFIG_ALLOW_DUPLICATE_FIELDS,
                                 _value);

    return *this;
}

// config_format::get_comment and kin
//   The characters' fields, read.
inline
char config_format::get_comment() const D_NOEXCEPT
{
    return this->dj_arg_config_format::comment;
}

inline
char config_format::get_array_start() const D_NOEXCEPT
{
    return this->dj_arg_config_format::array_start;
}

inline
char config_format::get_array_end() const D_NOEXCEPT
{
    return this->dj_arg_config_format::array_end;
}

inline
char config_format::get_array_separator() const D_NOEXCEPT
{
    return this->dj_arg_config_format::array_separator;
}

inline
char config_format::get_value_separator() const D_NOEXCEPT
{
    return this->dj_arg_config_format::value_delimiter;
}

inline
char config_format::get_string_quote() const D_NOEXCEPT
{
    return this->dj_arg_config_format::string_quote;
}

inline
char config_format::get_literal_quote() const D_NOEXCEPT
{
    return this->dj_arg_config_format::literal_quote;
}

inline
char config_format::get_parent_separator() const D_NOEXCEPT
{
    return this->dj_arg_config_format::parent_separator;
}

// config_format::section / index / get_section and kin
//   The C settings; the fields, read, the section "" when none is set.
inline
config_format& config_format::section(
    DCString _section
)
{
    internal::raise_out_of_memory(d_arg_config_format_set_section(
                                      this,
                                      _section.get()));

    return *this;
}

inline
config_format& config_format::index(
    int _index
) D_NOEXCEPT
{
    d_arg_config_format_set_index(this,
                                  _index);

    return *this;
}

inline
const char* config_format::get_section() const D_NOEXCEPT
{
    return (this->dj_arg_config_format::section != D_NULLPTR)
           ? this->dj_arg_config_format::section
           : "";
}

inline
int config_format::get_index() const D_NOEXCEPT
{
    return this->dj_arg_config_format::index;
}

inline
int config_format::get_max_layers() const D_NOEXCEPT
{
    return this->dj_arg_config_format::max_layers;
}

inline
bool config_format::get_comment_defaults() const D_NOEXCEPT
{
    return ((this->dj_arg_config_format::flags &
             D_ARG_CONFIG_COMMENT_DEFAULTS) != 0u);
}

inline
bool config_format::get_allow_duplicate_fields() const D_NOEXCEPT
{
    return ((this->dj_arg_config_format::flags &
             D_ARG_CONFIG_ALLOW_DUPLICATE_FIELDS) != 0u);
}

// config_format::from_config / to_config
//   With configuration files: the C library's default operations, with
// this format. Without them: unsupported, as the C library has no text to
// read or write.
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

inline
cli::status config_format::from_config(
    const text&   _input,
    DConfigItems& _out,
    buffer&       _error
) const
{
    return cli::status(d_arg_config_from_text_default(this,
                                                      _input,
                                                      &_out.get(),
                                                      &_error.get()));
}

inline
cli::status config_format::to_config(
    const cli::command&       _command,
    config_output_mode::value _mode,
    bool                      _write_description,
    const text&               _prefix,
    buffer&                   _out
) const
{
    return cli::status(d_arg_config_to_text_default(this,
                                                    _command.get(),
                                                    static_cast<int>(_mode),
                                                    _write_description,
                                                    _prefix,
                                                    &_out.get()));
}

#else

inline
cli::status config_format::from_config(
    const text&   _input,
    DConfigItems& _out,
    buffer&       _error
) const
{
    (void)_input;
    (void)_out;
    (void)_error;

    return cli::status(status_code::unsupported);
}

inline
cli::status config_format::to_config(
    const cli::command&       _command,
    config_output_mode::value _mode,
    bool                      _write_description,
    const text&               _prefix,
    buffer&                   _out
) const
{
    (void)_command;
    (void)_mode;
    (void)_write_description;
    (void)_prefix;
    (void)_out;

    return cli::status(status_code::unsupported);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// config_format::to_flag
//   The C library's, with this format.
inline
cli::status config_format::to_flag(
    const DConfigEntry& _item,
    buffer&             _out
) const
{
    return cli::status(d_arg_config_to_flag_default(this,
                                                    _item.get(),
                                                    &_out.get()));
}

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// config_format::from_text / from_file
//   The C dispatchers, through this format's table, the description built
// in a scratch buffer and appended.
inline
cli::status config_format::from_text(
    const text&      _input,
    DConfigItemList& _out,
    std::string&     _error
) const
{
    internal::scratch described;
    const cli::status result(d_arg_config_from_text(this,
                                                    _input,
                                                    &_out,
                                                    described.get()));

    _error.append(described.str());

    return result;
}

inline
cli::status config_format::from_file(
    DCString         _path,
    DConfigItemList& _out,
    std::string&     _error
) const
{
    internal::scratch described;
    const cli::status result(d_arg_config_from_file(this,
                                                    _path.get(),
                                                    &_out,
                                                    described.get()));

    _error.append(described.str());

    return result;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// 4.6    DConfigIni
//------------------------------------------------------------------------------
// DConfigIni::DConfigIni
//   The format made as TOML, then made again as INI, which the C library's
// init leaves without a table; the table taken back.
inline
DConfigIni::DConfigIni() D_NOEXCEPT
    : config_format()
{
    d_arg_config_format_init_ini(this);
    own_table();
}

// 4.7    Internals
//------------------------------------------------------------------------------
// internal::DFromConfigCall / DToConfigCall / DToFlagCall
//   The arguments kept as given -- references to the operation function's
// locals -- and the virtual function called.
inline
internal::DFromConfigCall::DFromConfigCall(
    const config_format& _self,
    const text&          _input,
    DConfigItems&        _out,
    buffer&              _error
) D_NOEXCEPT
    : m_self(_self),
      m_input(_input),
      m_out(_out),
      m_error(_error)
{}

inline
int internal::DFromConfigCall::operator()() const
{
    return m_self.from_config(m_input,
                              m_out,
                              m_error).code();
}

inline
internal::DToConfigCall::DToConfigCall(
    const config_format&      _self,
    const cli::command&       _command,
    config_output_mode::value _mode,
    bool                      _write_description,
    const text&               _prefix,
    buffer&                   _out
) D_NOEXCEPT
    : m_self(_self),
      m_command(_command),
      m_mode(_mode),
      m_write_description(_write_description),
      m_prefix(_prefix),
      m_out(_out)
{}

inline
int internal::DToConfigCall::operator()() const
{
    return m_self.to_config(m_command,
                            m_mode,
                            m_write_description,
                            m_prefix,
                            m_out).code();
}

inline
internal::DToFlagCall::DToFlagCall(
    const config_format& _self,
    const DConfigEntry&  _item,
    buffer&              _out
) D_NOEXCEPT
    : m_self(_self),
      m_item(_item),
      m_out(_out)
{}

inline
int internal::DToFlagCall::operator()() const
{
    return m_self.to_flag(m_item,
                          m_out).code();
}

// internal::config_table
//   A table of constant addresses, made before any code runs.
inline
const ::dj_arg_config_format_vtable& internal::config_table() D_NOEXCEPT
{
    static const ::dj_arg_config_format_vtable table =
    {
        &internal::operation_from_config,
        &internal::operation_to_config,
        &internal::operation_to_flag
    };

    return table;
}

// internal::operation_from_config and kin
//   The format object from the context; the C records as views and
// handles, the C buffers as buffers; the virtual operation under guard.
inline
int internal::operation_from_config(
    const ::dj_arg_config_format* _format,
    ::dj_pack_text                _input,
    ::dj_arg_config_item_list*    _out,
    ::dj_arg_buffer*              _error
)
{
    const config_format& self = *static_cast<const config_format*>(
                                    _format->context);
    const text           input(_input);
    DConfigItems         out(*_out);
    buffer               error(*_error);

    return internal::guard(internal::DFromConfigCall(self,
                                                      input,
                                                      out,
                                                      error),
                           internal::conversion_failure(D_NULLPTR));
}

inline
int internal::operation_to_config(
    const ::dj_arg_config_format* _format,
    const ::dj_arg_command*       _command,
    int                           _mode,
    bool                          _write_description,
    ::dj_pack_text                _prefix,
    ::dj_arg_buffer*              _out
)
{
    const config_format& self = *static_cast<const config_format*>(
                                    _format->context);
    const cli::command   subject(const_cast< ::dj_arg_command*>(_command));
    const text           prefix(_prefix);
    buffer               out(*_out);

    return internal::guard(internal::DToConfigCall(
                               self,
                               subject,
                               static_cast<config_output_mode::value>(_mode),
                               _write_description,
                               prefix,
                               out),
                           internal::conversion_failure(D_NULLPTR));
}

inline
int internal::operation_to_flag(
    const ::dj_arg_config_format* _format,
    const ::dj_arg_config_item*   _item,
    ::dj_arg_buffer*              _out
)
{
    const config_format& self = *static_cast<const config_format*>(
                                    _format->context);
    const DConfigEntry   item(_item);
    buffer               out(*_out);

    return internal::guard(internal::DToFlagCall(self,
                                                  item,
                                                  out),
                           internal::conversion_failure(D_NULLPTR));
}

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_CONFIG_HPP
