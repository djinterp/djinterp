/*******************************************************************************
* djinterp [env]                                                 env_printer.hpp
*
* djinterp environment printer, the C++ face.
*   DEnvPrinter is env_printer.h's machine as a C++ object: configured by
* statements and scripts, by the process's environment, by its templates and
* by C++ callables, and printing any report to any destination -- a d_ostream
* or DOutputStream, a FILE*, a std::ostream, a std::string, a caller's array.
* djinterp::print_env does the same for the calling unit's own environment
* in one call. Everything is env_printer.h's; this header lets C++ reach it.
*   Usage:
*     djinterp::print_env(std::cout);                        // the defaults
*     djinterp::print_env(std::cout, "style=json; sections=all");
*
*     djinterp::DEnvPrinter printer("style=markdown");       // keep one
*     printer.put("select=-cpp_features.*");
*     printer.set_template(D_ENV_TEMPLATE_ENTRY, "- {label}: {display}\n");
*     djinterp::print_env(printer, std::cout);
*
*     djinterp::DEnvPrinter shared(*d_env_printer_djin());   // the program's
*                                                            // settings
*
*   THE REPORT IS THE CALLER'S UNIT'S, SO IT IS PASSED IN. A class's inline
* members are one definition across a program, and so cannot each call the
* d_env_report_here of the unit they are compiled in; DEnvPrinter's members
* take a report, and the print_env functions -- internal linkage, as the
* collector is -- give them the calling unit's.
*   A filter or a field hook in C++ is an object deriving from DEnvFilter or
* DEnvField, or any callable through make_env_filter and make_env_field; each
* reaches the C machine through one extern "C" function, so the pointer the
* machine holds has C linkage, as its type says.
*   Every member and function reports whether it succeeded; none throws.
* C++98 and up, on the option core's 64-bit floor (env_printer.h).
*
*
* path:      /inc/djinterp/env/env_printer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.22
*                                                            revised: 2026.11.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  HOOKS
    -----
    1.  Filters
         1.  DEnvFilter
         2.  DEnvFilterOf
         3.  make_env_filter
    2.  Field hooks
         1.  DEnvField
         2.  DEnvFieldOf
         3.  make_env_field
2.  THE PRINTER
    -----------
    1.  DEnvPrinter
         1.  Construction
         2.  Configuration
         3.  Hooks
         4.  Printing a report
         5.  The variables as a script
         6.  State
3.  THE CALLING UNIT'S ENVIRONMENT
    ------------------------------
    1.  The report
    2.  With a printer
    3.  In one call
*/

#ifndef DJINTERP_ENV_ENV_PRINTER_HPP
#define DJINTERP_ENV_ENV_PRINTER_HPP 1

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::FILE
#include <ostream>  // std::ostream
#include <string>   // std::string
// djinterp
#include "../cpp/djinterp.hpp"                       // framework root
#include "./env_printer.h"                           // the machine
#include "../cpp/util/output/output_stream.hpp"      // DOutputStream
#include "../cpp/util/output/output_stream_std.hpp"  // attach_std_*
// re_std
#include "../../re_std/cpp/cstdint/cstdint.hpp"  // int32_t, uint32_t

#if RE_STD_HAS_INT64


NS_DJINTERP


//==============================================================================
// 1.  HOOKS
//==============================================================================


// 1.1    Filters
//------------------------------------------------------------------------------
// 1.1.1
// DEnvFilter
//   class: a filter as a C++ object: shows() decides whether an entry, a
// feature or a note shows, as dj_env_filter_fn does. Borrowed by the printer
// it is given to, so it must outlive it.
class DEnvFilter
{
public:
    virtual ~DEnvFilter()
    {
    }

    /**
     * @brief Whether an item shows.
     *
     * @param[in] _item  the item; its place is not yet known.
     * @return true to show it.
     */
    virtual
    bool shows(
        const struct dj_env_item& _item
    ) = 0;
};

// 1.1.2
// DEnvFilterOf
//   class: a filter from any callable taking a const dj_env_item& and giving
// something that converts to bool -- a function, a functor, a lambda.
template<typename Callable>
class DEnvFilterOf : public DEnvFilter
{
public:
    /**
     * @brief Keeps a copy of the callable.
     *
     * @param[in] _callable  the callable.
     */
    explicit
    DEnvFilterOf(
        const Callable& _callable
    )
        : m_callable(_callable)
    {
    }

    /**
     * @brief Asks the callable.
     *
     * @param[in] _item  the item.
     * @return its answer.
     */
    bool shows(
        const struct dj_env_item& _item
    )
    {
        return static_cast<bool>(m_callable(_item));
    }

private:
    Callable m_callable;
};

// 1.1.3
// make_env_filter
//   function: a DEnvFilterOf for a callable, its type deduced.
template<typename Callable>
DEnvFilterOf<Callable> make_env_filter(
    const Callable& _callable
)
{
    return DEnvFilterOf<Callable>(_callable);
}

// 1.2    Field hooks
//------------------------------------------------------------------------------
// 1.2.1
// DEnvField
//   class: a field hook as a C++ object, as dj_env_field_fn: field() returns
// 1 when it wrote the value, 0 to leave the field to the printer, and a
// negative number to stop the print. Borrowed, as a filter is.
class DEnvField
{
public:
    virtual ~DEnvField()
    {
    }

    /**
     * @brief A field's value, ahead of the printer's.
     *
     * @param[in]  _item   the part being printed.
     * @param[in]  _field  the field: enum dj_env_field, or D_ENV_FIELDS plus a
     *                     variable's index.
     * @param[out] _value  the value, when it gives one.
     * @return 1, 0 or negative, as above.
     */
    virtual
    int field(
        const struct dj_env_item& _item,
        uint32_t                  _field,
        struct dj_ostream_value&  _value
    ) = 0;
};

// 1.2.2
// DEnvFieldOf
//   class: a field hook from any callable taking (const dj_env_item&,
// uint32_t, dj_ostream_value&) and giving an int.
template<typename Callable>
class DEnvFieldOf : public DEnvField
{
public:
    /**
     * @brief Keeps a copy of the callable.
     *
     * @param[in] _callable  the callable.
     */
    explicit
    DEnvFieldOf(
        const Callable& _callable
    )
        : m_callable(_callable)
    {
    }

    /**
     * @brief Asks the callable.
     *
     * @param[in]  _item   the part.
     * @param[in]  _field  the field.
     * @param[out] _value  the value.
     * @return its answer.
     */
    int field(
        const struct dj_env_item& _item,
        uint32_t                  _field,
        struct dj_ostream_value&  _value
    )
    {
        return static_cast<int>(m_callable(_item, _field, _value));
    }

private:
    Callable m_callable;
};

// 1.2.3
// make_env_field
//   function: a DEnvFieldOf for a callable, its type deduced.
template<typename Callable>
DEnvFieldOf<Callable> make_env_field(
    const Callable& _callable
)
{
    return DEnvFieldOf<Callable>(_callable);
}

NS_INTERNAL

    extern "C"
    {
        /**
         * @brief The C machine's filter for a DEnvFilter.
         *
         * @param[in] _context  the DEnvFilter.
         * @param[in] _item     the item.
         * @return 1 to show it.
         */
        inline int32_t d_internal_cxx_env_filter(
            void*                     _context,
            const struct dj_env_item* _item
        )
        {
            return static_cast<DEnvFilter*>(_context)->shows(*_item) ? 1 : 0;
        }

        /**
         * @brief The C machine's field hook for a DEnvField.
         *
         * @param[in]  _context  the DEnvField.
         * @param[in]  _item     the part.
         * @param[in]  _field    the field.
         * @param[out] _value    the value.
         * @return the hook's answer.
         */
        inline int32_t d_internal_cxx_env_field(
            void*                     _context,
            const struct dj_env_item* _item,
            uint32_t                  _field,
            struct dj_ostream_value*  _value
        )
        {
            return static_cast<int32_t>(
                static_cast<DEnvField*>(_context)->field(*_item,
                                                         _field,
                                                         *_value));
        }
    }

NS_END  // internal


//==============================================================================
// 2.  THE PRINTER
//==============================================================================


// 2.1    DEnvPrinter
//------------------------------------------------------------------------------
// DEnvPrinter
//   class: the environment printer: a struct dj_env_printer, made at
// construction. Non-copyable, as the machine points into itself; make
// another and configure it the same way -- dump() writes the script that
// does it.
class DEnvPrinter
{
public:
    // 2.1.1
    // Construction
    /**
     * @brief A printer at its defaults.
     */
    DEnvPrinter()
        : m_status(0)
    {
        m_status = (d_env_printer_init(&m_printer) != 0)
                       ? static_cast<int32_t>(D_OPTION_STATUS_OK)
                       : static_cast<int32_t>(
                             D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    /**
     * @brief A printer configured by a script.
     *
     * @param[in] _script  statements, as load() reads them; status() says
     *                     whether they all succeeded.
     */
    explicit
    DEnvPrinter(
        const char* _script
    )
        : m_status(0)
    {
        m_status = (d_env_printer_init(&m_printer) != 0)
                       ? static_cast<int32_t>(D_OPTION_STATUS_OK)
                       : static_cast<int32_t>(
                             D_OPTION_STATUS_INVALID_ARGUMENT);

        if ( (m_status == static_cast<int32_t>(D_OPTION_STATUS_OK)) &&
             (_script != D_NULLPTR) )
        {
            (void)load(_script);
        }
    }

    /**
     * @brief A printer made in a parent frame -- the djin,
     *        *d_env_printer_djin(), or another printer's env() -- and
     *        configured by a script.
     *
     * @param[in,out] _parent  the parent; it must outlive the printer.
     * @param[in]     _script  statements, as load() reads them, or null;
     *                         status() says whether they all succeeded.
     */
    explicit
    DEnvPrinter(
        struct dj_option_env& _parent,
        const char*           _script = D_NULLPTR
    )
        : m_status(0)
    {
        m_status = (d_env_printer_init_in(&m_printer, &_parent) != 0)
                       ? static_cast<int32_t>(D_OPTION_STATUS_OK)
                       : static_cast<int32_t>(
                             D_OPTION_STATUS_INVALID_ARGUMENT);

        if ( (m_status == static_cast<int32_t>(D_OPTION_STATUS_OK)) &&
             (_script != D_NULLPTR) )
        {
            (void)load(_script);
        }
    }

    /**
     * @brief Lets go of the parent, if the printer was made in one.
     */
    ~DEnvPrinter()
    {
        d_env_printer_release(&m_printer);
    }

    // 2.1.2
    // Configuration
    //   Each is env_printer.h's, and returns whether it succeeded; status()
    // then gives the option status, and error() where a template failed.

    /**
     * @brief Sets one variable from text.
     *
     * @param[in] _name   the name, with an optional "section." scope.
     * @param[in] _value  the value.
     * @return true when it was set.
     */
    bool set(
        const char* _name,
        const char* _value
    )
    {
        return keep(d_env_printer_set(&m_printer, _name, _value));
    }

    /**
     * @brief Carries out one statement.
     *
     * @param[in] _statement  "name=value", "name" or "-name".
     * @return true when it was carried out.
     */
    bool put(
        const char* _statement
    )
    {
        return keep(d_env_printer_put(&m_printer, _statement));
    }

    /**
     * @brief Carries out a script.
     *
     * @param[in]  _script        the statements.
     * @param[out] _error_offset  where a failing statement starts; may be
     *                            null.
     * @return true when every statement was carried out.
     */
    bool load(
        const char*  _script,
        std::size_t* _error_offset = D_NULLPTR
    )
    {
        return keep(d_env_printer_load(&m_printer, _script, _error_offset));
    }

    /**
     * @brief Sets variables from the process's environment.
     *
     * @param[in] _prefix  the prefix.
     * @return true when every one found was set.
     */
    bool import_environment(
        const char* _prefix = D_ENV_PRINTER_ENV_PREFIX
    )
    {
        return keep(d_env_printer_import(&m_printer, _prefix));
    }

    /**
     * @brief Sets a part's template, refused when it does not compile.
     *
     * @param[in] _part    the part.
     * @param[in] _source  the template; null for the style's.
     * @return true when it was set.
     */
    bool set_template(
        enum dj_env_template _part,
        const char*          _source
    )
    {
        return keep(d_env_printer_set_template(&m_printer,
                                               static_cast<int32_t>(_part),
                                               _source));
    }

    /**
     * @brief Declares a variable of the program's own.
     *
     * @param[in] _var  the declaration; borrowed.
     * @return true when it was declared.
     */
    bool declare(
        const struct dj_option_env_var& _var
    )
    {
        return keep(d_env_printer_declare(&m_printer, &_var));
    }

    /**
     * @brief Every variable back to its default.
     */
    void reset()
    {
        d_env_printer_reset(&m_printer);
        m_status = static_cast<int32_t>(D_OPTION_STATUS_OK);

        return;
    }

    // 2.1.3
    // Hooks
    /**
     * @brief Sets a filter; borrowed.
     *
     * @param[in] _filter  the filter.
     */
    void set_filter(
        DEnvFilter& _filter
    )
    {
        d_env_printer_set_filter(&m_printer,
                                 &internal::d_internal_cxx_env_filter,
                                 &_filter);

        return;
    }

    /**
     * @brief Sets a field hook; borrowed.
     *
     * @param[in] _field  the hook.
     */
    void set_field(
        DEnvField& _field
    )
    {
        d_env_printer_set_field(&m_printer,
                                &internal::d_internal_cxx_env_field,
                                &_field);

        return;
    }

    /**
     * @brief Removes the filter and the field hook.
     */
    void clear_hooks()
    {
        d_env_printer_set_filter(&m_printer, D_NULLPTR, D_NULLPTR);
        d_env_printer_set_field(&m_printer, D_NULLPTR, D_NULLPTR);

        return;
    }

    // 2.1.4
    // Printing a report
    //   Each prints _report: env_report() for the calling unit's, or the
    // print_env functions, which pass it.

    /**
     * @brief Prints onto a C stream; writes only.
     *
     * @param[in,out] _out     the stream.
     * @param[in]     _report  the report.
     * @return true when the stream accepted every byte.
     */
    bool print(
        struct dj_ostream*          _out,
        const struct dj_env_report& _report
    )
    {
        return d_env_printer_print(&m_printer, _out, &_report) != 0;
    }

    /**
     * @brief Prints onto a DOutputStream; writes only.
     *
     * @param[in,out] _out     the stream.
     * @param[in]     _report  the report.
     * @return true when the stream accepted every byte.
     */
    bool print(
        DOutputStream&              _out,
        const struct dj_env_report& _report
    )
    {
        return print(_out.get(), _report);
    }

    /**
     * @brief Prints to a FILE*, and flushes it.
     *
     * @param[in,out] _file    the file.
     * @param[in]     _report  the report.
     * @return true when the whole report reached the file.
     */
    bool print(
        std::FILE*                  _file,
        const struct dj_env_report& _report
    )
    {
        DBufferedOutputStream<> stream;

        return ( (stream.attach_file(_file)) &&
                 (print(stream, _report)) &&
                 (stream.close()) );
    }

    /**
     * @brief Prints to a std::ostream, and flushes it.
     *
     * @param[in,out] _out     the std::ostream.
     * @param[in]     _report  the report.
     * @return true when the whole report reached it.
     */
    bool print(
        std::ostream&               _out,
        const struct dj_env_report& _report
    )
    {
        DBufferedOutputStream<> stream;

        return ( (attach_std_ostream(stream, _out)) &&
                 (print(stream, _report)) &&
                 (stream.close()) );
    }

    /**
     * @brief Appends to a std::string.
     *
     * @param[in,out] _out     the string.
     * @param[in]     _report  the report.
     * @return true when the whole report was appended.
     */
    bool print(
        std::string&                _out,
        const struct dj_env_report& _report
    )
    {
        DBufferedOutputStream<> stream;

        return ( (attach_std_string(stream, _out)) &&
                 (print(stream, _report)) &&
                 (stream.close()) );
    }

    /**
     * @brief Prints into a caller's array, with snprintf's contract.
     *
     * @param[out] _buffer    the array.
     * @param[in]  _capacity  its size.
     * @param[in]  _report    the report.
     * @param[out] _needed    the report's whole length; may be null.
     * @return true when the whole report fitted.
     */
    bool print(
        char*                       _buffer,
        std::size_t                 _capacity,
        const struct dj_env_report& _report,
        std::size_t*                _needed = D_NULLPTR
    )
    {
        DOutputStream stream;

        const bool printed = ( (stream.attach_fixed(_buffer,
                                                    _capacity,
                                                    true)) &&
                               (print(stream, _report)) );
        const bool closed  = stream.close();

        if (_needed != D_NULLPTR)
        {
            *_needed = stream.written();
        }

        return ( (printed) &&
                 (closed) &&
                 (!stream.fixed_truncated()) );
    }

    // 2.1.5
    // The variables as a script
    /**
     * @brief The variables as a script that sets them again.
     *
     * @param[in] _defaults  true to include unbound ones, commented out.
     * @return the script; empty if it could not be written.
     */
    std::string dump(
        bool _defaults = false
    ) const
    {
        std::string             script;
        DBufferedOutputStream<> stream;

        if ( (!attach_std_string(stream, script)) ||
             (d_env_printer_dump(&m_printer,
                                 stream.get(),
                                 _defaults ? 1 : 0) == 0) ||
             (!stream.close()) )
        {
            script.clear();
        }

        return script;
    }

    // 2.1.6
    // State
    /**
     * @brief The last configuration's option status.
     *
     * @return D_OPTION_STATUS_OK, or why it failed.
     */
    D_NODISCARD
    int32_t status() const
    {
        return m_status;
    }

    /**
     * @brief Where the templates last failed to compile.
     *
     * @return the record; its status is OK when they compiled.
     */
    D_NODISCARD
    const struct dj_env_printer_error& error() const
    {
        return *d_env_printer_last_error(&m_printer);
    }

    /**
     * @brief The machine's environment, for option_env.h's interface.
     *
     * @return the frame.
     */
    D_NODISCARD
    struct dj_option_env* env()
    {
        return d_env_printer_env(&m_printer);
    }

    /**
     * @brief The machine.
     *
     * @return it.
     */
    D_NODISCARD
    struct dj_env_printer* get()
    {
        return &m_printer;
    }

private:
    // non-copyable: the machine points into itself
    DEnvPrinter(const DEnvPrinter&);
    DEnvPrinter& operator=(const DEnvPrinter&);

    /**
     * @brief Keeps a result's status.
     *
     * @param[in] _result  the result.
     * @return whether it succeeded.
     */
    bool keep(
        struct dj_option_result _result
    )
    {
        m_status = d_option_result_status(&_result);

        return _result.is_ok;
    }

    struct dj_env_printer m_printer;
    int32_t               m_status;
};


//==============================================================================
// 3.  THE CALLING UNIT'S ENVIRONMENT
//==============================================================================
//   Internal linkage, every one, as d_env_report_here's: each unit's report is
// its own.


// 3.1    The report
//------------------------------------------------------------------------------
/**
 * @brief The calling unit's environment, as a report.
 *
 * @return the report, constant, alive for the program's life.
 */
D_STATIC_INLINE
const struct dj_env_report& env_report()
{
    return *d_env_report_here();
}

// 3.2    With a printer
//------------------------------------------------------------------------------
//   _printer's configuration, the calling unit's report.

/**
 * @brief Prints onto a DOutputStream; writes only.
 *
 * @param[in,out] _printer  the printer.
 * @param[in,out] _out      the stream.
 * @return true when the stream accepted every byte.
 */
D_STATIC_INLINE
bool print_env(
    DEnvPrinter&   _printer,
    DOutputStream& _out
)
{
    return _printer.print(_out, env_report());
}

/**
 * @brief Prints to a FILE*, and flushes it.
 *
 * @param[in,out] _printer  the printer.
 * @param[in,out] _file     the file.
 * @return true when the whole report reached the file.
 */
D_STATIC_INLINE
bool print_env(
    DEnvPrinter& _printer,
    std::FILE*   _file
)
{
    return _printer.print(_file, env_report());
}

/**
 * @brief Prints to a std::ostream, and flushes it.
 *
 * @param[in,out] _printer  the printer.
 * @param[in,out] _out      the std::ostream.
 * @return true when the whole report reached it.
 */
D_STATIC_INLINE
bool print_env(
    DEnvPrinter&  _printer,
    std::ostream& _out
)
{
    return _printer.print(_out, env_report());
}

/**
 * @brief Appends to a std::string.
 *
 * @param[in,out] _printer  the printer.
 * @param[in,out] _out      the string.
 * @return true when the whole report was appended.
 */
D_STATIC_INLINE
bool print_env(
    DEnvPrinter&  _printer,
    std::string&  _out
)
{
    return _printer.print(_out, env_report());
}

// 3.3    In one call
//------------------------------------------------------------------------------
//   A printer for the call, configured by _script (null for the defaults),
// the calling unit's report. A script that fails prints nothing and returns
// false.

/**
 * @brief Prints onto a DOutputStream; writes only.
 *
 * @param[in,out] _out     the stream.
 * @param[in]     _script  the configuration, or null.
 * @return true when the stream accepted every byte.
 */
D_STATIC_INLINE
bool print_env(
    DOutputStream& _out,
    const char*    _script = D_NULLPTR
)
{
    return d_env_print(_out.get(), _script) != 0;
}

/**
 * @brief Prints to a FILE*, and flushes it.
 *
 * @param[in,out] _file    the file; stdout for the console.
 * @param[in]     _script  the configuration, or null.
 * @return true when the whole report reached the file.
 */
D_STATIC_INLINE
bool print_env(
    std::FILE*  _file,
    const char* _script = D_NULLPTR
)
{
    return d_env_print_file(_file, _script) != 0;
}

/**
 * @brief Prints to a std::ostream, and flushes it.
 *
 * @param[in,out] _out     the std::ostream.
 * @param[in]     _script  the configuration, or null.
 * @return true when the whole report reached it.
 */
D_STATIC_INLINE
bool print_env(
    std::ostream& _out,
    const char*   _script = D_NULLPTR
)
{
    DEnvPrinter printer;

    return ( (_script == D_NULLPTR) ||
             (printer.load(_script)) ) &&
           (print_env(printer, _out));
}

/**
 * @brief Appends to a std::string.
 *
 * @param[in,out] _out     the string.
 * @param[in]     _script  the configuration, or null.
 * @return true when the whole report was appended.
 */
D_STATIC_INLINE
bool print_env(
    std::string& _out,
    const char*  _script = D_NULLPTR
)
{
    DEnvPrinter printer;

    return ( (_script == D_NULLPTR) ||
             (printer.load(_script)) ) &&
           (print_env(printer, _out));
}

/**
 * @brief Prints into a caller's array, with snprintf's contract.
 *
 * @param[out] _buffer    the array.
 * @param[in]  _capacity  its size.
 * @param[in]  _script    the configuration, or null.
 * @param[out] _needed    the report's whole length; may be null.
 * @return true when the whole report fitted.
 */
D_STATIC_INLINE
bool print_env(
    char*        _buffer,
    std::size_t  _capacity,
    const char*  _script = D_NULLPTR,
    std::size_t* _needed  = D_NULLPTR
)
{
    DEnvPrinter printer;

    return ( (_script == D_NULLPTR) ||
             (printer.load(_script)) ) &&
           (printer.print(_buffer, _capacity, env_report(), _needed));
}


NS_END  // djinterp


#endif  // RE_STD_HAS_INT64

#endif  // DJINTERP_ENV_ENV_PRINTER_HPP
