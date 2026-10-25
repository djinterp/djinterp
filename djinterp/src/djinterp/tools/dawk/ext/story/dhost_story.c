/*******************************************************************************
* djinterp [tools]                                                 dhost_story.c
*
* Definitions for the declarations in dhost_story.h.
*   The model owns its strings and a driver owns its format: the driver is
* handed a media file's new bytes, never the path they came from, so every
* check the model makes -- the slide, the kind, the sharing -- is made before
* a driver sees anything, and made the same for every format.
*   The host functions keep one story open at a time. A function that needs
* one when none is open is a program error and stops the run; opening,
* replacing and saving can fail in the world and report it as getline does,
* with -1, leaving story_error() to say why.
*
* path:      /src/djinterp/tools/dawk/ext/story/dhost_story.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/story/dhost_story.h"  // corresponding header
// std
#include <stdio.h>   // FILE, fopen, fread, fclose, snprintf
#include <stdlib.h>  // malloc, calloc, realloc, free, strtoul
#include <string.h>  // memcpy, memset, strchr, strcmp, strlen, strrchr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"     // d_fs_full_path
#include "../../../../../../inc/djinterp/tools/dawk/dhost.h"   // d_awk_host_function
#include "../../../../../../inc/djinterp/tools/dawk/dregex.h"  // d_regex_search_groups
#include "../../../../../../inc/djinterp/tools/dawk/dvalue.h"  // d_awk_value_text


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Model
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_media
//   struct: one media file, and how many slide references it has.
struct d_internal_media
{
    char*                   name;
    enum d_story_media_kind kind;
    size_t                  uses;
};

// 1.1.2
// d_internal_slide
//   struct: one slide, and the media it holds as indices into the story's.
struct d_internal_slide
{
    char*   name;
    size_t  scene;
    size_t  number;
    size_t* media;
    size_t  media_count;
    size_t  media_capacity;
};

// 1.1.3
// d_story
//   struct: the model, the driver that read it, and the driver's state.
struct d_story
{
    const struct d_story_driver* driver;
    void*                        state;
    char*                        path;
    struct d_internal_slide*     slides;
    size_t                       slide_count;
    size_t                       slide_capacity;
    struct d_internal_media*     media;
    size_t                       media_count;
    size_t                       media_capacity;
    size_t                       changes;
    char*                        notes;
};

// 1.2    Extension
//------------------------------------------------------------------------------
// 1.2.1
// d_host_story
//   struct: the drivers, the story open, the last failure, and the rule last
//   compiled, kept because a program applies one rule to every file.
struct d_host_story
{
    const struct d_story_driver** drivers;
    size_t                        driver_count;
    struct d_story*               story;
    char                          error[256];
    char*                         expression;
    struct d_regex*               regex;
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
d_internal_copy
  Copies a string; NULL copies as "".
*/
static char*
d_internal_copy(
    const char* _text
)
{
    const char*  text   = (_text) ? _text : "";
    const size_t length = strlen(text);
    char* const  copy   = malloc(length + 1u);

    // a copy that cannot be held is reported by its absence
    if (copy)
    {
        memcpy(copy, text, length + 1u);
    }

    return copy;
}


/*
d_internal_base_name
  The last part of a path, after any `/` or `\`: either separator, since a
path from Windows may hold both.
*/
static const char*
d_internal_base_name(
    const char* _path
)
{
    const char* base = _path;

    for (const char* at = _path; *at != '\0'; ++at)
    {
        // a separator moves the base past it
        if ( (*at == '/') ||
             (*at == '\\') )
        {
            base = at + 1;
        }
    }

    return base;
}


/*
d_internal_extension
  The extension of a path's last part, without its dot; "" when it has none.
*/
static const char*
d_internal_extension(
    const char* _path
)
{
    const char* const base = d_internal_base_name(_path);
    const char* const dot  = strrchr(base, '.');

    return ( (dot) &&
             (dot != base) ) ? (dot + 1) : "";
}


/*
d_internal_same_text
  Compares two strings ignoring ASCII case, as extensions are compared.
*/
static bool
d_internal_same_text(
    const char* _left,
    const char* _right
)
{
    // the strings are walked together to the first difference
    for (; (*_left != '\0') && (*_right != '\0'); ++_left, ++_right)
    {
        const char left  = ((*_left >= 'A') && (*_left <= 'Z'))
                         ? (char)(*_left - 'A' + 'a') : *_left;
        const char right = ((*_right >= 'A') && (*_right <= 'Z'))
                         ? (char)(*_right - 'A' + 'a') : *_right;

        if (left != right)
        {
            return false;
        }
    }

    return (*_left == *_right);
}


/*
d_internal_kind_of
  A media file's kind, from its extension.
*/
static enum d_story_media_kind
d_internal_kind_of(
    const char* _name
)
{
    static const char* const audio[] =
    {
        "mp3", "wav", "m4a", "aac", "ogg", "oga", "wma", "flac", "aif", "aiff"
    };
    static const char* const video[] =
    {
        "mp4", "m4v", "mov", "wmv", "avi", "webm", "flv", "mkv", "mpg", "mpeg"
    };
    static const char* const image[] =
    {
        "png", "jpg", "jpeg", "gif", "bmp", "svg", "webp", "tif", "tiff",
        "emf", "wmf"
    };

    const char* const extension = d_internal_extension(_name);

    for (size_t at = 0; at < (sizeof(audio) / sizeof(audio[0])); ++at)
    {
        if (d_internal_same_text(extension, audio[at]))
        {
            return D_STORY_MEDIA_AUDIO;
        }
    }

    for (size_t at = 0; at < (sizeof(video) / sizeof(video[0])); ++at)
    {
        if (d_internal_same_text(extension, video[at]))
        {
            return D_STORY_MEDIA_VIDEO;
        }
    }

    for (size_t at = 0; at < (sizeof(image) / sizeof(image[0])); ++at)
    {
        if (d_internal_same_text(extension, image[at]))
        {
            return D_STORY_MEDIA_IMAGE;
        }
    }

    return D_STORY_MEDIA_OTHER;
}


/*
d_internal_slurp
  Reads a whole file into memory.
*/
static unsigned char*
d_internal_slurp(
    const char* _path,
    size_t*     _out_length
)
{
    FILE* const stream = fopen(_path, "rb");

    // a file that will not open has no bytes
    if (!stream)
    {
        return NULL;
    }

    size_t         capacity = 65536u;
    size_t         used     = 0;
    unsigned char* buffer   = malloc(capacity);

    while (buffer)
    {
        used += fread(&buffer[used], 1u, capacity - used, stream);

        // a short read means the file is exhausted
        if (used < capacity)
        {
            break;
        }

        unsigned char* const grown = realloc(buffer, capacity * 2u);

        // abandon the read when the buffer cannot grow
        if (!grown)
        {
            free(buffer);
            buffer = NULL;
            break;
        }

        buffer    = grown;
        capacity *= 2u;
    }

    // a read error is a file that could not be read
    if ( (buffer) &&
         (ferror(stream)) )
    {
        free(buffer);
        buffer = NULL;
    }

    (void)fclose(stream);
    *_out_length = used;

    return buffer;
}


//==============================================================================
// 3.  BUILDING
//==============================================================================


/*
d_story_add_slide
  The slide takes the next ordinal; the driver adds them in story order.
*/
size_t
d_story_add_slide(
    struct d_story* _story,
    const char*     _name,
    size_t          _scene,
    size_t          _number
)
{
    // parameter validation first
    if (!_story)
    {
        return D_STORY_NONE;
    }

    // grow the slides when they are full
    if (_story->slide_count == _story->slide_capacity)
    {
        const size_t capacity = (_story->slide_capacity == 0)
                              ? 32u
                              : (_story->slide_capacity * 2u);

        struct d_internal_slide* const grown =
            realloc(_story->slides, capacity * sizeof(*grown));

        // report the failure rather than losing a slide
        if (!grown)
        {
            return D_STORY_NONE;
        }

        _story->slides         = grown;
        _story->slide_capacity = capacity;
    }

    struct d_internal_slide* const slide = &_story->slides[_story->slide_count];

    memset(slide, 0, sizeof(*slide));
    slide->name   = d_internal_copy(_name);
    slide->scene  = _scene;
    slide->number = _number;

    // a slide without its name is not added
    if (!slide->name)
    {
        return D_STORY_NONE;
    }

    return _story->slide_count++;
}


/*
d_internal_find_media
  The index of a media file by name, or D_STORY_NONE.
*/
static size_t
d_internal_find_media(
    const struct d_story* _story,
    const char*           _name
)
{
    for (size_t at = 0; at < _story->media_count; ++at)
    {
        if (strcmp(_story->media[at].name, _name) == 0)
        {
            return at;
        }
    }

    return D_STORY_NONE;
}


/*
d_internal_new_media
  Adds a media file the story has not seen.
*/
static size_t
d_internal_new_media(
    struct d_story* _story,
    const char*     _name
)
{
    // grow the media when they are full
    if (_story->media_count == _story->media_capacity)
    {
        const size_t capacity = (_story->media_capacity == 0)
                              ? 32u
                              : (_story->media_capacity * 2u);

        struct d_internal_media* const grown =
            realloc(_story->media, capacity * sizeof(*grown));

        // report the failure rather than losing a file
        if (!grown)
        {
            return D_STORY_NONE;
        }

        _story->media          = grown;
        _story->media_capacity = capacity;
    }

    struct d_internal_media* const media = &_story->media[_story->media_count];

    media->name = d_internal_copy(_name);
    media->kind = d_internal_kind_of(_name);
    media->uses = 0;

    // a file without its name is not added
    if (!media->name)
    {
        return D_STORY_NONE;
    }

    return _story->media_count++;
}


/*
d_story_add_media
  A file a second slide names is the same file, used again: that count is
what stops a replacement through one slide from changing another.
*/
size_t
d_story_add_media(
    struct d_story* _story,
    size_t          _slide,
    const char*     _media
)
{
    // parameter validation first
    if ( (!_story)                        ||
         (_slide >= _story->slide_count)  ||
         (!_media) )
    {
        return D_STORY_NONE;
    }

    size_t index = d_internal_find_media(_story, _media);

    // a file seen for the first time is added
    if (index == D_STORY_NONE)
    {
        index = d_internal_new_media(_story, _media);

        // abandon the reference when the file could not be added
        if (index == D_STORY_NONE)
        {
            return D_STORY_NONE;
        }
    }

    struct d_internal_slide* const slide = &_story->slides[_slide];

    // a slide naming one file twice still uses it once
    for (size_t at = 0; at < slide->media_count; ++at)
    {
        if (slide->media[at] == index)
        {
            return index;
        }
    }

    // grow the slide's references when they are full
    if (slide->media_count == slide->media_capacity)
    {
        const size_t capacity = (slide->media_capacity == 0)
                              ? 4u
                              : (slide->media_capacity * 2u);

        size_t* const grown = realloc(slide->media, capacity * sizeof(*grown));

        // report the failure rather than losing a reference
        if (!grown)
        {
            return D_STORY_NONE;
        }

        slide->media          = grown;
        slide->media_capacity = capacity;
    }

    slide->media[slide->media_count++] = index;
    _story->media[index].uses++;

    return index;
}


/*
d_story_note
  Appends a line to the notes; a note that cannot be held is dropped, since a
note is never what a run depends on.
*/
bool
d_story_note(
    struct d_story* _story,
    const char*     _note
)
{
    // parameter validation first
    if ( (!_story) ||
         (!_note) )
    {
        return false;
    }

    const size_t had    = (_story->notes) ? strlen(_story->notes) : 0u;
    const size_t length = strlen(_note);
    char* const  grown  = realloc(_story->notes, had + length + 2u);

    // a note that cannot be held is dropped
    if (!grown)
    {
        return false;
    }

    memcpy(&grown[had], _note, length);
    grown[had + length]      = '\n';
    grown[had + length + 1u] = '\0';
    _story->notes            = grown;

    return true;
}


//==============================================================================
// 4.  OPENING AND SAVING
//==============================================================================


/*
d_story_open
  The first driver to claim the file reads it; a driver that claims it and
cannot read it is the answer, and no other is asked, since a file that is
plainly one format's and damaged is not another format's.
*/
struct d_story*
d_story_open(
    const char*                         _path,
    const struct d_story_driver* const* _drivers,
    size_t                              _count,
    char*                               _problem,
    size_t                              _capacity
)
{
    // parameter validation first
    if (!_path)
    {
        (void)d_internal_problem(_problem, _capacity, "no story named", NULL);

        return NULL;
    }

    const struct d_story_driver* driver = NULL;

    for (size_t at = 0; (at < _count) && (!driver); ++at)
    {
        // the first driver to claim the file is the one used
        if ( (_drivers[at]) &&
             (_drivers[at]->claims(_path)) )
        {
            driver = _drivers[at];
        }
    }

    // a file no driver claims is no story this build can read
    if (!driver)
    {
        (void)d_internal_problem(_problem, _capacity,
                                 "no driver reads this kind of file", _path);

        return NULL;
    }

    struct d_story* const story = calloc(1u, sizeof(*story));
    char* const           path  = d_internal_copy(_path);

    // report the failure rather than opening half a story
    if ( (!story) ||
         (!path) )
    {
        free(story);
        free(path);
        (void)d_internal_problem(_problem, _capacity, "out of memory", NULL);

        return NULL;
    }

    story->driver = driver;
    story->path   = path;

    // the driver reads the file into the model, or the story is not opened
    if (!driver->open(story, _path, &story->state, _problem, _capacity))
    {
        d_story_free(story);

        return NULL;
    }

    return story;
}


/*
d_story_free
  The driver's state goes first, then the model it filled.
*/
void
d_story_free(
    struct d_story* _story
)
{
    if (_story)
    {
        // a driver that opened the story releases what it kept
        if ( (_story->state) &&
             (_story->driver->close) )
        {
            _story->driver->close(_story->state);
        }

        for (size_t at = 0; at < _story->slide_count; ++at)
        {
            free(_story->slides[at].name);
            free(_story->slides[at].media);
        }

        for (size_t at = 0; at < _story->media_count; ++at)
        {
            free(_story->media[at].name);
        }

        free(_story->slides);
        free(_story->media);
        free(_story->path);
        free(_story->notes);
        free(_story);
    }

    return;
}


/*
d_internal_same_file
  Whether two paths name one file: by spelling, and by full name where both
have one.  A path that names nothing yet cannot be a file that exists.
*/
static bool
d_internal_same_file(
    const char* _left,
    const char* _right
)
{
    // one spelling is one file
    if (strcmp(_left, _right) == 0)
    {
        return true;
    }

    char* const left  = d_fs_full_path(_left);
    char* const right = d_fs_full_path(_right);
    const bool  same  = ( (left) &&
                          (right) &&
                          (d_internal_same_text(left, right)) );

    free(left);
    free(right);

    return same;
}


/*
d_story_save
  The path is compared with the story's own, by full name, before the driver
is asked: on Windows names differ in case only, so the comparison ignores it.
*/
bool
d_story_save(
    struct d_story* _story,
    const char*     _path,
    char*           _problem,
    size_t          _capacity
)
{
    // parameter validation first
    if ( (!_story) ||
         (!_path)  ||
         (_path[0] == '\0') )
    {
        return d_internal_problem(_problem, _capacity, "no file to save to",
                                  NULL);
    }

    // the file the story was read from is never written
    if (d_internal_same_file(_story->path, _path))
    {
        return d_internal_problem(_problem, _capacity,
                                  "will not save over the story it was read "
                                  "from", _path);
    }

    return _story->driver->save(_story->state, _path, _problem, _capacity);
}


/*
d_story_changes
  Replacements made since the story was opened.
*/
size_t
d_story_changes(
    const struct d_story* _story
)
{
    return (_story) ? _story->changes : 0u;
}


/*
d_story_notes
  Every note so far, one per line; "" when there are none.
*/
const char*
d_story_notes(
    const struct d_story* _story
)
{
    return ( (_story) &&
             (_story->notes) ) ? _story->notes : "";
}


/*
d_story_driver_name
  The name of the driver that read the story.
*/
const char*
d_story_driver_name(
    const struct d_story* _story
)
{
    return (_story) ? _story->driver->name : "";
}


//==============================================================================
// 5.  SLIDES
//==============================================================================


/*
d_story_slide_count
  Every slide, in story order.
*/
size_t
d_story_slide_count(
    const struct d_story* _story
)
{
    return (_story) ? _story->slide_count : 0u;
}


/*
d_internal_number
  Reads a run of decimal digits as a number, refusing an empty run, any other
byte, and a value past what an index holds.
*/
static bool
d_internal_number(
    const char* _text,
    size_t      _length,
    size_t*     _out
)
{
    size_t value = 0;

    // an empty run is no number
    if (_length == 0)
    {
        return false;
    }

    for (size_t at = 0; at < _length; ++at)
    {
        // a byte that is not a digit, or a value that would wrap, is refused
        if ( (_text[at] < '0')                                 ||
             (_text[at] > '9')                                 ||
             (value > (((size_t)-1 - 9u) / 10u)) )
        {
            return false;
        }

        value = (value * 10u) + (size_t)(_text[at] - '0');
    }

    *_out = value;

    return true;
}


/*
d_story_resolve
  Digits first, since a slide named "8" is rarer than a rule saying 8; then
scene and slide; then a name.
*/
size_t
d_story_resolve(
    const struct d_story* _story,
    const char*           _key
)
{
    // parameter validation first
    if ( (!_story) ||
         (!_key) )
    {
        return 0;
    }

    const size_t length = strlen(_key);
    size_t       number = 0;

    // digits are an ordinal through the whole story
    if (d_internal_number(_key, length, &number))
    {
        return ( (number >= 1u) &&
                 (number <= _story->slide_count) ) ? number : 0u;
    }

    const char* const dot   = strchr(_key, '.');
    size_t            scene = 0;

    // S.N is slide N of scene S
    if ( (dot) &&
         (d_internal_number(_key, (size_t)(dot - _key), &scene)) &&
         (d_internal_number(dot + 1, strlen(dot + 1), &number)) )
    {
        for (size_t at = 0; at < _story->slide_count; ++at)
        {
            if ( (_story->slides[at].scene == scene) &&
                 (_story->slides[at].number == number) )
            {
                return at + 1u;
            }
        }

        return 0;
    }

    // anything else is a slide's name
    for (size_t at = 0; at < _story->slide_count; ++at)
    {
        if (strcmp(_story->slides[at].name, _key) == 0)
        {
            return at + 1u;
        }
    }

    return 0;
}


/*
d_internal_slide
  The slide an ordinal names, or NULL.
*/
static const struct d_internal_slide*
d_internal_slide(
    const struct d_story* _story,
    size_t                _slide
)
{
    return ( (_story)                        &&
             (_slide >= 1u)                  &&
             (_slide <= _story->slide_count) ) ? &_story->slides[_slide - 1u]
                                               : NULL;
}


/*
d_story_slide_name, d_story_slide_scene, d_story_slide_number
  What the driver recorded for a slide.
*/
const char*
d_story_slide_name(
    const struct d_story* _story,
    size_t                _slide
)
{
    const struct d_internal_slide* const slide = d_internal_slide(_story,
                                                                  _slide);

    return (slide) ? slide->name : "";
}


size_t
d_story_slide_scene(
    const struct d_story* _story,
    size_t                _slide
)
{
    const struct d_internal_slide* const slide = d_internal_slide(_story,
                                                                  _slide);

    return (slide) ? slide->scene : 0u;
}


size_t
d_story_slide_number(
    const struct d_story* _story,
    size_t                _slide
)
{
    const struct d_internal_slide* const slide = d_internal_slide(_story,
                                                                  _slide);

    return (slide) ? slide->number : 0u;
}


//==============================================================================
// 6.  MEDIA
//==============================================================================


/*
d_internal_audio
  The index into the story's media of a slide's k-th audio file, or
D_STORY_NONE.
*/
static size_t
d_internal_audio(
    const struct d_story* _story,
    size_t                _slide,
    size_t                _audio
)
{
    const struct d_internal_slide* const slide = d_internal_slide(_story,
                                                                  _slide);
    size_t                               seen  = 0;

    for (size_t at = 0; (slide) && (at < slide->media_count); ++at)
    {
        const size_t index = slide->media[at];

        // only audio is counted, in the slide's order
        if ( (_story->media[index].kind == D_STORY_MEDIA_AUDIO) &&
             (++seen == _audio) )
        {
            return index;
        }
    }

    return D_STORY_NONE;
}


/*
d_story_audio_count
  A slide's audio files.
*/
size_t
d_story_audio_count(
    const struct d_story* _story,
    size_t                _slide
)
{
    const struct d_internal_slide* const slide = d_internal_slide(_story,
                                                                  _slide);
    size_t                               count = 0;

    for (size_t at = 0; (slide) && (at < slide->media_count); ++at)
    {
        count += (_story->media[slide->media[at]].kind == D_STORY_MEDIA_AUDIO)
               ? 1u
               : 0u;
    }

    return count;
}


/*
d_story_audio_name
  The name the format gives a slide's k-th audio file.
*/
const char*
d_story_audio_name(
    const struct d_story* _story,
    size_t                _slide,
    size_t                _audio
)
{
    const size_t index = d_internal_audio(_story, _slide, _audio);

    return (index != D_STORY_NONE) ? _story->media[index].name : "";
}


/*
d_story_replace_audio
  Every check is made before the file is read: a refusal costs nothing, and a
rule that matches a thousand files is refused a thousand times quickly.
*/
enum d_story_result
d_story_replace_audio(
    struct d_story* _story,
    size_t          _slide,
    size_t          _audio,
    const char*     _source,
    char*           _problem,
    size_t          _capacity
)
{
    // parameter validation first
    if ( (!_story) ||
         (!_source) )
    {
        (void)d_internal_problem(_problem, _capacity, "no story or no file",
                                 NULL);

        return D_STORY_FAILED;
    }

    const size_t index = d_internal_audio(_story, _slide, _audio);

    // a slide or an audio file that is not there is absent, not failed
    if (index == D_STORY_NONE)
    {
        return D_STORY_ABSENT;
    }

    const struct d_internal_media* const media = &_story->media[index];

    // a file two slides use cannot be replaced through one of them
    if (media->uses > 1u)
    {
        char message[96];

        (void)snprintf(message, sizeof(message),
                       "used by %zu slides, so replacing it would change "
                       "them all", media->uses);
        (void)d_internal_problem(_problem, _capacity, media->name, message);

        return D_STORY_FAILED;
    }

    // audio is replaced only by a file of its own kind
    if (!d_internal_same_text(d_internal_extension(_source),
                              d_internal_extension(media->name)))
    {
        char message[96];

        (void)snprintf(message, sizeof(message),
                       "a .%s file cannot replace .%s audio",
                       d_internal_extension(_source),
                       d_internal_extension(media->name));
        (void)d_internal_problem(_problem, _capacity, _source, message);

        return D_STORY_FAILED;
    }

    size_t               length = 0;
    unsigned char* const bytes  = d_internal_slurp(_source, &length);

    // a file that cannot be read replaces nothing
    if (!bytes)
    {
        (void)d_internal_problem(_problem, _capacity, "cannot read", _source);

        return D_STORY_FAILED;
    }

    const bool replaced = _story->driver->replace(_story->state,
                                                  media->name,
                                                  bytes,
                                                  length,
                                                  _problem,
                                                  _capacity);

    free(bytes);

    // a driver that refused or failed has the last word
    if (!replaced)
    {
        return D_STORY_FAILED;
    }

    _story->changes++;

    return D_STORY_REPLACED;
}


//==============================================================================
// 7.  RULES
//==============================================================================


/*
d_internal_key
  Takes the key out of a match: the group asked for, when one is and it took
part, or else the last run of digits within the match, short of the name's
extension: the 3 of .mp3 is no slide number.
*/
static void
d_internal_key(
    const char*                 _name,
    const struct d_regex_match* _groups,
    size_t                      _group,
    char*                       _key,
    size_t                      _key_size
)
{
    size_t start  = 0;
    size_t length = 0;

    // a group asked for is the key, whatever it holds; one that took no
    // part in the match gives none
    if (_group > 0)
    {
        start  = _groups[_group].start;
        length = (start != D_REGEX_NO_MATCH) ? _groups[_group].length : 0u;
        start  = (start != D_REGEX_NO_MATCH) ? start : 0u;
    }
    else
    {
        const char* const dot = strrchr(_name, '.');
        size_t            at  = _groups[0].start + _groups[0].length;

        // the extension, when the match reaches into it, is left out
        if ( (dot) &&
             ((size_t)(dot - _name) > _groups[0].start) &&
             ((size_t)(dot - _name) < at) )
        {
            at = (size_t)(dot - _name);
        }

        // the last digit of the match ends the run
        while ( (at > _groups[0].start) &&
                ( (_name[at - 1u] < '0') || (_name[at - 1u] > '9') ) )
        {
            at--;
        }

        const size_t end = at;

        // and the run goes back to the first digit before it
        while ( (at > _groups[0].start) &&
                (_name[at - 1u] >= '0') &&
                (_name[at - 1u] <= '9') )
        {
            at--;
        }

        start  = at;
        length = end - at;
    }

    // a key longer than its buffer is cut, and still terminated
    if (length >= _key_size)
    {
        length = _key_size - 1u;
    }

    memcpy(_key, &_name[start], length);
    _key[length] = '\0';

    return;
}


/*
d_internal_match_with
  Matches a compiled rule against a name and takes the key out.
*/
static bool
d_internal_match_with(
    struct d_regex* _regex,
    const char*     _path,
    size_t          _group,
    char*           _key,
    size_t          _key_size,
    char*           _problem,
    size_t          _capacity
)
{
    const char* const    name  = d_internal_base_name(_path);
    struct d_regex_match groups[D_REGEX_GROUP_MAX + 1];

    _key[0] = '\0';

    // a group the expression does not have cannot be the key
    if (_group > d_regex_group_count(_regex))
    {
        return d_internal_problem(_problem, _capacity,
                                  "the rule has no such group", NULL);
    }

    const enum d_regex_status status = d_regex_search_groups(_regex,
                                                             name,
                                                             strlen(name),
                                                             0,
                                                             groups,
                                                             _group + 1u);

    // a pattern too costly to match is reported, not taken as no match
    if (status != D_REGEX_OK)
    {
        return d_internal_problem(_problem, _capacity,
                                  d_regex_status_text(status), _path);
    }

    // a name the rule does not match has no key
    if (groups[0].start != D_REGEX_NO_MATCH)
    {
        d_internal_key(name, groups, _group, _key, _key_size);
    }

    return true;
}


/*
d_story_match
  Compiled for one call; the extension keeps its compiled rule between calls.
*/
bool
d_story_match(
    const char* _expression,
    const char* _path,
    size_t      _group,
    char*       _key,
    size_t      _key_size,
    char*       _problem,
    size_t      _capacity
)
{
    // parameter validation first
    if ( (!_expression) ||
         (!_path)       ||
         (!_key)        ||
         (_key_size == 0) )
    {
        return d_internal_problem(_problem, _capacity, "nothing to match",
                                  NULL);
    }

    enum d_regex_status   status = D_REGEX_OK;
    struct d_regex* const regex  = d_regex_compile(_expression, &status);

    // a rule that does not compile is a mistake in the rule
    if (!regex)
    {
        _key[0] = '\0';

        return d_internal_problem(_problem, _capacity,
                                  d_regex_status_text(status), _expression);
    }

    const bool ok = d_internal_match_with(regex, _path, _group, _key,
                                          _key_size, _problem, _capacity);

    d_regex_free(regex);

    return ok;
}


//==============================================================================
// 8.  HOST FUNCTIONS
//==============================================================================


/*
d_internal_text, d_internal_index
  An argument's text, and an argument as a 1-based index: a value below 1,
or none at all, is 0, which names nothing.
*/
static const char*
d_internal_text(
    struct d_awk_value* _value
)
{
    return d_awk_value_text(_value, NULL, NULL);
}


static size_t
d_internal_index(
    struct d_awk_value* _value
)
{
    const double number = d_awk_value_number(_value);

    // NaN, and anything below 1, names nothing
    if (!(number >= 1.0))
    {
        return 0u;
    }

    // a value past what an index holds names nothing either
    return (number < 4294967296.0) ? (size_t)number : 0u;
}


/*
d_internal_needs_story
  The functions that read or change a story need one open; calling them
without one is a mistake in the program, and stops it.
*/
static bool
d_internal_needs_story(
    const struct d_host_story* _host,
    const char*                _function,
    char*                      _problem,
    size_t                     _capacity
)
{
    // a story open is all it takes
    if (_host->story)
    {
        return true;
    }

    (void)snprintf(_problem, _capacity,
                   "no story is open; call story_open before %s", _function);

    return false;
}


/*
d_internal_fail_soft
  Records a failure the program can recover from, and gives -1.
*/
static bool
d_internal_fail_soft(
    struct d_host_story* _host,
    struct d_awk_value*  _out
)
{
    // an empty reason is still a failure, said as one
    if (_host->error[0] == '\0')
    {
        (void)snprintf(_host->error, sizeof(_host->error), "failed");
    }

    d_awk_value_set_number(_out, -1.0);

    return true;
}


/*
story_open(path)
  The slide count, or -1; a story already open is closed first, unsaved
changes and all, as a program that opens a second has finished the first.
*/
static bool
d_internal_story_open(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    (void)_count;
    (void)_problem;
    (void)_capacity;

    d_story_free(host->story);
    host->error[0] = '\0';
    host->story    = d_story_open(d_internal_text(_args[0]),
                                  host->drivers,
                                  host->driver_count,
                                  host->error,
                                  sizeof(host->error));

    // a story that will not open is reported to the program
    if (!host->story)
    {
        return d_internal_fail_soft(host, _out);
    }

    d_awk_value_set_number(_out, (double)d_story_slide_count(host->story));

    return true;
}


/*
story_close()
  Always 1; closing nothing is not a mistake.
*/
static bool
d_internal_story_close(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    (void)_args;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    d_story_free(host->story);
    host->story = NULL;
    d_awk_value_set_number(_out, 1.0);

    return true;
}


/*
story_error()
  Why the last open, replacement or save failed; "" after one that worked.
*/
static bool
d_internal_story_error(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    (void)_args;
    (void)_count;
    (void)_problem;
    (void)_capacity;

    return d_awk_value_set_string(_out, host->error, strlen(host->error));
}


/*
story_notes()
  What the driver noted of the story open, one note per line; "" with none.
*/
static bool
d_internal_story_notes(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    (void)_args;
    (void)_count;

    // the notes on a story need one
    if (!d_internal_needs_story(host, "story_notes", _problem, _capacity))
    {
        return false;
    }

    const char* const notes = d_story_notes(host->story);

    return d_awk_value_set_string(_out, notes, strlen(notes));
}


/*
story_slides(), story_changes()
  The slide count, and the replacements not yet saved.
*/
static bool
d_internal_story_slides(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    (void)_args;
    (void)_count;

    // the count of a story needs one
    if (!d_internal_needs_story(host, "story_slides", _problem, _capacity))
    {
        return false;
    }

    d_awk_value_set_number(_out, (double)d_story_slide_count(host->story));

    return true;
}


static bool
d_internal_story_changes(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    (void)_args;
    (void)_count;

    // the changes to a story need one
    if (!d_internal_needs_story(host, "story_changes", _problem, _capacity))
    {
        return false;
    }

    d_awk_value_set_number(_out, (double)d_story_changes(host->story));

    return true;
}


/*
story_slide(key)
  The slide a key names, from 1, or 0.
*/
static bool
d_internal_story_slide(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    (void)_count;

    // resolving a key needs a story to resolve it in
    if (!d_internal_needs_story(host, "story_slide", _problem, _capacity))
    {
        return false;
    }

    d_awk_value_set_number(_out,
                           (double)d_story_resolve(host->story,
                                                   d_internal_text(_args[0])));

    return true;
}


/*
story_slide_name(n), story_slide_scene(n), story_slide_number(n)
  What the story says of a slide; "" or 0 for one that is not there.
*/
static bool
d_internal_story_slide_name(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    (void)_count;

    // a slide's name needs a story
    if (!d_internal_needs_story(host, "story_slide_name", _problem, _capacity))
    {
        return false;
    }

    const char* const name = d_story_slide_name(host->story,
                                                d_internal_index(_args[0]));

    return d_awk_value_set_input(_out, name, strlen(name));
}


static bool
d_internal_story_slide_scene(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    (void)_count;

    // a slide's scene needs a story
    if (!d_internal_needs_story(host, "story_slide_scene", _problem,
                                _capacity))
    {
        return false;
    }

    d_awk_value_set_number(_out,
                           (double)d_story_slide_scene(
                               host->story, d_internal_index(_args[0])));

    return true;
}


static bool
d_internal_story_slide_number(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    (void)_count;

    // a slide's number needs a story
    if (!d_internal_needs_story(host, "story_slide_number", _problem,
                                _capacity))
    {
        return false;
    }

    d_awk_value_set_number(_out,
                           (double)d_story_slide_number(
                               host->story, d_internal_index(_args[0])));

    return true;
}


/*
story_audio(n), story_audio_name(n [, k])
  How many audio files a slide holds, and the name of its k-th, 1 by default.
*/
static bool
d_internal_story_audio(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    (void)_count;

    // a slide's audio needs a story
    if (!d_internal_needs_story(host, "story_audio", _problem, _capacity))
    {
        return false;
    }

    d_awk_value_set_number(_out,
                           (double)d_story_audio_count(
                               host->story, d_internal_index(_args[0])));

    return true;
}


static bool
d_internal_story_audio_name(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    // a slide's audio needs a story
    if (!d_internal_needs_story(host, "story_audio_name", _problem,
                                _capacity))
    {
        return false;
    }

    const size_t      audio = (_count > 1u) ? d_internal_index(_args[1]) : 1u;
    const char* const name  = d_story_audio_name(host->story,
                                                 d_internal_index(_args[0]),
                                                 audio);

    return d_awk_value_set_input(_out, name, strlen(name));
}


/*
story_replace_audio(n, file [, k])
  1 when replaced, 0 when the slide or its k-th audio is not there, -1 when
refused or failed, with story_error() saying why.
*/
static bool
d_internal_story_replace_audio(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    // replacing audio needs a story to replace it in
    if (!d_internal_needs_story(host, "story_replace_audio", _problem,
                                _capacity))
    {
        return false;
    }

    const size_t audio = (_count > 2u) ? d_internal_index(_args[2]) : 1u;

    host->error[0] = '\0';

    const enum d_story_result result =
        d_story_replace_audio(host->story,
                              d_internal_index(_args[0]),
                              audio,
                              d_internal_text(_args[1]),
                              host->error,
                              sizeof(host->error));

    // a refusal or a failure is the program's to handle
    if (result == D_STORY_FAILED)
    {
        return d_internal_fail_soft(host, _out);
    }

    d_awk_value_set_number(_out, (result == D_STORY_REPLACED) ? 1.0 : 0.0);

    return true;
}


/*
story_save(path)
  1 when written, -1 otherwise.
*/
static bool
d_internal_story_save(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host = _user;

    (void)_count;

    // saving needs a story to save
    if (!d_internal_needs_story(host, "story_save", _problem, _capacity))
    {
        return false;
    }

    host->error[0] = '\0';

    // a save that failed is the program's to handle
    if (!d_story_save(host->story,
                      d_internal_text(_args[0]),
                      host->error,
                      sizeof(host->error)))
    {
        return d_internal_fail_soft(host, _out);
    }

    d_awk_value_set_number(_out, 1.0);

    return true;
}


/*
story_match(expression, name [, group])
  The key a rule reads out of a file's name, or "" when it does not match.
The key is input-derived text, so "08" compares equal to 8.  A rule that does
not compile is a mistake in the program, and stops it.
*/
static bool
d_internal_story_match(
    void*                _user,
    struct d_awk_value** _args,
    size_t               _count,
    struct d_awk_value*  _out,
    char*                _problem,
    size_t               _capacity
)
{
    struct d_host_story* const host       = _user;
    const char* const          expression = d_internal_text(_args[0]);
    const size_t               group      = (_count > 2u)
                                          ? d_internal_index(_args[2])
                                          : 0u;

    // a rule other than the one compiled last is compiled in its place
    if ( (!host->expression) ||
         (strcmp(host->expression, expression) != 0) )
    {
        enum d_regex_status   status = D_REGEX_OK;
        struct d_regex* const regex  = d_regex_compile(expression, &status);
        char* const           copy   = d_internal_copy(expression);

        // a rule that does not compile, or cannot be kept, stops the program
        if ( (!regex) ||
             (!copy) )
        {
            d_regex_free(regex);
            free(copy);

            return d_internal_problem(_problem, _capacity,
                                      (regex) ? "out of memory"
                                              : d_regex_status_text(status),
                                      expression);
        }

        d_regex_free(host->regex);
        free(host->expression);
        host->regex      = regex;
        host->expression = copy;
    }

    char key[1024];

    // a match too costly to make stops the program as well
    if (!d_internal_match_with(host->regex,
                               d_internal_text(_args[1]),
                               group,
                               key,
                               sizeof(key),
                               _problem,
                               _capacity))
    {
        return false;
    }

    return d_awk_value_set_input(_out, key, strlen(key));
}


// D_INTERNAL_STORY_FUNCTIONS
//   constant: how many host functions the extension adds.
#define D_INTERNAL_STORY_FUNCTIONS 15

/*
d_internal_functions
  The extension's host functions, as a program calls them.
*/
static const struct d_awk_host_function*
d_internal_functions(void)
{
    static const struct d_awk_host_function functions[
        D_INTERNAL_STORY_FUNCTIONS] =
    {
        { "story_open",          1u, 1u, d_internal_story_open          },
        { "story_close",         0u, 0u, d_internal_story_close         },
        { "story_error",         0u, 0u, d_internal_story_error         },
        { "story_notes",         0u, 0u, d_internal_story_notes         },
        { "story_slides",        0u, 0u, d_internal_story_slides        },
        { "story_changes",       0u, 0u, d_internal_story_changes       },
        { "story_slide",         1u, 1u, d_internal_story_slide         },
        { "story_slide_name",    1u, 1u, d_internal_story_slide_name    },
        { "story_slide_scene",   1u, 1u, d_internal_story_slide_scene   },
        { "story_slide_number",  1u, 1u, d_internal_story_slide_number  },
        { "story_audio",         1u, 1u, d_internal_story_audio         },
        { "story_audio_name",    1u, 2u, d_internal_story_audio_name    },
        { "story_replace_audio", 2u, 3u, d_internal_story_replace_audio },
        { "story_save",          1u, 1u, d_internal_story_save          },
        { "story_match",         2u, 3u, d_internal_story_match         }
    };

    return functions;
}


//==============================================================================
// 9.  THE EXTENSION
//==============================================================================


/*
d_host_story_new
  An extension with no drivers, which can open nothing until one is added.
*/
struct d_host_story*
d_host_story_new(void)
{
    return calloc(1u, sizeof(struct d_host_story));
}


/*
d_host_story_free
  Closes the story open, unsaved changes and all.
*/
void
d_host_story_free(
    struct d_host_story* _host
)
{
    if (_host)
    {
        d_story_free(_host->story);
        d_regex_free(_host->regex);
        free(_host->expression);
        free(_host->drivers);
        free(_host);
    }

    return;
}


/*
d_host_story_add_driver
  A driver missing any of its operations is refused, so a call through one is
never a call through NULL.
*/
bool
d_host_story_add_driver(
    struct d_host_story*         _host,
    const struct d_story_driver* _driver
)
{
    // parameter validation first
    if ( (!_host)                  ||
         (!_driver)                ||
         (!_driver->claims)        ||
         (!_driver->open)          ||
         (!_driver->replace)       ||
         (!_driver->save) )
    {
        return false;
    }

    const struct d_story_driver** const grown =
        realloc(_host->drivers,
                (_host->driver_count + 1u) * sizeof(*grown));

    // report the failure rather than dropping the driver
    if (!grown)
    {
        return false;
    }

    grown[_host->driver_count++] = _driver;
    _host->drivers               = grown;

    return true;
}


/*
d_host_story_install
  The functions are added with the extension as their user pointer.
*/
bool
d_host_story_install(
    struct d_host_story* _host,
    struct d_awk_interp* _interp
)
{
    // parameter validation first
    if ( (!_host) ||
         (!_interp) )
    {
        return false;
    }

    return d_awk_interp_add_functions(_interp,
                                      d_internal_functions(),
                                      D_INTERNAL_STORY_FUNCTIONS,
                                      _host);
}
