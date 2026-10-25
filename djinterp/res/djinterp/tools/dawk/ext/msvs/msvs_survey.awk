# msvs_survey.awk -- what Visual Studio's files hold, and what in them no
# longer names anything.
#
#   dawk -x msvs -f msvs_survey.awk [-v quiet=1] <file or directory>...
#
# Each solution, project, filters file, user file and property sheet named,
# or found beneath a directory named, is read and described in a line: a
# project's name, its toolsets, its configurations, its items by kind, the
# property sheets it imports; a solution's projects, folders and
# configurations. Beneath each, what is stale:
#     an import, a reference or an item naming a file that is not there,
#       read from the project's own directory, as MSBuild reads it -- an
#       import or an item naming $(...) is left alone, as only MSBuild
#       knows what it expands to
#     a reference by an id other than the project's own ProjectGuid
#     a filters file listing an item its project does not, or the reverse,
#       or showing one in a filter it does not define
#     a solution listing a project that is not there, or by another id
#     a file that does not read, and why
#
#   quiet   1 to print only what is stale.
#
# Exit status: 0 when nothing is stale, 1 when something is, 2 when a file
# could not be read.

BEGIN {
    status = 0
    for (i = 1; i < ARGC; i++)
        if (ARGV[i] != "")
            visit(ARGV[i])
    printf "%d files: %d solutions, %d projects, %d filters, %d user, " \
           "%d props; %d stale\n", files, count["solution"],
           count["project"], count["filters"], count["user"],
           count["props"], stale
    exit status
}


# visit(path): a file, or every Visual Studio file beneath a directory.
function visit(path,    found, m, names, i) {
    if (msvs_exists(path) == 2) {
        # .config and .msvs are where these files live, so hidden names too
        found = msvs_glob(path "/*", "all") "\n" msvs_glob(path "/.*", "all")
        m = split(found, names, "\n")
        for (i = 1; i <= m; i++)
            if (names[i] != "")
                visit(names[i])
        return
    }
    if (msvs_kind(path) == "other")
        return
    files++
    count[msvs_kind(path)]++
    pending = ""
    if (msvs_kind(path) == "solution")
        survey_solution(path)
    else
        survey_msbuild(path)

    # what is stale follows the file's own line, or its name alone
    if (quiet && pending != "")
        print path
    printf "%s", pending
}

# directory(path): the directory a file is in, "." for none.
function directory(path) {
    return (path ~ /[\/\\]/) ? substr(path, 1, match(path, /[\/\\][^\/\\]*$/) - 1) : "."
}

# joined(directory, relative): a path read from a directory.
function joined(dir, relative) {
    gsub(/\\/, "/", relative)
    return (relative ~ /^\// || relative ~ /^[A-Za-z]:/) ? relative \
                                                        : dir "/" relative
}

# problem(path, what): one stale thing, said beneath its file once the
# file's own line is.
function problem(path, what) {
    # a thing said once for the file is not said again
    if (index(pending, "    " what "\n") > 0)
        return
    pending = pending "    " what "\n"
    stale++
    if (status == 0)
        status = 1
}

# unreadable(path, why): a file that does not read, said with its line.
function unreadable(path, why) {
    if (!quiet)
        printf "%s: %s, which does not read\n", path, msvs_kind(path)
    problem(path, why)
    status = 2
}

# items(n, what): a count of items, said as one or many.
function items(n, what) {
    return n " " ((n == 1) ? "item" : "items") " " what
}

# guid_of(path): a project's own ProjectGuid, lower case, braces gone; ""
# when it cannot be read.
function guid_of(path,    p, e, guid) {
    p = vcxproj_open(path)
    if (p < 0)
        return ""
    guid = ""
    e = vcxproj_find(p, "ProjectGuid")
    if (e)
        guid = tolower(vcxproj_text(e))
    vcxproj_close(p)
    gsub(/[{}]/, "", guid)
    return guid
}

# survey_msbuild(path): a project, filters file, user file or property
# sheet.
function survey_msbuild(path,    p, dir, e, kind, kinds, names, k, line, \
                        toolsets, configs, imports, target, include, n, \
                        held, t, missing) {
    p = vcxproj_open(path)
    if (p < 0) {
        unreadable(path, vcxproj_error())
        return
    }
    dir = directory(path)
    kind = vcxproj_kind(p)
    delete kinds
    toolsets = configs = imports = ""
    missing = 0

    for (e = vcxproj_find(p, ""); e; e = vcxproj_find(p, "", e)) {
        n = vcxproj_name(e)
        if (n == "PlatformToolset" && index(" " toolsets " ",
                                            " " vcxproj_text(e) " ") == 0)
            toolsets = toolsets (toolsets != "" ? " " : "") vcxproj_text(e)
        else if (n == "ProjectConfiguration")
            configs = configs (configs != "" ? " " : "") \
                      vcxproj_attr(e, "Include")
        else if (n == "Import") {
            target = vcxproj_attr(e, "Project")
            if (target ~ /\.props$/ && target !~ /\$\(/) {
                if (index(" " imports " ", " " target " ") == 0)
                    imports = imports (imports != "" ? " " : "") target
                if (!msvs_exists(joined(dir, target)))
                    problem(path, "imports " target ", which is not there")
            }
        }
        else if (vcxproj_name(vcxproj_parent(e)) == "ItemGroup" &&
                 vcxproj_has(e, "Include")) {
            include = vcxproj_attr(e, "Include")
            kinds[n]++
            if (n == "ProjectReference")
                survey_reference(path, dir, e, include)
            else if (n != "Filter" && n != "ProjectConfiguration" &&
                     include != "" && include !~ /[$%@*?]/ &&
                     !msvs_exists(joined(dir, include)))
                missing++
            else if (include == "" && n != "Filter")
                missing++
        }
    }

    if (missing > 0)
        problem(path, items(missing, (missing == 1) ? \
                                     "names a file that is not there" : \
                                     "name files that are not there"))

    # the kinds in order of their names
    n = 0
    for (k in kinds)
        names[++n] = k
    for (k = 2; k <= n; k++) {
        held = names[k]
        for (t = k - 1; t >= 1 && names[t] > held; t--)
            names[t + 1] = names[t]
        names[t + 1] = held
    }
    line = ""
    for (k = 1; k <= n; k++)
        line = line (line != "" ? ", " : "") kinds[names[k]] " " names[k]
    if (!quiet) {
        printf "%s: %s", path, kind
        if (kind == "project")
            printf " %s", (vcxproj_property(p, "ProjectName") != "") ? \
                          vcxproj_property(p, "ProjectName") : "(unnamed)"
        if (toolsets != "")
            printf "; toolsets %s", toolsets
        if (configs != "")
            printf "; %s", configs
        if (line != "")
            printf "; %s", line
        if (imports != "")
            printf "; imports %s", imports
        printf "\n"
    }

    # a filters file is held to its project
    if (kind == "filters") {
        target = path
        sub(/\.filters$/, "", target)
        survey_filters(path, p, target)
    }

    vcxproj_close(p)
}

# survey_reference(path, dir, e, include): a reference, to a project that
# is there, by its own id.
function survey_reference(path, dir, e, include,    held, given, own) {
    if (include ~ /\$\(/)
        return
    if (!msvs_exists(joined(dir, include))) {
        problem(path, "refers to " include ", which is not there")
        return
    }
    for (held = vcxproj_first(e); held; held = vcxproj_next(held))
        if (vcxproj_name(held) == "Project") {
            given = tolower(vcxproj_text(held))
            gsub(/[{}]/, "", given)
            own = guid_of(joined(dir, include))
            if (own != "" && given != own)
                problem(path, "refers to " include " by another id")
        }
}

# survey_filters(path, f, project): a filters file's items, the project's
# own, each shown in a filter it defines.
function survey_filters(path, f, project,    p, e, n, defined, shown, mine, \
                        key, held, only_here, only_there) {
    for (e = vcxproj_find(f, "Filter"); e; e = vcxproj_find(f, "Filter", e))
        if (vcxproj_name(vcxproj_parent(e)) == "ItemGroup")
            defined[vcxproj_attr(e, "Include")] = 1
    for (e = vcxproj_find(f, ""); e; e = vcxproj_find(f, "", e)) {
        n = vcxproj_name(e)
        if (vcxproj_name(vcxproj_parent(e)) != "ItemGroup" || n == "Filter")
            continue
        shown[n SUBSEP tolower(vcxproj_attr(e, "Include"))] = 1
        held = 0
        for (key = vcxproj_first(e); key; key = vcxproj_next(key))
            if (vcxproj_name(key) == "Filter") {
                held = 1
                if (!(vcxproj_text(key) in defined))
                    problem(path, "shows " vcxproj_attr(e, "Include") \
                            " in " vcxproj_text(key) ", which it does not "\
                            "define")
            }
    }
    if (msvs_exists(project) != 1)
        return
    p = vcxproj_open(project)
    if (p < 0)
        return
    for (e = vcxproj_find(p, ""); e; e = vcxproj_find(p, "", e)) {
        n = vcxproj_name(e)
        if (vcxproj_name(vcxproj_parent(e)) == "ItemGroup" &&
            n ~ /^(ClCompile|ClInclude|None|Text|ResourceCompile)$/)
            mine[n SUBSEP tolower(vcxproj_attr(e, "Include"))] = 1
    }
    vcxproj_close(p)
    only_here = only_there = 0
    for (key in shown)
        if (!(key in mine))
            only_here++
    for (key in mine)
        if (!(key in shown))
            only_there++
    if (only_here)
        problem(path, items(only_here, "its project does not list"))
    if (only_there)
        problem(path, items(only_there, "of its project's not shown"))
}

# survey_solution(path): a solution's projects, folders and configurations.
function survey_solution(path,    s, dir, i, target, id, own, configs) {
    s = sln_open(path)
    if (s < 0) {
        unreadable(path, sln_error())
        return
    }
    dir = directory(path)
    configs = ""
    for (i = 1; i <= sln_configs(s); i++)
        configs = configs (i > 1 ? " " : "") sln_config(s, i)
    if (!quiet)
        printf "%s: %s, %d projects, %d folders; %s\n", path, sln_form(s),
               sln_projects(s), sln_folders(s),
               (configs != "") ? configs : "configurations from its projects"
    for (i = 1; i <= sln_projects(s); i++) {
        target = joined(dir, sln_project(s, i, "path"))
        if (msvs_exists(target) != 1) {
            problem(path, "lists " sln_project(s, i, "path") \
                    ", which is not there")
            continue
        }
        id = sln_project(s, i, "id")
        own = (msvs_kind(target) == "project") ? guid_of(target) : ""
        if (id != "" && own != "" && id != own)
            problem(path, "lists " sln_project(s, i, "path") \
                    " by another id")
    }
    sln_close(s)
}
