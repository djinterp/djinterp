# relink_audio.awk -- replace each slide's audio with the file whose name
# carries that slide's number.
#
#   dawk -x story -T <audio folder> -f relink_audio.awk \
#        -v story=<course.story> -v rule='<expression>' [-v apply=1]
#
# Every file under the folder whose name matches `rule`, an extended regular
# expression, replaces the audio of the slide its number names: the last
# number in the match, the extension left out, counted through the whole
# story, so filename_08.mp3 under rule='^filename_[0-9]+\.mp3$' is for the
# eighth slide. The paths can come from a list instead of a folder: each line
# of the input is a path, so `dawk -x story -f relink_audio.awk ... list.txt`
# reads the files list.txt names.
#
#   story   the .story file. It is read, and never written.
#   rule    which files are replacements, and where their number is.
#   apply   1 to write the new story. Without it the run only shows what it
#           would do -- every replacement is still tried, in memory, so what
#           it shows is what applying would do.
#   out     where the new story goes: <story> with .relinked.story for
#           .story by default.
#   group   n to take the key from group n of the rule instead of its last
#           number. A key is a slide number, S.N for slide N of scene S, or
#           a slide's name.
#   audio   which of a slide's audio files to replace, from 1; 1 by default.
#
# Nothing is written unless every matching file has a slide to go to and was
# accepted: two files for one slide, a file of another kind than the audio it
# would replace, or a media file shared with another slide or a layout leave
# the story as it was. Exit status: 0 when all is well, 1 when something was
# refused or did not match a slide, 2 when the story could not be read or
# written.

BEGIN {
    fatal = 0

    # the story and the rule are required
    if ((story == "") || (rule == "")) {
        print "relink_audio: -v story=<file.story> and -v rule=<expression> " \
              "are required" > "/dev/stderr"
        fatal = 2
        exit fatal
    }

    slides = story_open(story)

    # a story that cannot be read stops the run
    if (slides < 0) {
        print "relink_audio: " story_error() > "/dev/stderr"
        fatal = 2
        exit fatal
    }

    show_notes()

    if (out == "") {
        out = story
        if (!sub(/\.[sS][tT][oO][rR][yY]$/, ".relinked.story", out))
            out = out ".relinked.story"
    }

    audio   = (audio == "") ? 1 : audio + 0
    group   = (group == "") ? 0 : group + 0
    matched = replaced = refused = homeless = skipped = 0

    printf "story: %s, %d slides\n", story, slides
}

# each path is a file, and only a name the rule matches is a replacement
{
    path = $0
    key  = story_match(rule, path, group)

    if (key == "")
        next

    matched++
    slide = story_slide(key)

    # a key that names no slide has nowhere to go
    if (!slide) {
        printf "  no slide %s for %s\n", key, path
        homeless++
        next
    }

    # two files for one slide is a choice this script will not make
    if (slide in taken) {
        printf "  refused %s: slide %d already takes %s\n", path, slide,
               taken[slide]
        refused++
        next
    }

    taken[slide] = path
    target       = story_audio_name(slide, audio)
    result       = story_replace_audio(slide, path, audio)

    if (result > 0) {
        printf "  slide %d (%d.%d %s) <- %s, replacing %s\n", slide,
               story_slide_scene(slide), story_slide_number(slide),
               story_slide_name(slide), path, target
        replaced++
    }
    else if (result == 0) {
        printf "  skipped %s: slide %d has no audio %d\n", path, slide, audio
        skipped++
    }
    else {
        printf "  refused %s: %s\n", path, story_error()
        refused++
    }
}

END {
    # a run that never opened its story has nothing to report
    if (fatal)
        exit fatal

    printf "%d matched: %d %s, %d refused, %d with no slide, " \
           "%d skipped\n", matched, replaced,
           (apply ? "replaced" : "to replace"), refused, homeless, skipped

    # anything refused or homeless leaves the story as it was
    if (refused || homeless) {
        print "nothing written: fix or remove the files above and run again"
        exit 1
    }

    if (!apply) {
        if (replaced)
            printf "dry run: nothing written; -v apply=1 writes %s\n", out
        exit 0
    }

    if (!replaced) {
        print "nothing to write"
        exit 0
    }

    # the new story goes beside the old, never over it
    if (story_save(out) < 0) {
        print "relink_audio: " story_error() > "/dev/stderr"
        exit 2
    }

    printf "wrote %s\n", out
    show_notes()
}

# show_notes prints what the driver noted of the story, a line at a time
function show_notes(    notes, lines, at, line_of) {
    notes = story_notes()
    lines = split(notes, line_of, "\n")
    for (at = 1; at <= lines; at++)
        if (line_of[at] != "")
            print "note: " line_of[at] > "/dev/stderr"
}
