#!/usr/bin/env python3
#*******************************************************************************
# bitwisedom [tools]                                     winerror_to_bitfield.py
#
# The win32_hresults example, from a winerror.h: every HRESULT it defines, as
# a bitfield (docs/bitwisedom/json.md), and the platform check that compares each with
# the winerror.h a Windows build has.
#   The codes are grouped by facility, in the facility's order, each group's
# codes in order of value, and each group closed, to be opened as it is
# wanted; the HRESULT's own fields -- severity, the
# reserved, customer, NTSTATUS and X bits, the facility and the code -- come
# first, with every facility the header names as a value of the facility
# field. A facility whose codes the header has but whose FACILITY_ name it
# does not is named as the Windows SDK names it, and noted so.
#   Made from MinGW-w64's winerror.h, which is in the public domain:
#
#     python3 tools/winerror_to_bitfield.py \
#         /usr/share/mingw-w64/include/winerror.h \
#         res/djinterp/tools/bitwisedom/examples/large/win32_hresults.bitfield.json \
#         tests/djinterp/tools/bitwisedom/platform/win32_hresults.c
#
# path:      /src/djinterp/tools/bitwisedom/tools/winerror_to_bitfield.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
#                                                            revised: 2026.10.11
#*******************************************************************************
import collections
import re
import sys

import bitfield_json

# the facilities whose codes MinGW-w64's winerror.h has, under the names the
# Windows SDK gives them, which that header does not define -- with the
# whole facility number the SDK spells, where it reaches the X bit
SDK_FACILITIES = {
    39:  ("NAP", 0x27),
    40:  ("TPM_SERVICES", 0x28),
    50:  ("FWP", 0x32),
    121: ("DIRECT3D10", 0x879),
    122: ("DXGI", 0x87A),
    123: ("DXGI_DDI", 0x87B),
    124: ("DIRECT3D11", 0x87C),
    126: ("DIRECT3D12", 0x87E),
    152: ("WINCODEC_ERR", 0x898),
    153: ("D2D", 0x899),
}


def main(header, bitfield, check):
    text = open(header, encoding="latin-1").read()

    codes = re.findall(r"^#define\s+(\w+)\s+_HRESULT_TYPEDEF_\(\s*"
                       r"(0x[0-9A-Fa-f]+)L?\s*\)\s*$", text, re.M)
    named = re.findall(r"^#define\s+(FACILITY_\w+)\s+(\d+)\s*$", text, re.M)

    # facility number -> its names, as the header gives them
    names = collections.defaultdict(list)
    for name, number in named:
        names[int(number)].append(name)

    groups = collections.defaultdict(list)
    for name, value in codes:
        groups[(int(value, 16) >> 16) & 0x7FF].append((int(value, 16), name))

    # the HRESULT's own fields, and every facility as a value of its field
    facilities = {}
    passed_over = []
    for number in sorted(names):
        for name in sorted(names[number]):
            if number > 0x7FF:
                passed_over.append(name)
                continue
            facilities[name] = number
    for number, (name, whole) in sorted(SDK_FACILITIES.items()):
        note = "the Windows SDK's FACILITY_%s" % name
        if whole != number:
            note += " (0x%X: the X bit, with facility 0x%X)" % (whole, number)
        facilities["FACILITY_" + name] = {"number": number, "note": note}

    layout = {
        "group": "Layout",
        "note": "An HRESULT's fields: a severity, four flags, an 11-bit "
                "facility and a 16-bit code.",
        "rows": [
            {"name": "SEVERITY", "bit": 31, "note": "1 for a failure"},
            {"name": "RESERVED", "bit": 30,
             "note": "the R bit: reserved, but for a few NTSTATUS-mapped "
                     "codes"},
            {"name": "CUSTOMER", "bit": 29,
             "note": "the C bit: a code a customer defined"},
            {"name": "NTSTATUS", "bit": 28,
             "note": "the N bit: an NTSTATUS, mapped to an HRESULT"},
            {"name": "X", "bit": 27,
             "note": "the X bit: reserved, and set by the DirectX "
                     "facilities, whose numbers reach it"},
            {"name": "FACILITY", "bits": "16..26",
             "note": "who defined the code", "values": facilities},
            {"name": "CODE", "bits": "0..15",
             "note": "the code, within its facility"},
        ],
    }

    rows = [layout]
    for number in sorted(groups):
        listed = groups[number]
        if number in names:
            title = sorted(names[number], key=len)[0][len("FACILITY_"):]
            spelt = " and ".join(sorted(names[number]))
        else:
            title = SDK_FACILITIES[number][0]
            spelt = "FACILITY_" + title + ", as the Windows SDK names it"
        rows.append({
            "group": title,
            "note": "Facility %d, %s: %d code%s." % (
                number, spelt, len(listed), "" if len(listed) == 1 else "s"),
            "collapsed": True,
            "rows": [{"name": name, "value": "0x%X" % value}
                     for value, name in sorted(listed)],
        })

    sheet = {
        "format": "bitwisedom.bitfield",
        "version": 1,
        "title": "win32_hresults",
        "width": 32,
        "prefix": "HR_",
        "rows": rows,
        "segments": [
            {"name": "code", "bits": "0..15"},
            {"name": "facility", "bits": "16..26"},
            {"name": "flags", "bits": "27..31"},
        ],
    }

    with open(bitfield, "w", newline="\n") as out:
        out.write(bitfield_json.dumps(sheet))

    write_check(check, codes, facilities, len(codes))

    print("%d codes in %d facilities; %d facility value(s), %s passed over "
          "past the field" % (len(codes), len(groups), len(facilities),
                              ", ".join(passed_over) or "none"))


def spread(left, right, width=80):
    """left and right on one line of width columns, right flush with its
    end."""
    return left + " " * (width - len(left) - len(right)) + right


def continued(text):
    """A line of a macro's definition, its backslash in column 80."""
    return text + " " * (79 - len(text)) + "\\"


def same(ours, theirs):
    """One comparison, on one line where it fits and on two where not."""
    line = "SAME(%s, %s);" % (ours, theirs)
    if len(line) <= 80:
        return [line]
    return ["SAME(%s," % ours, "     %s);" % theirs]


def write_check(path, codes, facilities, count):
    lines = [
        "/" + "*" * 79,
        spread("* bitwisedom [tests]", "win32_hresults.c"),
        "*",
        "* The win32_hresults example against winerror.h: every one of its",
        "* %d codes, and every facility, compared with the platform's own at"
        % count,
        "* compile time. Built where the build targets Windows. Written by",
        "* tools/winerror_to_bitfield.py with the example; a code or a",
        "* facility the platform's header does not define is passed over.",
        "*",
        "* path:      /tests/djinterp/tools/bitwisedom/platform/win32_hresults.c",
        "* link(s):   TBA",
        spread("* author(s): Samuel 'teer' Neal-Blim", "created: 2026.10.11"),
        spread("*", "revised: 2026.10.11"),
        "*" * 79 + "/",
        "",
        "#include <stdint.h>              // uint32_t",
        "#include <windows.h>             // winerror.h",
        "#include \"win32_hresults.h\"      // the example, as bitwisedom-gen "
        "wrote it",
        "",
        "",
        "// SAME",
        "//   ours is theirs, compared as the 32 unsigned bits both are: "
        "winerror.h's",
        "//   codes are signed HRESULTs, the example's unsigned.",
        continued("#define SAME(_ours, _theirs)"),
        continued("    _Static_assert((uint32_t)(_ours) == "
                  "(uint32_t)(_theirs),"),
        "                   #_ours \" is \" #_theirs)",
        "",
        "",
        "// the facilities, as the facility field holds them: a DirectX",
        "// facility's number reaches the X bit, which the field does not",
    ]
    for name in facilities:
        lines += ["#if defined(%s)" % name]
        lines += same("HR_FACILITY_GET(HR_%s)" % name, "(%s) & 0x7FF" % name)
        lines += ["#endif"]
    lines += ["", "// the codes"]
    for name, value in codes:
        lines += ["#if defined(%s)" % name]
        lines += same("HR_" + name, name)
        lines += ["#endif"]
    with open(path, "w", newline="\n") as out:
        out.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit("usage: winerror_to_bitfield.py WINERROR.H BITFIELD.json "
                 "CHECK.c")
    main(*sys.argv[1:])
