/*******************************************************************************
* djinterp [tools]                                            dhost_articulate.c
*
* Definitions for the declarations in dhost_articulate.h.
*   The package is opened once and kept: a slide's name and its media are read
* when the story is opened, a replacement goes into the package's copy of the
* media part at once, and the checksums are rewritten only when the story is
* saved, after every replacement is known. Each assumption about the format
* named in the header is a function of its own here, so correcting one
* against a real .story changes one function.
*
* path:      /src/djinterp/tools/dawk/ext/story/dhost_articulate.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/story/dhost_articulate.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, calloc, realloc, free, qsort, strtoul
#include <string.h>  // memcpy, memcmp, memset, strchr, strcmp, strlen,
                     // strncmp, strrchr, strstr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dxml.h"  // d_xml_next
#include "../../../../../../inc/djinterp/tools/dawk/dzip.h"  // d_zip_open


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// D_INTERNAL_STORY_PART, D_INTERNAL_STORY_RELS
//   constant: where the story part and its relationships are taken to be.
#define D_INTERNAL_STORY_PART "story/story.xml"
#define D_INTERNAL_STORY_RELS "story/_rels/story.xml.rels"

// 1.1    State
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_use
//   struct: a media part, and how many relationships, in any part, use it.
struct d_internal_use
{
    char*  part;
    size_t count;
};

// 1.1.2
// d_internal_change
//   struct: a media part replaced, with the MD5 the XML records for it and
//   the MD5 it must record.  `applied` is set once a save has rewritten them.
struct d_internal_change
{
    char*         part;
    unsigned char before[16];
    unsigned char after[16];
    bool          applied;
};

// 1.1.3
// d_internal_articulate
//   struct: the package, and what replacing and saving need of it.
struct d_internal_articulate
{
    struct d_zip*             zip;
    struct d_story*           story;
    struct d_internal_use*    uses;
    size_t                    use_count;
    struct d_internal_change* changes;
    size_t                    change_count;
};

// 1.1.4
// d_internal_slide_part
//   struct: a slide part found by the scene list, or by its name.
struct d_internal_slide_part
{
    char*  part;
    size_t scene;
    size_t order;
};


//==============================================================================
// 2.  HELPERS
//==============================================================================


/*
d_internal_problem
  Writes a reason into a caller's buffer, which may be NULL; always false.
*/
static bool
d_internal_problem(
    char*       _problem,
    size_t      _capacity,
    const char* _message,
    const char* _subject
)
{
    // a caller that did not ask for the reason is given none
    if ( (_problem) &&
         (_capacity > 0) )
    {
        (void)snprintf(_problem,
                       _capacity,
                       (_subject) ? "%s: %s" : "%s",
                       _message,
                       _subject);
    }

    return false;
}


/*
d_internal_ends_with
  Whether a string ends with another, ignoring ASCII case.
*/
static bool
d_internal_ends_with(
    const char* _text,
    const char* _end
)
{
    const size_t length = strlen(_text);
    const size_t span   = strlen(_end);

    // a string shorter than its ending cannot end with it
    if (span > length)
    {
        return false;
    }

    for (size_t at = 0; at < span; ++at)
    {
        const char left  = _text[length - span + at];
        const char right = _end[at];
        const char lower = ((left >= 'A') && (left <= 'Z'))
                         ? (char)(left - 'A' + 'a') : left;

        if (lower != right)
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_attribute
  An attribute's value, decoded, as an allocated string; NULL when the tag
has no such attribute or memory ran out.
*/
static char*
d_internal_attribute(
    const struct d_xml_token* _token,
    const char*               _name
)
{
    const char* raw    = NULL;
    size_t      length = 0;

    // a tag without the attribute gives nothing
    if (!d_xml_attribute(_token, _name, &raw, &length))
    {
        return NULL;
    }

    char* const value = malloc(length + 1u);

    // a value that cannot be held is reported by its absence
    if (value)
    {
        value[d_xml_decode(raw, length, value)] = '\0';
    }

    return value;
}


/*
d_internal_read_part
  Reads a part as text; NULL when the package does not hold it.
*/
static char*
d_internal_read_part(
    struct d_zip* _zip,
    const char*   _part,
    size_t*       _out_length,
    char*         _problem,
    size_t        _capacity
)
{
    const size_t   index  = d_zip_find(_zip, _part);
    unsigned char* bytes  = NULL;
    size_t         length = 0;

    // a part the package does not hold reads as nothing
    if (index == D_ZIP_NONE)
    {
        return NULL;
    }

    // a part that cannot be read reads as nothing, saying why
    if (!d_zip_read(_zip, index, &bytes, &length, _problem, _capacity))
    {
        return NULL;
    }

    // a caller that does not want the length is not given it
    if (_out_length)
    {
        *_out_length = length;
    }

    return (char*)bytes;
}


/*
d_internal_resolve
  The package name a relationship's target names: an absolute target is the
part's name without its leading `/`; a relative one is joined to the
directory of the part the relationship belongs to, with `..` and `.` taken
out.
*/
static char*
d_internal_resolve(
    const char* _base_directory,
    const char* _target
)
{
    const bool   absolute = (_target[0] == '/');
    const char*  base     = absolute ? "" : _base_directory;
    const size_t length   = strlen(base) + strlen(_target) + 2u;
    char* const  joined   = malloc(length);
    char* const  out      = malloc(length);

    // a name that cannot be held is reported by its absence
    if ( (!joined) ||
         (!out) )
    {
        free(joined);
        free(out);

        return NULL;
    }

    (void)snprintf(joined, length, "%s%s", base, absolute ? &_target[1]
                                                          : _target);

    size_t used = 0;

    // each segment is copied, `.` dropped and `..` taking back the last
    for (const char* at = joined; *at != '\0';)
    {
        const char*  slash = strchr(at, '/');
        const size_t span  = (slash) ? (size_t)(slash - at) : strlen(at);

        if ( (span == 2u) &&
             (at[0] == '.') &&
             (at[1] == '.') )
        {
            // `..` removes the segment before it, if there is one
            while ( (used > 0) &&
                    (out[used - 1u] != '/') )
            {
                used--;
            }

            used = (used > 0) ? (used - 1u) : 0u;
        }
        else if ( (span > 0) &&
                  !( (span == 1u) && (at[0] == '.') ) )
        {
            // a segment is joined to what is there with a `/`
            if (used > 0)
            {
                out[used++] = '/';
            }

            memcpy(&out[used], at, span);
            used += span;
        }

        at = (slash) ? (slash + 1) : (at + span);
    }

    out[used] = '\0';
    free(joined);

    return out;
}


/*
d_internal_rels_of
  The relationship part of a part: dir/_rels/name.rels.  `_out_directory`
receives the part's directory, with its `/`, which its relative targets are
read against.
*/
static char*
d_internal_rels_of(
    const char* _part,
    char**      _out_directory
)
{
    const char* const slash     = strrchr(_part, '/');
    const size_t      directory = (slash) ? (size_t)(slash - _part) + 1u : 0u;
    const size_t      length    = strlen(_part) + sizeof("_rels/.rels");
    char* const       rels      = malloc(length);
    char* const       dir       = malloc(directory + 1u);

    // names that cannot be held are reported by their absence
    if ( (!rels) ||
         (!dir) )
    {
        free(rels);
        free(dir);

        return NULL;
    }

    memcpy(dir, _part, directory);
    dir[directory] = '\0';
    (void)snprintf(rels, length, "%s_rels/%s.rels", dir, &_part[directory]);
    *_out_directory = dir;

    return rels;
}


//==============================================================================
// 3.  MD5
//==============================================================================


/*
d_internal_md5
  RFC 1321.  The checksum the XML records for each media part; it is no
safeguard here, only a value to keep in step.
*/
static void
d_internal_md5(
    const unsigned char* _bytes,
    size_t               _length,
    unsigned char*       _out
)
{
    static const unsigned long sines[64] =
    {
        0xd76aa478ul, 0xe8c7b756ul, 0x242070dbul, 0xc1bdceeeul, 0xf57c0faful,
        0x4787c62aul, 0xa8304613ul, 0xfd469501ul, 0x698098d8ul, 0x8b44f7aful,
        0xffff5bb1ul, 0x895cd7beul, 0x6b901122ul, 0xfd987193ul, 0xa679438eul,
        0x49b40821ul, 0xf61e2562ul, 0xc040b340ul, 0x265e5a51ul, 0xe9b6c7aaul,
        0xd62f105dul, 0x02441453ul, 0xd8a1e681ul, 0xe7d3fbc8ul, 0x21e1cde6ul,
        0xc33707d6ul, 0xf4d50d87ul, 0x455a14edul, 0xa9e3e905ul, 0xfcefa3f8ul,
        0x676f02d9ul, 0x8d2a4c8aul, 0xfffa3942ul, 0x8771f681ul, 0x6d9d6122ul,
        0xfde5380cul, 0xa4beea44ul, 0x4bdecfa9ul, 0xf6bb4b60ul, 0xbebfbc70ul,
        0x289b7ec6ul, 0xeaa127faul, 0xd4ef3085ul, 0x04881d05ul, 0xd9d4d039ul,
        0xe6db99e5ul, 0x1fa27cf8ul, 0xc4ac5665ul, 0xf4292244ul, 0x432aff97ul,
        0xab9423a7ul, 0xfc93a039ul, 0x655b59c3ul, 0x8f0ccc92ul, 0xffeff47dul,
        0x85845dd1ul, 0x6fa87e4ful, 0xfe2ce6e0ul, 0xa3014314ul, 0x4e0811a1ul,
        0xf7537e82ul, 0xbd3af235ul, 0x2ad7d2bbul, 0xeb86d391ul
    };
    static const unsigned shifts[64] =
    {
        7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
        5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
    };

    unsigned long state[4] =
    {
        0x67452301ul, 0xefcdab89ul, 0x98badcfeul, 0x10325476ul
    };

    // the message, a 1 bit, zeros, and the bit length make whole blocks
    const size_t blocks = ((_length + 8u) / 64u) + 1u;

    for (size_t block = 0; block < blocks; ++block)
    {
        unsigned char chunk[64];
        unsigned long words[16];

        // each byte of the block is the message's, the pad's or the length's
        for (size_t at = 0; at < 64u; ++at)
        {
            const size_t offset = (block * 64u) + at;

            chunk[at] = (offset < _length)  ? _bytes[offset]
                      : (offset == _length) ? (unsigned char)0x80u
                      :                       (unsigned char)0u;
        }

        // the last block ends with the length in bits, least byte first
        if ((block + 1u) == blocks)
        {
            const unsigned long long bits = (unsigned long long)_length * 8u;

            for (size_t at = 0; at < 8u; ++at)
            {
                chunk[56u + at] = (unsigned char)((bits >> (8u * at)) & 0xFFu);
            }
        }

        for (size_t at = 0; at < 16u; ++at)
        {
            words[at] =  (unsigned long)chunk[at * 4u]               |
                        ((unsigned long)chunk[(at * 4u) + 1u] << 8)  |
                        ((unsigned long)chunk[(at * 4u) + 2u] << 16) |
                        ((unsigned long)chunk[(at * 4u) + 3u] << 24);
        }

        unsigned long a = state[0];
        unsigned long b = state[1];
        unsigned long c = state[2];
        unsigned long d = state[3];

        // sixty-four rounds, four functions of sixteen each
        for (unsigned round = 0; round < 64u; ++round)
        {
            unsigned long f    = 0;
            unsigned      word = 0;

            if (round < 16u)
            {
                f    = (b & c) | (~b & d);
                word = round;
            }
            else if (round < 32u)
            {
                f    = (d & b) | (~d & c);
                word = ((5u * round) + 1u) % 16u;
            }
            else if (round < 48u)
            {
                f    = b ^ c ^ d;
                word = ((3u * round) + 5u) % 16u;
            }
            else
            {
                f    = c ^ (b | ~d);
                word = (7u * round) % 16u;
            }

            const unsigned long sum =
                (a + (f & 0xFFFFFFFFul) + sines[round] + words[word]) &
                0xFFFFFFFFul;

            a = d;
            d = c;
            c = b;
            b = (b + (((sum << shifts[round]) |
                       (sum >> (32u - shifts[round]))) & 0xFFFFFFFFul)) &
                0xFFFFFFFFul;
        }

        state[0] = (state[0] + a) & 0xFFFFFFFFul;
        state[1] = (state[1] + b) & 0xFFFFFFFFul;
        state[2] = (state[2] + c) & 0xFFFFFFFFul;
        state[3] = (state[3] + d) & 0xFFFFFFFFul;
    }

    for (size_t at = 0; at < 16u; ++at)
    {
        _out[at] = (unsigned char)((state[at / 4u] >> (8u * (at % 4u))) &
                                   0xFFu);
    }

    return;
}


/*
d_internal_spell
  Spells a digest the three ways the XML might record it: lower-case hex,
upper-case hex, and base64 without its padding, which a padded value still
begins with.
*/
static void
d_internal_spell(
    const unsigned char* _digest,
    char                 _lower[33],
    char                 _upper[33],
    char                 _base64[23]
)
{
    static const char lower_digits[] = "0123456789abcdef";
    static const char upper_digits[] = "0123456789ABCDEF";
    static const char digits[]       =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    for (size_t at = 0; at < 16u; ++at)
    {
        _lower[at * 2u]      = lower_digits[_digest[at] >> 4];
        _lower[at * 2u + 1u] = lower_digits[_digest[at] & 0x0Fu];
        _upper[at * 2u]      = upper_digits[_digest[at] >> 4];
        _upper[at * 2u + 1u] = upper_digits[_digest[at] & 0x0Fu];
    }

    _lower[32] = '\0';
    _upper[32] = '\0';

    size_t used = 0;

    // three bytes are four digits; the sixteenth byte is two more
    for (size_t at = 0; at < 16u; at += 3u)
    {
        const unsigned long group =
             ((unsigned long)_digest[at] << 16)                         |
            (((at + 1u) < 16u) ? ((unsigned long)_digest[at + 1u] << 8) : 0ul) |
            (((at + 2u) < 16u) ? (unsigned long)_digest[at + 2u] : 0ul);
        const size_t  count = ((at + 3u) <= 16u) ? 4u : 2u;

        for (size_t digit = 0; digit < count; ++digit)
        {
            _base64[used++] = digits[(group >> (18u - (6u * digit))) & 0x3Fu];
        }
    }

    _base64[used] = '\0';

    return;
}


//==============================================================================
// 4.  READING THE PACKAGE
//==============================================================================


/*
d_internal_each_media
  Calls `_visit` for every relationship of type "media" in one relationship
part, with the part it targets.  A type is "media" itself or a URI ending in
"/media"; an external target is not a part of the package and is skipped.
*/
static bool
d_internal_each_media(
    struct d_zip* _zip,
    const char*   _rels,
    const char*   _directory,
    bool          (*_visit)(void*, const char*),
    void*         _user
)
{
    size_t      length = 0;
    char* const text   = d_internal_read_part(_zip, _rels, &length, NULL, 0);
    bool        ok     = true;

    // a part with no relationship part uses nothing
    if (!text)
    {
        return true;
    }

    struct d_xml       xml;
    struct d_xml_token token;

    d_xml_begin(&xml, text, length);

    while ( (ok) &&
            (d_xml_next(&xml, &token)) )
    {
        // only a Relationship element says anything
        if ( ( (token.kind != D_XML_START) &&
               (token.kind != D_XML_EMPTY) ) ||
             (!d_xml_is(&token, "Relationship")) )
        {
            continue;
        }

        char* const type   = d_internal_attribute(&token, "Type");
        char* const target = d_internal_attribute(&token, "Target");
        char* const mode   = d_internal_attribute(&token, "TargetMode");

        // a media relationship to a part of the package is visited
        if ( (type)                                              &&
             (target)                                            &&
             ( (strcmp(type, "media") == 0) ||
               (d_internal_ends_with(type, "/media")) )          &&
             ( (!mode) || (strcmp(mode, "External") != 0) ) )
        {
            char* const part = d_internal_resolve(_directory, target);

            ok = (part) && _visit(_user, part);
            free(part);
        }

        free(type);
        free(target);
        free(mode);
    }

    free(text);

    return ok;
}


/*
d_internal_count_use
  Counts one use of a media part, as d_internal_each_media visits it.
*/
static bool
d_internal_count_use(
    void*       _user,
    const char* _part
)
{
    struct d_internal_articulate* const state = _user;

    for (size_t at = 0; at < state->use_count; ++at)
    {
        // a part counted before is counted again
        if (strcmp(state->uses[at].part, _part) == 0)
        {
            state->uses[at].count++;

            return true;
        }
    }

    struct d_internal_use* const grown =
        realloc(state->uses, (state->use_count + 1u) * sizeof(*grown));
    char* const part = malloc(strlen(_part) + 1u);

    // report the failure rather than miscounting
    if ( (!grown) ||
         (!part) )
    {
        state->uses = (grown) ? grown : state->uses;
        free(part);

        return false;
    }

    memcpy(part, _part, strlen(_part) + 1u);
    grown[state->use_count].part  = part;
    grown[state->use_count].count = 1u;
    state->uses                   = grown;
    state->use_count++;

    return true;
}


/*
d_internal_count_uses
  Counts every media relationship of every relationship part: a slide's, a
layout's, a master's, the story's own.  What the count is for is refusing to
replace a part through one slide that something else also shows.
*/
static bool
d_internal_count_uses(
    struct d_internal_articulate* _state
)
{
    for (size_t at = 0; at < d_zip_count(_state->zip); ++at)
    {
        const char* const name = d_zip_name(_state->zip, at);
        const char* const rels = strstr(name, "_rels/");

        // only a relationship part relates anything
        if ( (!rels) ||
             (!d_internal_ends_with(name, ".rels")) )
        {
            continue;
        }

        const size_t directory = (size_t)(rels - name);
        char* const  base      = malloc(directory + 1u);

        // a name that cannot be held stops the count
        if (!base)
        {
            return false;
        }

        memcpy(base, name, directory);
        base[directory] = '\0';

        const bool ok = d_internal_each_media(_state->zip, name, base,
                                              d_internal_count_use, _state);

        free(base);

        // a part whose relationships could not be counted stops the count
        if (!ok)
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_add_part
  Appends a slide part to the list being built.
*/
static bool
d_internal_add_part(
    struct d_internal_slide_part** _parts,
    size_t*                        _count,
    char*                          _part,
    size_t                         _scene,
    size_t                         _order
)
{
    struct d_internal_slide_part* const grown =
        realloc(*_parts, (*_count + 1u) * sizeof(*grown));

    // report the failure rather than losing a slide
    if (!grown)
    {
        free(_part);

        return false;
    }

    grown[*_count].part  = _part;
    grown[*_count].scene = _scene;
    grown[*_count].order = _order;
    *_parts              = grown;
    (*_count)++;

    return true;
}


/*
d_internal_relationship_target
  The target the story's relationship part gives an id; NULL for an id it
does not have.
*/
static char*
d_internal_relationship_target(
    struct d_zip* _zip,
    const char*   _id
)
{
    size_t      length = 0;
    char* const text   = d_internal_read_part(_zip, D_INTERNAL_STORY_RELS,
                                              &length, NULL, 0);
    char*       found  = NULL;

    struct d_xml       xml;
    struct d_xml_token token;

    d_xml_begin(&xml, text, (text) ? length : 0u);

    while ( (!found) &&
            (d_xml_next(&xml, &token)) )
    {
        // only a Relationship element with this id is the answer
        if ( ( (token.kind == D_XML_START) ||
               (token.kind == D_XML_EMPTY) ) &&
             (d_xml_is(&token, "Relationship")) )
        {
            char* const id = d_internal_attribute(&token, "Id");

            // the target is taken when the id is the one asked for
            if ( (id) &&
                 (strcmp(id, _id) == 0) )
            {
                char* const target = d_internal_attribute(&token, "Target");

                found = (target) ? d_internal_resolve("story/", target)
                                 : NULL;
                free(target);
            }

            free(id);
        }
    }

    free(text);

    return found;
}


/*
d_internal_slide_id
  The relationship id an <sldId> holds: as an r:id or id attribute, or as its
text.
*/
static char*
d_internal_slide_id(
    struct d_xml*             _xml,
    const struct d_xml_token* _token
)
{
    char* id = d_internal_attribute(_token, "r:id");

    // the unprefixed attribute, then the text, are tried in turn
    if (!id)
    {
        id = d_internal_attribute(_token, "id");
    }

    if ( (!id) &&
         (_token->kind == D_XML_START) )
    {
        struct d_xml_token text;
        struct d_xml       peek = *_xml;

        // text directly inside the element is the id
        if ( (d_xml_next(&peek, &text)) &&
             (text.kind == D_XML_TEXT) )
        {
            id = malloc(text.text_length + 1u);

            if (id)
            {
                id[d_xml_decode(text.text, text.text_length, id)] = '\0';
            }
        }
    }

    return id;
}


/*
d_internal_scene_slides
  The slides the scene list names, in order: the assumption most worth
checking against a real .story.  Scenes are the <scene> elements inside
<sceneLst>, so the question banks' scenes, which are elsewhere, are not slides
of the story.
*/
static bool
d_internal_scene_slides(
    struct d_zip*                  _zip,
    struct d_internal_slide_part** _parts,
    size_t*                        _count,
    char*                          _problem,
    size_t                         _capacity
)
{
    size_t      length = 0;
    char* const text   = d_internal_read_part(_zip, D_INTERNAL_STORY_PART,
                                              &length, _problem, _capacity);

    // a package without the story part is no Storyline story
    if (!text)
    {
        return d_internal_problem(_problem, _capacity,
                                  "not a Storyline package: no part",
                                  D_INTERNAL_STORY_PART);
    }

    struct d_xml       xml;
    struct d_xml_token token;
    size_t             depth      = 0;
    size_t             list_depth = 0;
    size_t             scene      = 0;
    bool               ok         = true;

    d_xml_begin(&xml, text, length);

    while ( (ok) &&
            (d_xml_next(&xml, &token)) )
    {
        // an end tag closes the scene list when it closes its element
        if (token.kind == D_XML_CLOSE)
        {
            depth      = (depth > 0) ? (depth - 1u) : 0u;
            list_depth = (depth < list_depth) ? 0u : list_depth;
            continue;
        }

        // only tags matter here
        if ( (token.kind != D_XML_START) &&
             (token.kind != D_XML_EMPTY) )
        {
            continue;
        }

        const bool opens = (token.kind == D_XML_START);

        // the scene list, a scene in it, and a slide id in a scene; an empty
        // scene list holds nothing, and opens nothing
        if ( (list_depth == 0) &&
             (opens) &&
             (d_xml_is(&token, "sceneLst")) )
        {
            list_depth = depth + 1u;
        }
        else if ( (list_depth > 0) &&
                  (d_xml_is(&token, "scene")) )
        {
            scene++;
        }
        else if ( (list_depth > 0) &&
                  (scene > 0) &&
                  (d_xml_is(&token, "sldId")) )
        {
            char* const id     = d_internal_slide_id(&xml, &token);
            char* const target = (id) ? d_internal_relationship_target(_zip,
                                                                       id)
                                      : NULL;

            free(id);

            // a slide id that resolves is a slide; one that does not is
            // skipped, since it names no part this driver can read
            if (target)
            {
                ok = d_internal_add_part(_parts, _count, target, scene,
                                         *_count);
            }
        }

        depth += (opens) ? 1u : 0u;
    }

    free(text);

    return ok ? true
              : d_internal_problem(_problem, _capacity, "out of memory",
                                   NULL);
}


/*
d_internal_compare_parts
  Orders slide parts by the number in their names, slide2 before slide10.
*/
static int
d_internal_compare_parts(
    const void* _left,
    const void* _right
)
{
    const struct d_internal_slide_part* const left  = _left;
    const struct d_internal_slide_part* const right = _right;

    return (left->order < right->order) ? -1
         : (left->order > right->order) ?  1
         :                                 0;
}


/*
d_internal_named_slides
  The fallback order: every story/slides/slideN.xml, by N.
*/
static bool
d_internal_named_slides(
    struct d_zip*                  _zip,
    struct d_internal_slide_part** _parts,
    size_t*                        _count
)
{
    static const char prefix[] = "story/slides/slide";

    for (size_t at = 0; at < d_zip_count(_zip); ++at)
    {
        const char* const name = d_zip_name(_zip, at);
        char*             end  = NULL;

        // only a slide part, named by its number, is a slide
        if ( (strncmp(name, prefix, sizeof(prefix) - 1u) != 0)   ||
             (name[sizeof(prefix) - 1u] < '0')                    ||
             (name[sizeof(prefix) - 1u] > '9') )
        {
            continue;
        }

        const unsigned long number = strtoul(&name[sizeof(prefix) - 1u],
                                             &end, 10);

        // the number must be all that stands before .xml
        if ( (!end) ||
             (strcmp(end, ".xml") != 0) )
        {
            continue;
        }

        char* const part = malloc(strlen(name) + 1u);

        // a name that cannot be held stops the list
        if ( (!part) ||
             (!d_internal_add_part(_parts, _count,
                                   memcpy(part, name, strlen(name) + 1u),
                                   1u, (size_t)number)) )
        {
            return false;
        }
    }

    qsort(*_parts, *_count, sizeof(**_parts), d_internal_compare_parts);

    return true;
}


/*
d_internal_slide_name
  The name the root <sld> of a slide part gives it; "" when it gives none.
*/
static char*
d_internal_slide_name(
    struct d_zip* _zip,
    const char*   _part
)
{
    size_t      length = 0;
    char* const text   = d_internal_read_part(_zip, _part, &length, NULL, 0);
    char*       name   = NULL;

    struct d_xml       xml;
    struct d_xml_token token;

    d_xml_begin(&xml, text, (text) ? length : 0u);

    // the first element is the root, and only an <sld> root is read
    while (d_xml_next(&xml, &token))
    {
        if ( (token.kind == D_XML_START) ||
             (token.kind == D_XML_EMPTY) )
        {
            name = (d_xml_is(&token, "sld"))
                 ? d_internal_attribute(&token, "name")
                 : NULL;
            break;
        }
    }

    free(text);

    return name;
}


// d_internal_slide_media
//   struct: the slide whose media are being added, as the visitor below is
//   handed it.
struct d_internal_slide_media
{
    struct d_story* story;
    size_t          slide;
};


/*
d_internal_add_slide_media
  Adds one media part to the slide being built, as d_internal_each_media
visits it.
*/
static bool
d_internal_add_slide_media(
    void*       _user,
    const char* _part
)
{
    struct d_internal_slide_media* const target = _user;

    return (d_story_add_media(target->story, target->slide, _part) !=
            D_STORY_NONE);
}


/*
d_internal_build
  Adds every slide, with its name and its media, to the story.
*/
static bool
d_internal_build(
    struct d_internal_articulate*       _state,
    const struct d_internal_slide_part* _parts,
    size_t                              _count
)
{
    size_t scene  = 0;
    size_t number = 0;

    for (size_t at = 0; at < _count; ++at)
    {
        // the number counts through each scene from 1
        number = (_parts[at].scene == scene) ? (number + 1u) : 1u;
        scene  = _parts[at].scene;

        char* const  name  = d_internal_slide_name(_state->zip,
                                                   _parts[at].part);
        const size_t slide = d_story_add_slide(_state->story,
                                               (name) ? name : "",
                                               scene,
                                               number);

        free(name);

        char*       directory = NULL;
        char* const rels      = d_internal_rels_of(_parts[at].part,
                                                   &directory);

        struct d_internal_slide_media target = { _state->story, slide };

        const bool ok = (slide != D_STORY_NONE) &&
                        (rels) &&
                        d_internal_each_media(_state->zip, rels, directory,
                                              d_internal_add_slide_media,
                                              &target);

        free(rels);
        free(directory);

        // a slide that could not be added stops the build
        if (!ok)
        {
            return false;
        }
    }

    return true;
}


//==============================================================================
// 5.  THE DRIVER'S OPERATIONS
//==============================================================================


/*
d_internal_claims
  A .story file, in any case.
*/
static bool
d_internal_claims(
    const char* _path
)
{
    return d_internal_ends_with(_path, ".story");
}


/*
d_internal_close
  Releases the package and the bookkeeping.
*/
static void
d_internal_close(
    void* _state
)
{
    struct d_internal_articulate* const state = _state;

    if (state)
    {
        for (size_t at = 0; at < state->use_count; ++at)
        {
            free(state->uses[at].part);
        }

        for (size_t at = 0; at < state->change_count; ++at)
        {
            free(state->changes[at].part);
        }

        d_zip_free(state->zip);
        free(state->uses);
        free(state->changes);
        free(state);
    }

    return;
}


/*
d_internal_open
  Opens the package, counts every media use, finds the slides -- by the scene
list, or else by name, saying so -- and builds the story from them.
*/
static bool
d_internal_open(
    struct d_story* _story,
    const char*     _path,
    void**          _out_state,
    char*           _problem,
    size_t          _capacity
)
{
    struct d_internal_articulate* const state = calloc(1u, sizeof(*state));

    // report the failure rather than opening half a package
    if (!state)
    {
        return d_internal_problem(_problem, _capacity, "out of memory", NULL);
    }

    state->story = _story;
    state->zip   = d_zip_open(_path, _problem, _capacity);

    struct d_internal_slide_part* parts = NULL;
    size_t                        count = 0;

    bool ok = (state->zip)                                         &&
              (d_internal_scene_slides(state->zip, &parts, &count,
                                       _problem, _capacity))       &&
              (d_internal_count_uses(state));

    // a scene list that names no slide leaves the slides' names to order them
    if ( (ok) &&
         (count == 0) )
    {
        ok = d_internal_named_slides(state->zip, &parts, &count);

        (void)d_story_note(_story, "the scene list named no slides, so the "
                                   "slides are in the order of their part "
                                   "names (slide1, slide2, ...); check the "
                                   "order before saving");
    }

    ok = (ok) && d_internal_build(state, parts, count);

    for (size_t at = 0; at < count; ++at)
    {
        free(parts[at].part);
    }

    free(parts);

    // a package that could not be read leaves no state
    if (!ok)
    {
        // a reason not yet given is given now
        if ( (_problem) &&
             (_capacity > 0) &&
             (_problem[0] == '\0') )
        {
            (void)d_internal_problem(_problem, _capacity,
                                     "cannot read the story", _path);
        }

        d_internal_close(state);

        return false;
    }

    *_out_state = state;

    return true;
}


/*
d_internal_change_of
  The record of a part's replacement, made when the part is first replaced.
*/
static struct d_internal_change*
d_internal_change_of(
    struct d_internal_articulate* _state,
    const char*                   _part,
    size_t                        _index,
    char*                         _problem,
    size_t                        _capacity
)
{
    for (size_t at = 0; at < _state->change_count; ++at)
    {
        // a part replaced before keeps the checksum the XML still holds
        if (strcmp(_state->changes[at].part, _part) == 0)
        {
            struct d_internal_change* const change = &_state->changes[at];

            // after a save the XML holds the last new checksum instead
            if (change->applied)
            {
                memcpy(change->before, change->after, 16u);
                change->applied = false;
            }

            return change;
        }
    }

    unsigned char* bytes  = NULL;
    size_t         length = 0;

    // the bytes being replaced give the checksum the XML holds for them
    if (!d_zip_read(_state->zip, _index, &bytes, &length, _problem,
                    _capacity))
    {
        return NULL;
    }

    struct d_internal_change* const grown =
        realloc(_state->changes,
                (_state->change_count + 1u) * sizeof(*grown));
    char* const part = malloc(strlen(_part) + 1u);

    // report the failure rather than losing track of a change
    if ( (!grown) ||
         (!part) )
    {
        _state->changes = (grown) ? grown : _state->changes;
        free(part);
        free(bytes);
        (void)d_internal_problem(_problem, _capacity, "out of memory", NULL);

        return NULL;
    }

    struct d_internal_change* const change = &grown[_state->change_count];

    memset(change, 0, sizeof(*change));
    memcpy(part, _part, strlen(_part) + 1u);
    change->part = part;
    d_internal_md5(bytes, length, change->before);
    free(bytes);

    // until a replacement succeeds the change rewrites a checksum to itself
    memcpy(change->after, change->before, 16u);

    _state->changes = grown;
    _state->change_count++;

    return change;
}


/*
d_internal_replace
  Refuses a part another part also uses, then overwrites the part's bytes.
*/
static bool
d_internal_replace(
    void*                _state,
    const char*          _media,
    const unsigned char* _bytes,
    size_t               _length,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_internal_articulate* const state = _state;
    const size_t index = d_zip_find(state->zip, _media);

    // a media part the package does not hold cannot be replaced
    if (index == D_ZIP_NONE)
    {
        return d_internal_problem(_problem, _capacity,
                                  "the package holds no such part", _media);
    }

    for (size_t at = 0; at < state->use_count; ++at)
    {
        // a part something else also uses would change there too
        if ( (strcmp(state->uses[at].part, _media) == 0) &&
             (state->uses[at].count > 1u) )
        {
            char message[96];

            (void)snprintf(message, sizeof(message),
                           "used %zu times in the package, so replacing it "
                           "would change them all", state->uses[at].count);

            return d_internal_problem(_problem, _capacity, _media, message);
        }
    }

    struct d_internal_change* const change =
        d_internal_change_of(state, _media, index, _problem, _capacity);

    // a replacement whose checksum cannot be tracked is not made
    if ( (!change) ||
         (!d_zip_replace(state->zip, index, _bytes, _length)) )
    {
        return (change) ? d_internal_problem(_problem, _capacity,
                                             "out of memory", NULL)
                        : false;
    }

    d_internal_md5(_bytes, _length, change->after);

    return true;
}


/*
d_internal_rewrite
  Rewrites every spelling of one checksum in one part's text, in place: each
spelling of the new checksum is as long as the old one's.  Returns how many
were rewritten.
*/
static size_t
d_internal_rewrite(
    char*                           _text,
    size_t                          _length,
    const struct d_internal_change* _change
)
{
    char before[3][33];
    char after[3][33];
    char base64_before[23];
    char base64_after[23];

    d_internal_spell(_change->before, before[0], before[1], base64_before);
    d_internal_spell(_change->after, after[0], after[1], base64_after);
    memcpy(before[2], base64_before, sizeof(base64_before));
    memcpy(after[2], base64_after, sizeof(base64_after));

    size_t rewritten = 0;

    for (size_t form = 0; form < 3u; ++form)
    {
        const size_t span = strlen(before[form]);

        for (size_t at = 0; (at + span) <= _length; ++at)
        {
            // each copy of the old spelling becomes the new one
            if (memcmp(&_text[at], before[form], span) == 0)
            {
                memcpy(&_text[at], after[form], span);
                rewritten++;
                at += span - 1u;
            }
        }
    }

    return rewritten;
}


/*
d_internal_apply_checksums
  Rewrites the checksums of every change not yet applied, in every XML part,
and notes a change whose checksum no part holds.
*/
static bool
d_internal_apply_checksums(
    struct d_internal_articulate* _state,
    char*                         _problem,
    size_t                        _capacity
)
{
    size_t* const found = calloc(_state->change_count + 1u, sizeof(*found));

    // report the failure rather than saving stale checksums
    if (!found)
    {
        return d_internal_problem(_problem, _capacity, "out of memory", NULL);
    }

    bool ok = true;

    for (size_t at = 0; (ok) && (at < d_zip_count(_state->zip)); ++at)
    {
        // only XML parts record checksums
        if (!d_internal_ends_with(d_zip_name(_state->zip, at), ".xml"))
        {
            continue;
        }

        unsigned char* bytes   = NULL;
        size_t         length  = 0;
        size_t         changed = 0;

        ok = d_zip_read(_state->zip, at, &bytes, &length, _problem,
                        _capacity);

        for (size_t which = 0; (ok) && (which < _state->change_count);
             ++which)
        {
            // a change applied by an earlier save is not applied again
            if (!_state->changes[which].applied)
            {
                const size_t count = d_internal_rewrite(
                                         (char*)bytes,
                                         length,
                                         &_state->changes[which]);

                found[which] += count;
                changed      += count;
            }
        }

        // a part whose text changed is replaced by its new text
        if ( (ok) &&
             (changed > 0) )
        {
            ok = d_zip_replace(_state->zip, at, bytes, length);
        }

        free(bytes);
    }

    for (size_t which = 0; (ok) && (which < _state->change_count); ++which)
    {
        struct d_internal_change* const change = &_state->changes[which];

        // a checksum no part held is worth knowing about
        if ( (!change->applied) &&
             (found[which] == 0) )
        {
            char note[512];

            (void)snprintf(note, sizeof(note),
                           "no checksum of %s was found in the story's XML; "
                           "Storyline may notice that it changed",
                           change->part);
            (void)d_story_note(_state->story, note);
        }

        change->applied = true;
    }

    free(found);

    return ok;
}


/*
d_internal_save
  The checksums, then the package.
*/
static bool
d_internal_save(
    void*       _state,
    const char* _path,
    char*       _problem,
    size_t      _capacity
)
{
    struct d_internal_articulate* const state = _state;

    return (d_internal_apply_checksums(state, _problem, _capacity)) &&
           (d_zip_save(state->zip, _path, _problem, _capacity));
}


/*
d_articulate_driver
  The descriptor, built once.
*/
const struct d_story_driver*
d_articulate_driver(void)
{
    static const struct d_story_driver driver =
    {
        "articulate",
        d_internal_claims,
        d_internal_open,
        d_internal_replace,
        d_internal_save,
        d_internal_close
    };

    return &driver;
}
