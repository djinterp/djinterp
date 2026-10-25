#!/usr/bin/env python3
# msvs_tree_check.py -- proves a tree msvs_generate.awk made sound, reading
# it with Python's own XML parser, apart from the C that made it.
#
#   python3 -I msvs_tree_check.py <root> [<cmake-build-dir> <cmake-root>]
#
# For every project under <root>/.config/.msvs: each item names a file that
# is there; its djinterp.props import names <root>/.config/.msvs/
# djinterp.props; each reference names a project that is there, by its own
# ProjectGuid; its filters file lists the same items, each in a filter it
# defines, each filter's parents defined too.  For every solution: each
# project it lists is there, by its own ProjectGuid, and each id is used
# once.  With a CMake build directory -- Visual Studio's files CMake made
# from the same tree, its paths under <cmake-root> -- each test's compiled
# files are compared with those CMake gave the target of the same name.
#
# path:      /tests/djinterp/tools/dawk/ext/msvs/msvs_tree_check.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
#                                                            revised: 2026.10.16

import os
import sys
import xml.etree.ElementTree as tree

NS = "{http://schemas.microsoft.com/developer/msbuild/2003}"
problems = []


def say(path, why):
    problems.append("%s: %s" % (path, why))


def local(element):
    return element.tag.replace(NS, "")


def resolve(directory, relative):
    return os.path.normpath(os.path.join(directory,
                                         relative.replace("\\", os.sep)))


def guid_of(project):
    for element in tree.parse(project).getroot().iter(NS + "ProjectGuid"):
        return (element.text or "").strip("{}").lower()
    return ""


def items_of(root_element):
    items = {}
    for group in root_element.findall(NS + "ItemGroup"):
        for item in group:
            if local(item) in ("ClCompile", "ClInclude", "None"):
                items[(local(item), item.get("Include"))] = item
    return items


def check_project(path, props):
    directory = os.path.dirname(path)
    root_element = tree.parse(path).getroot()
    imports = [e.get("Project") for e in root_element.iter(NS + "Import")
               if (e.get("Project") or "").endswith("djinterp.props")]
    if not imports:
        say(path, "no djinterp.props import")
    for project in imports:
        if resolve(directory, project) != props:
            say(path, "djinterp.props import names " + project)
    items = items_of(root_element)
    for (kind, include) in items:
        if not os.path.isfile(resolve(directory, include)):
            say(path, "%s %s is not there" % (kind, include))
    for reference in root_element.iter(NS + "ProjectReference"):
        target = resolve(directory, reference.get("Include"))
        named = reference.find(NS + "Project")
        if not os.path.isfile(target):
            say(path, "reference " + reference.get("Include") +
                " is not there")
        elif named is None or (named.text or "").strip("{}").lower() \
                != guid_of(target):
            say(path, "reference to " + reference.get("Include") +
                " by another project's id")
    filters = path + ".filters"
    if not os.path.isfile(filters):
        say(path, "no filters file")
        return items
    filter_root = tree.parse(filters).getroot()
    defined = set()
    for group in filter_root.findall(NS + "ItemGroup"):
        for item in group.findall(NS + "Filter"):
            defined.add(item.get("Include"))
            if item.find(NS + "UniqueIdentifier") is None:
                say(filters, "filter " + item.get("Include") +
                    " with no identifier")
    for name in defined:
        parts = name.split("\\")
        for end in range(1, len(parts)):
            if "\\".join(parts[:end]) not in defined:
                say(filters, "filter %s, but not %s" %
                    (name, "\\".join(parts[:end])))
    shown = items_of(filter_root)
    if set(shown) != set(items):
        say(filters, "items other than the project's: %d against %d" %
            (len(shown), len(items)))
    for key, item in shown.items():
        held = item.find(NS + "Filter")
        if held is None or held.text not in defined:
            say(filters, "%s %s in no filter defined" % key)
    return items


def check_solution(path, ids):
    directory = os.path.dirname(path)
    seen = set()
    for project in tree.parse(path).getroot().iter("Project"):
        target = resolve(directory, project.get("Path"))
        given = (project.get("Id") or "").lower()
        if not os.path.isfile(target):
            say(path, "project " + project.get("Path") + " is not there")
            continue
        if given and given != guid_of(target):
            say(path, "project " + project.get("Path") +
                " by another project's id")
        if target in seen:
            say(path, "project " + project.get("Path") + " twice")
        seen.add(target)
    return len(seen)


def check_sln(path):
    directory = os.path.dirname(path)
    seen = set()
    folders = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}"
    with open(path, encoding="utf-8-sig") as handle:
        for line in handle:
            line = line.strip()
            if not line.startswith("Project("):
                continue
            parts = line.split('"')
            kind, project, ident = parts[1], parts[5], parts[7]
            if ident in seen:
                say(path, "id " + ident + " twice")
            seen.add(ident)
            if kind.upper() == folders:
                continue
            target = resolve(directory, project)
            if not os.path.isfile(target):
                say(path, "project " + project + " is not there")
            elif ident.strip("{}").lower() != guid_of(target):
                say(path, "project " + project + " by another project's id")
    return len(seen)


def cmake_sources(build, cmake_root):
    by_target = {}
    for directory, _, names in os.walk(build):
        for name in names:
            if not name.endswith(".vcxproj"):
                continue
            target = name[:-len(".vcxproj")]
            path = os.path.join(directory, name)
            files = set()
            for item in tree.parse(path).getroot().iter(NS + "ClCompile"):
                include = item.get("Include")
                if include is None:
                    continue
                include = include.replace("\\", "/")
                prefix = cmake_root.replace("\\", "/").rstrip("/") + "/"
                if include.lower().startswith(prefix.lower()):
                    files.add(include[len(prefix):])
            by_target[target] = files
    return by_target


def main():
    if len(sys.argv) not in (2, 4):
        print(__doc__ or "usage: msvs_tree_check.py <root> [<build> <root>]")
        return 2
    root = os.path.abspath(sys.argv[1])
    msvs = os.path.join(root, ".config", ".msvs")
    props = os.path.normpath(os.path.join(msvs, "djinterp.props"))
    projects = solutions = items = 0
    compiled = {}
    for directory, _, names in os.walk(msvs):
        for name in sorted(names):
            path = os.path.join(directory, name)
            if name.endswith(".vcxproj"):
                projects += 1
                found = check_project(path, props)
                items += len(found)
                compiled[name[:-len(".vcxproj")]] = set(
                    os.path.relpath(resolve(directory, include),
                                    root).replace(os.sep, "/")
                    for (kind, include) in found if kind == "ClCompile")
            elif name.endswith(".slnx"):
                solutions += 1
                check_solution(path, None)
            elif name.endswith(".sln"):
                solutions += 1
                check_sln(path)
    compared = differing = 0
    if len(sys.argv) == 4:
        theirs = cmake_sources(sys.argv[2], sys.argv[3])
        for target, files in sorted(compiled.items()):
            if target not in theirs:
                continue
            compared += 1
            if files != theirs[target]:
                differing += 1
                print("  differs from CMake: %s: only here %s, only CMake's "
                      "%s" % (target, sorted(files - theirs[target])[:3],
                              sorted(theirs[target] - files)[:3]))
    for problem in problems:
        print("  FAIL  " + problem)
    print("%d projects, %d items, %d solutions; %d compared with CMake, %d "
          "differing: %d problems" % (projects, items, solutions, compared,
                                       differing, len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
