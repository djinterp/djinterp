#!/bin/sh
# git.sh -- dgit against git itself, and the -x git extension and its
# scripts against what git says.
#
# Repositories are made here with git: a history with branches, a merge and
# tags of each kind, its refs packed and its objects packed with deltas and
# some left loose; a reflog and an upstream; a work tree holding every kind
# of change status reports -- staged, changed, gone, a type changed, a
# conflict, intent to add, skip-worktree, untracked and ignored files and
# directories, names that need quotes, \r\n endings and attributes over
# them; a linked work tree; a submodule; a bare clone; and a repository of
# SHA-256 names. Of each, test_dgit --oracle answers as git's commands do,
# and the answers must be git's own: rev-parse, rev-parse --short, rev-list
# with its ranges, paths and --first-parent, merge-base --all, cat-file over
# every object, ls-files -s, ls-tree, status --porcelain in each mode,
# check-ignore -v, check-attr -a, config --list --show-scope, for-each-ref,
# and config -f over random files. The repository this runs in is read too,
# when it is one. Then dawk -x git and git_ignore.awk, git_survey.awk and
# git_changelog.awk, over the same repositories.
#   Without git, nothing is compared, and the suite passes saying so; with
# no dawk -- "none" in its place, as CMake's suite runs it -- only dgit is.
#
#   sh tests/djinterp/tools/dawk/ext/git/git.sh [build/dawk] [build/test_dgit]
DAWK=${1:-build/dawk}
HELPER=${2:-build/test_dgit}
RES=res/djinterp/tools/dawk/ext/git
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
pass=0
fail=0

if ! command -v git > /dev/null 2>&1; then
    echo "note: git is not installed; nothing is compared"
    echo "git: 0 passed, 0 failed"
    exit 0
fi

ROOT=$(pwd)
case $DAWK in /*) ;; *) DAWK=$ROOT/$DAWK ;; esac
case $HELPER in /*) ;; *) HELPER=$ROOT/$HELPER ;; esac
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY \
      GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_CEILING_DIRECTORIES \
      GIT_ATTR_SOURCE GIT_CONFIG GIT_CONFIG_PARAMETERS GIT_CONFIG_COUNT \
      GIT_NAMESPACE GIT_COMMON_DIR XDG_CONFIG_HOME
export GIT_CONFIG_NOSYSTEM=1 GIT_ATTR_NOSYSTEM=1 HOME="$work/home"
export GIT_AUTHOR_NAME="A U Thor" GIT_AUTHOR_EMAIL=author@example.com
export GIT_COMMITTER_NAME="C O Mitter" GIT_COMMITTER_EMAIL=c@example.com
export GIT_ALLOW_PROTOCOL=file LC_ALL=C
mkdir -p "$HOME"
git config --global init.defaultBranch main
git config --global protocol.file.allow always
git config --global advice.detachedHead false
git config --global advice.nestedTag false
git config --global user.useConfigOnly false
tick=1700000000

# commit DIRECTORY MESSAGE [ARGS...]: a commit a minute after the last
commit() {
    dir=$1
    message=$2
    shift 2
    tick=$((tick + 60))
    GIT_AUTHOR_DATE="@$tick +0100" GIT_COMMITTER_DATE="@$tick +0000" \
        git -C "$dir" commit -q -m "$message" "$@"
}

# same NAME WANT GOT: two answers must be one
same() {
    if [ "$2" = "$3" ]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL $1"
        printf '%s\n' "$2" > "$work/want"
        printf '%s\n' "$3" > "$work/got"
        diff "$work/want" "$work/got" | head -20 | sed 's/^/    /'
    fi
}

# oracle REPOSITORY COMMAND [ARGS...]: test_dgit's answer
oracle() {
    repo=$1
    shift
    "$HELPER" --oracle "$repo" "$@" 2>&1
}

# revs REPOSITORY REVISION...: each named as rev-parse names it, and made
# short as rev-parse --short makes it
revs() {
    repo=$1
    shift
    for rev in "$@"; do
        want=$(git -C "$repo" rev-parse --verify -q "$rev" 2>/dev/null ||
               echo FAIL)
        same "rev-parse $rev in $repo" "$want" "$(oracle "$repo" rev "$rev")"
        if [ "$want" != FAIL ]; then
            same "rev-parse --short $rev in $repo" \
                "$(git -C "$repo" rev-parse --short "$rev" 2>/dev/null)" \
                "$(oracle "$repo" short "$rev")"
        fi
    done
}

# walks REPOSITORY ARGUMENTS...: rev-list, each argument a whole walk's
walks() {
    repo=$1
    shift
    for args in "$@"; do
        # the arguments split as the shell splits them
        # shellcheck disable=SC2086
        same "rev-list $args in $repo" \
            "$(git -C "$repo" rev-list $args 2>&1)" \
            "$(oracle "$repo" log $args)"
    done
}

# objects REPOSITORY: every object, its type and size, as cat-file gives
objects() {
    git -C "$1" cat-file --batch-all-objects --batch-check='%(objectname)' \
        > "$work/names"
    same "cat-file of every object in $1" \
        "$(git -C "$1" cat-file --batch-all-objects \
               --batch-check='%(objectname) %(objecttype) %(objectsize)')" \
        "$("$HELPER" --oracle "$1" cat-check < "$work/names")"
}

# reads REPOSITORY: refs, the index, trees and settings, as git gives them
reads() {
    repo=$1
    same "for-each-ref in $repo" \
        "$(git -C "$repo" for-each-ref --format='%(objectname) %(refname)')" \
        "$(oracle "$repo" refs)"
    same "config --list --show-scope in $repo" \
        "$(git -C "$repo" config --list --show-scope)" \
        "$(oracle "$repo" config-list)"
    if git -C "$repo" rev-parse --verify -q HEAD > /dev/null; then
        same "ls-tree HEAD in $repo" \
            "$(git -C "$repo" ls-tree HEAD)" "$(oracle "$repo" ls-tree HEAD)"
        same "ls-tree -r HEAD in $repo" \
            "$(git -C "$repo" ls-tree -r HEAD)" \
            "$(oracle "$repo" ls-tree -r HEAD)"
    fi
    if [ "$(git -C "$repo" rev-parse --is-bare-repository)" = false ]; then
        same "ls-files -s in $repo" \
            "$(git -C "$repo" ls-files -s)" "$(oracle "$repo" ls-files)"
    fi
}

# statuses REPOSITORY: status --porcelain --no-renames in each mode
statuses() {
    for mode in "" "-uall" "-uno" "--ignored" "-uall --ignored"; do
        # shellcheck disable=SC2086
        same "status $mode in $1" \
            "$(git -C "$1" status --porcelain --no-renames $mode 2>/dev/null)" \
            "$(oracle "$1" status $mode)"
    done
}

# ignores REPOSITORY PATH...: check-ignore -v of each, by itself
ignores() {
    repo=$1
    shift
    for path in "$@"; do
        same "check-ignore $path in $repo" \
            "$(git -C "$repo" check-ignore -v --non-matching --no-index \
                   -- "$path")" \
            "$(oracle "$repo" check-ignore "$path")"
    done
}

# attributes REPOSITORY PATH...: check-attr -a of each, sorted
attributes() {
    repo=$1
    shift
    for path in "$@"; do
        same "check-attr $path in $repo" \
            "$(git -C "$repo" check-attr -a -- "$path" 2>/dev/null | sort)" \
            "$(oracle "$repo" check-attr "$path" | sort)"
    done
}

# --- a history ---------------------------------------------------------------
R=$work/r
git init -q "$R"
printf 'one\n' > "$R/a.txt"
mkdir -p "$R/src/lib"
seq 1 400 | sed 's/^/line /' > "$R/src/lib/big.c"
git -C "$R" add -A
commit "$R" "feat: the first"
git -C "$R" tag lw
for i in 1 2 3 4 5; do
    printf 'change %s\n' "$i" >> "$R/src/lib/big.c"
    git -C "$R" add -A
    commit "$R" "fix: change $i"
done
git -C "$R" tag -a v1 -m "version one"
git -C "$R" tag -a v1-of-tag -m "a tag of a tag" v1
git -C "$R" checkout -q -b side HEAD~3
printf 'side\n' > "$R/side.txt"
git -C "$R" add -A
commit "$R" "feat(side)!: a side"
printf 'more\n' >> "$R/side.txt"
git -C "$R" add -A
commit "$R" "docs: more"
git -C "$R" checkout -q main
GIT_COMMITTER_DATE="@$((tick + 60))" GIT_AUTHOR_DATE="@$((tick + 60))" \
    git -C "$R" merge -q --no-edit side
tick=$((tick + 60))
printf 'two\n' > "$R/a.txt"
git -C "$R" add -A
commit "$R" "perf: two" -m "BREAKING CHANGE: a is two"
git -C "$R" tag -a v2 -m "version two"
git -C "$R" checkout -q -b topic
printf 't\n' > "$R/t.txt"
git -C "$R" add -A
commit "$R" "topic"
git -C "$R" checkout -q main
git -C "$R" gc -q
printf 'loose\n' > "$R/loose.txt"
git -C "$R" add -A
commit "$R" "after the pack"
git clone -q --bare "$R" "$work/r.git"
git -C "$R" remote add origin "$work/r.git"
git -C "$R" fetch -q origin
git -C "$R" branch -q -u origin/main main
git -C "$R" config branch.topic.remote .
git -C "$R" config branch.topic.merge refs/heads/main

revs "$R" HEAD @ main side topic lw v1 v1-of-tag v2 "v1^{}" "v1-of-tag^{}" \
    "v1-of-tag^{tag}" "v1^{commit}" "HEAD^{tree}" HEAD~1 HEAD~2 HEAD^2 \
    HEAD~2^2 "HEAD^^2~1" HEAD~9 HEAD~99 "HEAD:a.txt" "HEAD:src/lib" \
    "HEAD:src/lib/big.c" "side:side.txt" ":a.txt" ":0:a.txt" "HEAD:nope" \
    origin/main refs/remotes/origin/main heads/main tags/v1 "HEAD@{0}" \
    "HEAD@{1}" "HEAD@{4}" "main@{1}" "@{-1}" "@{-2}" "@{u}" "main@{u}" \
    "topic@{upstream}" "side@{u}" nosuch "HEAD^{blob}" \
    "$(git -C "$R" rev-parse HEAD | cut -c1-5)" \
    "$(git -C "$R" rev-parse HEAD | cut -c1-12)"
walks "$R" HEAD "HEAD ^side" "side ^HEAD" "--first-parent HEAD" \
    "HEAD -- side.txt" "HEAD -- src/lib/big.c" "HEAD -- src" "-n 3 HEAD" \
    "v1 side" "HEAD ^v1" "topic ^main" "--first-parent -n 2 HEAD"
for pair in "main side" "v1 side" "topic side" "main topic" "lw v2"; do
    # shellcheck disable=SC2086
    same "merge-base $pair" "$(git -C "$R" merge-base --all $pair)" \
        "$(oracle "$R" merge-base $pair)"
done
objects "$R"
objects "$work/r.git"
reads "$R"
reads "$work/r.git"
revs "$work/r.git" HEAD main v1 "v2^{}" HEAD~3
walks "$work/r.git" HEAD "main ^side"

# --- a work tree of every change ---------------------------------------------
W=$work/w
git init -q "$W"
cd "$W" || exit 1
printf '*.log\n!keep.log\nbuild/\n/top.txt\ndoc/**/*.tmp\n' > .gitignore
mkdir -p sub doc/a build
printf '*.txt\n!keep.log\nlocal/\n' > sub/.gitignore
printf 'secret\n' >> .git/info/exclude
printf '*.c text diff=cpp\n*.bin binary\n*.crlf text eol=crlf\n' \
    > .gitattributes
printf '[attr]mine text eol=lf\n*.m mine\nsub/** -diff\n' \
    >> .gitattributes
printf '*.c -diff\n' > sub/.gitattributes
printf 'same\n' > same.c
printf 'change\n' > changed.c
printf 'gone\n' > gone.c
printf 'staged\n' > staged.c
printf 'type\n' > type.c
printf 'x\n' > 'sp ace.c'
printf 'x\n' > "$(printf 'tab\there.c')"
printf 'x\n' > "$(printf '\303\244.c')"
printf 'x\n' > 'q"uote.c'
printf 'a\r\nb\r\n' > dos.crlf
printf 'bin\0ary\n' > data.bin
printf 'm\n' > x.m
printf 'skip\n' > skip.c
printf 'assume\n' > assume.c
printf 'c\n' > sub/inner.c
printf 'exec\n' > exec.sh
chmod +x exec.sh
ln -s same.c link
git add -A
commit . "the work tree"
git checkout -q -b other
printf 'theirs\n' > conflict.c
git add conflict.c
commit . "theirs"
git checkout -q main
printf 'ours\n' > conflict.c
git add conflict.c
commit . "ours"
git merge -q other > /dev/null 2>&1
printf 'changed!\n' > changed.c
rm gone.c
printf 'staged!\n' > staged.c
git add staged.c
rm type.c
ln -s same.c type.c
chmod -x exec.sh
printf 'new\n' > added.c
git add -N added.c
git update-index --skip-worktree skip.c
printf 'not seen\n' > skip.c
git update-index --assume-unchanged assume.c
printf 'not seen either\n' > assume.c
printf 'u\n' > untracked.c
mkdir -p newdir/deep onlyignored empty
printf 'n\n' > newdir/deep/n.c
printf 'i\n' > onlyignored/i.log
printf 'i\n' > newdir/i.log
printf 'l\n' > a.log
printf 'k\n' > keep.log
printf 'b\n' > build/out.o
printf 's\n' > secret
printf 't\n' > top.txt
printf 't\n' > sub/top.txt
mkdir -p sub/local
printf 'l\n' > sub/local/f
cd "$ROOT" || exit 1

statuses "$W"
reads "$W"
objects "$W"
ignores "$W" a.log keep.log sub/keep.log build build/out.o top.txt \
    sub/top.txt x/top.txt doc/a/b/c.tmp doc/c.tmp secret sub/secret \
    sub/local/f sub/local untracked.c onlyignored/i.log newdir/i.log
attributes "$W" same.c sub/inner.c data.bin dos.crlf x.m sub/x.m none.txt \
    "sp ace.c" sub/deep/y.c
revs "$W" HEAD MERGE_HEAD ":1:conflict.c" ":2:conflict.c" ":3:conflict.c" \
    ":conflict.c" ":same.c" "HEAD:link"

# --- a linked work tree, a submodule, SHA-256 --------------------------------
git -C "$R" worktree add -q "$work/linked" side 2> /dev/null ||
    git -C "$R" worktree add -q -f "$work/linked" -b linked-side side
printf 'linked\n' > "$work/linked/l.txt"
revs "$work/linked" HEAD main "HEAD@{0}"
statuses "$work/linked"
reads "$work/linked"

S=$work/s
git init -q "$S"
printf 'top\n' > "$S/top.txt"
git -C "$S" add -A
commit "$S" "top"
git -C "$S" submodule --quiet add "$R" lib 2> /dev/null
commit "$S" "a submodule"
printf 'dirty\n' > "$S/lib/dirty.txt"
statuses "$S"
reads "$S"
revs "$S" HEAD "HEAD:lib" ":lib"
# what a submodule's changes count for: .gitmodules, the configuration,
# diff.ignoreSubmodules; untracked files, tracked ones changed, a commit
for level in untracked dirty all none bogus; do
    git -C "$S" config -f .gitmodules submodule.lib.ignore "$level"
    statuses "$S"
done
git -C "$S" config -f .gitmodules --unset submodule.lib.ignore
git -C "$S" config diff.ignoreSubmodules dirty
statuses "$S"
git -C "$S" config --unset diff.ignoreSubmodules
git -C "$S" config submodule.lib.ignore all
statuses "$S"
git -C "$S" config --unset submodule.lib.ignore
printf 'changed\n' >> "$S/lib/a.txt"
statuses "$S"
git -C "$S/lib" add -A
commit "$S/lib" "moved on"
statuses "$S"
git -C "$S" config -f .gitmodules submodule.lib.ignore dirty
statuses "$S"

H=$work/h
if git init -q --object-format=sha256 "$H" 2> /dev/null; then
    printf 'one\n' > "$H/a"
    git -C "$H" add -A
    commit "$H" one
    printf 'two\n' > "$H/a"
    git -C "$H" add -A
    commit "$H" two
    git -C "$H" tag -a t -m t
    git -C "$H" gc -q
    printf 'three\n' > "$H/a"
    revs "$H" HEAD HEAD~1 t "t^{}" "HEAD:a"
    walks "$H" HEAD
    objects "$H"
    reads "$H"
    statuses "$H"
else
    echo "note: this git makes no SHA-256 repositories"
fi

# --- the repository this runs in ---------------------------------------------
if git -C "$ROOT" rev-parse --git-dir > /dev/null 2>&1 &&
   [ -z "${GIT_SKIP_SELF:-}" ]; then
    objects "$ROOT"
    revs "$ROOT" HEAD HEAD~1 HEAD~10
    walks "$ROOT" "-n 200 HEAD" "--first-parent -n 50 HEAD"
    same "status of the repository this runs in" \
        "$(git -C "$ROOT" status --porcelain --no-renames)" \
        "$(oracle "$ROOT" status)"
fi

# --- configurations at random -------------------------------------------------
AWK=$DAWK
[ -x "$AWK" ] || AWK=awk
"$AWK" -v dir="$work/cf" 'BEGIN {
    n = split("[ ] \" \\ \n \r\n a B = . # ; x 1 - n t \\n " \
              "[a]\n [a\"b\"]\n k=", piece, " ")
    piece[n + 1] = " "; piece[n + 2] = "\t"; n += 2
    srand(7)
    for (c = 1; c <= 400; c++) {
        text = ""
        m = int(rand() * 14) + 1
        for (i = 1; i <= m; i++)
            text = text piece[int(rand() * n) + 1]
        printf "%s", text > (dir "/" c)
        close(dir "/" c)
    }
}' 2> /dev/null || mkdir -p "$work/cf"
mkdir -p "$work/cf"
differ=0
for f in "$work"/cf/*; do
    [ -f "$f" ] || continue
    if want=$(git config -f "$f" --list 2> "$work/err"); then
        :
    else
        want=$(sed -n 's/^fatal: \(bad config line [0-9]*\).*/BAD \1/p' \
                   "$work/err")
    fi
    got=$("$HELPER" --oracle "$f" config-file "$f")
    if [ "$want" != "$got" ]; then
        differ=$((differ + 1))
        [ "$differ" -le 3 ] && same "config -f $(basename "$f")" "$want" "$got"
    fi
done
if [ "$differ" -eq 0 ]; then
    pass=$((pass + 1))
elif [ "$differ" -gt 3 ]; then
    fail=$((fail + 1))
    echo "FAIL $differ configurations"
fi

# --- dawk -x git, and its scripts ---------------------------------------------
if [ ! -x "$DAWK" ]; then
    echo "note: no dawk; the extension and its scripts are not run"
    echo
    echo "git: $pass passed, $fail failed"
    [ "$fail" -eq 0 ]
    exit
fi
same "git_rev and git_short" \
    "$(git -C "$R" rev-parse HEAD) $(git -C "$R" rev-parse --short v1)" \
    "$(cd "$R" && "$DAWK" -x git 'BEGIN { r = git_repo()
         print git_rev(r, "HEAD"), git_short(r, "v1") }' 2>&1)"
same "git_log" "$(git -C "$R" rev-list HEAD ^side)" \
    "$(cd "$R" && "$DAWK" -x git 'BEGIN { r = git_repo()
         printf "%s", git_log(r, "HEAD ^side") }' 2>&1)"
# names git's porcelain quotes and git_status writes as they are aside
plain() {
    grep -v -e '"' -e 'sp ace' -e "$(printf '\303')" -e "$(printf '\t')"
}
same "git_status" \
    "$(git -C "$W" status --porcelain --no-renames -uall | plain)" \
    "$(cd "$W" && "$DAWK" -x git 'BEGIN { r = git_repo()
         printf "%s", git_status(r, "-uall") }' 2>&1 | plain)"
same "git_ignore.awk -v mode=check" \
    "$(cd "$W" && git check-ignore -v --non-matching --no-index a.log \
           keep.log build/out.o untracked.c)" \
    "$(cd "$W" && "$DAWK" -x git -v mode=check \
           -f "$ROOT/$RES/git_ignore.awk" a.log keep.log build/out.o \
           untracked.c 2>&1)"
printf '*.o\n' > "$work/ig"
same "git_ignore.awk adds" "$work/ig: added 2 patterns
*.o
# made here
*.tmp
dist/" \
    "$("$DAWK" -x git -v file="$work/ig" -v comment="made here" \
           -f "$RES/git_ignore.awk" '*.o' '*.tmp' 'dist/' 2>&1 |
       grep -v 'already there'; cat "$work/ig")"
same "git_changelog.awk" "# Changes since v2

- after the pack (\`$(git -C "$R" rev-parse --short HEAD)\`)" \
    "$(cd "$R" && "$DAWK" -x git -v types=0 \
           -f "$ROOT/$RES/git_changelog.awk" 2>&1)"
survey=$(cd "$S" && "$DAWK" -x git -f "$ROOT/$RES/git_survey.awk" 2>&1)
case $survey in
    *"1 submodule"*) pass=$((pass + 1)) ;;
    *) fail=$((fail + 1)); echo "FAIL git_survey.awk"
       printf '%s\n' "$survey" | sed 's/^/    /' ;;
esac

echo
echo "git: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
