/*******************************************************************************
* djinterp [c]                                                  mongodb_common.c
*
* Implementation of mongodb_common.h.
*   Results are packed so that reading one needs no driver: the functions of
* section 1 work in both builds, over the driver or as stubs. What a server
* can do is decided by what the handle learned of it when it was made -- its
* version and its deployment -- and, for the one feature that needs a call
* the driver gained late, by the driver's headers: no request is sent to
* answer d_mongodb_supports.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/
// D_INTERNAL_MONGODB_SOURCE
//   macro: this file is one of the MongoDB layer's own sources, for which
// cfg_mongodb_common.h has env include mongoc.h; it precedes every include.
#define D_INTERNAL_MONGODB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_common.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX
#include <stddef.h>  // NULL
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // d_db_status_message
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // D_INTERNAL_MONGODB
#include "./mongodb_internal.h"  // mongoc.h, the floor, the plumbing


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// a packed code reaches 2^30 + 0x2FF * 65536 + 0xFFFF, which an int of 32
// bits holds
D_STATIC_ASSERT( (INT_MAX >= 2147483647),
                 "a packed MongoDB code needs an int of 32 bits" );


//==============================================================================
// 1.  RESULTS
//==============================================================================

// 1.1    Reading a code
//------------------------------------------------------------------------------
/*
d_mongodb_result_is_client
  Everything the server reports is below 2^30; everything packed, at or
above it.
*/
bool d_mongodb_result_is_client(
    int _result
)
{
    return (_result >= D_MONGODB_CLIENT_ERRORS);
}

/*
d_mongodb_result_domain
  The bits above the code's 16, past the base.
*/
unsigned d_mongodb_result_domain(
    int _result
)
{
    return (d_mongodb_result_is_client(_result))
               ? ((unsigned)(_result - D_MONGODB_CLIENT_ERRORS) >> 16)
               : 0u;
}

/*
d_mongodb_result_code
  A packed code's low 16 bits; a server's code as it is; nothing for 0 and
db_common.h's negatives.
*/
unsigned d_mongodb_result_code(
    int _result
)
{
    // a packed code's own
    if (d_mongodb_result_is_client(_result))
    {
        return (unsigned)(_result - D_MONGODB_CLIENT_ERRORS) & 0xFFFFu;
    }

    return (_result > 0) ? (unsigned)_result : 0u;
}

/*
d_mongodb_result_is_unreachable
  The two domains alone, whatever the code within them.
*/
bool d_mongodb_result_is_unreachable(
    int _result
)
{
    const unsigned domain = d_mongodb_result_domain(_result);

    return ( (domain == D_MONGODB_DOMAIN_STREAM) ||
             (domain == D_MONGODB_DOMAIN_SERVER_SELECTION) );
}

// 1.2    Result messages
//------------------------------------------------------------------------------
/*
d_mongodb_result_message
  db_common.h's words for its own values -- save D_DB_UNSUPPORTED in a build
without the driver, which the build explains -- the layer's for the codes it
packs itself, and a pointer to d_mongodb_error_message for the server's and
the driver's, whose words only the handle keeps.
*/
const char* d_mongodb_result_message(
    int _result
)
{
    // without the driver, the build is what cannot
    if ( (_result == D_DB_UNSUPPORTED) &&
         (D_INTERNAL_MONGODB == 0) )
    {
        return "this build of the MongoDB layer has no driver";
    }

    // db_common.h's values, in its words
    if (_result <= 0)
    {
        const char* const message = d_db_status_message(_result);

        return (message) ? message : "unknown result";
    }

    // the layer's own
    switch (_result)
    {
        case D_MONGODB_LAYER_NO_MEMORY:
            return "the layer could not allocate what it returns";

        case D_MONGODB_LAYER_TOO_LARGE:
            return "a document, or a file's bytes, too large to hand over";

        case D_MONGODB_LAYER_UNDESCRIBED:
            return "the driver reported a failure, and described none";

        default:
            break;
    }

    // the server's, the driver's, libbson's
    if (!d_mongodb_result_is_client(_result))
    {
        return "a failure the server reported, which "
               "d_mongodb_error_message describes";
    }

    return (d_mongodb_result_domain(_result) >= D_MONGODB_DOMAIN_LAYER)
               ? "unknown result"
               : ( (d_mongodb_result_domain(_result) >= D_MONGODB_DOMAIN_BSON)
                       ? "a failure of libbson's, which "
                         "d_mongodb_error_message describes"
                       : "a failure of the driver's, which "
                         "d_mongodb_error_message describes" );
}


#if (D_INTERNAL_MONGODB == 1)

// the driver's domains, as mongodb_common.h names them without its header
D_STATIC_ASSERT( (D_MONGODB_DOMAIN_STREAM == (unsigned)MONGOC_ERROR_STREAM),
                 "D_MONGODB_DOMAIN_STREAM is the driver's stream domain" );
D_STATIC_ASSERT( (D_MONGODB_DOMAIN_SERVER_SELECTION ==
                  (unsigned)MONGOC_ERROR_SERVER_SELECTION),
                 "D_MONGODB_DOMAIN_SERVER_SELECTION is the driver's server "
                 "selection domain" );

//==============================================================================
// 2.  LIBRARY
//==============================================================================

// 2.1    Driver
//------------------------------------------------------------------------------
/*
d_mongodb_client_version
  The loaded library's own string, which may be newer than the headers.
*/
const char* d_mongodb_client_version(void)
{
    return mongoc_get_version();
}

/*
d_mongodb_client_version_number
  Read from the loaded library, as its string is.
*/
unsigned long d_mongodb_client_version_number(void)
{
    return ((unsigned long)mongoc_get_major_version() * 10000ul) +
           ((unsigned long)mongoc_get_minor_version() * 100ul) +
           (unsigned long)mongoc_get_micro_version();
}

/*
d_mongodb_bson_version
  libbson's, loaded.
*/
const char* d_mongodb_bson_version(void)
{
    return bson_get_version();
}

// 2.2    The driver's log
//------------------------------------------------------------------------------
/*
d_mongodb_set_log_level
  The level is checked here; mongodb_internal.c installs it.
*/
int d_mongodb_set_log_level(
    int _level
)
{
    // the driver's levels, or none
    if ( (_level < -1) ||
         (_level > 6) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return (d_internal_mongodb_log_set(_level)) ? 0
                                                : D_MONGODB_LAYER_NO_MEMORY;
}

/*
d_mongodb_get_log_level
  mongodb_internal.c keeps it.
*/
int d_mongodb_get_log_level(void)
{
    return d_internal_mongodb_log_get();
}


//==============================================================================
// 3.  SERVERS
//==============================================================================

// 3.1    Versions
//------------------------------------------------------------------------------
/*
d_mongodb_server_version
  buildInfo's string, kept when the handle was made.
*/
const char* d_mongodb_server_version(
    const struct dj_mongodb* _db
)
{
    return (_db) ? _db->release : NULL;
}

/*
d_mongodb_server_version_number
  Kept with the string.
*/
unsigned long d_mongodb_server_version_number(
    const struct dj_mongodb* _db
)
{
    return (_db) ? _db->version : 0ul;
}

/*
d_mongodb_server_at_least
  The version given is encoded as the handle's is, a part past 99 standing
for more than any server has.
*/
bool d_mongodb_server_at_least(
    const struct dj_mongodb* _db,
    unsigned                 _major,
    unsigned                 _minor,
    unsigned                 _patch
)
{
    const unsigned long wanted = ((unsigned long)_major * 10000ul) +
                                 ((unsigned long)_minor * 100ul) +
                                 (unsigned long)_patch;

    return ( (_db) &&
             (_db->version >= wanted) );
}

/*
d_mongodb_server_wire_version
  hello's maxWireVersion.
*/
int d_mongodb_server_wire_version(
    const struct dj_mongodb* _db
)
{
    return (_db) ? _db->wire : 0;
}

// 3.2    Deployment
//------------------------------------------------------------------------------
/*
d_mongodb_server_topology
  What hello said when the handle was made.
*/
enum dj_mongodb_topology
d_mongodb_server_topology(
    const struct dj_mongodb* _db
)
{
    return (_db) ? _db->topology : D_MONGODB_TOPOLOGY_UNKNOWN;
}

/*
d_mongodb_server_replica_set
  hello's setName.
*/
const char* d_mongodb_server_replica_set(
    const struct dj_mongodb* _db
)
{
    return (_db) ? _db->replica_set : NULL;
}

// 3.3    Features
//------------------------------------------------------------------------------
/**
 * @brief Whether a handle reached more than one server's worth: a replica
 *        set, a sharded cluster or a load balancer.
 *
 * @param[in] _db  the handle.
 * @return `true` for any of the three.
 */
static
bool d_internal_mongodb_not_standalone(
    const struct dj_mongodb* _db
)
{
    return ( (_db->topology == D_MONGODB_TOPOLOGY_REPLICA_SET) ||
             (_db->topology == D_MONGODB_TOPOLOGY_SHARDED)     ||
             (_db->topology == D_MONGODB_TOPOLOGY_LOAD_BALANCED) );
}

/*
d_mongodb_supports
  A transaction needs a replica set from 4.0, a sharded cluster from 4.2, or
a load balancer, which only 5.0 and later sit behind; a change stream, a
snapshot read and pre- and post-images need an oplog, which no standalone
server keeps. The client's bulkWrite needs the server's 8.0 and the
driver's 1.28 headers, which declare its calls.
*/
bool
d_mongodb_supports(
    const struct dj_mongodb* _db,
    enum dj_mongodb_feature  _feature
)
{
    // no handle, no server
    if (!_db)
    {
        return false;
    }

    const unsigned long version = _db->version;

    switch (_feature)
    {
        case D_MONGODB_FEATURE_TRANSACTIONS:
            return ( ( (_db->topology == D_MONGODB_TOPOLOGY_REPLICA_SET) &&
                       (version >= 40000ul) )                           ||
                     ( (_db->topology == D_MONGODB_TOPOLOGY_SHARDED) &&
                       (version >= 40200ul) )                           ||
                     (_db->topology == D_MONGODB_TOPOLOGY_LOAD_BALANCED) );

        case D_MONGODB_FEATURE_CHANGE_STREAMS:
            return d_internal_mongodb_not_standalone(_db);

        case D_MONGODB_FEATURE_TIME_SERIES:
            return (version >= 50000ul);

        case D_MONGODB_FEATURE_CLUSTERED:
            return (version >= 50300ul);

        case D_MONGODB_FEATURE_STABLE_API:
            return (version >= 50000ul);

        case D_MONGODB_FEATURE_SNAPSHOT_READS:
            return ( (version >= 50000ul) &&
                     (d_internal_mongodb_not_standalone(_db)) );

        case D_MONGODB_FEATURE_PRE_POST_IMAGES:
            return ( (version >= 60000ul) &&
                     (d_internal_mongodb_not_standalone(_db)) );

        case D_MONGODB_FEATURE_BULK_WRITE:
            return ( (D_ENV_MONGO_HAS_CLIENT_BULK_WRITE) &&
                     (version >= 80000ul) );

        case D_MONGODB_FEATURE_HIDDEN_INDEXES:
            return (version >= 40400ul);

        case D_MONGODB_FEATURE_WILDCARD_INDEXES:
            return (version >= 40200ul);

        default:
            return false;
    }
}


//==============================================================================
// 4.  HANDLES
//==============================================================================

// 4.1    The driver's client
//------------------------------------------------------------------------------
/*
d_mongodb_native
  The client itself; the layer keeps it.
*/
void* d_mongodb_native(
    struct dj_mongodb* _db
)
{
    return (_db) ? (void*)_db->client : NULL;
}

// 4.2    Errors
//------------------------------------------------------------------------------
/*
d_mongodb_error_code
  As recorded.
*/
int d_mongodb_error_code(
    const struct dj_mongodb* _db
)
{
    return (_db) ? _db->error : 0;
}

/*
d_mongodb_error_message
  As recorded, cut at a character's start where it was long.
*/
const char* d_mongodb_error_message(
    const struct dj_mongodb* _db
)
{
    return (_db) ? _db->message : "";
}

/*
d_mongodb_error_name
  The server's codeName, or the layer's for a code it answered itself.
*/
const char* d_mongodb_error_name(
    const struct dj_mongodb* _db
)
{
    return (_db) ? _db->error_name : "";
}

/*
d_mongodb_error_has_label
  The labels are in the reply's errorLabels, where the server and the driver
both put them.
*/
bool d_mongodb_error_has_label(
    const struct dj_mongodb* _db,
    const char*              _label
)
{
    return ( (_db)        &&
             (_label)     &&
             (_db->reply) &&
             (mongoc_error_has_label(_db->reply, _label)) );
}

/*
d_mongodb_error_reply
  Written in the handle's format, canonical for BSON; nothing is recorded,
since the handle is read only and its last failure is what is asked for.
*/
int d_mongodb_error_reply(
    const struct dj_mongodb* _db,
    char**                   _reply
)
{
    // a handle, and somewhere to write
    if ( (!_db) ||
         (!_reply) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // no reply kept
    if (!_db->reply)
    {
        return D_DB_NOT_FOUND;
    }

    char* const text = d_internal_mongodb_json(_db->reply, _db->format);

    // no room for it
    if (!text)
    {
        return D_MONGODB_LAYER_NO_MEMORY;
    }

    *_reply = text;

    return 0;
}

#else

//==============================================================================
// WITHOUT THE DRIVER
//==============================================================================
// Built as stubs (D_CFG_MONGODB 0), each function reports what a build
// without the driver can: D_DB_UNSUPPORTED, or false, 0, "" or NULL where it
// cannot fail. No handle can exist, so none is read.

/*
d_mongodb_client_version
  Without the driver, "".
*/
const char* d_mongodb_client_version(void)
{
    return "";
}

/*
d_mongodb_client_version_number
  Without the driver, 0.
*/
unsigned long d_mongodb_client_version_number(void)
{
    return 0ul;
}

/*
d_mongodb_bson_version
  Without libbson, "".
*/
const char* d_mongodb_bson_version(void)
{
    return "";
}

/*
d_mongodb_set_log_level
  Without the driver there is no log: D_DB_UNSUPPORTED, whatever the level.
*/
int d_mongodb_set_log_level(
    int _level
)
{
    (void)_level;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_get_log_level
  Without the driver, -1.
*/
int d_mongodb_get_log_level(void)
{
    return -1;
}

/*
d_mongodb_server_version
  Without the driver, NULL.
*/
const char* d_mongodb_server_version(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return NULL;
}

/*
d_mongodb_server_version_number
  Without the driver, 0.
*/
unsigned long d_mongodb_server_version_number(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return 0ul;
}

/*
d_mongodb_server_at_least
  Without the driver, false.
*/
bool d_mongodb_server_at_least(
    const struct dj_mongodb* _db,
    unsigned                 _major,
    unsigned                 _minor,
    unsigned                 _patch
)
{
    (void)_db;
    (void)_major;
    (void)_minor;
    (void)_patch;

    return false;
}

/*
d_mongodb_server_wire_version
  Without the driver, 0.
*/
int d_mongodb_server_wire_version(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return 0;
}

/*
d_mongodb_server_topology
  Without the driver, D_MONGODB_TOPOLOGY_UNKNOWN.
*/
enum dj_mongodb_topology
d_mongodb_server_topology(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return D_MONGODB_TOPOLOGY_UNKNOWN;
}

/*
d_mongodb_server_replica_set
  Without the driver, NULL.
*/
const char* d_mongodb_server_replica_set(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return NULL;
}

/*
d_mongodb_supports
  Without the driver, false.
*/
bool
d_mongodb_supports(
    const struct dj_mongodb* _db,
    enum dj_mongodb_feature  _feature
)
{
    (void)_db;
    (void)_feature;

    return false;
}

/*
d_mongodb_native
  Without the driver, NULL.
*/
void* d_mongodb_native(
    struct dj_mongodb* _db
)
{
    (void)_db;

    return NULL;
}

/*
d_mongodb_error_code
  Without the driver, 0.
*/
int d_mongodb_error_code(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return 0;
}

/*
d_mongodb_error_message
  Without the driver, "".
*/
const char* d_mongodb_error_message(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return "";
}

/*
d_mongodb_error_name
  Without the driver, "".
*/
const char* d_mongodb_error_name(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return "";
}

/*
d_mongodb_error_has_label
  Without the driver, false.
*/
bool d_mongodb_error_has_label(
    const struct dj_mongodb* _db,
    const char*              _label
)
{
    (void)_db;
    (void)_label;

    return false;
}

/*
d_mongodb_error_reply
  Without the driver, D_DB_UNSUPPORTED, the reply untouched.
*/
int d_mongodb_error_reply(
    const struct dj_mongodb* _db,
    char**                   _reply
)
{
    (void)_db;
    (void)_reply;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MONGODB == 1
