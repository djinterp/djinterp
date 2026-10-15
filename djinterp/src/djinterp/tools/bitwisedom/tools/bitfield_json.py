#*******************************************************************************
# bitwisedom [tools]                                            bitfield_json.py
#
# JSON laid out as bitwis(e)dom lays it out (src/json.cpp): four spaces to
# an indent, an array of scalars on one line, and any other array or object
# on one line where the line has room for it, with its comma, in 100
# columns. A bitfield a script writes this way is the bitfield
# bitwisedom-gen --bitfield writes back, byte for byte, and a diff of two is
# a diff of what changed.
#
# path:      /src/djinterp/tools/bitwisedom/tools/bitfield_json.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
#                                                            revised: 2026.10.11
#*******************************************************************************
import json

# the columns a line fills, past which an array or an object is laid out an
# item or a member to a line
LINE = 100


def scalar(value):
    return not isinstance(value, (list, dict))


def flat_array(value):
    return isinstance(value, list) and all(scalar(item) for item in value)


def quoted(text):
    # json.dumps escapes as src/json.cpp does, for the text these scripts
    # write: quotes, backslashes and control characters
    return json.dumps(text, ensure_ascii=False)


def one_line(value):
    """Any value, written on one line."""
    if value is None:
        return "null"
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, int):
        return str(value)
    if isinstance(value, str):
        return quoted(value)
    if isinstance(value, list):
        return "[" + ", ".join(one_line(item) for item in value) + "]"
    if not value:
        return "{}"
    return "{ " + ", ".join(quoted(key) + ": " + one_line(member)
                            for key, member in value.items()) + " }"


def write_value(value, indent, column, out):
    pad = " " * (indent * 4)
    inner = " " * ((indent + 1) * 4)

    # a scalar, or an array of scalars, on one line however long
    if scalar(value) or flat_array(value):
        out.append(one_line(value))
        return

    # anything else on one line where the line has room, with its comma
    line = one_line(value)
    if column + len(line) + 1 <= LINE:
        out.append(line)
        return

    if isinstance(value, list):
        out.append("[\n")
        for i, item in enumerate(value):
            out.append(inner)
            write_value(item, indent + 1, len(inner), out)
            out.append(",\n" if i + 1 < len(value) else "\n")
        out.append(pad + "]")
        return

    out.append("{\n")
    members = list(value.items())
    for i, (key, member) in enumerate(members):
        start = quoted(key) + ": "
        out.append(inner + start)
        write_value(member, indent + 1, len(inner) + len(start), out)
        out.append(",\n" if i + 1 < len(members) else "\n")
    out.append(pad + "}")


def dumps(value):
    """value, as bitwis(e)dom writes it, with the newline that ends it."""
    out = []
    write_value(value, 0, 0, out)
    return "".join(out) + "\n"
