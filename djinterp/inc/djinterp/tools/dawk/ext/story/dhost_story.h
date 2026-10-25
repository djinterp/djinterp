/*******************************************************************************
* djinterp [tools]                                                 dhost_story.h
*
* Storyline: the host extension for e-learning stories.
*   A story is slides in order, each holding media -- audio, video, images --
* that a package stores apart from the slides. This unit is that model and no
* format: a driver reads one format into it and writes changes back, and the
* Articulate driver is one such. A program sees the model through host
* functions named story_*, and a rule for which file belongs to which slide is
* the program's own, with story_match to read a key out of a file's name.
*   What the model guards, whatever the format: a story is never saved over
* the file it was read from, a media file used by two slides is not replaced
* through one of them, and audio is replaced only by a file of the same kind
* as the one it replaces. A driver can refuse more; it cannot refuse less.
*
* path:      /inc/djinterp/tools/dawk/ext/story/dhost_story.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Model
         1.  d_story
         2.  d_story_media_kind
         3.  D_STORY_NONE
    2.  Drivers
         1.  d_story_driver
    3.  Results
         1.  d_story_result
    4.  Extension
         1.  d_host_story
2.  OPERATIONS
    ----------
    1.  Building, for drivers
    2.  Opening and saving
    3.  Slides
    4.  Media
    5.  Rules
    6.  The extension
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_STORY_DHOST_STORY_H
#define DJINTERP_TOOLS_DAWK_EXT_STORY_DHOST_STORY_H 1

// std
#include <stdbool.h>     // bool
#include <stddef.h>      // size_t
// djinterp
#include "../../dinterp.h"  // d_awk_interp


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Model
//------------------------------------------------------------------------------
// 1.1.1
// d_story
//   struct: one story read from a file, its driver, and the changes made to
//   it since.  Layout is private.
struct d_story;

// 1.1.2
// d_story_media_kind
//   enum: what a media file is, by its name's extension.
enum d_story_media_kind
{
    D_STORY_MEDIA_OTHER = 0,
    D_STORY_MEDIA_AUDIO,
    D_STORY_MEDIA_VIDEO,
    D_STORY_MEDIA_IMAGE
};

// 1.1.3
// D_STORY_NONE
//   constant: the index a builder gives when nothing could be added.
#define D_STORY_NONE ((size_t)-1)

// 1.2    Drivers
//------------------------------------------------------------------------------
// 1.2.1
// d_story_driver
//   struct: one format.  `claims` says whether a file is the format's, from
//   its name; `open` reads it into the story through the builders below and
//   keeps whatever it needs in `*_out_state`; `replace` takes a media file's
//   new bytes; `save` writes the story with every change; `close` releases
//   the state.  The three that can fail write why into `_problem`.  Slides
//   are numbered from 1 in story order, and media are named as the format
//   names them, which for a package is the path within it.
struct d_story_driver
{
    const char* name;
    bool        (*claims)(const char* _path);
    bool        (*open)(struct d_story* _story,
                        const char*     _path,
                        void**          _out_state,
                        char*           _problem,
                        size_t          _capacity);
    bool        (*replace)(void*                _state,
                           const char*          _media,
                           const unsigned char* _bytes,
                           size_t               _length,
                           char*                _problem,
                           size_t               _capacity);
    bool        (*save)(void*       _state,
                        const char* _path,
                        char*       _problem,
                        size_t      _capacity);
    void        (*close)(void* _state);
};

// 1.3    Results
//------------------------------------------------------------------------------
// 1.3.1
// d_story_result
//   enum: what a replacement came to.  D_STORY_ABSENT is a slide or an audio
//   index that is not there, which a rule matching every file meets often and
//   which is not a failure.
enum d_story_result
{
    D_STORY_REPLACED = 0,
    D_STORY_ABSENT,
    D_STORY_FAILED
};

// 1.4    Extension
//------------------------------------------------------------------------------
// 1.4.1
// d_host_story
//   struct: the extension's state behind the story_* functions: the drivers
//   it knows, the story open, the last failure.  Layout is private.
struct d_host_story;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// Every operation that can fail writes why into a caller's buffer of
// `_capacity` bytes, which may be NULL when the reason is not wanted.


// 2.1    Building, for drivers
//------------------------------------------------------------------------------
// A driver's `open` adds the slides in story order and, after each, the media
// it holds in the slide's own order; a media file named twice is one file
// used twice.  Both return D_STORY_NONE when memory runs out, and `open`
// then fails.
size_t      d_story_add_slide(struct d_story* _story,
                              const char*     _name,
                              size_t          _scene,
                              size_t          _number);
size_t      d_story_add_media(struct d_story* _story,
                              size_t          _slide,
                              const char*     _media);
// d_story_note records something a reader of the result should know that is
// not a failure -- an order inferred, a checksum not found -- for
// story_notes() to give the program; notes accumulate, one per line.
bool        d_story_note(struct d_story* _story,
                         const char*     _note);

// 2.2    Opening and saving
//------------------------------------------------------------------------------
/**
 * @brief Opens a story with the first driver that claims the file.
 *
 * @param[in]  _path      the story's file.
 * @param[in]  _drivers   the drivers to ask, in order.
 * @param[in]  _count     how many there are.
 * @param[out] _problem   receives why it could not be opened.
 * @param[in]  _capacity  the size of `_problem`.
 * @post   The caller releases the story with `d_story_free`.
 * @return the story, or `NULL` when no driver claims the file or the one that
 *         does cannot read it.
 */
struct d_story* d_story_open(const char*                         _path,
                             const struct d_story_driver* const* _drivers,
                             size_t                              _count,
                             char*                               _problem,
                             size_t                              _capacity);
/**
 * @brief Releases a story, discarding unsaved changes.
 *
 * @param[in] _story  the story; may be `NULL`.
 */
void        d_story_free(struct d_story* _story);
/**
 * @brief Writes the story, with every change, to another file.
 *
 * @param[in,out] _story     the story.
 * @param[in]     _path      the file to write; the story's own is refused.
 * @param[out]    _problem   receives why it could not be written.
 * @param[in]     _capacity  the size of `_problem`.
 * @return `true` if it was written; `false` otherwise.
 */
bool        d_story_save(struct d_story* _story,
                         const char*     _path,
                         char*           _problem,
                         size_t          _capacity);
size_t      d_story_changes(const struct d_story* _story);
const char* d_story_driver_name(const struct d_story* _story);
const char* d_story_notes(const struct d_story* _story);

// 2.3    Slides
//------------------------------------------------------------------------------
// Slides are numbered from 1 in story order.  d_story_resolve reads a key as
// a slide: digits are the slide that many into the story, so "08" is the
// eighth; "S.N" is slide N of scene S, as Storyline shows it; anything else is
// the first slide of that name.  It gives 0 for a key that names no slide, and
// each accessor gives "" or 0 for a slide that is not there.
size_t      d_story_slide_count(const struct d_story* _story);
size_t      d_story_resolve(const struct d_story* _story,
                            const char*           _key);
const char* d_story_slide_name(const struct d_story* _story,
                               size_t                _slide);
size_t      d_story_slide_scene(const struct d_story* _story,
                                size_t                _slide);
size_t      d_story_slide_number(const struct d_story* _story,
                                 size_t                _slide);

// 2.4    Media
//------------------------------------------------------------------------------
// A slide's audio is numbered from 1 in the slide's order.
size_t      d_story_audio_count(const struct d_story* _story,
                                size_t                _slide);
const char* d_story_audio_name(const struct d_story* _story,
                               size_t                _slide,
                               size_t                _audio);
/**
 * @brief Replaces one audio file of a slide with the bytes of another file.
 *
 * @note Refused, as D_STORY_FAILED: a media file another slide also uses, and
 *       a file whose extension is not the one it would replace.
 *
 * @param[in,out] _story     the story.
 * @param[in]     _slide     the slide, from 1.
 * @param[in]     _audio     which of its audio, from 1.
 * @param[in]     _source    the file whose bytes replace the audio.
 * @param[out]    _problem   receives why it was refused or failed.
 * @param[in]     _capacity  the size of `_problem`.
 * @return D_STORY_REPLACED; D_STORY_ABSENT when the slide or the audio is not
 *         there; D_STORY_FAILED otherwise.
 */
enum d_story_result d_story_replace_audio(struct d_story* _story,
                                          size_t          _slide,
                                          size_t          _audio,
                                          const char*     _source,
                                          char*           _problem,
                                          size_t          _capacity);

// 2.5    Rules
//------------------------------------------------------------------------------
/**
 * @brief Reads a key out of a file's name with an extended regular
 *        expression, the rule for which file belongs to which slide.
 *
 * @note The expression is matched against the name's last part, after any
 *       `/` or `\`. The key is the last run of digits within the match, the
 *       name's extension left out, so `^filename_[0-9]+\.(mp3|wav)$` gives
 *       "08" for filename_08.mp3. A group is the key only when it is asked
 *       for by number, since ERE has no group that does not capture and an
 *       alternation would otherwise become the key.
 *
 * @param[in]  _expression  the rule.
 * @param[in]  _path        the file.
 * @param[in]  _group       0 for the digits; n for the text of group n.
 * @param[out] _key         receives the key, terminated; "" when the name
 *                          does not match or the match holds no key.
 * @param[in]  _key_size    the size of `_key`.
 * @param[out] _problem     receives why the expression would not compile.
 * @param[in]  _capacity    the size of `_problem`.
 * @return `true` when the expression compiled, matched or not; `false`
 *         otherwise.
 */
bool        d_story_match(const char* _expression,
                          const char* _path,
                          size_t      _group,
                          char*       _key,
                          size_t      _key_size,
                          char*       _problem,
                          size_t      _capacity);

// 2.6    The extension
//------------------------------------------------------------------------------
// d_host_story_install adds the story_* functions to an interpreter, which
// then calls into the extension until it is freed; the extension outlives the
// interpreter.  Drivers are borrowed, and asked in the order they were added.
struct d_host_story* d_host_story_new(void);
void        d_host_story_free(struct d_host_story* _host);
bool        d_host_story_add_driver(struct d_host_story*         _host,
                                    const struct d_story_driver* _driver);
bool        d_host_story_install(struct d_host_story* _host,
                                 struct d_awk_interp* _interp);


#endif  // DJINTERP_TOOLS_DAWK_EXT_STORY_DHOST_STORY_H
