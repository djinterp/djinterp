/*******************************************************************************
* djinterp [net]                                               ssh_known_hosts.c
*
*   Definitions for ssh_known_hosts.h: matching lines against a host --
* glob patterns, negation, [host]:port names, hashed entries, and
* markers -- and recording new ones.
*
*
* path:      /src/djinterp/net/ssh/ssh_known_hosts.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssh/ssh_known_hosts.h"  // corresponding header
// std
#include <errno.h>    // errno, ENOENT
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // SIZE_MAX
#include <stdio.h>    // FILE, fopen, fgets, fwrite, snprintf
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcmp, memcpy, strchr, strcmp
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../inc/djinterp/net/ssh/ssh_base64.h"  // d_ssh_base64_*
#include "../../../../inc/djinterp/net/ssh/ssh_key.h"  // d_ssh_key_type, d_ssh_random
#include "../../../../inc/djinterp/net/ssh/ssh_wire.h"  // d_ssh_name_list_contains
#include "../../../../inc/djinterp/net/ssh/ssh_internal.h"  // d_ssh_internal_*


// file-local sizes
enum
{
    D_SSH_INTERNAL_LINE_MAX  = 16384,  // longest known_hosts line
    D_SSH_INTERNAL_FIELD_MAX = 320     // a hosts field this module writes
};

// d_ssh_internal_blob_capacity
//   room for the decoded key of the longest line a scan accepts.
D_STATIC const size_t d_ssh_internal_blob_capacity =
    ((D_SSH_INTERNAL_LINE_MAX / 4) * 3) + 2;

/*
d_ssh_internal_host_valid
  Internal: whether a host can stand in a known_hosts file: 1 to 255
printable characters, not starting with '@' (a marker), and none of them
whitespace or one of ",*?![]|#", which the format gives meaning to.
*/
bool
d_ssh_internal_host_valid(
    const char* _host
)
{
    if ( (!_host)             ||
         (_host[0] == '\0')   ||
         (_host[0] == '@') )
    {
        return false;
    }

    // every character printable and unreserved
    for (size_t length = 0; _host[length] != '\0'; length++)
    {
        const unsigned char c = (unsigned char)_host[length];

        if ( (c < 0x21)                               ||
             (c > 0x7E)                               ||
             (strchr(",*?![]|#", (int)c) != NULL)     ||
             (length >= 255) )
        {
            return false;
        }
    }

    return true;
}

/*
d_ssh_internal_host_string
  Internal: the name known_hosts records a host under, lowercased: the host
itself on port 22, and "[host]:port" on any other.
*/
enum d_ssh_status
d_ssh_internal_host_string(
    const char*  _host,
    unsigned int _port,
    char*        _name,
    size_t       _capacity
)
{
    if (!d_ssh_internal_host_valid(_host))
    {
        return D_SSH_ERR_FORMAT;
    }

    int written = 0;

    // only a non-default port is written out
    if (_port == D_SSH_PORT)
    {
        written = snprintf(_name, _capacity, "%s", _host);
    }
    else
    {
        written = snprintf(_name, _capacity, "[%s]:%u", _host, _port);
    }

    if ( (written < 0) ||
         ((size_t)written >= _capacity) )
    {
        return D_SSH_ERR_FORMAT;
    }

    // host names compare without regard to case
    for (size_t i = 0; _name[i] != '\0'; i++)
    {
        _name[i] = d_ssh_internal_lower(_name[i]);
    }

    return D_SSH_OK;
}

/*
d_ssh_internal_glob
  File-local: matches `_text` against `_length` bytes of a pattern in which
'*' matches any run and '?' any one character, without regard to case. A
mismatch backtracks to the last '*' only, so the worst case is the product
of the two lengths, never exponential.
*/
D_STATIC bool
d_ssh_internal_glob(
    const char* _pattern,
    size_t      _length,
    const char* _text
)
{
    size_t      p      = 0;
    size_t      star   = SIZE_MAX;
    const char* text   = _text;
    const char* resume = _text;

    // walk the text, retrying from the last star on a mismatch
    while (*text != '\0')
    {
        if ( (p < _length) &&
             (_pattern[p] == '*') )
        {
            star   = p;
            resume = text;
            p++;
        }
        else if ( (p < _length) &&
                  ( (_pattern[p] == '?') ||
                    (d_ssh_internal_lower(_pattern[p]) ==
                     d_ssh_internal_lower(*text)) ) )
        {
            p++;
            text++;
        }
        else if (star != SIZE_MAX)
        {
            p = star + 1;
            resume++;
            text = resume;
        }
        else
        {
            return false;
        }
    }

    // trailing stars match the empty remainder
    while ( (p < _length) &&
            (_pattern[p] == '*') )
    {
        p++;
    }

    return (p == _length);
}

/*
d_ssh_internal_patterns_match
  File-local: matches a comma list of patterns. A matching negated pattern
excludes the line outright, whatever else matched, as in OpenSSH.
*/
D_STATIC bool
d_ssh_internal_patterns_match(
    const char* _patterns,
    const char* _name
)
{
    bool        matched = false;
    const char* cursor  = _patterns;

    // each comma-separated pattern in turn
    for (;;)
    {
        const char* const end     = strchr(cursor, ',');
        const size_t      length  = (end) ? (size_t)(end - cursor)
                                          : strlen(cursor);
        const bool        negated = ( (length > 0) &&
                                      (cursor[0] == '!') );
        const char* const pattern = (negated) ? (cursor + 1) : cursor;
        const size_t      size    = (negated) ? (length - 1) : length;

        if ( (size > 0) &&
             (d_ssh_internal_glob(pattern, size, _name)) )
        {
            if (negated)
            {
                return false;
            }

            matched = true;
        }

        if (!end)
        {
            break;
        }

        cursor = end + 1;
    }

    return matched;
}

/*
d_ssh_internal_hashed_match
  File-local: matches a hashed "|1|salt|hash" field: the HMAC-SHA1 of the
name, keyed with the salt, must equal the hash. Both are 20 bytes, as
OpenSSH writes them.
*/
D_STATIC bool
d_ssh_internal_hashed_match(
    const char* _field,
    const char* _name
)
{
    const char* const salt = _field + 3;
    const char* const bar  = strchr(salt, '|');

    if (!bar)
    {
        return false;
    }

    unsigned char salt_bytes[32];
    unsigned char hash_bytes[32];
    size_t        salt_size = 0;
    size_t        hash_size = 0;

    const enum d_ssh_status salt_status =
        d_ssh_base64_decode(salt,
                            (size_t)(bar - salt),
                            salt_bytes,
                            sizeof(salt_bytes),
                            &salt_size);
    const enum d_ssh_status hash_status =
        d_ssh_base64_decode(bar + 1,
                            strlen(bar + 1),
                            hash_bytes,
                            sizeof(hash_bytes),
                            &hash_size);

    if ( (salt_status != D_SSH_OK)               ||
         (hash_status != D_SSH_OK)               ||
         (salt_size != D_SSH_INTERNAL_SHA1_SIZE) ||
         (hash_size != D_SSH_INTERNAL_SHA1_SIZE) )
    {
        return false;
    }

    unsigned char mac[D_SSH_INTERNAL_SHA1_SIZE];

    d_ssh_internal_hmac_sha1(salt_bytes,
                             salt_size,
                             _name,
                             strlen(_name),
                             mac);

    return (memcmp(mac, hash_bytes, sizeof(mac)) == 0);
}

/*
d_ssh_internal_scan_free
  Internal: releases a scan's buffers; safe to repeat.
*/
void
d_ssh_internal_scan_free(
    struct d_ssh_internal_scan* _scan
)
{
    free(_scan->line);
    free(_scan->blob);

    _scan->line = NULL;
    _scan->blob = NULL;

    return;
}

/*
d_ssh_internal_scan_init
  Internal: starts a scan for a host and, if it has arrived, its key. The
buffers are allocated once and reused for every line of every file.
*/
enum d_ssh_status
d_ssh_internal_scan_init(
    struct d_ssh_internal_scan* _scan,
    const char*                 _name,
    const unsigned char*        _key,
    size_t                      _key_size
)
{
    memset(_scan, 0, sizeof(*_scan));

    _scan->name     = _name;
    _scan->key      = _key;
    _scan->key_size = _key_size;
    _scan->line     = malloc((size_t)D_SSH_INTERNAL_LINE_MAX + 2);
    _scan->blob     = malloc(d_ssh_internal_blob_capacity);

    if ( (!_scan->line) ||
         (!_scan->blob) )
    {
        d_ssh_internal_scan_free(_scan);

        return D_SSH_ERR_MEMORY;
    }

    return D_SSH_OK;
}

/*
d_ssh_internal_token
  File-local: splits the next whitespace-delimited field off `*_cursor` in
place, or returns NULL at the end of the line.
*/
D_STATIC char*
d_ssh_internal_token(
    char** _cursor
)
{
    char* start = *_cursor;

    // fields are separated by runs of spaces and tabs
    while ( (*start == ' ') ||
            (*start == '\t') )
    {
        start++;
    }

    if ( (*start == '\0') ||
         (*start == '\r') ||
         (*start == '\n') )
    {
        *_cursor = start;

        return NULL;
    }

    char* end = start;

    while ( (*end != '\0') &&
            (*end != ' ')  &&
            (*end != '\t') &&
            (*end != '\r') &&
            (*end != '\n') )
    {
        end++;
    }

    // terminate the field, stepping past its delimiter if it had one
    if (*end != '\0')
    {
        *end = '\0';
        end++;
    }

    *_cursor = end;

    return start;
}

/*
d_ssh_internal_scan_note_type
  File-local: adds a key type to the scan's list once. A list that would
overflow is left as it is: it only orders preferences.
*/
D_STATIC void
d_ssh_internal_scan_note_type(
    struct d_ssh_internal_scan* _scan,
    const char*                 _type
)
{
    const size_t used   = strlen(_scan->types);
    const size_t length = strlen(_type);

    if ( (d_ssh_name_list_contains(_scan->types, used, _type)) ||
         ((used + length + 2) > sizeof(_scan->types)) )
    {
        return;
    }

    // a comma separates it from the types before it
    if (used > 0)
    {
        _scan->types[used] = ',';
        memcpy(_scan->types + used + 1, _type, length + 1);
    }
    else
    {
        memcpy(_scan->types, _type, length + 1);
    }

    return;
}

/*
d_ssh_internal_scan_names
  File-local: whether a hosts field names the scan's host, as a hashed entry
or as patterns.
*/
D_STATIC bool
d_ssh_internal_scan_names(
    const struct d_ssh_internal_scan* _scan,
    const char*                       _field
)
{
    return (strncmp(_field, "|1|", 3) == 0)
               ? d_ssh_internal_hashed_match(_field, _scan->name)
               : d_ssh_internal_patterns_match(_field, _scan->name);
}

/*
d_ssh_internal_scan_key
  File-local: decodes a line's key into the scan's blob buffer, and checks
that the type embedded in it is the one the line claims.
*/
D_STATIC bool
d_ssh_internal_scan_key(
    struct d_ssh_internal_scan* _scan,
    const char*                 _type,
    const char*                 _encoded,
    size_t*                     _size
)
{
    char blob_type[D_SSH_KEY_TYPE_SIZE];

    return ( (d_ssh_base64_decode(_encoded,
                                  strlen(_encoded),
                                  _scan->blob,
                                  d_ssh_internal_blob_capacity,
                                  _size) == D_SSH_OK)         &&
             (d_ssh_key_type(_scan->blob,
                             *_size,
                             blob_type,
                             sizeof(blob_type)) == D_SSH_OK)  &&
             (strcmp(blob_type, _type) == 0) );
}

/*
d_ssh_internal_scan_line
  File-local: applies the line in the scan's buffer. A line counts only if it
parses completely and its key's embedded type equals its type field; any
other line is skipped, as OpenSSH skips it.
*/
D_STATIC void
d_ssh_internal_scan_line(
    struct d_ssh_internal_scan* _scan
)
{
    char*       cursor = _scan->line;
    const char* marker = NULL;
    char*       field  = d_ssh_internal_token(&cursor);

    // blank lines and comments
    if ( (!field) ||
         (field[0] == '#') )
    {
        return;
    }

    // an optional marker comes first
    if (field[0] == '@')
    {
        marker = field;
        field  = d_ssh_internal_token(&cursor);
    }

    const char* const type    = d_ssh_internal_token(&cursor);
    const char* const encoded = d_ssh_internal_token(&cursor);
    size_t            size    = 0;

    // a @cert-authority line vouches for certificates, not for this key, and
    // an unknown marker is not understood well enough to act on
    if ( (!field)                                               ||
         (!type)                                                ||
         (!encoded)                                             ||
         ( (marker) &&
           (strcmp(marker, "@revoked") != 0) )                  ||
         (!d_ssh_internal_scan_names(_scan, field))             ||
         (!d_ssh_internal_scan_key(_scan, type, encoded, &size)) )
    {
        return;
    }

    const bool same = ( (_scan->key)              &&
                        (size == _scan->key_size) &&
                        (memcmp(_scan->blob, _scan->key, size) == 0) );

    // a revocation marks the key, never the host
    if (marker)
    {
        _scan->revoked = ( (_scan->revoked) ||
                           (same) );

        return;
    }

    _scan->recorded = true;
    _scan->found    = ( (_scan->found) ||
                        (same) );
    d_ssh_internal_scan_note_type(_scan, type);

    return;
}

/*
d_ssh_internal_scan_file
  Internal: applies every line of a file. A missing file records nothing;
an unreadable one, or a line too long to hold, fails the scan rather than
silently hiding an entry that might have mattered.
*/
enum d_ssh_status
d_ssh_internal_scan_file(
    struct d_ssh_internal_scan* _scan,
    const char*                 _path
)
{
    if (!_path)
    {
        return D_SSH_OK;
    }

    FILE* const file = fopen(_path, "rb");

    if (!file)
    {
        return (errno == ENOENT) ? D_SSH_OK : D_SSH_ERR_KNOWN_HOSTS;
    }

    enum d_ssh_status status = D_SSH_OK;

    // a line, newline included, fills at most D_SSH_INTERNAL_LINE_MAX + 1
    while (fgets(_scan->line, D_SSH_INTERNAL_LINE_MAX + 2, file))
    {
        const size_t length = strlen(_scan->line);

        if ( (length > 0)                          &&
             (_scan->line[length - 1] != '\n')     &&
             (!feof(file)) )
        {
            status = D_SSH_ERR_KNOWN_HOSTS;

            break;
        }

        d_ssh_internal_scan_line(_scan);
    }

    if ( (status == D_SSH_OK) &&
         (ferror(file)) )
    {
        status = D_SSH_ERR_KNOWN_HOSTS;
    }

    (void)fclose(file);

    return status;
}

/*
d_ssh_internal_scan_verdict
  Internal: a revocation outranks a match, a match outranks a different
recorded key, and only a host no line names is unknown.
*/
enum d_ssh_host_match
d_ssh_internal_scan_verdict(
    const struct d_ssh_internal_scan* _scan
)
{
    if (_scan->revoked)
    {
        return D_SSH_HOST_REVOKED;
    }

    if (_scan->found)
    {
        return D_SSH_HOST_KNOWN;
    }

    return (_scan->recorded) ? D_SSH_HOST_CHANGED : D_SSH_HOST_UNKNOWN;
}

/*
d_ssh_known_hosts_check
  One scan of one file. A session runs the same scan over the user's file and
the system's, so both agree on what every line means.
*/
enum d_ssh_status
d_ssh_known_hosts_check(
    const char*            _path,
    const char*            _host,
    unsigned int           _port,
    const unsigned char*   _key,
    size_t                 _key_size,
    enum d_ssh_host_match* _match
)
{
    if (_match)
    {
        *_match = D_SSH_HOST_UNKNOWN;
    }

    if ( (!_path)          ||
         (!_host)          ||
         (!_key)           ||
         (_key_size == 0)  ||
         (!_match)         ||
         (_port == 0)      ||
         (_port > 65535) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    char                       name[D_SSH_INTERNAL_HOST_MAX];
    struct d_ssh_internal_scan scan;
    enum d_ssh_status          status =
        d_ssh_internal_host_string(_host, _port, name, sizeof(name));

    if (status != D_SSH_OK)
    {
        return status;
    }

    status = d_ssh_internal_scan_init(&scan, name, _key, _key_size);

    if (status == D_SSH_OK)
    {
        status = d_ssh_internal_scan_file(&scan, _path);
    }

    if (status == D_SSH_OK)
    {
        *_match = d_ssh_internal_scan_verdict(&scan);
    }

    d_ssh_internal_scan_free(&scan);

    return status;
}

/*
d_ssh_internal_hosts_field
  File-local: the hosts field of a new line: the name itself, or a salted
HMAC-SHA1 of it in OpenSSH's "|1|salt|hash" form, with a fresh salt, so that
equal names never yield equal fields.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_hosts_field(
    const char* _name,
    bool        _hashed,
    char*       _field,
    size_t      _capacity
)
{
    const size_t length = strlen(_name);

    // "|1|", two 28-character encodings, a bar, and a NUL
    if ( (length >= _capacity) ||
         (_capacity < 64) )
    {
        return D_SSH_ERR_FORMAT;
    }

    if (!_hashed)
    {
        memcpy(_field, _name, length + 1);

        return D_SSH_OK;
    }

    unsigned char     salt[D_SSH_INTERNAL_SHA1_SIZE];
    unsigned char     mac[D_SSH_INTERNAL_SHA1_SIZE];
    enum d_ssh_status status = d_ssh_random(salt, sizeof(salt));

    if (status != D_SSH_OK)
    {
        return status;
    }

    d_ssh_internal_hmac_sha1(salt, sizeof(salt), _name, length, mac);
    memcpy(_field, "|1|", 3);

    status = d_ssh_base64_encode(salt,
                                 sizeof(salt),
                                 true,
                                 _field + 3,
                                 _capacity - 3);

    const size_t used = strlen(_field);

    if (status == D_SSH_OK)
    {
        _field[used] = '|';
        status       = d_ssh_base64_encode(mac,
                                           sizeof(mac),
                                           true,
                                           _field + used + 1,
                                           _capacity - used - 1);
    }

    return status;
}

/*
d_ssh_internal_append_line
  File-local: appends a line to a file, first ending an unterminated last
line, so that the two cannot merge into one entry.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_append_line(
    const char* _path,
    const char* _line,
    size_t      _length
)
{
    FILE* const file = fopen(_path, "a+b");

    if (!file)
    {
        return D_SSH_ERR_KNOWN_HOSTS;
    }

    bool terminated = true;

    // look at the last byte, if there is one
    if ( (fseek(file, 0, SEEK_END) == 0) &&
         (ftell(file) > 0)               &&
         (fseek(file, -1, SEEK_END) == 0) )
    {
        terminated = (fgetc(file) == '\n');
    }

    // a stream must be repositioned between a read and a write
    bool written = (fseek(file, 0, SEEK_END) == 0);

    if ( (written) &&
         (!terminated) )
    {
        written = (fputc('\n', file) != EOF);
    }

    if (written)
    {
        written = (fwrite(_line, 1, _length, file) == _length);
    }

    if (written)
    {
        written = (fflush(file) == 0);
    }

    const bool closed = (fclose(file) == 0);

    return ( (written) &&
             (closed) ) ? D_SSH_OK : D_SSH_ERR_KNOWN_HOSTS;
}

/*
d_ssh_internal_compose_line
  File-local: "prefix key\n" in a new allocation, the key in padded Base64,
with its length, terminator excluded.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_compose_line(
    const char*          _prefix,
    const unsigned char* _key,
    size_t               _key_size,
    char**               _line,
    size_t*              _length
)
{
    const size_t prefix_length = strlen(_prefix);
    const size_t key_length    = d_ssh_base64_encoded_size(_key_size, true);

    *_line   = NULL;
    *_length = 0;

    // the prefix, a space, the key, a newline, and a terminator
    if ( (key_length == 0) ||
         (key_length > (SIZE_MAX - prefix_length - 3)) )
    {
        return D_SSH_ERR_FORMAT;
    }

    const size_t key_at = prefix_length + 1;
    char* const  line   = malloc(key_at + key_length + 2);

    if (!line)
    {
        return D_SSH_ERR_MEMORY;
    }

    memcpy(line, _prefix, prefix_length);
    line[prefix_length] = ' ';

    const enum d_ssh_status status = d_ssh_base64_encode(_key,
                                                         _key_size,
                                                         true,
                                                         line + key_at,
                                                         key_length + 1);

    if (status != D_SSH_OK)
    {
        free(line);

        return status;
    }

    line[key_at + key_length]     = '\n';
    line[key_at + key_length + 1] = '\0';
    *_line                        = line;
    *_length                      = key_at + key_length + 1;

    return D_SSH_OK;
}

/*
d_ssh_known_hosts_add
  The line is composed in full before the file is opened and written with one
call, so a failure cannot leave half an entry behind.
*/
enum d_ssh_status
d_ssh_known_hosts_add(
    const char*          _path,
    const char*          _host,
    unsigned int         _port,
    const unsigned char* _key,
    size_t               _key_size,
    bool                 _hashed
)
{
    if ( (!_path)          ||
         (!_host)          ||
         (!_key)           ||
         (_key_size == 0)  ||
         (_port == 0)      ||
         (_port > 65535) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    char              name[D_SSH_INTERNAL_HOST_MAX];
    char              type[D_SSH_KEY_TYPE_SIZE];
    char              prefix[D_SSH_INTERNAL_FIELD_MAX + D_SSH_KEY_TYPE_SIZE];
    enum d_ssh_status status =
        d_ssh_internal_host_string(_host, _port, name, sizeof(name));

    if (status == D_SSH_OK)
    {
        status = d_ssh_key_type(_key, _key_size, type, sizeof(type));
    }

    // the hosts field, then the type after a space
    if (status == D_SSH_OK)
    {
        status = d_ssh_internal_hosts_field(name,
                                            _hashed,
                                            prefix,
                                            D_SSH_INTERNAL_FIELD_MAX);
    }

    if (status != D_SSH_OK)
    {
        return status;
    }

    const size_t field_length = strlen(prefix);
    char*        line         = NULL;
    size_t       length       = 0;

    prefix[field_length] = ' ';
    memcpy(prefix + field_length + 1, type, strlen(type) + 1);

    status = d_ssh_internal_compose_line(prefix,
                                         _key,
                                         _key_size,
                                         &line,
                                         &length);

    if (status == D_SSH_OK)
    {
        status = d_ssh_internal_append_line(_path, line, length);
        free(line);
    }

    return status;
}

/*
d_ssh_host_match_string
  One name per verdict.
*/
const char*
d_ssh_host_match_string(
    enum d_ssh_host_match _match
)
{
    switch (_match)
    {
        case D_SSH_HOST_KNOWN:
            return "known";
        case D_SSH_HOST_UNKNOWN:
            return "unknown";
        case D_SSH_HOST_CHANGED:
            return "changed";
        case D_SSH_HOST_REVOKED:
            return "revoked";
    }

    return "unknown";
}
