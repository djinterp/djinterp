/*******************************************************************************
* djinterp [core]                                               mysql_common.cpp
*
* Definitions for mysql_common.hpp that are not the template's: the MariaDB
* layer's failures -- described, then thrown -- and its prepared statements'
* one instantiation.
*   mysql_common_connection's own members are defined in its header and
* instantiated by each leaf's source, mariadb.cpp and mysql.cpp, so that a
* program builds only the leaves it uses, with this source beside them.
*
*
* path:      /src/djinterp/core/db/mysql/mysql_common.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../inc/djinterp/core/db/mysql/mysql_common.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstring>  // std::strcmp
#include <string>   // std::string, std::to_string
// djinterp
#include "../../../../../inc/djinterp/c/db/mariadb/mariadb.h"  // d_mariadb_*
#include "../../../../../inc/djinterp/core/db/database_statement.hpp"
                                       // prepared_statement, failure_kind


NS_DJINTERP

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

namespace
{

/**
 * @brief Whether a result is one the layer returns itself, whose number the
 *        client keeps no text for.
 *
 * @param[in] _result  the result.
 * @return `true` for 2000 (a failure without a number), 2008 (out of memory)
 *         and 2034 (no such parameter), and for every negative D_DB_*.
 */
bool
layer_own_result(
    int _result
) noexcept
{
    return ( (_result <= 0)    ||
             (_result == 2000) ||
             (_result == 2008) ||
             (_result == 2034) );
}

/**
 * @brief The server's name, as the connection learned it.
 *
 * @param[in] _db  the connection; may be `NULL`.
 * @return "MariaDB", "MySQL", or "MySQL-family" when it is not known.
 */
const char*
server_name(
    const struct d_mariadb* _db
) noexcept
{
    // the flavor the handle read from the server's version
    switch (d_mariadb_server_flavor(_db))
    {
        case D_MARIADB_FLAVOR_MARIADB:
            return "MariaDB";

        case D_MARIADB_FLAVOR_MYSQL:
            return "MySQL";

        case D_MARIADB_FLAVOR_UNKNOWN:
        default:
            return "MySQL-family";
    }
}

}  // namespace

//==============================================================================
// III. MYSQL-FAMILY COMMON CONNECTION TEMPLATE
//==============================================================================

/*
mariadb_layer::describe
  The connection's text and SQLSTATE describe the latest failure the client
met; for a result of the layer's own they may describe an older one, so the
result's own description stands in, and no SQLSTATE is named. HY000, the
general error, says nothing a message does not.
*/
std::string
mariadb_layer::describe(
    handle_type _db,
    const char* _operation,
    int         _result
)
{
    const bool  own  = layer_own_result(_result);
    const char* said = ( (!own) &&
                         (_db) ) ? d_mariadb_error_message(_db)
                                 : nullptr;

    // the result's words where the connection's are not about it
    if ( (!said) ||
         (said[0] == '\0') )
    {
        said = d_mariadb_result_message(_result);
    }

    std::string message = std::string(server_name(_db)) + " " + _operation +
                          ": " + said + " (" + std::to_string(_result);
    const char* const state = ( (!own) &&
                                (_db) ) ? d_mariadb_sqlstate(_db)
                                        : nullptr;

    // the SQLSTATE, where it says more than the number
    if ( (state)                              &&
         (state[0] != '\0')                   &&
         (std::strcmp(state, "00000") != 0)   &&
         (std::strcmp(state, "HY000") != 0) )
    {
        message += std::string(", SQLSTATE ") + state;
    }

    message += ")";

    return message;
}

void
mariadb_layer::raise_as(
    failure_kind       _kind,
    const std::string& _message,
    int                _result
)
{
    // a lost connection is the connection's failure, whatever was running
    if (connection_lost(_result))
    {
        throw connection_exception(_message, _result);
    }

    switch (_kind)
    {
        case failure_kind::connection:
            throw connection_exception(_message, _result);

        case failure_kind::transaction:
            throw transaction_exception(_message, _result);

        case failure_kind::query:
        default:
            throw query_exception(_message, _result);
    }
}

void
mariadb_layer::raise(
    handle_type  _db,
    failure_kind _kind,
    const char*  _operation,
    int          _result
)
{
    raise_as(_kind, describe(_db, _operation, _result), _result);
}

template class prepared_statement<mariadb_layer>;

NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
