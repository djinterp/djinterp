#!/bin/sh
# story.sh -- res/djinterp/tools/dawk/ext/story/relink_audio.awk against a story built
# to the layout the Articulate driver assumes.
#
# The story is test_dhost_story's fixture: three slides in scene-list order,
# 1.1 Welcome, 1.2 Objectives & Goals and 2.1 Summary, the last holding a
# second audio file a layout shares. Each case runs the script over a folder
# of files and checks what it prints, its status, and, when it writes, the
# bytes it wrote.
#
#   sh tests/djinterp/tools/dawk/ext/story/story.sh [build/dawk] [build/test_dhost_story]
DAWK=${1:-build/dawk}
STORY=${2:-build/test_dhost_story}
SCRIPT=res/djinterp/tools/dawk/ext/story/relink_audio.awk
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
pass=0
fail=0

# run NAME STATUS PATTERN [variables...]: the script over $work/audio must
# exit with STATUS and print a line matching PATTERN
run() {
    name=$1
    want=$2
    pattern=$3
    shift 3
    out=$(cd "$work" && "$DAWK" -x story -T audio -f "$OLDPWD/$SCRIPT" \
                              -v story=course.story "$@" 2>&1)
    got=$?
    if [ "$got" = "$want" ] && printf '%s\n' "$out" | grep -q -- "$pattern"
    then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL $name (status $got, wanted $want; wanted a line like"
        echo "     '$pattern')"
        printf '%s\n' "$out" | sed 's/^/    /'
    fi
}

# fresh: an empty audio folder and an untouched story
fresh() {
    rm -rf "$work/audio" "$work"/*.story
    mkdir -p "$work/audio/sub"
    "$STORY" --fixture "$work/course.story" || { echo "FAIL fixture"; exit 1; }
}

rule='^filename_[0-9]+\.mp3$'

fresh
printf 'ID3 two\n' > "$work/audio/filename_02.mp3"
printf 'ID3 three\n' > "$work/audio/sub/filename_03.mp3"
printf 'not audio\n' > "$work/audio/readme.txt"
run 'a dry run shows the plan' 0 \
    'slide 2 (1.2 Objectives & Goals) <- audio/filename_02.mp3' -v rule="$rule"
run '...and writes nothing' 0 'dry run: nothing written' -v rule="$rule"
[ -f "$work/course.relinked.story" ] && { echo "FAIL a dry run wrote"; fail=$((fail + 1)); }
run 'apply writes the new story' 0 'wrote course.relinked.story' \
    -v rule="$rule" -v apply=1
if [ "$("$STORY" --part "$work/course.relinked.story" story/media/BBBB.mp3)" = "ID3 two" ] &&
   [ "$("$STORY" --part "$work/course.relinked.story" story/media/CCCC.mp3)" = "ID3 three" ]
then
    pass=$((pass + 1))
else
    fail=$((fail + 1))
    echo "FAIL the written story does not hold the new audio"
fi
run '-v out names the new story' 0 'wrote mine.story' \
    -v rule="$rule" -v apply=1 -v out=mine.story
run 'the story is never written over itself' 2 'will not save over' \
    -v rule="$rule" -v apply=1 -v out=course.story

fresh
printf 'x\n' > "$work/audio/filename_09.mp3"
printf 'ID3 two\n' > "$work/audio/filename_02.mp3"
run 'a number past the story has no slide' 1 'no slide 09' \
    -v rule="$rule" -v apply=1
[ -f "$work/course.relinked.story" ] && { echo "FAIL a refused run wrote"; fail=$((fail + 1)); }

fresh
printf 'x\n' > "$work/audio/filename_2.mp3"
printf 'x\n' > "$work/audio/filename_02.mp3"
run 'two files for one slide are refused' 1 'already takes' -v rule="$rule"

fresh
printf 'RIFF\n' > "$work/audio/filename_01.wav"
run 'a .wav cannot replace an .mp3' 1 'cannot replace .mp3' \
    -v rule='^filename_[0-9]+\.(mp3|wav)$'

fresh
printf 'x\n' > "$work/audio/filename_03.mp3"
run 'audio a layout shares is refused' 1 'would change them all' \
    -v rule="$rule" -v audio=2
printf 'x\n' > "$work/audio/filename_02.mp3"
run 'a slide with no second audio is skipped' 0 'has no audio 2' \
    -v rule='^filename_02\.mp3$' -v audio=2

fresh
printf 'x\n' > "$work/audio/VO_2.1_take3.mp3"
run 'a group can give S.N' 0 'slide 3 (2.1 Summary)' \
    -v rule='^VO_([0-9]+\.[0-9]+)_' -v group=1
rm "$work/audio/VO_2.1_take3.mp3"
printf 'x\n' > "$work/audio/Summary.mp3"
run 'a group can give a name' 0 'slide 3 (2.1 Summary)' \
    -v rule='^(Summary)\.mp3$' -v group=1

run 'a rule is required' 2 'are required'

echo
echo "story: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
