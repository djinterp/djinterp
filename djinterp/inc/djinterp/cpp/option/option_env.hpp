/*******************************************************************************
* djinterp [cpp]                                                  option_env.hpp
*
* option_env's C++ face: a frame of run-time variables as a C++ object.
*   option_env.h is the machine: layered frames of declared variables, each
* bound from a source of known standing, read through the chain. This header
* lets C++ reach it without spelling the machine's calls:
*     DOptionEnvView    any frame, C's or C++'s, borrowed: finding, typed
*                       reads, binding from text and from a source, the
*                       value in force as text, did you mean;
*     DOptionEnv<...>   a frame and its storage in one object, a root or a
*                       child of another frame, which it lets go of when it
*                       goes;
*     DOptionEnvOverride  a variable forced from the OVERRIDE source for the
*                       override's life -- a test's, a scope's -- and given
*                       back, the OVERRIDE binding the frame had before
*                       restored, when it ends.
*   Usage:
*     djinterp::DOptionEnv<32, 8, 256> env;              // a root
*     env.declare_all(SCHEMA, 8);
*     env.load("width=80; color=never", D_OPTION_ENV_SOURCE_FILE);
*     const int64_t width = env.integer(env.find("width"));
*     {
*         djinterp::DOptionEnvOverride narrow(*env.get(), "width", "40");
*         run();                                         // width reads 40
*     }                                                  // and 80 again
*
*   Every member reports whether it succeeded, status() saying why not; none
* throws. A frame points into its storage, so DOptionEnv is not copied, and an
* override is not either. C++98 and up, on the option core's 64-bit floor
* (option_env.h).
*
*
* path:      /inc/djinterp/cpp/option/option_env.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  A FRAME, BORROWED
    -----------------
    1.  DOptionEnvView
         1.  Construction
         2.  Finding
         3.  Reading
         4.  Binding
         5.  State
2.  A FRAME, OWNED
    --------------
    1.  DOptionEnv
3.  A SCOPED OVERRIDE
    -----------------
    1.  DOptionEnvOverride
*/

#ifndef DJINTERP_OPTION_ENV_HPP
#define DJINTERP_OPTION_ENV_HPP 1

// std
#include <cstddef>  // std::size_t
#include <cstring>  // std::strlen, std::memset
#include <string>   // std::string
// djinterp
#include "../djinterp.hpp"                    // framework root
#include "../../c/option/option_common.h"     // d_option_result_status
#include "../../c/option/option_env.h"        // the machine
// re_std
#include "../../../re_std/cpp/cstdint/cstdint.hpp"  // int64_t, uint32_t;
                                                    // RE_STD_HAS_INT64

#if RE_STD_HAS_INT64


NS_DJINTERP


//==============================================================================
// 1.  A FRAME, BORROWED
//==============================================================================


// 1.1    DOptionEnvView
//------------------------------------------------------------------------------
// DOptionEnvView
//   class: a frame the caller keeps -- C's, a registry's, a printer's -- as a
// C++ object. Copied freely: it is the frame's address and the status of the
// last binding made through it. A variable is named by its id, from find(),
// and read in a scope, the global one unless one is given.
class DOptionEnvView
{
public:
    // 1.1.1
    // Construction
    /**
     * @brief A view of a frame.
     *
     * @param[in,out] _env  the frame; borrowed, so it must outlive the view.
     */
    explicit
    DOptionEnvView(
        struct dj_option_env* _env
    )
        : m_env(_env),
          m_status(static_cast<int32_t>(D_OPTION_STATUS_OK))
    {
    }

    /**
     * @brief The frame, for option_env.h's whole interface.
     *
     * @return the frame.
     */
    D_NODISCARD
    struct dj_option_env* get() const
    {
        return m_env;
    }

    // 1.1.2
    // Finding
    /**
     * @brief A variable's id from its name or alias.
     *
     * @param[in] _name  the name.
     * @return the id, or D_OPTION_ENV_NONE for a name nothing has.
     */
    D_NODISCARD
    uint32_t find(
        const char* _name
    ) const
    {
        return (_name != D_NULLPTR) ? find_n(_name, std::strlen(_name))
                                    : D_OPTION_ENV_NONE;
    }

    /**
     * @brief A variable's id from its name or alias.
     *
     * @param[in] _name  the name.
     * @return the id, or D_OPTION_ENV_NONE for a name nothing has.
     */
    D_NODISCARD
    uint32_t find(
        const std::string& _name
    ) const
    {
        return find_n(_name.data(), _name.size());
    }

    /**
     * @brief A scope's id from its name.
     *
     * @param[in] _name  the name.
     * @return the id, or D_OPTION_ENV_NONE for a name no scope has.
     */
    D_NODISCARD
    uint32_t find_scope(
        const char* _name
    ) const
    {
        if (_name == D_NULLPTR)
        {
            return D_OPTION_ENV_NONE;
        }

        const struct dj_option_result found =
            d_option_env_find_scope(m_env, _name, std::strlen(_name));

        return found.is_ok ? found.payload.value : D_OPTION_ENV_NONE;
    }

    /**
     * @brief Did you mean: the visible name nearest one nothing has.
     *
     * @param[in] _name  the name, as written.
     * @return the name to suggest, or null when nothing is near.
     */
    D_NODISCARD
    const char* suggest(
        const char* _name
    ) const
    {
        return (_name != D_NULLPTR)
                   ? d_option_env_suggest(m_env, _name, std::strlen(_name))
                   : D_NULLPTR;
    }

    // 1.1.3
    // Reading
    //   The value in force, through the chain; a variable that is not one
    // reads as false, 0, 0.0 or null.

    D_NODISCARD
    bool boolean(
        uint32_t _var,
        uint32_t _scope = D_OPTION_ENV_GLOBAL
    ) const
    {
        return d_option_env_bool(m_env, _scope, _var);
    }

    D_NODISCARD
    int64_t integer(
        uint32_t _var,
        uint32_t _scope = D_OPTION_ENV_GLOBAL
    ) const
    {
        return d_option_env_int(m_env, _scope, _var);
    }

    D_NODISCARD
    double real(
        uint32_t _var,
        uint32_t _scope = D_OPTION_ENV_GLOBAL
    ) const
    {
        return d_option_env_real(m_env, _scope, _var);
    }

    D_NODISCARD
    const char* text(
        uint32_t _var,
        uint32_t _scope = D_OPTION_ENV_GLOBAL
    ) const
    {
        return d_option_env_text(m_env, _scope, _var);
    }

    /**
     * @brief The value in force and where it came from.
     *
     * @param[in] _var    the variable.
     * @param[in] _scope  the scope.
     * @return the value; kind 0 for a variable that is not one.
     */
    D_NODISCARD
    struct dj_option_env_value value(
        uint32_t _var,
        uint32_t _scope = D_OPTION_ENV_GLOBAL
    ) const
    {
        struct dj_option_env_value out;

        std::memset(&out, 0, sizeof(out));
        (void)d_option_env_get(m_env, _scope, _var, &out);

        return out;
    }

    /**
     * @brief The value in force as text a binding reads back: "true", "-12",
     *        a choice's name, flags' names joined by '|'.
     *
     * @param[in] _var    the variable.
     * @param[in] _scope  the scope.
     * @return the text; empty for a variable that is not one.
     */
    D_NODISCARD
    std::string format(
        uint32_t _var,
        uint32_t _scope = D_OPTION_ENV_GLOBAL
    ) const
    {
        return written(_var, _scope, false);
    }

    /**
     * @brief The value in force as text that loads back as it is: as
     *        format, but a TEXT value quoted where it must be
     *        (d_option_env_format_quoted).
     *
     * @param[in] _var    the variable.
     * @param[in] _scope  the scope.
     * @return the text; empty for a variable that is not one.
     */
    D_NODISCARD
    std::string format_quoted(
        uint32_t _var,
        uint32_t _scope = D_OPTION_ENV_GLOBAL
    ) const
    {
        return written(_var, _scope, true);
    }

    // where the value in force came from: enum dj_option_env_source, DEFAULT
    // for one unbound, -1 for a variable that is not one
    D_NODISCARD
    int32_t source(
        uint32_t _var,
        uint32_t _scope = D_OPTION_ENV_GLOBAL
    ) const
    {
        return d_option_env_source(m_env, _scope, _var);
    }

    // 1.1.4
    // Binding
    //   Each binds in this frame, from the program unless a source is given,
    // and returns whether it succeeded; status() then says why not.

    /**
     * @brief Binds a variable, by name, from text.
     *
     * @param[in] _name    the name, with an optional "scope." before it.
     * @param[in] _value   the text, read as the variable's kind reads it.
     * @param[in] _source  the source.
     * @return true when it was bound.
     */
    bool set(
        const char* _name,
        const char* _value,
        int32_t     _source = D_OPTION_ENV_SOURCE_PROGRAM
    )
    {
        if ( (_name == D_NULLPTR) ||
             (_value == D_NULLPTR) )
        {
            return keep(d_option_fail(
                       static_cast<int32_t>(D_OPTION_STATUS_INVALID_ARGUMENT)));
        }

        return keep(d_option_env_assign_from(m_env,
                                             _name,
                                             std::strlen(_name),
                                             _value,
                                             std::strlen(_value),
                                             _source));
    }

    /**
     * @brief Carries out one statement: "name=value", "name", "-name".
     *
     * @param[in] _statement  the statement.
     * @param[in] _source     the source.
     * @return true when it was carried out.
     */
    bool put(
        const char* _statement,
        int32_t     _source = D_OPTION_ENV_SOURCE_PROGRAM
    )
    {
        return keep(d_option_env_put_from(m_env, _statement, _source));
    }

    /**
     * @brief Carries out a script of statements.
     *
     * @param[in]  _script        the statements.
     * @param[in]  _source        the source: FILE for a file's text.
     * @param[out] _error_offset  where a failing statement starts; may be
     *                            null.
     * @return true when every statement was carried out.
     */
    bool load(
        const char*  _script,
        int32_t      _source       = D_OPTION_ENV_SOURCE_PROGRAM,
        std::size_t* _error_offset = D_NULLPTR
    )
    {
        return keep(d_option_env_load_from(m_env,
                                           _script,
                                           _source,
                                           _error_offset));
    }

    /**
     * @brief Binds variables from the process's environment, under a prefix.
     *
     * @param[in] _prefix  the prefix; null or empty for none.
     * @return true when every one found was bound.
     */
    bool import_environment(
        const char* _prefix
    )
    {
        return keep(d_option_env_import(m_env, _prefix));
    }

    /**
     * @brief Removes this frame's bindings of a variable in a scope.
     *
     * @param[in] _var     the variable.
     * @param[in] _scope   the scope.
     * @return true when the variable is one the frame sees.
     */
    bool unset(
        uint32_t _var,
        uint32_t _scope = D_OPTION_ENV_GLOBAL
    )
    {
        return keep(d_option_env_unset(m_env, _scope, _var));
    }

    // 1.1.5
    // State
    // the last binding's option status; the generation in force, which moves
    // with every change of the chain
    D_NODISCARD
    int32_t status() const
    {
        return m_status;
    }

    D_NODISCARD
    uint32_t generation() const
    {
        return d_option_env_generation(m_env);
    }

protected:
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

    struct dj_option_env* m_env;
    int32_t               m_status;

private:
    /**
     * @brief A value as text, by d_option_env_format or, when quoted,
     *        d_option_env_format_quoted: on the stack, or once more into the
     *        string itself when it is too long for it.
     *
     * @param[in] _var     the variable.
     * @param[in] _scope   the scope.
     * @param[in] _quoted  whether a TEXT value is quoted where it must be.
     * @return the text; empty for a variable that is not one.
     */
    std::string written(
        uint32_t _var,
        uint32_t _scope,
        bool     _quoted
    ) const
    {
        char              small[64];
        const std::size_t length = write(_var, _scope, _quoted, small,
                                         sizeof(small));

        if (length == static_cast<std::size_t>(-1))
        {
            return std::string();
        }

        if (length < sizeof(small))
        {
            return std::string(small, length);
        }

        std::string whole(length + 1u, '\0');

        (void)write(_var, _scope, _quoted, &whole[0], whole.size());
        whole.resize(length);

        return whole;
    }

    /**
     * @brief Writes a value as text into a buffer, as snprintf does.
     *
     * @param[in]  _var       the variable.
     * @param[in]  _scope     the scope.
     * @param[in]  _quoted    whether a TEXT value is quoted where it must be.
     * @param[out] _buffer    where.
     * @param[in]  _capacity  its size.
     * @return the whole text's length, or (std::size_t)-1.
     */
    std::size_t write(
        uint32_t    _var,
        uint32_t    _scope,
        bool        _quoted,
        char*       _buffer,
        std::size_t _capacity
    ) const
    {
        return _quoted
                   ? d_option_env_format_quoted(m_env, _scope, _var, _buffer,
                                                _capacity)
                   : d_option_env_format(m_env, _scope, _var, _buffer,
                                         _capacity);
    }

    /**
     * @brief A variable's id from a counted name.
     *
     * @param[in] _name  the name.
     * @param[in] _size  its length.
     * @return the id, or D_OPTION_ENV_NONE.
     */
    uint32_t find_n(
        const char* _name,
        std::size_t _size
    ) const
    {
        const struct dj_option_result found = d_option_env_find(m_env,
                                                                _name,
                                                                _size);

        return found.is_ok ? found.payload.value : D_OPTION_ENV_NONE;
    }
};


//==============================================================================
// 2.  A FRAME, OWNED
//==============================================================================


// 2.1    DOptionEnv
//------------------------------------------------------------------------------
// DOptionEnv
//   class template: a frame and its storage -- room for Bindings bindings,
// Vars declarations and Text bytes of arena, as D_OPTION_ENV_DECLARE sizes
// them -- made at construction, a root or a child of another frame, and let
// go of as a child when it goes. Not copied: the frame points into the
// storage. status() says whether construction succeeded.
template<std::size_t Bindings,
         std::size_t Vars = 0u,
         std::size_t Text = 0u>
class DOptionEnv : public DOptionEnvView
{
public:
    /**
     * @brief A root frame, empty.
     */
    DOptionEnv()
        : DOptionEnvView(&m_frame)
    {
        init(D_NULLPTR);
    }

    /**
     * @brief A frame in a parent: it sees the parent's declarations and
     *        bindings through its own.
     *
     * @param[in,out] _parent  the parent; it must outlive the frame.
     */
    explicit
    DOptionEnv(
        struct dj_option_env& _parent
    )
        : DOptionEnvView(&m_frame)
    {
        init(&_parent);
    }

    /**
     * @brief Lets go of the parent, which then counts this frame a child no
     *        more.
     */
    ~DOptionEnv()
    {
        d_option_env_detach(&m_frame);
    }

    /**
     * @brief Declares one variable.
     *
     * @param[in] _var  the declaration; borrowed, so it must outlive the
     *                  frame.
     * @return its id, or D_OPTION_ENV_NONE with status() saying why.
     */
    uint32_t declare(
        const struct dj_option_env_var& _var
    )
    {
        const struct dj_option_result declared = d_option_env_declare(&m_frame,
                                                                      &_var);

        return keep(declared) ? declared.payload.value : D_OPTION_ENV_NONE;
    }

    /**
     * @brief Declares a schema, its ids consecutive.
     *
     * @param[in] _schema  the table; borrowed.
     * @param[in] _count   its rows.
     * @return the first id, or D_OPTION_ENV_NONE with status() saying why.
     */
    uint32_t declare_all(
        const struct dj_option_env_var* _schema,
        uint32_t                        _count
    )
    {
        const struct dj_option_result declared =
            d_option_env_declare_all(&m_frame, _schema, _count);

        return keep(declared) ? declared.payload.value : D_OPTION_ENV_NONE;
    }

    /**
     * @brief Names the frame's scopes: scope s, from 1, is _names[s - 1].
     *
     * @param[in] _names  the names; borrowed.
     * @param[in] _count  how many.
     * @return true when they were named.
     */
    bool set_scopes(
        const char* const* _names,
        uint32_t           _count
    )
    {
        return keep(d_option_env_set_scopes(&m_frame, _names, _count));
    }

    /**
     * @brief Names the frame, as a djin is named.
     *
     * @param[in] _name  an identifier; borrowed.
     * @return true when it was named.
     */
    bool set_name(
        const char* _name
    )
    {
        return keep(d_option_env_set_name(&m_frame, _name));
    }

    // every binding of this frame removed
    void clear()
    {
        d_option_env_clear(&m_frame);

        return;
    }

private:
    // non-copyable: the frame points into the storage
    DOptionEnv(const DOptionEnv&);
    DOptionEnv& operator=(const DOptionEnv&);

    /**
     * @brief Makes the frame over the storage, under a parent if one is
     *        given.
     *
     * @param[in,out] _parent  the parent, or null.
     */
    void init(
        struct dj_option_env* _parent
    )
    {
        if ( (keep(D_OPTION_ENV_VIEW(&m_frame, m_storage))) &&
             (_parent != D_NULLPTR) )
        {
            (void)keep(d_option_env_set_parent(&m_frame, _parent));
        }

        return;
    }

    D_OPTION_ENV_DECLARE(storage_type,
                         Bindings,
                         Vars,
                         Text);

    struct dj_option_env m_frame;
    storage_type         m_storage;
};


//==============================================================================
// 3.  A SCOPED OVERRIDE
//==============================================================================


// 3.1    DOptionEnvOverride
//------------------------------------------------------------------------------
// DOptionEnvOverride
//   class: a variable forced from the OVERRIDE source, by name and from text,
// for the object's life -- nothing a file, the environment or the command
// line says outranks it -- and given back when it ends: the OVERRIDE binding
// the frame had in that scope before, restored as text, or none. Overrides of
// one variable end in the reverse order they began, as scopes do. ok() says
// whether the override holds; one that failed changes nothing.
class DOptionEnvOverride
{
public:
    /**
     * @brief Forces a variable.
     *
     * @param[in,out] _env    the frame to bind in; it must outlive the
     *                        override.
     * @param[in]     _name   the name, with an optional "scope." before it.
     * @param[in]     _value  the text, read as the variable's kind reads it.
     */
    DOptionEnvOverride(
        struct dj_option_env& _env,
        const char*           _name,
        const char*           _value
    )
        : m_env(&_env),
          m_name((_name != D_NULLPTR) ? _name : ""),
          m_before(),
          m_var(D_OPTION_ENV_NONE),
          m_scope(D_OPTION_ENV_GLOBAL),
          m_status(static_cast<int32_t>(D_OPTION_STATUS_INVALID_ARGUMENT)),
          m_had(false)
    {
        if ( (_name == D_NULLPTR) ||
             (_value == D_NULLPTR) )
        {
            return;
        }

        struct dj_option_result result = d_option_env_resolve(m_env,
                                                              m_name.data(),
                                                              m_name.size(),
                                                              &m_scope);

        if (!result.is_ok)
        {
            m_status = d_option_result_status(&result);

            return;
        }

        const uint32_t var = result.payload.value;

        // the frame's own OVERRIDE binding in the scope is the one in force
        // there whenever it has one: nothing outranks it
        struct dj_option_env_layer first;
        const std::size_t          layers = d_option_env_layers(m_env,
                                                                m_scope,
                                                                var,
                                                                &first,
                                                                1u);

        m_had = ( (layers != static_cast<std::size_t>(-1)) &&
                  (layers >= 1u)                           &&
                  (first.frame == m_env)                   &&
                  (first.scope == m_scope)                 &&
                  (first.value.source ==
                   static_cast<int32_t>(D_OPTION_ENV_SOURCE_OVERRIDE)) );

        if (m_had)
        {
            m_before = DOptionEnvView(m_env).format(var, m_scope);
        }

        result   = d_option_env_assign_from(m_env,
                                            m_name.data(),
                                            m_name.size(),
                                            _value,
                                            std::strlen(_value),
                                            D_OPTION_ENV_SOURCE_OVERRIDE);
        m_status = d_option_result_status(&result);
        m_var    = result.is_ok ? var : D_OPTION_ENV_NONE;
    }

    /**
     * @brief Gives the variable back.
     */
    ~DOptionEnvOverride()
    {
        if (m_var == D_OPTION_ENV_NONE)
        {
            return;
        }

        if (m_had)
        {
            (void)d_option_env_assign_from(m_env,
                                           m_name.data(),
                                           m_name.size(),
                                           m_before.data(),
                                           m_before.size(),
                                           D_OPTION_ENV_SOURCE_OVERRIDE);
        }
        else
        {
            (void)d_option_env_unset_from(m_env,
                                          m_scope,
                                          m_var,
                                          D_OPTION_ENV_SOURCE_OVERRIDE);
        }
    }

    // whether the override holds; the option status of making it
    D_NODISCARD
    bool ok() const
    {
        return (m_var != D_OPTION_ENV_NONE);
    }

    D_NODISCARD
    int32_t status() const
    {
        return m_status;
    }

private:
    // non-copyable: the restoring is one object's
    DOptionEnvOverride(const DOptionEnvOverride&);
    DOptionEnvOverride& operator=(const DOptionEnvOverride&);

    struct dj_option_env* m_env;
    std::string           m_name;
    std::string           m_before;  // the OVERRIDE binding's text, when
                                     // m_had
    uint32_t              m_var;     // D_OPTION_ENV_NONE: nothing to give
                                     // back
    uint32_t              m_scope;
    int32_t               m_status;
    bool                  m_had;     // the frame had its own before
};


NS_END  // djinterp


#endif  // RE_STD_HAS_INT64

#endif  // DJINTERP_OPTION_ENV_HPP
