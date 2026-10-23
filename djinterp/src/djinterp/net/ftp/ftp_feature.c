/*******************************************************************************
* djinterp [net]                                                   ftp_feature.c
*
* Implementation of the feature negotiation declared in ftp_feature.h.
*
*
* path:      /src/djinterp/net/ftp/ftp_feature.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_feature.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"             // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"     // d_pack_text
#include "../../../../inc/djinterp/net/net_common.h"  // d_net_ascii_*
#include "../../../../inc/djinterp/net/codec/codec_line.h"   // d_net_line_next
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"     // d_ftp_error
#include "./ftp_internal.h"                                  // shared helpers
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_ftp_internal_feature_name
//   struct: one row of the table of FEAT names that stand alone.
struct d_ftp_internal_feature_name
{
    const char* name;  // the feature as FEAT lists it
    uint32_t    flag;  // its D_FTP_FEATURE_* bit
};

// FEATURES
//   constant: the FEAT names whose presence alone is the capability. AUTH,
// REST, MODE, MLST, and LANG carry parameters and are read separately.
static const struct d_ftp_internal_feature_name FEATURES[] =
{
    { "EPRT", D_FTP_FEATURE_EPRT },
    { "EPSV", D_FTP_FEATURE_EPSV },
    { "MDTM", D_FTP_FEATURE_MDTM },
    { "SIZE", D_FTP_FEATURE_SIZE },
    { "MLSD", D_FTP_FEATURE_MLST },
    { "TVFS", D_FTP_FEATURE_TVFS },
    { "UTF8", D_FTP_FEATURE_UTF8 },
    { "PBSZ", D_FTP_FEATURE_PBSZ },
    { "PROT", D_FTP_FEATURE_PROT },
    { "CCC",  D_FTP_FEATURE_CCC  },
    { "HOST", D_FTP_FEATURE_HOST },
    { "MFMT", D_FTP_FEATURE_MFMT },
    { "PRET", D_FTP_FEATURE_PRET },
    { "CLNT", D_FTP_FEATURE_CLNT }
};

//==============================================================================
// 2.  FEATURES
//==============================================================================

/*
d_ftp_internal_has_token
  File-local: reports whether a FEAT parameter list holds a token beginning
with `_word`, case-insensitively. Tokens split at ';', ',', and blanks,
covering "TLS;SSL", "TLS SSL", and "TLS-C, TLS-P" alike.
*/
D_STATIC bool
d_ftp_internal_has_token(
    struct d_pack_text _list,
    const char*        _word
)
{
    size_t position = 0;

    // token by token
    while (position < _list.length)
    {
        size_t end = position;

        // the token runs to the next separator
        while ( (end < _list.length)                     &&
                (_list.data[end] != ';')                 &&
                (_list.data[end] != ',')                 &&
                (!d_net_ascii_is_blank(_list.data[end])) )
        {
            end++;
        }

        const struct d_pack_text token = { _list.data + position,
                                           end - position };
        const bool               match = d_net_ascii_istarts(token,
                                                             _word);

        // an empty token matches nothing
        if ( (end > position) &&
             (match) )
        {
            return true;
        }

        position = end + 1u;
    }

    return false;
}

/*
d_ftp_internal_feature_tokens
  File-local: records a feature whose parameters name what it offers: AUTH's
mechanisms -- "TLS", "SSL", or variants such as "TLS-C" -- "REST STREAM",
and "MODE Z". Reports whether `_name` was one of them.
*/
D_STATIC bool
d_ftp_internal_feature_tokens(
    struct d_pack_text     _name,
    struct d_pack_text     _parameters,
    struct d_ftp_features* _features
)
{
    // AUTH lists mechanisms
    if (d_net_ascii_ieq_cstr(_name,
                             "AUTH"))
    {
        _features->flags |= (d_ftp_internal_has_token(_parameters,
                                                      "TLS"))
                            ? (uint32_t)D_FTP_FEATURE_AUTH_TLS : 0u;
        _features->flags |= (d_ftp_internal_has_token(_parameters,
                                                      "SSL"))
                            ? (uint32_t)D_FTP_FEATURE_AUTH_SSL : 0u;

        return true;
    }

    // "REST STREAM": restarts in stream mode
    if (d_net_ascii_ieq_cstr(_name,
                             "REST"))
    {
        _features->flags |= (d_ftp_internal_has_token(_parameters,
                                                      "STREAM"))
                            ? (uint32_t)D_FTP_FEATURE_REST_STREAM : 0u;

        return true;
    }

    // "MODE Z": deflate transfers
    if (d_net_ascii_ieq_cstr(_name,
                             "MODE"))
    {
        _features->flags |= (d_ftp_internal_has_token(_parameters,
                                                      "Z"))
                            ? (uint32_t)D_FTP_FEATURE_MODE_Z : 0u;

        return true;
    }

    return false;
}

/*
d_ftp_internal_feature_lists
  File-local: records a feature whose parameters are a list kept for the
caller: MLST's facts and LANG's languages.
*/
D_STATIC void
d_ftp_internal_feature_lists(
    struct d_pack_text     _name,
    struct d_pack_text     _parameters,
    struct d_ftp_features* _features
)
{
    // MLST lists the facts the server can report
    if (d_net_ascii_ieq_cstr(_name,
                             "MLST"))
    {
        _features->flags      |= D_FTP_FEATURE_MLST;
        _features->mlst_facts  = _parameters;
    }
    else if (d_net_ascii_ieq_cstr(_name,
                                  "LANG"))
    {
        _features->flags     |= D_FTP_FEATURE_LANG;
        _features->languages  = _parameters;
    }

    return;
}

/*
d_ftp_internal_feature
  File-local: records one FEAT line. Names that stand alone come from the
table; the five that carry parameters are read by the helpers above.
*/
D_STATIC void
d_ftp_internal_feature(
    struct d_pack_text     _name,
    struct d_pack_text     _parameters,
    struct d_ftp_features* _features
)
{
    const size_t count = sizeof(FEATURES) / sizeof(FEATURES[0]);

    // names whose presence is the capability
    for (size_t index = 0u; index < count; index++)
    {
        if (d_net_ascii_ieq_cstr(_name,
                                 FEATURES[index].name))
        {
            _features->flags |= FEATURES[index].flag;

            return;
        }
    }

    // names whose parameters say what is offered, else lists kept whole
    if (!d_ftp_internal_feature_tokens(_name,
                                       _parameters,
                                       _features))
    {
        d_ftp_internal_feature_lists(_name,
                                     _parameters,
                                     _features);
    }

    return;
}

/*
d_ftp_features_parse
  Every line is trimmed and split at its first blank into a name and its
parameters; the reply's own "Features:" and "End" lines match no name.
*/
enum d_ftp_error
d_ftp_features_parse(
    const char*            _text,
    size_t                 _length,
    struct d_ftp_features* _out
)
{
    // parameter validation
    if ( (!_text) ||
         (!_out) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    struct d_ftp_features features =
    {
        D_FTP_FEATURE_NONE,
        { NULL, 0u },
        { NULL, 0u }
    };
    struct d_pack_text    cursor   = { _text, _length };
    struct d_pack_text    line     = { NULL, 0u };

    // one feature per line
    while (d_net_line_next(&cursor,
                           &line))
    {
        const struct d_pack_text entry = d_ftp_internal_trim(line.data,
                                                             line.length);
        size_t                   name_length = 0;

        // the name runs to the first blank
        while ( (name_length < entry.length) &&
                (!d_net_ascii_is_blank(entry.data[name_length])) )
        {
            name_length++;
        }

        const struct d_pack_text name       = { entry.data, name_length };
        const struct d_pack_text parameters = d_ftp_internal_trim(
                                                  entry.data + name_length,
                                                  entry.length - name_length);

        d_ftp_internal_feature(name,
                               parameters,
                               &features);
    }

    *_out = features;

    return D_FTP_OK;
}
