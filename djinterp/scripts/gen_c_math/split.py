"""gen_c_math.split - the largest headers in parts (S436) (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .banner_sections import PART
from .common import (NO_CONTRACT_CLOSE, NO_CONTRACT_OPEN, banner, re,
                     textwrap)

# the counts of parts, as a banner's prose spells them
NUMBERS = {2: "two", 3: "three", 4: "four", 5: "five", 6: "six",
           7: "seven", 8: "eight", 9: "nine", 10: "ten", 11: "eleven",
           12: "twelve"}


# -----------------------------------------------------------------------------
# The largest headers in parts (S436)
# -----------------------------------------------------------------------------
# A header past about 4,000 lines is written as parts of at most that, each a
# header of its own that stands alone, and the header of its name includes
# them all, in order: an includer sees what it saw, in the order it saw it.
# The text is the one the generator always wrote, cut at section headings:
# a part holds the original's includes, the parts whose names it uses, and
# its slice, inside the no-fusing pragma as the whole header was; gcc's
# diagnostic set aside for a section is restored in the part that sets it
# aside and set aside again in the part it continues into. Each part's name
# is its header's, then what it holds: a family's suffix, kernels shared by
# the families, the C++ names.

SPLITS = {
    "decomp.h": [
        ("decomp_f.h", None,
         "the float family's LU, QR, SVD, symmetric eigensolver and Cholesky"),
        ("decomp_f_eigen_c.h", r"^// 1\.3    ",
         "the float family's general eigensolver, its _c forms"),
        ("decomp_f_eigen.h", r"^// d_internal_decomp_f_rotation_of$",
         "the float family's general eigensolver, its plain forms"),
        ("decomp_d.h", r"^// 2\.  THE DOUBLE",
         "the double family's LU, QR, SVD, symmetric eigensolver and Cholesky"),
        ("decomp_d_eigen_c.h", r"^// 2\.3    ",
         "the double family's general eigensolver, its _c forms"),
        ("decomp_d_eigen.h", r"^// d_internal_decomp_d_rotation_of$",
         "the double family's general eigensolver, its plain forms"),
        ("decomp_ld.h", r"^// 3\.  THE LONG DOUBLE",
         "the long double family's LU, QR, SVD, symmetric eigensolver and "
         "Cholesky"),
        ("decomp_ld_eigen_c.h", r"^// 3\.3    ",
         "the long double family's general eigensolver, its _c forms"),
        ("decomp_ld_eigen.h", r"^// d_internal_decomp_ld_rotation_of$",
         "the long double family's general eigensolver, its plain forms"),
        ("decomp_cpp.h", r"^// 4\.  ONE NAME",
         "one name for every family (C++)"),
    ],
    "cmat.h": [
        ("cmat_f.h", None,
         "the float family's products, LU, determinant, inverse, solution and "
         "Hermitian eigensystems"),
        ("cmat_f_eigen.h", r"^// 1\.3    ",
         "the float family's general eigensolver"),
        ("cmat_d.h", r"^// 2\.  THE DOUBLE",
         "the double family's products, LU, determinant, inverse, solution "
         "and Hermitian eigensystems"),
        ("cmat_d_eigen.h", r"^// 2\.3    ",
         "the double family's general eigensolver"),
        ("cmat_ld.h", r"^// 3\.  THE LONG DOUBLE",
         "the long double family's products, LU, determinant, inverse, "
         "solution and Hermitian eigensystems"),
        ("cmat_ld_eigen.h", r"^// 3\.3    ",
         "the long double family's general eigensolver"),
        ("cmat_cpp.h", r"^// 4\.  ONE NAME",
         "one name for every family (C++)"),
    ],
    "cplx.h": [
        ("cplx_arith.h", None,
         "the complex types and their arithmetic, every family"),
        ("cplx_kernels_pure.h", r"^// 3\.  THE KERNELS",
         "the functions' kernels in pure arithmetic, in long double"),
        ("cplx_kernels_lib.h", r"^// 3\.2    ",
         "the functions' kernels on the C library, in each family's type"),
        ("cplx_functions.h", r"^// 4\.  THE FUNCTIONS",
         "C99's complex functions, every family, and C99's _Complex"),
        ("cplx_cpp.h", r"^// 6\.  ONE NAME",
         "one name for every family (C++)"),
    ],
    "quat.h": [
        ("quat_kernels.h", None,
         "the kernels every family shares"),
        ("quat_f.h", r"^// 2\.  THE FLOAT", "the float family"),
        ("quat_d.h", r"^// 3\.  THE DOUBLE", "the double family"),
        ("quat_ld.h", r"^// 4\.  THE LONG DOUBLE", "the long double family"),
        ("quat_cpp.h", r"^// 5\.  ONE NAME",
         "one name for every family (C++)"),
    ],
    "vec.h": [
        ("vec_f.h", None,
         "the compensated sums' arithmetic and the float family"),
        ("vec_d.h", r"^// 3\.  THE DOUBLE", "the double family"),
        ("vec_ld.h", r"^// 4\.  THE LONG DOUBLE", "the long double family"),
        ("vec_cpp.h", r"^// 5\.  ONE NAME",
         "one name for every family (C++)"),
    ],
    "coord.h": [
        ("coord_f.h", None, "the float family"),
        ("coord_d.h", r"^// 2\.  THE DOUBLE", "the double family"),
        ("coord_ld.h", r"^// 3\.  THE LONG DOUBLE", "the long double family"),
        ("coord_cpp.h", r"^// 4\.  ONE NAME",
         "one name for every family, and for every level and any type (C++)"),
    ],
    "mat.h": [
        ("mat_f.h", None, "the float family"),
        ("mat_d.h", r"^// 2\.  THE DOUBLE", "the double family"),
        ("mat_ld.h", r"^// 3\.  THE LONG DOUBLE", "the long double family"),
        ("mat_cpp.h", r"^// 4\.  ONE NAME",
         "one name for every family (C++)"),
    ],
    "exponential.h": [
        ("exponential_kernels.h", None,
         "the long double kernels every family shares"),
        ("exponential_f.h", r"^// 2\.  THE FLOAT", "the float family"),
        ("exponential_d.h", r"^// 3\.  THE DOUBLE",
         "the double family, its correctly rounded exp and log among it"),
        ("exponential_ld.h", r"^// 4\.  THE LONG DOUBLE",
         "the long double family"),
        ("exponential_cpp.h", r"^// 5\.  ONE NAME",
         "one name for every family (C++)"),
    ],
}

PART_CREATED = "2026.11.21"
PART_DATE = "2026.11.24"
SPLIT_DATE = "2026.11.24"


def strip_c_comments(text):
    """the text without its comments and its string and character
    literals, its lines kept, for finding the names a part uses."""
    out = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("\n" * text.count("\n", i, j))
            i = j
        elif c in "\"'":
            j = i + 1
            while (j < n) and (text[j] != c):
                j += 2 if text[j] == "\\" else 1
            out.append(" ")
            i = j + 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


def names_defined(text):
    """the names a slice of generated C defines at its margin: functions,
    whose names begin a line; structs; macros; tables and constants."""
    code = strip_c_comments(text)
    found = set(re.findall(r"(?m)^([A-Za-z_]\w*)\(", code))
    found |= set(re.findall(r"(?m)^(?:typedef )?struct (\w+)\s*\n\{", code))
    found |= set(re.findall(r"(?m)^\s*#\s*define\s+(\w+)", code))
    found |= set(re.findall(
        r"(?m)^(?:static )?(?:D_\w+ )*(?:const )?(?:struct )?\w[\w ]*?[\s*]"
        r"(\w+)\s*(?:\[[^\]]*\])+\s*=", code))
    return found


def names_used(text):
    return set(re.findall(r"[A-Za-z_]\w*", strip_c_comments(text)))


def guard_of(name):
    return "DJINTERP_C_MATH_" + re.sub(r"\W", "_", name[:-2]).upper() + "_H"


def split_header(name, text, spec):
    """{file: text}: the header of <name>, which includes its parts, and
    each part, from the generated <text> and the parts' <spec>."""
    open_at = text.index(NO_CONTRACT_OPEN)
    close_at = text.rindex(NO_CONTRACT_CLOSE)
    head = text[:open_at]
    body = text[open_at + len(NO_CONTRACT_OPEN):close_at]
    guard = guard_of(name)
    define = "#define %s 1\n" % guard
    includes = head[head.index(define) + len(define):].strip("\n") + "\n"
    lines = body.split("\n")
    # where each part begins: its heading, with the rule above a section's
    starts = [0]
    for part, pattern, _ in spec[1:]:
        rx = re.compile(pattern)
        at = next(i for i in range(starts[-1] + 1, len(lines))
                  if rx.search(lines[i]))
        if (at > 0) and lines[at - 1].startswith("//===="):
            at -= 1
        # the blank lines before it, and a C++ section's #ifdef
        while (at > 0) and ((lines[at - 1].strip() == "") or
                            (lines[at - 1] == "#ifdef __cplusplus")):
            at -= 1
        starts.append(at)
    starts.append(len(lines))
    slices = ["\n".join(lines[a:b]) for a, b in zip(starts, starts[1:])]
    # gcc's diagnostic block, as the header sets a section's aside
    push = re.search(r"(?m)^#if \( defined\(__GNUC__\) &&[^\n]*\n"
                     r"[^\n]*\n    #pragma GCC diagnostic push\n"
                     r"(?:    #pragma GCC diagnostic ignored [^\n]*\n)+"
                     r"#endif\n", body)
    pop = re.search(r"(?m)^#if \( defined\(__GNUC__\) &&[^\n]*\n"
                    r"[^\n]*\n    #pragma GCC diagnostic pop\n#endif\n", body)
    files = {}
    defined = []
    depth_note = "the preprocessor's #if and #endif balance in each part"
    for k, ((part, _, what), piece) in enumerate(zip(spec, slices)):
        code = strip_c_comments(piece)
        ifs = len(re.findall(r"(?m)^\s*#\s*if", code))
        ends = len(re.findall(r"(?m)^\s*#\s*endif", code))
        assert ifs == ends, "%s: %s" % (part, depth_note)
        level = 0
        lead = ""
        for m in re.finditer(r"#pragma GCC diagnostic (push|pop)", code):
            if m.group(1) == "push":
                level += 1
            elif level == 0:
                lead = "\n// gcc's diagnostic as the part before set it\n" + \
                       push.group(0)
            else:
                level -= 1
        tail = ("\n// gcc's diagnostic restored for the part after\n" +
                pop.group(0)) if level else ""
        used = names_used(piece)
        deps = [spec[j][0] for j in range(k) if defined[j] & used]
        defined.append(names_defined(piece))
        lines_ = [
            "One part of %s (S436): %s." % (name, what),
            "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
            "families: edit the generator, not this file.",
            "",
            "  It stands alone, and %s includes it with the other parts, in"
            % name,
            "order: what it holds is described there.",
        ]
        guard_p = guard_of(part)
        own = "".join('#include "./%s"\n' % d for d in deps)
        if own:
            own = "// the parts before this one whose names it uses\n" + own
        files[part] = (
            banner(part, textwrap.wrap(" ".join(l for l in lines_[:1]), 72)
                   + lines_[1:], PART_CREATED, PART_DATE,
                   tuple(t % {"header": name, "count": NUMBERS[len(spec)]}
                         for t in PART)) + "\n\n" +
            "#ifndef %s\n#define %s 1\n\n" % (guard_p, guard_p) +
            includes + own + NO_CONTRACT_OPEN + lead + "\n\n" +
            piece.strip("\n") + "\n" + tail + NO_CONTRACT_CLOSE +
            "#endif  // %s\n" % guard_p)
    # the header of the name: its banner and contents, then the parts
    stop = head.index("\n#ifndef " + guard)
    top = head[:stop].rstrip("\n")
    top = re.sub(r"revised: \d{4}\.\d\d\.\d\d", "revised: " + SPLIT_DATE,
                 top, count=1)
    note = ("*   Its text is in %d parts, each a header that stands alone, "
            "included\n* below in order (S436)." % len(spec))
    # before the sections, which close the description (S474)
    assert top.count("\n*\n* HOW TO USE\n") == 1, name
    top = top.replace("\n*\n* HOW TO USE\n",
                      "\n*\n" + note + "\n*\n* HOW TO USE\n", 1)
    # the header's own sections count its parts as the split does
    prose = " ".join(" ".join(l[2:] for l in top.split("\n")
                              if l.startswith("* ")).split())
    assert ("its %s parts" % NUMBERS[len(spec)]) in prose, name
    width = max(len(p) for p, _, _ in spec) + 2
    listing = []
    for p, _, w in spec:
        rows = textwrap.wrap(w, 79 - width)
        listing.append(p.ljust(width) + rows[0])
        listing += [" " * width + r for r in rows[1:]]
    files[name] = (
        top + "\n\n/*\nTHE PARTS\n=========\n" + "\n".join(listing) +
        "\n*/\n\n#ifndef %s\n%s\n" % (guard, define) +
        "".join('#include "./%s"\n' % p for p, _, _ in spec) +
        "\n#endif  // %s\n" % guard)
    return files
