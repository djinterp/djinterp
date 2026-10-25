#!/usr/bin/env python3
# ******************************************************************************
# djinterp [tools]                                           gen_dwiki_tables.py
#
# Writes dwiki_tables.h, the tables the wiki reader looks characters up in:
#   - HTML's named character references, HTML 4's and &apos;;
#   - Unicode's punctuation and symbols (General_Category P* and S*), which
#     CommonMark's emphasis and GitHub's anchors read as punctuation;
#   - the characters of a word (L*, M*, Nd, Nl, Pc), which GitHub keeps in an
#     anchor;
#   - the space separators (Zs);
#   - each character's simple lower case, in runs;
#   - GitHub's emoji, by the names :name: gives them, from the gemoji
#     database pymdownx carries, if it is installed.
# Run it with the Python whose Unicode the tables should follow:
#       python3 gen_dwiki_tables.py > \
#           ../../../../../../src/djinterp/tools/dawk/ext/wiki/dwiki_tables.h
#
# path:      /res/djinterp/tools/dawk/ext/wiki/gen_dwiki_tables.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
#                                                            revised: 2026.10.19
# ******************************************************************************
import html.entities
import sys
import unicodedata

try:
    from pymdownx import gemoji_db
except ImportError:
    gemoji_db = None


def ranges(test):
    """Runs of code points above U+007F for which test holds."""
    out = []
    start = None
    for cp in range(0x80, 0x110000):
        if test(cp):
            if start is None:
                start = cp
        elif start is not None:
            out.append((start, cp - 1))
            start = None
    if start is not None:
        out.append((start, 0x10FFFF))
    return out


def category(cp):
    return unicodedata.category(chr(cp))


def lower_runs():
    """Runs of code points whose lower case is a fixed distance off, every
    code point of the run, or every other one."""
    pairs = []
    for cp in range(0x80, 0x110000):
        low = chr(cp).lower()
        if len(low) == 1 and ord(low) != cp:
            pairs.append((cp, ord(low) - cp))
    runs = []
    for cp, delta in pairs:
        if runs:
            start, end, step, last_delta = runs[-1]
            if last_delta == delta:
                if step == 0 and cp - end in (1, 2):
                    runs[-1] = (start, cp, cp - end, delta)
                    continue
                if step != 0 and cp - end == step:
                    runs[-1] = (start, cp, step, delta)
                    continue
        runs.append((cp, cp, 0, delta))
    return [(s, e, (st if st else 1), d) for s, e, st, d in runs]


def emoji_names():
    """Each emoji's name and text: GitHub's own, an image, has none."""
    if gemoji_db is None:
        return [], ""
    names = {}
    for key, entry in gemoji_db.emoji.items():
        code = entry.get("unicode_alt") or entry.get("unicode") or ""
        text = "".join(chr(int(part, 16)) for part in code.split("-") if part)
        names[key.strip(":")] = text
    for alias, key in gemoji_db.aliases.items():
        if key in gemoji_db.emoji:
            names[alias.strip(":")] = names[key.strip(":")]
    return sorted(names.items(), key=lambda item: item[0].encode()), \
        gemoji_db.version


def emit_count(name, count, out):
    out.write("#define %s_COUNT %du\n\n" % (name.upper(), count))


def emit_ranges(name, rows, out):
    out.write("static const uint32_t %s[][2] =\n{\n" % name)
    for low, high in rows:
        out.write("    { 0x%06X, 0x%06X },\n" % (low, high))
    out.write("};\n")
    emit_count(name, len(rows), out)


def utf8(text):
    return "".join("\\x%02X" % b for b in text.encode("utf-8"))


def main():
    out = sys.stdout
    punctuation = ranges(lambda cp: category(cp)[0] in "PS")
    word = ranges(lambda cp: (category(cp)[0] in "LM")
                  or (category(cp) in ("Nd", "Nl", "Pc")))
    space = ranges(lambda cp: category(cp) == "Zs")
    lower = lower_runs()
    emoji, emoji_version = emoji_names()
    names = dict(html.entities.name2codepoint)
    names["apos"] = 0x27
    entities = sorted(names.items())

    out.write("/* Auto-generated. Do not edit by hand.\n")
    out.write(" * res/djinterp/tools/dawk/ext/wiki/gen_dwiki_tables.py, from Python"
              " unicodedata %s.\n" % unicodedata.unidata_version)
    out.write(" * entities: HTML 4's named references and apos (%d);\n"
              % len(entities))
    out.write(" * punctuation: General_Category P* or S* (%d ranges);\n"
              % len(punctuation))
    out.write(" * word: L*, M*, Nd, Nl or Pc (%d ranges);\n" % len(word))
    out.write(" * space: Zs (%d ranges);\n" % len(space))
    out.write(" * lower: simple lower case, in runs (%d);\n" % len(lower))
    out.write(" * emoji: gemoji %s, by name (%d).\n"
              % (emoji_version or "(none)", len(emoji)))
    out.write(" * Every range above U+007F, sorted and disjoint.\n */\n")
    out.write("#ifndef DJINTERP_TOOLS_DAWK_EXT_WIKI_DWIKI_TABLES_H\n")
    out.write("#define DJINTERP_TOOLS_DAWK_EXT_WIKI_DWIKI_TABLES_H 1\n\n")
    out.write("#include <stdint.h>\n\n")
    out.write("#define D_INTERNAL_DWIKI_UNICODE_VERSION \"%s\"\n\n"
              % unicodedata.unidata_version)
    out.write("struct d_internal_dwiki_entity\n{\n    const char* name;\n"
              "    const char* text;\n};\n\n")
    out.write("static const struct d_internal_dwiki_entity "
              "d_internal_dwiki_entities[] =\n{\n")
    for name, cp in entities:
        out.write("    { \"%s\", \"%s\" },\n" % (name, utf8(chr(cp))))
    out.write("};\n")
    emit_count("d_internal_dwiki_entities", len(entities), out)
    emit_ranges("d_internal_dwiki_punctuation", punctuation, out)
    emit_ranges("d_internal_dwiki_word", word, out)
    emit_ranges("d_internal_dwiki_space", space, out)
    out.write("struct d_internal_dwiki_lower\n{\n    uint32_t first;\n"
              "    uint32_t last;\n    uint32_t step;\n    int32_t  delta;\n"
              "};\n\n")
    out.write("static const struct d_internal_dwiki_lower "
              "d_internal_dwiki_lowers[] =\n{\n")
    for first, last, step, delta in lower:
        out.write("    { 0x%06X, 0x%06X, %d, %d },\n"
                  % (first, last, step, delta))
    out.write("};\n")
    emit_count("d_internal_dwiki_lowers", len(lower), out)
    out.write("static const struct d_internal_dwiki_entity "
              "d_internal_dwiki_emoji[] =\n{\n")
    for name, text in emoji:
        out.write("    { \"%s\", \"%s\" },\n" % (name, utf8(text)))
    if not emoji:
        out.write("    { \"\", \"\" },\n")
    out.write("};\n")
    emit_count("d_internal_dwiki_emoji", len(emoji), out)
    out.write("#endif  /* DJINTERP_TOOLS_DAWK_EXT_WIKI_DWIKI_TABLES_H */\n")


if __name__ == "__main__":
    main()
