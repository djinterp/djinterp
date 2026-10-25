/*******************************************************************************
* djinterp [tools]                                            test_dhost_story.c
*
* Exercises `ext/story/dhost_story.h` and the Articulate driver behind it.
*   The story tested is built here, to the layout dhost_articulate.h assumes,
* and every assumption has a trap in it: the scene list orders the slides
* differently from their part names, a question bank's scene lists a slide
* that is not the story's, one target is relative, one media part is shared
* with a layout, and the three spellings of a checksum each appear once. A
* synthetic story tests the code against the assumptions; only a real .story
* tests the assumptions.
*
*   test_dhost_story                     run the checks
*   test_dhost_story --fixture <path>    write the story, for story.sh
*   test_dhost_story --part <zip> <name> write one part's bytes to stdout
*
* path:      /tests/djinterp/tools/dawk/ext/story/test_dhost_story.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/story/dhost_story.h"  // corresponding header
// std
#include <stdio.h>   // printf, fwrite, remove
#include <stdlib.h>  // free
#include <string.h>  // strcmp, strlen, strstr, memcmp
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dzip.h"                        // d_zip_new
#include "../../../../../../inc/djinterp/tools/dawk/ext/story/dhost_articulate.h"  // d_articulate_driver


static int g_checks = 0;
static int g_failed = 0;


/*
d_tests_check
  Counts one check, and names it when it fails.
*/
static void
d_tests_check(
    bool        _condition,
    const char* _what
)
{
    g_checks++;

    // a failed check is named, so a green run says what it checked
    if (!_condition)
    {
        g_failed++;
        printf("  FAIL  %s\n", _what);
    }

    return;
}


// the media, and the replacements, with the checksums Python's hashlib gives
static const char AUDIO_A[]  = "ID3 fake audio for the welcome slide\n";
static const char AUDIO_B[]  = "ID3 fake audio for the objectives slide\n";
static const char AUDIO_C[]  = "ID3 fake audio for the summary slide\n";
static const char AUDIO_D[]  = "ID3 fake audio shared with a layout\n";
static const char NEW_TWO[]  = "ID3 new narration two\n";
static const char NEW_THREE[] = "ID3 new narration three\n";

static const char STORY_XML[] =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<story xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/"
    "2006/relationships\" name=\"Course\">\n"
    "  <sceneLst>\n"
    "    <scene name=\"Scene 1\"><sldIdLst>\n"
    "      <sldId r:id=\"R1\"/><sldId r:id=\"R3\"/>\n"
    "    </sldIdLst></scene>\n"
    "    <scene name=\"Scene 2\"><sldIdLst><sldId>R2</sldId>"
    "</sldIdLst></scene>\n"
    "  </sceneLst>\n"
    "  <mediaLst>\n"
    "    <media id=\"a\" md5Checksum=\"8c94ec6c8c2a2f46fd811e976f15b361\"/>\n"
    "    <media id=\"b\" md5Checksum=\"331CD7C7563C586123B19760D084E44F\"/>\n"
    "    <media id=\"c\"><stream>z2s5qY09uwhYHjgqhx02ug==</stream></media>\n"
    "  </mediaLst>\n"
    "  <quizMgr><bankLst><scene name=\"Bank\"><sldIdLst><sldId r:id=\"R9\"/>"
    "</sldIdLst></scene></bankLst></quizMgr>\n"
    "</story>\n";

static const char STORY_RELS[] =
    "<?xml version=\"1.0\"?>\n"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/"
    "relationships\">\n"
    "  <Relationship Id=\"R1\" Type=\"slide\" "
    "Target=\"/story/slides/slide1.xml\"/>\n"
    "  <Relationship Id=\"R2\" Type=\"slide\" "
    "Target=\"/story/slides/slide2.xml\"/>\n"
    "  <Relationship Id=\"R3\" Type=\"slide\" Target=\"slides/slide3.xml\"/>\n"
    "  <Relationship Id=\"R9\" Type=\"slide\" "
    "Target=\"/story/slides/slide9.xml\"/>\n"
    "</Relationships>\n";

static const char THEME_XML[] =
    "<theme><mediaLst>"
    "<media id=\"a\" md5Checksum=\"8c94ec6c8c2a2f46fd811e976f15b361\"/>"
    "<media id=\"b\" md5Checksum=\"331CD7C7563C586123B19760D084E44F\"/>"
    "</mediaLst></theme>\n";


/*
d_tests_slide_rels
  A slide's relationship part, with up to three media targets.
*/
static void
d_tests_slide_rels(
    char*       _out,
    size_t      _size,
    const char* _first,
    const char* _second,
    const char* _third
)
{
    const char* const targets[3] = { _first, _second, _third };
    size_t            used       = 0;

    used += (size_t)snprintf(&_out[used], _size - used, "<Relationships>");

    for (size_t at = 0; at < 3u; ++at)
    {
        // an absent target adds nothing
        if (targets[at])
        {
            used += (size_t)snprintf(&_out[used], _size - used,
                                     "<Relationship Id=\"M%zu\" "
                                     "Type=\"http://schemas.articulate.com/"
                                     "media\" Target=\"%s\"/>",
                                     at, targets[at]);
        }
    }

    (void)snprintf(&_out[used], _size - used, "</Relationships>\n");

    return;
}


/*
d_tests_add_text
  Adds a text part to an archive being built.
*/
static bool
d_tests_add_text(
    struct d_zip* _zip,
    const char*   _name,
    const char*   _text
)
{
    return d_zip_add(_zip, _name, (const unsigned char*)_text,
                     strlen(_text));
}


/*
d_tests_make_story
  Builds the synthetic story and writes it to `_path`.  With `_scenes`
false, the story part has no scene list, which is the fallback's case.
*/
static bool
d_tests_make_story(
    const char* _path,
    bool        _scenes
)
{
    struct d_zip* const zip = d_zip_new();
    char                rels[1024];
    bool                ok  = (zip != NULL);

    ok = ok && d_tests_add_text(zip, "[Content_Types].xml", "<Types/>\n");
    ok = ok && d_tests_add_text(zip, "story/story.xml",
                                _scenes ? STORY_XML : "<story/>\n");
    ok = ok && d_tests_add_text(zip, "story/_rels/story.xml.rels",
                                STORY_RELS);
    ok = ok && d_tests_add_text(zip, "story/themes/theme1.xml", THEME_XML);
    ok = ok && d_tests_add_text(zip, "story/slides/slide1.xml",
                                "<sld name=\"Welcome\"><spTree/></sld>\n");
    ok = ok && d_tests_add_text(zip, "story/slides/slide2.xml",
                                "<p:sld xmlns:p=\"x\" name=\"Summary\"/>\n");
    ok = ok && d_tests_add_text(zip, "story/slides/slide3.xml",
                                "<sld name=\"Objectives &amp; Goals\"/>\n");
    ok = ok && d_tests_add_text(zip, "story/slides/slide9.xml",
                                "<sld name=\"Bank question\"/>\n");

    d_tests_slide_rels(rels, sizeof(rels), "/story/media/AAAA.mp3",
                       "/story/media/IMG1.png", NULL);
    ok = ok && d_tests_add_text(zip, "story/slides/_rels/slide1.xml.rels",
                                rels);
    d_tests_slide_rels(rels, sizeof(rels), "../media/CCCC.mp3",
                       "/story/media/DDDD.mp3", NULL);
    ok = ok && d_tests_add_text(zip, "story/slides/_rels/slide2.xml.rels",
                                rels);
    d_tests_slide_rels(rels, sizeof(rels), "/story/media/BBBB.mp3", NULL,
                       NULL);
    ok = ok && d_tests_add_text(zip, "story/slides/_rels/slide3.xml.rels",
                                rels);
    d_tests_slide_rels(rels, sizeof(rels), "/story/media/DDDD.mp3", NULL,
                       NULL);
    ok = ok && d_tests_add_text(zip,
                                "story/slideLayouts/_rels/slideLayout1.xml.rels",
                                rels);

    ok = ok && d_tests_add_text(zip, "story/media/AAAA.mp3", AUDIO_A);
    ok = ok && d_tests_add_text(zip, "story/media/BBBB.mp3", AUDIO_B);
    ok = ok && d_tests_add_text(zip, "story/media/CCCC.mp3", AUDIO_C);
    ok = ok && d_tests_add_text(zip, "story/media/DDDD.mp3", AUDIO_D);
    ok = ok && d_tests_add_text(zip, "story/media/IMG1.png", "PNG");
    ok = ok && d_zip_save(zip, _path, NULL, 0);

    d_zip_free(zip);

    return ok;
}


/*
d_tests_write
  Writes a small file, for a replacement's source.
*/
static bool
d_tests_write(
    const char* _path,
    const char* _text
)
{
    FILE* const out = fopen(_path, "wb");
    bool        ok  = (out != NULL);

    ok = ok && (fwrite(_text, 1u, strlen(_text), out) == strlen(_text));

    // a stream that fails to close has not written what it held
    if ( (out) &&
         (fclose(out) != 0) )
    {
        ok = false;
    }

    return ok;
}


/*
d_tests_part
  Reads one part of a saved package as text; NULL when it is not there.
*/
static char*
d_tests_part(
    const char* _path,
    const char* _name
)
{
    struct d_zip* const zip   = d_zip_open(_path, NULL, 0);
    unsigned char*      bytes = NULL;
    size_t              length = 0;

    // a package or a part that is not there reads as nothing
    if ( (!zip) ||
         (!d_zip_read(zip, d_zip_find(zip, _name), &bytes, &length, NULL, 0)) )
    {
        bytes = NULL;
    }

    d_zip_free(zip);

    return (char*)bytes;
}


/*
d_tests_order
  Tests the following:
  - slides come in scene-list order, not part-name order, across scenes
  - a question bank's scene is not the story's
  - an <sldId> given as text, and a relative target, both resolve
  - a slide's name is its root's, decoded, a prefix on the root allowed
  - an image is media but not audio
*/
static void
d_tests_order(
    const struct d_story* _story
)
{
    printf("slides and media\n");

    d_tests_check(d_story_slide_count(_story) == 3u, "three slides");
    d_tests_check(strcmp(d_story_slide_name(_story, 1), "Welcome") == 0,
                  "slide 1 is Welcome");
    d_tests_check(strcmp(d_story_slide_name(_story, 2),
                         "Objectives & Goals") == 0,
                  "slide 2 is slide3.xml, by the scene list");
    d_tests_check(strcmp(d_story_slide_name(_story, 3), "Summary") == 0,
                  "slide 3 is scene 2's");
    d_tests_check((d_story_slide_scene(_story, 3) == 2u) &&
                  (d_story_slide_number(_story, 3) == 1u),
                  "slide 3 is 2.1");
    d_tests_check(d_story_audio_count(_story, 1) == 1u,
                  "the image is not audio");
    d_tests_check(strcmp(d_story_audio_name(_story, 3, 1),
                         "story/media/CCCC.mp3") == 0,
                  "a relative target resolves");
    d_tests_check(d_story_audio_count(_story, 3) == 2u,
                  "slide 3 holds two audio files");
    d_tests_check(d_story_audio_count(_story, 4) == 0u,
                  "a slide past the end holds nothing");
    d_tests_check(d_story_notes(_story)[0] == '\0',
                  "a scene list leaves nothing to note");

    return;
}


/*
d_tests_keys
  Tests the following:
  - digits are an ordinal, leading zeros and all
  - S.N is a slide of a scene
  - anything else is a name; nothing named resolves to 0
  - a rule's key is the last number short of the extension, or a group
*/
static void
d_tests_keys(
    const struct d_story* _story
)
{
    char key[64];
    char why[256];

    printf("keys and rules\n");

    d_tests_check(d_story_resolve(_story, "02") == 2u, "\"02\" is slide 2");
    d_tests_check(d_story_resolve(_story, "3") == 3u, "\"3\" is slide 3");
    d_tests_check(d_story_resolve(_story, "4") == 0u, "\"4\" is no slide");
    d_tests_check(d_story_resolve(_story, "0") == 0u, "\"0\" is no slide");
    d_tests_check(d_story_resolve(_story, "1.2") == 2u, "1.2 is slide 2");
    d_tests_check(d_story_resolve(_story, "2.1") == 3u, "2.1 is slide 3");
    d_tests_check(d_story_resolve(_story, "2.2") == 0u, "2.2 is no slide");
    d_tests_check(d_story_resolve(_story, "Summary") == 3u,
                  "a name is a slide");
    d_tests_check(d_story_resolve(_story, "Nothing") == 0u,
                  "an unknown name is no slide");

    d_tests_check(d_story_match("^filename_[0-9]+\\.(mp3|wav)$",
                                "dir/filename_08.mp3", 0, key, sizeof(key),
                                why, sizeof(why)) &&
                  (strcmp(key, "08") == 0),
                  "the number is the key, the extension and the group not");
    d_tests_check(d_story_match("^filename_[0-9]+\\.mp3$",
                                "C:\\audio\\filename_8.mp3", 0, key,
                                sizeof(key), why, sizeof(why)) &&
                  (strcmp(key, "8") == 0),
                  "a Windows path is matched by its last part");
    d_tests_check(d_story_match("^filename_[0-9]+\\.mp3$", "notes_08.mp3", 0,
                                key, sizeof(key), why, sizeof(why)) &&
                  (key[0] == '\0'),
                  "a name the rule does not match has no key");
    d_tests_check(d_story_match("^s([0-9]+)_", "s12_take3.mp3", 1, key,
                                sizeof(key), why, sizeof(why)) &&
                  (strcmp(key, "12") == 0),
                  "a group asked for is the key");
    d_tests_check(!d_story_match("(", "x", 0, key, sizeof(key), why,
                                 sizeof(why)),
                  "a rule that does not compile is refused");
    d_tests_check(!d_story_match("a", "a", 1, key, sizeof(key), why,
                                 sizeof(why)),
                  "a group the rule does not have is refused");

    return;
}


/*
d_tests_replace
  Tests the following:
  - an absent slide or audio is absent, not failed
  - a part a layout shares is refused, naming the count
  - a file of another kind is refused
  - the story is never saved over itself
  - a saved story holds the new bytes, every checksum spelling rewritten in
    every part, and every other part byte for byte
*/
static void
d_tests_replace(
    struct d_story* _story,
    const char*     _path,
    const char*     _work
)
{
    char why[256];
    char two[512];
    char three[512];
    char wav[512];
    char out[512];

    printf("replacing and saving\n");

    (void)snprintf(two, sizeof(two), "%s/filename_02.mp3", _work);
    (void)snprintf(three, sizeof(three), "%s/filename_03.MP3", _work);
    (void)snprintf(wav, sizeof(wav), "%s/filename_01.wav", _work);
    (void)snprintf(out, sizeof(out), "%s/out.story", _work);
    d_tests_check(d_tests_write(two, NEW_TWO) &&
                  d_tests_write(three, NEW_THREE) &&
                  d_tests_write(wav, "RIFF"),
                  "the sources are written");

    d_tests_check(d_story_replace_audio(_story, 9, 1, two, why, sizeof(why))
                  == D_STORY_ABSENT, "slide 9 is absent");
    d_tests_check(d_story_replace_audio(_story, 1, 2, two, why, sizeof(why))
                  == D_STORY_ABSENT, "slide 1 has no second audio");
    d_tests_check((d_story_replace_audio(_story, 3, 2, two, why, sizeof(why))
                   == D_STORY_FAILED) &&
                  (strstr(why, "used 2 times") != NULL),
                  "a part a layout shares is refused");
    d_tests_check((d_story_replace_audio(_story, 1, 1, wav, why, sizeof(why))
                   == D_STORY_FAILED) &&
                  (strstr(why, ".wav") != NULL),
                  "a .wav cannot replace an .mp3");
    d_tests_check(d_story_replace_audio(_story, 2, 1, two, why, sizeof(why))
                  == D_STORY_REPLACED, "slide 2's audio is replaced");
    d_tests_check(d_story_replace_audio(_story, 3, 1, three, why, sizeof(why))
                  == D_STORY_REPLACED, ".MP3 replaces .mp3, any case");
    d_tests_check(d_story_changes(_story) == 2u, "two changes");
    d_tests_check(!d_story_save(_story, _path, why, sizeof(why)),
                  "the story is not saved over itself");
    d_tests_check(d_story_save(_story, out, why, sizeof(why)),
                  "the story is saved elsewhere");

    char* const b      = d_tests_part(out, "story/media/BBBB.mp3");
    char* const c      = d_tests_part(out, "story/media/CCCC.mp3");
    char* const a      = d_tests_part(out, "story/media/AAAA.mp3");
    char* const story  = d_tests_part(out, "story/story.xml");
    char* const theme  = d_tests_part(out, "story/themes/theme1.xml");
    char* const before = d_tests_part(_path, "story/slides/slide1.xml");
    char* const after  = d_tests_part(out, "story/slides/slide1.xml");

    d_tests_check((b) && (strcmp(b, NEW_TWO) == 0),
                  "slide 2's part holds the new bytes");
    d_tests_check((c) && (strcmp(c, NEW_THREE) == 0),
                  "slide 3's part holds the new bytes");
    d_tests_check((a) && (strcmp(a, AUDIO_A) == 0),
                  "an unreplaced part is unchanged");
    d_tests_check((story) &&
                  (strstr(story, "CD4B37A403367B7F6AAC0211DCCA7302")) &&
                  (strstr(story, "6dOo12G5we8qeOod34fLXQ==")) &&
                  (strstr(story, "8c94ec6c8c2a2f46fd811e976f15b361")) &&
                  (!strstr(story, "331CD7C7563C586123B19760D084E44F")),
                  "story.xml's checksums are rewritten, upper hex and base64");
    d_tests_check((theme) &&
                  (strstr(theme, "CD4B37A403367B7F6AAC0211DCCA7302")),
                  "the theme's copy is rewritten too");
    d_tests_check((before) && (after) && (strcmp(before, after) == 0),
                  "a slide part is copied as it was");
    d_tests_check(d_story_notes(_story)[0] == '\0',
                  "every checksum was found");

    free(b);
    free(c);
    free(a);
    free(story);
    free(theme);
    free(before);
    free(after);

    return;
}


/*
d_tests_fallback
  Tests the following:
  - without a scene list the slides follow their part names, numerically
  - and the story notes it
*/
static void
d_tests_fallback(
    const char* _work
)
{
    char path[512];
    char why[256];

    printf("the fallback order\n");

    (void)snprintf(path, sizeof(path), "%s/plain.story", _work);

    const struct d_story_driver* const drivers[1] = { d_articulate_driver() };
    struct d_story*                    story      = NULL;

    d_tests_check(d_tests_make_story(path, false), "the story is written");
    story = d_story_open(path, drivers, 1u, why, sizeof(why));
    d_tests_check(story != NULL, "it opens");
    d_tests_check(d_story_slide_count(story) == 4u,
                  "every slide part is a slide");
    d_tests_check(strcmp(d_story_slide_name(story, 3),
                         "Objectives & Goals") == 0,
                  "slide3.xml is third");
    d_tests_check(strstr(d_story_notes(story), "order of their part names")
                  != NULL, "the story notes the order it fell back to");
    d_story_free(story);

    return;
}


/*
main
  Runs the checks in a directory of the caller's, or serves story.sh.
*/
int
main(
    int    argc,
    char** argv
)
{
    // --fixture writes the story for the script's test
    if ( (argc == 3) &&
         (strcmp(argv[1], "--fixture") == 0) )
    {
        return d_tests_make_story(argv[2], true) ? 0 : 1;
    }

    // --part writes one part's bytes to stdout
    if ( (argc == 4) &&
         (strcmp(argv[1], "--part") == 0) )
    {
        char* const text = d_tests_part(argv[2], argv[3]);

        // a part that is not there is a failure
        if (!text)
        {
            return 1;
        }

        (void)fputs(text, stdout);
        free(text);

        return 0;
    }

    const char* const work = (argc > 1) ? argv[1] : ".";
    char              path[512];
    char              why[256];

    (void)snprintf(path, sizeof(path), "%s/course.story", work);

    const struct d_story_driver* const drivers[1] = { d_articulate_driver() };

    d_tests_check(d_tests_make_story(path, true), "the story is written");

    struct d_story* const story = d_story_open(path, drivers, 1u, why,
                                               sizeof(why));

    d_tests_check(story != NULL, "the story opens");
    d_tests_check((!d_story_open("x.txt", drivers, 1u, why, sizeof(why))) &&
                  (strstr(why, "no driver") != NULL),
                  "a file no driver claims is refused");

    // the checks that read a story need the story
    if (story)
    {
        d_tests_order(story);
        d_tests_keys(story);
        d_tests_replace(story, path, work);
        d_story_free(story);
    }

    d_tests_fallback(work);

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
