"""gen_c_math.common - what every header's text shares: the folder, the families, the banner, a template's instantiation, the C++ overloads, the no-fusing pragma and the parameters' alignment (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""
import os
import re
import textwrap
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))

# the folder every header is written to, from the root
CORE = "inc/djinterp/c/modules/math"

FAMILIES = [
    # suffix, type, <float.h> prefix, name in prose
    ("f",  "float",       "FLT",  "float"),
    ("d",  "double",      "DBL",  "double"),
    ("ld", "long double", "LDBL", "long double"),
]

DATE = "2026.10.04"


# the three sections every banner carries under its description (ruled
# 2026.11.17; chunk 62, S474)
SECTION_HEADS = ("HOW TO USE", "OPTIONS", "CONFIGURATION")


def banner_sections(texts):
    """the three sections' lines, before banner() prefixes each with "* ":
    a heading over its dashes, then the text indented two on its first
    line, so that the banner's lines run to 79."""
    assert len(texts) == len(SECTION_HEADS)
    out = []
    for head, text in zip(SECTION_HEADS, texts):
        out += ["", head, "-" * len(head)]
        out += textwrap.wrap(" ".join(text.split()), width=77,
                             initial_indent="  ", break_long_words=False,
                             break_on_hyphens=False)
    return out


def banner(filename, lines, created_on=DATE, revised_on=DATE,
           sections=None):
    """the standard header banner, its fixed geometry included; sections
    are the three sections' texts, by default banner_sections.SECTIONS's
    for the file, which must have them."""
    from .banner_sections import SECTIONS
    if sections is None:
        if filename not in SECTIONS:
            raise SystemExit("gen_c_math: %s has no banner sections "
                             "(banner_sections.py)" % filename)
        sections = SECTIONS[filename]
    lines = list(lines) + banner_sections(sections)
    path = "/" + CORE + "/" + filename
    title = "* djinterp [c][math]"
    head = title + " " * (80 - len(title) - len(filename)) + filename
    body = "\n".join(("*" + (" " + l if l else "")).rstrip() for l in lines)
    author = "* author(s): Samuel 'teer' Neal-Blim"
    created = "created: " + created_on
    revised = "revised: " + revised_on
    return "\n".join([
        "/" + "*" * 79,
        head,
        "*",
        body,
        "*",
        "*",
        "* path:      " + path,
        "* link(s):   TBA",
        author + " " * (80 - len(author) - len(created)) + created,
        "*" + " " * (80 - 1 - len(revised)) + revised,
        "*" * 79 + "/",
    ])


def instantiate(template, suffix, ctype, prefix, prose):
    """one family's text: the template with its placeholders filled."""
    out = template
    for key, value in (
        ("@T@", ctype),
        ("@S@", suffix),
        ("@NAME@", prose),
        ("@P@", prefix + "_MANT_DIG"),
        ("@PFX@", prefix),
        ("@EMIN@", prefix + "_MIN_EXP"),
        ("@EMAX@", prefix + "_MAX_EXP"),
        ("@MIN@", prefix + "_MIN"),
        ("@MAX@", prefix + "_MAX"),
        ("@EPS@", prefix + "_EPSILON"),
        ("@DIG@", prefix + "_DIG"),
        ("@PRINTF@", "L" if ctype == "long double" else ""),
        ("@SCANF@", {"float": "", "double": "l", "long double": "L"}[ctype]),
    ):
        out = out.replace(key, value)
    return out


# -----------------------------------------------------------------------------
# C++ overloads
# -----------------------------------------------------------------------------
# Compiled as C++, each generated header also names its kernels once for all
# three families - d_interval_contains(core, value) for d_interval_f_contains,
# _d_ and _ld_ - chosen by argument type: what lets one C++ template serve
# every floating-point type without a macro.

def overload(qual, ret, name, target, params):
    """one C++ overload forwarding to a family's kernel, in the house form."""
    width = max(len(t) for t, _ in params) if params else 0
    lines = [qual + " " + ret, name + "("]
    for i, (ptype, pname) in enumerate(params):
        sep = "," if i + 1 < len(params) else ""
        lines.append("    " + ptype.ljust(width) + " " + pname + sep)
    lines.append(")")
    lines.append("{")
    args = ", ".join(p for _, p in params)
    call = "    return " + target + "(" + args + ");"
    if len(call) > 80:
        call = "    return " + target + "(" + (",\n" + " " * (len("    return " + target + "("))).join(p for _, p in params) + ");"
    lines.append(call)
    lines.append("}")
    return "\n".join(lines)


def overload_section(title, specs):
    """the C++-only section: every spec, for every family."""
    out = ["", "", "#ifdef __cplusplus", "",
           "//" + "=" * 78,
           "// " + title,
           "//" + "=" * 78,
           "// C++ only: the kernels above under one name each, chosen by argument type,",
           "// so a C++ template serves float, double and long double alike.", "", ""]
    for suffix, ctype, prefix, prose in FAMILIES:
        for qual, ret, name, params in specs:
            r = ret.replace("@T@", ctype).replace("@S@", suffix)
            ps = [(t.replace("@T@", ctype).replace("@S@", suffix), n) for t, n in params]
            # the family's suffix goes after the module's prefix: d_vec_add
            # forwards to d_vec_d_add, d_math_next_up to d_math_d_next_up
            target = None
            for prefix_name in ("d_interval_", "d_math_", "d_vec_", "d_mat_", "d_quat_", "d_coord_", "d_isect_", "d_plane_", "d_decomp_", "d_cplx_", "d_cmat_", "d_internal_vec_"):
                if name.startswith(prefix_name):
                    target = prefix_name + suffix + "_" + name[len(prefix_name):]
            assert target is not None, name
            out.append(overload(qual, r, name, target, ps))
            out.append("")
    out.append("#endif  // __cplusplus")
    return "\n".join(out)


def auto_section(title, specs, exact_specs):
    """the C++-only section: each two-form kernel under one name for every
    family and level (_auto: the C library's at run time, the _c form in
    constant evaluation from C++20), then a template for any other element
    type, through double. specs: (ret, name, [(type, param, length)]);
    exact_specs, the one-form kernels: the template only."""
    out = ["", "", "#ifdef __cplusplus", "",
           "//" + "=" * 78,
           "// " + title,
           "//" + "=" * 78,
           "// C++ only. Each two-form kernel under one name, _auto, for every family",
           "// and level: the C library's form at run time at every level, and from",
           "// C++20 the _c form when std::is_constant_evaluated says so. Then, for",
           "// an element type no family holds, a template through double - the",
           "// subframework's generic path - for the _auto names and for the exact",
           "// one-form kernels alike.", "",
           "#if D_ENV_LANG_IS_CPP20_OR_HIGHER",
           "    #include <type_traits>  // std::is_constant_evaluated",
           "#endif", "", ""]

    def decl(ret, name, ps, constexpr_kw):
        lines = [constexpr_kw + " " + ret, name + "("]
        width = max(len(t) for t, _ in ps)
        for i, (t, n) in enumerate(ps):
            lines.append("    " + t.ljust(width) + " " + n + ("," if i + 1 < len(ps) else ""))
        lines.append(") D_NOEXCEPT")
        return lines

    for suffix, ctype, prefix, prose in FAMILIES:
        for ret, name, params in specs:
            r = ret.replace("@T@", ctype)
            ps = [(t.replace("@T@", ctype), n) for t, n, _ in params]
            target = name.replace("d_coord_", "d_coord_" + suffix + "_")
            args = ", ".join(n for _, n in ps)
            auto_name = name + "_auto"
            ret_kw = "return " if r != "void" else ""
            out.append("#if D_ENV_LANG_IS_CPP20_OR_HIGHER")
            out += decl(r, auto_name, ps, "constexpr")
            out.append("{")
            if r == "void":
                out += ["    if (std::is_constant_evaluated())", "    {",
                        "        " + target + "_c(" + args + ");", "    }",
                        "    else", "    {",
                        "        " + target + "(" + args + ");", "    }", "",
                        "    return;"]
            else:
                out += ["    if (std::is_constant_evaluated())", "    {",
                        "        return " + target + "_c(" + args + ");", "    }", "",
                        "    return " + target + "(" + args + ");"]
            out.append("}")
            out.append("#else")
            out += decl(r, auto_name, ps, "inline")
            out.append("{")
            out.append("    " + ret_kw + target + "(" + args + ");")
            if r == "void":
                out += ["", "    return;"]
            out.append("}")
            out.append("#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER")
            out.append("")

    def through_double(ret, name, params, constexpr_kw):
        r = ret.replace("@T@", "Type")
        ps = [(t.replace("@T@", "Type"), n) for t, n, _ in params]
        lines = ["template<typename Type>"] + decl(r, name, ps, constexpr_kw)
        lines.append("{")
        call_args = []
        writes = []
        for t, n, length in params:
            local = "d" + n
            if length is None:
                lines.append("    const double " + local + " = static_cast<double>(" + n + ");")
            elif t.startswith("const"):
                elems = ", ".join("static_cast<double>(" + n + "[" + str(k) + "])" for k in range(length))
                lines.append("    const double " + local + "[" + str(length) + "] = { " + elems + " };")
            else:
                zeros = ", ".join("0.0" for _ in range(length))
                lines.append("    double       " + local + "[" + str(length) + "] = { " + zeros + " };")
                writes.append((n, local, length))
            call_args.append(local)
        call = name + "(" + ", ".join(call_args) + ")"
        if r == "void":
            lines.append("")
            lines.append("    " + call + ";")
            for n, local, length in writes:
                for k in range(length):
                    lines.append("    " + n + "[" + str(k) + "] = static_cast<Type>(" + local + "[" + str(k) + "]);")
            lines += ["", "    return;"]
        else:
            lines.append("")
            lines.append("    return static_cast<Type>(" + call + ");")
        lines.append("}")
        return lines

    out.append("// for any other element type, through double")
    for ret, name, params in specs:
        out += through_double(ret, name + "_auto", params, "D_CONSTEXPR_CPP20")
        out.append("")
    for ret, name, params in exact_specs:
        out += through_double(ret, name, params, "D_CONSTEXPR_CPP14")
        out.append("")
    out.append("#endif  // __cplusplus")
    return "\n".join(out)


FLOAT_ORDER_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_next_up", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_next_down", [("@T@", "_x")]),
    ("D_MATH_FN", "dj_math_umax", "d_math_steps", [("@T@", "_from"), ("@T@", "_to")]),
    ("D_MATH_FN", "@T@", "d_math_advance", [("@T@", "_from"), ("dj_math_umax", "_steps")]),
]

IV = "struct dj_interval_@S@"
INTERVAL_FLOAT_OVERLOADS = [
    ("D_MATH_FN", IV, "d_interval_make", [("@T@", "_lower"), ("@T@", "_upper"), ("unsigned", "_bounds"), ("@T@", "_step")]),
    ("D_MATH_FN", "bool", "d_interval_is_valid", [(IV, "_interval")]),
    ("D_MATH_FN", "bool", "d_interval_is_discrete", [(IV, "_interval")]),
    ("D_MATH_FN", "bool", "d_interval_is_empty", [(IV, "_interval")]),
    ("D_MATH_FN", "@T@", "d_interval_inclusive_lower", [(IV, "_interval")]),
    ("D_MATH_FN", "@T@", "d_interval_inclusive_upper", [(IV, "_interval")]),
    ("D_MATH_FN", "@T@", "d_interval_first", [(IV, "_interval")]),
    ("D_MATH_FN", "@T@", "d_interval_last", [(IV, "_interval")]),
    ("D_MATH_FN", "dj_math_umax", "d_interval_count", [(IV, "_interval")]),
    ("D_MATH_FN", "@T@", "d_interval_at", [(IV, "_interval"), ("dj_math_umax", "_index")]),
    ("D_MATH_FN", "bool", "d_interval_contains_in_range", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "bool", "d_interval_contains", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "@T@", "d_interval_clamp", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "@T@", "d_interval_clamp_nearest", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "dj_math_umax", "d_interval_index_of", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "bool", "d_interval_overlaps", [(IV, "_left"), (IV, "_right")]),
    ("D_MATH_FN", "bool", "d_interval_overlaps_continuous", [(IV, "_left"), (IV, "_right")]),
    ("D_MATH_FN", "bool", "d_interval_intersects", [(IV, "_left"), (IV, "_right")]),
    ("D_MATH_FN", "struct dj_math_ratio", "d_interval_normalize_terms", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "struct dj_math_ratio", "d_interval_normalize_effective_terms", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN", "struct dj_math_ratio", "d_interval_normalize_discrete_terms", [(IV, "_interval"), ("@T@", "_value")]),
    ("D_MATH_FN_RT", "size_t", "d_interval_format", [("char*", "_buffer"), ("size_t", "_capacity"), (IV, "_interval")]),
]


NO_CONTRACT_OPEN = """
// no multiply and add fused by the compiler in this header, where it can be
// told; the including file's state is restored at the end (math_common.h,
// section 5.3)
#if D_MATH_PRAGMA_NO_CONTRACT
    #pragma float_control(push)
    #pragma STDC FP_CONTRACT OFF
#endif  // D_MATH_PRAGMA_NO_CONTRACT
"""

NO_CONTRACT_CLOSE = """
// the including file's own fusing again
#if D_MATH_PRAGMA_NO_CONTRACT
    #pragma float_control(pop)
#endif  // D_MATH_PRAGMA_NO_CONTRACT

"""


def align_params(text):
    """each parameter list written one parameter a line - a function's name
    and its "(" alone on a line, a ")" closing it at the margin - with its
    names in one column, one space past its longest type, as the owner
    aligned cmat.h's on 2026.11.21 (S435): the templates align them for the
    placeholders, so each family's types and suffixes shift them. A list
    holding a comment, a function pointer or a default is left as it is."""
    import re
    lines = text.split("\n")
    head = re.compile(r"^[A-Za-z_]\w*\($")
    param = re.compile(r"^    (\S.*?)\s+(\**[A-Za-z_]\w*(?:\[[^\]]*\])*)(,?)$")
    out = []
    i = 0
    while i < len(lines):
        out.append(lines[i])
        if not head.match(lines[i]):
            i += 1
            continue
        j = i + 1
        while (j < len(lines)) and lines[j].startswith("    "):
            j += 1
        block = lines[i + 1:j]
        closing = (j < len(lines)) and lines[j].startswith(")")
        parts = [param.match(l) for l in block]
        if ((not block) or (not closing) or (None in parts) or
                any(("//" in l) or ("/*" in l) or ("(" in l) or ("=" in l)
                    for l in block)):
            i += 1
            continue
        width = max(len(m.group(1)) for m in parts)
        for m in parts:
            out.append("    " + m.group(1).ljust(width) + " " + m.group(2) +
                       m.group(3))
        i = j
    return "\n".join(out)


def no_contract(text):
    """clang's no-fusing pragma, saved and restored, around a header's body:
    after its includes, before its last #endif."""
    first = text.index("\n//====")
    last = text.rindex("\n#endif  // DJINTERP_C_MATH_")
    body = text[first:last].rstrip("\n") + "\n"
    return (text[:first].rstrip("\n") + "\n" + NO_CONTRACT_OPEN + "\n" +
            body + NO_CONTRACT_CLOSE + text[last:].lstrip("\n"))
