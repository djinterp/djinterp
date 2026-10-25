#!/bin/sh
# wiki.sh -- the -x wiki extension, wiki_survey.awk, wiki_rename.awk,
# wiki_toc.awk and wiki_generate.awk, against outputs fixed in advance; then
# the reader and the documents over the pages made here, and, where they are
# installed, other readers over the same pages.
#
# Each case runs dawk over a small wiki written here, a GitHub wiki's pages
# in Markdown and a MediaWiki's, and compares what it prints, its status,
# and, when it writes, the bytes it wrote. The generator is run over a small
# tree of headers made here: a banner, a table of contents, a Doxygen
# comment, a run of macros, a struct and a class, in both dialects, twice,
# the second run writing nothing. After the cases: every page here read and
# proved sound; random changes made to them, each proved to touch only what
# it said; and, when python3 has markdown-it-py and pymdownx, or pandoc is
# installed, the pages read by them too -- and by MediaWiki itself, with its
# own parser tests, when MW_INSTALL_PATH names a MediaWiki 1.43 set up as
# wiki_mw_records.php says.
#
#   sh tests/djinterp/tools/dawk/ext/wiki/wiki.sh [build/dawk] [build/test_dwiki]
DAWK=${1:-build/dawk}
READER=${2:-build/test_dwiki}
HERE=tests/djinterp/tools/dawk/ext/wiki
RES=res/djinterp/tools/dawk/ext/wiki
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
pass=0
fail=0

# same NAME WANT ARGS...: dawk's output and status must be WANT exactly,
# its last line the status
same() {
    name=$1
    want=$2
    shift 2
    got=$("$DAWK" "$@" 2>&1; echo "status $?")
    if [ "$got" = "$want" ]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL $name"
        printf '%s\n' "$want" | sed 's/^/    want: /'
        printf '%s\n' "$got"  | sed 's/^/    got:  /'
    fi
}

# holds NAME FILE WANT-FILE: the file must hold the bytes WANT-FILE does
holds() {
    if cmp -s "$2" "$3"; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL $1"
        diff "$3" "$2" | sed 's/^/    /'
    fi
}

# check NAME COMMAND...: the command must succeed
check() {
    name=$1
    shift
    if out=$("$@" 2>&1); then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL $name"
        printf '%s\n' "$out" | tail -20 | sed 's/^/    /'
    fi
}

# -- the wikis ----------------------------------------------------------------
M=$work/md
W=$work/mw
mkdir -p "$M/docs" "$W" "$work/plain" "$work/src/demo/sub"
cat > "$M/Home.md" <<'EOF'
# Home

Welcome. See [[Usage]], [[the guide|Usage#install]] and [[Missing Page]].
Read [the manual](docs/manual.txt) or [the lost file](docs/lost.txt).
Jump to [nowhere](#no-such-anchor) or [setup](#setup).

## Setup

Run it.

<!-- dawk:begin toc -->
<!-- dawk:end toc -->
EOF
cat > "$M/Usage.md" <<'EOF'
# Usage

Back [[Home]]. Also [old](Old-Name.md) and [[Old Name#part]].

## Install

Steps.

## Install

Again.
EOF
cat > "$M/Old-Name.md" <<'EOF'
# Old Name

Called [[Old Name]] by [[Home]].

## Part

A part.
EOF
cat > "$M/Lonely.md" <<'EOF'
Nobody links here, and [[Usage#nothing]] is not there.
EOF
cat > "$M/_Sidebar.md" <<'EOF'
* [[Home]]
* [[Usage]]
EOF
echo "manual" > "$M/docs/manual.txt"
cat > "$W/Main_Page.mediawiki" <<'EOF'
{{Infobox
| name = Demo
}}
Welcome to [[Guide]] and [[Guide#Usage|the usage]] and [[Gone]].
[[Category:Demo]] [[File:Logo.png|thumb|Logo]] [[Help:Contents]]
== Usage ==
Text.
EOF
cat > "$W/Guide.mediawiki" <<'EOF'
== Usage ==
See [[Main Page]] and [[Main_Page#Usage]] and [[Main Page#Nothing]].
EOF
cat > "$work/plain/Plain.md" <<'EOF'
---
title: plain
---
# Plain page

A paragraph with *emphasis*, `code`, a [link](https://e.example/a "title"),
an ![image](pic.png), <https://auto.example/>, www.bare.example and a
[reference][ref] to a [shortcut].

[ref]: https://r.example/
[shortcut]: ./Other.md

Setext :tada:
-------------

> A quote with a [quoted link](q.md).

1. one
2. two
   - nested [deep](deep.md)

```python
print("[not](a link)")
```

    indented [not](either)

| a | b |
|---|---|
| [cell](cell.md) | ~~gone~~ |

<div>
<a href="raw.md">raw</a>
</div>
EOF

# -- reading a page -----------------------------------------------------------
cat > "$work/read.awk" <<'EOF'
BEGIN {
    p = wiki_open(file)
    print "page", p, wiki_page(p), wiki_dialect(p), wiki_path(p) == file
    print "sections", wiki_sections(p)
    for (s = 0; s <= wiki_sections(p); s++)
        print "  section", s, wiki_section(p, s, "level"),
              "[" wiki_section(p, s, "title") "]",
              "[" wiki_section(p, s, "anchor") "]",
              wiki_section(p, s, "line"), wiki_section(p, s, "end_line"),
              wiki_section(p, s, "parent"),
              "[" wiki_section(p, s, "path") "]"
    i = wiki_find_section(p, "Install")
    print "find", i, wiki_find_section(p, "Install", i),
          wiki_find_section(p, "install-1"),
          wiki_find_section(p, "Usage/Install"),
          wiki_find_section(p, "nothing")
    print "body [" wiki_section(p, 1, "body") "]"
    for (l = 1; l <= wiki_links(p); l++)
        print "  link", l, wiki_link(p, l, "form"), wiki_link(p, l, "kind"),
              "[" wiki_link(p, l, "target") "]",
              "[" wiki_link(p, l, "page") "]",
              "[" wiki_link(p, l, "anchor") "]",
              "[" wiki_link(p, l, "label") "]",
              wiki_link(p, l, "line") ":" wiki_link(p, l, "column"),
              "[" wiki_link(p, l, "text") "]"
    print "find link", wiki_find_link(p, "old name"),
          wiki_find_link(p, "Home"), wiki_find_link(p, "nothing")
    for (b = 1; b <= wiki_blocks(p); b++)
        print "  block", b, wiki_block(p, b, "kind"), wiki_block(p, b, "line"),
              wiki_block(p, b, "end_line"), wiki_block(p, b, "depth")
    print "headings", wiki_headings(p), wiki_heading(p, 3, "anchor"),
          wiki_heading(p, 3, "top")
    print "bad field", wiki_link(p, 1, "nonsense"), "[" wiki_error() "]"
    print "bad index", wiki_link(p, 99, "target"), "[" wiki_error() "]"
    print "close", wiki_close(p), wiki_close(p)
}
EOF
same 'a page read: its sections, links and blocks' "page 1 Usage markdown 1
sections 3
  section 0 0 [] [] 1 1 0 []
  section 1 1 [Usage] [usage] 1 11 0 [Usage]
  section 2 2 [Install] [install] 5 8 1 [Usage/Install]
  section 3 2 [Install] [install-1] 9 11 1 [Usage/Install]
find 2 3 3 2 0
body [
Back [[Home]]. Also [old](Old-Name.md) and [[Old Name#part]].

]
  link 1 wiki page [Home] [Home] [] [Home] 3:6 [[[Home]]]
  link 2 inline page [Old-Name.md] [Old-Name] [] [old] 3:21 [[old](Old-Name.md)]
  link 3 wiki page [Old Name#part] [Old Name] [part] [Old Name#part] 3:44 [[[Old Name#part]]]
find link 2 1 0
  block 1 heading 1 1 0
  block 2 paragraph 3 3 0
  block 3 heading 5 5 0
  block 4 paragraph 7 7 0
  block 5 heading 9 9 0
  block 6 paragraph 11 11 0
headings 3 install-1 1
bad field  [no field nonsense]
bad index -1 [the page has no link 99]
close 1 0
status 0" -x wiki -f "$work/read.awk" -v file="$M/Usage.md"

# -- changing a page ----------------------------------------------------------
cat > "$work/change.awk" <<'EOF'
BEGIN {
    p = wiki_open(file)
    print "page", wiki_page(p), wiki_dialect(p), wiki_sections(p),
          wiki_templates(p)
    t = wiki_find_template(p, "infobox")
    print "template", t, wiki_template(p, t, "name"),
          wiki_template(p, t, "kind"), wiki_template(p, t, "args"),
          wiki_template(p, t, "line"), "[" wiki_arg(p, t, "name") "]",
          wiki_has_arg(p, t, "name"), wiki_has_arg(p, t, "nothing"),
          wiki_arg_key(p, t, 1)
    print "set", wiki_set_arg(p, t, "name", "Changed"),
          wiki_set_arg(p, t, "license", "MIT"),
          wiki_remove_arg(p, t, "nothing")
    for (l = 1; l <= wiki_links(p); l++)
        print "  link", l, wiki_link(p, l, "form"), wiki_link(p, l, "kind"),
              "[" wiki_link(p, l, "target") "]",
              "[" wiki_link(p, l, "page") "]",
              "[" wiki_link(p, l, "label") "]", wiki_link(p, l, "embedded")
    print "relink", wiki_set_link(p, wiki_find_link(p, "Guide"), "Manual"),
          wiki_set_label(p, 2, "the manual's usage"),
          wiki_set_link(p, 3, "A]]B"), "[" wiki_error() "]"
    print "section", wiki_add_section(p, 0, "History", "Long ago."),
          wiki_rename_section(p, 1, "Using it"),
          wiki_set_body(p, 1, "x\n== Not a body ==\n"), "[" wiki_error() "]"
    print "save", wiki_save(p, out), wiki_save(p, out), wiki_changes(p)
}
EOF
same 'a page changed: arguments, links, sections; refusals; saved once' "page Main Page mediawiki 1 1
template 1 Infobox template 1 1 [Demo] 1 0 name
set 1 1 -1
  link 1 wiki page [Guide] [Guide] [Guide] 0
  link 2 wiki page [Guide#Usage] [Guide] [the usage] 0
  link 3 wiki page [Gone] [Gone] [Gone] 0
  link 4 wiki category [Category:Demo] [Category:Demo] [] 0
  link 5 wiki image [File:Logo.png] [File:Logo.png] [Logo] 1
  link 6 wiki page [Help:Contents] [Help:Contents] [Help:Contents] 0
relink 1 1 -1 [the destination cannot be written in this link's form]
section 2 1 -1 [the text holds a heading: insert a section for it]
save 1 0 6
status 0" -x wiki -f "$work/change.awk" -v file="$W/Main_Page.mediawiki" \
    -v out="$work/out/Main_Page.mediawiki"
cat > "$work/want" <<'EOF'
{{Infobox
| name = Changed
| license = MIT
}}
Welcome to [[Manual]] and [[Guide#Usage|the manual's usage]] and [[Gone]].
[[Category:Demo]] [[File:Logo.png|thumb|Logo]] [[Help:Contents]]
== Using it ==
Text.

== History ==
Long ago.
EOF
holds 'the page as saved, the rest of it as it was' \
    "$work/out/Main_Page.mediawiki" "$work/want"
holds 'the page read is left as it was' "$W/Main_Page.mediawiki" /dev/stdin <<'EOF'
{{Infobox
| name = Demo
}}
Welcome to [[Guide]] and [[Guide#Usage|the usage]] and [[Gone]].
[[Category:Demo]] [[File:Logo.png|thumb|Logo]] [[Help:Contents]]
== Usage ==
Text.
EOF

# -- writing ------------------------------------------------------------------
cat > "$work/write.awk" <<'EOF'
BEGIN {
    split("markdown mediawiki", ds, " ")
    for (i = 1; i <= 2; i++) {
        d = ds[i]
        print "--", d
        printf "%s", wiki_heading_text(d, 2, "Usage")
        print wiki_link_text(d, "Other Page")
        print wiki_link_text(d, "Other Page", "shown", "part")
        print wiki_url_text(d, "https://e.example/a", "the site")
        print wiki_image_text(d, "logo.png", "The logo")
        printf "%s", wiki_code_text(d, "int x;", "c")
        print wiki_inline_code(d, "a`b")
        printf "%s", wiki_item_text(d, "deep", 2, 1)
        printf "%s", wiki_table_text(d, "A\tB\n1\t2\n")
        print wiki_escape(d, "*not* [[a link]]")
        print wiki_comment_text(d, "note")
        printf "%s", wiki_region_text(d, "gen", "made")
        print wiki_anchor(d, "Hello, World!"), wiki_page_file("Page Name", d),
              wiki_page_key("Page_Name", d),
              wiki_same_page("page name", "Page-Name", d)
    }
    print wiki_page_name("wiki/Page-Name.md"),
          wiki_page_name("Main_Page.mediawiki"), wiki_dialect_of("x.wiki"),
          wiki_dialect_of("x.txt")
    print "bad", wiki_link_text("rst", "x"), "[" wiki_error() "]"
    print "bad", wiki_link_text("mediawiki", "a]]b"), "[" wiki_error() "]"
}
EOF
same 'the writers, both dialects' "-- markdown
## Usage
[[Other Page]]
[[shown|Other Page#part]]
[the site](https://e.example/a)
![The logo](logo.png)
\`\`\`c
int x;
\`\`\`
\`\`a\`b\`\`
    1. deep
| A | B |
| --- | --- |
| 1 | 2 |
\\*not\\* \\[\\[a link\\]\\]
<!-- note -->
<!-- dawk:begin gen -->
made
<!-- dawk:end gen -->
hello-world Page-Name.md page name 1
-- mediawiki
== Usage ==
[[Other Page]]
[[Other Page#part|shown]]
[https://e.example/a the site]
[[File:logo.png|alt=The logo]]
<syntaxhighlight lang=\"c\">
int x;
</syntaxhighlight>
<code><nowiki>a\`b</nowiki></code>
## deep
{| class=\"wikitable\"
! A !! B
|-
| 1 || 2
|}
&#42;not* &#91;&#91;a link&#93;&#93;
<!-- note -->
<!-- dawk:begin gen -->
made
<!-- dawk:end gen -->
Hello,_World! Page-Name.mediawiki page Name 0
Page Name Main Page mediawiki other
bad -1 [rst is not a dialect]
bad -1 [the text cannot be written so]
status 0" -x wiki -f "$work/write.awk"

# -- files ---------------------------------------------------------------------
cp -r "$M" "$work/files"
cat > "$work/files.awk" <<'EOF'
BEGIN {
    printf "pages:\n%s\n", wiki_pages(dir, 1)
    printf "headers:\n%s\n", wiki_files(src, "*.h *.hpp", 1)
    printf "none:[%s]\n", wiki_files(src, "*.c", 1)
    print "exists", wiki_exists(dir), wiki_exists(dir "/Home.md"),
          wiki_exists(dir "/nothing")
    print "open", wiki_open(dir "/nothing.md"), "[" wiki_error() "]"
    p = wiki_new(dir "/Fresh.md", "markdown", "# Fresh\n")
    print "new", p, wiki_dialect(p), wiki_sections(p), wiki_save(p),
          wiki_exists(dir "/Fresh.md")
    print "move", wiki_move(dir "/Fresh.md", dir "/Moved.md"),
          wiki_exists(dir "/Moved.md"),
          wiki_move(dir "/Moved.md", dir "/Home.md"), "[" wiki_error() "]"
}
EOF
cat > "$work/src/demo/thing.h" <<'EOF'
/*******************************************************************************
* demo                                                                 thing.h
*
* A thing, made and freed.
*   Things are counted.
*
* path:      /src/demo/thing.h
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    1.  thing
2.  OPERATIONS
*/

#ifndef DEMO_THING_H
#define DEMO_THING_H 1

#include "./sub/other.hpp"


//==============================================================================
// 1.  TYPES
//==============================================================================

// thing
//   struct: one thing.
struct thing
{
    int count;
};

#define THING_MOST 10
#define THING_LEAST 1


//==============================================================================
// 2.  OPERATIONS
//==============================================================================

/**
 * @brief Makes a thing.
 *
 * @param[in] _count  how many.
 * @return the thing; NULL when memory runs out.
 */
struct thing* thing_new(int _count);

// thing_free frees a thing.
void thing_free(struct thing* _thing);


#endif  // DEMO_THING_H
EOF
cat > "$work/src/demo/sub/other.hpp" <<'EOF'
// other.hpp -- another thing, in C++.
//
// An other holds a value.

#pragma once

namespace demo
{

/// An other.
class other
{
public:
    /// Its value.
    int value() const;
};

}  // namespace demo
EOF
same 'pages and files found, made and moved' "pages:
$work/files/Home.md
$work/files/Lonely.md
$work/files/Old-Name.md
$work/files/Usage.md
$work/files/_Sidebar.md
headers:
$work/src/demo/sub/other.hpp
$work/src/demo/thing.h
none:[]
exists 2 1 0
open -1 [cannot read $work/files/nothing.md]
new 1 markdown 1 1 1
move 1 1 -1 [$work/files/Home.md is there already]
status 0" -x wiki -f "$work/files.awk" -v dir="$work/files" -v src="$work/src"

# -- wiki_survey.awk ----------------------------------------------------------
same 'a GitHub wiki surveyed' "$M/Home.md: markdown, 2 sections, 7 links (3 page, 2 anchor, 2 file), 1 region
    line 3: a link to \"Missing Page\": no such page
    line 4: a link to \"docs/lost.txt\": no such file
    line 5: a link to \"#no-such-anchor\": no such anchor here
$M/Lonely.md: markdown, 0 sections, 1 link (1 page)
    line 1: a link to \"Usage#nothing\": no such anchor there
$M/Old-Name.md: markdown, 2 sections, 2 links (2 page)
$M/Usage.md: markdown, 3 sections, 3 links (3 page)
$M/_Sidebar.md: markdown, 0 sections, 2 links (2 page)
pages no other page links to:
    $M/Lonely.md
5 pages: 15 links, 4 broken; 1 orphan
status 1" -x wiki -f "$RES/wiki_survey.awk" "$M"
same '...a MediaWiki, quiet: another namespace left alone' "$W/Guide.mediawiki:
    line 2: a link to \"Main Page#Nothing\": no such anchor there
$W/Main_Page.mediawiki:
    line 4: a link to \"Gone\": no such page
2 pages: 9 links, 2 broken; 0 orphans
status 1" -x wiki -f "$RES/wiki_survey.awk" -v quiet=1 "$W"

# -- wiki_rename.awk ----------------------------------------------------------
cp -r "$M" "$work/renamed"
R=$work/renamed
same 'a rename, dry' "$R/Old-Name.md: line 3: Old Name -> New Name
$R/Usage.md: line 3: Old-Name.md -> New-Name.md
$R/Usage.md: line 3: Old Name#part -> New Name#part
would move $R/Old-Name.md to $R/New-Name.md, and change 3 links
status 0" -x wiki -f "$RES/wiki_rename.awk" -v from="Old Name" -v to="New Name" \
    -v dry=1 "$R"
same '...and done' "moved $R/Old-Name.md to $R/New-Name.md; 3 links changed, on 2 pages
status 0" -x wiki -f "$RES/wiki_rename.awk" -v from="old name" -v to="New Name" \
    "$R"
holds '...every form of link renamed as it was written' "$R/Usage.md" /dev/stdin <<'EOF'
# Usage

Back [[Home]]. Also [old](New-Name.md) and [[New Name#part]].

## Install

Steps.

## Install

Again.
EOF
same '...and the wiki as broken as it was' "$R/Home.md:
    line 3: a link to \"Missing Page\": no such page
    line 4: a link to \"docs/lost.txt\": no such file
    line 5: a link to \"#no-such-anchor\": no such anchor here
$R/Lonely.md:
    line 1: a link to \"Usage#nothing\": no such anchor there
pages no other page links to:
    $R/Lonely.md
5 pages: 15 links, 4 broken; 1 orphan
status 1" -x wiki -f "$RES/wiki_survey.awk" -v quiet=1 "$R"
same 'a rename to a page there already' "a page is named \"Home\" already: $R/Home.md
status 1" -x wiki -f "$RES/wiki_rename.awk" -v from="Usage" -v to="Home" "$R"
cp -r "$W" "$work/mwrenamed"
same 'a MediaWiki rename, a redirect left' "moved $work/mwrenamed/Guide.mediawiki to $work/mwrenamed/Manual.mediawiki; 2 links changed, on 1 page
status 0" -x wiki -f "$RES/wiki_rename.awk" -v from="Guide" -v to="Manual" \
    -v redirect=1 "$work/mwrenamed"
holds '...the redirect' "$work/mwrenamed/Guide.mediawiki" /dev/stdin <<'EOF'
#REDIRECT [[Manual]]
EOF

# -- wiki_toc.awk -------------------------------------------------------------
cp -r "$M" "$work/toc"
T=$work/toc
same 'contents set in the region a page has' "5 pages: 1 changed, 0 as they were, 4 without a toc region
status 0" -x wiki -f "$RES/wiki_toc.awk" "$T"
holds '...as a list of links' "$T/Home.md" /dev/stdin <<'EOF'
# Home

Welcome. See [[Usage]], [[the guide|Usage#install]] and [[Missing Page]].
Read [the manual](docs/manual.txt) or [the lost file](docs/lost.txt).
Jump to [nowhere](#no-such-anchor) or [setup](#setup).

## Setup

Run it.

<!-- dawk:begin toc -->
- [Home](#home)
    - [Setup](#setup)
<!-- dawk:end toc -->
EOF
same '...added to the pages with headings, dry' "$T/Old-Name.md: would change
$T/Usage.md: would change
5 pages: 2 changed, 1 as they were, 2 without a toc region
status 0" -x wiki -f "$RES/wiki_toc.awk" -v add=1 -v dry=1 "$T"
same '...and done' "5 pages: 2 changed, 1 as they were, 2 without a toc region
status 0" -x wiki -f "$RES/wiki_toc.awk" -v add=1 "$T"
same '...and again, nothing to change' "5 pages: 0 changed, 3 as they were, 2 without a toc region
status 0" -x wiki -f "$RES/wiki_toc.awk" -v add=1 "$T"

# -- wiki_generate.awk --------------------------------------------------------
G=$work/generated
same 'pages generated from headers' "2 headers, 1 modules: 5 pages written, 0 as they were
status 0" -x wiki -f "$RES/wiki_generate.awk" -v out="$G" -v project=demo \
    -v strip="$work/src/" "$work/src"
same '...and again, nothing written' "2 headers, 1 modules: 0 pages written, 5 as they were
status 0" -x wiki -f "$RES/wiki_generate.awk" -v out="$G" -v project=demo \
    -v strip="$work/src/" "$work/src"
same '...every link leading somewhere' "5 pages: 12 links, 0 broken; 0 orphans
status 0" -x wiki -f "$RES/wiki_survey.awk" -v quiet=1 "$G"
holds '...a header page: banner, contents, entries, includes' "$G/demo-thing.h.md" /dev/stdin <<'EOF'
<!-- dawk:begin api -->
`demo/thing.h` · module [[demo|Module demo]] · [[Home]]

A thing, made and freed.

Things are counted.

## Contents

- Types
    - [thing](#thing)
- Operations

## Types

### thing

*struct*

one thing.

```c
struct thing
{
    int count;
};
```

### THING_MOST, THING_LEAST

*2 macros*

```c
#define THING_MOST 10
#define THING_LEAST 1
```

## Operations

### thing_new

*function*

Makes a thing.

| Parameter | Direction | Description |
| --- | --- | --- |
| `_count` | in | how many. |

*Returns* the thing; NULL when memory runs out.

```c
struct thing* thing_new(int _count);
```

### thing_free

*function*

thing_free frees a thing.

```c
void thing_free(struct thing* _thing);
```

## Includes

- [[demo/sub/other.hpp|demo-sub-other.hpp]]

<!-- dawk:end api -->
EOF
holds '...a module page' "$G/Module-demo.md" /dev/stdin <<'EOF'
<!-- dawk:begin api -->
[[Home]] · 2 headers

| Header | What it is |
| --- | --- |
| [[demo/sub/other.hpp|demo-sub-other.hpp]] | another thing, in C++. |
| [[demo/thing.h|demo-thing.h]] | A thing, made and freed. |
<!-- dawk:end api -->
EOF
printf 'Written by hand.\n\n' > "$work/hand"
cat "$G/Home.md" >> "$work/hand"
printf '\nAnd after.\n' >> "$work/hand"
cp "$work/hand" "$G/Home.md"
same '...a page with more than the region: only the region is the generator'"'"'s' "2 headers, 1 modules: 0 pages written, 5 as they were
status 0" -x wiki -f "$RES/wiki_generate.awk" -v out="$G" -v project=demo \
    -v strip="$work/src/" "$work/src"
holds '...the text about it kept' "$G/Home.md" "$work/hand"
GW=$work/generated_mw
same 'pages generated in MediaWiki'"'"'s dialect' "2 headers, 1 modules: 5 pages written, 0 as they were
status 0" -x wiki -f "$RES/wiki_generate.awk" -v out="$GW" -v dialect=mediawiki \
    -v project=demo -v strip="$work/src/" "$work/src"
same '...and again, nothing written' "2 headers, 1 modules: 0 pages written, 5 as they were
status 0" -x wiki -f "$RES/wiki_generate.awk" -v out="$GW" -v dialect=mediawiki \
    -v project=demo -v strip="$work/src/" "$work/src"
same '...every link leading somewhere' "5 pages: 12 links, 0 broken; 0 orphans
status 0" -x wiki -f "$RES/wiki_survey.awk" -v quiet=1 "$GW"
holds '...a module page' "$GW/Module-demo.mediawiki" /dev/stdin <<'EOF'
<!-- dawk:begin api -->
[[Home]] · 2 headers

{| class="wikitable"
! Header !! What it is
|-
| [[demo-sub-other.hpp|demo/sub/other.hpp]] || another thing, in C++.
|-
| [[demo-thing.h|demo/thing.h]] || A thing, made and freed.
|}
<!-- dawk:end api -->
EOF

# -- the reader and the documents, over the pages here ------------------------
check 'every page here read and proved sound' "$READER" --corpus "$work"
check 'random changes to them, each touching only what it said' \
    "$READER" --fuzz 1 "$work"
if command -v python3 > /dev/null 2>&1 &&
   python3 -I -c 'import markdown_it, pymdownx.gemoji_db' 2> /dev/null; then
    check 'the plain page, read by markdown-it' \
        python3 -I "$HERE/wiki_oracle.py" markdown-it "$READER" "$work/plain"
else
    echo "note: markdown-it-py is not installed; no page is read by it"
fi
if command -v pandoc > /dev/null 2>&1; then
    check 'the plain page, read by pandoc' \
        python3 -I "$HERE/wiki_oracle.py" pandoc-gfm "$READER" "$work/plain"
    check 'the MediaWiki pages, read by pandoc' \
        python3 -I "$HERE/wiki_oracle.py" pandoc-mediawiki "$READER" "$W"
else
    echo "note: pandoc is not installed; no page is read by it"
fi
if [ -n "${MW_INSTALL_PATH:-}" ] && command -v php > /dev/null 2>&1; then
    check 'the MediaWiki pages, read by MediaWiki' \
        python3 -I "$HERE/wiki_oracle.py" mediawiki "$READER" "$W"
    check "MediaWiki's own parser tests, read by MediaWiki" \
        python3 -I "$HERE/wiki_oracle.py" mediawiki "$READER" -parser-tests
else
    echo "note: MW_INSTALL_PATH is not set; no page is read by MediaWiki"
fi

echo
echo "wiki: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
