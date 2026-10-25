# xml_tree.py -- an XML file's element tree, as Python's own parser reads it.
#
# Prints the tree as test_dmsvs --dump prints dmsvs's reading of the same
# file, so the two can be compared byte for byte: an element to a line, its
# depth, its local name, its attributes in name order, and, for one holding
# no element, its text. A backslash, a quote, a tab, a CR and an LF in a
# value are escaped.
#
#   python3 tests/djinterp/tools/dawk/ext/msvs/xml_tree.py <file>
import sys
import xml.etree.ElementTree as tree


def local(name):
    """A name without the namespace Python's parser puts before it."""
    return name.rsplit('}', 1)[-1]


def escaped(text):
    """A value as both printers write one."""
    out = []
    for char in text:
        out.append({'\\': '\\\\', '"': '\\"', '\t': '\\t', '\r': '\\r',
                    '\n': '\\n'}.get(char, char))
    return ''.join(out)


def show(element, depth, out):
    """An element's line, then its children's."""
    line = '%d %s' % (depth, local(element.tag))
    for name in sorted(element.attrib, key=lambda n: local(n).encode()):
        line += ' %s="%s"' % (local(name), escaped(element.attrib[name]))
    if len(element) == 0:
        line += ' text="%s"' % escaped(element.text or '')
    out.append(line)
    for child in element:
        show(child, depth + 1, out)


def main():
    out = []
    show(tree.parse(sys.argv[1]).getroot(), 0, out)
    sys.stdout.buffer.write(('\n'.join(out) + '\n').encode('utf-8'))


main()
