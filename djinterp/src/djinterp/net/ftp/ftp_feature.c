/*******************************************************************************
* djinterp [net]                                                   ftp_feature.c
*
* Implementation of the feature negotiation declared in ftp_feature.h.
*
*
* path:      /src/djinterp/net/ftp/ftp_feature.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_feature.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint32_t
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_span
#include "./ftp_internal.h"                               // shared helpers


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
    struct d_ftp_span _list,
    const char*       _word
)
{
    size_t position = 0;

    // token by token
    while (position < _list.length)
    {
        size_t end = position;

        // the token runs to the next separator
        while ( (end < _list.length)                         &&
                (_list.data[end] != ';')                     &&
                (_list.data[end] != ',')                     &&
                (!d_ftp_internal_is_blank(_list.data[end])) )
        {
            end++;
        }

        const bool match = d_ftp_internal_starts_nocase(_list.data + position,
                                                        end - position,
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
d_ftp_internal_feature
  File-local: records one FEAT line. Names that stand alone come from the
table; the five that carry parameters are read here.
*/
D_STATIC void
d_ftp_internal_feature(
    struct d_ftp_span      _name,
    struct d_ftp_span      _parameters,
    struct d_ftp_features* _features
)
{
    const size_t count = sizeof(FEATURES) / sizeof(FEATURES[0]);

    // names whose presence is the capability
    for (size_t index = 0; index < count; index++)
    {
        if (d_ftp_internal_equals_nocase(_name.data,
                                         _name.length,
                                         FEATURES[index].name))
        {
            _features->flags |= FEATURES[index].flag;

            return;
        }
    }

    // AUTH lists mechanisms: "TLS", "SSL", or variants such as "TLS-C"
    if (d_ftp_internal_equals_nocase(_name.data,
                                     _name.length,
                                     "AUTH"))
    {
        if (d_ftp_internal_has_token(_parameters,
                                     "TLS"))
        {
            _features->flags |= D_FTP_FEATURE_AUTH_TLS;
        }

        if (d_ftp_internal_has_token(_parameters,
                                     "SSL"))
        {
            _features->flags |= D_FTP_FEATURE_AUTH_SSL;
        }

        return;
    }

    // "REST STREAM": restarts in stream mode
    if ( (d_ftp_internal_equals_nocase(_name.data,
                                       _name.length,
                                       "REST")) &&
         (d_ftp_internal_has_token(_parameters,
                                   "STREAM")) )
    {
        _features->flags |= D_FTP_FEATURE_REST_STREAM;

        return;
    }

    // "MODE Z": deflate transfers
    if ( (d_ftp_internal_equals_nocase(_name.data,
                                       _name.length,
                                       "MODE")) &&
         (d_ftp_internal_has_token(_parameters,
                                   "Z")) )
    {
        _features->flags |= D_FTP_FEATURE_MODE_Z;

        return;
    }

    // MLST lists the facts the server can report
    if (d_ftp_internal_equals_nocase(_name.data,
                                     _name.length,
                                     "MLST"))
    {
        _features->flags      |= D_FTP_FEATURE_MLST;
        _features->mlst_facts  = _parameters;

        return;
    }

    // LANG lists the languages the server offers
    if (d_ftp_internal_equals_nocase(_name.data,
                                     _name.length,
                                     "LANG"))
    {
        _features->flags     |= D_FTP_FEATURE_LANG;
        _features->languages  = _parameters;
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
    struct d_ftp_span     cursor   = { _text, _length };
    struct d_ftp_span     line     = { NULL, 0u };

    // one feature per line
    while (d_ftp_span_next_line(&cursor,
                                &line))
    {
        const struct d_ftp_span entry = d_ftp_internal_trim(line.data,
                                                            line.length);
        size_t                  name_length = 0;

        // the name runs to the first blank
        while ( (name_length < entry.length) &&
                (!d_ftp_internal_is_blank(entry.data[name_length])) )
        {
            name_length++;
        }

        const struct d_ftp_span name       = { entry.data, name_length };
        const struct d_ftp_span parameters = d_ftp_internal_trim(
                                                 entry.data + name_length,
                                                 entry.length - name_length);

        d_ftp_internal_feature(name,
                               parameters,
                               &features);
    }

    *_out = features;

    return D_FTP_OK;
}
